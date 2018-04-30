
#include <avr/eeprom.h>
//8 profiles in total that can be used and saved to eeprom

//change to zero when written

//only need to write in there per chip, or if one wants to change the original data

//profile0 goes as follows
//preheat to 150 degrees in 150sec
//soak to 160 for 80sec
//peak to and in 220 for 45sec 


//uint16_t EEMEM PROFILE[8][6];
uint16_t EEMEM ACTIVE_PROFILE=0; 
uint16_t EEMEM PROFILE[8][6]; //2d eeprom array




void writefirsttime(void){
//cli(); //no interrupts during eeprom write
//active profile is so that you can start running that one right away.

//temp, time,temp, time, temp, time
const uint16_t PROFILEs1[8][6] = 
{130,150, 150, 80, 210,40};

/*
{0,0, 0, 0, 0,0},
{0,0, 0, 0, 0,0},
{0,0, 0, 0, 0,0},
{0,0, 0, 0, 0,0},
{0,0, 0, 0, 0,0},
{0,0, 0, 0, 0,0},
{0,0, 0, 0, 0,0} };*/




int justtobesure=0;
//write eeprom at start
		while(1){
			justtobesure++;
			if(eeprom_is_ready()==1) {
				eeprom_write_block((const void*)&PROFILEs1,(void*)&PROFILE[0][0],12); //write 12 bytes( first profile)
				eeprom_write_word(&ACTIVE_PROFILE, 0);
				break;
			}
			if (justtobesure>=100){
				break;
			}
		}

}

//eeprom_update_byte(&PROFILE[0][i], PROFILES_begin[0][i]);

//sei();//enable interrupts






