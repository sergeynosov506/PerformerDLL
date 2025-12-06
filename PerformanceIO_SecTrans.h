#pragma once

#include "OLEDBIOCommon.h"
#include "dailyflows.h"
#include "trans.h"
#include "assets.h"
#include "trantype.h"

DLLAPI void STDCALL SelectPerformanceTransaction(PARTTRANS *pzTR, PARTASSET *pzPartAsset, LEVELINFO *pzLevels, int iID, 
											long lStartPerfDate, long lEndPerfDate, int iVendorID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectAllPartTrantype(PARTTRANTYPE *pzPTType, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteDailyFlows(int iPortfolioID, long lBeginDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertDailyFlows(DAILYFLOWS zDF, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteDailyFlowsByID(int iSegmainID, long lBeginDate, ERRSTRUCT *pzErr);
