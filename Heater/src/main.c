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
#include "motor_rpm.h"
#include "display.h"
#include "esp_timer.h"



///////////////////////////////////////////////////////////////////////////
// el Pin 21 y 22 NO SE PUEDEN USAR PARA OTRA COSA, SON PARA EL LCD////////
// Pin 32 es el pin de sensado del voltage de la PT100/////////////
// Pin 4 es el pin de lectura de RPM del motor (NPN desde NE555)///
//////////////////////////////////////////////////////////////////////////

#define PWM 13
#define ZC 26
#define BTN 23   // ✅ BOTÓN

#define BTN_UP      15   
#define BTN_DOWN    19
#define BTN_ENTER   5

#define periodo 8000000

static volatile uint64_t now = 0;
static volatile uint64_t last=0;
static uint64_t last_adc, last_prnt,last_graf, last_log5,last_rpm = 0;
static volatile uint64_t pwm_on = 0;

float temp_actual_c =0.0f;
float duty_cycle = 0.0f;
static int graf_idx = 0;
static pid_temp_t pid;
static float graf[240] = {0.0f};


// ★ MENÚ — máquina de estados y dígitos
typedef enum {
    SET_DECENAS, SET_UNIDADES, SET_DECIMAL,
    KP_MILES, KP_CENTENAS, KP_DECENAS, KP_UNIDADES, KP_DECIMAL,
    KI_CENTENAS, KI_DECENAS, KI_UNIDADES, KI_DECIMAL1, KI_DECIMAL2,
    FINAL
} estado_t;

static estado_t estado = SET_DECENAS;
static int t_dec = 3, t_uni = 7, t_dp = 0;
static int kp_mil = 0, kp_cen = 0, kp_dec = 3, kp_uni = 0, kp_dp = 0;
static int ki_cen = 0, ki_dec = 0, ki_uni = 1, ki_dp1 = 0, ki_dp2 = 0;
static bool lastUp = false, lastDown = false, lastEnter = false;
static uint32_t lastBlink = 0;
static bool cursorOn = true;
#define BLINK_MS 300

// MENÚ — funciones auxiliares
static inline uint32_t millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static float getTemp(void) {
    return t_dec * 10.0f + t_uni + t_dp * 0.1f;
}

static float getKP(void) {
    return kp_mil * 1000.0f + kp_cen * 100.0f +
           kp_dec * 10.0f   + kp_uni + kp_dp * 0.1f;
}

static float getKI(void) {
    return ki_cen * 100.0f + ki_dec * 10.0f + ki_uni +
           ki_dp1 * 0.1f   + ki_dp2 * 0.01f;
}

static int* get_digito_activo(void) {
    switch (estado) {
        case SET_DECENAS:  return &t_dec;
        case SET_UNIDADES: return &t_uni;
        case SET_DECIMAL:  return &t_dp;
        case KP_MILES:     return &kp_mil;
        case KP_CENTENAS:  return &kp_cen;
        case KP_DECENAS:   return &kp_dec;
        case KP_UNIDADES:  return &kp_uni;
        case KP_DECIMAL:   return &kp_dp;
        case KI_CENTENAS:  return &ki_cen;
        case KI_DECENAS:   return &ki_dec;
        case KI_UNIDADES:  return &ki_uni;
        case KI_DECIMAL1:  return &ki_dp1;
        case KI_DECIMAL2:  return &ki_dp2;
        default:           return NULL;
    }
}

static void mostrar_setup(void) {
    char cd  = (estado == SET_DECENAS  && cursorOn) ? '_' : ('0' + t_dec);
    char cu  = (estado == SET_UNIDADES && cursorOn) ? '_' : ('0' + t_uni);
    char cdp = (estado == SET_DECIMAL  && cursorOn) ? '_' : ('0' + t_dp);
    char km  = (estado == KP_MILES     && cursorOn) ? '_' : ('0' + kp_mil);
    char kc  = (estado == KP_CENTENAS  && cursorOn) ? '_' : ('0' + kp_cen);
    char kd  = (estado == KP_DECENAS   && cursorOn) ? '_' : ('0' + kp_dec);
    char ku  = (estado == KP_UNIDADES  && cursorOn) ? '_' : ('0' + kp_uni);
    char kdp = (estado == KP_DECIMAL   && cursorOn) ? '_' : ('0' + kp_dp);
    char ic  = (estado == KI_CENTENAS  && cursorOn) ? '_' : ('0' + ki_cen);
    char id  = (estado == KI_DECENAS   && cursorOn) ? '_' : ('0' + ki_dec);
    char iu  = (estado == KI_UNIDADES  && cursorOn) ? '_' : ('0' + ki_uni);
    char id1 = (estado == KI_DECIMAL1  && cursorOn) ? '_' : ('0' + ki_dp1);
    char id2 = (estado == KI_DECIMAL2  && cursorOn) ? '_' : ('0' + ki_dp2);

    lcd_set_cursor(0, 0);
    lcd_print("T:");
    lcd_print_char(cd);  lcd_print_char(cu);
    lcd_print_char('.'); lcd_print_char(cdp);
    lcd_print(" KP:");
    lcd_print_char(km);  lcd_print_char(kc);
    lcd_print_char(kd);  lcd_print_char(ku);
    lcd_print_char('.'); lcd_print_char(kdp);

    lcd_set_cursor(0, 1);
    lcd_print("KI:");
    lcd_print_char(ic);  lcd_print_char(id);
    lcd_print_char(iu);
    lcd_print_char('.'); lcd_print_char(id1);
    lcd_print_char(id2);

    if (estado == FINAL) lcd_print(" OK!");
    else lcd_print("     ");
}
// ★ MENÚ — fin funciones auxiliares


// ✅ modo display
bool mostrar_temp = true;

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

    motor_rpm_init();    // ← MOTOR (2/3)

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
    lcd_print("Hola Usuario!");

    pid_temp_init(&pid);

    // ✅ INICIALIZAR DISPLAY
    display_init();

    // ✅ CONFIGURAR BOTÓN
    gpio_config_t btn1_cfg =
    {
        .pin_bit_mask = (1ULL << BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&btn1_cfg);
    int last_btn = 1;

        // ════════════════════════════════════════════
    // ★ MENÚ — configurar botones y correr menú
    // ════════════════════════════════════════════
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BTN_UP) | (1ULL << BTN_DOWN) | (1ULL << BTN_ENTER),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);

    vTaskDelay(pdMS_TO_TICKS(1500));   // splash "Hola lindas"
    lcd_clear();
    mostrar_setup();

    while (estado != FINAL) {
        if (millis() - lastBlink >= BLINK_MS) {
            cursorOn = !cursorOn;
            lastBlink = millis();
            if (estado != FINAL) mostrar_setup();
        }

        bool up    = (gpio_get_level(BTN_UP)    == 0);
        bool down  = (gpio_get_level(BTN_DOWN)  == 0);
        bool enter = (gpio_get_level(BTN_ENTER) == 0);

        if (up && !lastUp && estado != FINAL) {
            int *d = get_digito_activo();
            if (d) *d = (*d + 1) % 10;
            cursorOn = false;
            mostrar_setup();
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        lastUp = up;

        if (down && !lastDown && estado != FINAL) {
            int *d = get_digito_activo();
            if (d) *d = (*d + 9) % 10;
            cursorOn = false;
            mostrar_setup();
            vTaskDelay(pdMS_TO_TICKS(150));
        }
        lastDown = down;

        if (enter && !lastEnter) {
            if (estado == SET_DECIMAL) {
                estado = KP_MILES;
            } else if (estado == KP_DECIMAL) {
                estado = KI_CENTENAS;
            } else if (estado == KI_DECIMAL2) {
                estado = FINAL;
                printf("══════════════════════════════\n");
                printf("  CONFIGURACION COMPLETA:\n");
                printf("  Temp = %.1f C\n", getTemp());
                printf("  KP   = %.1f\n",   getKP());
                printf("  KI   = %.2f\n",   getKI());
                printf("══════════════════════════════\n");
            } else {
                estado = (estado_t)((int)estado + 1);
            }
            cursorOn = false;
            mostrar_setup();
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        lastEnter = enter;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Aplicar valores configurados al PID
    pid_temp_set_setpoint(&pid, getTemp());
    pid_temp_set_constants(&pid, getKP(), getKI(), pid.kd);

    
    
    // ════════════════════════════════════════════
    // ★ MENÚ — fin
    // ════════════════════════════════════════════



    while(1){

        timer_get_counter_value(TIMER_GROUP_0,TIMER_0,&now);
          
       

        // ✅ LECTURA BOTÓN
        int btn = gpio_get_level(BTN);
        if (last_btn == 1 && btn == 0) {
            mostrar_temp = !mostrar_temp;

            // anti-rebote simple
            for(volatile int i=0;i<100000;i++);
        }
        last_btn = btn;

        if (now-last_adc>=100){
            pt100_measurement();
            
            last_adc=now;
        }
        if (now - last_rpm >= 1500) {
            motor_rpm_update();
            last_rpm = now;
        }

        if (graf_idx < 240 && (now - last_graf) >= 30000000ULL) { // 30 s = 30 000 000 us
            uint32_t adc_raw = pt_get_adc();
            temp_actual_c = 0.01479f * adc_raw + 4.70f;
            graf[graf_idx] = temp_actual_c;
            graf_idx++;
            last_graf = now;
        }

        //
        if (now-last>=periodo){
            uint32_t adc_raw = pt_get_adc();
            temp_actual_c = 0.01479f * adc_raw + 4.70f;
            duty_cycle = pid_temp_update(&pid, temp_actual_c,now/1000);
            calc_pwm((float) duty_cycle);
            last=now;
        }

        if ((now - last_log5) >= 300000000ULL) {
            printf("GRAF_START t=%.1f min, graf_idx=%d\n",
                   now / 60000000.0f, graf_idx);
            for (int i = 0; i < graf_idx; i++) {
                float t_min = i * 0.5f;
                printf("%5.1f, %.3f\n", t_min, graf[i]);
            }
            printf("GRAF_END\n\n");
            last_log5 = now;
        }

        if (now - last_prnt >= 500000) {
            uint32_t adc_raw = pt_get_adc();
            temp_actual_c = 0.01479f * adc_raw + 4.70f;
            float error = pid.prev_error;
            float ki_term = pid.ki * pid.integral;
            float kp_term = pid.kp * error;
            float sp_ctrl = pid_temp_get_sp_ctrl(&pid);

            printf("ADC=%" PRIu32
                   " T_true=%.2f C T_ctrl=%.2f C"
                   " err=%.2f Kp=%.3f KiTerm=%.2f duty=%.1f %%\n",
                   adc_raw,
                   temp_actual_c,
                   sp_ctrl,
                   error,
                   kp_term,
                   ki_term,
                   duty_cycle);

            // ← MOTOR (3/3): actualizar y mostrar RPM
            
            uint32_t rpm = motor_rpm_get_rpm();
            // ✅ DEBUG RPM (ver si el modulo está calculando algo)
             printf("RPM DEBUG: %lu\n", (unsigned long)rpm);

            printf("Motor: %lu Hz | %lu RPM\n",
                   (unsigned long)motor_rpm_get_freq(),
                   (unsigned long)rpm);

            // ✅ DISPLAY (aquí se actualiza)
            display_show(temp_actual_c, mostrar_temp, rpm);

            last_prnt = now;
        }
    }
}

//21.6 807 21.4 831
// 35 1645 35.4 1771
