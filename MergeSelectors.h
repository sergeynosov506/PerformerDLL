/**
 * 
 * SUB-SYSTEM: Composite Merge Selectors
 * 
 * FILENAME: MergeSelectors.h
 * 
 * DESCRIPTION: Selector function prototypes for composite member and segment selection
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * PUBLIC FUNCTIONS: 
 *   - SelectAllMembersOfAComposite
 *   - SelectSegmainForPortfolio
 *   - SelectTransFor
 * 
 * NOTES: All DLLAPI signatures preserved for binary compatibility
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#ifndef MERGESELECTORS_H
#define MERGESELECTORS_H

#include "OLEDBIOCommon.h"
#include "segmain.h"
#include "trans.h"
#include "holdcash.h"
#include "holdings.h"
// Composite member selector - iterative calling pattern
// Call repeatedly to get all members, returns SQLNOTFOUND when complete
DLLAPI void STDCALL SelectAllMembersOfAComposite(int iOwnerID, long lDate, int *piID, ERRSTRUCT *pzErr);

// Segment selector for portfolio - iterative calling pattern  
// Call repeatedly to get all segments, returns SQLNOTFOUND when complete
DLLAPI void STDCALL SelectSegmainForPortfolio(SEGMAIN *pzSegmain, int iID, ERRSTRUCT *pzErr);

// Transaction selector - iterative calling pattern
// Call repeatedly to get all transactions in date range, returns SQLNOTFOUND when complete
DLLAPI void STDCALL SelectTransFor(TRANS *pzTR, int iID, long lEffDate1, long lEffDate2, ERRSTRUCT *pzErr);

// Helper functions for table forwarding (delegates to ValuationIO)
DLLAPI void STDCALL SelectHoldingsFor(int iID, HOLDINGS *pzHoldings, char *TableName, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectHoldcashFor(int iID, HOLDCASH *pzHoldcash, char *TableName, ERRSTRUCT *pzErr);

#endif // MERGESELECTORS_H
