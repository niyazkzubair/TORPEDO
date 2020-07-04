/* ---------- SET BELOW VARIABLES ---------- */

/* SET 1 -- Set todays Date */
/* Dont remove the '0x' prefix, it is a must for date entry */
#define DATE					0x19
#define MONTH					0x11
#define YEAR					0x19

/* SET 2 -- Set current time */
/* Dont remove the '0x' prefix, it is a must for day entry */
#define	HOUR					0x09
#define	MINUTE					0x13


// INSTRUCTION TO SET TIME & DATE
/* 
   (1) -- Ensure values to above #define steps are correctly mentioned 
   (2) -- Build (Tools -> MakeAll) and program (Tools -> Program) the chip
   (3) -- Put SWITCH1 to ON position (down position) when Yellow light only blinks after POWER ON
   (4) -- POWER OFF the board when "Time Set Plz Rst" text appears on the LCD
   (5) -- Put SWITCH1 to OFF position (up position)
   (6) -- POWER ON the board, now the board works with new time and date
*/
