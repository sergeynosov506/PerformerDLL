#pragma once

#include "OLEDBIOCommon.h"
#include "holdings.h"

// Holdings related functions
DLLAPI void STDCALL DeleteHoldings(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, 
							  long lTransNo, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL InsertHoldings(HOLDINGS zHR, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL InsertHoldingsBatch(HOLDINGS* pzHR, long lBatchSize, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectHoldings(HOLDINGS *pzHR, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
                              long lTransNo, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL UpdateHoldings(HOLDINGS zHR, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsForFifoAndAvgAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                          char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsForLifoAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                    char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsForHighAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                    char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsForLowAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                    char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsForMinimumGainAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
										   char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate,
										   ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsForMaximumGainAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
										   char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate,
										   ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsSumSelect(double *pfTotUnits, double *pfTotTotCost, double *pfTotOrigCost, double *pfTotBaseTCost, 
								 double *pfTotSysTCost, double *pfTotOpenLiability, double *pfTotBaseOLiability, double *pfTotSysOLiability,
								 int iID, char *sSecNo, char *sWi, char *sSecXtend, 
								 char *sAcctType, long lInvalidDate, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL HoldingsSumUpdate(double fTotUnitCost, double fTradingUnits, double fTotOrigUnit, double fUnitLiability,
								 double fNewBaseXrate, double fNewSysXrate, char *sTranType, long lTaxlotNo, 
								 char *sTransSrce, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
								 long lInvalidDate, ERRSTRUCT *pzErr); 
