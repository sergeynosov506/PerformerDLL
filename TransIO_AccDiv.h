#pragma once
#include "accdiv.h"
#include "OLEDBIOCommon.h"

// Accdiv related functions
DLLAPI void STDCALL DeleteAccdivOneLot(int iID, long lTransNo, long lDivintNo, ERRSTRUCT* pzErr);
DLLAPI void STDCALL DeleteAccruingAccdivOneLot(ACCDIV* pzAccdiv, ERRSTRUCT* pzErr);
DLLAPI void STDCALL DeleteAccdivAllLots(int iID, long lDivintNo, ERRSTRUCT* pzErr);
