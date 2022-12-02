#include "Arduino.h"
#include "SPI.h"
#include "TinyTextTFT.h"

/*
  I used code from the Adafruit ILI9341 driver and Adafruit_GFX
  to create this minimalist tiny anti-aliased text driver for
  ILI9341 320x240 TFT displays.

  Support Adafruit's work on drivers by purchasing directly from Adafruit:
    https://www.adafruit.com/category/825

*/

#define SPI_DEFAULT_FREQ 24000000 ///< Default SPI data clock frequency

#define ILI9341_TFTWIDTH 240  // ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 320 // ILI9341 max TFT height

//#define ILI9488_TFTWIDTH  320
//#define ILI9488_TFTHEIGHT 480

#define ILI9341_SWRESET 0x01 ///< Software reset register
#define ILI9341_RDMODE 0x0A     ///< Read Display Power Mode
#define ILI9341_RDMADCTL 0x0B   ///< Read Display MADCTL
#define ILI9341_RDPIXFMT 0x0C   ///< Read Display Pixel Format
#define ILI9341_RDIMGFMT 0x0D   ///< Read Display Image Format
#define ILI9341_RDSELFDIAG 0x0F ///< Read Display Self-Diagnostic Result
#define ILI9341_SLPOUT 0x11 ///< Sleep Out
#define ILI9341_DISPON 0x29   ///< Display ON
#define ILI9341_GAMMASET 0x26 ///< Gamma Set
#define ILI9341_CASET 0x2A ///< Column Address Set
#define ILI9341_PASET 0x2B ///< Page Address Set
#define ILI9341_RAMWR 0x2C ///< Memory Write
#define ILI9341_RAMRD 0x2E ///< Memory Read
#define ILI9341_MADCTL 0x36   ///< Memory Access Control
#define ILI9341_VSCRSADD 0x37 ///< Vertical Scrolling Start Address
#define ILI9341_PIXFMT 0x3A   ///< COLMOD: Pixel Format Set
#define ILI9341_FRMCTR1 0xB1 ///< Frame Rate Control (In Normal Mode/Full Colors)
#define ILI9341_DFUNCTR 0xB6 ///< Display Function Control
#define ILI9341_PWCTR1 0xC0 ///< Power Control 1
#define ILI9341_PWCTR2 0xC1 ///< Power Control 2
#define ILI9341_VMCTR1 0xC5 ///< VCOM Control 1
#define ILI9341_VMCTR2 0xC7 ///< VCOM Control 2
#define ILI9341_GMCTRP1 0xE0 ///< Positive Gamma Correction
#define ILI9341_GMCTRN1 0xE1 ///< Negative Gamma Correction

// see SetRotation(..)
#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
//#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
//#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
//#define MADCTL_MH 0x04  ///< LCD refresh right to left

static const uint8_t PROGMEM initcmd[] =
{
  0xEF, 3, 0x03, 0x80, 0x02,
  0xCF, 3, 0x00, 0xC1, 0x30,
  0xED, 4, 0x64, 0x03, 0x12, 0x81,
  0xE8, 3, 0x85, 0x00, 0x78,
  0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  0xF7, 1, 0x20,
  0xEA, 2, 0x00, 0x00,
  ILI9341_PWCTR1  , 1, 0x23,             // Power control VRH[5:0]
  ILI9341_PWCTR2  , 1, 0x10,             // Power control SAP[2:0];BT[3:0]
  ILI9341_VMCTR1  , 2, 0x3e, 0x28,       // VCM control
  ILI9341_VMCTR2  , 1, 0x86,             // VCM control2
  ILI9341_MADCTL  , 1, 0x48,             // Memory Access Control
  ILI9341_VSCRSADD, 1, 0x00,             // Vertical scroll zero
  ILI9341_PIXFMT  , 1, 0x55,
  ILI9341_FRMCTR1 , 2, 0x00, 0x18,
  ILI9341_DFUNCTR , 3, 0x08, 0x82, 0x27, // Display Function Control
  0xF2, 1, 0x00,                         // 3Gamma Function Disable
  ILI9341_GAMMASET , 1, 0x01,             // Gamma curve selected
  ILI9341_GMCTRP1 , 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00, // Set Gamma
  ILI9341_GMCTRN1 , 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F, // Set Gamma
  ILI9341_SLPOUT  , 0x80,                // Exit Sleep
  ILI9341_DISPON  , 0x80,                // Display on
  0x00                                   // End of list
};

static const uint8_t /*PROGMEM*/ toneToShade[] = { 0, 37, 60, 81, 99, 116, 133, 148, 163, 177, 191, 204, 217, 230, 243, 255 };

static const uint16_t PROGMEM tinyFont[][9] =
{
    // ' ' 0x20
    { 0x20, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '!' 0x21
    { 0x21, 0xFB7F, 0xFB7F, 0xFE7F, 0xFF7F, 0xFFFF, 0xFB5F, 0xFFFF, 0xFFFF} ,
    // '"' 0x22
    { 0x22, 0xF377, 0xF3B7, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '#' 0x23
    { 0x23, 0xFFFF, 0xF6D7, 0x3000, 0xF5BB, 0x3000, 0xF77D, 0xFFFF, 0xFFFF} ,
    // '$' 0x24
    { 0x24, 0xFF6F, 0xC103, 0x896F, 0xE61A, 0xFBB1, 0x7006, 0xF7FF, 0xFFFF} ,
    // '%' 0x25
    { 0x25, 0x50D6, 0x4098, 0xFF5F, 0xF8BF, 0xE660, 0x6E50, 0xFFFF, 0xFFFF} ,
    // '&' 0x26
    { 0x26, 0xE10D, 0xB79B, 0xE28F, 0x7882, 0x3E53, 0xA013, 0xFFFF, 0xFFFF} ,
    // ''' 0x27
    { 0x27, 0xFB3F, 0xFB7F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '(' 0x28
    { 0x28, 0xFF8D, 0xFD6F, 0xF6DF, 0xF3FF, 0xF3FF, 0xF5DF, 0xFD6F, 0xFF7D} ,
    // ')' 0x29
    { 0x29, 0xF7EF, 0xFC5F, 0xFF4E, 0xFF7B, 0xFF7B, 0xFF4F, 0xFD6F, 0xF6EF} ,
    // '*' 0x2A
    { 0x2A, 0xFF7F, 0xD748, 0xFF7F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '+' 0x2B
    { 0x2B, 0xFFFF, 0xFB7F, 0xFB7F, 0x3000, 0xFB7F, 0xFB7F, 0xFFFF, 0xFFFF} ,
    // ',' 0x2C
    { 0x2C, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFC2F, 0xF07F, 0xFFFF} ,
    // '-' 0x2D
    { 0x2D, 0xFFFF, 0xFFFF, 0xFFFF, 0xF007, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '.' 0x2E
    { 0x2E, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF95F, 0xFFFF, 0xFFFF} ,
    // '/' 0x2F
    { 0x2F, 0xFFE5, 0xFF8C, 0xFF4F, 0xFC8F, 0xF5EF, 0xE6FF, 0x9AFF, 0xFFFF} ,
    // '0' 0x30
    { 0x30, 0xFFFF, 0xE207, 0x7890, 0x7926, 0x74D1, 0xD109, 0xFFFF, 0xFFFF} ,
    // '1' 0x31
    { 0x31, 0xFFFF, 0x903F, 0xFF3F, 0xFF3F, 0xFF3F, 0xB000, 0xFFFF, 0xFFFF} ,
    // '2' 0x32
    { 0x32, 0xFFFF, 0xE20A, 0xBBE4, 0xFF8C, 0xE6CF, 0x7000, 0xFFFF, 0xFFFF} ,
    // '3' 0x33
    { 0x33, 0xFFFF, 0xB00A, 0xFFA7, 0xF30B, 0xFFE3, 0x7019, 0xFFFF, 0xFFFF} ,
    // '4' 0x34
    { 0x34, 0xFFFF, 0xFE27, 0xF597, 0x8BB7, 0x3000, 0xFFB7, 0xFFFF, 0xFFFF} ,
    // '5' 0x35
    { 0x35, 0xFFFF, 0xB007, 0xB7FF, 0xB009, 0xFFD3, 0xB01B, 0xFFFF, 0xFFFF} ,
    // '6' 0x36
    { 0x36, 0xFFFF, 0xF603, 0xA7FF, 0x7505, 0x85F1, 0xD107, 0xFFFF, 0xFFFF} ,
    // '7' 0x37
    { 0x37, 0xFFFF, 0x7000, 0xFFD5, 0xFF5D, 0xFC7F, 0xF4EF, 0xFFFF, 0xFFFF} ,
    // '8' 0x38
    { 0x38, 0xFFFF, 0xD107, 0xB7D5, 0xF20A, 0x87D2, 0xC006, 0xFFFF, 0xFFFF} ,
    // '9' 0x39
    { 0x39, 0xFFFF, 0xC10A, 0x7CC3, 0xA027, 0xFFB4, 0xB03D, 0xFFFF, 0xFFFF} ,
    // ':' 0x3A
    { 0x3A, 0xFFFF, 0xFFFF, 0xFB5F, 0xFFFF, 0xFFFF, 0xFB5F, 0xFFFF, 0xFFFF} ,
    // ';' 0x3B
    { 0x3B, 0xFFFF, 0xFFFF, 0xF95F, 0xFFFF, 0xFFFF, 0xFC2F, 0xF07F, 0xFFFF} ,
    // '<' 0x3C
    { 0x3C, 0xFFFF, 0xFFFF, 0xFF8B, 0xE5BF, 0xE5BF, 0xFF8B, 0xFFFF, 0xFFFF} ,
    // '=' 0x3D
    { 0x3D, 0xFFFF, 0xFFFF, 0x7000, 0xFFFF, 0x7000, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '>' 0x3E
    { 0x3E, 0xFFFF, 0xFFFF, 0xE4DF, 0xFE5A, 0xFE5A, 0xE4DF, 0xFFFF, 0xFFFF} ,
    // '?' 0x3F
    { 0x3F, 0xF35E, 0xFF78, 0xFFA7, 0xF70B, 0xFFFF, 0xF79F, 0xFFFF, 0xFFFF} ,
    // '@' 0x40
    { 0x40, 0xF605, 0xB5F6, 0x5A07, 0x6497, 0x736A, 0x5322, 0x5CFF, 0xC00C} ,
    // 'A' 0x41
    { 0x41, 0xFFFF, 0xF96F, 0xF79C, 0xE7E6, 0x9001, 0x5FF7, 0xFFFF, 0xFFFF} ,
    // 'B' 0x42
    { 0x42, 0xFFFF, 0x7008, 0x7BD4, 0x7009, 0x7BF1, 0x7007, 0xFFFF, 0xFFFF} ,
    // 'C' 0x43
    { 0x43, 0xFFFF, 0xE300, 0x87FF, 0x7BFF, 0x77FF, 0xE300, 0xFFFF, 0xFFFF} ,
    // 'D' 0x44
    { 0x44, 0xFFFF, 0x7009, 0x7BD1, 0x7BF3, 0x7BD2, 0x701A, 0xFFFF, 0xFFFF} ,
    // 'E' 0x45
    { 0x45, 0xFFFF, 0xB003, 0xB7FF, 0xB003, 0xB7FF, 0xB003, 0xFFFF, 0xFFFF} ,
    // 'F' 0x46
    { 0x46, 0xFFFF, 0xB003, 0xB7FF, 0xB007, 0xB7FF, 0xB7FF, 0xFFFF, 0xFFFF} ,
    // 'G' 0x47
    { 0x47, 0xFFFF, 0xE300, 0x68FF, 0x3F30, 0x59F3, 0xD301, 0xFFFF, 0xFFFF} ,
    // 'H' 0x48
    { 0x48, 0xFFFF, 0x7BF3, 0x7BF3, 0x7000, 0x7BF3, 0x7BF3, 0xFFFF, 0xFFFF} ,
    // 'I' 0x49
    { 0x49, 0xFFFF, 0xB003, 0xFB7F, 0xFB7F, 0xFB7F, 0xB003, 0xFFFF, 0xFFFF} ,
    // 'J' 0x4A
    { 0x4A, 0xFFFF, 0xB007, 0xFFB7, 0xFFB7, 0xFFA7, 0xB01D, 0xFFFF, 0xFFFF} ,
    // 'K' 0x4B
    { 0x4B, 0xFFFF, 0x7BC5, 0x7B5E, 0x73CF, 0x7A4F, 0x7BC6, 0xFFFF, 0xFFFF} ,
    // 'L' 0x4C
    { 0x4C, 0xFFFF, 0xF3FF, 0xF3FF, 0xF3FF, 0xF3FF, 0xF000, 0xFFFF, 0xFFFF} ,
    // 'M' 0x4D
    { 0x4D, 0xFFFF, 0x79E0, 0x68A7, 0x3B97, 0x3FF7, 0x3FF7, 0xFFFF, 0xFFFF} ,
    // 'N' 0x4E
    { 0x4E, 0xFFFF, 0x77F3, 0x77F3, 0x7BA3, 0x7BB3, 0x7BC2, 0xFFFF, 0xFFFF} ,
    // 'O' 0x4F
    { 0x4F, 0xFFFF, 0xD107, 0x5AE3, 0x3FF7, 0x5CE3, 0xC109, 0xFFFF, 0xFFFF} ,
    // 'P' 0x50
    { 0x50, 0xFFFF, 0x7006, 0x7BF1, 0x7008, 0x7BFF, 0x7BFF, 0xFFFF, 0xFFFF} ,
    // 'Q' 0x51
    { 0x51, 0xFFFF, 0xD106, 0x5AE3, 0x3FF7, 0x5AE3, 0xC107, 0xFF20, 0xFFFF} ,
    // 'R' 0x52
    { 0x52, 0xFFFF, 0xB009, 0xB7D3, 0xB00B, 0xB786, 0xB7F2, 0xFFFF, 0xFFFF} ,
    // 'S' 0x53
    { 0x53, 0xFFFF, 0xC103, 0x88FF, 0xE73B, 0xFFE1, 0x7007, 0xFFFF, 0xFFFF} ,
    // 'T' 0x54
    { 0x54, 0xFFFF, 0x3000, 0xFB7F, 0xFB7F, 0xFB7F, 0xFB7F, 0xFFFF, 0xFFFF} ,
    // 'U' 0x55
    { 0x55, 0xFFFF, 0x7BF3, 0x7BF3, 0x7BF3, 0x7AE1, 0xC007, 0xFFFF, 0xFFFF} ,
    // 'V' 0x56
    { 0x56, 0xFFFF, 0x3FF6, 0x8AF3, 0xD5D6, 0xF38C, 0xF81F, 0xFFFF, 0xFFFF} ,
    // 'W' 0x57
    { 0x57, 0xFFFF, 0x3FF7, 0x3FF7, 0x5D67, 0x7997, 0x77D1, 0xFFFF, 0xFFFF} ,
    // 'X' 0x58
    { 0x58, 0xFFFF, 0x7AE4, 0xF35B, 0xF92F, 0xE35A, 0x6BE2, 0xFFFF, 0xFFFF} ,
    // 'Y' 0x59
    { 0x59, 0xFFFF, 0x5DF5, 0xD4B7, 0xF83F, 0xFB7F, 0xFB7F, 0xFFFF, 0xFFFF} ,
    // 'Z' 0x5A
    { 0x5A, 0xFFFF, 0x7002, 0xFFCC, 0xFE9F, 0xF8FF, 0x9000, 0xFFFF, 0xFFFF} ,
    // '[' 0x5B
    { 0x5B, 0xF30B, 0xF3FF, 0xF3FF, 0xF3FF, 0xF3FF, 0xF3FF, 0xF3FF, 0xF30B} ,
    // '\' 0x5C
    { 0x5C, 0xC8FF, 0xF4FF, 0xF9BF, 0xFE5F, 0xFF6E, 0xFFB9, 0xFFF5, 0xFFFF} ,
    // ']' 0x5D
    { 0x5D, 0xF00F, 0xFF3F, 0xFF3F, 0xFF3F, 0xFF3F, 0xFF3F, 0xFF3F, 0xF00F} ,
    // '^' 0x5E
    { 0x5E, 0xFFFF, 0xF94F, 0xF69B, 0xB9E4, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF} ,
    // '_' 0x5F
    { 0x5F, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000} ,
    // 'a' 0x61
    { 0x61, 0xFFFF, 0xFFFF, 0xB008, 0xD103, 0x79D3, 0xB025, 0xFFFF, 0xFFFF} ,
    // 'b' 0x62
    { 0x62, 0x7BFF, 0x7BFF, 0x7306, 0x77F2, 0x7BE2, 0x9009, 0xFFFF, 0xFFFF} ,
    // 'c' 0x63
    { 0x63, 0xFFFF, 0xFFFF, 0xF403, 0xB6FF, 0xB7FF, 0xE303, 0xFFFF, 0xFFFF} ,
    // 'd' 0x64
    { 0x64, 0xFFF3, 0xFFF3, 0xE203, 0x79F3, 0x7AC3, 0xC036, 0xFFFF, 0xFFFF} ,
    // 'e' 0x65
    { 0x65, 0xFFFF, 0xFFFF, 0xE207, 0x7000, 0x79FF, 0xD103, 0xFFFF, 0xFFFF} ,
    // 'f' 0x66
    { 0x66, 0xFD10, 0xF7AF, 0x3003, 0xF7BF, 0xF7BF, 0xF7BF, 0xFFFF, 0xFFFF} ,
    // 'g' 0x67
    { 0x67, 0xFFFF, 0xFFFF, 0xE100, 0xB9D3, 0xC009, 0x9002, 0x7AF2, 0xA006} ,
    // 'h' 0x68
    { 0x68, 0x7BFF, 0x7BFF, 0x7306, 0x75F3, 0x7BF3, 0x7BF3, 0xFFFF, 0xFFFF} ,
    // 'i' 0x69
    { 0x69, 0xFB5F, 0xFFFF, 0xB03F, 0xFF3F, 0xFF3F, 0xB003, 0xFFFF, 0xFFFF} ,
    // 'j' 0x6A
    { 0x6A, 0xFF59, 0xFFFF, 0xB00B, 0xFF7B, 0xFF7B, 0xFF7B, 0xFF6B, 0x703F} ,
    // 'k' 0x6B
    { 0x6B, 0xB7FF, 0xB7FF, 0xB795, 0xB26F, 0xB64E, 0xB7C4, 0xFFFF, 0xFFFF} ,
    // 'l' 0x6C
    { 0x6C, 0xB03F, 0xFF3F, 0xFF3F, 0xFF3F, 0xFF3F, 0xB003, 0xFFFF, 0xFFFF} ,
    // 'm' 0x6D
    { 0x6D, 0xFFFF, 0xFFFF, 0x3230, 0x3B27, 0x3F37, 0x3F37, 0xFFFF, 0xFFFF} ,
    // 'n' 0x6E
    { 0x6E, 0xFFFF, 0xFFFF, 0x7306, 0x75F3, 0x7BF3, 0x7BF3, 0xFFFF, 0xFFFF} ,
    // 'o' 0x6F
    { 0x6F, 0xFFFF, 0xFFFF, 0xD107, 0x7CE2, 0x7CE2, 0xC109, 0xFFFF, 0xFFFF} ,
    // 'p' 0x70
    { 0x70, 0xFFFF, 0xFFFF, 0x7306, 0x77F2, 0x7BE2, 0x7009, 0x7BFF, 0x7BFF} ,
    // 'q' 0x71
    { 0x71, 0xFFFF, 0xFFFF, 0xE203, 0x79F3, 0x7AC3, 0xC023, 0xFFF3, 0xFFF3} ,
    // 'r' 0x72
    { 0x72, 0xFFFF, 0xFFFF, 0xB404, 0xB2E2, 0xB7FF, 0xB7FF, 0xFFFF, 0xFFFF} ,
    // 's' 0x73
    { 0x73, 0xFFFF, 0xFFFF, 0xE207, 0xD3AF, 0xFE94, 0xB008, 0xFFFF, 0xFFFF} ,
    // 't' 0x74
    { 0x74, 0xFFFF, 0xF4FF, 0x3003, 0xF3FF, 0xF3EF, 0xFA03, 0xFFFF, 0xFFFF} ,
    // 'u' 0x75
    { 0x75, 0xFFFF, 0xFFFF, 0x7BF3, 0x7BF3, 0x99C3, 0xD015, 0xFFFF, 0xFFFF} ,
    // 'v' 0x76
    { 0x76, 0xFFFF, 0xFFFF, 0x6CF5, 0xD7E6, 0xF69C, 0xF97F, 0xFFFF, 0xFFFF} ,
    // 'w' 0x77
    { 0x77, 0xFFFF, 0xFFFF, 0x5FF7, 0x5D67, 0x7A96, 0xA5C4, 0xFFFF, 0xFFFF} ,
    // 'x' 0x78
    { 0x78, 0xFFFF, 0xFFFF, 0xB7E4, 0xF75E, 0xF77D, 0xA7E3, 0xFFFF, 0xFFFF} ,
    // 'y' 0x79
    { 0x79, 0xFFFF, 0xFFFF, 0x7CF4, 0xD6D7, 0xF68D, 0xFA5F, 0xF89F, 0x33FF} ,
    // 'z' 0x7A
    { 0x7A, 0xFFFF, 0xFFFF, 0xB006, 0xFF8F, 0xFCEF, 0xA003, 0xFFFF, 0xFFFF} ,
    // '{' 0x7B
    { 0x7B, 0xFE17, 0xFB6F, 0xFA9F, 0x70DF, 0xFAAF, 0xFB7F, 0xFB6F, 0xFE27} ,
    // '|' 0x7C
    { 0x7C, 0xFB7F, 0xFB7F, 0xFB7F, 0xFB7F, 0xFB7F, 0xFB7F, 0xFB7F, 0xFB7F} ,
    // '} ,' 0x7D
    { 0x7D, 0xB09F, 0xFE3F, 0xFF2F, 0xFF53, 0xFF3F, 0xFF3F, 0xFE3F, 0xB0AF} ,
    // '~' 0x7E
    { 0x7E, 0xFFFF, 0xFFFF, 0xFFFF, 0xA096, 0x3E22, 0xFFFF, 0xFFFF, 0xFFFF}
};

TinyTextTFT::TinyTextTFT(int8_t _CS, int8_t _DC, int8_t _RST)
{
    _spi = &SPI;
    _width = ILI9341_TFTWIDTH;
    _height = ILI9341_TFTHEIGHT;
    _rotation = 0;
    _cs = _CS;
    _dc = _DC;
    _rst = _RST;

    _cellWidth = 5;
    _cellHeight = 10;

    _rows    = _height / _cellHeight;
    _columns = _width / _cellWidth;
    
    for (int i = 0; i < 256; i++)
    {
        fontMap[i] = 0;
    }
    uint16_t chr;
    const uint16_t* addr = (const uint16_t*)tinyFont;
    uint8_t index = 0;
    for (;;)
    {
        chr = pgm_read_word(addr);
        addr += 9;
        fontMap[(uint8_t)chr] = index;
        if (chr == 0x007E)
        {
            break;
        }
        index++;
    }
}

void TinyTextTFT::startWrite(void)
{
    _spi->beginTransaction(_settings);
    digitalWrite(_cs, LOW);
}
void TinyTextTFT::endWrite(void)
{
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}

void TinyTextTFT::spiWrite(uint8_t b)
{
    _spi->write(b);
}

void TinyTextTFT::spiWrite16(uint16_t w)
{
    _spi->write16(w);
}

uint8_t TinyTextTFT::spiRead(void)
{
    return _spi->transfer((uint8_t)0);
}

void TinyTextTFT::writeCommand(uint8_t cmd)
{
    digitalWrite(_dc, LOW);
    spiWrite(cmd);
    digitalWrite(_dc, HIGH);
}

void TinyTextTFT::sendCommand(uint8_t commandByte, uint8_t* dataBytes, uint8_t numDataBytes)
{
    _spi->beginTransaction(_settings);
    digitalWrite(_cs, LOW);

    digitalWrite(_dc, LOW); // Command mode
    spiWrite(commandByte);  // Send the command byte

    digitalWrite(_dc, HIGH);
    for (int i = 0; i < numDataBytes; i++)
    {
        spiWrite(*dataBytes); // Send the data bytes
        dataBytes++;
    }

    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}
void TinyTextTFT::sendCommand(uint8_t commandByte, const uint8_t* dataBytes, uint8_t numDataBytes)
{
    _spi->beginTransaction(_settings);
    digitalWrite(_cs, LOW);

    digitalWrite(_dc, LOW); // Command mode
    spiWrite(commandByte);  // Send the command byte

    digitalWrite(_dc, HIGH);
    for (int i = 0; i < numDataBytes; i++)
    {
        spiWrite(pgm_read_byte(dataBytes++));
    }
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}
/*
uint8_t TinyTextTFT::ReadCommand8(uint8_t commandByte, uint8_t index)
{
    uint8_t data = 0x10 + index;
    sendCommand(0xD9, &data, 1); // Set Index Register
    uint8_t result;
    startWrite();
    digitalWrite(_dc, LOW); // Command mode
    spiWrite(commandByte);
    digitalWrite(_dc, HIGH); // Data mode
    do
    {
        result = spiRead();
    }
    while (index--); // Discard bytes up to index'th
    endWrite();
    return result;
}
*/

void TinyTextTFT::initSPI(uint32_t freq)
{
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH); // Deselect
    pinMode(_dc, OUTPUT);
    digitalWrite(_dc, HIGH); // Data mode

    _settings = SPISettings(freq, MSBFIRST, SPI_MODE0);
    _spi->begin();

    if (_rst >= 0)
    {
        // hardware reset
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, HIGH);
        delay(100);
        digitalWrite(_rst, LOW);
        delay(100);
        digitalWrite(_rst, HIGH);
        delay(200);
    }
}


void TinyTextTFT::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h)
{
    static uint16_t old_x1 = 0xffff, old_x2 = 0xffff;
    static uint16_t old_y1 = 0xffff, old_y2 = 0xffff;

    uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
    if (x1 != old_x1 || x2 != old_x2)
    {
        writeCommand(ILI9341_CASET); // Column address set
        spiWrite16(x1);
        spiWrite16(x2);
        old_x1 = x1;
        old_x2 = x2;
    }
    if (y1 != old_y1 || y2 != old_y2)
    {
        writeCommand(ILI9341_PASET); // Row address set
        spiWrite16(y1);
        spiWrite16(y2);
        old_y1 = y1;
        old_y2 = y2;
    }
    writeCommand(ILI9341_RAMWR); // Write to RAM
}
/*
void TinyTextTFT::writePixel(int16_t x, int16_t y, uint16_t color)
{
    // Clip first...
    if ((x >= 0) && (x < _width) && (y >= 0) && (y < _height))
    {
        // THEN set up transaction (if needed) and draw...
        startWrite();
        setAddrWindow(x, y, 1, 1);
        spiWrite16(color);
        endWrite();
    }
}
void TinyTextTFT::writeColor(uint16_t color, uint32_t len)
{
    if (!len)
    {
        return; // Avoid 0-byte transfers
    }
    do
    {
        uint32_t pixelsThisPass = len;
        if (pixelsThisPass > 50000)
        {
            pixelsThisPass = 50000;
        }
        len -= pixelsThisPass;
        yield(); // Periodic yield() on long fills
        while (pixelsThisPass--)
        {
            _spi->write16(color, true);
        }
    }
    while (len);
}
*/
void TinyTextTFT::writeColor32(uint32_t color, uint32_t len)
{
    if (!len)
    {
        return; // Avoid 0-byte transfers
    }
    do
    {
        uint32_t pixelsThisPass = len;
        if (pixelsThisPass > 50000)
        {
            pixelsThisPass = 50000;
        }
        len -= pixelsThisPass;
        yield(); // Periodic yield() on long fills
        while (pixelsThisPass--)
        {
            _spi->write32(color, false);
        }
    }
    while (len);
}


uint32_t TinyTextTFT::makeColor32(uint16_t color0, uint16_t color1)
{
    // flip bytes so we can call _spi->write32(color, false); with false
    uint32_t color32 = ((color0 >> 8) | ((color0 & 0xFF) << 8) | ((color1 & 0xFF00) << 8) | ((color1 & 0xFF) << 24));
    return color32;
}

uint16_t TinyTextTFT::blendPixelColor(uint8_t tone, uint16_t foreColor, uint16_t backColor)
{
    // tone: 0..15
    //const uint8_t* taddr = (const uint8_t*)toneToShade;
    //uint8_t shade = pgm_read_word(taddr + tone); // not a linear conversion
    uint8_t shade = toneToShade[tone];
    uint8_t shade255 = 255 - shade;
    // shade: 0..255

    uint8_t rForeground = (foreColor >> 8);
    uint8_t gForeground = ((foreColor >> 4) & (0x0F));
    uint8_t bForeground = (foreColor & 0x0F);

    uint8_t rBackground = (backColor >> 8);
    uint8_t gBackground = ((backColor >> 4) & (0x0F));
    uint8_t bBackground = (backColor & 0x0F);

    uint16_t rBlended = ((shade255 * rForeground) + (shade * rBackground)) >> 8;
    uint16_t gBlended = ((shade255 * gForeground) + (shade * gBackground)) >> 8;
    uint16_t bBlended = ((shade255 * bForeground) + (shade * bBackground)) >> 8;

    uint16_t c565 = (uint16_t)((rBlended << 12) + (gBlended << 7) + (bBlended << 1));
    if (rBlended & 0x01 != 0)
    {
        c565 |= 0x0800;
    }
    if (gBlended & 0x01 != 0)
    {
        c565 |= 0x0060;
    }
    if (bBlended & 0x01 != 0)
    {
        c565 |= 0x0001;
    }
    return c565;
}
uint16_t TinyTextTFT::convertToRGB565(uint16_t rgb444)
{
    uint8_t rColor = (rgb444 >> 8);
    uint8_t gColor = ((rgb444 >> 4) & (0x0F));
    uint8_t bColor = (rgb444 & 0x0F);
    uint16_t c565 = (uint16_t)((rColor << 12) + (gColor << 7) + (bColor << 1));
    if (rColor & 0x01 != 0)
    {
        c565 |= 0x0800;
    }
    if (gColor & 0x01 != 0)
    {
        c565 |= 0x0060;
    }
    if (bColor & 0x01 != 0)
    {
        c565 |= 0x0001;
    }
    return c565;
}

void TinyTextTFT::Begin()
{
    initSPI(SPI_DEFAULT_FREQ);
    if (_rst < 0)
    {
        // software reset
        sendCommand(ILI9341_SWRESET); // Engage software reset
        delay(150);
    }

    uint8_t cmd, x, numArgs;
    const uint8_t* addr = initcmd;
    while ((cmd = pgm_read_byte(addr++)) > 0)
    {
        x = pgm_read_byte(addr++);
        numArgs = x & 0x7F;
        sendCommand(cmd, addr, numArgs);
        addr += numArgs;
        if (x & 0x80)
        {
            delay(150);
        }
    }
}

void TinyTextTFT::FillScreen(uint16_t color)
{
    color = convertToRGB565(color);
    startWrite();
    setAddrWindow(0, 0, _width, _height);
    uint32_t color32 = makeColor32(color, color);
    writeColor32(color32, (uint32_t)_width * _height / 2);
    endWrite();
}

void TinyTextTFT::DrawChar(uint8_t col, uint8_t row, char chr, uint16_t foreColor, uint16_t backColor)
{
    uint8_t index = fontMap[chr]; // if it is missing from the font, index will be 0 (' ')
    if ((col >= _columns) || (row >= _rows))
    {
        return; // clipping
    }
    int16_t x0 = col * _cellWidth;
    int16_t y0 = row * _cellHeight;

    startWrite();

    const uint16_t* addr = (const uint16_t*)tinyFont;
    addr = addr + (9 * index);
    addr++;

    setAddrWindow(x0, y0, 5, 10); // <-- this is slow

    uint16_t pixelb = blendPixelColor(0xF, backColor, backColor);
    uint32_t backColor32 = ((pixelb >> 8) | ((pixelb & 0xFF) << 8) | ((pixelb & 0xFF00) << 8) | ((pixelb & 0xFF) << 24));
    uint16_t backColor16 = ((pixelb >> 8) | ((pixelb & 0xFF) << 8));

    // top row
    _spi->write32(backColor32, false);
    _spi->write32(backColor32, false);
    _spi->write16(backColor16, false);

    uint8_t tonel = 16; // not possible
    uint16_t pixell = 0;
    for (int y = 0; y < 8; y++)
    {
        uint16_t tones = pgm_read_word(addr);
        addr++;

        uint8_t tone0 = ((tones >> 12) & 0x0F);
        uint8_t tone1 = ((tones >> 8) & 0x0F);
        uint16_t pixel0 = (tonel == tone0) ? pixell : blendPixelColor(tone0, foreColor, backColor);
        uint16_t pixel1 = (tone0 == tone1) ? pixel0 : blendPixelColor(tone1, foreColor, backColor);
        uint32_t pixelColor32 = ((pixel0 >> 8) | ((pixel0 & 0xFF) << 8) | ((pixel1 & 0xFF00) << 8) | ((pixel1 & 0xFF) << 24));
        _spi->write32(pixelColor32, false);

        uint8_t tone2 = ((tones >> 4) & 0x0F);
        uint8_t tone3 = (tones & 0x0F);
        uint16_t pixel2 = (tone1 == tone2) ? pixel1 : blendPixelColor(tone2, foreColor, backColor);
        uint16_t pixel3 = (tone2 == tone3) ? pixel2 : blendPixelColor(tone3, foreColor, backColor);
        pixelColor32 = ((pixel2 >> 8) | ((pixel2 & 0xFF) << 8) | ((pixel3 & 0xFF00) << 8) | ((pixel3 & 0xFF) << 24));
        _spi->write32(pixelColor32, false);

        //right column
        _spi->write16(backColor16, false);

        pixell = pixel3;
        tonel = tone3;
    }
    
    // bottom row
    _spi->write32(backColor32, false);
    _spi->write32(backColor32, false);
    _spi->write16(backColor16, false);

    endWrite();
}

void TinyTextTFT::SetRotation(uint8_t m)
{
    _rotation = m % 4; // can't be higher than 3
    switch (_rotation)
    {
        case 0:
            m = (MADCTL_MX | MADCTL_BGR);
            _width = ILI9341_TFTWIDTH;
            _height = ILI9341_TFTHEIGHT;
            break;
        case 1:
            m = (MADCTL_MV | MADCTL_BGR);
            _width = ILI9341_TFTHEIGHT;
            _height = ILI9341_TFTWIDTH;
            break;
        case 2:
            m = (MADCTL_MY | MADCTL_BGR);
            _width = ILI9341_TFTWIDTH;
            _height = ILI9341_TFTHEIGHT;
            break;
        case 3:
            m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            _width = ILI9341_TFTHEIGHT;
            _height = ILI9341_TFTWIDTH;
            break;
    }
    _rows = _height / _cellHeight;
    _columns = _width / _cellWidth;
    sendCommand(ILI9341_MADCTL, &m, 1);
}