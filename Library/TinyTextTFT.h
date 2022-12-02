#ifndef TinyText_h
#define TinyText_h

#include "Arduino.h"
#include "SPI.h"

/*
  I used code from the Adafruit ILI9341 driver and Adafruit_GFX
  to create this minimalist tiny anti-aliased text driver for
  ILI9341 320x240 TFT displays.

  Support Adafruit's work on drivers by purchasing directly from Adafruit:
    https://www.adafruit.com/category/825

  Some Adafruit attribution from the original version follows. Obviously
  this is my hack so any errors are 100% my fault.
 
     * Adafruit invests time and resources providing this open source code,
     * please support Adafruit and open-source hardware by purchasing
     * products from Adafruit!
     *
     * Written by Limor "ladyada" Fried for Adafruit Industries.
     *
 */

// Some handy colour definitions
#define RGB444_BLACK 0x000       //   0,   0,   0
#define RGB444_BLUE  0x00F       //   0,   0, 255
#define RGB444_GREEN 0x0F0       //   0, 255,   0
#define RGB444_RED   0xF00       // 255,   0,   0
#define RGB444_WHITE 0xFFF       // 255, 255, 255

class TinyTextTFT
{
private:
    SPIClass* _spi;

    SPISettings _settings; ///< SPI transaction settings
    uint16_t _width;
    uint16_t _height;
    uint8_t  _rotation;
    uint8_t _cellWidth;
    uint8_t _cellHeight;
    uint8_t _rows;
    uint8_t _columns;

    // pins
    int8_t _cs;
    int8_t _dc;
    int8_t _rst;

    uint8_t fontMap[256];

    void startWrite(void);
    void endWrite(void);
    void spiWrite(uint8_t b);
    void spiWrite16(uint16_t w);
    uint8_t spiRead(void);

    void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h);
    //void writePixel(int16_t x, int16_t y, uint16_t color);
    //void writeColor(uint16_t color, uint32_t len);
    void writeColor32(uint32_t color, uint32_t len);
    uint32_t makeColor32(uint16_t color0, uint16_t color1);

    uint16_t blendPixelColor(uint8_t tone, uint16_t foreColor, uint16_t backColor);
    uint16_t convertToRGB565(uint16_t rgb444);

    void writeCommand(uint8_t cmd);
    void sendCommand(uint8_t commandByte, uint8_t* dataBytes, uint8_t numDataBytes);
    void sendCommand(uint8_t commandByte, const uint8_t* dataBytes = NULL, uint8_t numDataBytes = 0);

    void initSPI(uint32_t freq);
    

public:
    uint16_t Height() { return _height; }
    uint16_t Width()  { return _width; }
    uint16_t Rows()   { return _rows; }
    uint16_t Columns() { return _columns; }

    TinyTextTFT(int8_t _CS, int8_t _DC, int8_t _RST);
    void Begin();
    void SetRotation(uint8_t m);
    void FillScreen(uint16_t color);
    void DrawChar(uint8_t col, uint8_t row, char chr, uint16_t foreColor, uint16_t backColor);

    //uint8_t ReadCommand8(uint8_t commandByte, uint8_t index = 0);

};

#endif