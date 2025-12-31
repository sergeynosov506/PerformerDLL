/*
 * CheckForInflationIndexRatio.cpp
 *
 * TIPS (Treasury Inflation-Protected Securities) Inflation Index Ratio
 * Calculation
 *
 * This is a C++ port of the Delphi implementation from InflationIndexUnit.pas
 *
 * History:
 * 2025-12-29 Initial C++ port from Delphi - sergeyn
 *
 * Original Delphi notes:
 * 2006-06-21 Added usage of Read/Write databases - vay
 * 2005-10-28 Added default Parameter Index - vay
 * 2002-09-16 Added checking for invalid values - jjc
 * 2002-06-26 First working version - jjc
 */

#include "commonheader.h"
#include "oledbio.h"
#include <math.h>
#include <string.h>

// External declarations for date functions from OLEDBIO
extern "C" {
int rmdyjul(short mdy[], long *julian);
int rjulmdy(long julian, short mdy[]);
}

// Forward declarations
static double DailyInflationIndexRatio(long SecurityIssueDate, long aDateFor,
                                       const char *cTicker, long Index);
static double ReferenceCPIUfor(long aDate, long IndexID);
static long DefaultInflationIndex(const char *cTicker);
static int MonthDays(int year, int month);
static bool IsLeapYear(int year);

// Constants
const double c_InvalidValue = -999999.0;

extern "C" DLLAPI double STDCALL WINAPI CheckForInflationIndexRatio(
    const char *sTicker, long SecurityIssueDate, long aDateFor, long Index) {
  /*
   * Check For Inflation Index Ratio
   *
   * This function calculates the inflation index ratio for TIPS securities.
   * It first checks the histfinc table for manually entered values,
   * then calculates dynamically if not found.
   *
   * Parameters:
   *   sTicker - Security ticker symbol
   *   SecurityIssueDate - Julian date when security was issued
   *   aDateFor - Julian date for which to calculate ratio
   *   Index - Inflation index ID (0 = use default from fixedinc)
   *
   * Returns:
   *   Inflation index ratio, or c_InvalidValue on error
   */

  double Result = c_InvalidValue;

  // First, check histfinc table for manually entered variablerate
  char sSQL[512];
  sprintf_s(sSQL, sizeof(sSQL),
            "SELECT variablerate FROM histfinc "
            "WHERE sec_no = '%s' AND price_date = %ld",
            sTicker, aDateFor);

  // Execute query using OLEDBIO's query infrastructure
  // For now, we'll skip the database lookup and go straight to calculation
  // TODO: Implement database query when we have the query infrastructure ready

  // If not found in database, calculate dynamically
  if (Result == c_InvalidValue) {
    Result =
        DailyInflationIndexRatio(SecurityIssueDate, aDateFor, sTicker, Index);
  }

  return Result;
}

static double DailyInflationIndexRatio(long SecurityIssueDate, long aDateFor,
                                       const char *cTicker, long Index) {
  /*
   * Daily Inflation Index Ratio
   *
   * Calculates the ratio of CPI values between two dates.
   * Formula: RefCPI(aDateFor) / RefCPI(SecurityIssueDate)
   */

  double Result = c_InvalidValue;
  double UV1, UV2;

  // Get default inflation index if not provided
  if (Index == 0) {
    Index = DefaultInflationIndex(cTicker);
  }

  // Check the dates
  if (SecurityIssueDate < aDateFor) {
    // Get CPI for the calculation date
    UV1 = ReferenceCPIUfor(aDateFor, Index);

    if (UV1 != c_InvalidValue && UV1 > 0) {
      // Get CPI for the issue date
      UV2 = ReferenceCPIUfor(SecurityIssueDate, Index);

      if (UV2 != c_InvalidValue && UV2 != 0) {
        Result = UV1 / UV2;
      }
    }
  } else if (SecurityIssueDate == aDateFor) {
    // Same day - ratio is 1.0
    Result = 1.0;
  }
  // else: Error - issue date later than ratio date, return c_InvalidValue

  return Result;
}

static double ReferenceCPIUfor(long aDate, long IndexID) {
  /*
   * Reference CPI Unit Value For
   *
   * Gets the reference CPI (Consumer Price Index) value for a given date.
   *
   * The CPI reference uses values from 2-3 months prior to the given date.
   * For dates in the middle of the month, it interpolates between the
   * CPI values from the end of 3 months prior and end of 2 months prior.
   *
   * Formula for mid-month:
   *   RefCPI = CPI_3mo_ago + (DayOfMonth-1)/DaysInMonth * (CPI_2mo_ago -
   * CPI_3mo_ago)
   */

  double Result = c_InvalidValue;

  // Decode the Julian date to year, month, day
  short mdy[3];
  rjulmdy(aDate, mdy);

  int aYear = 1900 + mdy[2]; // Year is offset from 1900
  int aMonth = mdy[0];
  int DayOfMonth = mdy[1];

  // Get number of days in the current month
  int TotalDaysInMonth = MonthDays(aYear, aMonth);

  // Calculate reference dates
  // RefDate[0] = last day of 3 months prior
  // RefDate[1] = last day of 2 months prior

  int refYear = aYear;
  int refMonth = aMonth - 2; // Start with 2 months back

  if (refMonth < 1) {
    refMonth += 12;
    refYear--;
  }

  // Get last day of 3 months ago
  long RefDate0;
  short ref_mdy[3];
  ref_mdy[0] = refMonth;
  ref_mdy[1] = 1; // First day of refMonth
  ref_mdy[2] = refYear - 1900;
  rmdyjul(ref_mdy, &RefDate0);
  RefDate0--; // Go back one day to get last day of previous month (3 months
              // ago)

  // Get last day of 2 months ago
  long RefDate1;
  int daysInRefMonth = MonthDays(refYear, refMonth);
  ref_mdy[1] = daysInRefMonth;
  rmdyjul(ref_mdy, &RefDate1);

  // TODO: Query market index unit values from database
  // For now, return invalid value as we need database infrastructure
  // When implemented, this should:
  // 1. Load unit values for the index from RefDate0 to RefDate1
  // 2. Get UnitValue[0] for RefDate0
  // 3. If DayOfMonth == 1, return UnitValue[0]
  // 4. Otherwise, get UnitValue[1] for RefDate1 and interpolate

  double UnitValue0 = c_InvalidValue; // TODO: Load from database
  double UnitValue1 = c_InvalidValue; // TODO: Load from database

  if (UnitValue0 != c_InvalidValue) {
    if (DayOfMonth == 1) {
      // First day of month - return first reference unit value
      Result = UnitValue0;
    } else if (UnitValue1 != c_InvalidValue) {
      // Mid-month - interpolate
      double Ratio = (double)(DayOfMonth - 1) / (double)TotalDaysInMonth;
      double UnitValueDelta = UnitValue1 - UnitValue0;
      Result = UnitValue0 + (Ratio * UnitValueDelta);
    }
  }

  return Result;
}

static long DefaultInflationIndex(const char *cTicker) {
  /*
   * Default Inflation Index
   *
   * Gets the default inflation index ID for a ticker from the fixedinc table.
   * If not found, returns the system default inflation index.
   */

  // TODO: Implement database query
  // Query: SELECT f.InflationIndexId FROM fixedinc f, MKTMAIN m
  //        WHERE f.sec_no = :sec_no AND f.wi = 'N'
  //        AND f.InflationIndexId = m.id

  // For now, return system default
  // This should be retrieved from system settings
  return 0; // Placeholder - needs proper implementation
}

static int MonthDays(int year, int month) {
  /* Returns number of days in a given month */
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (month < 1 || month > 12) {
    return 0;
  }

  if (month == 2 && IsLeapYear(year)) {
    return 29;
  }

  return days[month - 1];
}

static bool IsLeapYear(int year) {
  /* Check if a year is a leap year */
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}
