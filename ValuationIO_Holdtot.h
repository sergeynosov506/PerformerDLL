/**
 * SUB-SYSTEM: Database Input/Output for Valuation
 * FILENAME: ValuationIO_Holdtot.h  
 * DESCRIPTION: Holdtot and ratings query functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * NOTES: SelectAllForHoldTot has massive UNION with complex multi-table JOINs
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef VALUATIONIO_HOLDTOT_H
#define VALUATIONIO_HOLDTOT_H

#include "commonheader.h"
#include "OLEDBIOCommon.h"
#include "holdtot.h" // HOLDINGSASSETS

// ============================================================================
// Holdtot Functions
// ============================================================================

// Retrieve all holdings+assets data for portfolio (most complex query!)
// Tables: holdings, holdcash, assets, histpric, histfinc, histeqty, fixedinc, sectype, histassetindustry, segments
// UNION of holdings and holdcash with massive JOINs
//DLLAPI void STDCALL SelectAllForHoldTot(int iPortfolioID, long lDate, HOLDINGSASSETS *pzHoldingsAssets, ERRSTRUCT *pzErr);

// Retrieve all ratings (multi-row iterator)
// Table: ratings
//DLLAPI void STDCALL SelectAllRatings(RATING *pzRating, ERRSTRUCT *pzErr);

#endif // VALUATIONIO_HOLDTOT_H
