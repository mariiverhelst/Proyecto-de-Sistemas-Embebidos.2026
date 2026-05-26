#include "data_log.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "DATALOG";

static float    log_temp[DATA_LOG_MAX];
static uint32_t log_time[DATA_LOG_MAX];

static int log_head  = 0;
static int log_count = 0;

void data_log_init(void)
{
    memset(log_temp, 0, sizeof(log_temp));
    memset(log_time, 0, sizeof(log_time));
    log_head  = 0;
    log_count = 0;
    ESP_LOGI(TAG, "DataLog inicializado (%d entradas, ~%.0f s intervalo)",
             DATA_LOG_MAX, DATA_LOG_INTERVAL_US / 1000000.0);
}

void data_log_add(float temp, uint32_t time_s)
{
    log_temp[log_head] = temp;
    log_time[log_head] = time_s;
    log_head = (log_head + 1) % DATA_LOG_MAX;
    if (log_count < DATA_LOG_MAX) {
        log_count++;
    }
}

int data_log_get_count(void)
{
    return log_count;
}

float data_log_get_temp(int index)
{
    if (index < 0 || index >= log_count) return 0.0f;
    int real = (log_head - log_count + index + DATA_LOG_MAX) % DATA_LOG_MAX;
    return log_temp[real];
}

uint32_t data_log_get_time(int index)
{
    if (index < 0 || index >= log_count) return 0;
    int real = (log_head - log_count + index + DATA_LOG_MAX) % DATA_LOG_MAX;
    return log_time[real];
}

void data_log_reset(void)
{
    log_head  = 0;
    log_count = 0;
    ESP_LOGI(TAG, "Buffer reseteado (tiempo sigue corriendo)");
}