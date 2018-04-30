#include "state_machine.h"

enum { RampToSoak, Soak, RampUp, Peak};
volatile uint16_t activated_profile =0;
volatile int reflow_process = 0;

//temperature profile struct
typedef struct profileValues_s {
	uint16_t PreHeatTemp;
	uint16_t PreHeatTime;
	uint16_t SoakHeatTemp;
	uint16_t SoakHeatTime;
	uint16_t PeakHeatTemp;
	uint16_t PeakHeatTime;
} Profile_t;

Profile_t prof;

//read it at startup to see the first active profile in the main page
void READ_EEPROM_ACTIVE_PROFILE(){
activated_profile = eeprom_read_word(&ACTIVE_PROFILE);
return;
}

//this reads eeprom from active profile
void READ_EEPROM(){
activated_profile = eeprom_read_word(&ACTIVE_PROFILE);
prof.PreHeatTemp = eeprom_read_word(&PROFILE[activated_profile][0]);
prof.PreHeatTime = eeprom_read_word(&PROFILE[activated_profile][1]);
prof.SoakHeatTemp = eeprom_read_word(&PROFILE[activated_profile][2]);
prof.SoakHeatTime = eeprom_read_word(&PROFILE[activated_profile][3]);
prof.PeakHeatTemp = eeprom_read_word(&PROFILE[activated_profile][4]);
prof.PeakHeatTime = eeprom_read_word(&PROFILE[activated_profile][5]);
return;
}

//slow heat up 
void heat_up(uint16_t& temp,uint16_t& time, uint16_t& timers_help,bool& cancel,bool& res ){

	double slope_yx =0;
	uint16_t tmp_time=0;
	int16_t duty=ICR1;
	double tmp=0;
	//double mean_tmp=0;
	//uint16_t right_temp_at_this_second =0;
	
	int16_t draw_temp=0;
	int16_t draw_time=0;
	
	char duty1[10];
	char xx[8]="Heating";
	drawString( 200,0,xx, WHITE, GREEN, 2);	
	
	//OCR1A=0;
	//delay_ms(131);
	tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
	if (tmp > 1000 || tmp < 0){
		tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
	}
	tmp+=duty/20; //add 5% of duty cycle to value to get it about right
	//delay_ms(5);
	
	//fill
	for (uint8_t iii=0; iii < memory-1;iii++){
		mean_I_err[iii]=0;
	}

	
	//char xxyx[10];
	//char xxyx1[10];
	//char xxyx2[10];
	//char xxyx3[10];
	//char xxyx4[10];
		
	//for (int i=0;i< memory;i++){
	//	last_tempsval[i]=(uint16_t)round(tmp);
	//}
	uint16_t tmpstart=(uint16_t)round(tmp);
	//uint16_t last_temps[4]={(uint16_t)round(tmp),(uint16_t)round(tmp),(uint16_t)round(tmp),(uint16_t)round(tmp)}; //fill with current temp
	slope_yx = (double) (temp-tmpstart)/time; //this is the degrees / second we want to achieve
	//cancel =0;
	
	//profile times and added 30seconds for cooling whole 320 pixels for time
	double req_time=(double)320/(prof.PreHeatTime+prof.SoakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+30); 
	double req_temp=(double)220/(prof.PeakHeatTemp+35); //that's what we should need from display 220 pixels for temp
	
	while(timers_help < time){
	
		if( cancel == 1){
		OCR1A=0;			
		//drawString( 100, 30, "yes", BLACK, BLUE, 2); //x and y are inverted
		return;
		}
		//PWM is in 16-bit counter, max value is 100, OCR1A houses the uint16_t duty cycle value
		if (res){
			res =0;
			tmp_time++;

			//OCR1A=0;
			//delay_ms(131);			
			
			//delay_ms(5);
			//OCR1A=duty;
			tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			if (tmp > 1000 || tmp < 0){
				tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			}
			tmp+=duty/20; //add 5% of duty cycle to value to get it about right
			
			draw_temp=round(tmp*req_temp);	
			draw_time=round(timers_help*req_time);
			drawPixel(draw_time, 240-draw_temp,color_real); //x,y
			drawPixel(draw_time, 240-draw_temp+1,color_real); //x,y
			
			
			if(timers_help >= 1 ) {			
				
				err_old=err;
				err=slope_yx*timers_help+tmpstart-tmp;
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
				
				if(err > error_rate || err < -error_rate){
					PID=P_val*P_err + I_val*I_err + D_val*D_err;
					duty11=round(PID);	
				}
				//ftoa(err,xxyx1,5);
				//drawString( 100,60,xxyx1, BLACK, WHITE, 2);
				//ftoa(PID,xxyx,5);
				//drawString( 100,80,xxyx, BLACK, WHITE, 2);
				
				//ftoa(P_err,xxyx2,5);
				//drawString( 100,20,xxyx2, BLACK, WHITE, 2);
				//ftoa(I_err,xxyx3,5);
				//drawString( 100,40,xxyx3, BLACK, WHITE, 2);
				//ftoa(D_err,xxyx4,5);
				//drawString( 100,60,xxyx4, BLACK, WHITE, 2);
				
				if (duty11 > 0){
					duty=duty11;
				} 
				else if (duty11 < 0){
					duty=0;
				} 				
				if ( duty > ICR1 ){
					duty = ICR1;
				}		
				OCR1A=duty;
				
				intToStr(duty, duty1 ,3);				
				drawString( 70, 232, cycle112, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 100, 232, duty1, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 118, 232, percentmark, BLACK, YELLOW, 1); //x and y are inverted

					
			}
		}
	}
return;
}

void soak_up(uint16_t& temp,uint16_t& time, uint16_t& timers_help,bool& cancel,bool& res ){
	
double slope_yx =0;
	uint16_t tmp_time=0;
	int16_t duty=OCR1A;
	double tmp=0;
	//double mean_tmp=0;
	//uint16_t right_temp_at_this_second =0;
	
	int16_t draw_temp=0;
	int16_t draw_time=0;
			
	//char xx[10];
	char duty1[10];
	char xx[8]="Soaking";
	drawString( 200,0,xx, WHITE, GREEN, 2);
	
	//OCR1A=0;
	//delay_ms(131);
	tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
	if (tmp > 1000 || tmp < 0){
		tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
	}
	tmp+=duty/20; //add 5% of duty cycle to value to get it about right
	//delay_ms(5);
	//OCR1A=duty;

	//tmp+=round(duty/12);
	//uint16_t last_temps[4]={(uint16_t)round(tmp),(uint16_t)round(tmp),(uint16_t)round(tmp),(uint16_t)round(tmp)}; //fill with current temp
	slope_yx =(double) (temp-prof.PreHeatTemp)/time; //this is the degrees / second we want to achieve
	//cancel =0;
	
	//profile times and added 30seconds for cooling whole 320 pixels for time
	double req_time=(double)320/(prof.PreHeatTime+prof.SoakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+30); 
	double req_temp=(double)220/(prof.PeakHeatTemp+35); //that's what we should need from display 220 pixels for temp
	
	while(timers_help < time){
	
		if( cancel == 1){
		OCR1A=0;			
		//drawString( 100, 30, "yes", BLACK, BLUE, 2); //x and y are inverted
		return;
		}
		
		//PWM is in 16-bit counter, max value is 100, OCR1A houses the uint16_t duty cycle value
		if (res){
			res =0;
			tmp_time++;
			//OCR1A=0;
			//delay_ms(131);
			tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			if (tmp > 1000 || tmp < 0){
				tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			}
			tmp+=duty/20; //add 5% of duty cycle to value to get it about right
			//delay_ms(5);
			//OCR1A=duty;
			//tmp+=duty/20; //add 10% of duty cycle to value to get it about right
			draw_temp=round(tmp*req_temp);	
			draw_time=round((timers_help+prof.PreHeatTime)*req_time);
			drawPixel(draw_time, 240-draw_temp,color_real); //x,y
			drawPixel(draw_time, 240-draw_temp+1,color_real); //x,y
					
			if (timers_help >= 1 ) {
				
				err_old=err;
				err=slope_yx*timers_help+prof.PreHeatTemp-tmp;
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
				
				if(err > error_rate || err < -error_rate){
					PID=P_val*P_err + I_val*I_err + D_val*D_err;
					duty11=round(PID);	
				}
				//ftoa(err,xxyx1,5);
				//drawString( 100,60,xxyx1, BLACK, WHITE, 2);
				//ftoa(PID,xxyx,5);
				//drawString( 100,80,xxyx, BLACK, WHITE, 2);
				
				//ftoa(P_err,xxyx2,5);
				//drawString( 100,20,xxyx2, BLACK, WHITE, 2);
				//ftoa(I_err,xxyx3,5);
				//drawString( 100,40,xxyx3, BLACK, WHITE, 2);
				//ftoa(D_err,xxyx4,5);
				//drawString( 100,60,xxyx4, BLACK, WHITE, 2);
				
				if (duty11 > 0){
					duty=duty11;
				} 
				else if (duty11 < 0){
					duty=0;
				} 				
				if ( duty > ICR1 ){
					duty = ICR1;
				}		
				OCR1A=duty;
				//start early to max ramp temp
				//if(timers_help+5 >= time && mean_tmp < right_temp_at_this_second + 10 ){
				//	duty=ICR1;
				//}
				
				
				intToStr(duty, duty1 ,3);
				drawString( 70, 232, cycle112, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 100, 232, duty1, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 118, 232, percentmark, BLACK, YELLOW, 1); //x and y are inverted
			}
		}
	}
return;
}


void ramp_up(uint16_t& temp,uint16_t& time, uint16_t& timers_help,bool& cancel,bool& res ){
	OCR1A=ICR1; //max cycle
	
	//uint16_t temp_target= temp;//+5; //target is 5 Celsius above peak
	double tmp=0;
	int16_t duty=ICR1;	
	double slope_yx =0;
	//double mean_tmp=0;
	int16_t draw_temp=0;
	int16_t draw_time=0;
	
	//uint16_t right_temp_at_this_second =0;
	
	char duty1[10];
	char xx[8]="Ramping";
	drawString( 200,0,xx, WHITE, GREEN, 2);
	
	//OCR1A=0;
	//delay_ms(131);
	tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
	if (tmp > 1000 || tmp < 0){
		tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
	}
	tmp+=duty/20; //add 5% of duty cycle to value to get it about right
	//delay_ms(5);
	//OCR1A=duty;

	//tmp+=round(duty/12);
	//uint16_t last_temps[4]={(uint16_t)round(tmp),(uint16_t)round(tmp),(uint16_t)round(tmp),(uint16_t)round(tmp)}; //fill with current temp
	slope_yx =(double) (temp-prof.SoakHeatTemp)/time; //this is the degrees / second we want to achieve

	//profile times and added 30seconds for cooling whole 320 pixels for time
	double req_time=(double)320/(prof.PreHeatTime+prof.SoakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+30); 
	double req_temp=(double)220/(prof.PeakHeatTemp+35); //that's what we should need from display 220 pixels for temp
	
	while(timers_help < time){
	
		if( cancel == 1){
			OCR1A=0;
			return;
		}
		if (res){
			res =0;
			//OCR1A=0;
			//delay_ms(131);
			tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			if (tmp > 1000 || tmp < 0){
				tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			}
			tmp+=duty/20; //add 5% of duty cycle to value to get it about right
			//delay_ms(5);
			//OCR1A=duty;
			//tmp+=duty/12; //add x% of duty cycle to value to get it about right
			
						
			draw_temp=round(tmp*req_temp);	
			draw_time=round((timers_help+prof.PreHeatTime+prof.SoakHeatTime)*req_time);
			drawPixel(draw_time, 240-draw_temp,color_real); //x,y
			drawPixel(draw_time, 240-draw_temp+1,color_real); //x,y			
			
				err_old=err;
				err=slope_yx*timers_help+prof.SoakHeatTemp-tmp;
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
				
				if(err > error_rate || err < -error_rate){
					PID=P_val*P_err + I_val*I_err + D_val*D_err;
					duty11=round(PID);	
				}
				//ftoa(err,xxyx1,5);
				//drawString( 100,60,xxyx1, BLACK, WHITE, 2);
				//ftoa(PID,xxyx,5);
				//drawString( 100,80,xxyx, BLACK, WHITE, 2);
				
				//ftoa(P_err,xxyx2,5);
				//drawString( 100,20,xxyx2, BLACK, WHITE, 2);
				//ftoa(I_err,xxyx3,5);
				//drawString( 100,40,xxyx3, BLACK, WHITE, 2);
				//ftoa(D_err,xxyx4,5);
				//drawString( 100,60,xxyx4, BLACK, WHITE, 2);
				
				if (duty11 > 0){
				duty=duty11;
				} 
				else if (duty11 < 0){
					duty=0;
				} 				
				if ( duty > ICR1 ){
					duty = ICR1;
				}		
				OCR1A=duty;
			
			
				intToStr(duty, duty1 ,3);
				drawString( 70, 232, cycle112, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 100, 232, duty1, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 118, 232, percentmark, BLACK, YELLOW, 1); //x and y are inverted
			
		}


				
	}
	uint16_t dooh=timers_help;
	timers_help=0;
	slope_yx =0; //keep temp same
	while(timers_help < time){
	
		if( cancel == 1){
			OCR1A=0;
			return;
		}
		if (res){
			res =0;
			//OCR1A=0;
			//delay_ms(131);
			tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			if (tmp > 1000 || tmp < 0){
				tmp = MAX31855_readCelsius(); //reading in celcius works +0 to 1270 degrees
			}
			tmp+=duty/20; //add 5% of duty cycle to value to get it about right
			//delay_ms(5);
			//OCR1A=duty;
			
			draw_temp=round(tmp*req_temp);	
			draw_time=round((dooh+timers_help+prof.PreHeatTime+prof.SoakHeatTime)*req_time);
			drawPixel(draw_time, 240-draw_temp,color_real); //x,y
			drawPixel(draw_time, 240-draw_temp+1,color_real); //x,y
		
			
				err_old=err;
				err=prof.PeakHeatTemp-tmp;
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
				
				if(err > error_rate || err < -error_rate){
					PID=P_val*P_err + I_val*I_err + D_val*D_err;
					duty11=round(PID);	
				}
				//ftoa(err,xxyx1,5);
				//drawString( 100,60,xxyx1, BLACK, WHITE, 2);
				//ftoa(PID,xxyx,5);
				//drawString( 100,80,xxyx, BLACK, WHITE, 2);
				
				//ftoa(P_err,xxyx2,5);
				//drawString( 100,20,xxyx2, BLACK, WHITE, 2);
				//ftoa(I_err,xxyx3,5);
				//drawString( 100,40,xxyx3, BLACK, WHITE, 2);
				//ftoa(D_err,xxyx4,5);
				//drawString( 100,60,xxyx4, BLACK, WHITE, 2);
				
				if (duty11 > 0){
				duty=duty11;
				} 
				else if (duty11 < 0){
					duty=0;
				} 				
				if ( duty > ICR1 ){
					duty = ICR1;
				}		
				OCR1A=duty;
			
				intToStr(duty, duty1 ,3);
				drawString( 70, 232, cycle112, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 100, 232, duty1, BLACK, YELLOW, 1); //x and y are inverted
				drawString( 118, 232, percentmark, BLACK, YELLOW, 1); //x and y are inverted
		}
			
	}
	OCR1A=0; //cooldown
return;
}


//reference profile pic
void DrawTempDisplayRef(){
	
	//profile times and added 30seconds for cooling whole 320 pixels for time
	double req_time=(double)320/(prof.PreHeatTime+prof.SoakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+prof.PeakHeatTime+30); 
	double req_temp=(double)220/(prof.PeakHeatTemp+35); //that's what we should need from display 220 pixels for temp

//temperature scale here
	char temps[10];
	intToStr(50, temps ,3);
	
	drawString( 2,240-(50*req_temp)+2, temps, TEMP_SCALE, WHITE, 1);
	drawFastHLine(0, 240-(50*req_temp), 319, TEMP_SCALE);
		
	intToStr(100, temps ,3);
	drawString( 2,240-(100*req_temp)+2, temps, TEMP_SCALE,WHITE, 1);
	drawFastHLine(0, 240-(100*req_temp), 319, TEMP_SCALE);
	
	intToStr(150, temps ,3);
	drawString( 2,240-(150*req_temp)+2, temps, TEMP_SCALE, WHITE, 1);
	drawFastHLine(0, 240-(150*req_temp), 319, TEMP_SCALE);
	
	intToStr(200, temps ,3);
	drawString( 2,240-(200*req_temp)+2, temps, TEMP_SCALE, WHITE, 1);
	drawFastHLine(0, 240-(200*req_temp), 319, TEMP_SCALE);
	
	intToStr(250, temps ,3);
	drawString( 2,240-(250*req_temp)+2, temps, TEMP_SCALE, WHITE, 1);
	drawFastHLine(0, 240-(250*req_temp), 319, TEMP_SCALE);
	
	//here starts profile drawing
	uint16_t start_temp = MAX31855_readCelsius();
	
	uint16_t PreHeatTime1= round(prof.PreHeatTime*req_time);
	uint16_t SoakHeatTime1= round(prof.SoakHeatTime*req_time);
	uint16_t PeakHeatTime1= round(prof.PeakHeatTime*req_time);
	
	PreHeatTemp1= round(prof.PreHeatTemp*req_temp);
	SoakHeatTemp1= round(prof.SoakHeatTemp*req_temp);
	PeakHeatTemp1= round(prof.PeakHeatTemp*req_temp);
			
	drawLine( 0,240-(start_temp*req_temp), PreHeatTime1,240-PreHeatTemp1 ,color_ref);
	drawLine( PreHeatTime1,240-PreHeatTemp1 ,SoakHeatTime1+PreHeatTime1,240-SoakHeatTemp1,color_ref);
	drawLine( SoakHeatTime1+PreHeatTime1,240- SoakHeatTemp1,PeakHeatTime1+SoakHeatTime1+PreHeatTime1, 240-PeakHeatTemp1+5,color_ref);
	
	drawLine( PeakHeatTime1+SoakHeatTime1+PreHeatTime1,240-PeakHeatTemp1+5,PeakHeatTime1+PeakHeatTime1+SoakHeatTime1+PreHeatTime1, 240-PeakHeatTemp1+5,color_ref);	
	drawLine( PeakHeatTime1+PeakHeatTime1+SoakHeatTime1+PreHeatTime1,240-PeakHeatTemp1+5,PeakHeatTime1+PeakHeatTime1+SoakHeatTime1+PreHeatTime1+20, 240-(50*req_temp),color_ref); //to temp 50
	
	drawLine(PeakHeatTime1+PeakHeatTime1+SoakHeatTime1+PreHeatTime1+20, 240-(50*req_temp), 319,240-(50*req_temp), color_ref);
	//drawLine( PeakHeatTime1+SoakHeatTime1+PreHeatTime1,240-PeakHeatTemp1,PeakHeatTime1+SoakHeatTime1+PreHeatTime1+20, 240-(50*req_temp),color_ref); //to temp 50
	//drawLine(PeakHeatTime1+SoakHeatTime1+PreHeatTime1+20, 240-(50*req_temp), 319,240-(50*req_temp), color_ref);
	return;	
}

void DrawCancelBut(){
char cca[7]="Cancel";
drawRoundRect(170, 170, 60,60, 5, BLACK);
drawString( 182, 196, cca, BLACK, WHITE, 1);
return;	
}

void DrawProfilestart(){
char active1[16]="Active profile:"; 
char prof[2];
intToStr(activated_profile, prof ,1);
//draw active profile num xx
drawString( 30, 55, active1, BLACK, WHITE, 2);
drawString( 206,55, prof, RED,WHITE, 2);

//draw what to select, start profile, edit profile or set temp to some value
char activate[6]="Start"; 
drawRoundRect(70, 90, 60,60, 5, BLACK);
drawString( 80, 115, activate, BLACK, WHITE, 1);

char edit1[6]="Edit"; 
char edit2[8]="Profile"; 
drawRoundRect(200, 90, 60,60, 5, BLACK);
drawString( 210, 110, edit1, BLACK, WHITE, 1);
drawString( 210, 120, edit2, BLACK, WHITE, 1);

char sometemp[8]="Heat to";
char sometemp2[12]="temperature"; 
drawRoundRect(50, 170, 100,60, 5, BLACK);
drawString( 60, 185,sometemp, BLACK, WHITE, 1);
drawString( 60, 195, sometemp2, BLACK, WHITE, 1);
return;	
}


/*
typedef enum { RampToSoak, Soak, RampUp, Peak, RampDown, CoolDown} state_t;

typedef struct instance_data instance_data_t;
typedef state_t state_func_t( instance_data_t *data );

state_t do_state_initial( instance_data_t *data );
state_t do_state_foo( instance_data_t *data );
state_t do_state_bar( instance_data_t *data );

state_func_t* const state_table[ NUM_STATES ] = {
    do_state_initial, do_state_foo, do_state_bar
};

state_t run_state( state_t cur_state, instance_data_t *data ) {
    return state_table[ cur_state ]( data );
};

int main( void ) {
    state_t cur_state = STATE_INITIAL;
    instance_data_t data;

    while ( 1 ) {
        cur_state = run_state( cur_state, &data );

        // do other program logic, run other state machines, etc
    }
}
*/
