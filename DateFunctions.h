#pragma once
#include "commonheader.h"
#include "TransIO_DControl.h"

// Define mdytype structure matching Pascal record
typedef struct {
    short month;
    short day;
    short year;
} mdytype;

// Exported functions matching Pascal unit
DLLAPI int STDCALL rmdyjul(mdytype* mdy, long* lDate);
DLLAPI int STDCALL rjulmdy(long lDate, mdytype* mdy);
DLLAPI BOOL STDCALL rleapyear(int iYear);
DLLAPI int STDCALL rstrdate(char* sDate, long* lDate);
DLLAPI int STDCALL NewDateFromCurrent(long lCurrentDate, BOOL bAddToCurrent, int iYear, int iMonth, int iDay, long* lNewDate);
DLLAPI double STDCALL CurrentDateAndTime(void);

DLLAPI long STDCALL CurrentMonthEnd(long lCurrentDate);
DLLAPI long STDCALL LastMonthEnd(long lCurrentDate);
DLLAPI long STDCALL NextMonthEnd(long lCurrentDate);
DLLAPI BOOL STDCALL IsItAMonthEnd(long lCurrentDate);

DLLAPI int STDCALL IsItABankHoliday(long lCurrentDate, char* sCountry);
DLLAPI int STDCALL IsItAMarketHoliday(long lCurrentDate, char* sCountry);

DLLAPI int STDCALL LastBusinessDay(long lCurrentDate, char* sCountry, char* sWhichBusiness, long* lLastBusinessDate);
DLLAPI int STDCALL NextBusinessDay(long lCurrentDate, char* sCountry, char* sWhichBusiness, long* lNextBusinessDate);

// Internal helper (exposed if needed, but usually internal to unit)
void AdjustDays(short* iMDY, BOOL bJustMakeAValidDate);
