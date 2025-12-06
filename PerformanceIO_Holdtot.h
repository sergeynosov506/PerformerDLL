/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Holdtot.h
 * DESCRIPTION: Holdtot functions (2 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_HOLDTOT_H
#define PERFORMANCEIO_HOLDTOT_H

#include "OLEDBIOCommon.h"
#include "holdtot.h" // For HOLDTOT

// Delete Holdtot for an account
DLLAPI void STDCALL DeleteHoldtotForAnAccount(int iID, ERRSTRUCT *pzErr);

// Insert Holdtot record
DLLAPI void STDCALL InsertHoldtot(HOLDTOT zHT, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_HOLDTOT_H
