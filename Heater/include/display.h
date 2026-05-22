#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

void display_init(void);
void display_show(float temp, bool mostrar_temp, uint32_t rpm);

#endif