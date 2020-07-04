//#include "DEFINES.h"
#include <stdio.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <inttypes.h>
#include <avr/delay.h>
#include <avr/pgmspace.h>
//extern unsigned char Year_current,Minute_current,Second_current,HR_current,Date_current,Month_current,count;
//extern unsigned char Pumping_time[10],adc_complete_flag,key_pressed,key_input,x_point,y_point;
//extern unsigned char Pumping_drtn[10];
unsigned int  adc_value;
unsigned char adc_channel_number;
/*
extern unsigned char time_array[25],time_array_read[20],Pumping_duration_hour,Pumping_duration_minute;
extern unsigned char stop_at_minute,stop_at_hour,start_pumping,minute_incr,minute_count;
extern unsigned char Pumping_flag,Pumping_count,pending_run,Pumping_date,Pumping_month;
extern unsigned int  time_array_index;
extern unsigned char Next_scheduled_pumping_time_HR,Next_scheduled_pumping_time_Minute;
extern unsigned int  global_var1, pending_minutes;
extern unsigned long int total_available_minutes,time_ref;
extern unsigned char pending_hours,day_has_changed;
extern unsigned int  pqr,time_refr;
extern unsigned char temp1,temp2,temp3,remaining_minutes,dsp_counter;	
extern unsigned char earlier_completed,with_in_minute,entry_stat;
extern unsigned char efg;
*/

unsigned int sample_count,added_adc_value,number_of_samples =10;

void acquire_adc_channels(int channel)
{
	
		adc_channel_number = channel;	//AD0 for Tdry &ADC1 for Twet are used
		
		adc_complete_flag = 0;
		sample_count = 0;
		added_adc_value = 0;
			
		while(sample_count < number_of_samples)	//take 'number_of_samples' samples
		{
			adc_capture_routine(adc_channel_number);
			while(adc_complete_flag != 1)
			{
				cmdroutine(0x80);
			}
			adc_complete_flag = 0;
			ADCSRA = ADCSRA | (1<<ADIF); //Clearing the ADC conversion done interrupt	
		}
		
		// will show the captured ADC volatge levels
		if(adc_channel_number == 0)
		{
			cmdroutine(0x80);
		} else {
			cmdroutine(0xC0);
		}
		
		adc_value = added_adc_value/number_of_samples;
		
		/*
	
		dataroutine(adc_channel_number);
		dataroutine(' ');
		dataroutine(0x30 + adc_value/100);
		adc_value = adc_value % 100;
		dataroutine(0x30 + adc_value/10);
		dataroutine('.');
		adc_value = adc_value % 10;
		dataroutine(0x30 + adc_value);
		*/
		
}	


void adc_capture_routine(unsigned char channel)
{
    ADCSRA 	= 0x00; //disable adc
    ADMUX 	= 0x40 | channel ; //select adc input 0
    ACSR  	= 0x80; //Disable analog comparator
    ADCSRA 	= 0xC9; //enable ADC, start conversion, No auto trigger, ADC interrupt enabled
				    //division factor between clock frequency and input clock to ADC is 2
	sample_count++;
}

SIGNAL (SIG_ADC)
{
	unsigned char data1,data2;
	data1 = ADCL; //read lower byte first..important
	data2 = ADCH;
	adc_value = ADC *4.873;
	
	added_adc_value = added_adc_value + adc_value;	//accumulate the samples
	
	adc_complete_flag = 1;
}

