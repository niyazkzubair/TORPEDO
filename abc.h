
#define WRITE 			0
#define READ 			1
#define TSENSOR1 		0x0    //dry bulb
#define TSENSOR2 		0x1    //wet bulb
#define RTCADDR  		0xD0    //RTC


#define START 			0x08
#define REP_START		0x10
#define MT_ADDR_ACK 	0x18 //SLA + W transmitted and ACK got back
#define MT_DATA_ACK 	0x28 //Data has been written and ACK got back
#define MR_ADDR_ACK 	0x40 //SLA + R transmitted and ACK got back
#define MR_DATA_ACK 	0x50 //Read the data and ACK sent to slave
#define MR_DATA_NACK	0x58

#define CHICK			0x0
#define QUAIL			0x1
#define TURKY			0x2
#define DUCK			0x3

#define SWITCH1			((PINB & 0x10) == 0x10)		//PB-3
#define SWITCH2			((PIND & 0x04) == 0x0)		//PD-2
#define SWITCH2_ON		((PIND & 0x04) == 0x1)		//PD-2
#define ROT_CTRL			((PINB & 0x01) == 0x1)		//PB-0


void clear_bit_RS(void);
void clear_bit_RW(void);
void clear_bit_E(void);
void clear_bit_MIXIGCNTRL(void);
void clear_bit_ROTTNCNTRL(void);
void clear_bit_AIRTNCNTRL(void);
void clear_bit_LAMPDRIVER1_B1(void);
void clear_bit_LAMPDRIVER1_B2(void);
void clear_bit_rh_min(void);
void clear_bit_rh_max(void);
void clear_bit_CHARGE(void);
void clear_bit_ALARM_CTRL(void);

void set_bit_RS(void);
void set_bit_RW(void);
void set_bit_E(void);
void set_bit_MIXIGCNTRL(void);
void set_bit_ROTTNCNTRL(void);
void set_bit_AIRTNCNTRL(void);
void set_bit_LAMPDRIVER1_B1(void);
void set_bit_LAMPDRIVER1_B2(void);
void set_bit_rh_min(void);
void set_bit_rh_max(void);
void set_bit_CHARGE(void);
void set_bit_ALARM_CTRL(void);

void GenStart(void);
void I2CSendAddr(unsigned char addr, unsigned char rd);
void I2CSendByte(unsigned char data);
void I2CSendStop(void);
unsigned char I2CGetByte(unsigned char nack);
void incubator_reset(unsigned char bird_t);

void cmdroutine(char instrctn);
void cmdroutine_8bit(char instrctn);
void dataroutine(char dat);
void lcdinitroutine(void);
void USART_Init( void);
void USART_Transmit( unsigned char data );
void adc_capture(unsigned char channel);
void delay(long int p);
void delay_1(long int p);
void reset_variables(void);

void readtemp(unsigned char addr);
void readrtc(void);
void incubator_loop(void);  
void set_date_and_time(void);
void rotate_eggs(unsigned int dur);
void airate(unsigned int dur);
void mixing(unsigned int dur);
//void incubator_reset(void);
void new_hatch(void);
void read_rtc_and_update(int mode);
double tempsense=0.0,decimalpart = 0.0,integer_offset,decimal_offset,Tdry=0.0,Twet=0.0, Pr = 101.325,A,eSwet,ed,eSdry,RH;
unsigned char data1,data2,stop_rotation = 0,usart_data,siren_status=0;
long int lp = 2,value;
long int i,j;
int airation_factor = 5;
long int stop_rotation_at[4]={19,16,25,25},rot_stop_day = 17; //chicken,quail,duck
double reduce_temp_at[4];
unsigned char total_hatch_days[4];
// Date_trigger:Month_trigger stores the date on which rotation has to be stopped
unsigned char temp1,templow=0,temphigh=0,clow=0,chigh=0,Next_one = 0xFF,Next_humidity_drive = 0xFF,Minute_current,HR_current,HR_trigger;
unsigned char humidity_high_flag = 0,humidity_low_flag=0,humidity_max=0,humidity_min=0;
unsigned char first_time_humidity_high=0,first_time_humidity_low = 0;
unsigned char charging_status=0, dont_mix=0, sound_minute_trigger =0, adc_complete_flag = 0, sound_counter = 0;
unsigned char Date_current,Date_trigger,Date_countup,day_count;
unsigned char Month_current,Month_trigger,HR_ref_for_day_count,Minute_ref_for_day_count;
unsigned char count = 0x21,ab;
unsigned char Config_Data,screen1_first_time=1,screen2_first_time=1,bird_type=0,first_time_low=0,first_time_high=0,blank=0;
unsigned char dm[12] = {0x31,0x28,0x31,0x30,0x31,0x30,0x31,0x31,0x30,0x31,0x30,0x31};
unsigned char target_date;

double temp_high_max, temp_low_min, temp_high_threshold,temp_low_threshold;
int temp_loop_count;
unsigned char Minute_at_which_temp_sensing_started;
int temp_unstable,temp_loop_start;
unsigned char rot_direction_history;