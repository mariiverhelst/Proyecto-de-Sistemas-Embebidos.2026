#ifndef SYSTEM_LOG_H
#define SYSTEM_LOG_H

#include <stdint.h>

// ============================================================
//  SYSTEM LOG — Buffer circular en RAM
//  Guarda los últimos 64 eventos con timestamp y severidad.
//  Thread-safe con mutex de FreeRTOS.
// ============================================================

#define LOG_MAX_ENTRIES   64
#define LOG_MSG_LEN       80

typedef enum {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_ERROR = 2
} log_level_t;

typedef struct {
    uint64_t    timestamp_ms;
    log_level_t level;
    char        message[LOG_MSG_LEN];
} log_entry_t;

void system_log_init(void);

void system_log_add(log_level_t level, uint64_t now_ms, const char *fmt, ...);

const log_entry_t* system_log_get_entry(int index);

int system_log_get_count(void);

const log_entry_t* system_log_get_latest(void);

#endif