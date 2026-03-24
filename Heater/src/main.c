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
///////////////////////////////////////////////////////////////////////////
// el Pin 21 y 22 NO SE PUEDEN USAR PARA OTRA COSA, SON PARA EL LCD////////
// Pin 32 es el pin de sensado del voltage de la PT100/////////////
//////////////////////////////////////////////////////////////////////////
#define PWM 13
#define ZC 26
#define periodo 1000000



static uint64_t now = 0;
static uint64_t last, last_adc, last_prnt = 0;
static uint64_t pwm_on = 0;

//static volatile bool triac_enable = false;



void IRAM_ATTR zero_cross(void* arg){
    //gpio_set_level(PWM,triac_enable);
    if (now-last<= pwm_on){
            gpio_set_level(PWM,1);
    }
    else gpio_set_level(PWM,0); 
       
}


void calc_pwm(uint16_t duty){
    if (duty<100){
        pwm_on= (duty*periodo)/100;
    }    
    else pwm_on=periodo-1;
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


    while(1){
        timer_get_counter_value(TIMER_GROUP_0,TIMER_0,&now);


        if (now-last_adc>=1000){
            pt100_measurement();
            last_adc=now;
        }
        
       
         // duty cycle del 50%
        if (now-last>=periodo){
            last=now;
        }
        
        if (now - last_prnt >= 500000) {
            int v = pt100_get_voltage_mv();
            printf("PT100 = %d mV\n", v);
            last_prnt = now;
        }
        
    }






}