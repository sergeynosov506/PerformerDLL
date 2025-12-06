/**
* 
* SUB-SYSTEM: Database Input/Output for Performance   
* 
* FILENAME: PerformanceIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in Performance.DLL . 
*
* 
* PUBLIC FUNCTIONS(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDB.DLL project. 
*
* AUTHOR:	Valeriy Yegorov. (C) 2001 Effron Enterprises, Inc. 
*
*
**/

// History.
// 2013-01-14 VI# 51154 Added cleanup for orphaned unitvalue entries to be marked as deleted -mk
// 2010-07-14 VI# 44510 Improved daily deletions of unitvalues -mk
// 09/10/2001  Started.

#include "dailyflows.h"
#include "PerfAggregationMerge.h"
#include "perfctrl.h"
#include "perfkey.h"
#include "perfrule.h"
#include "pscrdet.h"
#include "pscrhdr.h"
#include "ptmpdet.h"
#include "ptmphdr.h"
#include "rtrnset.h"
#include "summdata.h"
#include "taxperf.h"
#include "unitvalu.h"



#include "PerformanceIO_SecTrans.h"
#include "PerformanceIO_Holdings.h"

DLLAPI ERRSTRUCT STDCALL InitializePerformanceIO(char *sHoldings, char *sHoldcash);
DLLAPI void STDCALL FreePerformanceIO(void);
void ClosePerformanceIO(void);

DLLAPI void STDCALL DeleteBankstat(int iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectAllPartCurrencies(PARTCURR *pzCurrency, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteDSumdata(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertDailySummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectDailySummdata(SUMMDATA *pzSD , int iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdateDailySummdata(SUMMDATA zSD, long lOldPerformDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteFFactorsForADateRange(int iID, long lStartDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteHkeyrltnForAnAccount(long lAsofDate, int iID, ERRSTRUCT *pzErr);

//DLLAPI void STDCALL InsertHkeyrltn(HKEYRLTN zHK, ERRSTRUCT *pzErr);



DLLAPI void STDCALL DeleteHoldtotForAnAccount(int iID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertHoldtot(HOLDTOT zHT, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteMonthSum(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertMonthlySummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectPerfctrl(PERFCTRL *pzPC, int iPortfolioID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdatePerfctrl(PERFCTRL zPC, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeletePerfkey(long lPerfKeyNo, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectPerfkeys(PERFKEY *pzPK, int iPortfolioID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdateNewPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdateOldPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectAllPerfrule(PERFRULE *pzPR, long iPortfolioID, long lDeleteDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectAllPorttax(int iId, long lTaxDate, PORTTAX *pzPTax, ERRSTRUCT *pzErr);



DLLAPI void STDCALL SelectAllPartSectype(PARTSTYPE *pzST, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectOneSegment(char *sSegment, int *piGroupID, int *piLevelID, int iSegmentID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL DeleteSummdata(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertPeriodSummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectPeriodSummdata(SUMMDATA *pzSD, int iPortfolioID, long lPerformDate1, long lPerformDate2, 
									ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdatePeriodSummdata(SUMMDATA zSD, ERRSTRUCT *pzErr);

DLLAPI void STDCALL MonthlyWeightedInfo(SUMMDATA *pzSData, RTRNSET *pzRor, long lPortfolioID,
								   long	lPerformDate, ERRSTRUCT *pzErr);

#include "PerformanceIO_Taxperf.h"





DLLAPI void STDCALL SelectSegmain(int iPortID, int iSegmentType, long *lSegmentID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertSegmain(SEGMAIN *pzSegmain, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectSegtree(int iPortID, int iSegmentType, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertSegtree(int iPortID, int iSegmentType, int iSegmainId, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertSegment(SEGMENTS *pzSegments, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectSegmap(int iSegmentLevel1ID, int iSegmentLevel2ID, int SegmentLevel3ID, 
														int *piSegmentID, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertSegmap(int iSegmentID, int iLevel1, int iLevel2, int iLevel3, ERRSTRUCT *pzErr);

DLLAPI int STDCALL SelectSecSegmap(char *sSecNo, char *sWi, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertSecSegmap(char *sSecNo, char *sWi, int iSegmentID, ERRSTRUCT *pzErr);

#include "PerformanceIO_UnitValue.h"

DLLAPI void STDCALL DeleteDSumdataForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteSummdataForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteMonthSumForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteTaxperfForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr);



DLLAPI void STDCALL SelectPerfAssetMerge(PERFASSETMERGE *pzPAM, ERRSTRUCT *pzErr);
