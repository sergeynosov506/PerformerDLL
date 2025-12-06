/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Histpric.h
 * 
 * DESCRIPTION: Historical price query functions (histpric, histeqty, histfinc)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: All DLLAPI signatures preserved for binary compatibility
 *        Simple single-record queries (no multi-row cursors)
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#ifndef VALUATIONIO_HISTPRIC_H
#define VALUATIONIO_HISTPRIC_H

#include "OLEDBIOCommon.h"
#include "ValuationIO.h"  // For HISTPRIC, HISTFINC structs
#include "priceinfo.h"     // For PRICEINFO struct

// ============================================================================
// Historical Price Query Functions
// ============================================================================

// Query historical fixed income data for a security on a specific date
// Table: histfinc
DLLAPI void STDCALL SelectOneHistfinc(char *sSecNo, char *sWi, long lPriceDate,
    HISTFINC *pzHFinc, ERRSTRUCT *pzErr);

// Query historical equity data for a security on a specific date  
// Table: histeqty
DLLAPI void STDCALL SelectOneHisteqty(char *sSecNo, char *sWi, long lPriceDate,
    PRICEINFO *pzPInfo, ERRSTRUCT *pzErr);

// Query historical price data for a security on a specific date
// Table: histpric
DLLAPI void STDCALL SelectOneHistpric(char *sSecNo, char *sWi, long lPriceDate,
    HISTPRIC *pzHPric, ERRSTRUCT *pzErr);

// Helper function to initialize HISTPRIC struct
void InitializeHistPric(HISTPRIC *pzHP);

// Helper function to initialize PRICEINFO struct
void InitializePriceInfo(PRICEINFO *pzPInfo);

#endif // VALUATIONIO_HISTPRIC_H
