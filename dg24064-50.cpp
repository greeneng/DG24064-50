#include "dg24064-50.h"
//#include "chartable.h"

// default values for LCD ports
int PIN_M     = PIN_OFFSET + 3;
int PIN_LOAD  = PIN_OFFSET + 4;
int PIN_CLOCK = PIN_OFFSET + 5;
int PIN_DATA0 = PIN_OFFSET;
int PIN_DATA1 = PIN_OFFSET + 1;
int PIN_DATA2 = PIN_OFFSET + 2;

// default bit masks for fast output
byte SET_PIN_DATA0 = B00000001; // pin 8
byte RES_PIN_DATA0 = B11111110;
byte SET_PIN_DATA1 = B00000010; // pin 9
byte RES_PIN_DATA1 = B11111101;
byte SET_PIN_DATA2 = B00000100; // pin 10
byte RES_PIN_DATA2 = B11111011;
byte SET_PIN_M     = B00001000; // pin 11
byte RES_PIN_M     = B11110111;
byte SET_PIN_LOAD  = B00010000; // pin 12 
byte RES_PIN_LOAD  = B11101111;
byte SET_PIN_CLOCK = B00100000; // pin 13
byte RES_PIN_CLOCK = B11011111;

// masks for fast bit set/reset
const byte MASK_SET[8] = { 
  B00000001,
  B00000010,
  B00000100,
  B00001000,
  B00010000,
  B00100000,
  B01000000,
  B10000000
};

const byte MASK_RES[8] = {
  B11111110,
  B11111101,
  B11111011,
  B11110111,
  B11101111,
  B11011111,
  B10111111,
  B01111111
};

// vram is our video memory (VRAM)
byte vram[DISPLAY_HEIGHT][DISPLAY_WIDTH / 8];

// default TAB size for text output by drawText and dispText functions
byte TAB_SIZE_TXT = 2;
byte TAB_SIZE_GRAPH = 12;

// default char for End Of Line
int EOL_SEQ = CHAR_LF;

// current line, do NOT change it outside interrupt handler
volatile int CURRENT_LINE = 0;

bool BACKLIGHT = false;
unsigned int brightness = 0;

void initLCDPorts(int FLM, int DATA1, int DATA2, int M, int LOAD, int CLOCK)
{
  PIN_DATA0 = FLM;
  PIN_DATA1 = DATA1;
  PIN_DATA2 = DATA2;
  PIN_M = M;
  PIN_LOAD = LOAD;
  PIN_CLOCK = CLOCK;

  SET_PIN_DATA0 = MASK_SET[PIN_DATA0 - PIN_OFFSET];
  RES_PIN_DATA0 = MASK_RES[PIN_DATA0 - PIN_OFFSET];
  SET_PIN_DATA1 = MASK_SET[PIN_DATA1 - PIN_OFFSET];
  RES_PIN_DATA1 = MASK_RES[PIN_DATA1 - PIN_OFFSET];
  SET_PIN_DATA2 = MASK_SET[PIN_DATA2 - PIN_OFFSET];
  RES_PIN_DATA2 = MASK_RES[PIN_DATA2 - PIN_OFFSET];
  SET_PIN_M     = MASK_SET[PIN_M - PIN_OFFSET];
  RES_PIN_M     = MASK_RES[PIN_M - PIN_OFFSET];
  SET_PIN_LOAD  = MASK_SET[PIN_LOAD - PIN_OFFSET];
  RES_PIN_LOAD  = MASK_RES[PIN_LOAD - PIN_OFFSET];
  SET_PIN_CLOCK = MASK_SET[PIN_CLOCK - PIN_OFFSET];
  RES_PIN_CLOCK = MASK_RES[PIN_CLOCK - PIN_OFFSET];
  
  pinMode(PIN_M, OUTPUT);
  pinMode(PIN_LOAD, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA0, OUTPUT);
  pinMode(PIN_DATA1, OUTPUT);
  pinMode(PIN_DATA2, OUTPUT);
  pinMode(PIN_BACKLIGHT, OUTPUT);
  
  digitalWrite(PIN_M, HIGH);
  digitalWrite(PIN_LOAD, LOW);
  digitalWrite(PIN_CLOCK, LOW);
  digitalWrite(PIN_DATA0, LOW);
  digitalWrite(PIN_DATA1, LOW);
  digitalWrite(PIN_DATA2, LOW);
  digitalWrite(PIN_BACKLIGHT, LOW);
  
  CURRENT_LINE = 0;
  
  cli();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = V_SYNC_TIMER;     // preload timer 
  switch (V_SYNC_PRESCALER)
  {
	  case 1: 
		TCCR1B = (1 << CS10);
		break;
	  case 8:
		TCCR1B = (1 << CS11);
		break;
	  case 64:
	    TCCR1B = (1 << CS10) | (1 << CS11);
		break;
	  case 256:
		TCCR1B = (1 << CS12);
		break;
	  case 1024:
		TCCR1B = (1 << CS12) | (1 << CS10);
		break;
	  default:
	    TCCR1B = 0;
  }
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  
  brightness = 0;
  backLightOff();
  sei();             // enable all interrupts
}

void clearVRAM()
{
  cli();
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = 0;
    }
  }
  sei();
}

void blackVRAM()
{
  cli();
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = B11111111;
    }
  }
  sei();
}

ISR(TIMER1_OVF_vect)
{
	TCNT1 = V_SYNC_TIMER;            // preload timer
  
	bool bit_data1 = 0;
	bool bit_data2 = 0;
  
	int height = DISPLAY_HEIGHT;
    int line = CURRENT_LINE;
	
    LCDPORT |= SET_PIN_LOAD; //    digitalWrite(PIN_LOAD, HIGH);
	
	if (line == 1)
          LCDPORT |= SET_PIN_DATA0; // digitalWrite(PIN_DATA0, HIGH);
    else
          LCDPORT &= RES_PIN_DATA0; // digitalWrite(PIN_DATA0, LOW);
	  
	if (line == 0) 
	{	
		LCDPORT ^= SET_PIN_M; //     digitalWrite(PIN_M, M); 
	}
	
	if (BACKLIGHT)
	{
	  if (line % 8 == 0)
        PORT_BACKLIGHT |= MASK_SET[BIT_BACKLIGHT]; //digitalWrite(PIN_BACKLIGHT, HIGH);
	}
	else 
	  PORT_BACKLIGHT &= MASK_RES[BIT_BACKLIGHT]; // digitalWrite(PIN_BACKLIGHT, LOW);	 
	
	LCDPORT &= RES_PIN_LOAD; //digitalWrite(PIN_LOAD, LOW);  
	
	for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {       
	  byte b1 = vram[line][j];
      byte b2 = 0;
      if (DISPLAY_HEIGHT > 32)
        b2 = vram[line + 32][j];
      
     if ((line % 8) * (DISPLAY_WIDTH / 8) + j > brightness) 
       PORT_BACKLIGHT &= MASK_RES[BIT_BACKLIGHT]; // digitalWrite(PIN_BACKLIGHT, LOW); 
	  
      for (int bitn = 7; bitn >= 0; bitn--)
      {
		LCDPORT |= SET_PIN_CLOCK; //  digitalWrite(PIN_CLOCK, HIGH);
		bit_data1 = b1 & MASK_SET[bitn]; // read leftmost bit
        bit_data2 = b2 & MASK_SET[bitn];
         
	  // b1 <<= 1;                     // shift to next
        //b2 <<= 1;
         
         
        if (bit_data1) 
          LCDPORT |= SET_PIN_DATA1; //digitalWrite(PIN_DATA1, HIGH);
        else
          LCDPORT &= RES_PIN_DATA1; // digitalWrite(PIN_DATA1, LOW);

        if (DISPLAY_HEIGHT > 32)
        {
          if (bit_data2)
            LCDPORT |= SET_PIN_DATA2; // digitalWrite(PIN_DATA2, HIGH);
          else 
            LCDPORT &= RES_PIN_DATA2; // digitalWrite(PIN_DATA2, LOW);
        }  
		else	
		  LCDPORT &= RES_PIN_DATA2; // digitalWrite(PIN_DATA2, LOW);
	  
	    LCDPORT &= RES_PIN_CLOCK; //        digitalWrite(PIN_CLOCK, LOW);
	  }
    }    
        
	line++;
	if (line >= 32)
		line = 0;
	CURRENT_LINE = line;
}

void displayVRAM()
{ 
  bool bit_data1 = 0;
  bool bit_data2 = 0;
  
  int height = DISPLAY_HEIGHT;
  if ((DISPLAY_HEIGHT > 32) && (DISPLAY_HEIGHT <= 64))
    height = DISPLAY_HEIGHT / 2;
  PORTB |= SET_PIN_LOAD; //    digitalWrite(PIN_LOAD, HIGH);
  for (int i = 0; i < height; i++)
  {    
    PORTB &= RES_PIN_LOAD; //digitalWrite(PIN_LOAD, LOW); 
    if (i == 0)
          PORTB |= SET_PIN_DATA0; // digitalWrite(PIN_DATA0, HIGH);
    else
          PORTB &= RES_PIN_DATA0; // digitalWrite(PIN_DATA0, LOW);
    
   
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {  
      byte b1 = vram[i][j];
      byte b2 = 0;
      if (DISPLAY_HEIGHT > 32)
        b2 = vram[i + height][j];
        
      for (int bitn = 7; bitn >= 0; bitn--)
      {
        bit_data1 = b1 & MASK_SET[7]; // read leftmost bit
        bit_data2 = b2 & MASK_SET[7];
        b1 <<= 1;                     // shift to next
        b2 <<= 1;
           
        PORTB |= SET_PIN_CLOCK; //  digitalWrite(PIN_CLOCK, HIGH); 
        if (bit_data1) 
          PORTB |= SET_PIN_DATA1; //digitalWrite(PIN_DATA1, HIGH);
        else
          PORTB &= RES_PIN_DATA1; // digitalWrite(PIN_DATA1, LOW);

        if (DISPLAY_HEIGHT > 32)
        {
          if (bit_data2)
            PORTB |= SET_PIN_DATA2; // digitalWrite(PIN_DATA2, HIGH);
          else 
            PORTB &= RES_PIN_DATA2; // digitalWrite(PIN_DATA2, LOW);
        }
     
        PORTB &= RES_PIN_CLOCK; //        digitalWrite(PIN_CLOCK, LOW);
        //PORTB &= B111101; // digitalWrite(PIN_DATA1, LOW);
      }         
    }    
	PORTB |= SET_PIN_LOAD; //    digitalWrite(PIN_LOAD, HIGH);
    if (i < height - 1) PORTB |= SET_PIN_LOAD; //    digitalWrite(PIN_LOAD, HIGH);  
  }
  PORTB ^= SET_PIN_M; //     digitalWrite(PIN_M, M);  
}
  
bool copyVRAMtoSRAM(uint32_t address)
{
	return false;
}
	
bool readVRAMfromSRAM(uint32_t address)
{
	return false;
}

// shift screen up by <count> pixels
void scrollUpVRAM(int count)
{
  if (count <= 0) return;
  for (int i = 0; i < DISPLAY_HEIGHT - count; i++)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = vram[i + count][j];
    }
  }
  for (int i = DISPLAY_HEIGHT - count; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = 0;
    }
  }
}

// shift screen down by <count> pixels
void scrollDownVRAM(int count)
{
  if (count <= 0) return;
  for (int i = DISPLAY_HEIGHT - 1; i >= count; i--)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = vram[i - count][j];
    }
  }
  for (int i = 0; i < count; i++)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = 0;
    }
  }
}

// scrolls left only by <count>*8 pixels
void scrollLeftVRAMx8(int count)
{
  if (count <= 0) return;
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = 0; j < DISPLAY_WIDTH / 8 - count; j++)
    {
      vram[i][j] = vram[i][j + count];
    }
  }
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = DISPLAY_WIDTH / 8 - count; j < DISPLAY_WIDTH / 8; j++)
    {
      vram[i][j] = 0;
    }
  }
}

// scrolls right only by <count>*8 pixels
void scrollRightVRAMx8(int count)
{
  if (count <= 0) return;
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = DISPLAY_WIDTH / 8 - 1; j >= count; j--)
    {
      vram[i][j] = vram[i][j - count];
    }
  }
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    for (int j = 0; j < count; j++)
    {
      vram[i][j] = 0;
    }
  }
}

void putSprite(byte *sprite, int x, int y, byte mode)
{
  byte* spr = sprite;
  for (int i = 0; i < 8; i++)
  {
    byte spriteLine = pgm_read_byte(spr);
    
    for (int j = 0; j < 8; j++)
    {
      bool spriteBit = spriteLine & MASK_SET[7 - j];
      bool bgBit = getPixel(x + j, y + i);
      bool resBit = 0;
      switch (mode)
      {
        case MODE_PUT: resBit = spriteBit; break;
        case MODE_AND: resBit = bgBit && spriteBit; break;
        case MODE_OR:  resBit = bgBit || spriteBit; break;
        case MODE_XOR: resBit = bgBit ^ spriteBit; break;
        case MODE_INV: resBit = !spriteBit; break;
        case MODE_DEL: resBit = ((spriteBit) ? false : bgBit); break;
      }
      putPixel(x + j, y + i, resBit);
    }
    spr++;
  }
}

void fillRect(int startX, int startY, int width, int height, int color)
{
  for (int i = 0; i < width; i++)
  { 
    for (int j = 0; j < height; j++)
    {
      putPixel(startX + i, startY + j, color);
    }
  }
}

void drawRect(int startX, int startY, int width, int height, int color)
{
  int i;
  int endX = startX + width - 1;
  int endY = startY + height - 1;
  for (i = 0; i < width; i++)
  { 
    putPixel(startX + i, startY, color);
    putPixel(startX + i, endY, color);
  }
  for (i = 0; i < height; i++)
  { 
    putPixel(startX, startY + i, color);
    putPixel(endX, startY + i, color);
  }
}

void putPixel(int x, int y, int color)
{
  if ((y < 0) || (y >= DISPLAY_HEIGHT) || (x < 0) || (x >= DISPLAY_WIDTH))
    return;
    
  if (color == 0)
    vram[y][x / 8] &= MASK_RES[7 - (x % 8)];
  else
    vram[y][x / 8] |= MASK_SET[7 - (x % 8)];
}

bool getPixel(int x, int y)
{
  if ((y >= 0) && (y < DISPLAY_HEIGHT) && (x >= 0) && (x < DISPLAY_WIDTH))
    return vram[y][x / 8] & MASK_SET[7 - (x % 8)];
  else
    return false;
}

// sign of a floating-point number, 
// returns 1 for 0 or greater value
// returns -1 for values less than 0
// we don't care about +0.0f and -0.0f
int signFloat(float x)
{
  if (x >= 0) return 1; else return -1;
}

void drawLine(int startX, int startY, int endX, int endY, int color)
{  
  float y = startY;
  float x = startX;
  float deltaY = 0;
  float deltaX = 0;

  if (endY > startY)
    deltaY = endY - startY + 1;
  else if (endY < startY)
    deltaY = endY - startY - 1;

  if (endX > startX)
    deltaX = endX - startX + 1;
  else if (endX < startX)
    deltaX = endX - startX - 1;  
 
  
  float a_deltaX = abs(deltaX);
  float a_deltaY = abs(deltaY);  
  float dx = 0; 
  float dy = 0;
  if ((deltaX == 0) && (deltaY == 0))
  {
    putPixel(startX, startY, color);
    return;
  }
  if (deltaX == 0)
  {
    dx = 0;
    dy = signFloat(deltaY);
  }
  else if (deltaY == 0)
  {
    dy = 0;
    dx = signFloat(deltaX);
  }
  else if (a_deltaX == a_deltaY)
  {
    dx = signFloat(deltaX);
    dy = signFloat(deltaY);
  }
  else if (a_deltaX > a_deltaY)
  {
    dx = signFloat(deltaX);
    dy = signFloat(deltaY) * a_deltaY / a_deltaX;
  }
  else
  {
    dx = signFloat(deltaX) * a_deltaX / a_deltaY;
    dy = signFloat(deltaY); 
  }

  // end coordinates reached
  bool bx = false;
  bool by = false;
  
  while (!(bx && by))
  {
    putPixel(int(x), int(y), color);
    x = x + dx;
    y = y + dy;
    
    if (dx >= 0)
      bx = x >= endX;
    else
      bx = x <= endX;

    if (dy >= 0)
      by = y >= endY;
    else
      by = y <= endY;
  }
  putPixel(int(x), int(y), color);
}

void drawCircle(int x, int y, int r, int color)
{
   if (r == 0)
   {
      return;
   }
   float delta_phi = 360.0 / (2 * PI * r);
   float phi = 0;
   while (phi < 360)
   {
      float rad = float(phi) / 180.0 * PI;
      float dx1 = round(x + sin(rad) * r);
      //float dx2 = round(x - sin(rad) * r);
      float dy = round(y + cos(rad) * r);
      putPixel(int(dx1), int(dy), color);
      //putPixel(int(dx2), int(dy), color);
      phi = phi + delta_phi;
   }
}

void fillCircle(int x, int y, int r, int color)
{
  if (r == 0)
  {
      return;
  }
  float delta_phi = 360.0 / (2 * PI * r);
  float phi = 0;
  while (phi < 180)
  {
      float rad = float(phi) / 180.0 * PI;
      float dx1 = round(x - sin(rad) * r);
      float dx2 = round(x + sin(rad) * r);
      float dy = y + cos(rad) * r;
      drawHorLine(int(dx1), int (dx2), int(dy), color);
      
      phi = phi + delta_phi;
  }
}

void drawText(char* str, int x, int y, bool wrap)
{
  char* ch = str;
  int c = x;
  int r = y;
  while (*ch != '\0')
  {
    // control / special characters 
    if (*ch >= ASCII_LIMIT)
    {
      ch++;
      continue;
    }
    if (*ch == 8)
    {
      if (c > DIGIT_WIDTH)
      {
         c = c - DIGIT_WIDTH - 1;
      }
      ch++;
      continue;
    }
    if (*ch == 9)
    {
      while (c % TAB_SIZE_GRAPH != 0)
      {
        c++;
      }
      ch++;
      continue;    
    }
    if (((EOL_SEQ == CHAR_LF) && (*ch == 10)) || (((EOL_SEQ == CHAR_CR) || (EOL_SEQ == CHAR_CRLF)) && (*ch == 13)))
    {
      char* next_ch = ch + 1;
      if ((EOL_SEQ == CHAR_CRLF) && (*next_ch == 10) || (EOL_SEQ != CHAR_CRLF))
      {
        c = 0;
        r += CHAR_HEIGHT;
        ch++;
        continue;
      }
    }
    if ((EOL_SEQ != CHAR_LF) && (*ch == 10))
    {
      ch++;
      continue;
    }
    if (*ch == 12)
    {
      clearVRAM();
      c = 0;
      r = 0;
      ch++;
      continue;
    }
    // check if next char is inside screen margins
    if (c > DISPLAY_WIDTH - DIGIT_WIDTH)
    {
      if (wrap)
      {
        c = 0;
        r += CHAR_HEIGHT;
      }
      else
        break;
    }
    if (r > DISPLAY_HEIGHT - CHAR_HEIGHT)
    {
       if (wrap)
       {
         scrollUpVRAM(r + CHAR_HEIGHT - DISPLAY_HEIGHT);
         r = DISPLAY_HEIGHT - CHAR_HEIGHT;
       }
       else
         break;
    }
    
    // regular characters
    for (int j = 0; j < DIGIT_WIDTH; j++)
    {
      byte column = pgm_read_byte(&(charTable5x7[*ch][j]));
      for (int i = 0; i < CHAR_HEIGHT; i++)
      {          
        bool dBit = column & MASK_SET[7 - i];
        if (dBit)
          vram[r + i][(c + j) / 8] |= MASK_SET[7 - (c + j) % 8];
        else
          vram[r + i][(c + j) / 8] &= MASK_RES[7 - (c + j) % 8];
      }
    }
    c = c + DIGIT_WIDTH + 1;
    if (c + DIGIT_WIDTH > DISPLAY_WIDTH)
    {
      if (wrap)
      {
        c = 0;
        r += CHAR_HEIGHT;
        if (r >= DISPLAY_HEIGHT - CHAR_HEIGHT)
        {
          scrollUpVRAM(8);
          r = DISPLAY_HEIGHT - CHAR_HEIGHT;
        }
      }
      else
        break;
    }
    ch++;
  }
  return;
}

void dispText(char* str, int col, int row, bool wrap)
{
  char* ch = str;
  int c = col;
  int r = row;
  while (*ch != 0)
  {
    if (*ch >= ASCII_LIMIT)
    {
      ch++;
      continue;
    }
    if (*ch == 8)
    {
      if (c > 0)
        c--;
      else
      {
        if (r > 0) r--;
      }
      ch++;
      continue;
    }
    if (*ch == 9)
    {
      if (c + TAB_SIZE_TXT < DISPLAY_WIDTH / 8)
        c += TAB_SIZE_TXT;
      else
      {
        c = 0;
        r++;
      }
      ch++;
      continue;
    }
    if (*ch == 12)
    {
      clearVRAM();
      c = 0;
      r = 0;
      ch++;
      continue;
    }
    
    if (c >= DISPLAY_WIDTH / 8)
    {
      if (wrap)
      {
        c = 0;
        r++;
      }
      else 
        break;
    }
    if (r >= DISPLAY_HEIGHT / 8)
    {
      if (wrap)
      {
        scrollUpVRAM(8);
        r = DISPLAY_HEIGHT / 8 - 1;
      }
      else
        break;
    }
    for (int i = 0; i < CHAR_HEIGHT; i++)
    {
        vram[r * 8 + i][c] = pgm_read_byte(&(charTable7x7[*ch][i]));
    }
    c++;    
    ch++;
  }
  return;
}

// show number
void putInt(long num, int x, int y, int width)
{
  long n = num;
  int d = 0;
  int c = x;
  int r = y;
  long ord = 10000;
  if (width > 0) {
    ord = 1;
    for (int i = 1; i < width; i++)
    {
      ord = ord * 10;
    }
  }
  bool start = width > 0;
  do {
    d = n / ord;
    if (!start && (d > 0))
      start = true;
      
    if ((d > 0) || start)
    {      for (int j = 0; j < DIGIT_WIDTH; j++)
      {
        byte column = pgm_read_byte(&(digitTable[d][j]));
        for (int i = 0; i < CHAR_HEIGHT; i++)
        {          
          bool dBit = column & MASK_SET[7];
          column <<= 1;
          //putPixel(col + j, row + i, dBit);
          if (dBit)
            vram[r + i][(c + j) / 8] |= MASK_SET[7 - (c + j) % 8];
          else
            vram[r + i][(c + j) / 8] &= MASK_RES[7 - (c + j) % 8];
          //bitWrite(vram[row + i][(c + j) / 8], 7 - (c + j) % 8, dBit);
        }
      }
      c = c + DIGIT_WIDTH + 1;
      if (c + DIGIT_WIDTH > DISPLAY_WIDTH)
      {
        /*c = 0;
        r += CHAR_HEIGHT;
        */
        break;
      }
    }
    n = n % ord;  
    ord = ord / 10;
    //if (ord == 0) break;
    
  } while (ord > 0);
}

void drawHorLine(int startX, int endX, int y, int color)
{
  int sx = startX;
  int ex = endX;
  if (sx > ex)
  {
    sx = endX;
    ex = startX;
  }
  for (int x = sx; x <= ex; x++)
  {
    putPixel(x, y, color);
  }
}

void drawVertLine(int startY, int endY, int x, int color)
{
  int sy = startY;
  int ey = endY;
  if (sy > ey)
  {
    sy = endY;
    ey = startY;
  }
  for (int y = sy; y <= ey; y++)
  {
    putPixel(x, y, color);
  }
}

void setTabSize(int chars)
{
  TAB_SIZE_TXT = chars;
  TAB_SIZE_GRAPH = chars * (DIGIT_WIDTH + 1);
}

void setEndOfLineChar(int eol)
{
  if ((eol == CHAR_LF) || (eol == CHAR_CR) || (eol == CHAR_CRLF))
  {
    EOL_SEQ = eol;
  }
}

void backLightOn()
{
	BACKLIGHT = true;
}

void backLightOff()
{
	BACKLIGHT = false;
}

void backLightSetBrightness(unsigned int number)
{
	brightness = number * (DISPLAY_WIDTH / 2) / 100;
}