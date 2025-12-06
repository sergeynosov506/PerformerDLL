/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Utils.h
 * 
 * DESCRIPTION: Initialization and cleanup functions for ValuationIO module
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: With nanodbc RAII, all Prepare/Close/Free functions become no-ops
 *        Maintained for binary compatibility
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#ifndef VALUATIONIO_UTILS_H
#define VALUATIONIO_UTILS_H

#include "OLEDBIOCommon.h"

// ============================================================================
// Initialization and Cleanup Functions
// ============================================================================

// Initialize ValuationIO module
// With nanodbc RAII, this becomes a no-op for compatibility
// Previously prepared 34 global CCommand objects - now unnecessary
DLLAPI ERRSTRUCT STDCALL InitializeValuationIO();

// Free ValuationIO resources
// With nanodbc RAII, automatic cleanup via static state reset
// Previously called Unprepare/ReleaseCommand on 34+ objects
DLLAPI void STDCALL FreeValuationIO(void);

// Close ValuationIO (internal)
// Helper function for compatibility
void CloseValuationIO(void);

#endif // VALUATIONIO_UTILS_H
