/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 *
 * RDXNIC008 HYSMAT002
 * 12/08/2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;
void initGPIO(void){
	/*
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware

	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC

	//Set up the LEDS
	for(int i = 0; i < sizeof(HOURPINS)/sizeof(HOURPINS[0]); i++){
	    pinMode(HOURPINS[i], OUTPUT);
	}


	 for(int i = 0; i < sizeof(MINPINS)/sizeof(MINPINS[0]); i++){
            pinMode(MINPINS[i], OUTPUT);
        }
	//Set Up the Seconds LED for PWM
	pinMode(SECS, PWM_OUTPUT);
	printf("LEDS done\n");

	//Set up the Buttons
	for(int j = 0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}

	//Attach interrupts to Buttons
	wiringPiISR(BTNS[0], INT_EDGE_FALLING, &hourInc);
	wiringPiISR(BTNS[1], INT_EDGE_FALLING, &minInc);
	printf("BTNS done\n");
	printf("Setup done\n");
}

void initRTC()
{
	// set to 12 hour time
	wiringPiI2CWriteReg8(RTC,HOUR, 0b01000000);
	// start clock
	wiringPiI2CWriteReg8(RTC,SEC, 0b10000000);
}

/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();
	initRTC();
	//Set random time (3:04PM)
	//You can comment this file out later
	wiringPiI2CWriteReg8(RTC, HOUR, (0x13+TIMEZONE) | 0b01000000);
	wiringPiI2CWriteReg8(RTC, MIN, 0x4);
	wiringPiI2CWriteReg8(RTC, SEC, 0x00 | 0b10000000);

	/*
	// Check the LEDs
	delay(2000);
	for(int i = 0; i < sizeof(HOURPINS)/sizeof(HOURPINS[0]); i++){
            digitalWrite(HOURPINS[i], 1);
	    delay(500);
	    digitalWrite(HOURPINS[i], 0);
        }

	for(int i = 0; i < sizeof(MINPINS)/sizeof(MINPINS[0]); i++){
            digitalWrite(MINPINS[i], 1);
            delay(500);
            digitalWrite(MINPINS[i], 0);
        }
	*/

	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		//Write your logic here

		//Function calls to toggle LEDs
		//Write your logic here



	//max value for pwm is 1042
	secs = wiringPiI2CReadReg8(RTC, SEC) & 0b01111111;
	// Print out the time we have stored on our RTC
	printf("The current time is: %x:%x:%x\n", hours, mins, secs);

	//using a delay to make program less cpu hungry
	delay(1000); //milliseconds
    	}
	return 0;
}
/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here
}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
        pwmWrite (SECS, units/60.0*1042) ;     /* provide PWM value for duty cycle */
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic)
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();
	//printf("Hours");
	if (interruptTime - lastInterruptTime>200){
		//Fetch RTC Time
		hours = hFormat((int)wiringPiI2CReadReg8(RTC, HOUR));
		//Increase hours by 1, ensuring not to overflow
		//Write hours back to the RTC

		printf("Hour = %x\n", hours);
	}
	lastInterruptTime = interruptTime;
}

/*
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();
	//printf("Minutes");
	if (interruptTime - lastInterruptTime>200){
		//Fetch RTC Time
		mins = wiringPiI2CReadReg8(RTC, MIN);
		//Increase minutes by 1, ensuring not to overflow
		//Write minutes back to the RTC
		printf("Minutes = %x\n", mins);
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
