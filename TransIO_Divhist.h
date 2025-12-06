#pragma once

#include "OLEDBIOCommon.h"

// Divhist related functions
DLLAPI void STDCALL DeleteDivhistOneLot(int iID, long lTransNo, long lDivintNo, long lDivTransNo, char *sTranType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteAccruingDivhistOneLot(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteDivhistAllLots(int iID, long lDivintNo, long lDivTransNo, char *sTranType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateDivhistOneLot(char *sNewTType, char *sTranLocation, long lDivTransNo, int iID, long lTransNo, long lDivintNo, char *sOldTType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateDivhistAllLots(char *sNewTType, char *sTranLocation, long lDivTransNo, int iID, long lDivintNo, char *sOldTType, ERRSTRUCT *pzErr);
