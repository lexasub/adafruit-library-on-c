/*
RGBmatrixPanel Arduino library for Adafruit 16x32 and 32x32 RGB LED
matrix panels.  Pick one up at:
  http://www.adafruit.com/products/420
  http://www.adafruit.com/products/607

This version uses a few tricks to achieve better performance and/or
lower CPU utilization:

- To control LED brightness, traditional PWM is eschewed in favor of
  Binary Code Modulation, which operates through a succession of periods
  each twice the length of the preceeding one (rather than a direct
  linear count a la PWM).  It's explained well here:

    http://www.batsocks.co.uk/readme/art_bcm_1.htm

  I was RGBmatrixPanel_initially skeptical, but it works exceedingly well in practice!
  And this uses considerably fewer CPU cycles than software PWM.

- Although many control pins are software-configurable in the user's
  code, a couple things are tied to specific PORT registers.  It's just
  a lot faster this way -- port lookups take time.  Please see the notes
  later regarding wiring on "alternative" Arduino boards.

- A tiny bit of inline assembly language is used in the most speed-
  critical section.  The C++ compiler wasn't making optimal use of the
  instruction RGBmatrixPanel_set in what seemed like an obvious chunk of code.  Since
  it's only a few short instructions, this loop is also "unrolled" --
  each iteration is stated explicitly, not through a control loop.

Written by Limor Fried/Ladyada & Phil Burgess/PaintYourDragon for
Adafruit Industries.
BSD license, all text above must be included in any redistribution.
*/

#include "RGBmatrixPanel.h"


// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
 #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
 #define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
 #define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
 #define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

inline GFXglyph * pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c)
{
    return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
}

inline uint8_t * pgm_read_bitmap_ptr(const GFXfont *gfxFont)
{
    return (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);
}

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

void RGBmatrixPanel_write(const char *str)
{
  while (*str)
    RGBmatrixPanel_write(*str++);
}

/* default implementation: may be overridden */
void RGBmatrixPanel_write(const uint8_t *buffer, size_t size)
{
  while (size--)
    RGBmatrixPanel_write(*buffer++);
}

void RGBmatrixPanel_print(const char str[])
{
  RGBmatrixPanel_write(str);
}

void RGBmatrixPanel_print(char c, int base)
{
  RGBmatrixPanel_print((long) c, base);
}

void RGBmatrixPanel_print(unsigned char b, int base)
{
  RGBmatrixPanel_print((unsigned long) b, base);
}

void RGBmatrixPanel_print(int n, int base)
{
  RGBmatrixPanel_print((long) n, base);
}

void RGBmatrixPanel_print(unsigned int n, int base)
{
  RGBmatrixPanel_print((unsigned long) n, base);
}

void RGBmatrixPanel_print(long n, int base)
{
  if (base == 0) {
    RGBmatrixPanel_write(n);
  } else if (base == 10) {
    if (n < 0) {
      RGBmatrixPanel_print('-');
      n = -n;
    }
    RGBmatrixPanel_printNumber(n, 10);
  } else {
    RGBmatrixPanel_printNumber(n, base);
  }
}

void RGBmatrixPanel_print(unsigned long n, int base)
{
  if (base == 0) RGBmatrixPanel_write(n);
  else RGBmatrixPanel_printNumber(n, base);
}

void RGBmatrixPanel_println(void)
{
  RGBmatrixPanel_print('\r');
  RGBmatrixPanel_print('\n');  
}

void RGBmatrixPanel_println(const char c[])
{
  RGBmatrixPanel_print(c);
  RGBmatrixPanel_println();
}

void RGBmatrixPanel_println(char c, int base)
{
  RGBmatrixPanel_print(c, base);
  RGBmatrixPanel_println();
}

void RGBmatrixPanel_println(unsigned char b, int base)
{
  RGBmatrixPanel_print(b, base);
  RGBmatrixPanel_println();
}

void RGBmatrixPanel_println(int n, int base)
{
  RGBmatrixPanel_print(n, base);
  RGBmatrixPanel_println();
}

void RGBmatrixPanel_println(unsigned int n, int base)
{
  RGBmatrixPanel_print(n, base);
  RGBmatrixPanel_println();
}

void RGBmatrixPanel_println(long n, int base)
{
  RGBmatrixPanel_print(n, base);
  RGBmatrixPanel_println();
}

void RGBmatrixPanel_println(unsigned long n, int base)
{
  RGBmatrixPanel_print(n, base);
  RGBmatrixPanel_println();
}

// Private Methods /////////////////////////////////////////////////////////////

void RGBmatrixPanel_printNumber(unsigned long n, uint8_t base)
{
  unsigned char buf[8 * sizeof(long)]; // Assumes 8-bit chars. 
  unsigned long i = 0;

  if (n == 0) {
    RGBmatrixPanel_print('0');
    return;
  } 

  while (n > 0) {
    buf[i++] = n % base;
    n /= base;
  }

  for (; i > 0; i--)
    RGBmatrixPanel_print((char) (buf[i - 1] < 10 ?
      '0' + buf[i - 1] :
      'A' + buf[i - 1] - 10));
}

/**************************************************************************/
/*!
   @brief    Instatiate a GFX context for graphics! Can only be done by a superclass
   @param    w   Display width, in pixels
   @param    h   Display height, in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_Adafruit_GFX(int16_t w, int16_t h)
{
	WIDTH = w;
	HEIGHT = h;
    _width    = WIDTH;
    _height   = HEIGHT;
    rotation  = 0;
    cursor_y  = cursor_x    = 0;
    textsize_x = textsize_y  = 1;
    textcolor = textbgcolor = 0xFFFF;
    wrap      = true;
    _cp437    = false;
    gfxFont   = NULL;
}

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
        uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            RGBmatrixPanel_writePixel(y0, x0, color);
        } else {
            RGBmatrixPanel_writePixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**************************************************************************/
/*!
   @brief    Start a display-writing routine, overwrite in subclasses.
*/
/**************************************************************************/
void RGBmatrixPanel_startWrite(){
}

/**************************************************************************/
/*!
   @brief    Write a pixel, overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
    @param   x   x coordinate
    @param   y   y coordinate
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_writePixel(int16_t x, int16_t y, uint16_t color){
    RGBmatrixPanel_drawPixel(x, y, color);
}

/**************************************************************************/
/*!
   @brief    Write a perfectly vertical line, overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_writeFastVLine(int16_t x, int16_t y,
        int16_t h, uint16_t color) {
    // Overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
    // Can be just RGBmatrixPanel_writeLine(x, y, x, y+h-1, color);
    // or RGBmatrixPanel_writeFillRect(x, y, 1, h, color);
    RGBmatrixPanel_drawFastVLine(x, y, h, color);
}

/**************************************************************************/
/*!
   @brief    Write a perfectly horizontal line, overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_writeFastHLine(int16_t x, int16_t y,
        int16_t w, uint16_t color) {
    // Overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
    // Example: RGBmatrixPanel_writeLine(x, y, x+w-1, y, color);
    // or RGBmatrixPanel_writeFillRect(x, y, w, 1, color);
    RGBmatrixPanel_drawFastHLine(x, y, w, color);
}

/**************************************************************************/
/*!
   @brief    Write a rectangle completely with one color, overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
        uint16_t color) {
    // Overwrite in subclasses if desired!
    RGBmatrixPanel_fillRect(x,y,w,h,color);
}

/**************************************************************************/
/*!
   @brief    End a display-writing routine, overwrite in subclasses if RGBmatrixPanel_startWrite is defined!
*/
/**************************************************************************/
void RGBmatrixPanel_endWrite(){
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly vertical line (this is often optimized in a subclass!)
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_drawFastVLine(int16_t x, int16_t y,
        int16_t h, uint16_t color) {
    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writeLine(x, y, x, y+h-1, color);
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly horizontal line (this is often optimized in a subclass!)
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_drawFastHLine(int16_t x, int16_t y,
        int16_t w, uint16_t color) {
    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writeLine(x, y, x+w-1, y, color);
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
        uint16_t color) {
    RGBmatrixPanel_startWrite();
    for (int16_t i=x; i<x+w; i++) {
        RGBmatrixPanel_writeFastVLine(i, y, h, color);
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief    Draw a line
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
        uint16_t color) {
    // Update in subclasses if desired!
    if(x0 == x1){
        if(y0 > y1) _swap_int16_t(y0, y1);
        RGBmatrixPanel_drawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if(y0 == y1){
        if(x0 > x1) _swap_int16_t(x0, x1);
        RGBmatrixPanel_drawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        RGBmatrixPanel_startWrite();
        RGBmatrixPanel_writeLine(x0, y0, x1, y1, color);
        RGBmatrixPanel_endWrite();
    }
}

/**************************************************************************/
/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawCircle(int16_t x0, int16_t y0, int16_t r,
        uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writePixel(x0  , y0+r, color);
    RGBmatrixPanel_writePixel(x0  , y0-r, color);
    RGBmatrixPanel_writePixel(x0+r, y0  , color);
    RGBmatrixPanel_writePixel(x0-r, y0  , color);

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        RGBmatrixPanel_writePixel(x0 + x, y0 + y, color);
        RGBmatrixPanel_writePixel(x0 - x, y0 + y, color);
        RGBmatrixPanel_writePixel(x0 + x, y0 - y, color);
        RGBmatrixPanel_writePixel(x0 - x, y0 - y, color);
        RGBmatrixPanel_writePixel(x0 + y, y0 + x, color);
        RGBmatrixPanel_writePixel(x0 - y, y0 + x, color);
        RGBmatrixPanel_writePixel(x0 + y, y0 - x, color);
        RGBmatrixPanel_writePixel(x0 - y, y0 - x, color);
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
    @brief    Quarter-circle RGBmatrixPanel_drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of the circle we're doing
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawCircleHelper( int16_t x0, int16_t y0,
        int16_t r, uint8_t cornername, uint16_t color) {
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {
            RGBmatrixPanel_writePixel(x0 + x, y0 + y, color);
            RGBmatrixPanel_writePixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            RGBmatrixPanel_writePixel(x0 + x, y0 - y, color);
            RGBmatrixPanel_writePixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            RGBmatrixPanel_writePixel(x0 - y, y0 + x, color);
            RGBmatrixPanel_writePixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            RGBmatrixPanel_writePixel(x0 - y, y0 - x, color);
            RGBmatrixPanel_writePixel(x0 - x, y0 - y, color);
        }
    }
}

/**************************************************************************/
/*!
   @brief    Draw a circle with RGBmatrixPanel_filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_fillCircle(int16_t x0, int16_t y0, int16_t r,
        uint16_t color) {
    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writeFastVLine(x0, y0-r, 2*r+1, color);
    RGBmatrixPanel_fillCircleHelper(x0, y0, r, 3, 0, color);
    RGBmatrixPanel_endWrite();
}


/**************************************************************************/
/*!
    @brief  Quarter-circle RGBmatrixPanel_drawer with RGBmatrixPanel_fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to RGBmatrixPanel_fill with
*/
/**************************************************************************/
void RGBmatrixPanel_fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
  uint8_t corners, int16_t delta, uint16_t color) {

    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    int16_t px    = x;
    int16_t py    = y;

    delta++; // Avoid some +1's in the loop

    while(x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT RGBmatrixPanel_drawing mode.
        if(x < (y + 1)) {
            if(corners & 1) RGBmatrixPanel_writeFastVLine(x0+x, y0-y, 2*y+delta, color);
            if(corners & 2) RGBmatrixPanel_writeFastVLine(x0-x, y0-y, 2*y+delta, color);
        }
        if(y != py) {
            if(corners & 1) RGBmatrixPanel_writeFastVLine(x0+py, y0-px, 2*px+delta, color);
            if(corners & 2) RGBmatrixPanel_writeFastVLine(x0-py, y0-px, 2*px+delta, color);
            py = y;
        }
        px = x;
    }
}

/**************************************************************************/
/*!
   @brief   Draw a rectangle with no RGBmatrixPanel_fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
        uint16_t color) {
    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writeFastHLine(x, y, w, color);
    RGBmatrixPanel_writeFastHLine(x, y+h-1, w, color);
    RGBmatrixPanel_writeFastVLine(x, y, h, color);
    RGBmatrixPanel_writeFastVLine(x+w-1, y, h, color);
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with no RGBmatrixPanel_fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawRoundRect(int16_t x, int16_t y, int16_t w,
  int16_t h, int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writeFastHLine(x+r  , y    , w-2*r, color); // Top
    RGBmatrixPanel_writeFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    RGBmatrixPanel_writeFastVLine(x    , y+r  , h-2*r, color); // Left
    RGBmatrixPanel_writeFastVLine(x+w-1, y+r  , h-2*r, color); // Right
    // RGBmatrixPanel_draw four corners
    RGBmatrixPanel_drawCircleHelper(x+r    , y+r    , r, 1, color);
    RGBmatrixPanel_drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    RGBmatrixPanel_drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    RGBmatrixPanel_drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with RGBmatrixPanel_fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw/fill with
*/
/**************************************************************************/
void RGBmatrixPanel_fillRoundRect(int16_t x, int16_t y, int16_t w,
  int16_t h, int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    RGBmatrixPanel_startWrite();
    RGBmatrixPanel_writeFillRect(x+r, y, w-2*r, h, color);
    // RGBmatrixPanel_draw four corners
    RGBmatrixPanel_fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    RGBmatrixPanel_fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a triangle with no RGBmatrixPanel_fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawTriangle(int16_t x0, int16_t y0,
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    RGBmatrixPanel_drawLine(x0, y0, x1, y1, color);
    RGBmatrixPanel_drawLine(x1, y1, x2, y2, color);
    RGBmatrixPanel_drawLine(x2, y2, x0, y0, color);
}

/**************************************************************************/
/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_fill/draw with
*/
/**************************************************************************/
void RGBmatrixPanel_fillTriangle(int16_t x0, int16_t y0,
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }

    RGBmatrixPanel_startWrite();
    if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        RGBmatrixPanel_writeFastHLine(a, y0, b-a+1, color);
        RGBmatrixPanel_endWrite();
        return;
    }

    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t
    sa   = 0,
    sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        RGBmatrixPanel_writeFastHLine(a, y, b-a+1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for(; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        RGBmatrixPanel_writeFastHLine(a, y, b-a+1, color);
    }
    RGBmatrixPanel_endWrite();
}

// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

/**************************************************************************/
/*!
   @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y) position, using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if(byte & 0x80) RGBmatrixPanel_writePixel(x+i, y, color);
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y) position, using the specified foreground (for RGBmatrixPanel_set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw pixels with
    @param    bg 16-bit 5-6-5 Color to RGBmatrixPanel_draw background with
*/
/**************************************************************************/
void RGBmatrixPanel_drawBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h,
  uint16_t color, uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            RGBmatrixPanel_writePixel(x+i, y, (byte & 0x80) ? color : bg);
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position, using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw with
*/
/**************************************************************************/
void RGBmatrixPanel_drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            if(byte & 0x80) RGBmatrixPanel_writePixel(x+i, y, color);
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position, using the specified foreground (for RGBmatrixPanel_set bits) and background (unset bits) colors.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw pixels with
    @param    bg 16-bit 5-6-5 Color to RGBmatrixPanel_draw background with
*/
/**************************************************************************/
void RGBmatrixPanel_drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = bitmap[j * byteWidth + i / 8];
            RGBmatrixPanel_writePixel(x+i, y, (byte & 0x80) ? color : bg);
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw PROGMEM-resident XBitMap Files (*.xbm), exported from GIMP.
   Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
   C Array can be directly used with this function.
   There is no RAM-resident version of this function; if generating bitmaps
   in RAM, use the format defined by RGBmatrixPanel_drawBitmap() and call that instead.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw pixels with
*/
/**************************************************************************/
void RGBmatrixPanel_drawXBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte >>= 1;
            else      byte   = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            // Nearly identical to RGBmatrixPanel_drawBitmap(), only the bit order
            // is reversed here (left-to-right = LSB to MSB):
            if(byte & 0x01) RGBmatrixPanel_writePixel(x+i, y, color);
        }
    }
    RGBmatrixPanel_endWrite();
}


/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) at the specified (x,y) pos.
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], int16_t w, int16_t h) {
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            RGBmatrixPanel_writePixel(x+i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) at the specified (x,y) pos.
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h) {
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            RGBmatrixPanel_writePixel(x+i, y, bitmap[j * w + i]);
        }
    }
    RGBmatrixPanel_endWrite();
}


/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) with a 1-bit mask
   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
   BOTH buffers (grayscale and mask) must be PROGMEM-resident.
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    mask  byte array with mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y,
  const uint8_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&mask[j * bw + i / 8]);
            if(byte & 0x80) {
                RGBmatrixPanel_writePixel(x+i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
            }
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 8-bit image (grayscale) with a 1-bit mask
   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
   BOTH buffers (grayscale and mask) must be RAM-residentt, no mix-and-match
   Specifically for 8-bit display devices such as IS31FL3731; no color reduction/expansion is performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with grayscale bitmap
    @param    mask  byte array with mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawGrayscaleBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = mask[j * bw + i / 8];
            if(byte & 0x80) {
                RGBmatrixPanel_writePixel(x+i, y, bitmap[j * w + i]);
            }
        }
    }
    RGBmatrixPanel_endWrite();
}


/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], int16_t w, int16_t h) {
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            RGBmatrixPanel_writePixel(x+i, y, pgm_read_word(&bitmap[j * w + i]));
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y) position.
   For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, int16_t w, int16_t h) {
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            RGBmatrixPanel_writePixel(x+i, y, bitmap[j * w + i]);
        }
    }
    RGBmatrixPanel_endWrite();
}


/**************************************************************************/
/*!
   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask (set bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH buffers (color and mask) must be PROGMEM-resident. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    mask  byte array with monochrome mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y,
  const uint16_t bitmap[], const uint8_t mask[],
  int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(&mask[j * bw + i / 8]);
            if(byte & 0x80) {
                RGBmatrixPanel_writePixel(x+i, y, pgm_read_word(&bitmap[j * w + i]));
            }
        }
    }
    RGBmatrixPanel_endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask (set bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH buffers (color and mask) must be RAM-resident. For 16-bit display devices; no color reduction performed.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with 16-bit color bitmap
    @param    mask  byte array with monochrome mask bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
void RGBmatrixPanel_drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h) {
    int16_t bw   = (w + 7) / 8; // Bitmask scanline pad = whole byte
    uint8_t byte = 0;
    RGBmatrixPanel_startWrite();
    for(int16_t j=0; j<h; j++, y++) {
        for(int16_t i=0; i<w; i++ ) {
            if(i & 7) byte <<= 1;
            else      byte   = mask[j * bw + i / 8];
            if(byte & 0x80) {
                RGBmatrixPanel_writePixel(x+i, y, bitmap[j * w + i]);
            }
        }
    }
    RGBmatrixPanel_endWrite();
}

// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw chraracter with
    @param    bg 16-bit 5-6-5 Color to RGBmatrixPanel_fill background with (if same as color, no background)
    @param    size  Font magnification level, 1 is 'original' size
*/
/**************************************************************************/
void RGBmatrixPanel_drawChar(int16_t x, int16_t y, unsigned char c,
  uint16_t color, uint16_t bg, uint8_t size) {
    RGBmatrixPanel_drawChar(x, y, c, color, bg, size, size);
}

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to RGBmatrixPanel_draw chraracter with
    @param    bg 16-bit 5-6-5 Color to RGBmatrixPanel_fill background with (if same as color, no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void RGBmatrixPanel_drawChar(int16_t x, int16_t y, unsigned char c,
  uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {

    if(!gfxFont) { // 'Classic' built-in font

        if((x >= _width)            || // Clip right
           (y >= _height)           || // Clip bottom
           ((x + 6 * size_x - 1) < 0) || // Clip left
           ((y + 8 * size_y - 1) < 0))   // Clip top
            return;

        if(!_cp437 && (c >= 176)) c++; // Handle 'classic' charset behavior

        RGBmatrixPanel_startWrite();
        for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
            uint8_t line = pgm_read_byte(&font[c * 5 + i]);
            for(int8_t j=0; j<8; j++, line >>= 1) {
                if(line & 1) {
                    if(size_x == 1 && size_y == 1)
                        RGBmatrixPanel_writePixel(x+i, y+j, color);
                    else
                        RGBmatrixPanel_writeFillRect(x+i*size_x, y+j*size_y, size_x, size_y, color);
                } else if(bg != color) {
                    if(size_x == 1 && size_y == 1)
                        RGBmatrixPanel_writePixel(x+i, y+j, bg);
                    else
                        RGBmatrixPanel_writeFillRect(x+i*size_x, y+j*size_y, size_x, size_y, bg);
                }
            }
        }
        if(bg != color) { // If opaque, RGBmatrixPanel_draw vertical line for last column
            if(size_x == 1 && size_y == 1) RGBmatrixPanel_writeFastVLine(x+5, y, 8, bg);
            else          RGBmatrixPanel_writeFillRect(x+5*size_x, y, size_x, 8*size_y, bg);
        }
        RGBmatrixPanel_endWrite();

    } else { // Custom font

        // Character is assumed previously filtered by RGBmatrixPanel_write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // RGBmatrixPanel_drawChar() directly with 'bad' characters of font may cause mayhem!

        c -= (uint8_t)pgm_read_byte(&gfxFont->first);
        GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, c);
        uint8_t  *bitmap = pgm_read_bitmap_ptr(gfxFont);

        uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
        uint8_t  w  = pgm_read_byte(&glyph->width),
                 h  = pgm_read_byte(&glyph->height);
        int8_t   xo = pgm_read_byte(&glyph->xOffset),
                 yo = pgm_read_byte(&glyph->yOffset);
        uint8_t  xx, yy, bits = 0, bit = 0;
        int16_t  xo16 = 0, yo16 = 0;

        if(size_x > 1 || size_y > 1) {
            xo16 = xo;
            yo16 = yo;
        }

        // Todo: Add character clipping here

        // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
        // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
        // has typically been used with the 'classic' font to overwrite old
        // screen contents with new data.  This ONLY works because the
        // characters are a uniform size; it's not a sensible thing to do with
        // proportionally-spaced fonts with glyphs of varying sizes (and that
        // may overlap).  To replace previously-drawn text when using a custom
        // font, use the getTextBounds() function to determine the smallest
        // rectangle encompassing a string, erase the area with RGBmatrixPanel_fillRect(),
        // then RGBmatrixPanel_draw new text.  This WILL infortunately 'blink' the text, but
        // is unavoidable.  Drawing 'background' pixels will NOT fix this,
        // only creates a new RGBmatrixPanel_set of problems.  Have an idea to work around
        // this (a canvas object type for MCUs that can afford the RAM and
        // displays supporting RGBmatrixPanel_setAddrWindow() and pushColors()), but haven't
        // implemented this yet.

        RGBmatrixPanel_startWrite();
        for(yy=0; yy<h; yy++) {
            for(xx=0; xx<w; xx++) {
                if(!(bit++ & 7)) {
                    bits = pgm_read_byte(&bitmap[bo++]);
                }
                if(bits & 0x80) {
                    if(size_x == 1 && size_y == 1) {
                        RGBmatrixPanel_writePixel(x+xo+xx, y+yo+yy, color);
                    } else {
                        RGBmatrixPanel_writeFillRect(x+(xo16+xx)*size_x, y+(yo16+yy)*size_y,
                          size_x, size_y, color);
                    }
                }
                bits <<= 1;
            }
        }
        RGBmatrixPanel_endWrite();

    } // End classic vs custom font
}
/**************************************************************************/
/*!
    @brief  Print one byte/character of data, used to support RGBmatrixPanel_print()
    @param  c  The 8-bit ascii character to RGBmatrixPanel_write
*/
/**************************************************************************/
size_t RGBmatrixPanel_write(uint8_t c) {
    if(!gfxFont) { // 'Classic' built-in font

        if(c == '\n') {                        // Newline?
            cursor_x  = 0;                     // Reset x to zero,
            cursor_y += textsize_y * 8;        // advance y one line
        } else if(c != '\r') {                 // Ignore carriage returns
            if(wrap && ((cursor_x + textsize_x * 6) > _width)) { // Off right?
                cursor_x  = 0;                 // Reset x to zero,
                cursor_y += textsize_y * 8;    // advance y one line
            }
            RGBmatrixPanel_drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
            cursor_x += textsize_x * 6;          // Advance x one char
        }

    } else { // Custom font

        if(c == '\n') {
            cursor_x  = 0;
            cursor_y += (int16_t)textsize_y *
                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if(c != '\r') {
            uint8_t first = pgm_read_byte(&gfxFont->first);
            if((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
                GFXglyph *glyph  = pgm_read_glyph_ptr(gfxFont, c - first);
                uint8_t   w     = pgm_read_byte(&glyph->width),
                          h     = pgm_read_byte(&glyph->height);
                if((w > 0) && (h > 0)) { // Is there an associated bitmap?
                    int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
                    if(wrap && ((cursor_x + textsize_x * (xo + w)) > _width)) {
                        cursor_x  = 0;
                        cursor_y += (int16_t)textsize_y *
                          (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                    }
                    RGBmatrixPanel_drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
                }
                cursor_x += (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
            }
        }

    }
    return 1;
}

/**************************************************************************/
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
    @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
/**************************************************************************/
void RGBmatrixPanel_setTextSize(uint8_t s) {
    RGBmatrixPanel_setTextSize(s, s);
}

/**************************************************************************/
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel that much bigger.
    @param  s_x  Desired text width magnification level in X-axis. 1 is default
    @param  s_y  Desired text width magnification level in Y-axis. 1 is default
*/
/**************************************************************************/
void RGBmatrixPanel_setTextSize(uint8_t s_x, uint8_t s_y) {
    textsize_x = (s_x > 0) ? s_x : 1;
    textsize_y = (s_y > 0) ? s_y : 1;
}

/**************************************************************************/
/*!
    @brief      Set rotation RGBmatrixPanel_setting for display
    @param  x   0 thru 3 corresponding to 4 cardinal rotations
*/
/**************************************************************************/
void RGBmatrixPanel_setRotation(uint8_t x) {
    rotation = (x & 3);
    switch(rotation) {
        case 0:
        case 2:
            _width  = WIDTH;
            _height = HEIGHT;
            break;
        case 1:
        case 3:
            _width  = HEIGHT;
            _height = WIDTH;
            break;
    }
}


/**************************************************************************/
/*!
    @brief Set the font to display when RGBmatrixPanel_print()ing, either custom or default
    @param  f  The GFXfont object, if NULL use built in 6x8 font
*/
/**************************************************************************/
void RGBmatrixPanel_setFont(const GFXfont *f) {
    if(f) {            // Font struct pointer passed in?
        if(!gfxFont) { // And no current font struct?
            // Switching from classic to new font behavior.
            // Move cursor pos down 6 pixels so it's on baseline.
            cursor_y += 6;
        }
    } else if(gfxFont) { // NULL passed.  Current font struct defined?
        // Switching from new to classic font behavior.
        // Move cursor pos up 6 pixels so it's at top-left of char.
        cursor_y -= 6;
    }
    gfxFont = (GFXfont *)f;
}


/**************************************************************************/
/*!
    @brief    Helper to determine size of a character with current font/size.
       Broke this out as it's used by both the PROGMEM- and RAM-resident getTextBounds() functions.
    @param    c     The ascii character in question
    @param    x     Pointer to x location of character
    @param    y     Pointer to y location of character
    @param    minx  Minimum clipping value for X
    @param    miny  Minimum clipping value for Y
    @param    maxx  Maximum clipping value for X
    @param    maxy  Maximum clipping value for Y
*/
/**************************************************************************/
void RGBmatrixPanel_charBounds(char c, int16_t *x, int16_t *y,
  int16_t *minx, int16_t *miny, int16_t *maxx, int16_t *maxy) {

    if(gfxFont) {

        if(c == '\n') { // Newline?
            *x  = 0;    // Reset x to zero, advance y by one line
            *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if(c != '\r') { // Not a carriage return; is normal char
            uint8_t first = pgm_read_byte(&gfxFont->first),
                    last  = pgm_read_byte(&gfxFont->last);
            if((c >= first) && (c <= last)) { // Char present in this font?
                GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
                uint8_t gw = pgm_read_byte(&glyph->width),
                        gh = pgm_read_byte(&glyph->height),
                        xa = pgm_read_byte(&glyph->xAdvance);
                int8_t  xo = pgm_read_byte(&glyph->xOffset),
                        yo = pgm_read_byte(&glyph->yOffset);
                if(wrap && ((*x+(((int16_t)xo+gw)*textsize_x)) > _width)) {
                    *x  = 0; // Reset x to zero, advance y by one line
                    *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                }
                int16_t tsx = (int16_t)textsize_x,
                        tsy = (int16_t)textsize_y,
                        x1 = *x + xo * tsx,
                        y1 = *y + yo * tsy,
                        x2 = x1 + gw * tsx - 1,
                        y2 = y1 + gh * tsy - 1;
                if(x1 < *minx) *minx = x1;
                if(y1 < *miny) *miny = y1;
                if(x2 > *maxx) *maxx = x2;
                if(y2 > *maxy) *maxy = y2;
                *x += xa * tsx;
            }
        }

    } else { // Default font

        if(c == '\n') {                     // Newline?
            *x  = 0;                        // Reset x to zero,
            *y += textsize_y * 8;           // advance y one line
            // min/max x/y unchaged -- that waits for next 'normal' character
        } else if(c != '\r') {  // Normal char; ignore carriage returns
            if(wrap && ((*x + textsize_x * 6) > _width)) { // Off right?
                *x  = 0;                    // Reset x to zero,
                *y += textsize_y * 8;       // advance y one line
            }
            int x2 = *x + textsize_x * 6 - 1, // Lower-right pixel of char
                y2 = *y + textsize_y * 8 - 1;
            if(x2 > *maxx) *maxx = x2;      // Track max x, y
            if(y2 > *maxy) *maxy = y2;
            if(*x < *minx) *minx = *x;      // Track min x, y
            if(*y < *miny) *miny = *y;
            *x += textsize_x * 6;             // Advance x one char
        }
    }
}

/**************************************************************************/
/*!
    @brief    Helper to determine size of a string with current font/size. Pass string and a cursor position, returns UL corner and W,H.
    @param    str     The ascii string to measure
    @param    x       The current cursor X
    @param    y       The current cursor Y
    @param    x1      The boundary X coordinate, RGBmatrixPanel_set by function
    @param    y1      The boundary Y coordinate, RGBmatrixPanel_set by function
    @param    w      The boundary width, RGBmatrixPanel_set by function
    @param    h      The boundary height, RGBmatrixPanel_set by function
*/
/**************************************************************************/
void RGBmatrixPanel_getTextBounds(const char *str, int16_t x, int16_t y,
        int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    uint8_t c; // Current character

    *x1 = x;
    *y1 = y;
    *w  = *h = 0;

    int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;

    while((c = *str++))
        RGBmatrixPanel_charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);

    if(maxx >= minx) {
        *x1 = minx;
        *w  = maxx - minx + 1;
    }
    if(maxy >= miny) {
        *y1 = miny;
        *h  = maxy - miny + 1;
    }
}
///

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

// A full PORT register is required for the data lines, though only the
// top 6 output bits are used.  For performance reasons, the port # cannot
// be changed via library calls, only by changing constants in the library.
// For similar reasons, the clock pin is only semi-configurable...it can
// be specified as any pin within a specific PORT register stated below.

 // Ports for "standard" boards (Arduino Uno, Duemilanove, etc.)
#define DATAPORT PORTD
#define DATADIR  DDRD
#define CLKPORT  PORTB

#define nPlanes 4

// The fact that the display driver interrupt stuff is tied to the
// singular Timer1 doesn't really take well to object orientation with
// multiple RGBmatrixPanel instances.  The solution at present is to
// allow instances, but only one is active at any given time, via its
// begin() method.  The implementation is still incomplete in parts;
// the prior active panel really should be gracefully disabled, and a
// stop() method should perhaps be added...assuming multiple instances
// are even an actual need.

// Code common to both the 16x32 and 32x32 constructors:
void RGBmatrixPanel_init(uint8_t rows, bool dbuf, uint8_t width
  ) {

  nRows = rows; // Number of multiplexed rows; actual height is 2X this

  // Allocate and RGBmatrixPanel_initialize matrix buffer:
  int buffsize  = width * nRows * 3, // x3 = 3 bytes holds 4 planes "packed"
      allocsize = (dbuf == true) ? (buffsize * 2) : buffsize;
  if(NULL == (matrixbuff[0] = (uint8_t *)malloc(allocsize))) return;
  memset(matrixbuff[0], 0, allocsize);
  // If not double-buffered, both buffers then point to the same address:
  matrixbuff[1] = (dbuf == true) ? &matrixbuff[0][buffsize] : matrixbuff[0];

  // Save pin numbers for use by begin() method later.

  // Look up port registers and pin masks ahead of time,
  // avoids many slow digitalWrite() calls later.
  plane     = nPlanes - 1;
  row       = nRows   - 1;
  swapflag  = false;
  backindex = 0;     // Array index of back buffer
}

// Constructor for 16x32 panel:
void RGBmatrixPanel_RGBmatrixPanel(bool dbuf)  {
  RGBmatrixPanel_Adafruit_GFX(32, 16);
  RGBmatrixPanel_init(8, dbuf, 32);
}

// Constructor for 32x32 or 32x64 panel:
void RGBmatrixPanel_RGBmatrixPanel(bool dbuf, uint8_t width){
  RGBmatrixPanel_Adafruit_GFX(width, 32);	
  RGBmatrixPanel_init(16, dbuf, width);
}


void RGBmatrixPanel_begin(void) {

  backindex   = 0;                         // Back buffer
  buffptr     = matrixbuff[1 - backindex]; // -> front buffer

  // Enable all comm & address pins as outputs, RGBmatrixPanel_set default states:
  CLK_DDR |=  1 << CLK_PIN; CLK_PORT &= ~(1 << CLK_PIN);
  //pinMode(_clk, OUTPUT); digitalWrite(_clk, LOW);  // Low

  LAT_DDR |=  1 << LAT_PIN; LAT_PORT &= ~(1 << LAT_PIN);
  //pinMode(_lat, OUTPUT); LAT_PORT   &= ~(1 << LAT_PIN);   // Low

  OE_DDR |=  1 << OE_PIN; OE_PORT |= 1 << OE_PIN;
  //pinMode(_oe , OUTPUT); OE_PORT    |= (1 << OE_PIN);     // High (disable output)

  A_DDR |=  1 << A_PIN; A_PORT &= ~(1 << A_PIN);
  //pinMode(_a  , OUTPUT); A_PORT &= ~(1 << A_PIN); // Low

  B_DDR |=  1 << B_PIN; B_PORT &= ~(1 << B_PIN);
  //pinMode(_b  , OUTPUT); B_PORT &= ~(1 << B_PIN); // Low
  
  C_DDR |=  1 << C_PIN; C_PORT &= ~(1 << C_PIN);
  //pinMode(_c  , OUTPUT); C_PORT &= ~(1 << C_PIN); // Low
  if(nRows > 8) {
  	D_DDR |=  1 << D_PIN; D_PORT &= ~(1 << D_PIN);
    //pinMode(_d, OUTPUT); D_PORT &= ~(1 << D_PIN); // Low
  }
  // The high six bits of the data port are RGBmatrixPanel_set as outputs;
  // Might make this configurable in the future, but not yet.
  DATADIR  = 0b11111100;
  DATAPORT = 0;
  // Set up Timer1 for interrupt:
  TCCR1A  = _BV(WGM11); // Mode 14 (fast PWM), OC1A off
  TCCR1B  = _BV(WGM13) | _BV(WGM12) | _BV(CS10); // Mode 14, no prescale
  ICR1    = 100;
  TIMSK |= _BV(TOIE1); // Enable Timer1 interrupt
  sei();                // Enable global interrupts
}

// Original RGBmatrixPanel library used 3/3/3 color.  Later version used
// 4/4/4.  Then RGBmatrixPanel_Adafruit_GFX (core library used across all Adafruit
// display devices now) standardized on 5/6/5.  The matrix still operates
// internally on 4/4/4 color, but all the graphics functions are written
// to expect 5/6/5...the matrix lib will truncate the color components as
// needed when RGBmatrixPanel_drawing.  These next functions are mostly here for the
// benefit of older code using one of the original color formats.

// Promote 3/3/3 RGB to RGBmatrixPanel_Adafruit_GFX 5/6/5
uint16_t RGBmatrixPanel_Color333(uint8_t r, uint8_t g, uint8_t b) {
  // RRRrrGGGgggBBBbb
  return ((r & 0x7) << 13) | ((r & 0x6) << 10) |
         ((g & 0x7) <<  8) | ((g & 0x7) <<  5) |
         ((b & 0x7) <<  2) | ((b & 0x6) >>  1);
}

// Promote 4/4/4 RGB to RGBmatrixPanel_Adafruit_GFX 5/6/5
uint16_t RGBmatrixPanel_Color444(uint8_t r, uint8_t g, uint8_t b) {
  // RRRRrGGGGggBBBBb
  return ((r & 0xF) << 12) | ((r & 0x8) << 8) |
         ((g & 0xF) <<  7) | ((g & 0xC) << 3) |
         ((b & 0xF) <<  1) | ((b & 0x8) >> 3);
}

// Demote 8/8/8 to RGBmatrixPanel_Adafruit_GFX 5/6/5
// If no gamma flag passed, assume linear color
uint16_t RGBmatrixPanel_Color888(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

// 8/8/8 -> gamma -> 5/6/5
uint16_t RGBmatrixPanel_Color888(
  uint8_t r, uint8_t g, uint8_t b, bool gflag) {
  if(gflag) { // Gamma-corrected color?
    r = pgm_read_byte(&gamma_table[r]); // Gamma correction table maps
    g = pgm_read_byte(&gamma_table[g]); // 8-bit input to 4-bit output
    b = pgm_read_byte(&gamma_table[b]);
    return ((uint16_t)r << 12) | ((uint16_t)(r & 0x8) << 8) | // 4/4/4->5/6/5
           ((uint16_t)g <<  7) | ((uint16_t)(g & 0xC) << 3) |
           (          b <<  1) | (           b        >> 3);
  } // else linear (uncorrected) color
  return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

uint16_t RGBmatrixPanel_ColorHSV(
  long hue, uint8_t sat, uint8_t val, bool gflag) {

  uint8_t  r, g, b, lo;
  uint16_t s1, v1;

  // Hue
  hue %= 1536;             // -1535 to +1535
  if(hue < 0) hue += 1536; //     0 to +1535
  lo = hue & 255;          // Low byte  = primary/secondary color mix
  switch(hue >> 8) {       // High byte = sextant of colorwheel
    case 0 : r = 255     ; g =  lo     ; b =   0     ; break; // R to Y
    case 1 : r = 255 - lo; g = 255     ; b =   0     ; break; // Y to G
    case 2 : r =   0     ; g = 255     ; b =  lo     ; break; // G to C
    case 3 : r =   0     ; g = 255 - lo; b = 255     ; break; // C to B
    case 4 : r =  lo     ; g =   0     ; b = 255     ; break; // B to M
    default: r = 255     ; g =   0     ; b = 255 - lo; break; // M to R
  }

  // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
  // on the result rather than a costly divide, while the type upgrade to int
  // avoids repeated type conversions in both directions.
  s1 = sat + 1;
  r  = 255 - (((255 - r) * s1) >> 8);
  g  = 255 - (((255 - g) * s1) >> 8);
  b  = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness) & 16-bit color reduction: similar to above, add 1
  // to allow shifts, and upgrade to int makes other conversions implicit.
  v1 = val + 1;
  if(gflag) { // Gamma-corrected color?
    r = pgm_read_byte(&gamma_table[(r * v1) >> 8]); // Gamma correction table maps
    g = pgm_read_byte(&gamma_table[(g * v1) >> 8]); // 8-bit input to 4-bit output
    b = pgm_read_byte(&gamma_table[(b * v1) >> 8]);
  } else { // linear (uncorrected) color
    r = (r * v1) >> 12; // 4-bit results
    g = (g * v1) >> 12;
    b = (b * v1) >> 12;
  }
  return (r << 12) | ((r & 0x8) << 8) | // 4/4/4 -> 5/6/5
         (g <<  7) | ((g & 0xC) << 3) |
         (b <<  1) | ( b        >> 3);
}

void RGBmatrixPanel_drawPixel(int16_t x, int16_t y, uint16_t c) {
  uint8_t r, g, b, bit, limit, *ptr;

  if((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

  switch(rotation) {
   case 1:
    _swap_int16_t(x, y);
    x = WIDTH  - 1 - x;
    break;
   case 2:
    x = WIDTH  - 1 - x;
    y = HEIGHT - 1 - y;
    break;
   case 3:
    _swap_int16_t(x, y);
    y = HEIGHT - 1 - y;
    break;
  }

  // RGBmatrixPanel_Adafruit_GFX uses 16-bit color in 5/6/5 format, while matrix needs
  // 4/4/4.  Pluck out relevant bits while separating into R,G,B:
  r =  c >> 12;        // RRRRrggggggbbbbb
  g = (c >>  7) & 0xF; // rrrrrGGGGggbbbbb
  b = (c >>  1) & 0xF; // rrrrrggggggBBBBb

  // Loop counter stuff
  bit   = 2;
  limit = 1 << nPlanes;

  if(y < nRows) {
    // Data for the upper half of the display is stored in the lower
    // bits of each byte.
    ptr = &matrixbuff[backindex][y * WIDTH * (nPlanes - 1) + x]; // Base addr
    // Plane 0 is a tricky case -- its data is spread about,
    // stored in least two bits not used by the other planes.
    ptr[WIDTH*2] &= ~0b00000011;           // Plane 0 R,G mask out in one op
    if(r & 1) ptr[WIDTH*2] |= 0b00000001; // Plane 0 R: 64 bytes ahead, bit 0
    if(g & 1) ptr[WIDTH*2] |= 0b00000010; // Plane 0 G: 64 bytes ahead, bit 1
    if(b & 1) ptr[WIDTH]   |= 0b00000001; // Plane 0 B: 32 bytes ahead, bit 0
    else      ptr[WIDTH]   &= ~0b00000001; // Plane 0 B unset; mask out
    // The remaining three image planes are more normal-ish.
    // Data is stored in the high 6 bits so it can be quickly
    // copied to the DATAPORT register w/6 output lines.
    for(; bit < limit; bit <<= 1) {
      *ptr &= ~0b00011100;            // Mask out R,G,B in one op
      if(r & bit) *ptr |= 0b00000100; // Plane N R: bit 2
      if(g & bit) *ptr |= 0b00001000; // Plane N G: bit 3
      if(b & bit) *ptr |= 0b00010000; // Plane N B: bit 4
      ptr  += WIDTH;                 // Advance to next bit plane
    }
  } else {
    // Data for the lower half of the display is stored in the upper
    // bits, except for the plane 0 stuff, using 2 least bits.
    ptr = &matrixbuff[backindex][(y - nRows) * WIDTH * (nPlanes - 1) + x];
    *ptr &= ~0b00000011;                  // Plane 0 G,B mask out in one op
    if(r & 1)  ptr[WIDTH] |= 0b00000010; // Plane 0 R: 32 bytes ahead, bit 1
    else       ptr[WIDTH] &= ~0b00000010; // Plane 0 R unset; mask out
    if(g & 1) *ptr        |= 0b00000001; // Plane 0 G: bit 0
    if(b & 1) *ptr        |= 0b00000010; // Plane 0 B: bit 0
    for(; bit < limit; bit <<= 1) {
      *ptr &= ~0b11100000;            // Mask out R,G,B in one op
      if(r & bit) *ptr |= 0b00100000; // Plane N R: bit 5
      if(g & bit) *ptr |= 0b01000000; // Plane N G: bit 6
      if(b & bit) *ptr |= 0b10000000; // Plane N B: bit 7
      ptr  += WIDTH;                 // Advance to next bit plane
    }
  }
}

void RGBmatrixPanel_fillScreen(uint16_t c) {
  if((c == 0x0000) || (c == 0xffff)) {
    // For black or white, all bits in frame buffer will be identically
    // RGBmatrixPanel_set or unset (regardless of weird bit packing), so it's OK to just
    // quickly memset the whole thing:
    memset(matrixbuff[backindex], c, WIDTH * nRows * 3);
  } else {
    // Otherwise, need to handle it the long way:
    RGBmatrixPanel_fillRect(0, 0, _width, _height, c);
  }
}

// Return address of back buffer -- can then load/store data directly
uint8_t *RGBmatrixPanel_backBuffer() {
  return matrixbuff[backindex];
}

// For smooth animation -- RGBmatrixPanel_drawing always takes place in the "back" buffer;
// this method pushes it to the "front" for display.  Passing "true", the
// updated display contents are then copied to the new back buffer and can
// be incrementally modified.  If "false", the back buffer then contains
// the old front buffer contents -- your code can either clear this or
// RGBmatrixPanel_draw over every pixel.  (No effect if double-buffering is not enabled.)
void RGBmatrixPanel_swapBuffers(bool copy) {
  if(matrixbuff[0] != matrixbuff[1]) {
    // To avoid 'tearing' display, actual swap takes place in the interrupt
    // handler, at the end of a complete screen refresh cycle.
    swapflag = true;                  // Set flag here, then...
    while(swapflag == true) delay(1); // wait for interrupt to clear it
    if(copy == true)
      memcpy(matrixbuff[backindex], matrixbuff[1-backindex], WIDTH * nRows * 3);
  }
}

// -------------------- Interrupt handler stuff --------------------

ISR(TIMER1_OVF_vect, ISR_BLOCK) { // ISR_BLOCK important -- see notes later
  RGBmatrixPanel_updateDisplay();   // Call refresh func for active display
  TIFR |= TOV1;                  // Clear Timer1 interrupt flag
}

// Two constants are used in timing each successive BCM interval.
// These were found empirically, by checking the value of TCNT1 at
// certain positions in the interrupt code.
// CALLOVERHEAD is the number of CPU 'ticks' from the timer overflow
// condition (triggering the interrupt) to the first line in the
// RGBmatrixPanel_updateDisplay() method.  It's then assumed (maybe not entirely 100%
// accurately, but close enough) that a similar amount of time will be
// needed at the opposite end, restoring regular program flow.
// LOOPTIME is the number of 'ticks' spent inside the shortest data-
// issuing loop (not actually a 'loop' because it's unrolled, but eh).
// Both numbers are rounded up slightly to allow a little wiggle room
// should different compilers produce slightly different results.
#define CALLOVERHEAD 60   // Actual value measured = 56
#define LOOPTIME     200  // Actual value measured = 188
// The "on" time for bitplane 0 (with the shortest BCM interval) can
// then be estimated as LOOPTIME + CALLOVERHEAD * 2.  Each successive
// bitplane then doubles the prior amount of time.  We can then
// estimate refresh rates from this:
// 4 bitplanes = 320 + 640 + 1280 + 2560 = 4800 ticks per row.
// 4800 ticks * 16 rows (for 32x32 matrix) = 76800 ticks/frame.
// 16M CPU ticks/sec / 76800 ticks/frame = 208.33 Hz.
// Actual frame rate will be slightly less due to work being done
// during the brief "LEDs off" interval...it's reasonable to say
// "about 200 Hz."  The 16x32 matrix only has to scan half as many
// rows...so we could either double the refresh rate (keeping the CPU
// load the same), or keep the same refresh rate but halve the CPU
// load.  We opted for the latter.
// Can also estimate CPU use: bitplanes 1-3 all use 320 ticks to
// issue data (the increasing gaps in the timing invervals are then
// available to other code), and bitplane 0 takes 920 ticks out of
// the 2560 tick interval.
// 320 * 3 + 920 = 1880 ticks spent in interrupt code, per row.
// From prior calculations, about 4800 ticks happen per row.
// CPU use = 1880 / 4800 = ~39% (actual use will be very slightly
// higher, again due to code used in the LEDs off interval).
// 16x32 matrix uses about half that CPU load.  CPU time could be
// further adjusted by padding the LOOPTIME value, but refresh rates
// will decrease proportionally, and 200 Hz is a decent target.

// The flow of the interrupt can be awkward to grasp, because data is
// being issued to the LED matrix for the *next* bitplane and/or row
// while the *current* plane/row is being shown.  As a result, the
// counter variables change between past/present/future tense in mid-
// function...hopefully tenses are sufficiently commented.
void RGBmatrixPanel_updateDisplay(void) {
  uint8_t  i, tick, tock, *ptr;
  uint16_t t, duration;

  OE_PORT  |= (1 << OE_PIN);  // Disable LED output during row/plane switchover
  LAT_PORT |= (1 << LAT_PIN); // Latch data loaded during *prior* interrupt

  // Calculate time to next interrupt BEFORE incrementing plane #.
  // This is because duration is the display time for the data loaded
  // on the PRIOR interrupt.  CALLOVERHEAD is subtracted from the
  // result because that time is implicit between the timer overflow
  // (interrupt triggered) and the RGBmatrixPanel_initial LEDs-off line at the start
  // of this method.
  t = (nRows > 8) ? LOOPTIME : (LOOPTIME * 2);
  duration = ((t + CALLOVERHEAD * 2) << plane) - CALLOVERHEAD;

  // Borrowing a technique here from Ray's Logic:
  // www.rayslogic.com/propeller/Programming/AdafruitRGB/AdafruitRGB.htm
  // This code cycles through all four planes for each scanline before
  // advancing to the next line.  While it might seem beneficial to
  // advance lines every time and interleave the planes to reduce
  // vertical scanning artifacts, in practice with this panel it causes
  // a green 'ghosting' effect on black pixels, a much worse artifact.

  if(++plane >= nPlanes) {      // Advance plane counter.  Maxed out?
    plane = 0;                  // Yes, reset to plane 0, and
    if(++row >= nRows) {        // advance row counter.  Maxed out?
      row     = 0;              // Yes, reset row counter, then...
      if(swapflag == true) {    // Swap front/back buffers if requested
        backindex = 1 - backindex;
        swapflag  = false;
      }
      buffptr = matrixbuff[1-backindex]; // Reset into front buffer
    }
  } else if(plane == 1) {
    // Plane 0 was loaded on prior interrupt invocation and is about to
    // latch now, so update the row address lines before we do that:
    if(row & 0x1)   A_PORT |=  (1 << A_PIN);
    else            A_PORT &= ~(1 << A_PIN);
    if(row & 0x2)   B_PORT |=  (1 << B_PIN);
    else            B_PORT &= ~(1 << B_PIN);
    if(row & 0x4)   C_PORT |=  (1 << C_PIN);
    else            C_PORT &= ~(1 << C_PIN);
    if(nRows > 8) {
      if(row & 0x8) D_PORT |=  (1 << D_PIN);
      else          D_PORT &= ~(1 << D_PIN);
    }
  }

  // buffptr, being 'volatile' type, doesn't take well to optimization.
  // A local register copy can speed some things up:
  ptr = (uint8_t *)buffptr;

  ICR1      = duration; // Set interval for next interrupt
  TCNT1     = 0;        // Restart interrupt timer
  OE_PORT  &= ~(1 << OE_PIN);  // Re-enable output
  LAT_PORT &= ~(1 << LAT_PIN); // Latch down

  // Record current state of CLKPORT register, as well as a second
  // copy with the clock bit RGBmatrixPanel_set.  This makes the innnermost data-
  // pushing loops faster, as they can just RGBmatrixPanel_set the PORT state and
  // not have to load/modify/store bits every single time.  It's a
  // somewhat rude trick that ONLY works because the interrupt
  // handler is RGBmatrixPanel_set ISR_BLOCK, halting any other interrupts that
  // might otherwise also be twiddling the port at the same time
  // (else this would clobber them). only needed for AVR's where you
  // cannot RGBmatrixPanel_set one bit in a single instruction
  tock = CLKPORT;
  tick = tock | (1 << CLK_PIN);

  if(plane > 0) { // 188 ticks from TCNT1=0 (above) to end of function

    // Planes 1-3 copy bytes directly from RAM to PORT without unpacking.
    // The least 2 bits (used for plane 0 data) are presumed masked out
    // by the port direction bits.
    // A tiny bit of inline assembly is used; compiler doesn't pick
    // up on opportunity for post-increment addressing mode.
    // 5 instruction ticks per 'pew' = 160 ticks total
    #define pew asm volatile(                 \
      "ld  __tmp_reg__, %a[ptr]+"    "\n\t"   \
      "out %[data]    , __tmp_reg__" "\n\t"   \
      "out %[clk]     , %[tick]"     "\n\t"   \
      "out %[clk]     , %[tock]"     "\n"     \
      :: [ptr]  "e" (ptr),                    \
         [data] "I" (_SFR_IO_ADDR(DATAPORT)), \
         [clk]  "I" (_SFR_IO_ADDR(CLKPORT)),  \
         [tick] "r" (tick),                   \
         [tock] "r" (tock));
    // Loop is unrolled for speed:
    pew pew pew pew pew pew pew pew
    pew pew pew pew pew pew pew pew
    pew pew pew pew pew pew pew pew
    pew pew pew pew pew pew pew pew

    if(WIDTH == 64) {
      pew pew pew pew pew pew pew pew
      pew pew pew pew pew pew pew pew
      pew pew pew pew pew pew pew pew
      pew pew pew pew pew pew pew pew
    }

    buffptr = ptr; //+= 32;

  } else { // 920 ticks from TCNT1=0 (above) to end of function
    // Planes 1-3 (handled above) formatted their data "in place,"
    // their layout matching that out the output PORT register (where
    // 6 bits correspond to output data lines), maximizing throughput
    // as no conversion or unpacking is needed.  Plane 0 then takes up
    // the slack, with all its data packed into the 2 least bits not
    // used by the other planes.  This works because the unpacking and
    // output for plane 0 is handled while plane 3 is being displayed...
    // because binary coded modulation is used (not PWM), that plane
    // has the longest display interval, so the extra work fits.
    for(i=0; i<WIDTH; i++) {
      DATAPORT =
        ( ptr[i]         << 6)         |
        ((ptr[i+WIDTH]   << 4) & 0x30) |
        ((ptr[i+WIDTH*2] << 2) & 0x0C);
      CLKPORT = tick; // Clock lo
      CLKPORT = tock; // Clock hi
    } 
  }
}

