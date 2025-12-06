/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Perfkey.h
 * DESCRIPTION: Performance Key functions (5 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_PERFKEY_H
#define PERFORMANCEIO_PERFKEY_H

#include "OLEDBIOCommon.h"
#include "perfkey.h" // For PERFKEY

// Delete a performance key
DLLAPI void STDCALL DeletePerfkey(long lPerfKeyNo, ERRSTRUCT *pzErr);

// Insert a new performance key
DLLAPI void STDCALL InsertPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr);

// Select all performance keys for a portfolio (Multi-row cursor)
DLLAPI void STDCALL SelectPerfkeys(PERFKEY *pzPK, int iPortfolioID, ERRSTRUCT *pzErr);

// Update specific fields of a performance key (New version)
DLLAPI void STDCALL UpdateNewPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr);

// Update specific fields of a performance key (Old version)
DLLAPI void STDCALL UpdateOldPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_PERFKEY_H
