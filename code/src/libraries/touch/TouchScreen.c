#include "TouchScreen.h"

// increase or decrease the touchscreen oversampling. This is a little different than you make think:
// 1 is no oversampling, whatever data we get is immediately returned
// 2 is double-sampling and we only return valid data if both points are the same
// 3+ uses insert sort to get the median value.
// We found 2 is precise yet not too slow so we suggest sticking with it!
#define NUMSAMPLES 2

#define pressureThreshhold 10

//setting up macros for reading bytes and setting write direction
//since using same pins at touchscreen as with the display driver
//remember to set the DDR or write direction back to the original
//XP and YM default set to DDR outputs as display data[B7,B6] lines
//YP and XM have to have DDR set as input and back to output

#define _xmdir 		REGISTER_BIT(DDRC,2) //first write directions 
#define _ypdir 		REGISTER_BIT(DDRC,1)
#define _xpdir 		REGISTER_BIT(DDRD,5) 
#define _ymdir 		REGISTER_BIT(DDRD,6)

#define _xmport 		REGISTER_BIT(PORTC,2) //then pin toggles
#define _ypport			REGISTER_BIT(PORTC,1)
#define _xpport 		REGISTER_BIT(PORTD,5)
#define _ymport 		REGISTER_BIT(PORTD,6)

#define XMReadDir 		_xmdir =0
#define XMWriteDir 		_xmdir =1
#define YPReadDir 		_ypdir =0
#define YPWriteDir 		_ypdir =1
#define XPReadDir 		_xpdir =0
#define XPWriteDir 		_xpdir =1
#define YMReadDir 		_ymdir =0
#define YMWriteDir 		_ymdir =1

#define XMLOW 			_xmport =0
#define XMHIGH 			_xmport =1
#define YPLOW 			_ypport =0
#define YPHIGH 			_ypport =1
#define XPLOW 			_xpport =0
#define XPHIGH 			_xpport =1
#define YMLOW 			_ymport =0
#define YMHIGH 			_ymport =1

volatile uint16_t _rxplate = 0;

void TouchScreen(uint16_t rxplate) {
  _rxplate = rxplate;
 return;
}

#if (NUMSAMPLES > 2)
void insert_sort(uint8_t array[], uint8_t size) {
  uint8_t j;
  uint8_t save;
  for (uint8_t i = 1; i < size; i++) {
    save = array[i];
    for (j = i; j >= 1 && save < array[j - 1]; j--)
      array[j] = array[j - 1];
    array[j] = save; 
  }
return;
  }
#endif

void getPoint( uint16_t *x_point, uint16_t *y_point, double *z_point) {
 //_delay_us(1);
  //int x, y, z;
  int samples[NUMSAMPLES];
  uint8_t i, valid;
  
  valid = 1;
 
 // pinMode(_yp, INPUT);
  //pinMode(_ym, INPUT);
  //digitalWrite(_yp, LOW);
 // digitalWrite(_ym, LOW);
  
 // pinMode(_xp, OUTPUT);
 // pinMode(_xm, OUTPUT);
 // digitalWrite(_xp, HIGH);
 // digitalWrite(_xm, LOW);
 
  YPLOW;
  YMLOW;
  YPReadDir;
  YMReadDir; 
  
  XPWriteDir;
  XMWriteDir;
  XPHIGH;
  XMLOW;
  
  //write digital inputs off in used ADC pins to save power!
  DIDR0=0B00000110; //remember to write back to zeros when done!!!!
  //ADMUX ADC1=YP=PC1=0001 , ADC2=XM=PC2=0010
	_delay_us(1);
	ADMUX=0b00000001; 		//YP
	_delay_us(1);
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	
    for (i=0; i<NUMSAMPLES; i++) {
		ADCSRA |= (1 << ADSC);	 // start an ADC conversion
		while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		samples[i] = ADC;
    }
#if NUMSAMPLES > 2
	insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
	// Allow small amount of measurement noise, because capacitive
	// coupling to a TFT display's signals can induce some noise.
	if (samples[0] - samples[1] < -4 || samples[0] - samples[1] > 4) {
		valid = 0;
	} else {
		samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples 
	}
#endif
	
	*x_point =1023-samples[NUMSAMPLES/2];
	

 //  pinMode(_xp, INPUT);
 //  pinMode(_xm, INPUT);
 //  digitalWrite(_xp, LOW);
 //  pinMode(_yp, OUTPUT);
 //  digitalWrite(_yp, HIGH);
 //  pinMode(_ym, OUTPUT);
  
	XPLOW;
	XPReadDir;
	XMReadDir;
	YPWriteDir;
	YPHIGH;
	YMWriteDir;
	
	_delay_us(1);
	ADMUX=0b00000010; 		//XM	
	_delay_us(1);	
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	
    for (i=0; i<NUMSAMPLES; i++) {
		ADCSRA |= (1 << ADSC);	 // start an ADC conversion
		while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
		samples[i] = ADC;
    }

#if NUMSAMPLES > 2
	insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
	// Allow small amount of measurement noise, because capacitive
	// coupling to a TFT display's signals can induce some noise.
	if (samples[0] - samples[1] < -4 || samples[0] - samples[1] > 4) {
		valid = 0;
	} else {
		samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
	}
#endif

   *y_point =1023-samples[NUMSAMPLES/2];
  // // Set X+ to ground
   //pinMode(_xp, OUTPUT);
  // digitalWrite(_xp, LOW);
   // Set Y- to VCC
  // digitalWrite(_ym, HIGH); 
   // Hi-Z X- and Y+   
  // digitalWrite(_yp, LOW);
  // pinMode(_yp, INPUT);
   
	XPWriteDir;
	XPLOW;	
	YMHIGH;
	YPLOW;
	YPReadDir;	
	_delay_us(1);
	ADMUX=0b00000010; 		//XM	  
	_delay_us(1);
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	int z1=ADC;
	_delay_us(1);
	ADMUX=0b00000001; 		//YP
	_delay_us(1);	
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	int z2 =ADC;

   
     // now read the x 
     double rtouch;
     rtouch = z2;
     rtouch /= z1;
     rtouch -= 1;
     rtouch *= (*x_point);
     rtouch *= _rxplate;
     rtouch /= 1024;

     *z_point = rtouch;
   

   if (! valid) {
     z_point = 0;
   }
//at the end of function revert pins back to original states
	DIDR0=0B00000000;
	XPHIGH;
	XMHIGH;
	YPHIGH;
	YMHIGH;
	XPWriteDir;
	XMWriteDir;
	YPWriteDir;
	YMWriteDir;
	// _delay_us(1);
	return;
}

/*
void TSPoint(int16_t x0, int16_t y0, int16_t z0) {
  x = x0;
  y = y0;
  z = z0;
}

bool TSPoint::operator==(TSPoint p1) {
  return  ((p1.x == x) && (p1.y == y) && (p1.z == z));
}

bool TSPoint::operator!=(TSPoint p1) {
  return  ((p1.x != x) || (p1.y != y) || (p1.z != z));
}

*/

int readTouchX(void) {
/*
   pinMode(_yp, INPUT);
   pinMode(_ym, INPUT);
   digitalWrite(_yp, LOW);
   digitalWrite(_ym, LOW);
   
   pinMode(_xp, OUTPUT);
   digitalWrite(_xp, HIGH);
   pinMode(_xm, OUTPUT);
   digitalWrite(_xm, LOW);
*/   

	// _delay_us(1);
	YPReadDir;
	YMReadDir;
	YPLOW;  
	YMLOW;  
   
	XPWriteDir;
	XPHIGH;
	XMWriteDir;	
	XMLOW;	
	
	_delay_us(1);
    ADMUX=0b00000001; 		//YP
	_delay_us(1);	
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	return (1023-ADC);
}

/*
int TouchScreen::readTouchY(void) {
   pinMode(_xp, INPUT);
   pinMode(_xm, INPUT);
   digitalWrite(_xp, LOW);
   digitalWrite(_xm, LOW);
   
   pinMode(_yp, OUTPUT);
   digitalWrite(_yp, HIGH);
   pinMode(_ym, OUTPUT);
   digitalWrite(_ym, LOW);
   
   return (1023-analogRead(_xm));
}
*/

uint16_t pressure(void) {
  /*
  // Set X+ to ground
  pinMode(_xp, OUTPUT);
  digitalWrite(_xp, LOW);
  
  // Set Y- to VCC
  pinMode(_ym, OUTPUT);
  digitalWrite(_ym, HIGH); 
  
  // Hi-Z X- and Y+
  digitalWrite(_xm, LOW);
  pinMode(_xm, INPUT);
  digitalWrite(_yp, LOW);
  pinMode(_yp, INPUT);
  */
  //_delay_us(1);
	XPWriteDir;
	XPLOW;
	YMWriteDir;	  
	YMHIGH;
	
	XMLOW;
	XMReadDir;
	YPLOW;
	YPReadDir;
	
	 //write digital inputs off in used ADC pins to save power!
	DIDR0=0B00000110; //remember to write back to zeros when done!!!!
	
	_delay_us(1);
    ADMUX=0b00000010; 		//XM	  
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	int z1= ADC;
	_delay_us(1);
	ADMUX=0b00000001; 		//YP
	_delay_us(1);	
	//ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	//while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	ADCSRA |= (1 << ADSC);	 // start an ADC conversion
	while(ADCSRA & _BV(ADSC));    // wait until conversion is complete
	int z2= ADC;
  
  if (_rxplate != 0) {
    // now read the x 
    double rtouch;
    rtouch = z2;
    rtouch /= z1;
    rtouch -= 1;
    rtouch *= readTouchX();
    rtouch *= _rxplate;
    rtouch /= 1024;
    //at the end of function revert pins back to original states
	DIDR0=0B00000000;
	XPHIGH;
	XMHIGH;
	YPHIGH;
	YMHIGH;
	XPWriteDir;
	XMWriteDir;
	YPWriteDir;
	YMWriteDir;
	 //_delay_us(1);
    return rtouch;
  } else {
	//at the end of function revert pins back to original states
	DIDR0=0B00000000;
	XPHIGH;
	XMHIGH;
	YPHIGH;
	YMHIGH;
	XPWriteDir;
	XMWriteDir;
	YPWriteDir;
	YMWriteDir;
	// _delay_us(1);
    return (1023-(z2-z1));
  }
}

long map(long x, long in_min, long in_max, long out_min, long out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

