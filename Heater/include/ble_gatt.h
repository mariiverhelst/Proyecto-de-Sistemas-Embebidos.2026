#ifndef BLE_GATT_H
#define BLE_GATT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    float    temperature;
    float    setpoint;
    float    duty_cycle;
    float    kp;
    float    ki;
    float    kd;
    uint32_t rpm;
    uint8_t  error_flags;
    uint8_t  error_severity;
} ble_sensor_packet_t;

#define BLE_CMD_SET_TEMP 0x01
#define BLE_CMD_SET_KP 0x02
#define BLE_CMD_SET_KI 0x03
#define BLE_CMD_SET_KD 0x04
#define BLE_CMD_HIST_DOWNLOAD 0x10
#define BLE_CMD_HIST_RESET 0x11

typedef void (*ble_cmd_callback_t)(uint8_t cmd_type, float value);

void ble_gatt_init(ble_cmd_callback_t cmd_cb);
void ble_gatt_update_data(const ble_sensor_packet_t *data);
bool ble_gatt_is_connected(void);

#endif
