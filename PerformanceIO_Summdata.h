/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Summdata.h
 * DESCRIPTION: Summdata, Monthsum, and Bankstat functions (9 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_SUMMDATA_H
#define PERFORMANCEIO_SUMMDATA_H

#include "OLEDBIOCommon.h"
#include "summdata.h" // For SUMMDATA struct

// Delete from SUMMDATA
DLLAPI void STDCALL DeleteSummdata(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

// Delete from SUMMDATA for a Portfolio
DLLAPI void STDCALL DeleteSummdataForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

// Delete from MONTHSUM
DLLAPI void STDCALL DeleteMonthSum(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

// Delete from MONTHSUM for a Portfolio
DLLAPI void STDCALL DeleteMonthSumForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

// Delete from BANKSTAT
DLLAPI void STDCALL DeleteBankstat(int iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

// Insert into SUMMDATA
DLLAPI void STDCALL InsertPeriodSummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

// Insert into MONTHSUM
DLLAPI void STDCALL InsertMonthlySummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

// Select from SUMMDATA and DSUMDATA (Union)
DLLAPI void STDCALL SelectPeriodSummdata(SUMMDATA *pzSD, int iPortfolioID, long lPerformDate1, long lPerformDate2, ERRSTRUCT *pzErr);

// Update SUMMDATA
DLLAPI void STDCALL UpdatePeriodSummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_SUMMDATA_H
