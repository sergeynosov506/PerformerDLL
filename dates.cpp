/*

	dates.c


		
		  Dates Library

  */


#include "dates.h"


/*   dlLongToDateStruct

	Convert a date represented as a long (i.e. 19981031) to
	a dlDateStruct with integers for month, day, and year.  */


void dlLongToDateStruct(long date, dlDateStruct *ds)
{
	ds->day = date % 100;
	date -= ds->day;

	ds->month = (date % 10000) / 100;
	date -= (ds->month * 100);

	ds->year = date / 10000;
}


/*	dlDateStructToLong

	given a dlDateStruct, convert it to the long date  */

long dlDateStructToLong(dlDateStruct *ds)
{
	long d;

	d = (ds->year * 10000) + (ds->month * 100) + (ds->day);
	return d;
}



/*	dlDaysInMonth

	Given a month, determine the number of days in the month.
	Will account for leap-year in February.   */

int dlDaysInMonth(int month, int year)
{
	int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if (month == 2 && dlIsLeapYear(year))
		return 29;
	return daysInMonth[month - 1];
}


/*  dlMakeDateLast

		given a long date, return the long date which is the
		last day of the month   */

long dlMakeDateLast(long date)
{
	dlDateStruct ds;
	long retDate;

	dlLongToDateStruct(date, &ds);
	ds.day = dlDaysInMonth(ds.month, ds.year);
	retDate = dlDateStructToLong(&ds);

	return retDate;
}



/*	dlWhichDay

		given a long date, return the day  */

int dlWhichDay(long date)
{
	dlDateStruct ds;

	dlLongToDateStruct(date, &ds);
	return ds.day;
}



/*	dlWhichMonth

		given a long date, return the month  */

int dlWhichMonth(long date)
{
	dlDateStruct ds;

	dlLongToDateStruct(date, &ds);
	return ds.month;
}


/*	dlWhichYear

		given a long date, return the year  */

int dlWhichYear(long date)
{
	dlDateStruct ds;

	dlLongToDateStruct(date, &ds);
	return ds.year;
}


/*  dlIsLeapYear

	Return true (1) if the given year is a leap year.  Otherwise
	return false (0).    */

int dlIsLeapYear(int year)
{
	if (year % 4 == 0)  {
		if ((year % 100) == 0 && (year % 400 != 0))
			return 0;
		return 1;
	}
	return 0;
}



/*  dlDayCount

	Given two dates and a basis for year count and day count,
	return the number of days between the dates   */

int dlDayCount(long date1, long date2, int monthBasis, int yearBasis, int endOfMonthRule)
{
	dlDateStruct ds1, ds2;

	/*  put earlier date information into ds1; later date information to ds2  */
	if (date2 > date1)  
  {
		dlLongToDateStruct(date1, &ds1);
		dlLongToDateStruct(date2, &ds2);
	}
	else  
  {
		dlLongToDateStruct(date2, &ds1);
		dlLongToDateStruct(date1, &ds2);
	}

	/*  handle 30/360 convention
			Rules are from TIPS manual, page 18.  */
  /* SB 3/22/2002 - According to handbook of global fixed income calculations 
     by dragomir krgin, page 22, daycount are calculated same way for 30/365 */
	if (monthBasis == dl30 && (yearBasis == dl360 || yearBasis == dl365))  
  {
		int dayDiff;

		/* if d2 is the last day of February and d1 is the last day of February,
			change d2 to 30 */
		if (ds2.month == 2 && ds2.day == dlDaysInMonth(2, ds2.year) &&
			  ds1.month == 2 && ds1.day == dlDaysInMonth(2, ds1.year) &&
			  endOfMonthRule)
			ds2.day = 30;

		/* if d1 is the last day of February, change d1 to 30 */
		if (ds1.month == 2 && ds1.day == dlDaysInMonth(2, ds1.year) &&
			  endOfMonthRule)
			ds1.day = 30;

		/* if d2 is 31 and d1 is 30 or 31, change d2 to 30 */
		if (ds2.day == 31 && ds1.day >= 30)
			ds2.day = 30;

		/* if d1 is 31, change d1 to 30 */
		if (ds1.day == 31)
			ds1.day = 30;

		/* now compute the number of days between the dates */
		dayDiff = ((ds2.year - ds1.year) * 360) +
					    ((ds2.month - ds1.month) * 30) +
					     (ds2.day - ds1.day);

		return dayDiff;
  } // 30/360 & 30/365


  /* SB 3/22/2002 - Formula for 30/360E - from handbook of global fixed income calculations 
     by dragomir krgin, page 22. Also, for the time being 30/360E+1 is being treated the same way */
	if (monthBasis == dl30 && (yearBasis == dl360E || yearBasis == dl365))  
  {
		int dayDiff;

  	/* if d1 is 31, change d1 to 30 */
		if (ds1.day == 31)
			ds1.day = 30;

  	/* if d2 is 31, change d2 to 30 */
		if (ds2.day == 31)
			ds2.day = 30;

		/* now compute the number of days between the dates */
		dayDiff = ((ds2.year - ds1.year) * 360) +
					    ((ds2.month - ds1.month) * 30) +
					     (ds2.day - ds1.day);
  } // 30/360E & 30/360E+1

	/*  handle ACT/ACT convention.
			rules are from the TIPS manual, p. 20  */
  /* SB 3/22/2002 ACT/365, ACT/360 should have same rule to calculate # of days */
	if (monthBasis == dlACT && (yearBasis == dlACT || dl360 || dl365))  
  {
		return (dlDayNumber(&ds2) - dlDayNumber(&ds1));
	}

  return 0;
}



/*	dlDayNumber

		compute the number of days since 00/00/0000.
		rules from TIPS manual, p. 20   */

int dlDayNumber(dlDateStruct *ds)
{
/*	int mp, yp, t, nd;

	if (ds->month <= 2)  {
		mp = 0;
		yp = ds->year - 1;
	}
	else  {
		mp = (int)(0.4 * ds->month + 2.3);
		yp = ds->year;
	}

	t = (int)(yp / 4) - (int)(yp / 100) + (int)(yp / 400);

	nd = (365 * ds->year) + (31 * (ds->month - 1)) + ds->day + t - mp;
	return nd; */

	int j;
	int m = ds->month, d = ds->day, y = ds->year;

	j =  d - 32075 + (1461 * (y + 4800 + (m - 14) / 12)) / 4;
	j += (367 * (m - 2 - ((m - 14) / 12) * 12)) / 12;
	j -= (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4;

	return j;
}



/*   dlAddMonthsToDate

	given a long date, add the given number of months to it
	and return a long date.  Will NOT handle end-of-month, i.e.
	Jan 31 + 1 month = Feb 31 (which is nonsensical)  */

long dlAddMonthsToDate(long date, int nMonths)
{
	dlDateStruct ds;
	long retDate;
	int i, sign = 1;

	if (nMonths < 0)  {
		sign = -1;
		nMonths *= -1;
	}
	dlLongToDateStruct(date, &ds);

	for (i=0; i < nMonths; i++)  {
		ds.month += sign;
		if (ds.month <= 0)  {   /* make sure we don't go out of bounds */
			ds.year--;
			ds.month = 12;
		}
		else if (ds.month >= 13)  {   /* bounds check */
			ds.year++;
			ds.month = 1;
		}
	}

	retDate = dlDateStructToLong(&ds);
	return retDate;
}



/*  dlIsValidDate:

		take a long date and determine if it is a valid date.
		returns 1 (true) or 0 (false)  */

int dlIsValidDate(long date)
{
	dlDateStruct ds;

	dlLongToDateStruct(date, &ds);

	if (ds.year <= 1901)
		return 0;
	if (ds.month <= 0 || ds.month >= 13)
		return 0;
	if (ds.day <= 0 || ds.day > dlDaysInMonth(ds.month, ds.year))
		return 0;

	return 1;
}


/*  dlJulianToLong

	given a Julian date and the long date that it is based off of,
	return a date in long format  */

__declspec(dllexport) long dlJulianToLong(long julian, long baseDate)
{
	int nDayBase;
	dlDateStruct ds;

	dlLongToDateStruct(baseDate, &ds);

	nDayBase = dlDayNumber(&ds);
	return dlAddDaysToDate(baseDate, julian);
}



/*	dlAddDaysToDate

	Given a base date, add the given number of days to the date
	and return it in long format  */

long dlAddDaysToDate(long baseDate, int nDays)
{
	dlDateStruct ds;
	int n, direction;
	long date;

	dlLongToDateStruct(baseDate, &ds);
	if (nDays < 0)  {
		direction = -1;
		nDays *= -1;
	}
	else
		direction = 1;

	/* add/subtract one day at a time */
	for (n=0; n < nDays; n++)  {
		if (direction == 1)
			dlOneDayForward(&ds);
		else
			dlOneDayBackward(&ds);
	}

	date = dlDateStructToLong(&ds);
	return date;
}



/*	dlOneDayForward

	given a dlDateStruct, add one day to the date  */

void dlOneDayForward(dlDateStruct *ds)
{
	ds->day++;
	if (ds->day > dlDaysInMonth(ds->month, ds->year))  {
		ds->month++;
		ds->day = 1;
	}
	if (ds->month >= 13)  {
		ds->year++;
		ds->month = 1;
	}

	return;
}


/*  dsOneDayBackward

	given a dlDateStruct, step backwards one day.  */

void dlOneDayBackward(dlDateStruct *ds)
{
	ds->day--;
	if (ds->day <= 0)  {
		ds->month--;
		if (ds->month <= 0)  {
			ds->month = 12;
			ds->year--;
		}
		ds->day = dlDaysInMonth(ds->month, ds->year);
	}

	return;
}
