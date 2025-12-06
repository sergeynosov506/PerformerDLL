/**
 * 
 * SUB-SYSTEM: Database Input/Output for Payments   
 * 
 * FILENAME: MaturityIO.h
 * 
 * DESCRIPTION: Function prototypes for maturity and forward contract processing
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: All DLLAPI signatures preserved for binary compatibility
 *        Multi-row cursor pattern using static state
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#ifndef MATURITYIO_H
#define MATURITYIO_H

#include "OLEDBIOCommon.h"
#include "PaymentsIO.h"
#include "sectype.h"

// ============================================================================
// Initialization and Cleanup
// ============================================================================

// Initialize MaturityIO module
// With nanodbc RAII, this is maintained as no-op for compatibility
DLLAPI ERRSTRUCT STDCALL InitializeMaturityIO(char *sMode);

// Free MaturityIO resources
// With nanodbc RAII, automatic cleanup via static state reset
DLLAPI void STDCALL FreeMaturityIO(void);

// Close MaturityIO (internal)
void CloseMaturityIO(void);

// ============================================================================
// Maturity Processing
// ============================================================================

// Query maturity data with mode-based filtering
// Parameters:
//   pzMS - Output MATSTRUCT buffer
//   lStartDate - Start date range (long format YYYYMMDD)
//   lEndDate - End date range (long format YYYYMMDD)
//   sMode - Query mode: 'N'=None (all), 'A'=Account, 'S'=Security
//   iID - Account ID (used in modes A and S)
//   sSecNo - Security number (used in mode S)
//   sWi - When issued flag (used in mode S)
//   sSecXtend - Security extension (used in mode S)
//   sAcctType - Account type (used in mode S)
//   pzErr - Error structure
// 
// Iterative calling pattern: Call repeatedly with same parameters
// to retrieve all matching records. Returns SQLNOTFOUND when complete.
DLLAPI void STDCALL MaturityUnload(MATSTRUCT *pzMS, long lStartDate, long lEndDate, 
    char *sMode, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
    char *sAcctType, ERRSTRUCT *pzErr);

// ============================================================================
// Forward Contract Processing
// ============================================================================

// Query forward contract expiration data with mode-based filtering
// Parameters: Similar to MaturityUnload but uses FMATSTRUCT
// Supports table name substitution for holdings table
DLLAPI void STDCALL ForwardMaturityUnload(FMATSTRUCT *pzMS, long lStartDate, 
    long lEndDate, char *sMode, int iID, char *sSecNo, char *sWi, 
    char *sSecXtend, char *sAcctType, ERRSTRUCT *pzErr);

// ============================================================================
// Security Type Processing
// ============================================================================

// Query all security types (iterative)
// Call repeatedly to retrieve all security types
// Returns SQLNOTFOUND when complete
DLLAPI void STDCALL SelectAllSectypes(SECTYPE *pzST, ERRSTRUCT *pzErr);

#endif // MATURITYIO_H
