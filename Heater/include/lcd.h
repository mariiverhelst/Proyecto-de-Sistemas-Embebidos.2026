#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t lcd_init(void);
void      lcd_clear(void);
void      lcd_set_cursor(uint8_t col, uint8_t row);
void      lcd_print(const char *s);