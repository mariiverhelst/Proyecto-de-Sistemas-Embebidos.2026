#pragma once
#include <stdint.h>
#include "esp_err.h"
// Inicialización del ADC PT100 (lo que ahora tienes en app_main de ADC.c)
esp_err_t pt100_init(void);

// Función que se llamará periódicamente (cada 1 ms)
void pt100_measurement(void);
int pt100_get_voltage_mv(void);
uint32_t pt_get_adc(void);