/*
Revision History
	1. 21 Nov 2011	- Corrected some rotation related stuffs
	2. 06 Dec 2011	- Fixed day count being stuck at the month boundary
					  Day count will now increment only after 24 hours of hatch
	3. 30 Dec 2011  - Date count stuck at Oct/Nov/Dec month boundaries -- fixed
					  Day count increment at the hour at which hatch started
	4. 27 Jan 2012	- For chick, rotation stopped at 12th day -- fixed
					  '00' issue corrected -- fixed
					  Reduce temp by 0.5 degree on the last day of hatch -- fixed
					  reduce humidity at the last 3 days of hatch -- fixed
					  corrected default direction of rotation -- fixed
	5. 27 Feb 2012	- Rotation stopped early since BCD value is compared against decimal -- fixed
	6. 02 Mar 2012	- Corrected day count issue on Relative humity reduction
	7. 21 Apr 2012	- Date count not working
	8. 07 May 2012	- Fixed leap year issue in Date_countup
	9. 06 Jul 2012	- Corrected issues around Date_countup calculation
    10. 19 Jul 2012 - Humidity control outputs not proper -- fixed
	11. 20 Aug 2012 - Total Hatch count not correct for Quail
					  Reducing temp on last days not working for Quail
	12. 04 Jul 2020 - To use with AtmelStudio 7 changed following 
						- Changed implementation of delay() function using _delay_ms() function
						- #ifndef F_CPU added
						- #define __AVR_LIBC_DEPRECATED_ENABLE__ 1 added

*/

//Below code is added for Atmel Studio
#ifndef F_CPU
	 F_CPU 12000000UL
#endif
////////////////
#define __AVR_LIBC_DEPRECATED_ENABLE__ 1 //Added to get around issue in SIG_ADC when moved to Atmel Studio

#include <stdio.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <inttypes.h>
#include <util/delay.h>
#include "abc.h"
#include "user_settings.h"
#include "time_date.h"
#include "date_routines.h"
#include "ADC.h"

unsigned char Year_current=0,Year_trigger,days_to_go=0;
extern unsigned int  adc_value;
float temperature;

int main(void)
{                          
	//configuring all ports 

	DDRA = 0xFC;//0xFF;
	DDRB = 0xEE; //PORTB.4 &PB0 are input
	DDRC = 0xFF;
	DDRD = 0xFB; //PORTD.2 is input	(SWITCH2 and reed switch for to&fro rotation are connected here)
	
	//PORTD = PORTD | 0x04;
	
	
	//PORTA = (PORTA & 0x9F) | 0x20;	//PA5-> 35, PA6-> 36
	//while(1) {
	//;
	//}
	
	//Disable JTAG
	MCUCSR = MCUCSR | 0x80;
	MCUCSR = MCUCSR | 0x80;
	MCUCSR = MCUCSR | 0x80;
	MCUCSR = MCUCSR | 0x80;
	
	/* Global Interrupt Enable -- ADC capture completion signalled through ADC interrupt*/
	SREG = SREG | (0x80);
	
	//F_CPU < 3600000UL
    TWBR = 10;                    /* smallest TWBR value*/
	TWBR = 5;
	
	ab = 0x32; // default value for a date overflow
	
	// Disable all motor controls
 	clear_bit_AIRTNCNTRL();
 	clear_bit_ROTTNCNTRL();
 	clear_bit_MIXIGCNTRL();
	clear_bit_LAMPDRIVER1_B1();
	clear_bit_LAMPDRIVER1_B2();
	clear_bit_rh_max();
	clear_bit_rh_min();
	
	reset_variables();
	lcdinitroutine();
	delay(2000);
	
	/*
	rotate_eggs(30);
	cmdroutine(0x80);
	dataroutine('D');
	dataroutine('O');
	dataroutine('N');
	dataroutine('E');
	*/
	
	
	
//Setting Options
	

	// (1) -- Set todays time and date
	set_date_and_time();
		
	// (2) -- Reset Incubator for new hatch with bird selection
	new_hatch();

	lcdinitroutine();
	
	/*Rotation has to be to and fro. Reed switches gives an interrupt at the max points
	GICR 	= GICR | (0x40); //Enable INT0
	MCUCR	= MCUCR | (0x0); //detect LOW level on INT0
	*/
	PORTB = 0x09;
			
	
//*** in case of uC power supply failure, get the values for Date_trigger and Month_trigger from the RTC RAM

	I2CSendAddr(RTCADDR,WRITE); 
	I2CSendByte(0x0C);
	I2CSendAddr(RTCADDR,READ); // command byte read temp
	//	HR_trigger = I2CGetByte(0); // will have seconds_ but not saved
	Date_trigger 				= I2CGetByte(0x00); // will have date trigger
	Month_trigger 				= I2CGetByte(0x00); // will have month trigger
	Date_countup 				= I2CGetByte(0x00); // will have date countup
	day_count 					= I2CGetByte(0x00);
	bird_type 					= I2CGetByte(0x00);
	Year_trigger 				= I2CGetByte(0x00);
	HR_ref_for_day_count 		= I2CGetByte(0x00);
	Minute_ref_for_day_count 	= I2CGetByte(0x01);
	I2CSendStop(); //send stop
	
	/*
	//If want to check whether temperature sensors are reading fine
	while (1) 
	{
		//acquire_adc_channels(0);
		//acquire_adc_channels(1);
		readtemp(TSENSOR1);
		readtemp(TSENSOR2);
	}
	*/
	
	dm[1] = ((Year_current % 4) == 1) + 28;
	
	
	readrtc();
	HR_trigger = HR_current;
	
	unsigned int lcd_clr_count = 0;

	
	while(1)
	{
	lcd_clr_count ++;
	
	if(lcd_clr_count == 25)
	{
	    lcdinitroutine();
		lcd_clr_count = 0;
	}
		
	incubator_loop();
	delay(1000);
	}
		
}
void incubator_reset(unsigned char bird_t)		//Called as a part of new hatch
{

	unsigned char days_in_month;
	readrtc();
	Date_trigger 	= Date_current;
	Date_countup 	= Date_current;
	Month_trigger 	= Month_current;
	Year_trigger 	= Year_current;
	
	// Keep next day deatails in Date_countup
	Date_countup++;
	if((Date_countup & 0x0F) == 0x0A)
		Date_countup = (Date_countup & 0xF0) + 0x10;		

	unsigned char  month_corrected;
				
	month_corrected = Month_current;
				
	if(Month_current == 0x10)
					month_corrected = 10;
				
	if(Month_current == 0x11)
					month_corrected = 11;
				
	if(Month_current == 0x12)
					month_corrected = 12;
					
	if(Date_countup == (dm[month_corrected - 1] + 0x1))	
	{
		Date_countup = 0x01;
	}
	
	day_count = 0;	
	
	
	for(i=0;i<(stop_rotation_at[bird_t] + ((HR_current > 0x12) ? 1:0));i++)
	{
		Date_trigger++;
		if((Date_trigger & 0x0F) == 0x0A)
		{
			Date_trigger = (Date_trigger & 0xF0) + 0x10;	
			//printf("\n Condition 1\n");
		}

		if((Month_trigger == 0x2) && ((2000+Year_trigger)% 4 == 0))
		{
			days_in_month = 0x29;
			//printf("\n Condition 2\n");
		} else {
			days_in_month = dm[((Month_trigger&0xF0)>>4)*10 + (Month_trigger&0xF)- 0x1];
			//printf("\n Condition 3.. DAYS IN MONTH: %d %c %x is %d %c %x\n",Month_trigger,Month_trigger,Month_trigger,days_in_month,days_in_month,days_in_month);
		}
		

		if((( days_in_month + 0x1) & 0x0F) == 0xA)
		{
			days_in_month = (( days_in_month + 0x1) & 0xF0) +0x10;
			//printf("\n Condition 4\n");
		}
		else {
			days_in_month = days_in_month +1;
			//printf("\n Condition 5\n");
		}
		
		//printf("\n\n Date trigger: %x Days in month : %x\n",Date_trigger,days_in_month);

		if(Date_trigger == days_in_month)	
		{
			Date_trigger = 0x01;
			Month_trigger = Month_trigger + 1;
			if(Month_trigger == 0x13)
				Month_trigger = 1;
			
			if(Month_trigger == 1)
			{
				Year_trigger = Year_current + 1;
			}
		}	
	}
	
	
	I2CSendAddr(RTCADDR,WRITE); 
	I2CSendByte(0x08); // write register address, 1st clock register
	I2CSendByte(HR_current);	//0x8
	I2CSendByte(Date_current);	//0x9
	I2CSendByte(Month_current);	//0xA
	I2CSendByte(HR_trigger);	//0xB
	I2CSendByte(Date_trigger);	//0xC
	I2CSendByte(Month_trigger);	//0xD
	I2CSendByte(Date_countup);	//0xE
	I2CSendByte(day_count);		//0xF
	I2CSendByte(bird_type);		//0x10
	I2CSendByte(Year_trigger);	//0x11		-- added lately
	I2CSendByte(HR_current);	//0x12		-- hour reference for day count
	I2CSendByte(Minute_current);//0x13		-- minute reference for day count
	I2CSendStop(); 
	//*** write to RTC RAM location HR_current:Date_current:Month_current and HR_triggger:Date_trigger:Month_trigger

	cmdroutine(0x80);
	temp1 = Date_current >> 4;   //Today's Date
	dataroutine(0x30+temp1);
	temp1 = Date_current & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine('/');	
	temp1 = Month_current >> 4;  //Month
	dataroutine(0x30+temp1);
	temp1 = Month_current & 0x0F;
	dataroutine(0x30+temp1);
	
	dataroutine(' ');
	dataroutine('t');
	dataroutine('o');
	dataroutine(' ');
	
	temp1 = Date_trigger >> 4;   //Today's Date
	dataroutine(0x30+temp1);
	temp1 = Date_trigger & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine('/');	
	temp1 = Month_trigger >> 4;  //Month
	dataroutine(0x30+temp1);
	temp1 = Month_trigger & 0x0F;
	dataroutine(0x30+temp1);	
	dataroutine(' ');		
	dataroutine(' ');
	dataroutine(' ');		
	dataroutine(' ');
	dataroutine(' ');		
	dataroutine(' ');
	dataroutine(' ');		
	dataroutine(' ');
	dataroutine(' ');		
	dataroutine(' ');
	dataroutine(' ');		
	dataroutine(' ');
	dataroutine(' ');		
	dataroutine(' ');
}	


void read_rtc_and_update(int mode)
{
   readrtc();
   unsigned char total_days;
   
   
	cmdroutine(0x94);
	temp1 = Date_current >> 4;   //Today's Date
	dataroutine(0x30+temp1);
	temp1 = Date_current & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine('/');	
	temp1 = Month_current >> 4;  //Month
	dataroutine(0x30+temp1);
	temp1 = Month_current & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine('/');	
	temp1 = Year_current >> 4;  //Year
	dataroutine(0x30+temp1);
	temp1 = Year_current & 0x0F;
	dataroutine(0x30+temp1);

	dataroutine(' ');

	temp1 = HR_current >> 4;     //hour
	dataroutine(0x30+temp1);
	temp1 = HR_current & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine(':');
	temp1 = Minute_current >> 4; //minutes
	dataroutine(0x30+temp1);
	temp1 = Minute_current & 0x0F;
	dataroutine(0x30+temp1);
	

	
	cmdroutine(0xD4);	//at line 4 in LCD
	dataroutine('C');
	dataroutine('D');
	dataroutine(':');
	
	//Find number of days between current date and the trigger date
	//BCD to hex conversion of date/month/year, done inside find_no_of_days
	//days_to_go = find_no_of_days(bcd2hex(Date_current),bcd2hex(Month_current),bcd2hex(Year_current),
	//							bcd2hex(Date_trigger),bcd2hex(Month_trigger),17);
	
	//days_to_go = find_no_of_days(2,11,2011,19,11,2011);
	//days_to_go = hex2bcd(days_to_go); // Total days till today in BCD
	
	dataroutine(0x30 + (((day_count) & 0xF0) >> 4)); 
	dataroutine(0x30 + ((day_count)  & 0x0F));
	dataroutine('/');
	
	total_days = hex2bcd(total_hatch_days[bird_type]);	//Total hatch days needed in BCD
	
	dataroutine(0x30 + (((total_days) & 0xF0) >> 4));
	dataroutine(0x30 + ((total_days)  & 0x0F));	
	
	dataroutine(' ');
	dataroutine('S');
	dataroutine('R');
	dataroutine(':');
	temp1 = Date_trigger >> 4;   //Trigger Date
	dataroutine(0x30+temp1);
	temp1 = Date_trigger & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine('/');	
	temp1 = Month_trigger >> 4;  //Month
	dataroutine(0x30+temp1);
	temp1 = Month_trigger & 0x0F;
	dataroutine(0x30+temp1);
	dataroutine('/');	
	temp1 = Year_trigger >> 4;  //Year
	dataroutine(0x30+temp1);
	temp1 = Year_trigger & 0x0F;
	dataroutine(0x30+temp1);
   
	
	
	cmdroutine(0x86);
	dataroutine(0x30+((Date_countup >> 4) &0x0F));
	dataroutine(0x30+(Date_countup & 0x0F));
	dataroutine(0x30+((HR_ref_for_day_count >> 4) &0x0F));
	dataroutine(0x30+(HR_ref_for_day_count & 0x0F));
	
	

   if(HR_current == HR_trigger)
   {     
   		//if((Date_current == Date_countup) && (HR_current == HR_ref_for_day_count))
		if(Date_current == Date_countup)
   		{
				day_count++;
				if((day_count & 0x0F) == 0x0A)
					day_count = (day_count & 0xF0) + 0x10;	
					
				Date_countup++;
				if((Date_countup & 0x0F) == 0x0A)
					Date_countup = (Date_countup & 0xF0) + 0x10;	

				int month_corrected;
				
				month_corrected = Month_trigger;
				
				if(Month_trigger == 0x10)
					month_corrected = 10;
				
				if(Month_trigger == 0x11)
					month_corrected = 11;
				
				if(Month_trigger == 0x12)
					month_corrected = 12;

				if((Month_trigger == 2) && ((2000 + Year_trigger) % 4 == 0)) {
					dm[month_corrected -1] = 29;
				}
					

				
			//cmdroutine(0x80);
			//dataroutine(0x30+(((Date_countup) >> 4) &0x0F));
			//dataroutine(0x30+((Date_countup) & 0x0F));
			//dataroutine(' ');
			//dataroutine(0x30+(((month_corrected) >> 4) &0x0F));
			//dataroutine(0x30+((month_corrected) & 0x0F));
			//dataroutine(' ');
			//dataroutine(0x30+(((dm[month_corrected -2] + 1) >> 4) &0x0F));
			//dataroutine(0x30+((dm[month_corrected -2] + 0x1) & 0x0F));
		
				if (month_corrected == 1) {	//next month is January
					if(Date_countup == (dm[11] + 0x1))	//Date_countup goes 1 above
					Date_countup = 0x01;
				} else {
					//month_corrected is next month and so to get the max days of the current month, need to subtract by 2
					if(Date_countup == (dm[month_corrected -2] + 0x1))	
					{
						Date_countup = 0x01;
					}
				}
					
				I2CSendAddr(RTCADDR,WRITE); 
				I2CSendByte(0x0E); // write register address, 1st clock register
				I2CSendByte(Date_countup);
				I2CSendByte(day_count);
				I2CSendStop();
        	}
		
    }
	
	if(HR_current == HR_trigger)
	{
         
     	temp1 = (((HR_current) & 0xF0) >> 4)*10;     // BCD to hex coversion to get the hour in decimal
   		temp1 = temp1 + (HR_current & 0x0F);

	//if(((Date_current < Date_trigger) && (Month_current == Month_trigger))|| ((Date_current > Date_trigger) && (Month_current < Month_trigger)))
        if((day_count >= ROT_START_DAY) && (((((day_count & 0xF0) >> 4) *10) + (day_count & 0xF)) <= stop_rotation_at[bird_type]))
   		{
			if((temp1 % ROT_RPT) == 0) 
			{
							
				//if(SWITCH2)
				{
					cmdroutine(0x8F);
					dataroutine('(');
					dataroutine('R');
					dataroutine('O');
					dataroutine('T');
					dataroutine(')');
				}
					
				rotate_eggs(ROT_DURATION);
				//if(SWITCH2)
				{
					cmdroutine(0x8F);
					dataroutine(' ');
					dataroutine(' ');
					dataroutine(' ');
					dataroutine(' ');
				}
			}
   		}
		
		else
		   stop_rotation = 1;
   	
   		
   		if((temp1 % AIR_RPT) == 0) 
   		{
   		   airation_factor = AIR_DURATION;	   
		   /*
		   if (stop_rotation == 0x0) //control humidity 
		   {
				if ((RH > 54) && (RH < 66))
					airation_factor = RH - 54;  			
		   }
		   else //allow humidity to develop on last days, so reduce aeration.
			    airation_factor = 2;
			*/
					
			//if(SWITCH2)
			{
				cmdroutine(0x8F);
				dataroutine('(');
				dataroutine('A');
				dataroutine('I');
				dataroutine('R');
				dataroutine(')');
			}
			
				airate(airation_factor);
			//if(SWITCH2)
			{
				cmdroutine(0x8F);
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
			}
   		}
		
   		if((temp1 % MIX_RPT) == 0)
   		{
			//if(SWITCH2)
			{
				cmdroutine(0x8F);
				dataroutine('(');
				dataroutine('M');
				dataroutine('I');
				dataroutine('X');
				dataroutine(')');
			}
				
				mixing(MIX_DURATION);
			//if(SWITCH2)
			{
				cmdroutine(0x8F);
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
			}
   		}
		
		// set the HR_Trigger to next hour
   		HR_trigger = HR_current + 1;
   		if ((HR_trigger & 0x0F) == 0x0A)
			HR_trigger = (HR_trigger & 0xF0) + 0x10;
   		if (HR_trigger == 0x24)
			HR_trigger = 0x00;
   		
   } // mode == 1
}	//read_rtc_and_update
   
void incubator_loop (void)
{

	//double temp_high_max, temp_low_min, temp_high_threshold,temp_low_threshold;
	read_rtc_and_update(1);

	// if rotation is already stopped (means approaching hatch day), reduce the over all  temperature by 0.5 degree  

	temp_loop_count = 1;
	Minute_at_which_temp_sensing_started = Minute_current;
	
	temp_loop_start = 1;
	
	read_rtc_and_update(1);
	unsigned char Minute_check = Minute_current;
   
	while(temp_loop_count > 0) {
		read_rtc_and_update(1);
		readtemp(TSENSOR1);
		//Tdry = (tempsense + decimalpart/1000);
		Tdry = temperature;
		readtemp(TSENSOR2);
		//Twet = (tempsense + decimalpart/1000);
		Twet = temperature;
   
		//if((((day_count & 0xF0) >> 4)*10 + (day_count &0xF))  < stop_rotation_at[bird_type])
		if((((day_count & 0xF0) >> 4)*10 + (day_count &0xF)) < reduce_temp_at[bird_type])
		{
			temp_high_max 		= 	TEMP_HIGH_MAX_1;
			temp_low_min 		= 	TEMP_LOW_MIN_1;
			temp_high_threshold	=	TEMP_HIGH_THRESHOLD_1;
			temp_low_threshold	=	TEMP_LOW_THRESHOLD_1;
		}
		else {
			temp_high_max 		= 	TEMP_HIGH_MAX_2;
			temp_low_min 		= 	TEMP_LOW_MIN_2;
			temp_high_threshold	=	TEMP_HIGH_THRESHOLD_2;
			temp_low_threshold	=	TEMP_LOW_THRESHOLD_2;   
		}
   
   
		if(Tdry > temp_high_max)
		{
			temphigh = 1;
			if((Next_one == 0xFF) && (first_time_high == 0))
			{
				first_time_high = 1;
				temp_loop_count	= 10;
				Next_one		= Minute_current;
			}
		}
	
	
		if(Tdry < temp_low_min)
		{
			templow = 1;
			if ((Next_one == 0xFF) && (first_time_low == 0))
			{
				first_time_low 	= 1;
				temp_loop_count	= 10;
				Next_one		= Minute_current;
			}
		}
	
		if(Tdry < temp_high_threshold)
		{
			temphigh = 0;
			//first_time_high = 0;
		
		}
	
		if(Tdry > temp_low_threshold)
		{
			templow = 0;
			//first_time_low = 0;
		}
	
		//if(SWITCH2) -- print of both screens
		{
			cmdroutine(0x8B);
		
			if(temphigh == 1)
			{
				dataroutine('(');
				dataroutine('T');
				dataroutine('H');
				dataroutine(')');
			}
			if(templow == 1)
			{
				dataroutine('(');
				dataroutine('T');
				dataroutine('L');
				dataroutine(')');
			}
		
			if((templow == 0) && (temphigh == 0))
			{
				cmdroutine(0x8B);
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				Next_one = 0xFF;
				temp_loop_count	= 0;
				temp_unstable	= 0;
			} else {
				temp_unstable 	= 1;
				if(temp_loop_start == 1)
				{
					temp_loop_start = 0;
					temp_loop_count = 10;
				}
			}
		}
		
		if(temp_unstable == 1) 
					{
						cmdroutine(0x8F);
						dataroutine('[');
						dataroutine(0x30+((Next_one &0xF0) >> 4));
						dataroutine(0x30+(Next_one &0x0F));
						dataroutine(']');
						dataroutine(' ');
					} else {
						Next_one = 0xFF;
						/*
						cmdroutine(0x89);//8F);
						dataroutine('O');
						dataroutine('K');*/
					}

		if((templow == 1) || (temphigh ==1))
		{
			if ((first_time_high == 1) || (first_time_low == 1) || (((Next_one == Minute_check) && (temp_unstable == 1)) && (temp_loop_count >0)))
			{
				if(temp_loop_count == 1)
				{
					//Next_one = Minute_at_which_temp_sensing_started + LAMP_DRIVE_INTERVAL;
					//try removing the comment below to fix '00' issue
					Next_one = Minute_current + LAMP_DRIVE_INTERVAL;
					if ((Next_one & 0x0F) > 0x09)
						Next_one = ((Next_one & 0xF0) + 0x10) + ((Next_one & 0x0F) - 0x0A);
					if (Next_one > 0x59)
						Next_one = 0;
					
				}
				
				first_time_low =0;
				first_time_high = 0;
			
				if(templow == 1)
					set_bit_LAMPDRIVER1_B1();
				else
					set_bit_LAMPDRIVER1_B2();
				
				cmdroutine(0x8B);
				if(templow == 1) 
				{
				
					dataroutine('(');
					dataroutine('T');
					dataroutine('L');
					dataroutine(')');
					dataroutine('(');
					dataroutine('I');
					dataroutine('N');
					dataroutine('C');
					dataroutine(')');
				} else {	
					dataroutine('(');
					dataroutine('T');
					dataroutine('H');
					dataroutine(')');
					dataroutine('(');
					dataroutine('D');
					dataroutine('E');
					dataroutine('C');
					dataroutine(')');
				}
			
				delay(LAMP_DRIVE_DURATION/10 * 1000);
				
				clear_bit_LAMPDRIVER1_B1();
				clear_bit_LAMPDRIVER1_B2();
				cmdroutine(0x8F);
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
			
			}
		}
		temp_loop_count--;
	}
	
   
// Relative Humidity Control
	if(Tdry > Twet)
	{
		A = 0.00066*(1+0.0011*Twet);
		eSwet = exp((16.78*Twet - 116.9)/(Twet + 237.3));
		ed = eSwet - A*Pr*(Tdry-Twet);
		eSdry = exp((16.78*Tdry - 116.9)/(Tdry + 237.3));
		RH = (ed/eSdry) * 100;
            
		//if (!SWITCH2)
		{
			cmdroutine(0xA5);//0xD1
			temp1 = ((int) RH) /10;
			dataroutine(0x30+temp1);
			temp1 = ((int)RH) % 10;
			dataroutine(0x30+temp1);
			dataroutine('%');
		}
	
		if((((day_count & 0xF0) >> 4)*10 + (day_count &0xF))  < stop_rotation_at[bird_type])
		{
			humidity_max 	= HUM_MAX_TILL_LAST_DAYS;
			humidity_min	= HUM_MIN_TILL_LAST_DAYS;
		} else {
			humidity_max 	= HUM_MAX_ON_LAST_DAYS;
			humidity_min	= HUM_MIN_ON_LAST_DAYS;
		}
	
		if((RH >= humidity_min) && (RH <= humidity_max))
		{
			cmdroutine(0x8F);
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
		}
/*
	//If we want to drive signals always, use below code
	if(RH > humidity_max)
	{
		set_bit_rh_max();
	}
	else
		clear_bit_rh_max();
	
	if(RH < humidity_min)
	{
		set_bit_rh_min();	
	}
	else
		clear_bit_rh_min();	
		*/
//////////
	
		if(RH > humidity_max)
		{
			humidity_high_flag = 1;
			cmdroutine(0x8F);
			dataroutine('(');
			dataroutine('H');
			dataroutine('H');
			dataroutine(')');
			humidity_low_flag = 0;
		} else {
			humidity_high_flag = 0;
		}
		
		
		if((Next_humidity_drive == 0xFF) && (first_time_humidity_high == 0) &&(humidity_high_flag == 1))
		{
			first_time_humidity_high = 1;
		}
		
		if(RH < humidity_min)
		{
			humidity_low_flag = 1;
			cmdroutine(0x8F);
			dataroutine('(');
			dataroutine('H');
			dataroutine('L');
			dataroutine(')');
			humidity_high_flag = 0;
		} else {
			humidity_low_flag = 0;
		}
		
		if ((Next_humidity_drive == 0xFF) && (first_time_humidity_low == 0) && (humidity_low_flag == 1))
		{
			first_time_humidity_low = 1;
		}
	
		if((humidity_high_flag == 0) && (humidity_low_flag == 0))
		{
			cmdroutine(0x8F);
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			Next_humidity_drive = 0xFF;
		}


		if((humidity_low_flag == 1) || (humidity_high_flag ==1))
		{
			if ((first_time_humidity_high == 1) || (first_time_humidity_low == 1) || (Minute_current >= Next_humidity_drive))
			{
				first_time_humidity_low =0;
				first_time_humidity_high = 0;
			
				if(humidity_low_flag == 1)
					set_bit_rh_min();
				if(humidity_high_flag == 1)
					set_bit_rh_max();
			
				cmdroutine(0x8B);
				if(humidity_low_flag == 1) 
				{
					dataroutine('(');
					dataroutine('H');
					dataroutine('L');
					dataroutine(')');
					dataroutine('(');
					dataroutine('I');
					dataroutine('N');
					dataroutine('C');
					dataroutine(')');
				} 
				
				if(humidity_high_flag == 1)
				{
					dataroutine('(');
					dataroutine('H');
					dataroutine('H');
					dataroutine(')');
					dataroutine('(');
					dataroutine('D');
					dataroutine('E');
					dataroutine('C');
					dataroutine(')');
				}
			
				
				delay(HUMIDITY_DRIVE_DURATION * 1000);
			
				clear_bit_rh_max();
				clear_bit_rh_min();
			
				cmdroutine(0x8B);
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
				dataroutine(' ');
			
				Next_humidity_drive = Minute_current + HUMIDITY_DRIVE_INTERVAL;
				if ((Next_humidity_drive & 0x0F) > 0x09)
					Next_humidity_drive = ((Next_humidity_drive & 0xF0) + 0x10) + ((Next_humidity_drive & 0x0F) - 0x0A);
				if (Next_humidity_drive > 0x59)
					Next_humidity_drive = 0;
			}
		}
	}	
	else
	{
		if (!SWITCH2)
		{
			cmdroutine(0xA5);
			dataroutine('I');
			dataroutine('N');
			dataroutine('V');
			clear_bit_rh_min();	
			clear_bit_rh_max();	
		}
	}
}   			
void set_rtc_time(void)
{
	I2CSendAddr(RTCADDR,WRITE);
	I2CSendByte(0x00);	 //address from which writing has to start
	I2CSendByte(0x00);	 //seconds
	I2CSendByte(MINUTE); //minutes
	I2CSendByte(HOUR);   //hr in 24 hr format
	I2CSendByte(0x01);   //day (mon = 0x1, tue = 0x2, wed = 0x3 ...sun = 0x7)
	I2CSendByte(DATE);   //date
	I2CSendByte(MONTH);  //month
	I2CSendByte(YEAR);   //year (Eg: 2009 is noted as 0x10)
	I2CSendByte(0x00);   //disable square wave output
	I2CSendStop(); 
}
//**********************************************************************
void readrtc(void)
{
	I2CSendAddr(RTCADDR,WRITE); 
	I2CSendByte(0x00);
	I2CSendAddr(RTCADDR,READ); // command byte read temp
	HR_current 		= I2CGetByte(0x00); // will have seconds_ but not saved
	Minute_current 	= I2CGetByte(0x00); // will have minutes
	HR_current 		= I2CGetByte(0x00); // will have current hour
	Date_current 	= I2CGetByte(0x00); // will have day_ but not saved
	Date_current 	= I2CGetByte(0x00); // will have current date
	Month_current 	= I2CGetByte(0x00); // will have current month
	Year_current	= I2CGetByte(0x01); // will have current year
	I2CSendStop(); //send stop
	
}

//*******************************************************************
/*
void readtemp( unsigned char addr)
{
	int i,j,t1,t2,s;
	int temp_and_half_bit_sum,half_bit_sum,temp_and_half_bit,half_bit;
// taking 5 readings and averaging it...
	
	i=0;
	j=0;
	t1=0;
	t2=0;
	temp_and_half_bit = 0;
	temp_and_half_bit_sum = 0;
	half_bit = 0;
	half_bit_sum = 0;
	s=0;
	
   for(j=0;j<5;j++)
   {
   	I2CSendAddr(addr,WRITE); // address 000 device 1
	I2CSendByte(0x51); // command byte start conversion  0x51 for DS1631 and 0xEE for DS1624
	I2CSendStop(); //send stop
	
//		delay(10000);
// 	delay(10000);
//		delay(10000);
	
//		cmdroutine(0x80);
	 	I2CSendAddr(addr,WRITE); // command byte device address
		I2CSendByte(0xAA); // command byte read temp
		I2CSendAddr(addr,READ); // command byte read temp
		t1 = I2CGetByte(0x00);
		t2 = I2CGetByte(0x01);
		temp_and_half_bit_sum = temp_and_half_bit_sum + t1;
		half_bit_sum = half_bit_sum + t2; //byte two
//		delay(10000);
   }
		
	temp_and_half_bit = (temp_and_half_bit_sum / 5) ;  
	half_bit = half_bit_sum / 5;
	
   t1 = 500;
   for(i=0;i<5;i++)
   { 
	   if (half_bit & 0x80) 
   		s = s+ t1;
	   t1 = t1 >> 1;
		half_bit = half_bit <<1;
	}
	
   if(addr==0x9E)
	{
		integer_offset = TEMP_OFFSET_WET_INTEGER;
		decimal_offset = TEMP_OFFSET_WET_DECIMAL;
	}
	else
	{
		integer_offset = TEMP_OFFSET_DRY_INTEGER;
		decimal_offset = TEMP_OFFSET_DRY_DECIMAL;
	}
	
   
   
   temp_and_half_bit = temp_and_half_bit + integer_offset;
   s = s + decimal_offset;
   
	if(s < 0)
	{
		temp_and_half_bit--;
		s = 1000 + s;
	}
	
	if(s >= 1000)
	{
		temp_and_half_bit++;
		s = s - 1000;
	}
	
   decimalpart = s;
   
   
   if (!SWITCH2)
	{
		cmdroutine(0x80);
		dataroutine('D');
		dataroutine(':');
		
		cmdroutine(0x88);
		if(bird_type == 0)
		{
			dataroutine('H');
			dataroutine('E');
			dataroutine('N');
		}
		else if(bird_type == 1)
		{
			dataroutine('Q');
			dataroutine('U');
			dataroutine('A');
			dataroutine('I');
			dataroutine('L');
		}
		else if(bird_type == 2)
		{
			dataroutine('T');
			dataroutine('U');
			dataroutine('R');
			dataroutine('K');
			dataroutine('Y');
		}
		else
		{
			dataroutine('D');
			dataroutine('U');
			dataroutine('C');
			dataroutine('K');
		}
		cmdroutine(0xC0);
		dataroutine('W');
		dataroutine(':');
		cmdroutine(0xC8);
		dataroutine('H');
		dataroutine(':');
		
		if(addr==0x9E)
			cmdroutine(0xC2);
		else
			cmdroutine(0x82);
		tempsense = temp_and_half_bit;
		t1 = temp_and_half_bit/10;
		dataroutine(0x30+t1);
		t1 = temp_and_half_bit % 10;
		dataroutine(0x30+t1);
		dataroutine('.');
		t1 = s / 100;
		dataroutine(0x30+t1);
		s = s % 100;
		t1 = s / 10;
		dataroutine(0x30+t1);
	}
	// last digit truncated while displaying
	//temp1 = s % 10;
	//dataroutine(0x30+temp1);
}
*/

void readtemp( unsigned char addr)
{
	
	
	acquire_adc_channels(addr);
	temperature	= adc_value;
	
	if(addr==TSENSOR1)
	{
		temperature = temperature + TEMP_OFFSET_DRY_SENSOR;
	}
	else
	{
		temperature = temperature + TEMP_OFFSET_WET_SENSOR;
	}
	
	
	adc_value = temperature;	//temperature value without decimal point. Used for dispaly 
	temperature = temperature/10;
   
   //if (!SWITCH2)
	{
		cmdroutine(0xC0);
		dataroutine('D');
		dataroutine(':');
		
		cmdroutine(0x80);
		if(bird_type == 0)
		{
			dataroutine('B');
			dataroutine(':');
			dataroutine('H');
			dataroutine('E');
			dataroutine('N');
		}
		else if(bird_type == 1)
		{
			dataroutine('B');
			dataroutine(':');
			dataroutine('Q');
			dataroutine('U');
			dataroutine('L');
		}
		else if(bird_type == 2)
		{
			dataroutine('B');
			dataroutine(':');
			dataroutine('T');
			dataroutine('K');
			dataroutine('Y');
		}
		else
		{
			dataroutine('B');
			dataroutine(':');
			dataroutine('D');
			dataroutine('C');
			dataroutine('K');
		}
		cmdroutine(0xCA);
		dataroutine('W');
		dataroutine(':');
		cmdroutine(0xA3);//0xCF
		dataroutine('H');
		dataroutine(':');
		
		if(addr==TSENSOR2)
			cmdroutine(0xCC);
		else
			cmdroutine(0xC2);
			
		dataroutine(0x30 + adc_value/100);
		adc_value = adc_value % 100;
		dataroutine(0x30 + adc_value/10);
		dataroutine('.');
		adc_value = adc_value % 10;
		dataroutine(0x30 + adc_value);
		dataroutine(0xDF); //degree symbol
		dataroutine('C');
	}
	// last digit truncated while displaying
	//temp1 = s % 10;
	//dataroutine(0x30+temp1);
}

void GenStart(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	//cmdroutine(0xC0);
	//dataroutine(TWSR & 0xF8);

}

void I2CSendAddr(unsigned char addr, unsigned char rd)
{	
	GenStart();
	TWDR = addr + rd;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	//cmdroutine(0xC1);
	//dataroutine(TWSR & 0xF8);
	
	if ((rd == 0x00) && ((TWSR & 0xF8) != MT_ADDR_ACK))
	{
		I2CSendStop();
		cmdroutine(0xCA);
		dataroutine('A');
		
		while(1)
		{
			;
		}
	}

	if ((rd == 0x01) && ((TWSR & 0xF8) != MR_ADDR_ACK))
	{
		I2CSendStop();
		cmdroutine(0xCA);
		dataroutine('a');
		
		while(1)
		{
			;
		}
	}
}



void I2CSendByte(unsigned char data)
{
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	//cmdroutine(0xC2);
	//dataroutine(TWSR & 0xF8);
	
	if ((TWSR & 0xF8) != MT_DATA_ACK)
	{
		I2CSendStop();
		cmdroutine(0xCB);
		dataroutine('B');
		
		while(1)
		{
			;
		}
	}	
}


unsigned char I2CGetByte(unsigned char nack)
{
	unsigned char read_data;
	if (nack == 0x00)
	{
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	}
	else
	{
		TWCR = (1<<TWINT)|(1<<TWEN);
	}
		
	while (!(TWCR & (1<<TWINT)));
	//cmdroutine(0x86);
	//dataroutine(TWDR);
	
	//cmdroutine(0xCC);
	while((TWSR & 0xF8) == 0xF8);
		
	if ((nack == 0x00) && ((TWSR & 0xF8) != MR_DATA_ACK))
	{
		I2CSendStop();
		cmdroutine(0xCD);
		dataroutine('C');
		while(1)
		{
			;
		}
	}
	
	if ((nack == 0x01) && ((TWSR & 0xF8) != MR_DATA_NACK))
	{
		I2CSendStop();
		cmdroutine(0xCE);
		dataroutine('c');
		
		while(1)
		{
			;
		}
	}
	read_data = TWDR;
	return (read_data);
}
	
void I2CSendStop(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}



void display_8bit(char data,char location)
{	
	unsigned char a;
	cmdroutine(location);
	a = (data & 0xF0) >> 4;
	dataroutine(0x30+a);
	cmdroutine(location+1);
	a = data & 0x0F;
	dataroutine(0x30+a);
}

void cmdroutine_8bit(char instrctn)
{	 
	 PORTC = (PORTC & 0xC3) | (((instrctn & 0x80) >> 5) | ((instrctn & 0x40) >> 3) |((instrctn & 0x20) >>1) | ((instrctn & 0x10) << 1));
	 clear_bit_RS();
	 delay(lp);
	 clear_bit_RW();
	 delay(lp);
	 set_bit_E();
	 delay(lp);
	 clear_bit_E();
	 delay(lp);
	 set_bit_RW();
	 delay(lp);
}

void cmdroutine(char instrctn)
{
	 char temp;
	 temp = instrctn & 0xF0;
	 PORTC = (PORTC & 0xC3) | (((temp & 0x80) >> 5) | ((temp & 0x40) >> 3) |((temp & 0x20) >>1) | ((temp & 0x10) << 1));
	 clear_bit_RS();
	 delay(lp);
	 clear_bit_RW();
	 delay(lp);
	 set_bit_E();
	 delay(lp);
	 clear_bit_E();
	 delay(lp);
	 set_bit_RW();
	 delay(lp);
	 temp = (instrctn << 4) & 0xF0;
	 PORTC = (PORTC & 0xC3) | (((temp & 0x80) >> 5) | ((temp & 0x40) >> 3) |((temp & 0x20) >>1) | ((temp & 0x10) << 1));
	 clear_bit_RS();
	 delay(lp);
	 clear_bit_RW();
	 delay(lp);
	 set_bit_E();
	 delay(lp);
	 clear_bit_E();
	 delay(lp);
	 set_bit_RW();
	 delay(lp);
}

void dataroutine(char dat)
{
	
	char temp;
	cmdroutine(0x06);
	temp = dat & 0xF0;
	PORTC = (PORTC & 0xC3) | (((temp & 0x80) >> 5) | ((temp & 0x40) >> 3) |((temp & 0x20) >>1) | ((temp & 0x10) << 1));
	set_bit_RS();
	delay(lp);
	clear_bit_RW();
	delay(lp);
	set_bit_E();
	delay(lp);
	clear_bit_E();
	delay(lp);
	set_bit_RW();
	delay(lp);
	
	temp = (dat << 4) & 0xF0;
	PORTC = (PORTC & 0xC3) | (((temp & 0x80) >> 5) | ((temp & 0x40) >> 3) |((temp & 0x20) >>1) | ((temp & 0x10) << 1));
	set_bit_RS();
	delay(lp);
	clear_bit_RW();
	delay(lp);
	set_bit_E();
	delay(lp);
	clear_bit_E();
	delay(lp);
	set_bit_RW();
	delay(lp);
}

void lcdinitroutine(void)
{
    cmdroutine_8bit(0x30);
	cmdroutine_8bit(0x30);
	cmdroutine_8bit(0x30);
	cmdroutine_8bit(0x20);
	cmdroutine(0x28);
	cmdroutine(0x0C);
	cmdroutine(0x01);
	cmdroutine(0x06);
	cmdroutine(0x80);
}	

void USART_Init( void)
{
/* Set baud rate */
/* Enable receiver and transmitter, Rx complete interrupt*/
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
/* Set frame format: 8data, 2stop bit,Even parity */
/* (0<<USBS) --> 1 stop bit
   (1<<USBS) --> 2 stop bits */
	UCSRC = (1<<URSEL)|(0<<USBS)|(3<<UCSZ0)|(2<<UPM0);
/* Configuring for 1200 bps baus*/
	UBRRH = 0;
	UBRRL = 50;
/* Global Interrupt Enable*/
	SREG = SREG | (0x80);
}

void USART_Transmit( unsigned char data )
{
    
	unsigned char letter;
	while ( !( UCSRA & (1<<UDRE)))
			;
/* Put data into buffer, sends the data */
		UDR = data;
}

/*
void adc_capture(unsigned char channel)
{
    ADCSRA = 0x00; //disable adc
    ADMUX = 0xC0 | channel; //using 2.56V internal reference source, select adc channel
    ACSR  = 0x80; // shut off power to analog comparator
    ADCSRA = 0xC9; //enable ADC, start conversion, No auto trigger, ADC interrupt enabled
				   //division factor between clock frequency and input clock to ADC is 2
}

SIGNAL (SIG_ADC)
{
	data1 = ADCL; //read lower byte first..important
	data2 = ADCH;
	value = data2 * 256 + data1;
	adc_complete_flag = 1;
}
*/

void reset_variables(void)
{
 tempsense=0.0,decimalpart = 0.0,Tdry=0.0,Twet=0.0, Pr = 101.325;
 stop_rotation = 0;
 lp = 2;
 airation_factor = 5;
 templow=0,temphigh=0,clow=0,chigh=0,Next_one = 0xFF;
 charging_status=0, dont_mix=0, sound_minute_trigger =0, adc_complete_flag = 0, sound_counter = 0;
 count = 0x21;
 stop_rotation_at[0] = CHICK_ROT_STOP,stop_rotation_at[1]=QUAIL_ROT_STOP;
 stop_rotation_at[2] = TURKY_ROT_STOP,stop_rotation_at[3]=DUCK_ROT_STOP;
 
 reduce_temp_at[0] = CHICK_TEMP_REDUCE_DAY, reduce_temp_at[1] = QUAIL_TEMP_REDUCE_DAY;
 reduce_temp_at[2] = TURKY_TEMP_REDUCE_DAY, reduce_temp_at[3] = DUCK_TEMP_REDUCE_DAY; 
 
 total_hatch_days[0] = CHICK_HATCH_DAYS;
 total_hatch_days[1] = QUAIL_HATCH_DAYS;
 total_hatch_days[2] = TURKEY_HATCH_DAYS;
 total_hatch_days[3] = DUCK_HATCH_DAYS;
 rot_stop_day = stop_rotation_at[BIRD];
 rot_direction_history = 0x20;
}

void delay(long int p) // p =1000 === 1sec
{
	long int i,j;
/*
	for (j=0;j<p;j++)
	{
		for (i=0;i<45;i++)
		delay_1(5000000);
	}
	*/
	for (j=0;j<(1+p/6);j++)
	{
		_delay_ms(1);
	}


}
void delay_1(long int p)
{
	long int i,j,k,l;
	for (j=0;j<p;j++)
	{
		for (i=0;i<1000000;i++)
		{
			for (k=0;k<1000000;k++)
			{
				for (l=0;l<1000000;l++)
					{
						;
					}
			}
		}
	}
}

void rotate_eggs(unsigned int dur)
{
	long int i,j,k,l,p;
	set_bit_ROTTNCNTRL();
/*
	p = dur;
	
	DDRD = DDRD & 0xFB;	//make PORTD.2 as input
	
	PORTB = 0;
	PORTB = PORTB | 0x40;
	
	PORTA = (PORTA & 0x9F) | rot_direction_history;
	
	for (j=0;j<p;j++)
	{
		//for (i=0;i<1000000;i++)
		{
			for (k=0;k<3300;k++)
			{
				for (l=0;l<30;l++)
				{
					//Removed UP and DOWN control on the rotation on 25th July 2013
					//due to some board issue
					if(0)
					{
						//PORTA ^= (1 << 5);
						if((PORTA & 0x60) == 0x20)
						{
							PORTA  = (PORTA & 0x9F) | 0x40;	// clear
							rot_direction_history = PORTA & 0x60;
							cmdroutine(0x8B);
							dataroutine('(');
							dataroutine('U');
							dataroutine('P');
							dataroutine(')');
						} else if ((PORTA & 0x60) == 0x40) {
							PORTA  = (PORTA & 0x9F) | 0x20;	//	set
							rot_direction_history = PORTA & 0x60;
							cmdroutine(0x8B);
							dataroutine('(');
							dataroutine('D');
							dataroutine('N');
							dataroutine(')');
						}
						while(ROT_CTRL)
						{
							;
						}
					}
				}
			}
		}
	}
	
	PORTB = 0;
	PORTB = PORTB | 0x1;
	*/
	delay(dur*1000);
	clear_bit_ROTTNCNTRL();
	PORTA  = PORTA & 0x9F;	// clear both bits PA5&PA6 (pins 34&35)
	cmdroutine(0x89);
	dataroutine(' ');
	DDRD = DDRD | 0x04;
	
}

void airate(unsigned int dur)
{
	set_bit_AIRTNCNTRL();
	delay(dur*1000);
	clear_bit_AIRTNCNTRL();
}

void mixing(unsigned int dur)
{
	set_bit_MIXIGCNTRL();
	delay(dur*1000);
	clear_bit_MIXIGCNTRL();
}   

void clear_bit_RS(void)
{
	PORTD &= ~(1<<7);
}
	
void clear_bit_RW(void)
{
	PORTC &= ~(1<<7);
}

void clear_bit_E(void)
{
	PORTC &= ~(1<<6);
}

void clear_bit_CHARGE(void)
{
	PORTD &= ~(1<<5);
}

void clear_bit_ALARM_CTRL(void)
{
	PORTD &= ~(1<<7);
}

void clear_bit_LAMPDRIVER1_B2(void)
{
	PORTD &= ~(1<<4);
}

void clear_bit_LAMPDRIVER1_B1(void)
{
	PORTD &= ~(1<<3);
}
void clear_bit_rh_min(void)
{
	PORTD &= ~(1<<5);
}
void clear_bit_rh_max(void)
{
	PORTD &= ~(1<<6);
}
void clear_bit_AIRTNCNTRL(void)
{
	PORTA &= ~(1<<3);
}

void clear_bit_ROTTNCNTRL(void)
{
	//PORTA &= ~(1<<2);
	PORTA &= ~(1<<5);

}

void clear_bit_MIXIGCNTRL(void)
{
    PORTA &= ~(1<<4);
}

void set_bit_RS(void)
{
	PORTD |= 1<<7;
}

void set_bit_RW(void)
{
	PORTC |= 1<<7;
}
void set_bit_E(void)
{
	PORTC |= 1<<6;
}

void set_bit_CHARGE(void)
{
	PORTD |= (1<<5);
}

void set_bit_ALARM_CTRL(void)
{
	PORTD |= (1<<7);
}

void set_bit_LAMPDRIVER1_B2(void)
{
	PORTD |= (1<<4);
}

void set_bit_LAMPDRIVER1_B1(void)
{
	PORTD |= (1<<3);
}

void set_bit_AIRTNCNTRL(void)
{
	PORTA |= (1<<3);
}
void set_bit_rh_min(void)
{
	PORTD |= (1<<5);
}
void set_bit_rh_max(void)
{
	PORTD |= (1<<6);
}
void set_bit_ROTTNCNTRL(void)
{
	//PORTA |= (1<<2);
	PORTA |= (1<<5);
}

void set_bit_MIXIGCNTRL(void)
{
    PORTA |= (1<<4);
}



void set_date_and_time(void)
{

		cmdroutine(0x80);
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('F');
		dataroutine('U');
		dataroutine('L');
		dataroutine('L');
		dataroutine('Y');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('A');
		dataroutine('U');
		dataroutine('T');
		dataroutine('O');
		dataroutine('M');
		dataroutine('A');
		dataroutine('T');
		dataroutine('I');
		dataroutine('C');
		dataroutine(' ');
		dataroutine(' ');
		cmdroutine(0xC0);
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('E');
		dataroutine('G');
		dataroutine('G');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('I');
		dataroutine('N');
		dataroutine('C');
		dataroutine('U');
		dataroutine('B');
		dataroutine('A');
		dataroutine('T');
		dataroutine('O');
		dataroutine('R');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		cmdroutine(0x94);
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('v');
		dataroutine('1');
		dataroutine('.');
		dataroutine('1');
		dataroutine('.');
		dataroutine('0');
		dataroutine('7');
		dataroutine('.');
		dataroutine('2');
		dataroutine('0');
		dataroutine('2');
		dataroutine('0');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		cmdroutine(0xD4);
		dataroutine('.');
		dataroutine('.');
		dataroutine('*');
		dataroutine('A');
		dataroutine('L');
		dataroutine('E');
		dataroutine('R');
		dataroutine('T');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('D');
		dataroutine('E');
		dataroutine('V');
		dataroutine('I');
		dataroutine('C');
		dataroutine('E');
		dataroutine('S');
		dataroutine('*');
		dataroutine('.');
		dataroutine('.');
		
		delay(2000);
		j=0;
	while(j < 3)
	{		
		cmdroutine(0x80);
		dataroutine('I');
		dataroutine('f');
		dataroutine(' ');
		dataroutine('y');
		dataroutine('o');
		dataroutine('u');
		dataroutine(' ');
		dataroutine('w');
		dataroutine('a');
		dataroutine('n');
		dataroutine('t');
		dataroutine(' ');
		dataroutine('t');
		dataroutine('o');
		dataroutine(' ');
		dataroutine('r');
		dataroutine('e');
		dataroutine('s');
		dataroutine('e');
		dataroutine('t');
		cmdroutine(0xC0);
		dataroutine(' ');
		dataroutine('t');
		dataroutine('i');
		dataroutine('m');
		dataroutine('e');
		dataroutine(' ');
		dataroutine('a');
		dataroutine('n');
		dataroutine('d');
		dataroutine(' ');
		dataroutine('d');
		dataroutine('a');
		dataroutine('t');
		dataroutine('e');
		dataroutine(',');
		dataroutine(' ');
		dataroutine('p');
		dataroutine('u');
		dataroutine('t');
		dataroutine(' ');
		cmdroutine(0x94);
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('S');
		dataroutine('1');
		dataroutine(' ');
		dataroutine('s');
		dataroutine('w');
		dataroutine('i');
		dataroutine('t');
		dataroutine('c');
		dataroutine('h');
		dataroutine(' ');
		dataroutine('O');
		dataroutine('N');	
		dataroutine(' ');
		dataroutine('n');
		dataroutine('o');
		dataroutine('w');
		dataroutine(' ');
		dataroutine(' ');
		cmdroutine(0xD4);
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('a');
		dataroutine('n');
		dataroutine('d');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine('w');
		dataroutine('a');
		dataroutine('i');
		dataroutine('t');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		dataroutine(' ');
		
			
		delay(1000);
		
		if(SWITCH1)
		{			
			delay(2000);
			cmdroutine(0x80);
			dataroutine('G');
			dataroutine('o');
			dataroutine('i');
			dataroutine('n');
			dataroutine('g');
			dataroutine(' ');
			dataroutine('t');
			dataroutine('o');
			dataroutine(' ');
			dataroutine('s');
			dataroutine('e');
			dataroutine('t');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			
			//Routine to set time with DATE/MONTH/YEAR && HOUR:MINUTE
			set_rtc_time();
			readrtc();
		
			cmdroutine(0x80);
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('T');
			dataroutine('i');
			dataroutine('m');
			dataroutine('e');
			dataroutine(' ');
			dataroutine('S');
			dataroutine('e');
			dataroutine('t');
			dataroutine(' ');
			dataroutine('P');
			dataroutine('l');
			dataroutine('z');
			dataroutine(' ');
			dataroutine('R');
			dataroutine('s');
			dataroutine('t');
			dataroutine(' ');
			dataroutine(' ');
			
			cmdroutine(0xC0);
			for (j=0;j<20;j++)
				dataroutine(' ');
	
			cmdroutine(0xC0);
			temp1 = Date_current >> 4;   //Today's Date
			dataroutine(0x30+temp1);
			temp1 = Date_current & 0x0F;
			dataroutine(0x30+temp1);
			dataroutine('/');	
			temp1 = Month_current >> 4;  //Month
			dataroutine(0x30+temp1);
			temp1 = Month_current & 0x0F;
			dataroutine(0x30+temp1);

			dataroutine(' ');

			temp1 = HR_current >> 4;     //hour
			dataroutine(0x30+temp1);
			temp1 = HR_current & 0x0F;
			dataroutine(0x30+temp1);
			dataroutine(':');
			temp1 = Minute_current >> 4; //minutes
			dataroutine(0x30+temp1);
			temp1 = Minute_current & 0x0F;
			dataroutine(0x30+temp1);
			
			cmdroutine(0x94);
			for (j=0;j<20;j++)
				dataroutine(' ');
			cmdroutine(0xD4);
			for (j=0;j<20;j++)
				dataroutine(' ');
				
			while(1)
			{
				;
			}
		}
		
		if (j%2 == 0)
			PORTB = 0x3; //All LEDs OFF
		else
			PORTB = 0x2; //LED @ PORTB.0 ON
		j++;
	}
}

void new_hatch(void)
{
	j=0;	
	lcdinitroutine();	
	cmdroutine(0x80);
		dataroutine('F');
		dataroutine('o');
		dataroutine('r');
		dataroutine(' ');
		dataroutine('n');
		dataroutine('e');
		dataroutine('w');
		dataroutine(' ');
		dataroutine('h');
		dataroutine('a');
		dataroutine('t');
		dataroutine('c');
		dataroutine('h');
		dataroutine(' ');
		dataroutine('p');
		dataroutine('u');
		dataroutine('t');
		dataroutine(' ');
		dataroutine('S');
		dataroutine('1');
		cmdroutine(0xC0);
		dataroutine('s');
		dataroutine('w');
		dataroutine('i');
		dataroutine('t');
		dataroutine('c');
		dataroutine('h');
		dataroutine(' ');
		dataroutine('O');
		dataroutine('N');
		dataroutine(' ');
		dataroutine('a');
		dataroutine('n');
		dataroutine('d');
		dataroutine(' ');
		dataroutine('s');
		dataroutine('e');
		dataroutine('l');
		dataroutine('e');
		dataroutine('c');
		dataroutine('t');
		cmdroutine(0x94);
		dataroutine('b');
		dataroutine('i');
		dataroutine('r');
		dataroutine('d');
		dataroutine(' ');
		dataroutine('u');
		dataroutine('s');
		dataroutine('i');
		dataroutine('n');
		dataroutine('g');
		dataroutine(' ');
		dataroutine('s');
		dataroutine('w');
		dataroutine('i');	
		dataroutine('t');
		dataroutine('c');
		dataroutine('h');
		dataroutine(' ');
		dataroutine('S');
		dataroutine('2');
		
	while(j < 7)
	{		
		delay(1000);
		
		
		if(SWITCH1)
		{
			delay(2000);
			lcdinitroutine();
			cmdroutine(0x80);
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('R');
			dataroutine('e');
			dataroutine('s');
			dataroutine('e');
			dataroutine('t');
			dataroutine(':');
			dataroutine(':');
			dataroutine('N');
			dataroutine('e');
			dataroutine('w');
			dataroutine(' ');
			dataroutine('H');
			dataroutine('a');
			dataroutine('t');
			dataroutine('c');
			dataroutine('h');
			dataroutine(' ');
			dataroutine(' ');
		cmdroutine(0xC0);
		dataroutine('S');
		dataroutine('e');
		dataroutine('l');
		dataroutine('e');
		dataroutine('c');
		dataroutine('t');
		dataroutine(' ');
		dataroutine('b');
		dataroutine('i');
		dataroutine('r');
		dataroutine('d');
		dataroutine(' ');
		dataroutine('u');
		dataroutine('s');
		dataroutine('i');
		dataroutine('n');
		dataroutine('g');
		dataroutine(' ');
		dataroutine('S');
		dataroutine('2');
		
			
			I2CSendAddr(RTCADDR,WRITE); 
			I2CSendByte(0x0C);
			I2CSendAddr(RTCADDR,READ); // command byte read temp
			//	HR_trigger = I2CGetByte(0); // will have seconds_ but not saved
			Date_trigger 	= I2CGetByte(0x00); // will have date trigger
			Month_trigger 	= I2CGetByte(0x00); // will have month trigger
			Date_countup 	= I2CGetByte(0x00); // will have date countup
			day_count 		= I2CGetByte(0x00);
			bird_type 		= I2CGetByte(0x00);
			Year_trigger 	= I2CGetByte(0x00);	//added late
			HR_ref_for_day_count = I2CGetByte(0x00);
			Minute_ref_for_day_count = I2CGetByte(0x01);
					
			I2CSendStop(); //send stop
			
			if (SWITCH2)
			{
				lcdinitroutine();
				while(SWITCH2);
				delay(1000);
				set_bird_type(CHICK);
				while(!SWITCH2);
				delay(1000);
				set_bird_type(QUAIL);
				while(SWITCH2);
				delay(1000);
				set_bird_type(TURKY);
				while(!SWITCH2);
				delay(1000);
				set_bird_type(DUCK);				
			}
			else
			{
				while(!SWITCH2);
				delay(1000);
				set_bird_type(CHICK);
				while(SWITCH2);
				delay(1000);
				set_bird_type(QUAIL);
				while(!SWITCH2);
				delay(1000);
				set_bird_type(TURKY);
				while(SWITCH2);
				delay(1000);
				set_bird_type(DUCK);	
			}
		
			while(1)
			{
				;
			}
		}
		
		if (j%2 == 0)
			PORTB = 0x3;	//All LEDs OFF
		else
			PORTB = 0xB;	//LED @ PORTB.3 ON
		j++;
	}	
}

void set_bird_type(unsigned char b)
{		
		unsigned char bird_select;
		cmdroutine(0xC0);
	
	
		if(b == 0)
		{
			
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('f');
			dataroutine('o');
			dataroutine('r');
			dataroutine(' ');
			dataroutine(' ');
		
			dataroutine('H');
			dataroutine('E');
			dataroutine('N');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			bird_select = b;
		}
		else if(b == 1)
		{
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('f');
			dataroutine('o');
			dataroutine('r');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('Q');
			dataroutine('U');
			dataroutine('A');
			dataroutine('I');
			dataroutine('L');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			bird_select = b;
		}
		else if(b == 2)
		{
			
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('f');
			dataroutine('o');
			dataroutine('r');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('T');
			dataroutine('U');
			dataroutine('R');
			dataroutine('K');
			dataroutine('Y');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			bird_select = b;
		}
		else
		{
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('f');
			dataroutine('o');
			dataroutine('r');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine('D');
			dataroutine('U');
			dataroutine('C');
			dataroutine('K');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			dataroutine(' ');
			bird_select = b;
		}
	
	I2CSendAddr(RTCADDR,WRITE); 
	I2CSendByte(0x08); // write register address, 1st clock register
	I2CSendByte(HR_current);
	I2CSendByte(Date_current);
	I2CSendByte(Month_current);
	I2CSendByte(HR_trigger);
	I2CSendByte(Date_trigger);
	I2CSendByte(Month_trigger);
	I2CSendByte(Date_countup);
	I2CSendByte(day_count);
	I2CSendByte(b);	
	I2CSendByte(Year_trigger);	
	I2CSendByte(HR_current);		// Hour reference for day count up
	I2CSendByte(Month_trigger);		// Minute reference for day count up
	
	HR_ref_for_day_count = HR_current;
	
	I2CSendStop(); 
	
	bird_type = b;
	
	incubator_reset(bird_select); //reseting incubator -- recounts the hatch day
}

