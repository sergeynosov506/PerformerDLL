#pragma once

#include "commonheader.h"
#include "OLEDBIOCommon.h"
#include "fixedinc.h"

// Fixedinc related functions
DLLAPI void STDCALL SelectPartFixedinc(char* sSecNo, char* sWi, PARTFINC* pzPFinc, ERRSTRUCT* pzErr);
