#pragma once

#include "priceinfo.h"
#include "OLEDBIOCommon.h"

// SecurityPrice related functions
DLLAPI void STDCALL GetSecurityPrice(char *sSecNo, char *sWi, long lPriceDate, PRICEINFO *pzPInfo, ERRSTRUCT *pzErr);
DLLAPI BOOL STDCALL IsItManualForSecurity(char *sSecNo, char *sWi, long lPriceDate, ERRSTRUCT *pzErr);
DLLAPI BOOL STDCALL IsManualClosingPrice(char *sSecNo, char *sWi, long lPriceDate, ERRSTRUCT *pzErr);
