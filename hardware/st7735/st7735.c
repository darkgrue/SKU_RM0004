#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include "rpiInfo.h"
#include "st7735.h"

int i2cd;

/**
 * @brief Set display coordinates.
 *
 * @param x0 X-coordinate of column origin.
 * @param y0 Y-coordinate of row origin.
 * @param x1 X-coordinate of column origin.
 * @param y1 Y-coordinate of row origin.
 * @return void
 */
void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // col address set
    i2c_write_command(X_COORDINATE_REG, x0 + ST7735_XSTART, x1 + ST7735_XSTART);
    // row address set
    i2c_write_command(Y_COORDINATE_REG, y0 + ST7735_YSTART, y1 + ST7735_YSTART);
    // write to RAM
    i2c_write_command(CHAR_DATA_REG, 0x00, 0x00);

    i2c_write_command(SYNC_REG, 0x00, 0x01);
}

/**
 * @brief Display a single character.
 *
 * @param x X-coordinate of text origin.
 * @param y Y-coordinate of text origin.
 * @param ch Character to display.
 * @param font Font set to use.
 * @param color Text foreground color.
 * @param bgcolor Text background color.
 * @return void
 */
void lcd_write_char(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;

    lcd_set_address_window(x, y, x + font.width - 1, y + font.height - 1);

    for (i = 0; i < font.height; i++)
    {
        b = font.data[(ch - 32) * font.height + i];
        for (j = 0; j < font.width; j++)
        {
            if ((b << j) & 0x8000)
            {
                i2c_write_data(color >> 8, color & 0xFF);
            }
            else
            {
                i2c_write_data(bgcolor >> 8, bgcolor & 0xFF);
            }
        }
    }
}

void lcd_write_ch(uint16_t x, uint16_t y, char ch, FontType font, uint16_t color, uint16_t bgcolor)
{
    switch (font)
    {
    case FontType_7x10:
        lcd_write_char(x, y, ch, Font_7x10, color, bgcolor);
        break;
    case FontType_8x16:
        lcd_write_char(x, y, ch, Font_8x16, color, bgcolor);
        break;
    case FontType_11x18:
        lcd_write_char(x, y, ch, Font_11x18, color, bgcolor);
        break;
    case FontType_16x26:
        lcd_write_char(x, y, ch, Font_16x26, color, bgcolor);
        break;
    }
}

/**
 * @brief Display a text string.
 *
 * @param x X-coordinate of text origin.
 * @param y Y-coordinate of text origin.
 * @param str String to display.
 * @param font Font set to use.
 * @param color Text foreground color.
 * @param bgcolor Text background color.
 * @return void
 */
void lcd_write_string(uint16_t x, uint16_t y, char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{

    while (*str)
    {
        if (x + font.width >= ST7735_WIDTH)
        {
            x = 0;
            y += font.height;
            if (y + font.height >= ST7735_HEIGHT)
            {
                break;
            }

            if (*str == ' ')
            {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        lcd_write_char(x, y, *str, font, color, bgcolor);
        i2c_write_command(SYNC_REG, 0x00, 0x01);
        x += font.width;
        str++;
    }
}

void lcd_write_str(uint16_t x, uint16_t y, char *str, FontType font, uint16_t color, uint16_t bgcolor)
{
    switch (font)
    {
    case FontType_7x10:
        lcd_write_string(x, y, str, Font_7x10, color, bgcolor);
        break;
    case FontType_8x16:
        lcd_write_string(x, y, str, Font_8x16, color, bgcolor);
        break;
    case FontType_11x18:
        lcd_write_string(x, y, str, Font_11x18, color, bgcolor);
        break;
    case FontType_16x26:
        lcd_write_string(x, y, str, Font_16x26, color, bgcolor);
        break;
    }
}

/**
 * @brief Flood fill rectangle with selected color.
 *
 * @param x X-coordinate of rectangle origin.
 * @param y Y-coordinate of rectangle origin.
 * @param w Width of rectangle in pixels.
 * @param h Height of rectangle in pixels.
 * @param color Color to fill with.
 * @return void
 */
void lcd_fill_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint8_t buff[320] = {0};
    uint16_t count = 0;
    // clipping
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;
    if ((x + w - 1) >= ST7735_WIDTH)
        w = ST7735_WIDTH - x;
    if ((y + h - 1) >= ST7735_HEIGHT)
        h = ST7735_HEIGHT - y;
    lcd_set_address_window(x, y, x + w - 1, y + h - 1);

    for (count = 0; count < w; count++)
    {
        buff[count * 2] = color >> 8;
        buff[count * 2 + 1] = color & 0xFF;
    }
    for (y = h; y > 0; y--)
    {
        i2c_burst_transfer(buff, sizeof(uint16_t) * w);
    }
}

/**
 * @brief Flood fill screen with selected color.
 *
 * @param color Color to fill with.
 * @return void
 */
void lcd_fill_screen(uint16_t color)
{
    lcd_fill_rectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
    i2c_write_command(SYNC_REG, 0x00, 0x01);
}

void lcd_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data)
{
    uint16_t col = h - y;
    uint16_t row = w - x;
    lcd_set_address_window(x, y, x + w - 1, y + h - 1);
    i2c_burst_transfer(data, sizeof(uint16_t) * col * row);
}

uint8_t lcd_begin(void)
{
    uint8_t count = 0;
    uint8_t buffer[20] = {0};
    uint8_t i2c[20] = "/dev/i2c-1";
    // I2C Init
    i2cd = open(i2c, O_RDWR); //"/dev/i2c-1"
    if (i2cd < 0)
    {
        fprintf(stderr, "Device I2C-1 failed to initialize\n");
        return 1;
    }
    if (ioctl(i2cd, I2C_SLAVE_FORCE, I2C_ADDRESS) < 0)
    {
        return 1;
    }
    return 0;
}

void i2c_write_data(uint8_t high, uint8_t low)
{
    uint8_t msg[3] = {WRITE_DATA_REG, high, low};
    write(i2cd, msg, 3);
    usleep(10);
}

void i2c_write_command(uint8_t command, uint8_t high, uint8_t low)
{
    uint8_t msg[3] = {command, high, low};
    write(i2cd, msg, 3);
    usleep(10);
}

void i2c_burst_transfer(uint8_t *buff, uint32_t length)
{
    uint32_t count = 0;
    i2c_write_command(BURST_WRITE_REG, 0x00, 0x01);
    while (length > count)
    {
        if ((length - count) > BURST_MAX_LENGTH)
        {
            write(i2cd, buff + count, BURST_MAX_LENGTH);
            count += BURST_MAX_LENGTH;
        }
        else
        {
            write(i2cd, buff + count, length - count);
            count += (length - count);
        }
        usleep(700);
    }
    i2c_write_command(BURST_WRITE_REG, 0x00, 0x00);
    i2c_write_command(SYNC_REG, 0x00, 0x01);
}

void lcd_display(uint8_t symbol)
{
    switch (symbol)
    {
    case 0:
        lcd_display_cpuLoad();
        break;
    case 1:
        lcd_display_ram();
        break;
    case 2:
        lcd_display_temp();
        break;
    case 3:
        lcd_display_disk();
        break;
    default:
        break;
    }
}

void lcd_display_percentage(uint8_t val, uint16_t color)
{
    uint8_t count = 0;
    uint8_t xCoordinate = 30;
    val += 10;
    if (val >= 100)
    {
        val = 100;
    }
    val /= 10;
    for (count = 0; count < val; count++)
    {
        lcd_fill_rectangle(xCoordinate, 60, 6, 10, color);
        xCoordinate += 10;
    }
    for (count = 0; count < 10 - val; count++)
    {
        lcd_fill_rectangle(xCoordinate, 60, 6, 10, ST7735_GRAY);
        xCoordinate += 10;
    }
}

/**
 * @brief Display CPU utilization.
 *
 * @return void
 */
void lcd_display_cpuLoad(void)
{
    char hostname[HOST_NAME_MAX + 1];
    char buffer[20] = {0};
    uint8_t cpuLoad = 0;
    char cpuStr[10] = {0};

    gethostname(hostname, HOST_NAME_MAX + 1);

    /*
    printf("hostname: %s\n", hostname);
    */

    lcd_fill_screen(ST7735_BLACK);
    lcd_fill_rectangle(0, 20, ST7735_WIDTH, 5, ST7735_BLUE);

    // cpuLoad = GetCPUUsageTop();
    cpuLoad = GetCPUUsagePstat();
    sprintf(cpuStr, "%u", cpuLoad);

    if (DISPLAY_IP_ADDR)
    {
        lcd_write_string(0, 0, "IP:", Font_8x16, ST7735_WHITE, ST7735_BLACK);
        strcpy(buffer, GetIPAddress()); // Get the IP address of the default interface
        lcd_write_string(24, 0, buffer, Font_8x16, ST7735_WHITE, ST7735_BLACK);
    }
    else
    {
        strncpy(buffer, (*CUSTOM_DISPLAY != '\0') ? CUSTOM_DISPLAY : hostname, sizeof(buffer) - 1);
        // Maximum of 20 characters for 8x16 font
        // Center 8x16 font
        lcd_write_string(4 + ((19 - strlen(buffer)) / 2 * 8), 0, buffer, Font_8x16, ST7735_WHITE, ST7735_BLACK);
    }

    lcd_write_string(36, 35, "CPU:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(80, 35, cpuStr, Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(113, 35, "%", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_display_percentage(cpuLoad, ST7735_GREEN);
}

/**
 * @brief Display RAM utilization.
 *
 * @return void
 */
void lcd_display_ram(void)
{
    uint8_t ramPct = 0;
    char ramStr[10] = {0};

    ramPct = GetMemory();
    sprintf(ramStr, "%d", ramPct);

    lcd_fill_rectangle(0, 35, ST7735_WIDTH, 20, ST7735_BLACK);
    lcd_write_string(36, 35, "RAM:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(80, 35, ramStr, Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(113, 35, "%", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_display_percentage(ramPct, ST7735_YELLOW);
}

/**
 * @brief Display CPU temperature.
 *
 * @return void
 */
void lcd_display_temp(void)
{
    uint16_t temp = 0;
    char tempStr[10] = {0};

    temp = GetCPUTemperature();
    sprintf(tempStr, "%d", temp);

    lcd_fill_rectangle(0, 35, ST7735_WIDTH, 20, ST7735_BLACK);
    lcd_write_string(30, 35, "TEMP:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(85, 35, tempStr, Font_11x18, ST7735_WHITE, ST7735_BLACK);
    if (TEMPERATURE_TYPE == FAHRENHEIT)
    {
        lcd_write_string(118, 35, "F", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    }
    else
    {
        lcd_write_string(118, 35, "C", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    }
    if (TEMPERATURE_TYPE == FAHRENHEIT)
    {
        temp -= 32;
        temp /= 1.8;
    }
    lcd_display_percentage((uint16_t)((float)temp / 85 * 100.0), ST7735_RED);
}

/**
 * @brief Display filesystem utilization.
 *
 * @return void
 */
void lcd_display_disk(void)
{
    uint8_t fsUsed = 0;
    char fsStr[10] = {0};

    // fsUsed = GetFSMemoryStatfs();
    fsUsed = GetFSMemoryDf();
    sprintf(fsStr, "%u", fsUsed);

    lcd_fill_rectangle(0, 35, ST7735_WIDTH, 20, ST7735_BLACK);
    lcd_write_string(30, 35, "DISK:", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(85, 35, fsStr, Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_write_string(118, 35, "%", Font_11x18, ST7735_WHITE, ST7735_BLACK);
    lcd_display_percentage(fsUsed, ST7735_BLUE);
}
