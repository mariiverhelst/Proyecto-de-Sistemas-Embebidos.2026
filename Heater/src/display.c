#include "display.h"
#include "driver/spi_master.h"
#include <math.h>

#define MOSI 16
#define CLK  18
#define CS   17

static spi_device_handle_t spi_dev;

// ----------------------------------------------------
// ✅ MANTENEMOS tu inversión (IMPORTANTE)
static inline uint8_t inv(uint8_t x)
{
    return (uint8_t)~x;
}

// ----------------------------------------------------
static void Max7219_write(uint8_t adress, uint8_t com){

    uint8_t data[2] = { inv(adress), inv(com) };

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = data
    };
    
    spi_device_transmit(spi_dev, &t);
}

// ----------------------------------------------------
// ✅ TU CONFIG ORIGINAL (que sí funcionaba)
static void max7219_setup(void)
{
    Max7219_write(0x00,0x00); 
    Max7219_write(0x00,0x00);

    Max7219_write(0x0F, 0x00);
    Max7219_write(0x09, 0xFF);
    Max7219_write(0x0B, 0x02);
    Max7219_write(0x0A, 0x04);
    Max7219_write(0x0F, 0x00);
    Max7219_write(0x0C, 0x01);
}

// ----------------------------------------------------
void display_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = MOSI,
        .sclk_io_num = CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 50000,
        .mode = 2,   // ✅ TU CONFIG ORIGINAL
        .spics_io_num = CS,
        .queue_size = 1
    };

    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED);
    spi_bus_add_device(SPI2_HOST, &dev, &spi_dev);

    max7219_setup();
}

// ----------------------------------------------------
void display_show(float temp, bool mostrar_temp, uint32_t rpm)
{
    if (mostrar_temp)
    {
        float scaled = roundf(temp * 10.0f);
        int num = (int)scaled;

        int u = num % 10;
        int ent = num / 10;

        int d1 = ent / 10;
        int d0 = ent % 10;

        uint8_t mayor = d1;
        uint8_t medio = d0 | 0x80;
        uint8_t menor = u;

        Max7219_write(0x03, menor);
        Max7219_write(0x02, medio);
        Max7219_write(0x01, mayor);
    }
    else
    {
        int num = rpm;

        int u = num % 10;
        int ent = num / 10;

        int d1 = ent / 10;
        int d0 = ent % 10;

        uint8_t mayor = d1;
        uint8_t medio = d0;
        uint8_t menor = u;

        Max7219_write(0x03, menor);
        Max7219_write(0x02, medio);
        Max7219_write(0x01, mayor);
    }
}