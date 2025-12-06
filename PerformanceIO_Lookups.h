/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Lookups.h
 * DESCRIPTION: Lookup and reference table functions (5 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_LOOKUPS_H
#define PERFORMANCEIO_LOOKUPS_H

#include "OLEDBIOCommon.h"
#include "currency.h"  // For PARTCURR
#include "sectype.h"   // For PARTSTYPE
#include "segmain.h"   // For SEGMAIN

// Currency Lookups
DLLAPI void STDCALL SelectAllPartCurrencies(PARTCURR *pzCurrency, ERRSTRUCT *pzErr);

// Country Lookups
DLLAPI void STDCALL SelectAllCountries(COUNTRY *pzCountry, ERRSTRUCT *pzErr);

// Security Type Lookups
DLLAPI void STDCALL SelectAllPartSectype(PARTSTYPE *pzST, ERRSTRUCT *pzErr);

// Segment Lookups
DLLAPI void STDCALL SelectOneSegment(char *sSegment, int *piGroupID, int *piLevelID, int iSegmentID, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectSegmain(int iPortID, int iSegmentType, long *lSegmentID, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_LOOKUPS_H
