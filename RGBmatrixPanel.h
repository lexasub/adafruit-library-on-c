#ifndef RGBMATRIXPANEL_H
#define RGBMATRIXPANEL_H
#define F_CPU 8000000UL
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "glcdfont.c"
#include "gfxfont.h"
#include "gamma.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
#define BYTE 0
#define delay _delay_ms
void _delay_ms(double ms);

uint8_t         *matrixbuff[2];
uint8_t          nRows;
volatile uint8_t backindex;
volatile bool swapflag;
int16_t
    WIDTH,          ///< This is the 'raw' display width - never changes
    HEIGHT,         ///< This is the 'raw' display height - never changes
    _width,         ///< Display width as modified by current rotation
    _height,        ///< Display height as modified by current rotation
    cursor_x,       ///< x location to start print()ing text
    cursor_y;       ///< y location to start print()ing text
uint16_t
    textcolor,      ///< 16-bit background color for print()
    textbgcolor,    ///< 16-bit text color for print()
    textsize_x,      ///< Desired magnification in X-axis of text to print()
    textsize_y,      ///< Desired magnification in Y-axis of text to print()
    rotation;       ///< Display rotation (0 thru 3)
bool 
	wrap,           ///< If set, 'wrap' text at right edge of display
    _cp437;         ///< If set, use correct CP437 charset (default is off)
GFXfont *gfxFont;       ///< Pointer to special font
    
volatile uint8_t row, plane;
volatile uint8_t *buffptr;

void RGBmatrixPanel_printNumber(unsigned long, uint8_t);
size_t RGBmatrixPanel_write(uint8_t c);
void RGBmatrixPanel_write(const char *str);
void RGBmatrixPanel_write(const uint8_t *buffer, size_t size);

void RGBmatrixPanel_print(const char[]);
void RGBmatrixPanel_print(char, int = BYTE);
void RGBmatrixPanel_print(unsigned char, int = BYTE);
void RGBmatrixPanel_print(int, int = DEC);
void RGBmatrixPanel_print(unsigned int, int = DEC);
void RGBmatrixPanel_print(long, int = DEC);
void RGBmatrixPanel_print(unsigned long, int = DEC);
void RGBmatrixPanel_println(const char[]);
void RGBmatrixPanel_println(char, int = BYTE);
void RGBmatrixPanel_println(unsigned char, int = BYTE);
void RGBmatrixPanel_println(int, int = DEC);
void RGBmatrixPanel_println(unsigned int, int = DEC);
void RGBmatrixPanel_println(long, int = DEC);
void RGBmatrixPanel_println(unsigned long, int = DEC);
void RGBmatrixPanel_println(void);

void RGBmatrixPanel_Adafruit_GFX(int16_t w, int16_t h); 

// TRANSACTION API / CORE DRAW API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
void RGBmatrixPanel_startWrite(void);
void RGBmatrixPanel_writePixel(int16_t x, int16_t y, uint16_t color);
void RGBmatrixPanel_writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void RGBmatrixPanel_writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void RGBmatrixPanel_writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void RGBmatrixPanel_writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void RGBmatrixPanel_endWrite(void);

// CONTROL API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
void RGBmatrixPanel_setRotation(uint8_t r);
void RGBmatrixPanel_invertDisplay(bool i);

// BASIC DRAW API
// These MAY be overridden by the subclass to provide device-specific
// optimized code.  Otherwise 'generic' versions are used.
void
// It's good to implement those, even if using transaction API
RGBmatrixPanel_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
RGBmatrixPanel_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
RGBmatrixPanel_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color),
// Optional and probably not necessary to change
RGBmatrixPanel_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color),
RGBmatrixPanel_drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// These exist only with Adafruit_GFX (no subclass overrides)
void
RGBmatrixPanel_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
RGBmatrixPanel_drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
  uint16_t color),
RGBmatrixPanel_fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color),
RGBmatrixPanel_fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
  int16_t delta, uint16_t color),
RGBmatrixPanel_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
  int16_t x2, int16_t y2, uint16_t color),
RGBmatrixPanel_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
  int16_t x2, int16_t y2, uint16_t color),
RGBmatrixPanel_drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
  int16_t radius, uint16_t color),
RGBmatrixPanel_fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
  int16_t radius, uint16_t color),
RGBmatrixPanel_drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
  int16_t w, int16_t h, uint16_t color),
RGBmatrixPanel_drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
  int16_t w, int16_t h, uint16_t color, uint16_t bg),
RGBmatrixPanel_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,
  int16_t w, int16_t h, uint16_t color),
RGBmatrixPanel_drawBitmap(int16_t x, int16_t y, uint8_t *bitmap,
  int16_t w, int16_t h, uint16_t color, uint16_t bg),
RGBmatrixPanel_drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
  int16_t w, int16_t h, uint16_t color),
RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
  int16_t w, int16_t h),
RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap,
  int16_t w, int16_t h),
RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h),
RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h),
RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
  int16_t w, int16_t h),
RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap,
  int16_t w, int16_t h),
RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h),
RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h),
RGBmatrixPanel_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
  uint16_t bg, uint8_t size),
RGBmatrixPanel_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size_x, uint8_t size_y),
RGBmatrixPanel_getTextBounds(const char *string, int16_t x, int16_t y,
  int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h),
RGBmatrixPanel_setTextSize(uint8_t s),
RGBmatrixPanel_setTextSize(uint8_t sx, uint8_t sy),
RGBmatrixPanel_setFont(const GFXfont *f = NULL);

/**********************************************************************/
/*!
@brief  Set text cursor location
@param  x    X coordinate in pixels
@param  y    Y coordinate in pixels
*/
/**********************************************************************/
void RGBmatrixPanel_setCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; }

/**********************************************************************/
/*!
@brief   Set text font color with transparant background
@param   c   16-bit 5-6-5 Color to draw text with
@note    For 'transparent' background, background and foreground
         are set to same color rather than using a separate flag.
*/
/**********************************************************************/
void RGBmatrixPanel_setTextColor(uint16_t c) { textcolor = textbgcolor = c; }

/**********************************************************************/
/*!
@brief   Set text font color with custom background color
@param   c   16-bit 5-6-5 Color to draw text with
@param   bg  16-bit 5-6-5 Color to draw background/fill with
*/
/**********************************************************************/
void RGBmatrixPanel_setTextColor(uint16_t c, uint16_t bg) { textcolor = c; textbgcolor = bg;}

/**********************************************************************/
/*!
@brief  Set whether text that is too long for the screen width should
      automatically wrap around to the next line (else clip right).
@param  w  true for wrapping, false for clipping
*/
/**********************************************************************/
void RGBmatrixPanel_setTextWrap(bool w) { wrap = w; }

/**********************************************************************/
/*!
@brief  Enable (or disable) Code Page 437-compatible charset.
        There was an error in glcdfont.c for the longest time -- one
        character (#176, the 'light shade' block) was missing -- this
        threw off the index of every character that followed it.
        But a TON of code has been written with the erroneous
        character indices. By default, the library uses the original
        'wrong' behavior and old sketches will still work. Pass
        'true' to this function to use correct CP437 character values
        in your code.
@param  x  true = enable (new behavior), false = disable (old behavior)
*/
/**********************************************************************/
void RGBmatrixPanel_cp437(bool x=true) { _cp437 = x; }


/************************************************************************/
/*!
@brief      Get width of the display, accounting for current rotation
@returns    Width in pixels
*/
/************************************************************************/
int16_t RGBmatrixPanel_width(void) { return _width; };

/************************************************************************/
/*!
@brief      Get height of the display, accounting for current rotation
@returns    Height in pixels
*/
/************************************************************************/
int16_t RGBmatrixPanel_height(void) { return _height; }

/************************************************************************/
/*!
@brief      Get rotation setting for display
@returns    0 thru 3 corresponding to 4 cardinal rotations
*/
/************************************************************************/
uint8_t RGBmatrixPanel_getRotation(void) { return rotation; }

// get current cursor position (get rotation safe maximum values,
// using: width() for x, height() for y)
/************************************************************************/
/*!
@brief  Get text cursor X location
@returns    X coordinate in pixels
*/
/************************************************************************/
int16_t RGBmatrixPanel_getCursorX(void) { return cursor_x; }

/************************************************************************/
/*!
@brief      Get text cursor Y location
@returns    Y coordinate in pixels
*/
/************************************************************************/
int16_t RGBmatrixPanel_getCursorY(void) { return cursor_y; };

void RGBmatrixPanel_charBounds(char c, int16_t *x, int16_t *y,
  int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy);

// Constructor for 16x32 panel:
void RGBmatrixPanel_RGBmatrixPanel(bool dbuf);

// Constructor for 32x32 panel (adds 'd' pin):
void RGBmatrixPanel_RGBmatrixPanel(bool dbuf, uint8_t width=32);

void
RGBmatrixPanel_begin(void),
RGBmatrixPanel_drawPixel(int16_t x, int16_t y, uint16_t c),
RGBmatrixPanel_fillScreen(uint16_t c),
RGBmatrixPanel_updateDisplay(void),
RGBmatrixPanel_swapBuffers(bool);
uint8_t
*RGBmatrixPanel_backBuffer(void);
uint16_t
RGBmatrixPanel_Color333(uint8_t r, uint8_t g, uint8_t b),
RGBmatrixPanel_Color444(uint8_t r, uint8_t g, uint8_t b),
RGBmatrixPanel_Color888(uint8_t r, uint8_t g, uint8_t b),
RGBmatrixPanel_Color888(uint8_t r, uint8_t g, uint8_t b, bool gflag),
RGBmatrixPanel_ColorHSV(long hue, uint8_t sat, uint8_t val, bool gflag);

// Init/alloc code common to both constructors:
void RGBmatrixPanel_init(uint8_t rows, bool dbuf, uint8_t width);

#endif // RGBMATRIXPANEL_H
