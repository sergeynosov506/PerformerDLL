/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Accdiv.h
 * 
 * DESCRIPTION: Accrued dividend query functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: Both functions use multi-row cursor pattern
 *        3 date field conversions: TrdDate, StlDate, EffDate
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#ifndef VALUATIONIO_ACCDIV_H
#define VALUATIONIO_ACCDIV_H

#include "OLEDBIOCommon.h"
#include "accdiv.h"  // For PARTACCDIV struct

// ============================================================================
// Accrued Dividend Functions
// ============================================================================

// Retrieve all accrued dividends for an account (multi-row iterator)
// Table: accdiv
// Complex WHERE: checks divint existence and date ranges
DLLAPI void STDCALL SelectAllAccdivForAnAccount(int iID, long lTrdDate, long lStlDate,
    PARTACCDIV *pzAccdiv, ERRSTRUCT *pzErr);

// Retrieve pending dividend transactions from trans table (multi-row iterator)
// Table: trans (accru dividend transactions with tran_type='RD')
DLLAPI void STDCALL SelectPendingAccdivTransForAnAccount(int iID, long lValDate,
    PARTACCDIV *pzAccdiv, ERRSTRUCT *pzErr);

#endif // VALUATIONIO_ACCDIV_H
