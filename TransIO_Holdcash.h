#pragma once

#include "OLEDBIOCommon.h"
#include "holdcash.h"
#include "holdings.h"

// Holdcash related functions
DLLAPI void STDCALL DeleteHoldcash(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertHoldcash(HOLDCASH zHCR, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectHoldcash(HOLDCASH *pzHCR, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateHoldcash(HOLDCASH zHCR, ERRSTRUCT *pzErr);

DLLAPI void STDCALL HoldcashForFifoAndAvgAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL HoldcashForLifoAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL HoldcashForHighAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL HoldcashForLowAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL HoldcashForMinimumGainAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL HoldcashForMaximumGainAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate, ERRSTRUCT *pzErr);
