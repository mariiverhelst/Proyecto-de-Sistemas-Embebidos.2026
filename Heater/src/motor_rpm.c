#include "motor_rpm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

/* ======================== DEFINES ========================== */

// Pin de entrada (NE555 → NPN → pull-up 4.7kΩ a 3.3V)
#define PIN_PULSO       4

// Parámetros del motor (calibrado con tacómetro)
#define PASOS_POR_REV   200       // Motor 1.8°
#define MICROSTEPPING   8         // 1/8 (DIP S3=0, S4=1)
#define PULSOS_POR_REV  (PASOS_POR_REV * MICROSTEPPING)  // 1600

// Promedio móvil
#define MUESTRAS        40

// Timeout: si no hay pulsos en 2 segundos → motor parado
#define TIMEOUT_US      2000000

/* =================== VARIABLES GLOBALES ==================== */

// Timestamps (modificados en ISR)
static volatile int64_t tiempo_actual   = 0;
static volatile int64_t tiempo_anterior = 0;
static volatile int64_t periodo_us      = 0;

// Buffer para promedio móvil
static int64_t buffer_periodo[MUESTRAS] = {0};
static int     idx_buffer = 0;

// Resultados accesibles desde otros archivos
static uint32_t rpm_actual  = 0;
static uint32_t freq_actual = 0;

/* ======================== FUNCIONES ======================== */

/**
 *  captura el timestamp en cada flanco de BAJADA.
 */
static void IRAM_ATTR isr_flanco_bajada(void *arg)
{
    tiempo_actual = esp_timer_get_time();

    if (tiempo_anterior != 0) {
        periodo_us = tiempo_actual - tiempo_anterior;
    }

    tiempo_anterior = tiempo_actual;
    
}

/**
 * Calcula el promedio móvil del periodo.
 */
static int64_t promedio_periodo(int64_t nuevo_periodo)
{
    buffer_periodo[idx_buffer] = nuevo_periodo;
    idx_buffer = (idx_buffer + 1) % MUESTRAS;

    int64_t suma = 0;
    int count = 0;
    for (int i = 0; i < MUESTRAS; i++) {
        if (buffer_periodo[i] > 0) {
            suma += buffer_periodo[i];
            count++;
        }
    }

    if (count == 0) return 0;
    return suma / count;
}

/* =================== FUNCIONES PÚBLICAS ==================== */

void motor_rpm_init(void)
{
    // Configurar GPIO de entrada (flanco de bajada)
    gpio_config_t io_pulso = {
        .pin_bit_mask  = (1ULL << PIN_PULSO),
        .mode          = GPIO_MODE_INPUT,
        .pull_up_en    = GPIO_PULLUP_ENABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_pulso);

    // NO llamamos gpio_install_isr_service() aquí
    // porque main.c ya lo hizo para el zero cross

    // Solo registramos nuestro handler
    gpio_isr_handler_add(PIN_PULSO, isr_flanco_bajada, NULL);

    ESP_LOGI(TAG, "Motor RPM init OK (GPIO %d, %d pulsos/rev)",
             PIN_PULSO, PULSOS_POR_REV);
}

void motor_rpm_update(void)
{
    // Verificar timeout (motor parado)
    int64_t ahora = esp_timer_get_time();
    int64_t ultimo_pulso = tiempo_actual;

    if ((ahora - ultimo_pulso) > TIMEOUT_US || periodo_us == 0) {
        rpm_actual  = 0;
        freq_actual = 0;
        return;
    }

    // Calcular promedio del periodo
    int64_t periodo_prom = promedio_periodo(periodo_us);

    if (periodo_prom <= 0) {
        rpm_actual  = 0;
        freq_actual = 0;
        return;
    }

    // Frecuencia y RPM
    freq_actual = (uint32_t)(1000000 / periodo_prom);
    rpm_actual  = (freq_actual * 60) / PULSOS_POR_REV;
}

uint32_t motor_rpm_get_rpm(void)
{
    return rpm_actual;
}

uint32_t motor_rpm_get_freq(void)
{
    return freq_actual;
}