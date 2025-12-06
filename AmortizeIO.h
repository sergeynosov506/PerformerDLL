/**
 * 
 * SUB-SYSTEM: Database Input/Output for Payments   
 * 
 * FILENAME: AmortizeIO.h
 * 
 * DESCRIPTION: Function prototypes for amortization and TIPS phantom income processing
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: All DLLAPI signatures preserved for binary compatibility
 *        Multi-row cursor pattern using static state
 *        Mode-based query routing (None/Account/Security)
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#ifndef AMORTIZEIO_H
#define AMORTIZEIO_H
#include "commonheader.h"
#include "OLEDBIOCommon.h"
#include "holdings.h"
#include "PaymentsIO.h"

// ============================================================================
// Cleanup Functions
// ============================================================================

// Close AmortizeIO resources (internal)
void CloseAmortizeIO(void);

// Free AmortizeIO resources
// With nanodbc RAII, maintained as no-op for compatibility
DLLAPI void STDCALL FreeAmortizeIO(void);

// ============================================================================
// Amortization Processing
// ============================================================================

// Update holdings with amortization data
// Updates orig_yield, cost_eff_mat_yld, eff_mat_date, eff_mat_price, tot_cost
DLLAPI void STDCALL UpdateAmortizeHoldings(HOLDINGS zHD, ERRSTRUCT *pzErr);

// Query amortization data with mode-based filtering
// Parameters:
//   sMode - Query mode: 'N'=None (all), 'A'=Account, 'S'=Security  
//   iID - Account ID
//   sSecNo, sWi, sSecXtend, sAcctType, lTransNo - Security filters (mode S)
//   pzAM - Output AMORTSTRUCT buffer
//   pzErr - Error structure
// 
// Iterative calling: Call repeatedly with same parameters to get all records
// Returns SQLNOTFOUND when complete
DLLAPI void STDCALL AmortizeUnload(char *sMode, int iID, char *sSecNo, char *sWi,
    char *sSecXtend, char *sAcctType, long lTransNo, AMORTSTRUCT *pzAM, 
    ERRSTRUCT *pzErr);

// ============================================================================
// TIPS Phantom Income Processing
// ============================================================================

// Query TIPS phantom income data with mode-based filtering
// TIPS (Treasury Inflation-Protected Securities) require phantom income tracking
// Parameters similar to AmortizeUnload but with valuation date
//   lValDate - Valuation date for TIPS calculation
DLLAPI void STDCALL PITIPSUnload(char *sMode, long lValDate, int iID, char *sSecNo,
    char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, 
    PITIPSSTRUCT *pzPI, ERRSTRUCT *pzErr);

#endif // AMORTIZEIO_H
