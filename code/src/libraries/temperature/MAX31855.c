
#define _CSMAX31855			REGISTER_BIT(PORTD,0) //always write dir
//#define _SCK				REGISTER_BIT(PORTB,5) //clk out
//#define _MISO				REGISTER_BIT(PORTB,4) //read bit pb4 pinB,4

#define MAX31855_ACTIVE 	_CSMAX31855=0	
#define MAX31855_IDLE 		_CSMAX31855=1
//#define SCKHIGH				_SCK=1
//#define SCKLOW				_SCK=0

#include "MAX31855.h"



void MAX31855_init(void) {
  	
MAX31855_IDLE; 
  
}

double MAX31855_readInternal(void) {
//MAX31855_IDLE; 
//delay_ms(1);
//spi enabled, clk/16 mode, master
	SPCR=0b01010010; //0b01010001; //now fixed with rising edge sampling 
 uint32_t v;
  v = MAX31855_spireadhw32();

  // ignore bottom 4 bits - they're just thermocouple data
  v >>= 4;

  // pull the bottom 11 bits off
  float internal = v & 0x7FF;
  // check sign bit!
  if (v & 0x800) {
    // Convert to negative value by extending sign and casting to signed type.
    int16_t tmp = 0xF800 | (v & 0x7FF);
    internal = tmp;
  }
  internal *= 0.0625; // LSB = 0.0625 degrees
	
  return internal;
}

double MAX31855_readCelsius(void) {
//MAX31855_IDLE; 
//delay_ms(1);
//spi enabled, clk/16 mode, master
	SPCR=0b01010010; //0b01010001; //now fixed with rising edge sampling
  uint32_t v;
  v = MAX31855_spireadhw32();
  
  /*
//  if (v & 0x7) { //0B0111
    // uh oh, a serious problem! //bits are: short to vcc,gnd and open circuit
  //  return NAN; //or NAN
  //}
 */

  if (v & 0x80000000) { //0B1000 0000 0000 0000 0000 0000 0000 0000
    // Negative value, drop the lower 18 bits and explicitly extend sign bits.
	 v = 0xFFFFC000 | ((v >> 18) & 0x00003FFFF);   //   1111 1111 1111 1111 1100 0000 0000 0000     &0B0011 1111 1111 1111 1111
  }
  else {
    // Positive value, just drop the lower 18 bits.
    v >>= 18;
  }
  double centigrade = v;

  // LSB = 0.25 degrees C
  centigrade *= 0.25;
  return centigrade;
}

uint32_t MAX31855_spireadhw32(void) { 
uint32_t d = 0;
uint8_t dummy=0x00;
uint8_t tmp=0;

	MAX31855_ACTIVE;
	_delay_us(1);
    tmp = TransactSPI(dummy);
	d=d+tmp;
	d<<=8;
	tmp = TransactSPI(dummy);
	d=d+tmp;
	d<<=8;
	tmp = TransactSPI(dummy);
	d=d+tmp;
	d<<=8;
	tmp = TransactSPI(dummy);
	d=d+tmp;

 MAX31855_IDLE;
  return d;
}

uint8_t TransactSPI(uint8_t data){
    SPDR = data;
    while(!(SPSR & (1<<SPIF)))
    {
    //do nothing, loop itself is a jump (2 instr. cycles?)
    }
    return SPDR;
}
/*
//reads all bits at once in software SPI mode!!
uint32_t MAX31855_spiread32(void) { 
  
  uint32_t d = 0;
  
	SCKLOW;
	_delay_us(1);
	MAX31855_ACTIVE;
	_delay_us(1);

  for (int i=31; i>=0; i--)
  {
    SCKLOW;
    _delay_us(1);
    d <<= 1;
    if (_MISO) {
      d |= 1;
    }
    SCKHIGH;
    _delay_us(1);
  }

  MAX31855_IDLE;
  return d;
}
*/