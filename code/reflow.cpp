/* S6D0154 LCD driver
 * reflow.cpp
 *	
 *
 * Created: 2.2.2016 15:42:52
 *  Author: Panu Leinonen
 */ 

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>




//void screen_setup(void);
//void system_setup(void);

#define TRUE  1
#define FALSE 0

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF



#define TEMP_SCALE CYAN

#define color_ref BLUE	//profile color
#define color_real RED  //real temp graph color

#define BOXSIZE 40
#define PENRADIUS 10

#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define background 0xFFFF

//touchscreen AD values min-max
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// Uno dig. pin :   7   6   5   4   3   2   9   8
// Uno port/pin :  PD6  PD5  PD4  PD3  PD2  PD1  PB0  PD7

//PID values
#define memory 10
#define	P_val 2
#define I_val 0.8
#define D_val 0.5


/************************************************************************************************************************************************/
/* Global Objects                                                       																		*/
/************************************************************************************************************************************************/

//pid global values
double PID=0;
double D_err=0;
double err=0;
double err_old=0;
double I_err=0;
double P_err=0;
int16_t duty11=0;
double mean_I_error=0;
double mean_I_err[memory];

double error_rate=1;

//char xxyx2[10];
//char xxyx3[10];
//char xxyx4[10];

volatile int16_t duty21=0;

volatile uint32_t	timer_1s=0,
					timer_press=0;		//these are test counters
					
			
uint16_t			PreHeatTemp1=0,
					
					SoakHeatTemp1=0,
					PeakHeatTemp1=0,
					
					timerbuzz=0,
					cancel_timer=0;			

volatile uint8_t	fufu=0,
					resample2=0,
					buzzer=0,
					pressed2=0,
					gahbuzz=0,
					press_now=0,
					earlier_press=1;

					
volatile uint16_t 
					timer_seconds=0;					
					
uint16_t timer_help_seconds=0;					
volatile int16_t	duty2=0,
					duty3=0;
			
uint8_t	
					
					
					pressed=0,					
					
					timer_refresh=0;
	
									
//char celcius[2]={'°','c'};	
					
 bool				
					cancel=0;
					

bool			cmd_pulse=0,
				resample=0;		
					
char cycle112[6]="duty:",
		percentmark[2]="%";	
		
char seconds[10];
char temp_current[10];
volatile double temper=0;		
		
//uint16_t last_tempsval[memory]; //make a array of earlier temps and fill it with current temp						
					
/************************************************************************************************************************************************/
/* Global Objects end here                                                      																*/
/************************************************************************************************************************************************/

//own libraries
#include "S6D0154.c" // Hardware-specific library
#include "TouchScreen.c"
#include "MAX31855.c"
#include "profiles_initial.c"
#include "state_machine.c"

#define	buzzport REGISTER_BIT(PORTC,5) 
#define	buzzeron buzzport = 1
#define	buzzeroff buzzport = 0 

void screen_setup() {
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 318.7 ohms across the X plate

TouchScreen(319);

init_S6D0154();

return;
}


void system_setup() {
	
	//clock_prescale_set(clock_div_1); //disable clk prescaler, or fuse to 1
	

	// select minimal prescaler (max system speed)
	CLKPR = 0x80;
	CLKPR = 0x00;
	
	//pc0= LCD_RD(out), pc1 =  LCD_WR (out),
	//pc2 = LCD_RS (out), pc3=LCD_CS(out), pc4=LCD_RST(out), pc5=speaker(out)
	//PC1 =YP touchscreen(i/o) ADC1, PC2 = XM touchscreen(i/o) ADC2 
	DDRC = 0b00111111; 
	PORTC = 0b00000000; //output 0 and input tri-state

	//pd0=rtemp select(out), pd1-7=display digital outputs(out)[B2][B3][B4][B5][B6][B7][B0]
	//PD5=XP(i/o), PD6=YM(i/o)
	DDRD = 0b11111111;
	PORTD = 0b00000000; //output 0 and input tri-state

	//pb0=display digital output(out)[B1], pb1 = PWM(out),pb2=SS(out), pb3=MOSI(out), pb4=MISO(in), pb5=sck(out) 
	DDRB = 0b00101111; //DDRB = 0b00101111; 
	PORTB = 0b00000000; //output 0 and input tri-state
	
	//spi enabled, clk/16 mode, master
	SPCR=0b01010010; //0b01010001; //now fixed with rising edge sampling

	//PWM phase corrected, 16-bit counter
	//output is 12MHz/(256*101*2)=4.3ms pwm, since phase corrected(goes up and then down)
	TCCR1A = 0b10000010; // ICR1 selected as top value mode 1010
	TCCR1B = 0b00010100; //clk/256 prescaling selected 
	ICR1=100; //max
	OCR1A=0; //start value
	TIMSK1= 0b00000000;
	TIFR1=0b00000000;

	//timer control 8 bit
	//output is 12MHz/(256*125)=2.6667ms tick 1s with 375 rounds
	TCNT0 = 0x00; //set timer to 0
	OCR0A = 124; //runs to value 0:124
	TCCR0A = 0b00000010; //CTC operation mode
	TCCR0B = 0b10000100; //force compare match A, prescaler to 256
	TIMSK0= 0b00000010; //masked compare match A
	
	//ADMUX ADC1=YP=PC1=0001 , ADC2=XM=PC2=0010 
	ADMUX=0b00000110; //now in unused ADC6 so it won't affect running the code in anyway at beginning!
	//autotriggering disabled starts conversion right away, since 1st ADC-value is usually bad
	//ADCSRA=0b11000000;
	ADCSRA=0b11000110; //with 64 clk division from 12MHz now 187.5khz, should be between 50-200khz!!
	
	//disable watchdog
	wdt_disable();
	
	
	return;
}



uint16_t	x_point=0, y_point=0;
double    	z_point=0;

ISR (TIMER0_COMPA_vect)  //2.6667ms tick 1s with 375 rounds for 1sec
{
		timer_1s++;		
		if(timer_1s >= 375)		//1s
		{
			delay_ms(10);  //if some delay not added here it will try to read touchscreen while still writing to display
			timer_help_seconds++;
			timer_seconds++;
			timer_1s=0;
			cmd_pulse=1; //1 second has passed time to refresh display
			
			intToStr(timer_seconds,seconds,5);
			drawString( 0,0,seconds, WHITE, BLUE, 2);
			
			duty21=OCR1A;
			//OCR1A=0;
			//delay_ms(131);
			temper=MAX31855_readCelsius();
			//delay_ms(5);
			//OCR1A=duty21;
			temper+=duty21/20; //add 5% of duty cycle to value to get it about right
			ftoa( temper,temp_current, 1);
			drawString(10,20,temp_current, BLACK, WHITE, 2);
			resample=1; //time to draw pixel to display
			resample2=1;
			delay_ms(10);  //some delay to get conversion before going back to function to read it
		}
/*		
	timer_refresh++;
	//draw new pixel twice a second and set new pwm cycle
	if(timer_refresh >= 188){
		
		resample=1;
		timer_refresh=0;
	}
	*/
	
timer_press++;
	if(timer_press >= 38) {//with every 100ms check if has been pressed for cancel
		 //delay so probably nothing to send when it is reading time
		 //if some delay not added here it will try to read touchscreen while still writing to display
		delay_ms(10);
		pressed2=0;
		pressed2=pressure();
		delay_ms(10);
		timer_press=0;
		press_now = 0;
		
		if (pressed2 > MINPRESSURE && pressed2 < MAXPRESSURE ) {
			getPoint( &x_point, &y_point, &z_point); //now points house the values
		
			x_point = map(x_point, TS_MINX, TS_MAXX, 240, 0);
			y_point = map(y_point, TS_MINY, TS_MAXY, 320, 0);
			y_point =320-y_point;
		
			//press at the cancel button position
			if (x_point < 230 && x_point > 170 && y_point > 170 && y_point < 230){
				if (((y_point-PENRADIUS) > BOXSIZE) && ((y_point+PENRADIUS) < 320)) {
					fillCircle( y_point,x_point, PENRADIUS, YELLOW); }	
				press_now = 1;
				if (earlier_press == 0) //when pressed again
				{
					fufu=1;
					cancel =1; //zero cancel in function before return
					fillScreen(WHITE);
					delay_ms(10);
					earlier_press=1;
				}
			
			} 
		}
		if (earlier_press == 1){
			cancel_timer++;
			//first if it has still been pressed
			if (press_now == 1){
				cancel_timer=0;
			}
			//if not then zero the timer after 3sec and cancel can be pressed again
			if (cancel_timer >= 3){
				cancel_timer=0;
				earlier_press =0;
			}
		}

		//setWriteDirInline();
	}
	
	
	timerbuzz++;
	if(buzzer ==1 && timerbuzz > 5){
		gahbuzz++;
		timerbuzz=0;
		buzzeron;
		delay_ms(1); //should be 0.28ms to be most effective
		buzzeroff;
		
	}
	if(timerbuzz > 1000){
		timerbuzz=0;
	}
	//stop buzzer after a while
	if(gahbuzz > 100){
		buzzer =0;
		gahbuzz=0;
	}

}


int main(void)
{
//writefirsttime();
	
	delay_ms(1000);

	system_setup();
	screen_setup();	//screen and touchscreen setup
	MAX31855_init(); //temperature chip
cli(); //disable interrupts //was at beginning of system setup, start PWM can be glitchy if not next to each other
sei(); //enable interrupts
//for hardware spi to work, have to redo these here
DDRB = 0b00101111;
PORTB = 0b00000000; //output 0 and input tri-state
//spi enabled, clk/16 mode, master
SPCR=0b01010010; //0b01010001; //now fixed with rising edge sampling

fillScreen(WHITE);
delay_ms(1000);
//int pressed=0;
char temperature_now[4]={0,0,0,0};

//char xx[10];
//char xxx[10];
//uint16_t gah=0;
//uint16_t gah1=0;
fillScreen(WHITE);
delay_ms(500);
   
	//drawLine(0, 240,100,50,BLACK);
	
	
	READ_EEPROM();
	//DrawTempDisplayRef();
	
	/*gah = eeprom_read_word(&PROFILE[0][5]);
//testing eeprom	
	intToStr(gah,xx,5);
	drawString( 100,120,xx, BLACK, BLUE, 2);
	
	gah1 = eeprom_read_word(&ACTIVE_PROFILE);
	//testing eeprom
	intToStr(gah1,xxx,5);
	drawString( 120,140,xxx, BLACK, BLUE, 2);
	*/
		
	while(1)
    {
		
		//if(cmd_pulse == 1) {			
		//cmd_pulse =0;				
		//}
		
		
		timer_help_seconds=0;
		//fillScreen(WHITE);
		//delay_ms(100);
		DrawProfilestart(); //startup screen
		
		pressed=0;
		pressed=pressure();
		if (pressed > MINPRESSURE && pressed < MAXPRESSURE ) { //it has been pressed or pressure is something measurable
			getPoint( &x_point, &y_point, &z_point); //now points house the values
			
			x_point = map(x_point, TS_MINX, TS_MAXX, 240, 0);
			y_point = map(y_point, TS_MINY, TS_MAXY, 320, 0);
			y_point =320-y_point;
			
			if (((y_point-PENRADIUS) > BOXSIZE) && ((y_point+PENRADIUS) < 320)) {
				fillCircle( y_point,x_point, PENRADIUS, RED);
			}
			//intToStr(x_point, xx,4);
			//intToStr(y_point, xxx,4);
			//drawString( 120,120,xxx, BLACK, WHITE, 3);
			//drawString( 100,100,xx, BLACK, WHITE, 3);
			delay_ms(500);
			
			//start active profile 
			if(y_point <  150 && y_point > 90 && x_point < 130 && x_point > 70){
				//cancel =0; //stop cancel
				fillScreen(WHITE);
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(500);
				DrawTempDisplayRef();
				DrawCancelBut();
				delay_ms(100);
				DrawTempDisplayRef();
				DrawCancelBut();
				delay_ms(100);
				timer_help_seconds=0;
				heat_up(prof.PreHeatTemp, prof.PreHeatTime, timer_help_seconds, cancel, resample);
				timer_help_seconds=0;
				//DrawCancelBut();
				soak_up(prof.SoakHeatTemp, prof.SoakHeatTime, timer_help_seconds, cancel, resample);
				timer_help_seconds=0;
				//DrawCancelBut();
				ramp_up(prof.PeakHeatTemp,prof.PeakHeatTime,timer_help_seconds, cancel, resample);
				//cancel =0; //stop cancel at the end
				OCR1A=0;		
				//do a loop here where temp is checked and dot drawn every second till some time to show cooldown
				timer_help_seconds=0;
				int16_t draw_temp=0;
				int16_t draw_time=0;
				double tmp=0;
				double req_time=(double)320/(prof.PreHeatTime+prof.SoakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+30);
				double req_temp=(double)220/(prof.PeakHeatTemp+35); //that's what we should need from display 220 pixels for temp
				//DrawCancelBut();
				
				char xx[9]="Cooldown";
				char duty1[10];
				drawString( 200,0,xx, WHITE, GREEN, 2);
					buzzer=1;
					while(cancel==0){
						
						if(fufu==1){
							OCR1A=0;
							break;
						}
						
						if(resample2 == 1) {
							//delay_ms(131);
							resample2= 0;
							tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
							//delay_ms(5);
							//should be zero
							//duty3=OCR1A;
							
							draw_temp=round(tmp*req_temp);
							draw_time=round((timer_help_seconds+prof.PreHeatTime+prof.SoakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime)*req_time);
							
							intToStr(duty3, duty1 ,3);
							drawString( 70, 232, cycle112, BLACK, YELLOW, 1); //x and y are inverted
							drawString( 100, 232, duty1, BLACK, YELLOW, 1); //x and y are inverted
							drawString( 118, 232, percentmark, BLACK, YELLOW, 1); //x and y are inverted
							
							if( draw_time < 320){
							drawPixel(draw_time, 240-draw_temp,color_real); //x,y
							drawPixel(draw_time, 240-draw_temp+1,color_real); //x,y
							}
							else if( draw_time >= 320){
							
								drawPixel(draw_time-320, 240-draw_temp,color_real); //x,y
								drawPixel(draw_time-320, 240-draw_temp+1,color_real); //x,y
								drawPixel(draw_time-320, 240-50,color_ref); //x,y
							}
							if( draw_time >= 640){
							
								break;
							}
						
						}
					}
				fufu=0;
				timer_help_seconds=0;
				cancel =0; //stop cancel at the end
				buzzer=0;
				fillScreen(WHITE);
				delay_ms(200);
				fillScreen(WHITE);
				delay_ms(200);
				
			}
			
			
			
			//set to a temp
			if(x_point < 230 && x_point > 170 && y_point < 150 && y_point > 50) { //x and y are now inverted
				cancel =0; //stop cancel
				fillScreen(WHITE);
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(500);
				DrawCancelBut();			
				delay_ms(500);
				//default value
				double temp_target = 42;				
				
				//fill
				for (uint8_t ii=0; ii < memory-1;ii++){
					mean_I_err[ii]=0;
				}				
				//drawLine(0,240-temp_target, 320,240-temp_target ,color_ref );
				//uint16_t timer_help_line=0;
				
				double tmp=0;
				int32_t duty12=0;
				char duty1[10];
				timer_help_seconds=0;
				
				
					while(cancel==0){
						if(fufu==1){
							OCR1A=0;
							break;
						}
						
						if (resample2){
							resample2=0;
							
							//OCR1A=0;
							//delay_ms(131);
							tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
							tmp +=round(duty2/20);					
							//delay_ms(5);
							//OCR1A=duty2;
							ftoa( tmp,temperature_now, 1);
							drawString( 80,110,temperature_now, BLACK, WHITE, 5);
							//OCR1A=duty2;
							//drawString_gfx( 140,120,celcius, BLACK, WHITE, 1);					
							/*
							if(timer_help_line< 320) {
								timer_help_line++;
								//drawPixel(timer_help_line, 240-tmp,color_real); //x,y
								//drawPixel(timer_help_line, 240-tmp+1,color_real); //x,y
							} 
							else if (timer_help_line>=320){
								timer_help_line=0;
								fillScreen(WHITE);
								delay_ms(100);								
							}
							*/
							err_old=err;
							err=temp_target-tmp;
							P_err=err;
							
							mean_I_err[memory-1]=err_old;
							for (int i=0;i< memory-1;i++){
								mean_I_err[i]=(double)mean_I_err[i+1];
							}
							mean_I_error=0;
							for (int i=0;i< memory;i++){
								mean_I_error+=(double)mean_I_err[i];
							}
							I_err=mean_I_error;
							//I_err+=err_old
							D_err=err-err_old;
							
							if(err > 1 || err < -1){
								PID=P_val*P_err + I_val*I_err + D_val*D_err;
								duty12=round(PID);
							}
							if (duty12 > 0){
								duty2=duty12;
							} 
							if ( duty12 >= ICR1 ) {
								duty2 = ICR1;
							}
							
							if ( duty12 <= 0){
								duty2 = 0;
							}
								
												
							OCR1A=duty2;
							intToStr(duty2, duty1 ,3);
							drawString( 70, 232, cycle112, BLACK, YELLOW, 1); //x and y are inverted
							drawString( 100, 232, duty1, BLACK, YELLOW, 1); //x and y are inverted
							drawString( 118, 232, percentmark, BLACK, YELLOW, 1); //x and y are inverted
						}
					
					}
				OCR1A=0;
				fufu=0;
				cancel =0; //stop cancel at the end
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(100);
				fillScreen(WHITE);
				delay_ms(500);
			}
		
			delay_ms(100);
		}
		
		
		
		
			/*	
		pressed=0;
		pressed=pressure();
		if (pressed > MINPRESSURE && pressed < MAXPRESSURE ) { //it has been pressed or pressure is something measurable
			getPoint( &x_point, &y_point, &z_point); //now points house the values
			
			x_point = map(x_point, TS_MINX, TS_MAXX, 240, 0);
			y_point = map(y_point, TS_MINY, TS_MAXY, 320, 0);
			y_point =320-y_point;
			
			if (((y_point-PENRADIUS) > BOXSIZE) && ((y_point+PENRADIUS) < 320)) {
			fillCircle( y_point,x_point, PENRADIUS, RED);
			}
			delay_ms(1500);
			//6 states to do the reflow process
			//when we have selected right profile and running it
			timer_help_seconds=0;
			DrawTempDisplayRef();
			DrawCancelBut();
			heat_up(prof.PreHeatTemp, prof.PreHeatTime, timer_help_seconds, cancel, resample);
			timer_help_seconds=0;
			soak_up(prof.SoakHeatTemp, prof.SoakHeatTime, timer_help_seconds, cancel, resample);
			timer_help_seconds=0;
			ramp_up(prof.PeakHeatTemp,prof.PeakHeatTime,timer_help_seconds, cancel, resample);
			cancel =0; //stop cancel at the end
			OCR1A=0;
			fillScreen(WHITE);
			DrawCancelBut();
		}
		*/
		delay_ms(1000);
		
		
    }
}


