#ifndef CONFIG_H
#define CONFIG_H
#include <avr/io.h>
#define CLK_PIN 1
#define CLK_PORT PORTB
#define CLK_DDR DDRB
#define OE_PIN  2
#define OE_PORT PORTB
#define OE_DDR DDRB
#define LAT_PIN 3
#define LAT_PORT PORTB
#define LAT_DDR DDRB
#define A_PIN   4
#define A_PORT PORTB
#define A_DDR DDRB
#define B_PIN   5
#define B_PORT PORTB
#define B_DDR DDRB
#define C_PIN   6
#define C_PORT PORTB
#define C_DDR DDRB
#define D_PIN   7
#define D_PORT PORTB
#define D_DDR DDRB
#define LSBFIRST 0
#define MSBFIRST 1
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#endif
