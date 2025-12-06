/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_DSumdata.h
 * DESCRIPTION: Daily Summary Data functions (4 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_DSUMDATA_H
#define PERFORMANCEIO_DSUMDATA_H

#include "OLEDBIOCommon.h"
#include "summdata.h" // For SUMMDATA

// Delete Daily Summary Data
DLLAPI void STDCALL DeleteDSumdata(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

// Insert Daily Summary Data
DLLAPI void STDCALL InsertDailySummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

// Select Daily Summary Data (Multi-row cursor)
DLLAPI void STDCALL SelectDailySummdata(SUMMDATA *pzSD , int iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr);

// Update Daily Summary Data
DLLAPI void STDCALL UpdateDailySummdata(SUMMDATA zSD, long lOldPerformDate, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_DSUMDATA_H
