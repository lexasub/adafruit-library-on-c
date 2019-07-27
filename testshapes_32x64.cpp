// testshapes demo for RGBmatrixPanel library.
// Demonstrates the drawing abilities of the RGBmatrixPanel library.
// For 32x64 RGB LED matrix.

// WILL NOT FIT on ARDUINO UNO -- requires a Mega, M0 or M4 board


// Most of the signal pins are configurable, but the CLK pin has some
// special constraints.  On 8-bit AVR boards it must be on PORTB...
// Pin 8 works on the Arduino Uno & compatibles (e.g. Adafruit Metro),
// Pin 11 works on the Arduino Mega.  On 32-bit SAMD boards it must be
// on the same PORT as the RGB data pins (D2-D7)...
// Pin 8 works on the Adafruit Metro M0 or Arduino Zero,
// Pin A4 works on the Adafruit Metro M4 (if using the Adafruit RGB
// Matrix Shield, cut trace between CLK pads and run a wire to A4).


#include "RGBmatrixPanel.cpp"
#include <util/delay.h>

uint16_t Wheel(uint8_t WheelPos);
#define delay _delay_ms

int main() {
  RGBmatrixPanel_RGBmatrixPanel( false, 64);
  RGBmatrixPanel_begin();

  void _delay_ms(double ms);
  // draw a pixel in solid white
  RGBmatrixPanel_drawPixel(0, 0, RGBmatrixPanel_Color333(7, 7, 7));
  delay(500);

  // fix the screen with green
  RGBmatrixPanel_fillRect(0, 0, RGBmatrixPanel_width(), RGBmatrixPanel_height(), RGBmatrixPanel_Color333(0, 7, 0));
  delay(500);

  // draw a box in yellow
  RGBmatrixPanel_drawRect(0, 0, RGBmatrixPanel_width(), RGBmatrixPanel_height(), RGBmatrixPanel_Color333(7, 7, 0));
  delay(500);

  // draw an 'X' in red
  RGBmatrixPanel_drawLine(0, 0, RGBmatrixPanel_width()-1, RGBmatrixPanel_height()-1, RGBmatrixPanel_Color333(7, 0, 0));
  RGBmatrixPanel_drawLine(RGBmatrixPanel_width()-1, 0, 0, RGBmatrixPanel_height()-1, RGBmatrixPanel_Color333(7, 0, 0));
  delay(500);

  // draw a blue circle
  RGBmatrixPanel_drawCircle(10, 10, 10, RGBmatrixPanel_Color333(0, 0, 7));
  delay(500);

  // fill a violet circle
  RGBmatrixPanel_fillCircle(40, 21, 10, RGBmatrixPanel_Color333(7, 0, 7));
  delay(500);

  // fill the screen with 'black'
  RGBmatrixPanel_fillScreen(RGBmatrixPanel_Color333(0, 0, 0));

  // draw some text!
  RGBmatrixPanel_setTextSize(1);     // size 1 == 8 pixels high
  RGBmatrixPanel_setTextWrap(false); // Don't wrap at end of line - will do ourselves

  RGBmatrixPanel_setCursor(8, 0);    // start at top left, with 8 pixel of spacing
  uint8_t w = 0;
  char *str = "AdafruitIndustries";
  for (w=0; w<8; w++) {
    RGBmatrixPanel_setTextColor(Wheel(w));
    RGBmatrixPanel_print(str[w]);
  }
  RGBmatrixPanel_setCursor(2, 8);    // next line
  for (w=8; w<18; w++) {
    RGBmatrixPanel_setTextColor(Wheel(w));
    RGBmatrixPanel_print(str[w]);
  }
  RGBmatrixPanel_println();
  //matrix.setTextColor(matrix.Color333(4,4,4));
  //matrix.println("Industries");
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(7,7,7));
  RGBmatrixPanel_println("LED MATRIX!");

  // print each letter with a rainbow color
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(7,0,0));
  RGBmatrixPanel_print('3');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(7,4,0));
  RGBmatrixPanel_print('2');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(7,7,0));
  RGBmatrixPanel_print('x');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(4,7,0));
  RGBmatrixPanel_print('6');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(0,7,0));
  RGBmatrixPanel_print('4');
  RGBmatrixPanel_setCursor(34, 24);
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(0,7,7));
  RGBmatrixPanel_print('*');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(0,4,7));
  RGBmatrixPanel_print('R');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(0,0,7));
  RGBmatrixPanel_print('G');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(4,0,7));
  RGBmatrixPanel_print('B');
  RGBmatrixPanel_setTextColor(RGBmatrixPanel_Color333(7,0,4));
  RGBmatrixPanel_print('*');
  while(1){}
  // whew!
}


// Input a value 0 to 24 to get a color value.
// The colours are a transition r - g - b - back to r.
uint16_t Wheel(uint8_t WheelPos) {
  if(WheelPos < 8) {
   return RGBmatrixPanel_Color333(7 - WheelPos, WheelPos, 0);
  } else if(WheelPos < 16) {
   WheelPos -= 8;
   return RGBmatrixPanel_Color333(0, 7-WheelPos, WheelPos);
  } else {
   WheelPos -= 16;
   return RGBmatrixPanel_Color333(0, WheelPos, 7 - WheelPos);
  }
}
