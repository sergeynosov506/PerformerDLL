#pragma once
#include "commonheader.h"
#include "unitvalu.h"


DLLAPI void STDCALL DeleteDailyUnitValueForADate(long iPortfolioID, long iID, long lPerformDate, long iRorType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL MarkPeriodUVForADateRangeAsDeleted(long iPortfolioID, long iID, long lPerformDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteMarkedUnitValue(long iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteUnitValueSince(long iPortfolioID, long iID, long lBeginDate, int iRorType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteUnitValueSince2(long iPortfolioID, long iID, long lBeginDate, int iRorType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteUnitValueForPortfolioSince(long iPortfolioID, long lBeginDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteUnitValueForPortfolioSince2(long iPortfolioID, long lBeginDate, long iBeginRorType, long iEndRorType, ERRSTRUCT *pzErr);
DLLAPI void STDCALL RecalcDailyUV(long iPortfolioID, long lStartDate, long lEndDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertUnitValue(UNITVALUE zUV, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectUnitValue(UNITVALUE *pzUV, int iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateUnitValue(UNITVALUE zUV, long lOldDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertUnitValueBatch(UNITVALUE* pzUV, long lBatchSize, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectOneUnitValue(UNITVALUE *pzUV, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectUnitValueRange(UNITVALUE *pzUV, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectUnitValueRange2(UNITVALUE *pzUV, int iPortfolioID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

