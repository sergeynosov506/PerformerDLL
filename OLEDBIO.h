// ============================================================================
// FILE: OLEDBIO.h
// DESC: Modern ODBC refactor export header (Delphi 7 / C++ compatible)
// DATE: 2025-11-09
// AUTHOR: Original Valeriy Yegorov (2001), Refactor Grok/xAI (2025)
// ============================================================================

#pragma once
#include <windows.h>
#include <cstdint>
#include "commonheader.h"


// ----------------------------------------------------------------------------
// Primary exported API (Delphi-compatible)
// ----------------------------------------------------------------------------
DLLAPI void     STDCALL InitializeOLEDBIO(char* sAlias, char* sMode, char* sType,
    long lAsofDate, int iPrepareWhat, ERRSTRUCT* pzErr);
//DLLAPI HRESULT  STDCALL InitializeOLEDBIOForVBA(char* sBDEAlias, char* sConnectionStr, bool bInitCom);
//DLLAPI HRESULT  STDCALL InitializeOLEDBIOForVBAEx(char* sBDEAlias, char* sServer, char* sDBName,
//    char* sUser, char* sPwd, bool bInitCom = false);
DLLAPI void     STDCALL FreeOLEDBIO(void);

DLLAPI void     STDCALL SetTransCount(long lCnt);
DLLAPI long     STDCALL GetTransCount(void);
DLLAPI int      STDCALL StartDBTransaction(void);
DLLAPI int      STDCALL CommitDBTransaction(void);
DLLAPI int      STDCALL RollbackDBTransaction(void);
DLLAPI int      STDCALL AbortDBTransaction(BOOL bInTrans);

DLLAPI void     STDCALL InitHoldmapCS(long lAsofDate, ERRSTRUCT* pzErr);
DLLAPI ERRSTRUCT STDCALL InitHoldmap(long lAsofDate);
//DLLAPI HRESULT  STDCALL HashTest(char* sHash, char* sData);


// ----------------------------------------------------------------------------
// Module initialization exports (stubs or implemented in modules)
// ----------------------------------------------------------------------------
//DLLAPI ERRSTRUCT STDCALL InitializeTransEngineIO(void);
//DLLAPI ERRSTRUCT STDCALL InitializeDivIntGenerateIO(char* sMode, char* sType);
//DLLAPI ERRSTRUCT STDCALL InitializeDivIntPayIO(char* sMode, char* sType);
//DLLAPI ERRSTRUCT STDCALL InitializeMaturityIO(char* sMode);
//DLLAPI ERRSTRUCT STDCALL InitializeValuationIO(void);
//DLLAPI ERRSTRUCT STDCALL InitializePerformanceIO(char* sHoldings, char* sHoldcash);

// ----------------------------------------------------------------------------
// Global state (declared extern; defined thread_local in .cpp)
// ----------------------------------------------------------------------------
//extern bool OLEDBIOInitialized;
extern long lTransCount;
extern bool bAssetIsValid;
extern bool bPMainIsValid;
extern char sDBAlias[256];
