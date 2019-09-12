#include <arduino.h>
#include <avr/pgmspace.h>
#include "chartable.h"

// comment this to disable SPI SRAM
#define USE_SPI 

// display size, pixels
#define DISPLAY_HEIGHT 64
#define DISPLAY_WIDTH 240

// timer characteristics
#define V_SYNC_FREQ (int(1.0e6 / (DISPLAY_WIDTH * 5)))    // Speed of 1 line output
#define V_SYNC_PRESCALER 8					// Constant for timer setup
#define V_SYNC_TIMER (65536 - 16000000/V_SYNC_PRESCALER/V_SYNC_FREQ) // Constant for timer setup

// modes for sprite output
#define MODE_PUT 0 // clear background, draw sprite
#define MODE_AND 1 // mask background (leave pixel black only if sprite pixel is also black, clear everything else)
#define MODE_OR  2 // draw only black pixels of sprite, leave background ontouched
#define MODE_XOR 3 // if background and sprite pixels are the same then clear background, else draw black pixel
#define MODE_INV 4 // clear background, draw sprite negative
#define MODE_DEL 5 // clear background where sprite pixels are black

#define CHAR_LF 10
#define CHAR_CR 13
#define CHAR_CRLF 1310

// 168 and 328 Arduinos
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega328P__)
  #define PIN_OFFSET 8
  #define LCDPORT PORTB
  #define PORT_BACKLIGHT PORTD
  #define BIT_BACKLIGHT 6
  #define PIN_BACKLIGHT 6 
// Mega 1280 & 2560
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define PIN_OFFSET 22
  #define LCDPORT PORTA
  #define PORT_BACKLIGHT PORTC
  #define BIT_BACKLIGHT 6
  #define PIN_BACKLIGHT 31
#endif  

#if defined(USE_SPI)
  #include <SPI.h>
  #include <SRAM_23LC.h>
#endif

void initLCDPorts(int FLM, int DATA1, int DATA2, int M, int LOAD, int CLOCK); // set up pin modes and initial states
void displayVRAM(); // send content of video memory to LCD DG24064-50
bool copyVRAMtoSRAM(uint32_t address);
bool readVRAMfromSRAM(uint32_t address);

void clearVRAM(); // fill video memory with zeroes
void blackVRAM(); // fill video memory with all black pixels
void scrollUpVRAM(int count); // shift vram up by <count> pixels
void scrollDownVRAM(int count); // shift screen down by <count> pixels
void scrollLeftVRAMx8(int count); // scrolls left only by <count x8> pixels
void scrollRightVRAMx8(int count); // scrolls right only by <count x8> pixels

// set pixel (x,y) with <color>, color is 0 or 1
// if x or y are beyond screen VRAM is not touched
// so it's safe to pass any x, y to this function
// all other drawing functions use putPixel
void putPixel(int x, int y, int color);

// get pixel color at VRAM position (x,y) returns true or false
bool getPixel(int x, int y);

// put 8x8 sprite into (x,y) position of VRAM, <mode> affects how information already present in VRAM is combined with <sprite> data
void putSprite(byte *sprite, int x, int y, byte mode);

// fill rectangle area in VRAM with color
// startX, startY - upper left corner of rectangle
// width, height - size of rectangle 
void fillRect(int startX, int startY, int width, int height, int color);

// draw hollow rectangle area
void drawRect(int startX, int startY, int width, int height, int color);

// draw line from (startX, startY) to (endX, endY) of <color> in VRAM
// for vertical and horizontal lines use special functions instead, because this one works slow
void drawLine(int startX, int startY, int endX, int endY, int color);

// draw horizontal line from (startX, y) to (endX, y)
void drawHorLine(int startX, int endX, int y, int color);

// draw Vertical line from (x, startY) to (x, endY)
void drawVertLine(int startY, int endY, int x, int color);

// draw hollow circle with center at (x, y) and radius <r> with <color> in VRAM
void drawCircle(int x, int y, int r, int color);

// draw filled circle
void fillCircle(int x, int y, int r, int color);

// show string in graphic mode (each character is 5x7)
// str - pointer to a string
// x, y - upper left corner of text
// wrap - if true continue to output on (0, y + 8) after reaching right edge of LCD, if false stop output when reaching right edge
//
// KNOWN ISSUES:
// Backspace char (BS, ASCII 08) does nothing if occured just after text wrap
void drawText(char* str, int x, int y, bool wrap);

// show string in text mode (each character is 7x7)
// str - pointer to a string
// col, row - upper left corner of text
// wrap - if true continue to output on (0, y + 8) after reaching right edge of LCD, scroll screen up by 1 line (8 pixels) when reaching end of last line
// if wrap is false - stop output when reaching right edge
void dispText(char* str, int col, int row, bool wrap);

// show integer number in graphic mode with 5x7 font
// <width> is used for alignment
// symbols exceeding right edge of the screen are thrown
void putInt(long num, int x, int y, int width);

// sets tabulations size for drawText and dispText functions, <chars> is TAB size in CHARACTERS
void setTabSize(int chars);

// sets which char is treated as EOL (LF, CR of CR LF) by drawText and dispText functions
// <eol> should be CHAR_LF, CHAR_CR or CHAR_CRLF respectively
void setEndOfLineChar(int eol);

void backLightOn();
void backLightOff();
// set backlight brightness in range 0 - 1023
void backLightSetBrightness(unsigned int number);