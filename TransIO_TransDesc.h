#pragma once
#include "commonheader.h"
#include "OLEDBIOCommon.h"
#include "trans.h"
#include "trndesc.h"

// TransDesc related functions
DLLAPI void STDCALL SelectTransDesc(TRNDESC *pzTransDesc, int iID, long lTransNo, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL InsertTransDesc(TRNDESC zTransDesc, ERRSTRUCT *pzErr); 
