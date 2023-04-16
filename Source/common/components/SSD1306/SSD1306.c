#include "SSD1306.h"
// Fonts
#include "fonts/FreeMono12pt7b.h"
#include "fonts/FreeMono9pt7b.h"
#include "fonts/Tiny3x3a2pt7b.h"
#include "fonts/Picopixel.h"

typedef enum
{
    ControlByte_Command = 0x00,
    ControlByte_Data = 0x40
} ControlByte;

static void sendCommand1(SSD1306_handle* pHandle, uint8_t value);
static void sendCommand(SSD1306_handle* pHandle, uint8_t* value, int n);

static void sendData1(SSD1306_handle* pHandle, uint8_t value);
static void sendData(SSD1306_handle* pHandle, uint8_t* value, int n);
static void writeI2C(SSD1306_handle* pHandle, ControlByte controlByte, uint8_t* value, int length);

static bool IsXYValid(SSD1306_handle* pHandle, uint16_t x, uint16_t y);

void SSD1306_Init(SSD1306_handle* pHandle, i2c_port_t i2c_port, SSD1306_config* pconfig)
{
    pHandle->i2c_port = i2c_port;
    pHandle->config = *pconfig;

    pHandle->width = 128;
    pHandle->height = 64;

    pHandle->bufferLen = (pHandle->height * pHandle->width) / 8;
    pHandle->buffer = malloc(sizeof(uint8_t) * pHandle->bufferLen);

    // Clear screen
    SSD1306_ClearDisplay(pHandle);

    // Reset
    if (pconfig->pinReset >= 0)
    {
        gpio_set_direction((gpio_num_t)pconfig->pinReset, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)pconfig->pinReset, false);
        vTaskDelay(pdMS_TO_TICKS(50)+1);
        gpio_set_level((gpio_num_t)pconfig->pinReset, true);
    }

    sendCommand1(pHandle, SSD1306_DISPLAYOFF);
    
    sendCommand1(pHandle, SSD1306_SETDISPLAYCLOCKDIV);
    sendCommand1(pHandle, 0xF0); // Increase speed of the display max ~96Hz
    sendCommand1(pHandle, SSD1306_SETMULTIPLEX);
    sendCommand1(pHandle, pHandle->height - 1);
    sendCommand1(pHandle, SSD1306_SETDISPLAYOFFSET);
    sendCommand1(pHandle, 0x00);
    
    sendCommand1(pHandle, SSD1306_SETSTARTLINE);
    sendCommand1(pHandle, SSD1306_CHARGEPUMP);
    sendCommand1(pHandle, 0x14);
    sendCommand1(pHandle, SSD1306_MEMORYMODE);
    sendCommand1(pHandle, 0x00);
    sendCommand1(pHandle, SSD1306_SEGREMAP|0x01);
    sendCommand1(pHandle, SSD1306_COMSCANDEC);
    sendCommand1(pHandle, SSD1306_SETCOMPINS);
    sendCommand1(pHandle, 0x12);

    sendCommand1(pHandle, SSD1306_SETCONTRAST);
    sendCommand1(pHandle, 0xCF);

    sendCommand1(pHandle, SSD1306_SETPRECHARGE);
    sendCommand1(pHandle, 0xF1);
    sendCommand1(pHandle, SSD1306_SETVCOMDETECT); //0xDB, (additionally needed to lower the contrast)
    sendCommand1(pHandle, 0x40);	        //0x40 default, to lower the contrast, put 0
    sendCommand1(pHandle, SSD1306_DISPLAYALLON_RESUME);
    sendCommand1(pHandle, SSD1306_NORMALDISPLAY);
    sendCommand1(pHandle, 0x2e);            // stop scroll
    sendCommand1(pHandle, SSD1306_DISPLAYON);

    //pHandle->font = &Tiny3x3a2pt7b;
    //pHandle->font = &FreeMono9pt7b;
    pHandle->font = &FreeMono9pt7b;

    // Find the baseline
    pHandle->baselineY = 0;
    
    for(int i = 0; i < (pHandle->font->last - pHandle->font->first); i++)
    {
        if (pHandle->baselineY < pHandle->font->glyph[i].height)
            pHandle->baselineY = pHandle->font->glyph[i].height;
    }
    // Sometime to help initialization ..
    vTaskDelay(pdMS_TO_TICKS(10)+1);
}

void SSD1306_Uninit(SSD1306_handle* pHandle)
{
    if (pHandle->buffer != NULL)
    {
        free(pHandle->buffer);
    }
}

void SSD1306_ClearDisplay(SSD1306_handle* pHandle)
{
    memset(pHandle->buffer, 0, pHandle->bufferLen);
}

void SSD1306_UpdateDisplay(SSD1306_handle* pHandle)
{
    const uint8_t displayInit[] = {
      SSD1306_PAGEADDR,
      0,                      // Page start address
      0xFF,                   // Page end (not really, but works here)
      SSD1306_COLUMNADDR, 0, pHandle->width - 1}; // Column start address

    sendCommand(pHandle, displayInit, sizeof(displayInit));
    sendData(pHandle, pHandle->buffer, pHandle->bufferLen);
}

void SSD1306_DisplayState(SSD1306_handle* pHandle, bool isActive)
{
    if (isActive)
        sendCommand1(pHandle, SSD1306_DISPLAYON);
    else
        sendCommand1(pHandle, SSD1306_DISPLAYOFF);
}

void SSD1306_InvertDisplay(SSD1306_handle* pHandle) 
{
    sendCommand1(pHandle, SSD1306_INVERTDISPLAY);
}

void SSD1306_NormalDisplay(SSD1306_handle* pHandle) 
{
    sendCommand1(pHandle, SSD1306_NORMALDISPLAY);
}

void SSD1306_SetPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    if (!IsXYValid(pHandle, x, y))
        return;
    pHandle->buffer[x + (y >> 3) * pHandle->width] |=  (1 << (y & 7));
}

void SSD1306_ClearPixel(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    if (!IsXYValid(pHandle, x, y))
        return;
    pHandle->buffer[x + (y >> 3) * pHandle->width] &=  ~(1 << (y & 7));
}

int SSD1306_DrawChar(SSD1306_handle* pHandle, uint16_t x, uint16_t y, unsigned char c)
{   
    // Character is not supported by the font
    if (c < pHandle->font->first || c > pHandle->font->last)
        return;

    int index = c - pHandle->font->first;
    GFXglyph* glyph = pHandle->font->glyph + index;

    uint8_t* pMap = pHandle->font->bitmap + glyph->bitmapOffset;
    
    uint8_t bitsize = glyph->width * glyph->height;
    uint8_t byteCnt = ((bitsize) + 7) / 8;

    uint8_t byte = 0;   

    for(int i = 0; i < bitsize; i++)
    {    
        int x1 = i % glyph->width + x + glyph->xOffset;
        int y1 = i / glyph->width + y + (pHandle->baselineY + glyph->yOffset);

        if (i % 8 == 0)
            byte = pMap[i / 8];

        if (byte & 0x80)
            SSD1306_SetPixel(pHandle, x1, y1);
        byte <<= 1;
    }

    return glyph->xAdvance;
}

void SSD1306_DrawString(SSD1306_handle* pHandle, uint16_t x, uint16_t y, const char* buffer, int len)
{
    int x1 = x, y1 = y;

    for(int i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)buffer[i];
        if (c == '\r')
        {
            y1 += pHandle->font->yAdvance;
            x1 = 0;
        }
        else
        {
            int xAdvance = SSD1306_DrawChar(pHandle, x1, y1, c);       
            x1 += xAdvance;
        }
    }
}

static bool IsXYValid(SSD1306_handle* pHandle, uint16_t x, uint16_t y)
{
    return (x < pHandle->width) && (y < pHandle->height); 
}

static void sendCommand1(SSD1306_handle* pHandle, uint8_t value)
{
    writeI2C(pHandle, ControlByte_Command, &value, 1);
}

static void sendCommand(SSD1306_handle* pHandle, uint8_t* value, int length)
{
    writeI2C(pHandle, ControlByte_Command, value, length);
}

static void sendData1(SSD1306_handle* pHandle, uint8_t value)
{
    writeI2C(pHandle, ControlByte_Data, &value, 1);
}

static void sendData(SSD1306_handle* pHandle, uint8_t* value, int length)
{
    writeI2C(pHandle, ControlByte_Data, value, length);
}

static void writeI2C(SSD1306_handle* pHandle, ControlByte controlByte, uint8_t* value, int length)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// Write sampling command
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (pHandle->config.i2cAddress<<1), true)); 
    // CO = 0, D/C = 0 (Data = 1, Command = 0)
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (uint8_t)controlByte, true));  // Data mode (bit 6)
    for(int i = 0; i < length; i++)
    {
	    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, *(value + i), true));
    }
	ESP_ERROR_CHECK(i2c_master_stop(cmd));

	esp_err_t ret = i2c_master_cmd_begin(pHandle->i2c_port, cmd, 1000 / portTICK_RATE_MS);
	ESP_ERROR_CHECK(ret);

	i2c_cmd_link_delete(cmd);
}
