
/*    dates.h


  Header file for dates library.   */



/*  definitions for daycount conventions  */

#define dl30    1
#define dl360   2
#define dlACT   3
#define dl365   4
#define dl360E  5
#define dl360E1 6

/*  dlDateStruct is a convenient way to hold date information  */
typedef struct {
	int day;
	int month;
	int year;
} dlDateStruct;


/*  prototypes  */
int dlDayCount(long, long, int, int, int);
int dlDayNumber(dlDateStruct *);
int dlDaysInMonth(int, int);
void dlLongToDateStruct(long, dlDateStruct *);
long dlDateStructToLong(dlDateStruct *);
int dlIsLeapYear(int);
int dlWhichDay(long);
int dlWhichMonth(long);
int dlWhichYear(long);
long dlMakeDateLast(long);
long dlAddMonthsToDate(long, int);
int dlIsValidDate(long);
__declspec(dllexport) long dlJulianToLong(long, long);
long dlAddDaysToDate(long, int);
void dlOneDayForward(dlDateStruct *);
void dlOneDayBackward(dlDateStruct *);





