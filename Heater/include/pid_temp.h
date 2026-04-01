#ifndef PID_TEMP_H
#define PID_TEMP_H
//definimos variables 
#include <stdint.h>
#include <stdbool.h>
// ===============================
// CONSTANTES variables 
// ===============================
// Parametros PID iniciales
#define PID_KP_DEFAULT              10.341f
#define PID_KI_DEFAULT              1.25f
#define PID_KD_DEFAULT              0.0277f
// Tiempo de muestreo del control
#define PID_DT_S                    8.0f // 8000ms
// Setpoint verdadero por defecto
#define PID_SP_TRUE_DEFAULT         37.0f
// Rampa por defecto
// Si vale 0, no hay rampa
#define PID_RAMP_RATE_DEFAULT       0.0f
// Si la diferencia contra el setpoint verdadero es mayor a 5,
// entra en boost de maxima potencia
#define PID_BOOST_DELTA_START       7.0f
// Sale del boost cuando haya subido esta cantidad de grados
#define PID_BOOST_DELTA_EXIT        3.0f
// Si la temperatura supera sp_true + 3, apaga 10 minutos
#define PID_OVERTEMP_DELTA          2.5f
#define PID_LOCK_TIME_MS            600000ULL
// Limites de integral
#define PID_INT_MIN                -15.0f
#define PID_INT_MAX                 70.0f
// Limites de derivada
#define PID_DERIV_MIN              -2.0f
#define PID_DERIV_MAX               2.0f
// Limites de salida
#define PID_OUT_MIN                 0.0f
#define PID_OUT_MAX                 100.0f

typedef struct
{
    float kp;
    float ki;
    float kd;
    float dt_s;

    float sp_true;
    float sp_ctrl;
    float ramp_rate_c_s;

    float integral;
    float prev_error;

    bool boost_active;
    float boost_start_temp;

    uint64_t heater_lock_until_ms;

} pid_temp_t;

void pid_temp_init(pid_temp_t *pid);
void pid_temp_set_constants(pid_temp_t *pid, float kp, float ki, float kd);
void pid_temp_set_setpoint(pid_temp_t *pid, float sp_true);
void pid_temp_set_ramp(pid_temp_t *pid, float ramp_rate_c_s);
void pid_temp_reset(pid_temp_t *pid);

float pid_temp_update(pid_temp_t *pid, float temp_actual_c, uint64_t now_ms);

float pid_temp_get_sp_ctrl(pid_temp_t *pid);
bool pid_temp_is_boost_active(pid_temp_t *pid);
bool pid_temp_is_locked(pid_temp_t *pid, uint64_t now_ms);

#endif // PID_TEMP_H se cierra la condicion 