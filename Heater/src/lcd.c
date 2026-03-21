
#include "lcd.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_rom_sys.h"


// Declaración de la función ROM antigua para que el compilador no se queje
void ets_delay_us(uint32_t us);

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

// Cambia esta dirección si tu scanner dice otra (0x27, 0x3F, etc.)
#define PCF8574_ADDR                0x20

static const char *TAG = "LCD1602";

// Mapeo P0–P7 -> LCD, según tu cableado:
//
// P0 -> RS
// P1 -> RW
// P2 -> EN
// P3 -> VO (contraste)  <-- lo mantenemos a 1 o 0 fijo
// P4 -> D4
// P5 -> D5
// P6 -> D6
// P7 -> D7
//
#define PIN_RS  (1 << 0)
#define PIN_RW  (1 << 1)
#define PIN_EN  (1 << 2)
#define PIN_VO  (1 << 3)
#define PIN_D4  (1 << 4)
#define PIN_D5  (1 << 5)
#define PIN_D6  (1 << 6)
#define PIN_D7  (1 << 7)

// pon PIN_VO si quieres VO alto, o 0 si lo quieres bajo.
// prueba qué nivel te da mejor contraste.
static uint8_t contrast_level = 0;

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t pcf8574_write(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCF8574_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data | contrast_level, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void lcd_pulse_enable(uint8_t data)
{
    pcf8574_write(data | PIN_EN);
    esp_rom_delay_us(1);
    pcf8574_write(data & ~PIN_EN);
    esp_rom_delay_us(50);
}

static void lcd_write4bits(uint8_t nibble, uint8_t mode)
{
    uint8_t data = 0;

    if (nibble & 0x01) data |= PIN_D4;
    if (nibble & 0x02) data |= PIN_D5;
    if (nibble & 0x04) data |= PIN_D6;
    if (nibble & 0x08) data |= PIN_D7;

    if (mode) data |= PIN_RS;   // datos
    // RW lo dejamos siempre en 0 (escritura), no activamos PIN_RW

    lcd_pulse_enable(data);
}

static void lcd_send(uint8_t value, uint8_t mode)
{
    lcd_write4bits((value >> 4) & 0x0F, mode);
    lcd_write4bits(value & 0x0F, mode);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_send(cmd, 0);
}

static void lcd_data(uint8_t data)
{
    lcd_send(data, 1);
}

esp_err_t lcd_init(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized");

    vTaskDelay(pdMS_TO_TICKS(50));  // >40 ms after power-on

    // init sequence 4‑bit (HD44780)
    lcd_write4bits(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write4bits(0x03, 0);
    esp_rom_delay_us(150);
    lcd_write4bits(0x03, 0);
    lcd_write4bits(0x02, 0); // 4‑bit mode

    lcd_cmd(0x28); // 4‑bit, 2 lines, 5x8 dots
    lcd_cmd(0x08); // display off
    lcd_cmd(0x01); // clear
    vTaskDelay(pdMS_TO_TICKS(2));
    lcd_cmd(0x06); // entry mode set
    lcd_cmd(0x0C); // display on, cursor off

    return ESP_OK;
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 1) row = 1;
    lcd_cmd(0x80 | (col + row_offsets[row]));
}

void lcd_print(const char *s)
{
    while (*s) {
        lcd_data((uint8_t)*s++);
    }
}