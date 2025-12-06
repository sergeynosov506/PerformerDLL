#include "DateFunctions.h"
#include "dateutils.h"
#include <time.h>
#include <math.h>

// Helper to check leap year (internal)
BOOL IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

DLLAPI BOOL STDCALL rleapyear(int iYear) {
    return IsLeapYear(iYear);
}

// AdjustDays implementation ported from Pascal
void AdjustDays(short* iMDY, BOOL bJustMakeAValidDate) {
    int iNumDays = 0;

    // iMDY[0] = month, iMDY[1] = day, iMDY[2] = year

    // If day <= 0, subtract months
    while (iMDY[1] <= 0) {
        if (iMDY[0] == 1) { // January -> December prev year
            iMDY[0] = 12;
            iMDY[2]--;
        } else {
            iMDY[0]--;
        }

        // Add days of the new month
        if (iMDY[0] == 1 || iMDY[0] == 3 || iMDY[0] == 5 || iMDY[0] == 7 ||
            iMDY[0] == 8 || iMDY[0] == 10 || iMDY[0] == 12) {
            iMDY[1] += 31;
        } else if (iMDY[0] == 4 || iMDY[0] == 6 || iMDY[0] == 9 || iMDY[0] == 11) {
            iMDY[1] += 30;
        } else if (IsLeapYear(iMDY[2])) {
            iMDY[1] += 29;
        } else {
            iMDY[1] += 28;
        }
    }

    // If day >= 29, check if it exceeds month days
    while (iMDY[1] >= 29) {
        if (iMDY[0] == 1 || iMDY[0] == 3 || iMDY[0] == 5 || iMDY[0] == 7 ||
            iMDY[0] == 8 || iMDY[0] == 10 || iMDY[0] == 12) {
            if (iMDY[1] <= 31) break;
            iNumDays = 31;
        } else if (iMDY[0] == 4 || iMDY[0] == 6 || iMDY[0] == 9 || iMDY[0] == 11) {
            if (iMDY[1] <= 30) break;
            iNumDays = 30;
        } else if (IsLeapYear(iMDY[2])) {
            if (iMDY[1] == 29) break;
            iNumDays = 29;
        } else {
            iNumDays = 28;
        }

        if (bJustMakeAValidDate) {
            iMDY[1] = (short)iNumDays;
        } else {
            iMDY[1] -= (short)iNumDays;
            if (iMDY[0] == 12) {
                iMDY[0] = 1;
                iMDY[2]++;
            } else {
                iMDY[0]++;
            }
        }
    }
}

DLLAPI int STDCALL rmdyjul(mdytype* mdy, long* lDate) {
    if (!mdy || !lDate) return -1;
    nanodbc::date d = { mdy->year, mdy->month, mdy->day };
    *lDate = date_to_long(d);
    return 0;
}

DLLAPI int STDCALL rjulmdy(long lDate, mdytype* mdy) {
    if (!mdy) return -1;
    nanodbc::timestamp ts;
    long_to_timestamp(lDate, ts);
    if (ts.year == 0 && ts.month == 0 && ts.day == 0) return -1;
    mdy->year = (short)ts.year;
    mdy->month = (short)ts.month;
    mdy->day = (short)ts.day;
    return 0;
}

DLLAPI int STDCALL rstrdate(char* sDate, long* lDate) {
    // Basic parsing for mm/dd/yyyy
    if (!sDate || !lDate) return -1;
    int m, d, y;
    if (sscanf_s(sDate, "%d/%d/%d", &m, &d, &y) == 3) {
        nanodbc::date dt = { y, m, d };
        *lDate = date_to_long(dt);
        return 0;
    }
    return -1;
}

DLLAPI int STDCALL NewDateFromCurrent(long lCurrentDate, BOOL bAddToCurrent, int iYear, int iMonth, int iDay, long* lNewDate) {
    if (iYear < 0 || iMonth < 0 || iDay < 0) return -1;

    mdytype mdy;
    if (rjulmdy(lCurrentDate, &mdy) != 0) return -1;

    short iMDY[3];
    iMDY[0] = mdy.month;
    iMDY[1] = mdy.day;
    iMDY[2] = mdy.year;

    int iTemp = iMonth / 12;
    iYear += iTemp;
    iMonth -= (12 * iTemp);

    BOOL bJustMakeAValidDate = (iDay == 0);

    if (bAddToCurrent) {
        iMDY[2] += (short)iYear;
        iMDY[0] += (short)iMonth;
        if (iMDY[0] > 12) {
            iMDY[0] -= 12;
            iMDY[2]++;
        }
        iMDY[1] += (short)iDay;
        AdjustDays(iMDY, bJustMakeAValidDate);
    } else {
        iMDY[2] -= (short)iYear;
        iMDY[0] -= (short)iMonth;
        if (iMDY[0] <= 0) {
            iMDY[0] += 12;
            iMDY[2]--;
        }
        iMDY[1] -= (short)iDay;
        AdjustDays(iMDY, bJustMakeAValidDate);
    }

    mdy.month = iMDY[0];
    mdy.day = iMDY[1];
    mdy.year = iMDY[2];
    return rmdyjul(&mdy, lNewDate);
}

DLLAPI double STDCALL CurrentDateAndTime(void) {
    // Return OLE Automation date (days since 1899-12-30)
    time_t now = time(NULL);
    struct tm t;
    localtime_s(&t, &now);
    
    nanodbc::timestamp ts;
    ts.year = t.tm_year + 1900;
    ts.month = t.tm_mon + 1;
    ts.day = t.tm_mday;
    ts.hour = t.tm_hour;
    ts.min = t.tm_min;
    ts.sec = t.tm_sec;
    ts.fract = 0;

    // date_to_long returns days. We need to add fraction.
    long days = timestamp_to_long(ts); // This truncates in current impl?
    // Let's use custom calculation for double precision
    // timestamp_to_long in dateutils returns long, truncating time if not careful.
    // But wait, timestamp_to_long implementation in dateutils.h:
    // double frac = ...
    // return static_cast<long>(days + frac);
    // It returns long! So we lose time.
    
    // Re-implement for double return
    nanodbc::date d = { ts.year, ts.month, ts.day };
    long jdn = date_to_long(d); 
    double frac = (ts.hour * 3600.0 + ts.min * 60.0 + ts.sec) / 86400.0;
    return (double)jdn + frac;
}

DLLAPI BOOL STDCALL IsItAMonthEnd(long lCurrentDate) {
    nanodbc::timestamp ts1, ts2;
    long_to_timestamp(lCurrentDate, ts1);
    long_to_timestamp(lCurrentDate + 1, ts2);
    
    if (ts1.year == 0 || ts2.year == 0) return FALSE;
    return (ts1.month != ts2.month);
}

DLLAPI long STDCALL CurrentMonthEnd(long lCurrentDate) {
    if (IsItAMonthEnd(lCurrentDate)) return lCurrentDate;

    nanodbc::timestamp ts;
    long_to_timestamp(lCurrentDate, ts);
    if (ts.year == 0) return -1;

    ts.month++;
    if (ts.month > 12) {
        ts.month = 1;
        ts.year++;
    }
    ts.day = 1;
    
    nanodbc::date d = { ts.year, ts.month, ts.day };
    return date_to_long(d) - 1;
}

DLLAPI long STDCALL LastMonthEnd(long lCurrentDate) {
    nanodbc::timestamp ts;
    long_to_timestamp(lCurrentDate, ts);
    if (ts.year == 0) return -1;

    ts.day = 1;
    nanodbc::date d = { ts.year, ts.month, ts.day };
    return date_to_long(d) - 1;
}

DLLAPI long STDCALL NextMonthEnd(long lCurrentDate) {
    nanodbc::timestamp ts;
    long_to_timestamp(lCurrentDate, ts);
    if (ts.year == 0) return -1;

    ts.month += 2;
    while (ts.month > 12) {
        ts.month -= 12;
        ts.year++;
    }
    ts.day = 1;
    
    nanodbc::date d = { ts.year, ts.month, ts.day };
    return date_to_long(d) - 1;
}

// Helper to read DControl
int ReadDcontrolFile(DCONTROL* pzDC, BOOL* bRecFound, long lDate, char* sCountry) {
    ERRSTRUCT zErr;
    SelectDcontrol(pzDC, lDate, sCountry, &zErr);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
        if (zErr.iSqlError == SQLNOTFOUND) {
            *bRecFound = FALSE;
            return 0; // Not an error, just not found
        }
        return zErr.iSqlError ? zErr.iSqlError : -1;
    }
    // If SelectDcontrol returns success but fields are empty? 
    // SelectDcontrol implementation doesn't explicitly set "found" flag but fills struct.
    // If SQLNOTFOUND was handled inside SelectDcontrol it might return error.
    // Let's assume if no error, record found.
    // Wait, SelectDcontrol in TransIO_DControl.cpp:
    // if (result.next()) { ... }
    // It doesn't return error if not found, just doesn't fill struct.
    // We should probably check if struct was filled or initialize it.
    // But SelectDcontrol doesn't return a "found" boolean.
    // We can check if sCountry is filled?
    // Or we can modify SelectDcontrol to return found status?
    // For now, let's assume if sMarketClosed is not empty, it's found.
    
    // Actually, TransIO_DControl.cpp:
    // if (result.next()) { ... }
    // else { ... } -> Does nothing if not found!
    // So we should initialize pzDC before calling.
    
    *bRecFound = (pzDC->sMarketClosed[0] != '\0' || pzDC->sBAnkClosed[0] != '\0');
    return 0;
}

DLLAPI int STDCALL IsItABankHoliday(long lCurrentDate, char* sCountry) {
    // DayOfWeek: 0=Sunday, 1=Monday... 6=Saturday? 
    // Need standard C tm_wday: 0=Sunday.
    nanodbc::timestamp ts;
    long_to_timestamp(lCurrentDate, ts);
    
    // Calculate day of week from JDN
    nanodbc::date d = { ts.year, ts.month, ts.day };
    long jdn = date_to_long(d) + 2415019;
    int dow = (jdn + 1) % 7; // 0=Sunday, 6=Saturday
    
    if (dow == 0 || dow == 6) return 1; // Weekend

    DCONTROL zDC;
    memset(&zDC, 0, sizeof(zDC));
    BOOL bRecFound = FALSE;
    int res = ReadDcontrolFile(&zDC, &bRecFound, lCurrentDate, sCountry);
    if (res != 0) return res;

    if (bRecFound && _stricmp(zDC.sBAnkClosed, "Y") == 0) return 1;
    return 0;
}

DLLAPI int STDCALL IsItAMarketHoliday(long lCurrentDate, char* sCountry) {
    nanodbc::timestamp ts;
    long_to_timestamp(lCurrentDate, ts);
    nanodbc::date d = { ts.year, ts.month, ts.day };
    long jdn = date_to_long(d) + 2415019;
    int dow = (jdn + 1) % 7; 
    
    if (dow == 0 || dow == 6) return 1; 

    DCONTROL zDC;
    memset(&zDC, 0, sizeof(zDC));
    BOOL bRecFound = FALSE;
    int res = ReadDcontrolFile(&zDC, &bRecFound, lCurrentDate, sCountry);
    if (res != 0) return res;

    if (bRecFound && _stricmp(zDC.sMarketClosed, "Y") == 0) return 1;
    return 0;
}

DLLAPI int STDCALL LastBusinessDay(long lCurrentDate, char* sCountry, char* sWhichBusiness, long* lLastBusinessDate) {
    *lLastBusinessDate = lCurrentDate - 1;
    int res = -1;
    
    while (*lLastBusinessDate >= 0) {
        int iBank = 0;
        int iMarket = 0;
        BOOL bCheckBank = (_stricmp(sWhichBusiness, "M") != 0);
        BOOL bCheckMarket = (_stricmp(sWhichBusiness, "B") != 0);

        if (bCheckBank) {
            iBank = IsItABankHoliday(*lLastBusinessDate, sCountry);
            if (iBank == 1) {
                (*lLastBusinessDate)--;
                continue;
            } else if (iBank != 0) {
                return iBank; // Error
            }
        }

        if (bCheckMarket) {
            iMarket = IsItAMarketHoliday(*lLastBusinessDate, sCountry);
            if (iMarket == 1) {
                (*lLastBusinessDate)--;
                continue;
            } else if (iMarket != 0) {
                return iMarket; // Error
            }
        }

        // If we got here, it's a business day
        return 0;
    }
    return -1;
}

DLLAPI int STDCALL NextBusinessDay(long lCurrentDate, char* sCountry, char* sWhichBusiness, long* lNextBusinessDate) {
    *lNextBusinessDate = lCurrentDate + 1;
    long cMaxDate = 219510; // From commonheader.h MAXDATE

    while (*lNextBusinessDate < cMaxDate) {
        int iBank = 0;
        int iMarket = 0;
        BOOL bCheckBank = (_stricmp(sWhichBusiness, "M") != 0);
        BOOL bCheckMarket = (_stricmp(sWhichBusiness, "B") != 0);

        if (bCheckBank) {
            iBank = IsItABankHoliday(*lNextBusinessDate, sCountry);
            if (iBank == 1) {
                (*lNextBusinessDate)++;
                continue;
            } else if (iBank != 0) {
                return iBank; // Error
            }
        }

        if (bCheckMarket) {
            iMarket = IsItAMarketHoliday(*lNextBusinessDate, sCountry);
            if (iMarket == 1) {
                (*lNextBusinessDate)++;
                continue;
            } else if (iMarket != 0) {
                return iMarket; // Error
            }
        }

        return 0;
    }
    return -1;
}
