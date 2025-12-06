/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Scripts.h
 * DESCRIPTION: Performance Script and Template functions (5 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#ifndef PERFORMANCEIO_SCRIPTS_H
#define PERFORMANCEIO_SCRIPTS_H

#include "OLEDBIOCommon.h"
#include "pscrdet.h"
#include "pscrhdr.h"
#include "ptmpdet.h"
#include "ptmphdr.h"

// Insert Performance Script Detail
DLLAPI void STDCALL InsertPerfscriptDetail(PSCRDET zPSDetail, ERRSTRUCT *pzErr);

// Insert Performance Script Header
DLLAPI void STDCALL InsertPerfscriptHeader(PSCRHDR zPSHeader, ERRSTRUCT *pzErr);

// Update Performance Script Header
DLLAPI void STDCALL UpdatePerfscriptHeader(PSCRHDR zPSHeader, ERRSTRUCT *pzErr);

// Select All Script Headers and Details
DLLAPI void STDCALL SelectAllScriptHeaderAndDetails(PSCRHDR *pzPSHeader, PSCRDET *pzPSDetail, ERRSTRUCT *pzErr);

// Select All Template Headers and Details
DLLAPI void STDCALL SelectAllTemplateHeaderAndDetails(PTMPHDR *pzPTHeader, PTMPDET *pzPTDetail, ERRSTRUCT *pzErr);

#endif // PERFORMANCEIO_SCRIPTS_H
