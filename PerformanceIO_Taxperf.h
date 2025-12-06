#pragma once
#include "PerformanceIO_Common.h"
#include "taxperf.h"

#ifdef __cplusplus
extern "C" {
#endif

DLLAPI void STDCALL DeleteTaxperf(int iPortfolioID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertTaxperf(TAXPERF zTP, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectTaxperf(int iPortfolioID, long lPerformDate, TAXPERF *pzTP, ERRSTRUCT *pzErr);

#ifdef __cplusplus
}
#endif
