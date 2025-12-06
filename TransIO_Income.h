#pragma once

#include "OLEDBIOCommon.h"

DLLAPI void STDCALL GetLastIncomeDate(int iID, long lTaxlotNo, long lCurrentDate, long *lLastIncDate, int *piCount, ERRSTRUCT *pzErr);
DLLAPI void STDCALL GetLastIncomeDateMIPS(int iID, long lTaxlotNo, long lCurrentDate, long *lLastIncDate, int *piCount, ERRSTRUCT *pzErr);
DLLAPI void STDCALL GetIncomeForThePeriod(int iID, long lTaxlotNo, long lBeginDate, long lEndDate, 
                                  char *sTranType, char *sDrCr, long *lCashImpact, long *lSecImpact,
                                  double *fIncAmount, double *fIncUnits, ERRSTRUCT *pzErr);
