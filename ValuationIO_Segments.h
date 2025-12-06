/**
 * SUB-SYSTEM: Database Input/Output for Valuation
 * FILENAME: ValuationIO_Segments.h
 * DESCRIPTION: Segment hierarchy and mapping functions (11 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef VALUATIONIO_SEGMENTS_H
#define VALUATIONIO_SEGMENTS_H

#include "OLEDBIOCommon.h"
#include "segmain.h"  // For SEGMENTS, SEGMAP structs
#include "portmain.h"  // For PORTMAIN struct

// Multi-row iterators
DLLAPI void STDCALL SelectAllSegments(SEGMENTS *pzSegments, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectAllSegmap(SEGMAP *pzSegmap, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectSegment(SEGMENTS *pzSegment, ERRSTRUCT *pzErr);

// International/currency checking
DLLAPI BOOL STDCALL IsAssetInternational(int iID, char *sSecNo, char *sWi, int iLevel1, int *piLevel2, ERRSTRUCT *pzErr);

// Segment ID lookups (return int)
DLLAPI int STDCALL GetInterSegID(int iSegTypeID, ERRSTRUCT *pzErr);
DLLAPI int STDCALL SelectSegmentIdFromSegMap(int IndustLevel1, int IndustLevel2, int IndustLevel3, ERRSTRUCT *pzErr);
DLLAPI int STDCALL SelectSegmentLevelId(int SegmentTypeId, ERRSTRUCT *pzErr);
DLLAPI int STDCALL SelectUnsupervisedSegmentId(int iLevelId, int iSqnNbr, ERRSTRUCT *pzErr);
DLLAPI int STDCALL SelectPledgedSegment(int Level1, int Level3, ERRSTRUCT *pzErr);

// Aggregate queries
DLLAPI void STDCALL SelectUnsupervised(int iPortfolioId, char *sSecXtend, double *pfTotalMktValue, double *pfTotalCost, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectAdhocPortmain(PORTMAIN *pzPR, int iID, ERRSTRUCT *pzErr);

#endif // VALUATIONIO_SEGMENTS_H
