#include "error_flags.h"

// ── Umbrales de detección ──
#define ADC_RAW_MIN          30      // debajo → sensor desconectado
#define ADC_RAW_MAX          3900    // encima → sensor dañado
#define TEMP_HIGH_DELTA      1.5f    // warn si temp > sp + 1.5
#define TEMP_LOW_DELTA       5.0f    // warn si temp < sp - 5.0

void error_flags_init(error_state_t *err)
{
    err->flags      = 0;
    err->severity   = 0;
    err->prev_flags = 0;
}

void error_flags_update(error_state_t *err,
                         float temp, float setpoint,
                         uint32_t rpm, uint32_t adc_raw,
                         bool boost, bool locked,
                         float ramp_rate, float sp_ctrl)
{
    err->prev_flags = err->flags;

    uint8_t f = 0;
    uint8_t sev = 0;

    // ══════════════ CRITICAL (2) ══════════════

    if (adc_raw < ADC_RAW_MIN || adc_raw > ADC_RAW_MAX) {
        f   |= ERR_CRIT_SENSOR_FAIL;
        sev  = 2;
    }

    if (locked) {
        f   |= ERR_CRIT_OVERTEMP;
        sev  = 2;
    }

    // ══════════════ WARNING (1) ══════════════

    // Solo evaluar si el sensor funciona
    if (!(f & ERR_CRIT_SENSOR_FAIL)) {
        if (temp > (setpoint + TEMP_HIGH_DELTA)) {
            f |= ERR_WARN_TEMP_HIGH;
            if (sev < 1) sev = 1;
        }

        if (temp < (setpoint - TEMP_LOW_DELTA) && !boost) {
            f |= ERR_WARN_TEMP_LOW;
            if (sev < 1) sev = 1;
        }
    }

    if (rpm == 0) {
        f |= ERR_WARN_RPM_ZERO;
        if (sev < 1) sev = 1;
    }

    // ══════════════ INFO (0) ══════════════

    if (boost) {
        f |= ERR_INFO_BOOST_ACTIVE;
    }

    if (ramp_rate > 0.0f && sp_ctrl < setpoint) {
        f |= ERR_INFO_RAMP_ACTIVE;
    }

    if (locked) {
        f |= ERR_INFO_HEATER_LOCKED;
    }

    err->flags    = f;
    err->severity = sev;
}

bool error_flags_changed(const error_state_t *err)
{
    return (err->flags != err->prev_flags);
}

const char* error_flag_to_string(uint8_t single_flag)
{
    switch (single_flag) {
        case ERR_INFO_BOOST_ACTIVE:   return "INFO: Boost activo";
        case ERR_INFO_RAMP_ACTIVE:    return "INFO: Rampa activa";
        case ERR_INFO_HEATER_LOCKED:  return "INFO: Heater bloqueado";
        case ERR_WARN_TEMP_HIGH:      return "WARN: Temperatura alta";
        case ERR_WARN_TEMP_LOW:       return "WARN: Temperatura baja";
        case ERR_WARN_RPM_ZERO:       return "WARN: Motor sin RPM";
        case ERR_CRIT_SENSOR_FAIL:    return "CRIT: Sensor PT100 fallo";
        case ERR_CRIT_OVERTEMP:       return "CRIT: Sobretemperatura";
        default:                      return "DESCONOCIDO";
    }
}