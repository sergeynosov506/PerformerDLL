#pragma once

#include "OLEDBIOCommon.h"
#include "portmain.h"

// Portmain related functions
DLLAPI void STDCALL UpdatePortmainLastTransNo(long lLastTransNo, int iID, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL SelectPortmain(PORTMAIN *pzPR, int iID, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL SelectPortmainByUniqueName(PORTMAIN *pzPR, char *sName, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL SelectAcctMethod(char *sAcctMethod, int iID, ERRSTRUCT *pzErr); 
