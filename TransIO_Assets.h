#pragma once

#include "OLEDBIOCommon.h"
#include "assets.h"

// Asset related functions
DLLAPI void STDCALL SelectAsset(ASSETS *pzAssets, char *sSecNo, char *sWhenIssue, int iVendorID, ERRSTRUCT *pzErr);
