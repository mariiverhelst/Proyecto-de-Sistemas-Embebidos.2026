#ifndef ERROR_FLAGS_H
#define ERROR_FLAGS_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
//  BITS DE ERROR — cada bit = una condición
//  Severidad: INFO(0), WARNING(1), CRITICAL(2)
// ============================================================

// ── INFO (severidad 0) ──
#define ERR_INFO_BOOST_ACTIVE   (1 << 0)
#define ERR_INFO_RAMP_ACTIVE    (1 << 1)
#define ERR_INFO_HEATER_LOCKED  (1 << 2)

// ── WARNING (severidad 1) ──
#define ERR_WARN_TEMP_HIGH      (1 << 3)
#define ERR_WARN_TEMP_LOW       (1 << 4)
#define ERR_WARN_RPM_ZERO       (1 << 5)

// ── CRITICAL (severidad 2) ──
#define ERR_CRIT_SENSOR_FAIL    (1 << 6)
#define ERR_CRIT_OVERTEMP       (1 << 7)

// ── Máscaras por severidad ──
#define ERR_MASK_INFO     (ERR_INFO_BOOST_ACTIVE | ERR_INFO_RAMP_ACTIVE | ERR_INFO_HEATER_LOCKED)
#define ERR_MASK_WARNING  (ERR_WARN_TEMP_HIGH | ERR_WARN_TEMP_LOW | ERR_WARN_RPM_ZERO)
#define ERR_MASK_CRITICAL (ERR_CRIT_SENSOR_FAIL | ERR_CRIT_OVERTEMP)

// ── Estructura principal ──
typedef struct {
    uint8_t  flags;
    uint8_t  severity;      // 0=info, 1=warn, 2=critical
    uint8_t  prev_flags;    // para detectar cambios
} error_state_t;

void error_flags_init(error_state_t *err);

void error_flags_update(error_state_t *err,
                         float temp, float setpoint,
                         uint32_t rpm, uint32_t adc_raw,
                         bool boost, bool locked,
                         float ramp_rate, float sp_ctrl);

bool error_flags_changed(const error_state_t *err);

const char* error_flag_to_string(uint8_t single_flag);

#endif