/**
 * 
 * SUB-SYSTEM: Composite Merge Utilities
 * 
 * FILENAME: MergeUtils.h
 * 
 * DESCRIPTION: Cleanup and initialization function prototypes
 *              Modernized to C++20 + nanodbc RAII pattern
 * 
 * NOTES: With nanodbc RAII, explicit cleanup is not required
 *        Functions maintained for binary compatibility but simplified
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#ifndef MERGEUTILS_H
#define MERGEUTILS_H

#include "OLEDBIOCommon.h"

// Close all composite merge resources
// With nanodbc RAII, this is no longer required but maintained for binary compatibility
DLLAPI void STDCALL CloseCompositeMergeIO(void);

// Free all composite merge resources
// With nanodbc RAII, this is no longer required but maintained for binary compatibility
DLLAPI void STDCALL FreeCompositeMergeIO(void);

#endif // MERGEUTILS_H
