#ifndef SSD1306_H
#define SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>
#include "gfxfont.h"
#include <stdint.h>

#define SSD1306_TAG "SSD1306"

/// Solen from : Adafruit_SSD1306
/// The following "raw" color names are kept for backwards client compatability
/// They can be disabled by predefining this macro before including the Adafruit header
/// client code will then need to be modified to use the scoped enum values directly
#ifndef NO_ADAFRUIT_SSD1306_COLOR_COMPATIBILITY
#define BLACK                     SSD1306_BLACK    ///< Draw 'off' pixels
#define WHITE                     SSD1306_WHITE    ///< Draw 'on' pixels
#define INVERSE                   SSD1306_INVERSE  ///< Invert pixels
#endif
        /// fit into the SSD1306_ naming scheme
#define SSD1306_BLACK               0    ///< Draw 'off' pixels
#define SSD1306_WHITE               1    ///< Draw 'on' pixels
#define SSD1306_INVERSE             2    ///< Invert pixels

#define SSD1306_MEMORYMODE          0x20 ///< See datasheet
#define SSD1306_COLUMNADDR          0x21 ///< See datasheet
#define SSD1306_PAGEADDR            0x22 ///< See datasheet
#define SSD1306_SETCONTRAST         0x81 ///< See datasheet
#define SSD1306_CHARGEPUMP          0x8D ///< See datasheet
#define SSD1306_SEGREMAP            0xA0 ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON        0xA5 ///< Not currently used
#define SSD1306_NORMALDISPLAY       0xA6 ///< See datasheet
#define SSD1306_INVERTDISPLAY       0xA7 ///< See datasheet
#define SSD1306_SETMULTIPLEX        0xA8 ///< See datasheet
#define SSD1306_DISPLAYOFF          0xAE ///< See datasheet
#define SSD1306_DISPLAYON           0xAF ///< See datasheet
#define SSD1306_COMSCANINC          0xC0 ///< Not currently used
#define SSD1306_COMSCANDEC          0xC8 ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET    0xD3 ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5 ///< See datasheet
#define SSD1306_SETPRECHARGE        0xD9 ///< See datasheet
#define SSD1306_SETCOMPINS          0xDA ///< See datasheet
#define SSD1306_SETVCOMDETECT       0xDB ///< See datasheet

#define SSD1306_SETLOWCOLUMN        0x00 ///< Not currently used
#define SSD1306_SETHIGHCOLUMN       0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE        0x40 ///< See datasheet

#define SSD1306_EXTERNALVCC         0x01 ///< External display voltage source
#define SSD1306_SWITCHCAPVCC        0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26 ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27 ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL                    0x2E ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL                      0x2F ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3 ///< Set scroll range

#define SCREEN_OLED_I2CADDRESS 0x3C

typedef struct 
{
    uint8_t i2cAddress;
    gpio_num_t pinReset;
} SSD1306_config;


#define SSD1306_CONFIG_DEFAULT_128x64  \
{                               \
    .i2cAddress = SCREEN_OLED_I2CADDRESS,  \
    .pinReset = (gpio_num_t)-1 \
} 

typedef struct 
{
    i2c_port_t i2c_port;
    SSD1306_config config;
    
    uint8_t* buffer;
    uint16_t bufferLen;

    uint16_t width;
    uint16_t height;

    GFXfont* font;
    uint8_t baselineY;
} SSD1306_handle;

void SSD1306_Init(SSD1306_handle* pHandle, i2c_port_t i2c_port, SSD1306_config* pconfig);

void SSD1306_Uninit(SSD1306_handle* pHandle);

void SSD1306_SetPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y);
void SSD1306_ClearPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y);

void SSD1306_ClearDisplay(SSD1306_handle* pHandle);

void SSD1306_DisplayState(SSD1306_handle* pHandle, bool isActive);

void SSD1306_InvertDisplay(SSD1306_handle* pHandle);
void SSD1306_NormalDisplay(SSD1306_handle* pHandle);

int SSD1306_DrawChar(SSD1306_handle* pHandle, uint16_t x, uint16_t y, unsigned char c);

void SSD1306_DrawString(SSD1306_handle* pHandle, uint16_t x, uint16_t y, const char* buffer, int len);

void SSD1306_UpdateDisplay(SSD1306_handle* pHandle);

#ifdef __cplusplus
}
#endif

#endif