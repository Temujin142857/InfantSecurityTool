#include "lcd.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#define I2C_PORT I2C_NUM_0
#define LCD_DRIVER_ADDR 0x56
#define LCD_BACKLIGHT_ADDR 0x55

#define LCD_CMD   0x00
#define LCD_DATA  0x40

/* ----------- Low-level functions ----------- */

static void LCD_SendCommand(uint8_t cmd)
{
    uint8_t data[2] = {LCD_CMD, cmd};

    i2c_master_write_to_device(
        I2C_PORT,
        LCD_DRIVER_ADDR,
        data,
        2,
        pdMS_TO_TICKS(100)
    );
}

static void LCD_SendData(uint8_t data_byte)
{
    uint8_t data[2] = {LCD_DATA, data_byte};

    i2c_master_write_to_device(
        I2C_PORT,
        LCD_DRIVER_ADDR,
        data,
        2,
        pdMS_TO_TICKS(100)
    );
}

static void LCD_SetBacklight(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t pkt[2];

    // Red
    pkt[0] = 0x01;
    pkt[1] = r;
    i2c_master_write_to_device(I2C_PORT, LCD_BACKLIGHT_ADDR, pkt, 2, 100);

    // Green
    pkt[0] = 0x02;
    pkt[1] = g;
    i2c_master_write_to_device(I2C_PORT, LCD_BACKLIGHT_ADDR, pkt, 2, 100);

    // Blue
    pkt[0] = 0x03;
    pkt[1] = b;
    i2c_master_write_to_device(I2C_PORT, LCD_BACKLIGHT_ADDR, pkt, 2, 100);
}

/* ----------- Public API ----------- */

void LCD_Init(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));

    LCD_SetBacklight(0x0F, 0x0F, 0x0F);

    LCD_SendCommand(0x38);
    LCD_SendCommand(0x39);
    LCD_SendCommand(0x14);
    LCD_SendCommand(0x70);
    LCD_SendCommand(0x56);
    LCD_SendCommand(0x6C);

    vTaskDelay(pdMS_TO_TICKS(200));

    LCD_SendCommand(0x38);
    LCD_SendCommand(0x0C);
    LCD_SendCommand(0x01);

    vTaskDelay(pdMS_TO_TICKS(2));
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_SendCommand(address);
}

void LCD_SendChar(char c)
{
    LCD_SendData((uint8_t)c);
}

void LCD_Print(char *str)
{
    while (*str)
    {
        LCD_SendChar(*str++);
    }
}