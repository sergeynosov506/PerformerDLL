/**
 * SUB-SYSTEM: Database Input/Output for Valuation
 * FILENAME: ValuationIO_Holdings.h
 * DESCRIPTION: Holdings, Holdcash, and Hedgxref query functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * NOTES: All functions use multi-row cursor or aggregate patterns
 *        Table name substitution for h oldcash and holdings
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef VALUATIONIO_HOLDINGS_H
#define VALUATIONIO_HOLDINGS_H

#include "OLEDBIOCommon.h"
#include "holdings.h"  // For HOLDINGS, HOLDCASH, HEDGEXREF structs
#include "commonheader.h"
#include "holdcash.h"
#include "hedgexref.h"

// External globals for table names
extern char sHoldings[STR80LEN];
extern char sHoldcash[STR80LEN];
extern char sHedgxref[STR80LEN];

// ============================================================================
// Holdings-Related Functions
// ============================================================================

// Retrieve all holdings for an account (multi-row iterator)
// Table: holdings (or alternate via sHoldings global)
// Complex: 59 columns, 10 date fields
DLLAPI void STDCALL SelectAllHoldingsForAnAccount(int iID, HOLDINGS *pzHoldings, ERRSTRUCT *pzErr);

// Calculate total units held for a security (aggregate query)
// Table: holdings (or alternate via sHoldings global)
DLLAPI void STDCALL SelectUnitsHeldForASecurity(int iID, char *sSecNo, char *sWi,
    char *sSecXtend, char *sAcctType, long lRecDate, long lPayDate, 
    double *pfUnits, ERRSTRUCT *pzErr);

// Retrieve all cash holdings for an account (multi-row iterator)
// Table: holdcash (or alternate via sHoldcash global)
DLLAPI void STDCALL SelectAllHoldcashForAnAccount(int iID, HOLDCASH *pzHoldcash, ERRSTRUCT *pzErr);

// Retrieve all hedge cross-references for an account (multi-row iterator)
// Table: hedgxref (or alternate via sHedgxref global)
DLLAPI void STDCALL SelectAllHedgxrefForAnAccount(int iID, HEDGEXREF *pzHedgxref, ERRSTRUCT *pzErr);

#endif // VALUATIONIO_HOLDINGS_H
