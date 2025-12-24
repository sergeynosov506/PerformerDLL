/*
 *
 * SUB-SYSTEM: calcperf
 *
 * FILENAME: calcperf.h
 *
 * DESCRIPTION:
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR: Shobhit Barman
 * 2024-03-04 J#PER13058 Fix issue w/duplicate asset on prior change - mk.
 * 2024-02-12 J#PER12987 Undid change - mk.
 * 2024-01-02 J#PER12987 Added logic to load historical industry levels for
 * continuing history assets - mk. 2023-10-13 J#PER12923 Modified logic to
 * exclude standard FI accruals when flag is M - mk. 2023-04-08 J#PER11602
 * Delete perf date when inception date moves within a month - mk. 2021-03-11 J#
 * PER 11415 Restored changes based on new FeesOut logic - mk. 2021-03-03 J#
 * PER-11415 Rolled back changes - mk 2021-02-26 J# PER-11415 Adjustments for
 * feesout based on perf.dll and reporting - mk. 2020-10-14 J# PER-11169
 * Controlled calculation of NCF returns -mk. 2020-04-17 J# PER-10655 Added CN
 * transactions -mk. 2013-04-12 VI# 52068 Fixed sideeffect w/Calcselected -mk
 * 2013-01-14 VI# 51154 Added cleanup for orphaned unitvalue entries to be
 * marked as deleted -mk 2011-10-02 VI# 46694 More fixed for UV/summary values
 * -mk 2010-07-14 VI# 44510 Improved daily deletions of unitvalues -mk
 * 2010-06-30 VI# 44433 Added calculation of estimated annual income -mk
 * 2010-06-16 VI# 42903 Added TodayFeesOut into DAILYINFO - sergeyn
 * 2009-04-22 VI#42310: Changed call to GetSyssettings - mk
 * 2009-04-21 VI#42310: Exposed GetSysSettings - mk.
 * 2007-11-08 Added Vendor id    - yb
 */

#define PERFHASH 1 /* Needed for ../include/nbcHashPerf.h file */

#ifndef CALCPERFH
#define CALCPERFH 1
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "PerfAggregationMerge.h"
#include "accdiv.h"
#include "assets.h"
#include "commonheader.h"
#include "currency.h"
#include "dailyflows.h"
#include "holdcash.h"
#include "holdings.h"
#include "perfctrl.h"
#include "perfkey.h"
#include "perfrule.h"
#include "portmain.h"
#include "porttax.h"
#include "pscrdet.h"
#include "pscrhdr.h"
#include "ptmpdet.h"
#include "ptmphdr.h"
#include "rtrnset.h"
#include "sectype.h"
#include "segmain.h"
#include "summdata.h"
#include "syssettings.h"
#include "taxperf.h"
#include "trans.h"
#include "trantype.h"
#include "unitvalu.h"

//	#import "c:\effron\capicom.dll"
#include "capicom.tlh"

#define EXTRAPKEY 10 /* number of keys(in perfkey table) created at a time*/
#define EXTRAACCDIV 25
#define EXTRAASSET 25 /* number of asset table records created at a time */
// #define	EXTRADAILYAINFO		1 // number of daily asset info record
// (for an asset) created at a time
#define EXTRAHOLD 25     /* number of holding table records created at a time */
#define EXTRATRANS 50    /* number of trans table records created at a time */
#define EXTRASHDRDET 100 /* script header detail records */
#define EXTRAPRULE 10    /* rule records */
#define EXTRATHDRDET 10  /* template header detail records */
#define EXTRAPORTTAX 5   /* porttax records */
#define EXTRASEGMAP 50
#define EXTRASEGMENTS 50
#define EXTRAVALIDDPR 5
#define EXTRAWTDDINFO 5
#define EXTRARESULTITEM 5
#define EXTRAPERFASSETMERGE 5
#define MAXITERATIONS 10000 /*max iterations for DWROR calculation */
#define NAVALUE -999.00     /* NA Value for return */
#define NA_MV -1e+308       /* NA Value for market value, flow, etc */
#define SRESULT_LONG 1
#define SRESULT_SHORT 2
#define ARESULT_LONG 4
#define ARESULT_SHORT 8
#define TOTALSEGMENTTYPE 1 // Match to segmenttypeid for TOTAL in Mercury
// Following Values Match to groupid (in Segments table) in Mercury
#define EQUITYSEGMENT 102 // Equity
#define FIXEDSEGMENT 103
#define CASHSEGMENT 104
#define EQUITYPLUSCASHSEGMENT 112 // Equity and Equity Cash
#define FIXEDPLUSCASHSEGMENT 113
#define EQUITYCASHSEGMENT                                                      \
  122 // Not part of equity or cash but cash flow goes to Equity Cash e.g.
      // Preferred Stocks
#define FIXEDCASHSEGMENT 123
#define DRDELIGPERCENTAGE 0.3 // For DRD calculation

#define MAXAlphaValueLEN 30 + NT
/*
** BIGPRIMENUMBER is a prime number to limit the output to a long
** The largest signed long is 2147483647
*/
#define BIGPRIMENUMBER 2147483629
#define DB_E_PKVIOLATION -2147217873

typedef struct {
  int iNumRorType;
  double fBaseRorIdx[NUMRORTYPE_ALL];
  // RTRNSET	zIndex[NUMRORTYPE_ALL];
  UNITVALUE zUVIndex[NUMRORTYPE_ALL];
} ALLRORS;

typedef struct {
  int iCapacity;
  int iCount;
  ALLRORS *pzRor;
} RORTABLE;

typedef struct DAILYINFO {
  long lDate;
  double fMktVal;
  double fBookValue;
  double fAccrInc;
  double fAccrDiv;
  double fNetFlow;
  double fTodayFlow;
  double fCumFlow;
  double fWtdFlow;
  double fIncome;
  double fTodayIncome;
  double fCumIncome;
  double fWtdInc;
  double fFees;
  double fTodayFees;
  double fCumFees;
  double fWtdFees;
  double fCNFees;
  double fTodayCNFees;
  double fCumCNFees;
  double fWtdCNFees;
  double fExchRateBase;
  double fPurchases;
  double fSales;
  double fContributions;
  double fWithdrawals;
  long lDaysSinceNond;
  double fFeesOut;
  double fTodayFeesOut;
  double fCumFeesOut;
  double fWtdFeesOut;
  double fNotionalFlow;
  BOOL bPeriodEnd; // is it a sub-period end(10 % flow date or period end)
  BOOL bGotMV;
  double fEstAnnIncome;
} DAILYINFO;

typedef struct {
  long lPerformNo;
  double fFedinctaxWthld;
  double fTdyFedinctaxWthld;
  double fCumFedinctaxWthld;
  double fWtdFedinctaxWthld;
  /* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double	fStinctaxWthld;
  double	fTdyStinctaxWthld;
  double	fCumStinctaxWthld;
  double	fWtdStinctaxWthld;
  */
  double fFedtaxRclm;
  double fTdyFedtaxRclm;
  double fCumFedtaxRclm;
  double fWtdFedtaxRclm;
  /* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double	fSttaxRclm;
  double	fTdySttaxRclm;
  double	fCumSttaxRclm;
  double	fWtdSttaxRclm;
  */
  double fFedetaxInc;
  double fTdyFedetaxInc;
  double fCumFedetaxInc;
  double fWtdFedetaxInc;
  double fFedataxInc;
  double fTdyFedataxInc;
  double fCumFedataxInc;
  double fWtdFedataxInc;
  /* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double	fStetaxInc;
  double	fTdyStetaxInc;
  double	fCumStetaxInc;
  double	fWtdStetaxInc;
  double	fStataxInc;
  double	fTdyStataxInc;
  double	fCumStataxInc;
  double	fWtdStataxInc;
  */
  double fFedetaxStrgl;
  double fTdyFedetaxStrgl;
  double fCumFedetaxStrgl;
  double fWtdFedetaxStrgl;
  double fFedetaxLtrgl;
  double fTdyFedetaxLtrgl;
  double fCumFedetaxLtrgl;
  double fWtdFedetaxLtrgl;
  double fFedetaxCrrgl;
  double fTdyFedetaxCrrgl;
  double fCumFedetaxCrrgl;
  double fWtdFedetaxCrrgl;
  double fFedataxStrgl;
  double fTdyFedataxStrgl;
  double fCumFedataxStrgl;
  double fWtdFedataxStrgl;
  double fFedataxLtrgl;
  double fTdyFedataxLtrgl;
  double fCumFedataxLtrgl;
  double fWtdFedataxLtrgl;
  double fFedataxCrrgl;
  double fTdyFedataxCrrgl;
  double fCumFedataxCrrgl;
  double fWtdFedataxCrrgl;
  /* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double	fStetaxStrgl;
  double	fTdyStetaxStrgl;
  double	fCumStetaxStrgl;
  double	fWtdStetaxStrgl;
  double	fStetaxLtrgl;
  double	fTdyStetaxLtrgl;
  double	fCumStetaxLtrgl;
  double	fWtdStetaxLtrgl;
  double	fStetaxCrrgl;
  double	fTdyStetaxCrrgl;
  double	fCumStetaxCrrgl;
  double	fWtdStetaxCrrgl;
  double	fStataxStrgl;
  double	fTdyStataxStrgl;
  double	fCumStataxStrgl;
  double	fWtdStataxStrgl;
  double	fStataxLtrgl;
  double	fTdyStataxLtrgl;
  double	fCumStataxLtrgl;
  double	fWtdStataxLtrgl;
  double	fStataxCrrgl;
  double	fTdyStataxCrrgl;
  double	fCumStataxCrrgl;
  double	fWtdStataxCrrgl;
  */
  double fFedataxAccrInc;
  double fFedataxAccrDiv;
  double fFedataxIncRclm;
  double fFedataxDivRclm;
  double fFedetaxAccrInc;
  double fFedetaxAccrDiv;
  double fFedetaxIncRclm;
  double fFedetaxDivRclm;
  /* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double	fStataxAccrInc;
  double	fStataxAccrDiv;
  double	fStataxIncRclm;
  double	fStataxDivRclm;
  double	fStetaxAccrInc;
  double	fStetaxAccrDiv;
  double	fStetaxIncRclm;
  double	fStetaxDivRclm;
  */
  double fExchRateBase;
  double fExchRateSys;

  double fFedataxAmort;
  double fTdyFedataxAmort;
  double fCumFedataxAmort;
  double fWtdFedataxAmort;

  double fFedetaxAmort;
  double fTdyFedetaxAmort;
  double fCumFedetaxAmort;
  double fWtdFedetaxAmort;
} DAILYTAXINFO;

typedef struct {
  long lPerformNo;
  long lDate;
  double fMktVal;
  double fAccrInc;
  double fAccrDiv;
  double fNetFlow;
  double fWtdFlow;
  double fIncome;
  double fWtdInc;
  double fFees;
  double fWtdFees;
  double fFeesOut;
  double fWtdFeesOut;
  double fCNFees;
  double fWtdCNFees;
  long lDaysSinceNond;
  double fGBaseRor;
  double fGPrincipalRor;
  double fGIncomeRor;
  double fNBaseRor;
  double fNPrincipalRor;
  double fNIncomeRor;
  BOOL bPerformUsed;
} WTDDAILYINFO;

typedef struct {
  double fFedinctaxWthld;
  double fWtdFedinctaxWthld;
  double fFedtaxRclm;
  double fWtdFedtaxRclm;
  double fFedetaxInc;
  double fWtdFedetaxInc;
  double fFedataxInc;
  double fWtdFedataxInc;
  double fFedetaxStrgl;
  double fWtdFedetaxStrgl;
  double fFedetaxLtrgl;
  double fWtdFedetaxLtrgl;
  double fFedetaxCrrgl;
  double fWtdFedetaxCrrgl;
  double fFedataxStrgl;
  double fWtdFedataxStrgl;
  double fFedataxLtrgl;
  double fWtdFedataxLtrgl;
  double fFedataxCrrgl;
  double fWtdFedataxCrrgl;
  double fFedBegataxAccrInc;
  double fFedBegataxAccrDiv;
  double fFedEndataxAccrInc;
  double fFedEndataxAccrDiv;
  double fFedataxIncRclm;
  double fFedataxDivRclm;
  double fFedBegetaxAccrInc;
  double fFedBegetaxAccrDiv;
  double fFedEndetaxAccrInc;
  double fFedEndetaxAccrDiv;
  double fFedetaxIncRclm;
  double fFedetaxDivRclm;

  double fFedetaxAmort;
  double fWtdFedetaxAmort;
  double fFedataxAmort;
  double fWtdFedataxAmort;

  /* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double	fStinctaxWthld;
  double	fWtdStinctaxWthld;
  double	fSttaxRclm;
  double	fWtdSttaxRclm;
  double	fStetaxInc;
  double	fWtdStetaxInc;
  double	fStataxInc;
  double	fWtdStataxInc;
  double	fStetaxStrgl;
  double	fWtdStetaxStrgl;
  double	fStetaxLtrgl;
  double	fWtdStetaxLtrgl;
  double	fStetaxCrrgl;
  double	fWtdStetaxCrrgl;
  double	fStataxStrgl;
  double	fWtdStataxStrgl;
  double	fStataxLtrgl;
  double	fWtdStataxLtrgl;
  double	fStataxCrrgl;
  double	fWtdStataxCrrgl;
  double	fStBegataxAccrInc;
  double	fStBegataxAccrDiv;
  double	fStEndataxAccrInc;
  double	fStEndataxAccrDiv;
  double	fStataxIncRclm;
  double	fStataxDivRclm;
  double	fStBegetaxAccrInc;
  double	fStBegetaxAccrDiv;
  double	fStEndetaxAccrInc;
  double	fStEndetaxAccrDiv;
  double	fStetaxIncRclm;
  double	fStetaxDivRclm;
  */
} TAXINFO;

typedef struct {
  double fBeginMV;  // begin market value
  double fBeginAI;  // begin accrued interest
  double fBeginAD;  // begin accrued dividend
  double fBeginInc; // begin income
  double fEndMV;
  double fEndAI;
  double fEndAD;
  double fEndInc;
  double fNetFlow;
  double fWtFlow;
  double fFees;
  double fWtFees;
  double fCNFees;
  double fWtCNFees;
  double fIncome;
  double fWtIncome;
  double fFeesOut;
  double fWtFeesOut;

  TAXINFO zTInfo; // Tax adjustments - in case tax related RORs need to be
                  // calculated
  PORTTAX zPTax;  // Tax rates - in case tax related RORs need to be calculated

  double fGFTWFFactor;   // Fudge factor for gross of fee time weighted ror
  double fGFPcplFFactor; // Fudge factor for gross of fee principal ror
  double fNFTWFFactor;   // Fudge factor for net of fee time weighted ror
  double fCNTWFFactor;   // Fudge factor for net of consulting fee time weighted
                         // ror, type = 14
  double fNFPcplFFactor; // Fudge factor for net of fee principal ror
  double fIncFFactor;    // Fudge factor for income ror
  double fGFTEFFactor;   // Fudge factor for gross of fee Tax Equivalent ror
  double fNFTEFFactor;   // Fudge factor for net of fee Tax Equivalent ror
  double fGFATFFactor;   // Fudge factor for gross of fee After Tax ror
  double fNFATFFactor;   // Fudge factor for net of fee After Tax ror

  // Following fields are used only by calculated weighted flow function
  double fGFTWWtdFlow; // Calculated Wtd Flow for gross of fee time weighted ror
  double fGFPcplWtdFlow; // Calculated Wtd Flow for gross of fee principal ror
  double fNFTWWtdFlow; // Calculated Wtd Flow for net of fee time weighted ror,
                       // type = 14
  double fCNTWWtdFlow; // Calculated Wtd Flow for net of consulting fee time
                       // weighted ror
  double fNFPcplWtdFlow; // Calculated Wtd Flow for net of fee principal ror
  double fIncWtdFlow;    // Calculated Wtd Flow for income ror
  double
      fGFTEWtdFlow; // Calculated Wtd Flow for gross of fee Tax Equivalent ror
  double fNFTEWtdFlow; // Calculated Wtd Flow for net of fee Tax Equivalent ror
  double fGFATWtdFlow; // Calculated Wtd Flow for gross of fee After Tax ror
  double fNFATWtdFlow; // Calculated Wtd Flow for net of fee After Tax ror

  BOOL bTotalPortfolio;
  BOOL bInceptionRor;
  BOOL bTerminationRor;
  int iReturnstoCalculate;
  BOOL bCalcNetForSegments;
  // char		sTaxCalc[1+NT];
  ALLRORS zAllRor;
} RORSTRUCT;

typedef struct {
  double fEndMV;  // End Market value
  double fEndAI;  // Ending Accrued Interest
  double fEndAD;  // Ending Acrrued Dividend
  double fEndInc; // Ending Income
  double *pfNetFlow;
  double fBeginAI;
  double fBeginAD;
  double *pfIncome;
  double *pfWeight;
  int iNumFlows;
  BOOL bTotalPortfolio;
  double fBaseRor;
  double fIncomeRor;
} DWRORSTRUCT;

typedef struct {
  PERFKEY zPK;
  char sRecordType[2]; // W - Weighted, E - Equity + Equity CASH, F - FI + FI
                       // CASH
  long lParentRuleNo;
  int iScrHDIndex; // script header-detail index
  double fBeginMV;
  double fBeginAI;
  double fBeginAD;
  double fBeginInc;
  double fBegFedataxAI;
  double fBegFedataxAD;
  double fBegFedetaxAI;
  double fBegFedetaxAD;
  /* state tax fields are not in use - 5/12/06 vay
  double			fBegStataxAI;
  double			fBegStataxAD;
  double			fBegStetaxAI;
  double			fBegStetaxAD;
  */
  BOOL bGotBeginMVFromPerformance; // Has it been read yet from performance
                                   // table?
  BOOL bGotBeginMVFromHoldings;    // Has it been read yet from holdings table?
  int iDInfoCapacity;
  int iDInfoCount;
  DAILYINFO *pzDInfo;
  DAILYTAXINFO *pzTInfo;
  int iWDInfoCapacity;
  int iWDInfoCount;
  WTDDAILYINFO *pzWDInfo;
  double fAbs10PrcntMV;
  long lScratchDate; // date in daily db which can be over-written with current
                     // dsumdata/drtrnset records
  ALLRORS zBeginIndex;
  ALLRORS zNewIndex;
  // BOOL			bWtdKey; //does perfrule associated with it has
  // WtdRecInd = W
  BOOL bNewKey;        // did Key get created this time ?
  BOOL bDeleteKey;     // Is key going to be deleted this time(EndMV = 0)
  BOOL bKeyCopied;     // Another copy of the key created this time ?
  BOOL bDeletedFromDB; // Deleted from database table but still here
} PKEYSTRUCT;

typedef struct {
  int iCapacity;
  int iCount;
  PKEYSTRUCT *pzPKey;
  //  char	sTaxCalc[1+NT]; // same as perfctrl.sTaxCalc SB 5/29/08 no
  //  longer needed, replaced by returnstocalculate in portiinfo table
  char sCalcFlow[1 + NT];     // same as perfctrl.sCalcFlow
  char sPerfInterval[1 + NT]; // same as perfctrl.sPerfInterval
  char sDRDElig[1 + NT];      // same as perfctrl.sDRDElig
  long lFlowStartDate;        // same as perfctrl.lFlowStartDate
                              // BOOL		bRorType[NUMRORTYPE_ALL];
} PKEYTABLE;

// this structure keeps cumulative flows for the month for the segment (not the
// key) in order to save correct NetFlow number on Monthsum record in cases of
// keys terminating and re-incepting multiple times within a month (like M..
// T... IFT... IFT... I... M); also to keep calculated weighted flow for the
// month;
typedef struct {
  SUMMDATA zMonthsum;
  RORSTRUCT zMonthlyRor;
  int iNumSubPeriods;
} MONTHSTRUCT;

typedef struct {
  int iCapacity;
  int iCount;
  MONTHSTRUCT *pzMonthlyData;
} MONTHTABLE;

/*  typedef struct
{
int      iCapacity;
int      iCount;
PERFRULE *pzPRule;
int			 *piTHDIndex;
} PERFRULETABLE;*/

typedef struct {
  PTMPHDR zHeader;
  int iCapacity;
  int iCount;
  PTMPDET *pzDetail;
} PTMPHDRDET;

typedef struct {
  int iTHdrDetCreated;
  int iNumTHdrDet;
  PTMPHDRDET *pzTHdrDet;
} PTMPHDRDETTABLE;

/*  typedef struct
{
short iSResult; // result with security currency
short iAResult; // result with accrual currency //
} CURRENCYRESULT;

// Matrix with perfkey on one dimension and asset on the other
typedef struct
{
int            iKeyCount;
int            iNumAsset;
CURRENCYRESULT *pzStatusFlag;
} PKEYASSETTABLE;*/

typedef struct {
  int iNumDays;
  short *piResult; // result array for security as well as accrual currency
} CURRENCYRESULT2;

/* Matrix with perfkey on one dimension and asset array on the other */
typedef struct {
  int iKeyCount;
  int iNumAsset;
  CURRENCYRESULT2 *pzStatusFlag;
} PKEYASSETTABLE2;

/*
** Table to keep a list of unique parent perfrules having WtdRecInd = 'W' for
** an account.
*/
typedef struct {
  int iCapacity;
  int iCount;
  long *plPRule;
} PARENTRULETABLE;

typedef struct {
  char sValue[2 + NT];
} STRING2;

typedef struct {
  int iCapacity;
  int iCount;
  STRING2 *sItem;
} RESULTLIST;

/*
** structure to keep track of what perfrules(parent perfrules) are required
** for each continuous date range (between start and end date ranges, the
** program is running for) and if a perfkey(created using that perfrule)
** already exist for that perfrule and date range.
*/
typedef struct {
  long lStartDate;
  long lEndDate;
  long lPerfruleNo;
  long lPerfkeyNo;
} VALIDDATEPRULE;

typedef struct {
  int iVDPRCreated;
  int iNumVDPR;
  VALIDDATEPRULE *pzVDPR;
} VALIDDPTABLE;

typedef void(CALLBACK *LP2PR1PCHAR)(char *);
typedef ERRSTRUCT(CALLBACK *LPFNCREATEPERFSCRIPT)(PTMPHDRDET, PARTASSET2, int,
                                                  BOOL, PSCRHDRDET *);
typedef ERRSTRUCT(CALLBACK *LPFNADDPERFRULE)(PERFRULETABLE *, PERFRULE,
                                             PTMPHDRDETTABLE);
typedef ERRSTRUCT(CALLBACK *LPFNADDTEMPLATEHEADER)(PTMPHDRDETTABLE *, PTMPHDR,
                                                   int *);
typedef ERRSTRUCT(CALLBACK *LPFNADDTEMPLATEDETAIL)(PTMPHDRDETTABLE *, PTMPDET,
                                                   int);
typedef ERRSTRUCT(CALLBACK *LPFNADDASSET)(ASSETTABLE2 *, PARTASSET2, LEVELINFO,
                                          PARTSTYPETABLE, int *, long, long);
typedef ERRSTRUCT(CALLBACK *LPFNADDSCRDETAIL)(PSCRDET zSDetail,
                                              PSCRHDRDET *pzScrHdrDet);
typedef ERRSTRUCT(CALLBACK *LPFNFINDCREATESCRIPTHEADER)(int, long, ASSETTABLE2,
                                                        PSCRHDRDETTABLE *,
                                                        PSCRHDRDET *, BOOL,
                                                        long, int, int *);
typedef void(CALLBACK *LPPRSELECTACTIVEPERFRETURNTYPE)(long *, long, long, long,
                                                       ERRSTRUCT *);
typedef ERRSTRUCT(CALLBACK *LPFNPARTASSET2)(PARTASSET2 *);

/****** GLOBAL VARIABLES ******/
#ifndef GLOBALVARS
#define GLOBALVARS

//	extern		long alPrimesForChars[];
//	extern	  long alPrimesForLines[];
static long alPrimesForChars[] = {
    101,  109,  113,  137,  149,  157,  173,  181,  193,  197,  229,  233,
    241,  257,  269,  277,  281,  293,  313,  317,  337,  349,  353,  373,
    389,  397,  401,  409,  421,  433,  449,  457,  461,  509,  521,  541,
    557,  569,  577,  593,  601,  613,  617,  641,  653,  661,  673,  677,
    701,  709,  733,  757,  761,  769,  773,  797,  809,  821,  829,  853,
    857,  877,  881,  929,  937,  941,  953,  977,  997,  1009, 1013, 1021,
    1033, 1049, 1061, 1069, 1093, 1097, 1109, 1117, 1129, 1153, 1181, 1193,
    1201, 1213, 1217, 1229, 1237, 1249, 1277, 1289, 1297, 1301, 1321, 1361,
    1373, 1381, 1409, 1429, 1433, 1453, 1481, 1489, 1493, 1549, 1553, 1597,
    1601, 1609, 1613, 1621, 1637, 1657, 1669, 1693, 1697, 1709, 1721, 1733,
    1741, 1753, 1777, 1789, 1801, 1861, 1873, 1877, 1889};

static long alPrimesForLines[] = {
    103,  107,  127,  131,  139,  151,  163,  167,  179,  191,  199,  211,
    223,  227,  239,  251,  263,  271,  283,  307,  311,  331,  347,  359,
    367,  379,  383,  419,  431,  439,  443,  463,  467,  479,  487,  491,
    499,  503,  523,  547,  563,  571,  587,  599,  607,  619,  631,  643,
    647,  659,  683,  691,  719,  727,  739,  743,  751,  787,  811,  823,
    827,  839,  859,  863,  883,  887,  907,  911,  919,  947,  967,  971,
    983,  991,  1019, 1031, 1039, 1051, 1063, 1087, 1091, 1103, 1123, 1151,
    1163, 1171, 1187, 1223, 1231, 1259, 1279, 1283, 1291, 1303, 1307, 1319,
    1327, 1367, 1399, 1423, 1427, 1439, 1447, 1451, 1459, 1471, 1483, 1487,
    1499, 1511, 1523, 1531, 1543, 1559, 1567, 1571, 1579, 1583, 1607, 1619,
    1627, 1663, 1667, 1699, 1723, 1747, 1759, 1783, 1787};

//	extern	FILE            *fp;
extern CAPICOM::IHashedDataPtr pHashData;

extern TRANTYPETABLE zTTypeTable;
extern PARTSTYPETABLE zPSTypeTable;
extern CURRENCYTABLE zCurrTable;
extern COUNTRYTABLE zCountryTable;
extern PSCRHDRDETTABLE g_zSHdrDetTable;
extern PTMPHDRDETTABLE zTHdrDetTable;
extern SYSTEM_SETTINGS zSysSet;
extern char cCalcAccdiv;

// functions from transengine dll
extern LPFN1PCHAR lpfnSetErrorFileName;
extern LPPRERRSTRUCT lpprInitializeErrStruct;
extern LPFNPRINTERROR lpfnPrintError;
extern LPPRINITTRANS lpprInitTrans;

// function from calcgainloss dll
extern LPFNCALCGAINLOSS lpfnCalcGainLoss;

// functions from starsutils dll
extern LPFNRMDYJUL lpfnrmdyjul;
extern LPFNRJULMDY lpfnrjulmdy;
extern LPFN1INT lpfnCurrentMonthEnd, lpfnLastMonthEnd, lpfnIsItAMonthEnd;
extern LPFN2VOID lpfnCurrentDateAndTime;
extern LPFNISITAMARKETHOLIDAY lpfnIsItAMarketHoliday;

// functions from oledbioio dll
extern LPPR3PCHAR1LONG1INT lpprOLEDBIOInit;
extern LPPRVOID lpprFreeOLEDBIO;
extern LPFNVOID lpfnCommitTransaction;
extern LPFNVOID lpfnRollbackTransaction;
extern LPFNVOID lpfnStartTransaction;
extern LPFNVOID lpfnGetTransCount;
extern LPFN1BOOL lpfnAbortTransaction;

extern LPPRPARTASSET lpprSelectPartAsset;
extern LPPRALLACCDIVFORANACCOUNT lpprSelectAllAccdivForAnAccount;
//	extern		LPFNACCDIVTABLE
// lpfnFindDividendInAccdiv;
extern LPPR3LONG lpprDeleteBankstat;
extern LPPRSELECTALLPARTCURRENCIES lpprSelectAllPartCurrencies;
extern LPPRSELECTALLCOUNTRIES lpprSelectAllCountries;
extern LPPR1INT1LONG lpprDeleteDailyFlows;
extern LPPR1INT1LONG lpprDeleteDailyFlowsByID;
extern LPPRDAILYFLOWS lpprInsertDailyFlows;
extern LPPR3LONG lpprDeleteDSumdata;
extern LPPRSUMMDATA lpprInsertDailySummdata;
extern LPPRPSUMMDATA2LONG lpprSelectDailySummdata;
extern LPPRSUMMDATA1LONG lpprUpdateDailySummdata;
extern LPPRSELECTPERFORMANCEHOLDINGS lpprSelectPerformanceHoldcash;
extern LPPRSELECTPERFORMANCEHOLDINGS lpprSelectPerformanceHoldings;
extern LPPR8PCHAR1PLONG lpprReadAllHoldmap;
extern LPPR3LONG lpprDeleteMonthSum;
extern LPPRSUMMDATA lpprInsertMonthlySummdata;
extern LPPRPPERFCTRL1INT lpprSelectPerfctrl;
extern LPPRPERFCTRL lpprUpdatePerfctrl;
extern LPPR1LONG lpprDeletePerfkey;
extern LPPRPERFKEY lpprInsertPerfkey;
extern LPPRPPERFKEY1INT lpprSelectPerfkeys;
extern LPPRPERFKEY lpprUpdateNewPerfkey, lpprUpdateOldPerfkey;
extern LPPRSELECTALLPERFRULE lpprSelectAllPerfrule;
extern LPPRSELECTONEPARTPORTMAIN lpprSelectOnePartPortmain;
extern LPPRSELECTALLSCRIPTHEADERANDDETAILS lpprSelectAllScriptHeaderAndDetails;
extern LPPRPERFSCRIPTHEADER lpprInsertPerfscriptHeader,
    lpprUpdatePerfscriptHeader;
extern LPPRPERFSCRIPTDETAIL lpprInsertPerfscriptDetail;
extern LPPRPORTTAX lpprSelectAllPorttax;
extern LPPRSELECTALLPARTSECTYPE lpprSelectAllPartSectype;
extern LPPR2PLONG lpprSelectStarsDate;
extern LPPR2PLONG lpprSelectCFStartDate;
extern LPPR3LONG lpprDeleteSummdata;
extern LPPRSUMMDATA lpprInsertPeriodSummdata;
extern LPPRPSUMMDATA3LONG lpprSelectPeriodSummdata;
extern LPPRSUMMDATA lpprUpdatePeriodSummdata;
extern LPPR1PCHAR2PINT1INT lpprSelectOneSegment;
extern LPPRSELECTSYSSETTINGS lpprSelectSyssetng;
extern LPPRSELECTSYSVALUES lpprSelectSysvalues;
extern LPPR3LONG lpprDeleteTaxperf;
extern LPPR3LONG lpprDeleteTaxperfForSegment;
extern LPPRTAXPERF lpprInsertTaxperf;
extern LPPRSELECTTAXPERF lpprSelectTaxperf;
extern LPPRSELECTALLTEMPLATEDETAILS lpprSelectAllTemplateHeaderAndDetails;
extern LPPRSELECTPERFORMANCETRANSACTION lpprSelectPerformanceTransaction;
extern LPPRSELECTALLPARTTRANTYPE lpprSelectAllPartTrantype;
extern LPFNSELECTSEGMENTIDFROMSEGMAP lpfnSelectSegmentIDFromSegmap;
extern LPPRSELECTSEGMAIN lpprSelectSegmain;
extern LPPRINSERTSEGMAIN lpprInsertSegmain;
extern LPPRSELECTSEGTREE lpprSelectSegtree;
extern LPPRINSERTSEGTREE lpprInsertSegtree;
extern LPPRSELECTSEGMENT lpprSelectSegment;
extern LPPRINSERTSEGMENT lpprInsertSegment;
extern LPFNSELECTSEGMAP lpfnSelectSegmap;
extern LPPRINSERTSEGMAP lpprInsertSegmap;
extern LPFN2PCHAR lpfnSelectSecSegmap;
extern LPPRINSERTSECSEGMAP lpprInsertSecSegmap;

extern LPPRSELECTUNITVALUE lpprSelectUnitValue;
extern LPPRSELECTUNITVALUERANGE2 lpprSelectUnitValueRange2;
extern LPPRINSERTUNITVALUE lpprInsertUnitValue;
extern LPPRINSERTUNITVALUEBATCH lpprInsertUnitValueBatch;
extern LPPRUPDATEUNITVALUE lpprUpdateUnitValue;
extern LPPR4LONG lpprDeleteUnitValueSince2;
extern LPPR4LONG lpprDeleteDailyUnitValueForADate;
extern LPPR3LONG lpprMarkPeriodUVForADateRangeAsDeleted;
extern LPPR2LONG lpprDeleteMarkedUnitValue;
extern LPPR3LONG lpprRecalcDailyUV;
extern LPPRSELECTACTIVEPERFRETURNTYPE lpprSelectActivePerfReturnType;
extern LPPRPERFASSETMERGE lpprSelectPerfAssetMerge;

// functions from calcflow dll
extern LPFNCALCFLOW lpfnCalcNetFlow;
extern LPFN1PCHAR lpfnInitCalcFlow;

// functions from roll dll
extern LPFN3PCHAR1LONG1PCHAR lpfnInitRoll;
extern LPFNROLL lpfnRoll;

// functions from valuation dll
extern LPFN1LONG2PCHAR lpfnInitValuation;
extern LPFN1PCHAR1INT1LONG1BOOL lpfnValuation;

// functions from timer dll
/*
extern		LP2PR1PCHAR	lpprTimerResult;
extern		LPFN1INT	lpfnTimer;
*/

#endif

/* Prototype of functions defined in calcperf.cpp file */
ERRSTRUCT CalculatePerformance(PARTPMAIN *pzPdir, long lLastPerfDate,
                               long lCurrentPerfDate, long lEarliestPerfDate,
                               PKEYTABLE *pzPKeyTable, char *sBaseCurrency,
                               PERFCTRL zPCtrl, BOOL bOnlyCreateScripts,
                               PERFRULETABLE *pzRuleTable);
ERRSTRUCT GetAllPerfkeys(int iID, PKEYTABLE *pzPKTable);
ERRSTRUCT GetAllReturnTypes(int iID, PKEYTABLE *pzPKTable);
ERRSTRUCT CalculateFlow(PKEYTABLE zPKTable, TRANSTABLE zTTable,
                        PKEYASSETTABLE2 zPATable, ASSETTABLE2 zATable,
                        PERFRULETABLE zRuleTable, PERFASSETMERGETABLE zPAMTable,
                        int iWhichFlow, long lLastPerfDate,
                        long lCurrentPerfDate, long lEarliestPerfDate,
                        PARTPMAIN zPmain);
ERRSTRUCT GetValuesOnFixedPoints(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable,
                                 TRANSTABLE *pzTTable,
                                 PKEYASSETTABLE2 *pzPATable,
                                 PKEYTABLE *pzPTable, PERFRULETABLE zRuleTable,
                                 PARTPMAIN *pzPdir, ACCDIVTABLE zADTable,
                                 PERFASSETMERGETABLE zPAMTable);
int Calc10PercentOfMV(PKEYSTRUCT *pzPKey, int iDateIndex, double fRate);
int ChangeTotalInceptDateIfRequired(PKEYTABLE zPKTable, long lSegmentInitDate);
double SumOfBeginValuesForADate(PKEYSTRUCT zPKey, int iDateIndex);

ERRSTRUCT CopyAnExistingKey(PKEYTABLE *pzPKTable, int iKeyIndex, int iDateIndex,
                            int iReturnsToCalculate);
ERRSTRUCT CopyTotalIfRequired(PKEYTABLE *pzPKTable, int iDIndex,
                              int iReturnsToCalculate);
ERRSTRUCT CalculateWeightedValues(PKEYTABLE zPKTable, BOOL bSpecialCase,
                                  int iReturnsToCalculate);
void NewNetCumWtd(double fPNet, double fPCum, BOOL bPPeriodEnd,
                  long lPDaysSinceNond, double fNToday, double *pzNNet,
                  double *pzNCum, double *pzNWtd, BOOL bAddToCum);
ERRSTRUCT CalculateReturnForAllKeys(PKEYTABLE zPTable, long lLastPerfDate,
                                    long lCurrentPerfDate, PARTPMAIN zPdir,
                                    PERFCTRL zPCtrl, int bCalcSelected);
// This is obsolete function - no UV storage in BLOB anymore - vay, 8/13/03
/*
ERRSTRUCT WriteReturnsToBlob(PKEYTABLE zPTable, int iKIndex, long lReturnDate,
long lCurrentPerfDate, BOOL bTermination, BOOL *pbStart, BOOL *pbFinish);
*/

ERRSTRUCT SetTaxInformation(PKEYTABLE zPTable, int iKey, int iDate,
                            RORSTRUCT *pzRor);
// ERRSTRUCT TaxRelatedReturns(PKEYTABLE zPTable, int iKey, int iDate, PORTTAX
// zPorttax, RORSTRUCT *pzRor);
void CreatePerformFromDInfo(PKEYTABLE zPTable, int iKey, int iDate,
                            SUMMDATA *pzPerform);
void CreateMonthsumFromTwoSummdata(SUMMDATA zCurrentSum, SUMMDATA zPreviousSum,
                                   SUMMDATA *pzMSum, BOOL bCumOnly = FALSE);
ERRSTRUCT WeightedAveragePerformance(PKEYTABLE *pzPTable, PERFRULETABLE zRTable,
                                     PKEYASSETTABLE2 zPATable,
                                     ASSETTABLE2 zATable, long lLastPerfDate,
                                     long lCurrentPerfDate);
ERRSTRUCT RequiredKeysForParentRule(VALIDDPTABLE *pzVDPTable, long lParentRule,
                                    long lStartDate, long lEndDate,
                                    PKEYTABLE zPTable);
ERRSTRUCT GetWtdInfoAndDelKeys(PKEYTABLE *pzPTable, PERFRULETABLE zRTable,
                               VALIDDPTABLE *pzVDPTable,
                               PKEYASSETTABLE2 zPATable, ASSETTABLE2 zATable,
                               long lLastPerfDate, long lStartDate,
                               long lEndDate);
ERRSTRUCT CalcWtdPerformance(VALIDDPTABLE zVDPTable, PKEYTABLE *pzPTable,
                             long lLastPerfDate, long lStartDate,
                             long lEndDate);

/* Prototype of functions defined in calcperf2.cpp */
DLLAPI ERRSTRUCT STDCALL TestAsset(PARTASSET2 zPAsset, PSCRDET *pzDetail,
                                   int iCount, BOOL bSecCurr, long lDate,
                                   PARTPMAIN zPmain, BOOL *pbResult,
                                   long lLastPerfDate, long lCurrPerfDate);
ERRSTRUCT TestAlphaValue(char *sLowVal, char *sHighVal, char *sComparisonRule,
                         char *sMatchExpand, char *sMatchWild, char *sMatchRest,
                         char *sTestString, BOOL *bResult);
ERRSTRUCT TestDoubleValue(char *sLowStr, char *sHighStr, char *sComparisonRule,
                          double fTestValue, BOOL *bResult);
ERRSTRUCT TestIntegerValue(char *sLowStr, char *sHighStr, char *sComparisonRule,
                           int iTestValue, BOOL *bResult);
ERRSTRUCT TestFieldValue(char *sTableName, char *sFieldName,
                         char *sComparisonRule, char *sTestValue,
                         PARTPMAIN zPmain, BOOL *pbResult);
ERRSTRUCT TestSpecialValue(char *sBeginVal, int iTestValue, BOOL *pbResult);
ERRSTRUCT CreateDynamicPerfkeys(PKEYTABLE *pzPTable, PARTPMAIN zPmain,
                                long lLastPerfDate, long lCurrentPerfDate,
                                ASSETTABLE2 zATable, PERFRULETABLE *pzRuleTable,
                                PERFASSETMERGETABLE zPAM, long lInitPerfDate,
                                BOOL bOnlyCreateScripts);
// ERRSTRUCT PerfkeyForWeightedParent(PKEYTABLE *pzPTable, PKEYASSETTABLE2
// zPATable, ASSETTABLE2 zATable, PERFRULETABLE zRTable,
// long lParentRuleNo,
// long lKeyStartDate, long lKeyEndDate);
DLLAPI ERRSTRUCT STDCALL CreateVirtualPerfScript(PTMPHDRDET zTmpHdrDet,
                                                 PARTASSET2 zPAsset,
                                                 int iDateOffset, BOOL bSecCurr,
                                                 PSCRHDRDET *pzScrHdrDet);
ERRSTRUCT GetSelectTypeValueForAnAsset(PARTASSET2 zPAsset, int iDateOffset,
                                       char *sSelectType, BOOL bSecCurr,
                                       char *sAlphaValue, double *pfFloatValue,
                                       int *piIntegerValue);
void MaskStringUsingTemplate(char *sTemplateString, char *sMaskRest,
                             char *sMaskWild, char *sMaskExpand,
                             char *sTestString, char *sResultString);
DLLAPI ERRSTRUCT STDCALL CreateNewScript(PSCRHDRDET *pzScrHdrDet,
                                         PSCRHDRDETTABLE *pzSHDTable,
                                         BOOL bSingleSecurity, char *sSecNo,
                                         char *sWi, char *SecDesc1);
ERRSTRUCT CreateNewPerfkey(PKEYTABLE *pzPTable, PERFRULE zPrule, long lScrhdrNo,
                           int iSegmentTypeID, long lCurrentPerfDate);
long FindParentPerfkeyNo(PKEYTABLE zPTable, long lParentRuleNo);
ERRSTRUCT FillHoldingAndAssetTables(PARTPMAIN *pzPdir, long lDate,
                                    ASSETTABLE2 *pzATable,
                                    HOLDINGTABLE *pzHTable, long lLastPerfDate,
                                    long lCurrentPerfDate, char DoAccruals);
ERRSTRUCT FillTransAndAssetTables(int iID, int iVendorid, long lDate1,
                                  long lDate2, ASSETTABLE2 *pzATable,
                                  TRANSTABLE *pzTTable, BOOL bAdjustPerfDate);
ERRSTRUCT FillAccdivTable(int iID, long lLastPerfDate, long lCurrentPerfDate,
                          ASSETTABLE2 *pzATable, ACCDIVTABLE *pzAccdivTable);
ERRSTRUCT GetAccruals(ACCDIVTABLE zADTable, ASSETTABLE2 *pzATable,
                      TRANSTABLE *pzTTable, HOLDINGTABLE *pzHTable,
                      int iVendorID, long lPerfDate, long lLastPerfDate,
                      long lCurrentPerfDate);
ERRSTRUCT AddAccrualToHolding(PARTTRANS zTR, PARTACCDIV zAR, BOOL bUseAccdiv,
                              int iVendorID, ASSETTABLE2 *pzATable,
                              HOLDINGTABLE *pzHTable, long lDate,
                              long lLastPerfDate, long lCurrentPerfDate);
ERRSTRUCT GetCurrencyAsset(ASSETTABLE2 *pzATable, TRANSTABLE zTTable,
                           int iVendorID, long lLastPerfDate,
                           long lCurrentPerfDate);
ERRSTRUCT FindAndAddCurrency(char *sCurrId, ASSETTABLE2 *pzATable, int iID,
                             long lTransNo, int iVendorID, int *piIndex,
                             long lLastPerfDate, long lCurrentPerfDate);
ERRSTRUCT CreatePKeyAssetTable(PKEYASSETTABLE2 *pzPATable, PKEYTABLE zPKTable,
                               ASSETTABLE2 zATable, PERFASSETMERGETABLE zPAM,
                               PERFRULETABLE zruleTable, long lDate,
                               PARTPMAIN zPmain, long lLastPerfDate,
                               long lCurrentPerfDate);
ERRSTRUCT GetHoldingValues(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable,
                           TRANSTABLE *pzTTable, PKEYASSETTABLE2 *pzPATable,
                           PKEYTABLE zPKTable, PERFRULETABLE zRuleTable,
                           PARTPMAIN *pzPdir, ACCDIVTABLE zADTable,
                           PERFASSETMERGETABLE zPAMTable, long lDate,
                           long lLastPerfDate, long lCurrPerfDAte,
                           char cDoAccruals);
ERRSTRUCT GetLastAndBeginValues(int iID, long lLastPerfDate,
                                PKEYTABLE *pzPKTable, int iReturnsToCalculate);
ERRSTRUCT GetScratchRecord(PKEYTABLE zPTable, int iPortfolioID,
                           long lLastPerfDate, long lCurrentPerfDate);
// ERRSTRUCT GetStartIndexAndScratchRecord(PKEYTABLE zPTable, int iID, long
// lLastPerfDate, long lCurrentPerfDate);
ERRSTRUCT FillPorttaxTable(int iID, long lLastPerfDate, long lCurrentPerfDate,
                           PORTTAXTABLE *pzPTaxTable);
ERRSTRUCT DeleteKeysMemoryDatabase(PKEYTABLE *pzPTable, long lLastPerfDate,
                                   long lCurrentPerfDate);
ERRSTRUCT DeleteDataIfNew(PKEYTABLE zPTable, int iID, long lLastPerfDate,
                          long lCurrentPerfDate, BOOL bDaily);
short LongShortBitForTrans(PARTTRANS zTR);
ERRSTRUCT FindParentPerfkeys(PKEYTABLE *pzPTable, long lStartDate);
void CopySummDataToDailyInfo(SUMMDATA zSD, DAILYINFO *pzDInfo);
ERRSTRUCT GenerateNotionalFlow(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable,
                               TRANSTABLE *pzTTable, PKEYASSETTABLE2 *pzPATable,
                               PKEYTABLE *pzPTable, PERFRULETABLE zRuleTable,
                               PARTPMAIN *pzPmain, ACCDIVTABLE zADTable,
                               PERFASSETMERGETABLE zPAMTable);
void AddNotionalFlowToTheKey(PKEYTABLE *pzPKTable, ASSETTABLE2 zATable,
                             int iKeyIndex, int iKeyDateIndex, int iAssetIndex,
                             int iAssetDateIndex, BOOL bThisSResult,
                             BOOL bPreviousSResult, BOOL bThisAResult,
                             BOOL bPreviousAResult);

/* Prototype of functions defined in calcperf3.cpp file */
void CalcPerfCleanUp();
void InitializeDailyInfo(DAILYINFO *pzDIVal);
void InitializeTranTypeTable(TRANTYPETABLE *pzTTable);
void InitializeWtdDailyInfo(WTDDAILYINFO *pzWDIVal);
void InitializeSecTypesTable(PARTSTYPETABLE *pzSTable);
void InitializeCurrencyTable(CURRENCYTABLE *pzCTable);
DLLAPI void STDCALL InitializePScrHdrDet(PSCRHDRDET *pzSHdrDet);
void InitializePScrHdrDetTable(PSCRHDRDETTABLE *pzSHdrDetTable);
void InitializePTmpHdrDet(PTMPHDRDET *pzTHdrDet);
void InitializePTmpHdrDetTable(PTMPHDRDETTABLE *pzTHdrDetTable);
void InitializePartialAccdiv(PARTACCDIV *pzPAccdiv);
void InitializeAccdivTable(ACCDIVTABLE *pzADTable);
DLLAPI ERRSTRUCT STDCALL GetSysSettings(void);
DLLAPI void STDCALL InitializePartialAsset(PARTASSET2 *pzPAsset);
DLLAPI ERRSTRUCT STDCALL AllocateMemoryToDailyAssetInfo(PARTASSET2 *pzPAsset);
DLLAPI void STDCALL InitializeAssetTable(ASSETTABLE2 *pzATable);
void InitializePartialHolding(PARTHOLDING *pzPHold);
void InitializeHoldingTable(HOLDINGTABLE *pzHTable);
void InitializePartialTrans(PARTTRANS *pzPTrans);
void InitializeTransTable(TRANSTABLE *pzTTable);
DLLAPI void STDCALL InitializePerfrule(PERFRULE *pzPrule);
DLLAPI void STDCALL InitializePerfruleTable(PERFRULETABLE *pzPrTable);
void InitializePerfScrHdr(PSCRHDR *pzHeader);
void InitializePerfScrDet(PSCRDET *pzDetail);
DLLAPI void STDCALL InitializePerfTmpHdr(PTMPHDR *pzHeader);
DLLAPI void STDCALL InitializePerfTmpDet(PTMPDET *pzDetail);
void InitializePerform(SUMMDATA *pzPerfVal, long lCurrentDate);
void InitializeAllRors(ALLRORS *pzROR, BOOL bInitAsIndex);
void InitializePKeyStruct(PKEYSTRUCT *pzPKey);
void InitializePKeyTable(PKEYTABLE *pzPKeyTable);
void InitializePKeyAssetTable(PKEYASSETTABLE2 *pzPKATable);
void InitializeRorStruct(RORSTRUCT *pzRorStruct, long lDate);
void InitializeDWRorStruct(DWRORSTRUCT *pzDWRor);
void InitializeRtrnset(double *pfRorIdx, BOOL bInitAsIndex);
void InitializeUnitValue(UNITVALUE *pzUV, BOOL bInitAsIndex, BOOL bFullInit);
void InitializeResultList(RESULTLIST *pzList);
void InitializeValidDatePrule(VALIDDATEPRULE *pzVDPR);
void InitializeValidDPTable(VALIDDPTABLE *pzVDPTable);
void InitializeDailyTaxinfo(DAILYTAXINFO *pzTaxinfo);
void InitializeTaxperf(TAXPERF *pzTaxperf);
void CreateTaxperfFromDailyTaxInfo(DAILYTAXINFO zTI, int iPortfolioID, int iID,
                                   long lPerfDate, TAXPERF *pzTaxperf);
void CreateTaxinfoForAKey(PKEYTABLE zPTable, int iKey, int iDate,
                          TAXINFO *pzTInfo);
void InitializePorttax(PORTTAX *pzPTax, long lDate);
void InitializePorttaxTable(PORTTAXTABLE *pzPTaxTable);
void InitializeMonthlyTable(MONTHTABLE *pzMTable);
void InitializeTaxinfo(TAXINFO *pzTaxinfo);
void InitializePerfAssetMerge(PERFASSETMERGE *pzPAMerge);
void InitializePerfAssetMergeTable(PERFASSETMERGETABLE *pzAMTable);
void CopyPartTransToTrans(PARTTRANS zPT, TRANS *pzTR);
char LookupHoldName(char *sHoldingsName, char *sHoldCashName, long lDate);
void InitializeSecTypeTable(PARTSTYPETABLE *pzPSTypeTable);
ERRSTRUCT SelectAsset(char *sSecNo, char *sWi, int iVendorID,
                      PARTASSET2 *pzPAsset, LEVELINFO *pzLevels,
                      short iLongShort, int iID, long lTransNo);
ERRSTRUCT SelectAssetAllLevels(ASSETTABLE2 *pzATable, char *sSecNo, char *sWi,
                               int iVendorID, PARTASSET2 *pzPAsset,
                               LEVELINFO *pzLevels, short iLongShort, int iID,
                               long lTransNo, int *iToSecNoIndex,
                               long lLastPerfDate, long lCurrentPerfDate);
ERRSTRUCT GetScriptHeaderAndDetail(char *iID, PKEYTABLE *pzPTable);
ERRSTRUCT DeletePerformSetByPerfNo(long lPerfkeyNo, BOOL bDaily);
ERRSTRUCT DeletePeriodPerformSet(PKEYTABLE zPTable, int iID, long lStartDate,
                                 long lEndDate, int bCalcSelected,
                                 long lInceptionDate);
ERRSTRUCT InsertPerform(SUMMDATA zPRec, char *sDailyMonthly, long *plPerformNo);
ERRSTRUCT UpdatePerform(SUMMDATA zPRec2, char *sDailyMonthly2);
ERRSTRUCT InsertRorRecord(RTRNSET zRorRec, char *sDailyMonthly2,
                          char *sTableName2);
ERRSTRUCT UpdateRorRecord(RTRNSET zRorRec, char *sDailyMonthly3,
                          char *sTableName3, long lPerformNo);
ERRSTRUCT InsertPerfScriptHeader(PSCRHDR zPSHdr, long *plScrhdrNo);
ERRSTRUCT InsertPerfScriptDetail(PSCRDET zPSDet);
ERRSTRUCT InsertOrUpdateRor(PKEYTABLE zPTable, int iKeyIndex, char *sDBName);
ERRSTRUCT InsertOrUpdateUV(PKEYTABLE *pzPTable, int iKeyIndex, char *sDBName,
                           RORSTRUCT zRorStruct, long lLastPerfDate,
                           long lLndPerfDate, long lCurrPerfDate,
                           BOOL bPeriodEnd, PARTPMAIN zPmain);
ERRSTRUCT UpdateAllPerfkeys(PKEYTABLE zPTable);
ERRSTRUCT ReadPorttax(int iID12, long lTaxDate, PORTTAX *pzPTax);
ERRSTRUCT EquityAndFixedCashContWithForThePeriod(TRANSTABLE zTTable,
                                                 ASSETTABLE2 zATable,
                                                 long lEndDate,
                                                 double *pfEqFlow,
                                                 double *pfFiFlow);
BOOL SpecialRuleForEquityAndFixedExists(PKEYTABLE zPTable,
                                        PERFRULETABLE zRuleTable);
int FindKeyForASegmentType(PKEYTABLE zPTable, int iSegmentType, long lDate);
int FindSpecialRule(PERFRULETABLE zRuleTable, char *sWhichRule);
int FindScriptForASpecialSegmentType(int iSegmentType, int iStartPoint);
PORTTAX FindCorrectPorttax(PORTTAXTABLE zPTaxTable, long lDate);
ERRSTRUCT GetEquityAndFixedPercent(PKEYTABLE zPKTable, PERFRULETABLE zPRTable,
                                   PARTPMAIN zPmain, long lDate,
                                   double *pfEqPct, double *pfFiPct);
BOOL IsThisASpecialAsset(PARTASSET2 zAsset, int iSpecailType);
ERRSTRUCT ResolveResultList(RESULTLIST zList, BOOL *pbResult);
ERRSTRUCT IsResultListOK(RESULTLIST zList);
BOOL IsValueZero(double fValue, unsigned short iPrecision);
int SegmentTypeForTheKey(PERFKEY zPK);
void CalcPerfInitialize(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable,
                        TRANSTABLE *pzTTable, PKEYASSETTABLE2 *pzPKATable,
                        PERFRULETABLE *pzRuleTable, ACCDIVTABLE *pzADTable,
                        PERFASSETMERGETABLE *zAMTable);
DllExport ERRSTRUCT FillPartSectypeTable(PARTSTYPETABLE *pzPSTTable);
BOOL TaxInfoRequired(int ireturnstoCalculate);
ERRSTRUCT FillScriptHeaderDetailTable(PSCRHDRDETTABLE *pzPSHdrDetTable,
                                      BOOL bAppendToTable);
int FindDailyInfoOffset(long lDate, PARTASSET2 zPAsset, BOOL bMatchExactDate);
//	int FindDailyInfoOffset(long lDate, long lLastPerfDate, long
// lCurrentPerfDate);
ERRSTRUCT FindOrAddDailyAssetInfo(PARTASSET2 *pzPAsset, long lDate,
                                  long lCurrentPerfDate, int *piIndex);
// BOOL IsTodaysDailyValuesSameAsYesterday(PARTASSET2 zAsset, int iToday);
BOOL IsThisDailyValuesSameAsPrevious(PARTASSET2 zAsset, int iDateOffset);

/* Prototype of functions defined in calcperf4.cpp */
DllExport int FindAssetInTable(ASSETTABLE2 zATable, char *sSecNo, char *sWi,
                               BOOL bCheckLongShort, short iLongShort);
int FindHoldingInTable(HOLDINGTABLE zHTable, int iID, long lTransNo);
int FindSecTypeInTable(PARTSTYPETABLE zPSTTable, int iSecType);
int FindTranTypeInTable(char *sTranType, char *sDrCr);
int FindCurrencyInTable(char *sCurrId);
int FindCountryInTable(char *sCountryCode);
int FindScrHdrByHdrNo(PSCRHDRDETTABLE zSHDTable, long lSHdrNo);
DllExport int FindScrHdrByHashkeyAndTmpNo(PSCRHDRDETTABLE zSHdrDetTable,
                                          long lHashkey, long lTmphdrNo,
                                          char *sSHAKey,
                                          PSCRHDRDET zSHdrDetail);
int FindTemplateHeaderInTable(PTMPHDRDETTABLE zTHdrDetTable, long lTHdrNo);
int FindPerfkeyByPerfkeyNo(PKEYTABLE zPKTable, long lPKNo);
int FindPerfkeyByID(PKEYTABLE zPKTable, int iID, int iStartIndex,
                    BOOL bSkipNewAndDeleted);
int FindPerfkeyByRule(PKEYTABLE zPKTable, long lRuleNo, long lScrhdrNo,
                      long lCurrFlagNo);
int FindRule(PERFRULETABLE zRTable, long lRuleNo);
int FindMonthlyDataByID(MONTHTABLE zMTable, int iID);
ERRSTRUCT AddPerfkeyToTable(PKEYTABLE *pzPKeyTable, PKEYSTRUCT zPKey);
ERRSTRUCT AddMonthlyDataToTable(MONTHTABLE *pzMTable, SUMMDATA zMonthsum,
                                RORSTRUCT zMonthlyRor);
ERRSTRUCT AddAccdivToTable(ACCDIVTABLE *pzAccdivTable, PARTACCDIV zPAccdiv);
DllExport ERRSTRUCT AddAssetToTable(ASSETTABLE2 *pzAssetTable,
                                    PARTASSET2 zPAsset, LEVELINFO zLevels,
                                    PARTSTYPETABLE zPSTtable, int *piArrayIndex,
                                    long lLastPerfDate, long lCurrentPerfDate);
ERRSTRUCT AddPorttaxToTable(PORTTAXTABLE *pzPTaxTable, PORTTAX zPTax);
DllExport ERRSTRUCT AddScriptHeaderToTable(PSCRHDR zPSHeader,
                                           PSCRHDRDETTABLE *pzSHDTable,
                                           int *piArrayIndex);
DllExport ERRSTRUCT AddDetailToScrHdrDet(PSCRDET zSDetail,
                                         PSCRHDRDET *pzScrHdrDet);
DllExport ERRSTRUCT AddTemplateHeaderToTable(PTMPHDRDETTABLE *pzTHdrDetTable,
                                             PTMPHDR zTHeader,
                                             int *piArrayIndex);
DllExport ERRSTRUCT AddTemplateDetailToTable(PTMPHDRDETTABLE *pzTHdrDetTable,
                                             PTMPDET zTDetail,
                                             int iHeaderIndex);
ERRSTRUCT AddHoldingToTable(HOLDINGTABLE *pzHoldTable, PARTHOLDING zPHold);
ERRSTRUCT AddTransToTable(TRANSTABLE *pzTransTable, PARTTRANS zPTrans);
DllExport ERRSTRUCT AddPerfruleToTable(PERFRULETABLE *pzPRTable,
                                       PERFRULE zPRule,
                                       PTMPHDRDETTABLE zTHdrDetTable);
ERRSTRUCT AddAnItemToResultList(RESULTLIST *pzNewList, char *sStr);
ERRSTRUCT AddPerfAssetMergeToTable(PERFASSETMERGETABLE *pzAMTable,
                                   PERFASSETMERGE zPAMerge);
BOOL StrToBool(char *sStr);
char *BoolToStr(BOOL bBool);
ERRSTRUCT AddDailyInfoToAsset(PARTASSET2 *pzPAsset, LEVELINFO zDInfo,
                              long lLastPerfDate, long lCurrentPerfDate);
ERRSTRUCT AddDailyInfoIfNeededOnPertfAssetMergeDates(ASSETTABLE2 *pzATable,
                                                     PERFASSETMERGE zPAM,
                                                     BOOL bAddToFromMergedAsset,
                                                     long lLastPerfDate,
                                                     long lCurrentPerfDate);
BOOL AreTheseScriptsSame(PSCRHDRDET zSHD1, PSCRHDRDET zSHD2);
// void SetLevelsForADay(PARTASSET2 *pzPAsset, LEVELINFO zDInfo, long
// lLastPerfDate, long lCurrentPerfDate);
void FillLevelsForThePeriod(ASSETTABLE2 *pzATable);
/*	ERRSTRUCT AddSegmapToTable(SEGMAP zSegmap);
ERRSTRUCT AddSegmentsToTable(SEGMENTS zSegments);*/
ERRSTRUCT AddParentRuleIfNew(PARENTRULETABLE *pzPRTable, long lRuleNo);
ERRSTRUCT AddValidDatePRuleToTable(VALIDDPTABLE *pzDPTable,
                                   VALIDDATEPRULE zVDPRecord);
ERRSTRUCT CreateDailyInfo(PKEYTABLE *pzPKTable, int iKeyIndex,
                          long lLastPerfDate, long lCurrentPerfDate,
                          int iReturnsToCalculate);
ERRSTRUCT AddWtdDailyInfoToPerfkey(PKEYSTRUCT *pzPKey, WTDDAILYINFO zWDInfo);
int GetDateIndex(PKEYSTRUCT zPKey, long lDate);
double GetEndIndex(double fBegIndex, double fRor);
double GetRorFromTwoIndexes(double fBegIndex, double fEndIndex);
void LinkReturnsForTheMonth(ALLRORS zPreviousRor, ALLRORS zCurrentRor,
                            ALLRORS *pzNewRor);
DllExport long CreateHashkeyForScript(PSCRHDRDET *zSHdrDet, BOOL bUseInternal);
DllExport ERRSTRUCT FindCreateScriptHeader(int iID, long lCurrentPerfDate,
                                           ASSETTABLE2 zATable,
                                           PSCRHDRDETTABLE *pzSHdrDetTable,
                                           PSCRHDRDET *pzVirtualSHD,
                                           BOOL bSingleSecurity, long lTmphdrNo,
                                           int iAssetIndex, int *iSegmentIndex);
long MakeHash(long lOldHash, char *sString, int iLineNo);
// DllExport void  CalculateGFAndNFRor(RORSTRUCT *pzRS, BOOL bCalcOnlyBase);
DllExport void CalculateGFAndNFRor(RORSTRUCT *pzRS);
void TaxAdjustments(RORSTRUCT zRS, char *sTaxType, double *pfNetFlowAdj,
                    double *pfWtdFlowAdj, double *pfBegAccrAdj,
                    double *pfEndAccrAdj);
DllExport void CalculateDWRor(DWRORSTRUCT *pzDWRor);
double GFAndNFBaseROR(double fEMV, double fBMV, double fNFlow, double fWFlow,
                      double fFudgeFactor, BOOL bInceptionRor,
                      BOOL bTerminationRor);
double GFAndNFIncomeROR(double fEAccr, double fBAccr, double fInc,
                        double fBegMV, double fFudgeFactor);
int DWBaseRor(double fEndMV, double fFlow[], int iNumIterations,
              double fWeight[], double *pfROR);
ERRSTRUCT GetRorTypeIndex(char *sRorType, int *piIndex);
void CalculateNewRorIndex(ALLRORS zBeginIndex, ALLRORS zRor,
                          ALLRORS *pzNewIndex, int iWhichIndex);
DllExport void CalculateWeightedFlow(RORSTRUCT *pzRS);
double TimeWeightedFudgeFactor(double fEndMV, double fBegMV, double fNFlow,
                               double fWtFlow, double fRor, BOOL bInceptionRor);
double TimeWeightedIncomeFudgeFactor(double fEndAccr, double fBegAccr,
                                     double fIncome, double fBegMV,
                                     double fRor);
double TWWeightedFlow(double fEndMV, double fBegMV, double fNFlow,
                      double fFudge, double fRor, BOOL bInceptionRor);
// BOOL      IsThisAmonthEndDate(long lDate);
BOOL IsKeyDeleted(long lDeleteDate);
ERRSTRUCT ReadPerfctrl(int iID1, PERFCTRL *pzPCtl);
int FindPerfkeyByPerfkeyNo(PKEYTABLE pzPTable, long lPerfkeyNo);
int FindPerfkeyInValidDPRTable(VALIDDPTABLE pzVDPTable, long lPerfkeyNo);
void InitializeParentRuleTable(PARENTRULETABLE *zPRTable);
double EffectiveTaxRate(double fPortTaxRate);
/*	double    TaxEquivalentRate(double fTaxRate);
double    TaxableRate(double fTaxRate);*/
double TaxEquivalentAdjustment(double fTotValue, double fTaxRate);
double AfterTaxAdjustment(double fTotValue, double fTaxRate);
ERRSTRUCT FillPerfAssetMergeTable(PERFASSETMERGETABLE *pzPAMTable,
                                  ASSETTABLE2 *pzATable, int iVendorID,
                                  long lLastPerfDate, long lCurrentPerfDate);
// int		  CurrentOrMergedAssetIndex(PERFASSETMERGETABLE zPAMTAble, int
// j);
int CurrentOrMergedAssetIndexForToday(PERFASSETMERGETABLE zPAMTable,
                                      int iAssetIndex, long lDate);
int FindAssetInperfAssetMergTable(PERFASSETMERGETABLE zPAMTable,
                                  int iAssetIndexToFind,
                                  BOOL bFindAssetMergedFrom);

// Perfdll.cpp
ERRSTRUCT InitializeCalcPerfLibrary();
ERRSTRUCT InitializeAppropriateLibrary(char *sDBAlias, char *sErrFile,
                                       char sWhichLibrary, long lAsofDate,
                                       BOOL bOnlyResetErrorFile);
DLLAPI void STDCALL FreePerformanceLibrary();

/* Prototype of functions defined in calcperf5.cpp */
DllExport int AddNewSegment(int iPortID, char *sName, char *sAbbrev,
                            int iSegmentType);
int AddNewSegmentType(int iLevel1, int iLevel2, int iLevel3, char *sName,
                      char *sAbbrev, char *sCode, char *sSecNo, char *sWi,
                      char *sSecDesc1, BOOL bShouldExistInSegmap,
                      BOOL bSingleSecurity);
// This is obsolete code - no UV storage in BLOB anymore - vay, 8/13/03
/*
int  AddReturnToBlob(long iSegmentID, long iReturnType, long lReturnDate,
double fROR, BOOL bStart, BOOL bFinish);
*/
ERRSTRUCT ReadBeginningUnitValues(PKEYTABLE *pzPKTable, int iPortfolioID,
                                  long lEarliestNondDate);
double RORToEndingUV(double fROR, double fBUV);
void CopyUVIndex(ALLRORS *pzUVNew, ALLRORS zUVOld);
long NextPeriod(long lCurrentDate, int iPeriodType);
int FindPerfkeyByIDBackward(PKEYTABLE *pzPKTable, int iID, int iStartIndex);
DllExport void FreeLib(void);
BOOL LoadPerfRule(int iID, long lPerfDate, PERFRULETABLE *pzRuleTable);
void DeletePerfkeyByPerfRule(PERFRULETABLE *pzRuleTable, PKEYTABLE *pzPTable);
//	extern BOOL CalcSelected;
static BOOL CalcSelected;
#endif
