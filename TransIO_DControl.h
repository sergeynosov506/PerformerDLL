#pragma once
#include "dcontrol.h"
#include "OLEDBIOCommon.h"

// Dcontrol related functions
DLLAPI void STDCALL SelectDcontrol(DCONTROL *pzDC, long lRecDate, char *sCountry, ERRSTRUCT *pzErr);
