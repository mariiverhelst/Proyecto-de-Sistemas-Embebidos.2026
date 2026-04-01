#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "esp_task_wdt.h"
#include "lcd.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "ADC.h"
#include <stdio.h>
#include <inttypes.h>
#include "pid_temp.h"
///////////////////////////////////////////////////////////////////////////
// el Pin 21 y 22 NO SE PUEDEN USAR PARA OTRA COSA, SON PARA EL LCD////////
// Pin 32 es el pin de sensado del voltage de la PT100/////////////
//////////////////////////////////////////////////////////////////////////
#define PWM 13
#define ZC 26
#define periodo 8000000




static volatile uint64_t now = 0;
static volatile uint64_t last=0;
static uint64_t last_adc, last_prnt,last_graf, last_log5 = 0;
static volatile uint64_t pwm_on = 0;
float temp_actual_c =0.0f;
float duty_cycle = 0.0f;
static int   graf_idx  = 0;

static pid_temp_t pid;
static float graf[240] = {0.0f};
//static volatile bool triac_enable = false;



void IRAM_ATTR zero_cross(void* arg){
    //gpio_set_level(PWM,triac_enable);
    if (now-last<= pwm_on){
            gpio_set_level(PWM,1);
    }
    else gpio_set_level(PWM,0); 
       
}


void calc_pwm(float duty){
    if (duty<0) duty=0;
    if (duty<100){
        pwm_on= (duty*periodo)/100;
    }    
    else pwm_on=periodo;
}
void app_main() {


    gpio_config_t io_config =
    {
        .pin_bit_mask = (1ULL << ZC),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io_config);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ZC, zero_cross, NULL);




    gpio_config_t out_cfg =
    {
        .pin_bit_mask = (1ULL << PWM),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_cfg);


    timer_config_t timer_config = {
        .divider=80,
        .counter_dir=TIMER_COUNT_UP,
        .alarm_en=TIMER_ALARM_DIS,
        .auto_reload=false
    };
    timer_init(TIMER_GROUP_0,TIMER_0,&timer_config);
    timer_set_counter_value(TIMER_GROUP_0,TIMER_0,0);
    timer_start(TIMER_GROUP_0,TIMER_0);
    lcd_init();
    lcd_clear();
    if (pt100_init() != ESP_OK) {
        lcd_set_cursor(0, 1);
        lcd_print("Error PT100");
    }
    lcd_set_cursor(0, 0);
    lcd_print("Hola lindas");
    pid_temp_init(&pid);


    while(1){
        timer_get_counter_value(TIMER_GROUP_0,TIMER_0,&now);


        if (now-last_adc>=100){
            pt100_measurement();
            last_adc=now;
        }
        if (graf_idx < 240 && (now - last_graf) >= 30000000ULL) {   // 30 s = 30 000 000 us
            uint32_t adc_raw = pt_get_adc();
            temp_actual_c = 0.01479f * adc_raw + 9.11f;
            graf[graf_idx] = temp_actual_c;     // o la variable de temperatura que quieras registrar
            graf_idx++;
            last_graf = now;
        }
       
         // 
        if (now-last>=periodo){
            uint32_t adc_raw = pt_get_adc();
            temp_actual_c = 0.01479f * adc_raw + 9.11f;
            duty_cycle = pid_temp_update(&pid, temp_actual_c,now/1000);
            calc_pwm((float) duty_cycle);
            last=now;
        }
        
        if ((now - last_log5) >= 300000000ULL) {
            printf("GRAF_START t=%.1f min, graf_idx=%d\n",
                 now / 60000000.0f, graf_idx);

            for (int i = 0; i < graf_idx; i++) {
             // tiempo relativo de cada muestra (0.5 min por punto)
             float t_min = i * 0.5f;
             printf("%5.1f, %.3f\n", t_min, graf[i]);
            }

            printf("GRAF_END\n\n");
            last_log5 = now;
        }


        if (now - last_prnt >= 1000000) {
            uint32_t adc_raw = pt_get_adc();
            temp_actual_c = 0.01479f * adc_raw + 9.11f;

            float error   = pid.prev_error;
            float ki_term = pid.ki * pid.integral;
            float kp_term = pid.kp * error;
            float sp_ctrl = pid_temp_get_sp_ctrl(&pid);

            printf("ADC=%" PRIu32
                "  T_true=%.2f C  T_ctrl=%.2f C"
                "  err=%.2f  Kp=%.3f  KiTerm=%.2f  duty=%.1f %%\n",
                adc_raw,
                temp_actual_c,   // T_true
                sp_ctrl,         // T_ctrl
                error,
                kp_term,
                ki_term,
                duty_cycle);

            last_prnt = now;
        }
        
    }
}
//21.6 807     21.4 831
// 35 1645      35.4 1771