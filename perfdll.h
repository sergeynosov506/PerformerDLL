/*
*
* SUB-SYSTEM: Performance.dll
*
* FILENAME: perfdll.h
*
* DESCRIPTION: This file has ACTUAL GLOBAL variables.
*			   Calcperf.h now contains EXTERN definitions 
*			   to resolve linking 		
*
*
* AUTHOR: Valeriy Yegorov
*
*/
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2013-01-14 VI# 51154 Added cleanup for orphaned unitvalue entries to be marked as deleted -mk
// 2010-07-14 VI# 44510 Improved daily deletions of unitvalues -mk

#include "calcperf.h"

CAPICOM::IHashedDataPtr pHashData;

TRANTYPETABLE   zTTypeTable;
PARTSTYPETABLE  zPSTypeTable;
CURRENCYTABLE   zCurrTable;
COUNTRYTABLE	zCountryTable;
PSCRHDRDETTABLE g_zSHdrDetTable; 
PTMPHDRDETTABLE zTHdrDetTable;
SYSTEM_SETTINGS	zSysSet;
char			cCalcAccdiv;
//FILE			*fp;

/*
** These two Prime Number tables are used by the hash algorithm
** The PrimesForChars table is selected based on char position
** The PrimesForLines table is selected based on line number
*/
  
// functions from transengine dll
LPFN1PCHAR		lpfnSetErrorFileName;
LPPRERRSTRUCT	lpprInitializeErrStruct;
LPFNPRINTERROR	lpfnPrintError;
LPPRINITTRANS	lpprInitTrans;

// function from calcgainloss dll
LPFNCALCGAINLOSS	lpfnCalcGainLoss;

// functions from starsutils dll
LPFNRMDYJUL				lpfnrmdyjul;
LPFNRJULMDY				lpfnrjulmdy;
LPFN1INT				lpfnCurrentMonthEnd, lpfnLastMonthEnd, lpfnIsItAMonthEnd;
LPFN2VOID				lpfnCurrentDateAndTime;
LPFNISITAMARKETHOLIDAY	lpfnIsItAMarketHoliday;

// functions from oledbio dll
LPPR3PCHAR1LONG1INT						lpprOLEDBIOInit;
LPPRVOID								lpprFreeOLEDBIO;
LPFNVOID								lpfnCommitTransaction;
LPFNVOID								lpfnRollbackTransaction;
LPFNVOID								lpfnStartTransaction;
LPFNVOID								lpfnGetTransCount;
LPFN1BOOL								lpfnAbortTransaction;
//LPPRASSETS							lpprSelectAsset;
LPPRPARTASSET							lpprSelectPartAsset;
LPPRALLACCDIVFORANACCOUNT				lpprSelectAllAccdivForAnAccount;
//LPFNACCDIVTABLE						lpfnFindDividendInAccdiv;
LPPR3LONG								lpprDeleteBankstat;
LPPRSELECTALLPARTCURRENCIES				lpprSelectAllPartCurrencies;
LPPRSELECTALLCOUNTRIES					lpprSelectAllCountries;
LPPR1INT1LONG							lpprDeleteDailyFlows;
LPPR1INT1LONG							lpprDeleteDailyFlowsByID;
LPPRDAILYFLOWS							lpprInsertDailyFlows;
LPPR3LONG								lpprDeleteDSumdata, lpprDeleteDSumdataForSegment;
LPPRSUMMDATA							lpprInsertDailySummdata;
LPPRPSUMMDATA2LONG						lpprSelectDailySummdata;
LPPRSUMMDATA1LONG						lpprUpdateDailySummdata;
LPPRSELECTPERFORMANCEHOLDINGS			lpprSelectPerformanceHoldcash;
LPPRSELECTPERFORMANCEHOLDINGS			lpprSelectPerformanceHoldings;
LPPR8PCHAR1PLONG						lpprReadAllHoldmap;
LPPR3LONG								lpprDeleteMonthSum, lpprDeleteMonthSumForSegment;
LPPRSUMMDATA							lpprInsertMonthlySummdata;
LPPRPPERFCTRL1INT						lpprSelectPerfctrl;
LPPRPERFCTRL							lpprUpdatePerfctrl;
LPPR1LONG								lpprDeletePerfkey;
LPPRPERFKEY								lpprInsertPerfkey;
LPPRPPERFKEY1INT						lpprSelectPerfkeys;
LPPRPERFKEY								lpprUpdateNewPerfkey, lpprUpdateOldPerfkey;
LPPRSELECTALLPERFRULE					lpprSelectAllPerfrule;
LPPRSELECTONEPARTPORTMAIN				lpprSelectOnePartPortmain;
LPPRSELECTALLSCRIPTHEADERANDDETAILS		lpprSelectAllScriptHeaderAndDetails;
LPPRPERFSCRIPTHEADER					lpprInsertPerfscriptHeader, lpprUpdatePerfscriptHeader;
LPPRPERFSCRIPTDETAIL					lpprInsertPerfscriptDetail;
LPPRPORTTAX								lpprSelectAllPorttax;
LPPRSELECTALLPARTSECTYPE				lpprSelectAllPartSectype;
LPPR2PLONG								lpprSelectStarsDate;
LPPR2PLONG								lpprSelectCFStartDate;
LPPR3LONG								lpprDeleteSummdata, lpprDeleteSummdataForSegment;
LPPRSUMMDATA							lpprInsertPeriodSummdata;
LPPRPSUMMDATA3LONG						lpprSelectPeriodSummdata;  
LPPRSUMMDATA							lpprUpdatePeriodSummdata;
LPPR1PCHAR2PINT1INT						lpprSelectOneSegment;
LPPRSELECTSYSSETTINGS					lpprSelectSyssetng;
LPPRSELECTSYSVALUES						lpprSelectSysvalues;
LPPR3LONG								lpprDeleteTaxperf, lpprDeleteTaxperfForSegment;
LPPRTAXPERF								lpprInsertTaxperf;
LPPRSELECTTAXPERF						lpprSelectTaxperf;		
LPPRSELECTALLTEMPLATEDETAILS			lpprSelectAllTemplateHeaderAndDetails;
LPPRSELECTPERFORMANCETRANSACTION		lpprSelectPerformanceTransaction;
LPPRSELECTALLPARTTRANTYPE				lpprSelectAllPartTrantype;
LPFNSELECTSEGMENTIDFROMSEGMAP			lpfnSelectSegmentIDFromSegmap;
LPPRSELECTSEGMAIN						lpprSelectSegmain;
LPPRINSERTSEGMAIN						lpprInsertSegmain;
LPPRSELECTSEGTREE						lpprSelectSegtree;
LPPRINSERTSEGTREE						lpprInsertSegtree;
LPPRSELECTSEGMENT						lpprSelectSegment;
LPPRINSERTSEGMENT						lpprInsertSegment;
LPFNSELECTSEGMAP						lpfnSelectSegmap;
LPPRINSERTSEGMAP						lpprInsertSegmap;
LPFN2PCHAR								lpfnSelectSecSegmap;
LPPRINSERTSECSEGMAP						lpprInsertSecSegmap;
LPPRSELECTUNITVALUE						lpprSelectUnitValue;
LPPRSELECTUNITVALUERANGE2				lpprSelectUnitValueRange2;
LPPRINSERTUNITVALUE						lpprInsertUnitValue;
LPPRINSERTUNITVALUEBATCH				lpprInsertUnitValueBatch;
LPPRUPDATEUNITVALUE						lpprUpdateUnitValue;
LPPR4LONG								lpprDeleteDailyUnitValueForADate;
LPPR4LONG								lpprDeleteUnitValueSince2;
LPPR3LONG								lpprMarkPeriodUVForADateRangeAsDeleted;
LPPR2LONG								lpprDeleteMarkedUnitValue;
LPPR3LONG								lpprRecalcDailyUV;
LPPRSELECTACTIVEPERFRETURNTYPE			lpprSelectActivePerfReturnType;
LPPRPERFASSETMERGE						lpprSelectPerfAssetMerge;

// functions from calcflow dll
LPFNCALCFLOW	lpfnCalcNetFlow;
LPFN1PCHAR		lpfnInitCalcFlow;

// functions from roll dll
LPFN3PCHAR1LONG1PCHAR	lpfnInitRoll;
LPFNROLL				lpfnRoll;

//functions from valuation dll
LPFN1LONG2PCHAR				lpfnInitValuation;
LPFN1PCHAR1INT1LONG1BOOL	lpfnValuation;

//functions from timer dll
//LP2PR1PCHAR	lpprTimerResult;
//LPFN1INT		lpfnTimer;
