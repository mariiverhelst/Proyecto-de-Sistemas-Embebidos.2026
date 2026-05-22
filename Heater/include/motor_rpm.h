/**
 * ============================================================
 *  MOTOR RPM — Header
 *  Medidor de RPM para motor paso a paso
 *  NE555 → NPN (2N3904) → ESP32 (flanco de bajada)
 * ============================================================
 */

#ifndef MOTOR_RPM_H
#define MOTOR_RPM_H

#include <stdint.h>

/**
 * Inicializa el medidor de RPM.
 * Configura el GPIO y la interrupción.
 * IMPORTANTE: gpio_install_isr_service() debe haberse
 * llamado ANTES desde main.c
 */
void motor_rpm_init(void);

/**
 * Actualiza la lectura de RPM.
 * Llamar periódicamente desde el loop principal (~cada 500 ms).
 * Calcula frecuencia y RPM a partir del periodo medido.
 */
void motor_rpm_update(void);

/**
 * Devuelve las RPM actuales (para usar en displays u otros archivos).
 * Retorna 0 si el motor está parado o no hay pulsos.
 */
uint32_t motor_rpm_get_rpm(void);

/**
 * Devuelve la frecuencia actual en Hz.
 */
uint32_t motor_rpm_get_freq(void);

#endif // MOTOR_RPM_H