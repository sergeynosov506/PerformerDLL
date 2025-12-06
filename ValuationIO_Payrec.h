/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Payrec.h
 * 
 * DESCRIPTION: Payment record (payrec) query and update functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: SelectAllPayrecForAnAccount uses multi-row cursor
 *        UpdatePayrec and UpdatePortmainValDate are UPDATE operations
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#ifndef VALUATIONIO_PAYREC_H
#define VALUATIONIO_PAYREC_H

#include "OLEDBIOCommon.h"
#include "PaymentsIO.h"  // For PAYREC struct
#include "portmain.h"  // For PORTMAIN struct
#include "payrec.h"

// ============================================================================
// Payment Record Functions
// ============================================================================

// Retrieve all payment records for an account (multi-row iterator)
// Table: payrec (or alternate table via sPayrec global)
DLLAPI void STDCALL SelectAllPayrecForAnAccount(int iID, PAYREC *pzPayrec, ERRSTRUCT *pzErr);

// Update payment record
// Table: payrec (or alternate table via sPayrec global)
DLLAPI void STDCALL UpdatePayrec(PAYREC zPayrec, ERRSTRUCT *pzErr);

// Update portfolio valuation date
// Table: portmain
DLLAPI void STDCALL UpdatePortmainValDate(int iID, long lValDate, ERRSTRUCT *pzErr);

#endif // VALUATIONIO_PAYREC_H
