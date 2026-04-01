#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include <inttypes.h>
#include "ADC.h"

static const char *TAG = "PT100";
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t cali_handle;

#define PT100_ADC_CHANNEL   ADC_CHANNEL_4 

static uint64_t acumulado = 0;
static uint32_t divisor   = 0;
static uint32_t promedio_raw= 0;
static int      voltage_pt100_mv = 0;

esp_err_t pt100_init(void){
    
    // Configurar unidad ADC1
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %d", ret);
        return ret;
    }

    // Configurar canal
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_0,
    };
    ret = adc_oneshot_config_channel(adc1_handle, PT100_ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %d", ret);
        return ret;
    }

    // Calibración
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id  = ADC_UNIT_1,
        .atten    = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "adc_cali_create_scheme_line_fitting failed: %d", ret);
        return ret;
    }

    // Lectura de descarte
    int raw = 0;
    adc_oneshot_read(adc1_handle, PT100_ADC_CHANNEL, &raw);
    return ESP_OK; 
    // Ejemplo: llamar a pt100_measurement cada 1 ms desde una tarea
}
// Llamar cada 1000 us (1 ms). Cada llamada toma 16 muestras.
// 16 muestras x 500 llamadas ≈ 8000 muestras ≈ 500 ms.
void pt100_measurement(void)
{
    int raw = 0;

    // Lectura de descarte por llamada (opcional)
    adc_oneshot_read(adc1_handle, PT100_ADC_CHANNEL, &raw);

    for (int i = 0; i < 50; i++) {
        if (adc_oneshot_read(adc1_handle, PT100_ADC_CHANNEL, &raw) == ESP_OK) {
            acumulado += (uint64_t)raw;
            divisor++;
        }
    }

    if (divisor >= 33333) {   // 8000 muestras acumuladas
        promedio_raw = (uint32_t)(acumulado / divisor);

        int voltage_mv = 0;
        if (adc_cali_raw_to_voltage(cali_handle, promedio_raw, &voltage_mv) == ESP_OK) {
            voltage_pt100_mv = voltage_mv;  // variable global o pásala a donde necesites
            ESP_LOGI(TAG, "PT100: raw_avg=%" PRIu32 ", V=%d mV", promedio_raw, voltage_pt100_mv);
        }
        

        // Reset acumuladores
        acumulado = 0;
        divisor   = 0;
    }
    
}
// devuelve el voltage ajustado de la funcion
int pt100_get_voltage_mv(void){
    return voltage_pt100_mv;
}
// devuelve el valor en bits del ADC, este es mejor para la curva de calibracion del sensor
uint32_t pt_get_adc(void){
    return promedio_raw;
}
/// 23.3 0.478
