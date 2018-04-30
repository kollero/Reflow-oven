//avr-gcc libary to work on atmel studio
#include "S6D0154.h"
#include "pin_magic.h"

//fonts
#include "glcdfont.c"
#include "sans15x7.h"
#include "FreeSerifBold18pt7b.h"
#include "FreeSans18pt7b.h"

typedef struct{
	unsigned int bit0:1;
	unsigned int bit1:1;
	unsigned int bit2:1;
	unsigned int bit3:1;
	unsigned int bit4:1;
	unsigned int bit5:1;
	unsigned int bit6:1;
	unsigned int bit7:1;
} _io_reg;
#define REGISTER_BIT(rg,bt) ((volatile _io_reg*)&rg)->bit##bt

/*
   *csPort      = (PORTC & _BV(3)) & (DDRC & _BV(3)); //portOutputRegister(digitalPinToPort(cs)); //from arduino
   *cdPort      = (PORTC & _BV(2)) & (DDRC & _BV(2)); 
   *wrPort		= (PORTC & _BV(1)) & (DDRC & _BV(1));
   *rdPort  	= (PORTC & _BV(0)) & (DDRC & _BV(0));
  
  csPinSet   = PINC & _BV(3); //PC3 or writing PORTC & _BV(3);
  cdPinSet   = PINC & _BV(2); //PC2 LCD_RS
  wrPinSet   = PINC & _BV(1); //PC1
  rdPinSet   = PINC & _BV(0); //PC0
 */

#define	csPort REGISTER_BIT(PORTC,3) 
#define	cdPort REGISTER_BIT(PORTC,2) 
#define	wrPort REGISTER_BIT(PORTC,1) 
#define	rdPort REGISTER_BIT(PORTC,0)
#define	resetPort REGISTER_BIT(PORTC,4)

#define	csPinSet REGISTER_BIT(PINC,3) 
#define	cdPinSet REGISTER_BIT(PINC,2) 
#define	wrPinSet REGISTER_BIT(PINC,1) 
#define	rdPinSet REGISTER_BIT(PINC,0) 
#define	resetPinSet REGISTER_BIT(PINC,4)
// When using the TFT breakout board, control pins are configurable.
// Signals are ACTIVE LOW

#define RD_IDLE    rdPort = 1//~rdPinSet //PC0
#define RD_ACTIVE  rdPort = 0//rdPinSet

#define CD_DATA    cdPort = 1//~cdPinSet //PC2 1
#define CD_COMMAND cdPort = 0//cdPinSet //   0

#define WR_IDLE    wrPort = 1//~wrPinSet //PC1 //1
#define WR_ACTIVE  wrPort = 0//wrPinSet
#define CS_IDLE    csPort = 1//~csPinSet //PC3 //1
#define CS_ACTIVE  csPort = 0//csPinSet

#define RESET_ACTIVE  resetPort = 0//resetPinSet
#define RESET_NOT  resetPort = 1//~resetPinSet

//ends here

volatile int _width=0,
			  _height=0,
			  rotation=0;	



#define TFTWIDTH 240
#define TFTHEIGHT 320

#define LCD_DELAY 0xFF
/*
const uint16_t S6D0154_regValues[] PROGMEM = {
//0x00, //first index is null
			0x11, 0x001A,
            0x12, 0x3121,
            0x13, 0x006C,
            0x14, 0x4249,
            0x10, 0x0800,
            LCD_DELAY, 30,
            0x11, 0x011A,
            LCD_DELAY, 30,
            0x11, 0x031A,
            LCD_DELAY, 30,
            0x11, 0x071A,
            LCD_DELAY, 30,
            0x11, 0x0F1A,
            LCD_DELAY, 20,
            0x11, 0x0F3A,
            LCD_DELAY, 100,
            0x01, 0x0128,
            0x02, 0x0100,
            0x03, 0x1030,
            0x07, 0x1012,
            0x08, 0x0303,
            0x0B, 0x1100,
            0x0C, 0x0000,
            0x0F, 0x1801,
            0x15, 0x0020,
            0x07, 0x0012,
            LCD_DELAY, 40,
            0x07, 0x0013,        //GRAM Address Set 
            0x07, 0x0017       //Display Control DISPLAY ON 
 };
*/

//remember to only call init_S6D0154 when initializing
void init_S6D0154(void) {
  
  setWriteDir(); //sets write direction of data pins in pin magic
 CS_IDLE;
  WR_IDLE;
   CD_DATA; //added
   
RESET_ACTIVE;
  delay_ms(200); 
RESET_NOT; //reset off
  delay_ms(200); 
  // Data transfer sync
  CS_ACTIVE;
  CD_COMMAND;
  write8(0x00);
  for(uint8_t i=0; i<3; i++) WR_STROBE; // Three extra 0x00s
  
 RD_IDLE;
  CS_IDLE; // Set all control bits high
  WR_IDLE;

  CD_COMMAND;

  delay_ms(200);
CS_ACTIVE;  

   //writeRegister16(0x80,0x008D); //Testkey
   //writeRegister16(0x92,0x0010);
   writeRegister16(0x11,0x001A); //power control 2 VCL1 amplifier control  2.7V
   writeRegister16(0x12,0x3121); //power control 3 dc21 and dc30, bt2,bt1 dc10
   writeRegister16(0x13,0x006C);
   writeRegister16(0x14,0x4249);
   writeRegister16(0x10,0x0800);
   delay_ms(30);
   writeRegister16(0x11,0x011A);
   delay_ms(30);
   writeRegister16(0x11,0x031A);
   delay_ms(30);
   writeRegister16(0x11,0x071A);
   delay_ms(30);
   writeRegister16(0x11,0x0F1A);
   delay_ms(20);
   writeRegister16(0x11,0x0F3A);
   delay_ms(100);
   writeRegister16(0x01,0x0128); //ss, nl5, nl3  //bin13 rising edge data capture with set to 0 
   // image mirroring is on ss
   // drive line is 160 or 720x160 number of horizontal lines 
   writeRegister16(0x02,0x0100); //two line mixed inversion
   writeRegister16(0x03,0x1030); //automatic increment in address direction vertical and horizontal
   //if trying to write long outside display, will automatically move to next line or to (0,0) position
   writeRegister16(0x07,0x1012); //normal display, operate and display is on, (colours are on)
   writeRegister16(0x08,0x0303); //fp1,fp0 bp1-0
   writeRegister16(0x0B,0x1100);
   writeRegister16(0x0C,0x0000);
   writeRegister16(0x0F,0x1801);
   writeRegister16(0x15,0x0020);
   writeRegister16(0x07,0x0012);
   delay_ms(40);
   writeRegister16(0x07,0x0013);//  GRAM Address Set 
   writeRegister16(0x07,0x0017);//  Display Control  DISPLAY ON 
   
    CS_IDLE;
  
	
/*
  uint16_t a, d;
  uint16_t i = 0;
  while(i < sizeof(&S6D0154_regValues) ) {
      a = pgm_read_byte(&S6D0154_regValues[i++]);
	
      d = pgm_read_byte(&S6D0154_regValues[i++]);

      if(a == LCD_DELAY) {
	  delay_ms(d);
	  }
      else { 
	  CS_ACTIVE;
	  writeRegister16(a, d);
	  CS_IDLE;
	  }
    }
  */
  
 
  rotation  = 3;
  //cursor_y  = cursor_x = 0;
  //textsize  = 1;
  //textcolor = 0xFFFF;
  
   _width    = TFTWIDTH;
   _height   = TFTHEIGHT;
   
  setRotation(rotation); //set right rotation here
  setAddrWindow(0, 0, TFTWIDTH-1, TFTHEIGHT-1); //with cs_idle
  return;
  
} 


void setRotation(uint8_t x) {
  rotation = (x & 3);
  switch(rotation) {
   case 0:
   case 2:
   _width  = TFTWIDTH;
   _height = TFTHEIGHT;
    break;
   case 1:
   case 3:
    _width  = TFTHEIGHT;
    _height = TFTWIDTH;
    break;
  }
  
  uint16_t t;
    switch(rotation) {
     default: t = 0x1030; break;
     case 1 : t = 0x1028; break;
     case 2 : t = 0x1000; break;
     case 3 : t = 0x1018; break;
    }
	writeRegister16(0x0003, t); // MADCTL
  
  
  
  return;
}


void setAddrWindow(int x1, int y1, int x2, int y2) {
  CS_ACTIVE;
 int x, y, t;
    switch(rotation) {
     default:
      x  = x1;
      y  = y1;
      break;
     case 1:
      t  = y1;
      y1 = x1;
      x1 = TFTWIDTH  - 1 - y2;
      y2 = x2;
      x2 = TFTWIDTH  - 1 - t;
      x  = x2;
      y  = y1;
      break;
     case 2:
      t  = x1;
      x1 = TFTWIDTH  - 1 - x2;
      x2 = TFTWIDTH  - 1 - t;
      t  = y1;
      y1 = TFTHEIGHT - 1 - y2;
      y2 = TFTHEIGHT - 1 - t;
      x  = x2;
      y  = y2;
      break;
     case 3:
      t  = x1;
      x1 = y1;
      y1 = TFTHEIGHT - 1 - x2;
      x2 = y2;
      y2 = TFTHEIGHT - 1 - t;
      x  = x1;
      y  = y2;
      break;
    }
	
	writeRegister16(0x37, x1); //HorizontalStartAddress
	writeRegister16(0x36, x2); //HorizontalEndAddress
	writeRegister16(0x39, y1); //VerticalStartAddress
	writeRegister16(0x38, y2); //VertocalEndAddress
	writeRegister16(0x20, x); //GRAM Address Set
	writeRegister16(0x21, y);
	writeRegister8(0x22, 0);

  CS_IDLE;
  return;
}

void flood(uint16_t color, uint32_t len) {
  uint16_t blocks;
  uint8_t  i, hi = color >> 8,
              lo = color;

  CS_ACTIVE;
  CD_COMMAND;
  write8(0x22);
 
  // Write first pixel normally, decrement counter by 1
  CD_DATA;
  write8(hi);
  write8(lo);
  len--;

  blocks = (uint16_t)(len / 64); // 64 pixels/block
  if(hi == lo) {
    // High and low bytes are identical.  Leave prior data
    // on the port(s) and just toggle the write strobe.
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // 2 bytes/pixel
        WR_STROBE; WR_STROBE; WR_STROBE; WR_STROBE; // x 4 pixels
      } while(--i);
    }
    // Fill any remaining pixels (1 to 64)
    for(i = (uint8_t)len & 63; i--; ) {
      WR_STROBE;
      WR_STROBE;
    }
  } else {
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        write8(hi); write8(lo); write8(hi); write8(lo);
        write8(hi); write8(lo); write8(hi); write8(lo);
      } while(--i);
    }
    for(i = (uint8_t)len & 63; i--; ) {
      write8(hi);
      write8(lo);
    }
  }
  CS_IDLE;
}

void drawPixel(int16_t x, int16_t y, uint16_t color) {

  // Clip
 	//if((x < 0) || (y < 0) || (x >= _width) || (y >= _height)) return;

  CS_ACTIVE;
  
    int16_t t;
    switch(rotation) {
     case 1:
      t = x;
      x = TFTWIDTH  - 1 - y;
      y = t;
      break;
     case 2:
      x = TFTWIDTH  - 1 - x;
      y = TFTHEIGHT - 1 - y;
      break;
     case 3:
      t = x;
      x = y;
      y = TFTHEIGHT - 1 - t;
      break;
	}

    writeRegister16(0x20, x);
    writeRegister16(0x21, y);
    writeRegister16(0x22, color);

  CS_IDLE;
  return;
}

void drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color) {
  int16_t y2;

  // Initial off-screen clipping
  if((length <= 0      ) ||
     (x      <  0      ) || ( x                  >= _width) ||
     (y      >= _height) || ((y2 = (y+length-1)) <  0     )) return;
  if(y < 0) {         // Clip top
    length += y;
    y       = 0;
  }
  if(y2 >= _height) { // Clip bottom
    y2      = _height - 1;
    length  = y2 - y + 1;
  }

  setAddrWindow(x, y, x, y2);
  flood(color, length);

}

void drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color) {
  int16_t x2;

  // Initial off-screen clipping
  if((length <= 0     ) ||
     (y      <  0     ) || ( y                  >= _height) ||
     (x      >= _width) || ((x2 = (x+length-1)) <  0      )) return;

  if(x < 0) {        // Clip left
    length += x;
    x       = 0;
  }
  if(x2 >= _width) { // Clip right
    x2      = _width - 1;
    length  = x2 - x + 1;
  }

  setAddrWindow(x, y, x2, y);
  flood(color, length);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep != 0) {
	int16_t tmpvar=0;
	tmpvar = x0;
	x0=y0;
	y0=tmpvar;
	
	tmpvar=0;	
	tmpvar = x1;
	x1=y1;
	y1=tmpvar;	
 
  }

  if (x0 > x1) {
	int16_t tmpvar2=0;
	
	tmpvar2 = x0;
	x0=x1;
	x1=tmpvar2;
	
	tmpvar2=0;	
	tmpvar2 = y0;
	y0=y1;
	y1=tmpvar2;	

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
      drawPixel(y0, x0, color);
    } else {
      drawPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void writeRegister8(uint8_t a, uint8_t d) {

 writeRegister8inline(a,d); 

  return;
}

void writeRegister16(uint16_t a, uint16_t d) {
 writeRegister16inline(a, d);
 
    return;
}

void setWriteDir(void) {
  setWriteDirInline();
}

void delay_ms( int ms ){
	for (int i = 0; i < ms; i++)
	{
		_delay_ms(1);
	}
}


void fillScreen(uint16_t color) {
   //setAddrWindow(0, 0, _width - 1, _height - 1);
    setAddrWindow(0, 0, _width - 1, _height - 1);
  flood(color, (long)_width * (long)_height);
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void drawChar_gfx(int16_t x, int16_t y, unsigned char c, uint16_t color,uint16_t bg, uint8_t size) {
//gfx font
c =c-0x20;
    uint16_t bo = pgm_read_word(&FreeSans18pt7bGlyphs[c][0]); //read word instead of byte
    uint8_t  w  = pgm_read_byte(&FreeSans18pt7bGlyphs[c][1]),
             h  = pgm_read_byte(&FreeSans18pt7bGlyphs[c][2]);
             //xa = pgm_read_byte(&FreeSans18pt7bGlyphs[c][3]);
    int8_t   xo = pgm_read_byte(&FreeSans18pt7bGlyphs[c][4]),
             yo = pgm_read_byte(&FreeSans18pt7bGlyphs[c][5]);
    
	uint8_t  xx, yy, bit = 0;
	uint16_t bits =0;
    int16_t  xo16 =0; 
	int16_t yo16 =0;

    if(size > 1) {
      xo16 = xo;
      yo16 = yo;
    }

for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = pgm_read_byte(&FreeSans18pt7bBitmaps[bo++]);
        }
        if(bits & 0x80) {
          if(size == 1) {
            drawPixel(x+xo+xx-1, y+yo+yy, color);
          } else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
		  }
		
		else if(bg != color){
		 if(size == 1) {
            drawPixel(x+xo+xx-1, y+yo+yy, bg);
          } else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, bg);
          }
		}		
        
        bits <<= 1;
      }
    }
}


// Draw a character
void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,uint16_t bg, uint8_t size) {

//THESE work don't touch!!!!!!!!!!!!!!!
//THESE work don't touch!!!!!!!!!!!!!!!
//THESE work don't touch!!!!!!!!!!!!!!!
//THESE work don't touch!!!!!!!!!!!!!!!
//THESE work don't touch!!!!!!!!!!!!!!!

//gfx font
/*
c =c-0x20;
    uint16_t bo = pgm_read_word(&FreeSans18pt7bGlyphs[c][0]); //read word instead of byte
    uint8_t  w  = pgm_read_byte(&FreeSans18pt7bGlyphs[c][1]),
             h  = pgm_read_byte(&FreeSans18pt7bGlyphs[c][2]);
             //xa = pgm_read_byte(&FreeSans18pt7bGlyphs[c][3]);
    int8_t   xo = pgm_read_byte(&FreeSans18pt7bGlyphs[c][4]),
             yo = pgm_read_byte(&FreeSans18pt7bGlyphs[c][5]);
    
	uint8_t  xx, yy, bit = 0;
	uint16_t bits =0;
    int16_t  xo16 =0; 
	int16_t yo16 =0;

    if(size > 1) {
      xo16 = xo;
      yo16 = yo;
    }

for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = pgm_read_byte(&FreeSans18pt7bBitmaps[bo++]);
        }
        if(bits & 0x80) {
          if(size == 1) {
            drawPixel(x+xo+xx-1, y+yo+yy, color);
          } else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
		  }
		  
		else if(bg != color){
		 if(size == 1) {
            drawPixel(x+xo+xx-1, y+yo+yy, bg);
          } else {
            fillRect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, bg);
          }
		}		
        
        bits <<= 1;
      }
    }
*/




//basic 5x7 font
if((x >= _width)            || // Clip right
       (y >= _height)           || // Clip bottom
       ((x + 6 * size - 1) < 0) || // Clip left
       ((y + 8 * size - 1) < 0))   // Clip top
      return;

    if( c >= 176) c++; // Handle 'classic' charset behavior

    for(int8_t i=0; i<6; i++ ) {
      uint8_t line;
      if(i < 5) line = pgm_read_byte(font+(c*5)+i);
      else      line = 0x0;
      for(int8_t j=0; j<8; j++, line >>= 1) {
        if(line & 0x1) {
          if(size == 1) drawPixel(x+i, y+j, color);
          else          fillRect(x+(i*size), y+(j*size), size, size, color);
        } else if(bg != color) {
          if(size == 1) drawPixel(x+i, y+j, bg);
          else          fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
    }
	
if((x >= _width)            || // Clip right
       (y >= _height)           || // Clip bottom
       ((x + 6 * size - 1) < 0) || // Clip left
       ((y + 8 * size - 1) < 0))   // Clip top
      return;


/*
//self made fonts
	c=c-0x20;
	for(uint8_t i=0; i<= 9;) {
      uint8_t line;
	  if(i <= 8){ 
	  line = pgm_read_byte(&Sans_Serif9x15[(c*19)+1+i*2]);	
	  }
      else {
	  line = 0x0;
	  }
	
      for(uint8_t j=0; j < 8; ) {
        if(line & 0x1 ) {
          if(size == 1) drawPixel(x+i, y+j, color);
          else          fillRect(x+(i*size), y+(j*size), size, size, color);
        } else if(bg != color) {
          if(size == 1) drawPixel(x+i, y+j, bg);
          else          fillRect(x+i*size, y+j*size, size, size, bg);
        }
		j++;
		line >>= 1;
      }
	  i++;
	}

	for(uint8_t i=0; i<= 9;) {
	  uint8_t line2;
	  if(i <= 8){
	  line2 = pgm_read_byte(&Sans_Serif9x15[(c*19)+2+i*2]);
	  }
      else {
	  line2 = 0x0;	
		}
	  	
	  for(uint8_t jj=8; jj < 16; ) { //line2 > 0x0;
		if(line2 & 0x1 ) { //not zero
          if(size == 1) drawPixel(x+i, y+jj, color);
          else          fillRect(x+(i*size), y+(jj*size), size, size, color);
        } else if(bg != color) {
          if(size == 1) drawPixel(x+i, y+jj, bg);
          else          fillRect(x+i*size, y+jj*size, size, size, bg);
        }
		jj++;
		line2 >>= 1;
	  }
	  i++;
	}	
	*/
}



void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  // Update in subclasses if desired!
  for (int16_t i=x; i<x+w; i++) {
    drawFastVLine(i, y, h, color);
  }
}


// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
	int i=0, j=len-1, temp;
	while (i<j)
	{
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++; j--;
	}
}

// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
	int i = 0;
	while (x)
	{
		str[i++] = (x%10) + '0';
		x = x/10;
	}
	
	// If number of digits required is more, then
	// add 0s at the beginning
	while (i < d)
	str[i++] = '0';
	reverse(str, i);	
	str[i] = '\0';
	return i;
}
// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
	// Extract integer part
	int ipart = (int)n;
	
	// Extract floating part
	float fpart =(float) (n - (float)ipart);
	
	// convert integer part to string

	//int i = intToStr(ipart, res, 0);

	int i = intToStr(ipart, res, 1); //forces to show 0 at the beginning

	// check for display option after point
	if (afterpoint != 0)
	{
		res[i] = '.';  // add dot
		
		// Get the value of fraction part upto given no.
		// of points after dot. The third parameter is needed
		// to handle cases like 233.007
		fpart = fpart * pow(10, afterpoint);
		
		intToStr((int)fpart, res + i + 1, afterpoint);
	}
}	

void drawString( int16_t x, int16_t y, char stri[], uint16_t color,uint16_t bg, uint8_t size){
	uint8_t i=0;
	char k;
	while(i < strlen(stri)){
	k=stri[i];
	drawChar(x+(i*(size)*6), y, k, color,bg, size);
	i++;
	} 
}

void drawString_gfx( int16_t x, int16_t y, char stri[], uint16_t color,uint16_t bg, uint8_t size){
	uint8_t ii=0;
	char kk;
	while(ii < strlen(stri)){
	kk=stri[ii];
	drawChar_gfx(x+(ii*(size)*17), y, kk, color,bg, size);
	ii++;
	} 
}

void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
  drawFastVLine(x0, y0-r, 2*r+1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color) {

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

    if (cornername & 0x1) {
      drawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      drawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      drawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y+h-1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x+w-1, y, h, color);
}

// Draw a rounded rectangle
void drawRoundRect(int16_t x, int16_t y, int16_t w,int16_t h, int16_t r, uint16_t color) {
  // smarter version
  drawFastHLine(x+r  , y    , w-2*r, color); // Top
  drawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  drawFastVLine(x    , y+r  , h-2*r, color); // Left
  drawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

void drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
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
      drawPixel(x0 + x, y0 + y, color);
      drawPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      drawPixel(x0 + x, y0 - y, color);
      drawPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      drawPixel(x0 - y, y0 + x, color);
      drawPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      drawPixel(x0 - y, y0 - x, color);
      drawPixel(x0 - x, y0 - y, color);
    }
  }
}


