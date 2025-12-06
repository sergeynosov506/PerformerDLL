/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Divhist.h
 * 
 * DESCRIPTION: Dividend history and units calculation functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: SelectAllDivhistForAnAccount uses multi-row cursor pattern
 *        SelectUnitsForASoldSecurity is aggregate query
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#ifndef VALUATIONIO_DIVHIST_H
#define VALUATIONIO_DIVHIST_H

#include "OLEDBIOCommon.h"
#include "divhist.h"  // For DIVHIST struct

// ============================================================================
// Dividend History Functions
// ============================================================================

// Retrieve all dividend history records for an account (multi-row iterator)
// Table: divhist
// Pattern: Multi-row cursor with parameter caching
DLLAPI void STDCALL SelectAllDivhistForAnAccount(int iID, DIVHIST *pzDivhist, ERRSTRUCT *pzErr);

// Calculate total units sold for a security within date range (aggregate query)
// Tables: trans, trantype, holddel
// Returns sum of units from closing transactions
DLLAPI void STDCALL SelectUnitsForASoldSecurity(int iID, char *sSecNo, char *sWi,
    char *sSecXtend, char *sAcctType, long lValDate, long lRecDate, long lPayDate, 
    double *pfUnits, ERRSTRUCT *pzErr);

#endif // VALUATIONIO_DIVHIST_H
