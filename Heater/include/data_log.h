#ifndef DATA_LOG_H
#define DATA_LOG_H

#include <stdint.h>

#define DATA_LOG_MAX 500
#define DATA_LOG_INTERVAL_US 58000000ULL

void data_log_init(void);
void data_log_add(float temp, uint32_t time_s);
int data_log_get_count(void);
float data_log_get_temp(int index);
uint32_t data_log_get_time(int index);
void data_log_reset(void);

#endif