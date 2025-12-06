#pragma once

#include "OLEDBIOCommon.h"
#include "holdings.h"
#include "assets.h"

DLLAPI void STDCALL SelectPerformanceHoldings(PARTHOLDING *pzHold, PARTASSET2 *pzPartAsset, LEVELINFO *pzLInfo,
																				 int iID, char *sHoldings, int iVendorID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectPerformanceHoldcash(PARTHOLDING *pzHold, PARTASSET2 *pzPartAsset, LEVELINFO	*pzLInfo,
																				int iID, char *sHoldcash, int iVendorID, ERRSTRUCT *pzErr);
