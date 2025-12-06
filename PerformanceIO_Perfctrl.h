/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Perfctrl.h
 * DESCRIPTION: Performance Control record functions (2 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_PERFCTRL_H
#define PERFORMANCEIO_PERFCTRL_H

#include "OLEDBIOCommon.h"
#include "perfctrl.h" // For PERFCTRL

// Select Performance Control record for a portfolio
DLLAPI void STDCALL SelectPerfctrl(PERFCTRL *pzPC, int iPortfolioID, ERRSTRUCT *pzErr);

// Update Performance Control record
DLLAPI void STDCALL UpdatePerfctrl(PERFCTRL zPC, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_PERFCTRL_H
