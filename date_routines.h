#include <stdio.h>
#include <math.h>

int find_no_of_days(int day1,int mon1,int year1,int day2, int mon2, int year2) 
{ 
	int ref,dd1,dd2,i; 
	ref = year1; 
	if(year2<year1) 
		ref = year2; 
	dd1=0; 
	dd1=func1(mon1); 
	for(i=ref;i<year1;i++) 
	{ 
		if(i%4==0) 
			dd1+=1; 
	} 
	//No. of days of first date from the Jan 1
	dd1=dd1+day1+(year1-ref)*365; 

	/* Count for additional days due to leap years*/ 
	dd2=0; 
	for(i=ref;i<year2;i++) 
	{ 
		if(i%4==0) 
			dd2+=1; 
	} 
	//No. of days from the reference year's first Jan
	dd2=func1(mon2)+dd2+day2+((year2-ref)*365); 
	return (abs(dd2-dd1)); 

	getch(); 
} 



int func1(x) //x for month y for dd 
{ 
	int y=0; 
	switch(x) 
	{ 
		case 1: y=0; break; 
		case 2: y=31; break; 
		case 3: y=59; break; 
		case 4: y=90; break; 
		case 5: y=120;break; 
		case 6: y=151; break; 
		case 7: y=181; break; 
		case 8: y=212; break; 
		case 9: y=243; break; 
		case 10:y=273; break; 
		case 11:y=304; break; 
		case 12:y=334; break; 
		default: y= -1;
	} 
	return(y); 
}

unsigned char hex2bcd (unsigned char x)
{
    unsigned char y;
    y = (x / 10) << 4;
    y = y | (x % 10);
    return (y);
}

unsigned char bcd2hex (unsigned char x)
{
	unsigned char y;
	y = ((x&0xF0) >> 4 )*10 + (x&0x0F);
	return(y);
}