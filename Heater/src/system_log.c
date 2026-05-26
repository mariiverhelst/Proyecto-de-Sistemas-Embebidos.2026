#include "system_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static const char *TAG = "SYSLOG";

static log_entry_t       log_buffer[LOG_MAX_ENTRIES];
static int               log_head  = 0;
static int               log_count = 0;
static SemaphoreHandle_t log_mutex = NULL;

static const char* level_str[] = { "INFO", "WARN", "ERROR" };

void system_log_init(void)
{
    log_mutex = xSemaphoreCreateMutex();
    configASSERT(log_mutex != NULL);

    memset(log_buffer, 0, sizeof(log_buffer));
    log_head  = 0;
    log_count = 0;

    ESP_LOGI(TAG, "Log inicializado (%d entradas max)", LOG_MAX_ENTRIES);
}

void system_log_add(log_level_t level, uint64_t now_ms, const char *fmt, ...)
{
    if (log_mutex == NULL) return;

    char tmp[LOG_MSG_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);

    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        log_entry_t *entry = &log_buffer[log_head];
        entry->timestamp_ms = now_ms;
        entry->level        = level;
        strncpy(entry->message, tmp, LOG_MSG_LEN - 1);
        entry->message[LOG_MSG_LEN - 1] = '\0';

        log_head = (log_head + 1) % LOG_MAX_ENTRIES;
        if (log_count < LOG_MAX_ENTRIES) {
            log_count++;
        }

        xSemaphoreGive(log_mutex);
    }

    // También imprimir por serial
    uint32_t seg = (uint32_t)(now_ms / 1000);
    uint32_t min = seg / 60;
    seg = seg % 60;

    switch (level) {
        case LOG_LEVEL_INFO:
            ESP_LOGI(TAG, "[%02lu:%02lu] %s: %s",
                     (unsigned long)min, (unsigned long)seg, level_str[level], tmp);
            break;
        case LOG_LEVEL_WARN:
            ESP_LOGW(TAG, "[%02lu:%02lu] %s: %s",
                     (unsigned long)min, (unsigned long)seg, level_str[level], tmp);
            break;
        case LOG_LEVEL_ERROR:
            ESP_LOGE(TAG, "[%02lu:%02lu] %s: %s",
                     (unsigned long)min, (unsigned long)seg, level_str[level], tmp);
            break;
    }
}

const log_entry_t* system_log_get_entry(int index)
{
    if (log_mutex == NULL || index < 0 || index >= log_count) {
        return NULL;
    }

    const log_entry_t *result = NULL;

    if (xSemaphoreTake(log_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        int real_idx = (log_head - log_count + index + LOG_MAX_ENTRIES) % LOG_MAX_ENTRIES;
        result = &log_buffer[real_idx];
        xSemaphoreGive(log_mutex);
    }

    return result;
}

int system_log_get_count(void)
{
    return log_count;
}

const log_entry_t* system_log_get_latest(void)
{
    if (log_count == 0) return NULL;
    return system_log_get_entry(log_count - 1);
}