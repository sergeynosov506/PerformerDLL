#pragma once

#include "OLEDBIOCommon.h"
#include "holddel.h"

// Holddel related functions
DLLAPI void STDCALL InsertHolddel(HOLDDEL zHD, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectHolddel(HOLDDEL *pzHD, long lCreateTransNo, long lCreateDate, int iID, char *sSecNo, char *sWi,
                             char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr);
DLLAPI void STDCALL HolddelUpdate(int iID, long lRevTransNo, long lCreateTransNo, long lCreateDate, ERRSTRUCT *pzErr);
