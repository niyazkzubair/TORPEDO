/* ---------- SET BELOW VARIABLES ---------- */

/* SET 1 -- Select bird; Options: CHICK, QUAIL */
#define BIRD					CHICK  

/* SET 2 -- Temperature related (all in degree celcious) */
//                             |<---system tries to keep temp b/w these points--->|
//---- |37.5|------------------|37.6-----------------------------------------|37.7|------------------|37.80|-------
// TEMP_LOW_MIN           TEMP_LOW_THRESHOLD                           TEMP_HIGH_THRESHOLD         TEMP_HIGH_MAX

/*

#define TEMP_HIGH_MAX_ACTUAL_1			37.8
#define	TEMP_HIGH_THRESHOLD_ACTUAL_1	37.7
#define TEMP_LOW_THRESHOLD_ACTUAL_1		37.6
#define TEMP_LOW_MIN_ACTUAL_1			37.5
*/

#define TEMP_HIGH_MAX_ACTUAL_1			37.8
#define	TEMP_HIGH_THRESHOLD_ACTUAL_1	37.7
#define TEMP_LOW_THRESHOLD_ACTUAL_1		37.6
#define TEMP_LOW_MIN_ACTUAL_1			37.5

/* Last days */
#define TEMP_HIGH_MAX_ACTUAL_2			37.3
#define	TEMP_HIGH_THRESHOLD_ACTUAL_2	37.2
#define TEMP_LOW_THRESHOLD_ACTUAL_2		37.1
#define TEMP_LOW_MIN_ACTUAL_2			37.0

//If +0.6 give as 6, for +0.75 give as 7.5
//If -0.6 give as -6
#define TEMP_OFFSET_DRY_SENSOR			-5
#define TEMP_OFFSET_WET_SENSOR			+5
#define TEMP_OFFSET				0

/* Normal Temp range */
#define TEMP_HIGH_MAX_1					TEMP_HIGH_MAX_ACTUAL_1 + TEMP_OFFSET
#define	TEMP_HIGH_THRESHOLD_1			TEMP_HIGH_THRESHOLD_ACTUAL_1 + TEMP_OFFSET
#define TEMP_LOW_THRESHOLD_1			TEMP_LOW_THRESHOLD_ACTUAL_1 + TEMP_OFFSET
#define TEMP_LOW_MIN_1					TEMP_LOW_MIN_ACTUAL_1 + TEMP_OFFSET

/* Temp range in the last day(s) of hatch */
#define TEMP_HIGH_MAX_2					TEMP_HIGH_MAX_ACTUAL_2 + TEMP_OFFSET
#define	TEMP_HIGH_THRESHOLD_2			TEMP_HIGH_THRESHOLD_ACTUAL_2 + TEMP_OFFSET
#define TEMP_LOW_THRESHOLD_2			TEMP_LOW_THRESHOLD_ACTUAL_2 + TEMP_OFFSET
#define TEMP_LOW_MIN_2					TEMP_LOW_MIN_ACTUAL_2 + TEMP_OFFSET

/* SET 3 -- Durations in seconds*/
#define ROT_DURATION			10
#define	AIR_DURATION			10
#define	MIX_DURATION			8
#define ROT_UP_DURATION			5
#define ROR_DN_DURATION			5

/* SET 4 -- Intermittance in hours */
#define	ROT_RPT					0x1
#define	AIR_RPT					0x1
#define	MIX_RPT					0x1

/* SET 5 -- Wick Lamp related settings; applicable only when temp is low or high */
//max value of LAMP_DRIVE_DURATION is 250
#define	LAMP_DRIVE_DURATION		100 // for how many seconds,lamp to be driven (temp low/high)
#define LAMP_DRIVE_INTERVAL		2 // time in minutes after which wick lamp motor is driven next time

/* SET 6 -- Humidity LOW/HIGH driver */
#define HUMIDITY_DRIVE_DURATION	5 // for how many seconds signal has to come
#define HUMIDITY_DRIVE_INTERVAL	4 // time in minutes after which humidity signal is driven

/* SET 7 -- Total number of hatch days */
#define CHICK_HATCH_DAYS		21
#define QUAIL_HATCH_DAYS		18
#define TURKEY_HATCH_DAYS		28	//not updated properly please check
#define DUCK_HATCH_DAYS			28  //not updated properly please check
 
/* SET 8 -- Rotation stop day 	*/
#define	CHICK_ROT_STOP			18
#define QUAIL_ROT_STOP			15
#define TURKY_ROT_STOP			25
#define DUCK_ROT_STOP			25

/* SET 9 -- Humidity related (Changes from the day temp. changed) */
#define HUM_MIN_TILL_LAST_DAYS	        75
#define HUM_MAX_TILL_LAST_DAYS	        80
#define HUM_MIN_ON_LAST_DAYS		    85
#define HUM_MAX_ON_LAST_DAYS	        90

/* SET 10 -- from which day to reduce temperature */
#define CHICK_TEMP_REDUCE_DAY	18
#define QUAIL_TEMP_REDUCE_DAY	16
#define TURKY_TEMP_REDUCE_DAY	26
#define DUCK_TEMP_REDUCE_DAY	26

/* SET 11 -- Rotation Start Day */
#define ROT_START_DAY		0x0

	
	
/* ------- USER SETTINGS END HERE -------- */

//------------------------------ Pin Number

#define RS  			(PORTD,7)	//21
#define RW  			(PORTC,7)	//29
#define E   			(PORTC,6)	//28

#define ROTTNCNTRL 		(PORTA,2)	//38
#define AIRTNCNTRL 		(PORTA,3)	//37
#define MIXIGCNTRL 		(PORTA,4)	//36
#define ROTTNCNTROL_P1	(PORTA,5)	//35
#define ROTTNCNTROL_P2	(PORTA,6)	//34

#define LAMPDRIVER1_B1 	(PORTD,3)	//17
#define LAMPDRIVER1_B2 	(PORTD,4)	//18
#define HUM_LOW_DRIVER	(PORTD,5)	//19
#define HUM_HIGH_DRIVER	(PORTD,6)	//20