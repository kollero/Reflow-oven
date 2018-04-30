
void init_S6D0154(void);
void setRotation(uint8_t x);
void setAddrWindow(int x1, int y1, int x2, int y2);
void writeRegister16(uint16_t a, uint16_t d);
void drawPixel(int16_t x, int16_t y, uint16_t color);

  void         write8(uint8_t value);

   void        setWriteDir(void);

   void        setReadDir(void);

    void       writeRegister8(uint8_t a, uint8_t d);
 void       writeRegister16(uint16_t a, uint16_t d);
// void   writeRegister24(uint8_t a, uint32_t d);
 // void  writeRegister32(uint8_t a, uint32_t d);

 void          flood(uint16_t color, uint32_t len);
 
void drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color);
void drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,uint16_t color);
 
void delay_ms( int ms );
void flood(uint16_t color, uint32_t len);
void fillScreen(uint16_t color);
uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
void drawChar_gfx(int16_t x, int16_t y, unsigned char c, uint16_t color,uint16_t bg, uint8_t size);
void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,uint16_t bg, uint8_t size);
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void reverse(char *str, int len);
int intToStr(int x, char str[], int d);
void ftoa(float n, char *res, int afterpoint);


void drawString( int16_t* x, int16_t* y, char* stri[], uint16_t* color,uint16_t* bg, uint8_t* size);
void drawString_gfx( int16_t* x, int16_t* y, char* stri[], uint16_t* color,uint16_t* bg, uint8_t* size);
void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) ;			  
void drawRoundRect(int16_t x, int16_t y, int16_t w,int16_t h, int16_t r, uint16_t color);
void drawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color); 
