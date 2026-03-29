#include "pid_temp.h"

static float clampf(float x, float min_val, float max_val)
{
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

void pid_temp_init(pid_temp_t *pid)
{
    pid->kp = PID_KP_DEFAULT;
    pid->ki = PID_KI_DEFAULT;
    pid->kd = PID_KD_DEFAULT;
    pid->dt_s = PID_DT_S;

    pid->sp_true = PID_SP_TRUE_DEFAULT;
    pid->sp_ctrl = PID_SP_TRUE_DEFAULT;
    pid->ramp_rate_c_s = PID_RAMP_RATE_DEFAULT;

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;

    pid->boost_active = false;
    pid->boost_start_temp = 0.0f;

    pid->heater_lock_until_ms = 0;
}

void pid_temp_set_constants(pid_temp_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void pid_temp_set_setpoint(pid_temp_t *pid, float sp_true)
{
    pid->sp_true = sp_true;

    if (pid->ramp_rate_c_s <= 0.0f) {
        pid->sp_ctrl = sp_true;
    }
}

void pid_temp_set_ramp(pid_temp_t *pid, float ramp_rate_c_s)
{
    pid->ramp_rate_c_s = ramp_rate_c_s;

    if (ramp_rate_c_s <= 0.0f) {
        pid->sp_ctrl = pid->sp_true;
    }
}

void pid_temp_reset(pid_temp_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->boost_active = false;
    pid->boost_start_temp = 0.0f;
    pid->heater_lock_until_ms = 0;

    if (pid->ramp_rate_c_s <= 0.0f) {
        pid->sp_ctrl = pid->sp_true;
    }
}

float pid_temp_update(pid_temp_t *pid, float temp_actual_c, uint64_t now_ms)
{
    // Proteccion por sobretemperatura
    if (temp_actual_c >= (pid->sp_true + PID_OVERTEMP_DELTA)) {
        //============================================
        //esta variable "temp_actual_c es la temperatura actual del sistema
        //(osea la conversion de el voltaje a tem)"
        //=====================================================
        pid->heater_lock_until_ms = now_ms + PID_LOCK_TIME_MS;
        pid->integral = 0.0f;
        pid->prev_error = 0.0f;
        pid->boost_active = false;
        return 0.0f;
    }

    // Si sigue bloqueado, salida cero
    if (now_ms < pid->heater_lock_until_ms) {
        pid->integral = 0.0f;
        return 0.0f;
    }

    // Manejo de rampa
    if (pid->ramp_rate_c_s <= 0.0f) {
        pid->sp_ctrl = pid->sp_true;
    } else {
        if (pid->sp_ctrl < temp_actual_c) {
            pid->sp_ctrl = temp_actual_c;
        }

        if (pid->sp_ctrl < pid->sp_true) {
            pid->sp_ctrl += pid->ramp_rate_c_s * pid->dt_s;

            if (pid->sp_ctrl > pid->sp_true) {
                pid->sp_ctrl = pid->sp_true;
            }
        } else {
            pid->sp_ctrl = pid->sp_true;
        }
    }

    // Boost a maxima potencia
    if (!pid->boost_active && ((pid->sp_true - temp_actual_c) > PID_BOOST_DELTA_START)) {
        pid->boost_active = true;
        pid->boost_start_temp = temp_actual_c;
        pid->integral = 0.0f;
        pid->prev_error = 0.0f;
    }

    if (pid->boost_active) {
        if (temp_actual_c < (pid->boost_start_temp + PID_BOOST_DELTA_EXIT)) {
            return 100.0f;
        } else {
            pid->boost_active = false;
            pid->integral = 0.0f;
            pid->prev_error = pid->sp_ctrl - temp_actual_c;
        }
    }

    // PID normal
    float error = pid->sp_ctrl - temp_actual_c;
    // y aqui se usa directamente la temperatura 

    pid->integral += error * pid->dt_s;
    pid->integral = clampf(pid->integral, PID_INT_MIN, PID_INT_MAX);

    float derivative = (error - pid->prev_error) / pid->dt_s;
    derivative = clampf(derivative, PID_DERIV_MIN, PID_DERIV_MAX);

    float output = (pid->kp * error) +
                   (pid->ki * pid->integral) +
                   (pid->kd * derivative);

    output = clampf(output, PID_OUT_MIN, PID_OUT_MAX);

    pid->prev_error = error;

    return output;
}

float pid_temp_get_sp_ctrl(pid_temp_t *pid)
{
    return pid->sp_ctrl;
}

bool pid_temp_is_boost_active(pid_temp_t *pid)
{
    return pid->boost_active;
}

bool pid_temp_is_locked(pid_temp_t *pid, uint64_t now_ms)
{
    return (now_ms < pid->heater_lock_until_ms);
}