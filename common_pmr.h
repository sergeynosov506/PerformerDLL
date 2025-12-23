/*
*
* FILENAME: common_pmr.h
*
* DESCRIPTION: Common include files for TransactionEngine, Payments, etc.
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Shobhit Barman
*
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2009-03-04 VI# 41539: added accretion functionality - mk
*/

#ifndef COMMONHEADER
#define DllImport __declspec(dllimport)
#define DLLAPI __declspec(dllexport)

#define COMMONHEADER 1

#define NT 1            // null terminator
#define NUMSUBACCT 15   // maximum number of records in subaccount table
#define NUMTRANTYPE 90  // maximum number of trantype records
#define NUMSECTYPES 225 // maximum number of sectype records
#define NUMCURRENCY 200 // maximum number of currency records
#define NUMWITHRECL 200 // maximum number of withhold recliam records

#define DEFAULTMMLOT 9999999 // default money market lot #

#define SQLNOTFOUND 100 // error code for record not found
#define ERR_INVALIDFILENAME -100
#define ERR_INVALIDFUNCTION -101

// different values for primary type field on sectype table
#define PTYPE_FUTURE 'F'
#define PTYPE_EQUITY 'E'
#define PTYPE_BOND 'B'
#define PTYPE_CASH 'C'
#define PTYPE_OPTION 'O'
#define PTYPE_MMARKET 'M'
#define PTYPE_UNITS 'U'

// different values for secondary type field on sectype table
#define STYPE_TBILLS 'T'

// different account type on subacct table
#define ACCTYPE_IHLONG '0'
#define ACCTYPE_OHLONG 'A'
#define ACCTYPE_IHSHORT '5'
#define ACCTYPE_OHSHORT 'B'
#define ACCTYPE_WHENISSUE 'W'
#define ACCTYPE_IHINCOME '3'
#define ACCTYPE_OHINCOME 'N'
#define ACCTYPE_FUTURE 'F'
#define ACCTYPE_SUSPENDED '6'
#define ACCTYPE_NONPURPOSELOAN '4'

#define NUMRORTYPE 7

#define GTWRor 1
#define GPcplRor 2
#define NTWRor 3
#define NPcplRor 4
#define IncomeRor 5
#define GDWRor 6
#define NDWRor 7

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "accdiv.h"
#include "assets.h"
#include "divhist.h"
#include "divint.h"
#include "dtransdesc.h"
#include "hedgexref.h"
#include "hkeyrltn.h"
#include "holdcash.h"
#include "holddel.h"
#include "holdings.h"
#include "holdtot.h"
#include "payrec.h"
#include "perfctrl.h"
#include "perfkey.h"
#include "perfrule.h"
#include "portbal.h"
#include "portmain.h"
#include "porttax.h"
#include "pscrdet.h"
#include "pscrhdr.h"
#include "ptmpdet.h"
#include "ptmphdr.h"
#include "rtrnset.h"
#include "sectype.h"
#include "subacct.h"
#include "summdata.h"
#include "taxperf.h"
#include "trans.h"
#include "trantype.h"
#include "withhold_rclm.h"

#define ADHOCDATE 0   // 12/30/1899
#define PERFORMDATE 1 // 12/31/1899

// Following defines are required for InitializeStarsIO functions(to tell
// the functions which set of queries should be prepared at the time of
// initialization) and their value should match to that in
// Delphi(StarsIODefinitions)
#define PREPQRY_NONE 0
#define PREPQRY_TRANSENGINE 1
#define PREPQRY_DIGENERATE 2
#define PREPQRY_DIPAY 3
#define PREPQRY_MATURITY 4
#define PREPQRY_AMORTIZE 5
#define PREPQRY_ROLL 6
#define PREPQRY_PERFORM 7
#define PREPQRY_VALUATION 8
#define PREPQRY_PAYMENTS 20 // digenerate + dipay + maturity
#define PREPQRY_ALL 100

// Portfolio type should match to those defined in Delphi (MercuryTypesUnit)
enum PortfolioType { ptUknown, ptComposite, ptFamily, ptIndividual, ptModel };

/* error structure used by all the libraries */
typedef struct {
  int iID;
  long lRecNo; /* RecType - D(Dtransno), F(perfFormno), H(templateHdrno) */
  char sRecType[1 +
                NT];  /* I(divIntno), P(Perfkey), R(perfRule), S(Scripthdrno) */
  int iBusinessError; /* or T(TemplatehdrNo) */
  int iSqlError;
  int iIsamCode;
} ERRSTRUCT;

typedef struct {
  char sCurrId[4 + NT];
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  double fCurrExrate;
} PARTCURR;

typedef struct {
  int iNumCurrency;
  PARTCURR zCurrency[NUMCURRENCY];
} CURRTABLE;

typedef struct {
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  double fEps;
  BOOL bRecordFound;
} PARTEQTY;

typedef struct {
  int iEqtyCreated;
  int iNumEqty;
  PARTEQTY *pzEqty;
} EQTYTABLE;

typedef struct {
  long lDate;
  double fMktVal;
  double fBookValue;
  double fAccrInc;
  double fAccrDiv;
  double fNetFlow;
  double fTodayFlow;
  double fCumFlow;
  double fWtdFlow;
  double fPurchases;
  double fSales;
  double fIncome;
  double fTodayIncome;
  double fCumIncome;
  double fWtdInc;
  double fFees;
  double fTodayFees;
  double fCumFees;
  double fWtdFees;
  double fExchRateBase;
  long lDaysSinceNond;
  BOOL bPeriodEnd; /* is it a sub-period end(10 % flow date or period end) */
  BOOL bGotMV;
} DAILYINFO;

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
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  long lMaturityDate;
  double fRedemptionPrice;
  char sFlatCode[1 + NT];
  char sPayType[1 + NT];
  long lDatedDate;
  char sAmortFlag[1 + NT];
  char sAccretFlag[1 + NT];
  char sOriginalFace[1 + NT];
  double fCurYld;
  double fCurYtm;
  BOOL bRecordFound;
} PARTFINC;

typedef struct {
  int iFincCreated;
  int iNumFinc;
  PARTFINC *pzFinc;
} FINCTABLE;

typedef struct {
  int iID;
  short iFiscalMonth;
  short iFiscalDay;
  long lInceptionDate;
  char sAcctMethod[1 + NT];
  char sBaseCurrId[4 + NT];
  BOOL bIncome;
  BOOL bActions;
  BOOL bMature;
  BOOL bCAvail;
  BOOL bFAvail;
  long lValDate;
  long lDeleteDate;
  BOOL bIsInactive;
  BOOL bAmortMuni;
  BOOL bAmortOther;
  long lAmortStart;
  BOOL bIncByLot;
  long lPurgeDate;
  long lRollDate;
  char sTax[1 + NT];
  int iPortfolioType;
  long lPricingEffectiveDate;
  int iCurrIndex; // points to the entry for basecurrid in currency table
  double fMaxEqPct;
  double fMaxFiPct;
} PARTPMAIN;

typedef struct {
  int iPmainCreated;
  int iNumPmain;
  PARTPMAIN *pzPmain;
} PORTTABLE;

typedef struct {
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  long lPriceDate;
  long lDatePriceUpdated;
  long lDateExrateUpdated;
  char sPriceSource[1 + NT];
  double fClosePrice;
  double fExrate;
  double fIncExrate;
  double fAccrInt;
  double fAnnDivCpn;
  // it is possible that the price of a security is not found, but its exchange
  // rate is found, next flag is to indicate whether the price on this record is
  // valid or not
  BOOL bIsPriceValid;
  BOOL bIsExrateValid;
  char sCurrId[4 + NT];
  char sIncCurrId[4 + NT];
  double fTradUnit;
  short iSecType;
  int iSTypeIdx; // location of this asset's sectype in the sectype table
} PRICEINFO;

typedef struct {
  // Even though write now there is no check, all the records in the table
  // should be for price date
  long lPriceDate;
  int iPriceCreated;
  int iNumPrice;
  PRICEINFO *pzPrice;
} PRICETABLE;

typedef struct {
  char sSecNo[13];
  char sWhenIssue[2];
  int iSecType;
  char sCurrId[5];
  char sIncCurrId[5];
  int iIndustLevel1;
  int iIndustLevel2;
  int iIndustLevel3;
  char sSpRating[5];
  char sMoodyRating[5];
  char sNbRating[5];
  char sTaxStat[2];
  double fAnnDivCpn;
  double fCurExrate;
  int iSTypeIndex;
  short iLongShort;
} PARTASSET;

typedef struct {
  int iNumSType;
  SECTYPE zSType[NUMSECTYPES];
} SECTYPETABLE;

typedef struct {
  char sAcctType[1 + NT];
  char sXrefAcctType[1 + NT];
} PARTSUBACCT;

typedef struct {
  int iNumSAcct;
  PARTSUBACCT zSAcct[NUMSUBACCT];
} SUBACCTTABLE;

typedef struct {
  char sCurrId[5];
  char sSecNo[13];
  char sWi[2];
} PARTCURRENCY;

typedef struct {
  int iID;
  char sSecNo[13];
  char sWi[2];
  char sSecXtend[3];
  long lTransNo;
  double fTotCost;
  double fMktVal;
  double fMvBaseXrate;
  double fAccrInt;
  double fAiBaseXrate;
  BOOL bHoldCash; /*Currently Unused, TRUE-from holdcash,FALSE-from holdings*/
  int iAssetIndex;
} PARTHOLDING;

typedef struct {
  double fBPcplSecFlow;    /* base security flow */
  double fLPcplSecFlow;    /* local security flow */
  double fBIncomeSecFlow;  /* base security flow */
  double fLIncomeSecFlow;  /* local security flow */
  double fBPcplCashFlow;   /* base principal cash flow */
  double fLPcplCashFlow;   /* local principal cash flow */
  double fBAccrualFlow;    /* base accrual flow */
  double fLAccrualFlow;    /* local accrual flow */
  double fBIncomeCashFlow; /* base income cash flow */
  double fLIncomeCashFlow; /* local income cash flow */
  double fBSecFees;        /* base security fees */
  double fLSecFees;        /* local security fees */
  double fBSecIncome;      /* base security income */
  double fLSecIncome;      /* local security income */
} FLOWCALCSTRUCT;          /* flow calculator structure */

typedef struct {
  char sTranType[3];
  char sDrCr[3];
  char sTranCode[2];
  long lSecImpact;
  char sPerfImpact[2];
} PARTTRANTYPE;

typedef struct {
  int iNumRorType;
  RTRNSET zIndex[NUMRORTYPE];
  BOOL bIndexPopulated[NUMRORTYPE]; /* NOT USED ANY MORE */
} ALLRORS;

typedef struct {
  int iID;
  char sPortComp[2];
  char sBaseCurrId[5];
  long lValDate;
  long lDeleteDate;
  long lInceptDate;
  long lFiscal;
} PARTPORTDIR;

typedef struct {
  PERFKEY zPK;
  char sRecordType[2]; // W - Weighted, E - Equity + Equity CASH, F - FI + FI
                       // CASH
  long lParentRuleNo;
  int iScrHDIndex; /* script header-detail index */
  double fBeginMV;
  double fBeginAI;
  double fBeginAD;
  double fBeginInc;
  double fBegFedataxAI;
  double fBegFedataxAD;
  double fBegFedetaxAI;
  double fBegFedetaxAD;
  double fBegStataxAI;
  double fBegStataxAD;
  double fBegStetaxAI;
  double fBegStetaxAD;
  BOOL bGotBeginMV; /* Have it been read yet ? */
  int iDInfoCreated;
  int iNumDInfo;
  DAILYINFO *pzDInfo;
  TAXPERF *pzTInfo;
  int iWDInfoCreated;
  int iNumWDInfo;
  WTDDAILYINFO *pzWDInfo;
  double fAbs10PrcntMV;
  long lScratchDate; // date in daily db which can be over-written with current
                     // dsumdata/drtrnset records
  ALLRORS zBeginIndex;
  ALLRORS zNewIndex;
  // BOOL         bWtdKey; /*does perfrule associated with it has WtdRecInd =
  // W*/
  BOOL bNewKey;        /* did Key get created this time ? */
  BOOL bDeleteKey;     /*Is key going to be deleted this time(EndMV = 0)*/
  BOOL bKeyCopied;     /* Another copy of the key created this time ? */
  BOOL bDeletedFromDB; /* Deleted from database table but still here*/
} PKEYSTRUCT;

typedef struct {
  int iPKeyCreated;
  int iNumPKey;
  PKEYSTRUCT *pzPKey;
  char sTaxCalc[2]; /* same as perfctrl.sTaxCalc */
} PKEYTABLE;

typedef struct {
  int iID;
  long lTransNo;
  char sTranType[3];
  char sSecNo[13];
  char sWi[2];
  char sSecXtend[3];
  char sAcctType[2];
  double fPcplAmt;
  double fAccrInt;
  double fIncomeAmt;
  double fNetFlow;
  long lTrdDate;
  long lStlDate;
  long lEntryDate;
  char sRevType[3];
  long lPerfDate;
  char sXSecNo[13];
  char sXWi[2];
  char sXSecXtend[3];
  char sXAcctType[2];
  char sCurrId[5];
  char sCurrAcctType[2];
  char sIncCurrId[5];
  char sXCurrId[5];
  char sXCurrAcctType[2];
  double fBaseXrate;
  double fIncBaseXrate;
  double fSecBaseXrate;
  double fAccrBaseXrate;
  char sDrCr[3];
  BOOL bReversal;
  int iSecAssetIndex;
  int iCashAssetIndex;
  int iIncAssetIndex;
  int iXSecAssetIndex;  /* XSec and XCash will be filled only for Xfer */
  int iXCashAssetIndex; /* trades(TS and TC) */
  int iTranTypeIndex;
} PARTTRANS;

typedef struct {
  int iSecType;
  char sPrimaryType[2];
  char sSecondaryType[2];
  char sMktValInd[2];
} PARTSTYPE;

// typedefine all the function which are dynamically loaded from other Dlls.
typedef int(CALLBACK *LPFN1INT)(int);
typedef void(CALLBACK *LPPR1INT1LONG)(int, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1INT2LONG1PCHAR)(int, long, long, char *,
                                            ERRSTRUCT *);
typedef int(CALLBACK *LPFN1INT2PCHAR1INT)(int, char *, char *, int);
typedef void(CALLBACK *LPPR1INT3LONG1PCHAR)(int, long, long, long, char *,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPR1INT4PCHAR1LONG)(int, char *, char *, char *, char *,
                                            long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1INT4PCHAR2LONG)(int, char *, char *, char *, char *,
                                            long, long, ERRSTRUCT *);
typedef int(CALLBACK *LPFN3INT3PCHAR1BOOL)(int, int, int, char *, char *,
                                           char *, BOOL);

typedef void(CALLBACK *LPPR1PINT3LONG2PCHAR)(int *, long, long, long, char *,
                                             char *, ERRSTRUCT *);

typedef void(CALLBACK *LPPR1LONG)(long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG1PCHAR)(long, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG1PCHAR1BOOL)(long, char *, BOOL, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG1PCHAR1LONG)(long, char *, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG1PCHAR1PLONG)(long, char *, long *,
                                              ERRSTRUCT *);
typedef ERRSTRUCT(CALLBACK *LPFN1LONG2PCHAR)(long, char *, char *);
typedef void(CALLBACK *LPPR1LONG2PCHAR)(long, char *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG3PCHAR1BOOL)(long, char *, char *, char *, BOOL,
                                             ERRSTRUCT *);
typedef ERRSTRUCT(CALLBACK *LPFN1LONG3PCHAR1BOOL1PCHAR)(long, const char *,
                                                        const char *,
                                                        const char *, BOOL,
                                                        const char *);
typedef void(CALLBACK *LPPR1LONG4PCHAR)(long, char *, char *, char *, char *,
                                        ERRSTRUCT *);
typedef ERRSTRUCT(CALLBACK *LPFN1LONG4PCHAR)(long, char *, char *, char *,
                                             char *);
typedef void(CALLBACK *LPPR1LONG4PCHAR1BOOL)(long, char *, char *, char *,
                                             char *, BOOL, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG6PCHAR)(long, char *, char *, char *, char *,
                                        char *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1LONG7PCHAR)(long, char *, char *, char *, char *,
                                        char *, char *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR2LONG)(long, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR2LONG1PCHAR)(long, long, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR3LONG)(long, long, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR3LONG1PCHAR)(long, long, long, char *, ERRSTRUCT *);
typedef int(CALLBACK *LPFN3LONG1DOUBLE2BOOL)(long, long, long, double, BOOL,
                                             BOOL);
typedef void(CALLBACK *LPPR3LONG2PCHAR1BOOL1PINT)(long, long, long, char *,
                                                  char *, BOOL, int *,
                                                  ERRSTRUCT *);
typedef void(CALLBACK *LPPR3LONG2PCHAR1BOOL1PTRANS)(long, long, long, char *,
                                                    char *, BOOL, TRANS *,
                                                    ERRSTRUCT *);

typedef void(CALLBACK *LPPR2PLONG)(long *, long *, ERRSTRUCT *);

typedef int(CALLBACK *LPFN1PCHAR1INT)(char *, int);
typedef void(CALLBACK *LPPR1PLONG1LONG)(long *, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PLONG2PCHAR)(long *, char *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PLONG2PCHAR1LONG)(long *, char *, char *, long,
                                              ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PLONG1PCHAR)(long *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PLONG1PCHAR2LONG)(long *, char *, long, long,
                                              ERRSTRUCT *);

typedef int(CALLBACK *LPFN1PCHAR)(const char *);
typedef void(CALLBACK *LPPR1PCHAR)(char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PCHAR1LONG)(char *, long, ERRSTRUCT *);
typedef ERRSTRUCT(CALLBACK *LPFN1PCHAR1INT1LONG1BOOL)(char *, int, long, BOOL);
typedef void(CALLBACK *LPPR1PCHAR2PINT1INT)(char *, int *, int *, int,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PCHAR1LONG2PCHAR)(char *, long, char *, char *,
                                              ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PCHAR1INT4PHAR1LONG)(const char *, int,
                                                 const char *, const char *,
                                                 const char *, const char *,
                                                 long, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PCHAR2LONG)(char *, long, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR1PCHAR3LONG1PCHAR)(char *, long, long, long, char *,
                                              ERRSTRUCT *);
typedef void(CALLBACK *LPPR2PCHAR)(char *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR2PCHAR3LONG1PCHAR)(char *, char *, long, long, long,
                                              char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR2PCHAR4LONG1PCHAR)(char *, char *, long, long, long,
                                              long, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR3PCHAR)(char *, char *, char *, ERRSTRUCT *);
typedef int(CALLBACK *LPFN3PCHAR)(char *, char *, char *);
typedef void(CALLBACK *LPPR3PCHAR1LONG)(char *, char *, char *, long,
                                        ERRSTRUCT *);
typedef void(CALLBACK *LPPR3PCHAR1LONG1BOOL)(char *, char *, char *, long, BOOL,
                                             ERRSTRUCT *);
typedef void(CALLBACK *LPPR3PCHAR1LONG1INT)(char *, char *, char *, long, int,
                                            ERRSTRUCT *);
typedef ERRSTRUCT(CALLBACK *LPFN3PCHAR1LONG1PCHAR)(char *, char *, char *, long,
                                                   char *);
typedef void(CALLBACK *LPPR5PCHAR)(char *, char *, char *, char *, char *,
                                   ERRSTRUCT *);
typedef void(CALLBACK *LPPR5PCHAR1LONG)(char *, char *, char *, char *, char *,
                                        long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR5PCHAR2LONG)(char *, char *, char *, char *, char *,
                                        long, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPR8PCHAR1PLONG)(char *, char *, char *, char *, char *,
                                         char *, char *, char *, long *,
                                         ERRSTRUCT *);

typedef void(CALLBACK *LPPR5PDBL5PCHAR1LONG)(double *, double *, double *,
                                             double *, double *, char *, char *,
                                             char *, char *, char *, long,
                                             ERRSTRUCT *);

typedef int(CALLBACK *LPFNVOID)();
typedef void(CALLBACK *LPPRVOID)();

// accdiv related function
// typedef void(CALLBACK *LPPRACCDIV)(ACCDIV, ERRSTRUCT *);
// typedef void(CALLBACK *LPPRSELECTONEACCDIV)(ACCDIV *, int, long, long,
//                                             ERRSTRUCT *);
// typedef void(CALLBACK *LPPRSELECTALLACCDIV)(ACCDIV *, int *, double *, double
// *,
//                                             double *, const char *,
//                                             const char *, int, const char *,
//                                             const char *, const char *,
//                                             const char *, long, long,
//                                             ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLACCDIVFORANACCOUNT)(int, ACCDIV *, ERRSTRUCT *);

// assets related function
typedef void(CALLBACK *LPPRASSETS)(ASSETS *, char *, char *, ERRSTRUCT *);

// currency related functions
typedef void(CALLBACK *LPPRSELECTALLPARTCURRENCIES)(PARTCURRENCY *,
                                                    ERRSTRUCT *);

// date related functions
typedef int(CALLBACK *LPFNRMDYJUL)(short[], long *);
typedef int(CALLBACK *LPFNRJULMDY)(long, short[]);
typedef int(CALLBACK *LPFNNEWDATE)(long, BOOL, int, int, int, long *);

// divhist related function
typedef void(CALLBACK *LPPRDIVHIST)(DIVHIST *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRUPDATEDIVHIST)(int, long, long, double, long, long,
                                          ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLDIVHISTFORANACCOUNT)(int, DIVHIST *, ERRSTRUCT *);

// dtrans related functions
typedef void(CALLBACK *LPPRDTRANSSELECT)(TRANS *, int, char *, char *, long,
                                         ERRSTRUCT *);
typedef void(CALLBACK *LPPRDTRANSUPDATE)(int, const char *, const char *, long,
                                         const char *, int, int, int, long,
                                         const char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRDTRANSDESCSELECT)(DTRANSDESC *, int, long,
                                             ERRSTRUCT *);

// equities related function
typedef void(CALLBACK *LPPRSELECTPARTEQTY)(char *, char *, PARTEQTY *,
                                           ERRSTRUCT *);

// Fetch Holdings map cursor
typedef void(CALLBACK *LPPRFETCHHMAPCURSOR)(long *, char *, char *, char *,
                                            char *, char *, ERRSTRUCT *);

// fixedinc related function
typedef void(CALLBACK *LPPRSELECTPARTFINC)(char *, char *, PARTFINC *,
                                           ERRSTRUCT *);

// errstruct related function
typedef void(CALLBACK *LPPRERRSTRUCT)(ERRSTRUCT *);

// fixedinc related function
typedef void(CALLBACK *LPPRSELECTPARTFINC)(char *, char *, PARTFINC *,
                                           ERRSTRUCT *);

// gain-loss related functions
typedef int(CALLBACK *LPFNCALCGAINLOSS)(TRANS, char *, long, char *, char *,
                                        char *, double *, double *, double *,
                                        double *, double *, double *);

// histpric related functions
typedef void(CALLBACK *LPPRSECPRICE)(char *, char *, long, PRICEINFO *,
                                     ERRSTRUCT *);

// hedgexref related functions
typedef void(CALLBACK *LPPRHEDGEXREF)(HEDGEXREF, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHEDGEXREF)(HEDGEXREF *, int, char *, char *,
                                            char *, char *, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLHXREFFORANACCOUNT)(int, HEDGEXREF *, ERRSTRUCT *);

// holdcah related functions
typedef void(CALLBACK *LPPRHOLDCASH)(HOLDCASH, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHOLDCASH)(HOLDCASH *, int, char *, char *,
                                           char *, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLHOLDCASHFORANACCOUNT)(int, HOLDCASH *,
                                                    ERRSTRUCT *);

// holddel related functions
typedef void(CALLBACK *LPPRHOLDDEL)(HOLDDEL, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHOLDDEL)(HOLDDEL *, long, long, int, char *,
                                          char *, char *, char *, long,
                                          ERRSTRUCT *);

// holdings related functions
typedef void(CALLBACK *LPPRHOLDINGS)(HOLDINGS, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHOLDINGS)(HOLDINGS *, int, char *, char *,
                                           char *, char *, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLHOLDINGSFORANACCOUNT)(int, HOLDINGS *,
                                                    ERRSTRUCT *);
typedef void(CALLBACK *LPPRHOLDINGSPOINTER)(HOLDINGS *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRHSUMSELECT)(double *, double *, double *, double *,
                                       double *, double *, double *, double *,
                                       int, const char *, const char *,
                                       const char *, const char *, long,
                                       ERRSTRUCT *);
typedef void(CALLBACK *LPPRHSUMUPDATE)(double, double, double, double, double,
                                       double, const char *, long, const char *,
                                       int, const char *, const char *,
                                       const char *, const char *, long,
                                       ERRSTRUCT *);

// payrec related functions
typedef void(CALLBACK *LPPRPAYREC)(PAYREC, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPAYREC)(PAYREC *, int, char *, char *, char *,
                                         char *, long, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLPAYRECFORANACCOUNT)(int, PAYREC *, ERRSTRUCT *);

// portbal related functions
typedef void(CALLBACK *LPPRSELECTPORTBAL)(PORTBAL *, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRUPDATEPORTBAL)(PORTBAL, ERRSTRUCT *);

// portmain related functions
typedef void(CALLBACK *LPPRPORTMAIN)(PORTMAIN *, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRPMAINPOINTER)(PORTMAIN *);
typedef void(CALLBACK *LPPRPARTPMAINALL)(PARTPMAIN *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRPARTPMAINONE)(PARTPMAIN *, int, ERRSTRUCT *);

// porttax related functions
typedef void(CALLBACK *LPPRPORTTAX)(int, long, PORTTAX *, ERRSTRUCT *);

// printerror function
typedef ERRSTRUCT(CALLBACK *LPFNPRINTERROR)(const char *, int, long,
                                            const char *, int, int, int,
                                            const char *, BOOL);

// RTRNSET RELATED FUNCTIONS
typedef void(CALLBACK *LPPRROR)(RTRNSET, ERRSTRUCT *);
typedef void(CALLBACK *LPPRRTRNSET1LONG)(RTRNSET, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRRTRNSETPOINTER1INT)(RTRNSET *, int, ERRSTRUCT *);
//  typedef void      (CALLBACK* LPPRSELECTDAILYROR)(RTRNSET *, int, ERRSTRUCT
//  *);
typedef void(CALLBACK *LPPRSELECTPERIODROR)(RTRNSET *, long, long, ERRSTRUCT *);

// script header and detait related functions
typedef void(CALLBACK *LPPRSELECTALLSCRIPTHEADERANDDETAILS)(PSCRHDR *,
                                                            PSCRDET *,
                                                            ERRSTRUCT *);

// sectype related functions
typedef void(CALLBACK *LPPRSECCHAR)(char *, char *, char *, char *, char *,
                                    char *, char *, double *, char *, char *,
                                    char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSECTYPE)(SECTYPE *, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLSECTYPES)(SECTYPE *, ERRSTRUCT *);

// summdata/dsumdata related functions
typedef void(CALLBACK *LPPRSUMMDATA)(SUMMDATA, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSUMMDATA1LONG)(SUMMDATA, long, ERRSTRUCT *);

// trans related functions
typedef void(CALLBACK *LPPRTRANS)(TRANS, ERRSTRUCT *);
typedef void(CALLBACK *LPPRTRANSPOINTER)(TRANS *, ERRSTRUCT *);
typedef void(CALLBACK *LPPR2TRANSPOINTER)(TRANS *);
typedef void(CALLBACK *LPPRSELECTTRANS)(int, long, TRANS *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRTRANSSELECT)(TRANS *, char *, int *, char *, int,
                                        long, long, ERRSTRUCT *);

// transdesc related function
typedef void(CALLBACK *LPPRTRANSDESC)(
    DTRANSDESC, ERRSTRUCT *); // use DtransDesc as TransDesc

// trantype related functions
typedef void(CALLBACK *LPPRTRANTYPE)(TRANTYPE, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTTRANTYPE)(TRANTYPE *, const char *,
                                           const char *, ERRSTRUCT *);

// tranalloc function
typedef ERRSTRUCT(CALLBACK *LPFNTRANALLOC)(TRANS *, TRANTYPE, SECTYPE, ASSETS,
                                           DTRANSDESC[], int, PAYTRAN *,
                                           const char *, BOOL);

// updatehold function
typedef ERRSTRUCT(CALLBACK *LPFNUPDATEHOLD)(TRANS, PORTMAIN, DTRANSDESC[], int,
                                            char *, BOOL);

// withrclm related functions
typedef void(CALLBACK *LPPRWITHHOLDRCLM)(WITHHOLDRCLM *, ERRSTRUCT *);

typedef void(CALLBACK *LPPRPDTRANSDESC)(DTRANSDESC *);
typedef void(CALLBACK *LPPRALLSECTYPE)(SECTYPE *, ERRSTRUCT *);

typedef void(CALLBACK *LPPRINITTRANS)(TRANS *);
typedef void(CALLBACK *LPPRSELECTALLPARTTRANTYPE)(PARTTRANTYPE *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTALLTEMPLATEDETAILS)(PTMPHDR *, PTMPDET *,
                                                     ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPERFCTRL)(PERFCTRL *, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRUPDATEPERFCTRL)(PERFCTRL, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTALLPERFRULE)(PERFRULE *, int, long,
                                              ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPERFKEYS)(PERFKEY *, int, ERRSTRUCT *);
// 	typedef void      (CALLBACK* LPPRMONTHLYWEIGHTEDINFO)(SUMMDATA *,
// RTRNSET *, long, long,  ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPERFORMANCEHOLDINGS)(PARTHOLDING *, int,
                                                      char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPERFORMANCEHOLDCASH)(PARTHOLDING *, int,
                                                      char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPERFORMANCETRANSACTION)(PARTTRANS *, int, long,
                                                         long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTDAILYSUMMDATA)(SUMMDATA *, long, long,
                                                ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPERIODSUMMDATA)(SUMMDATA *, long, long, long,
                                                 ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTALLPARTSECTYPE)(PARTSTYPE *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTONEPARTPORTMAIN)(PARTPMAIN *, long,
                                                  ERRSTRUCT *);
typedef void(CALLBACK *LPPRREADALLHOLDMAP)(char *, char *, char *, char *,
                                           char *, char *, long *, ERRSTRUCT *);

typedef void(CALLBACK *LPPRDELETEPERFKEY)(long, ERRSTRUCT *);

typedef void(CALLBACK *LPPRINSERTUPDATESUMMDATA)(SUMMDATA, ERRSTRUCT *);

// typedef void      (CALLBACK* LPPRDELETESUMMDATARTRNSET)(long, long, ERRSTRUCT
// *);

typedef void(CALLBACK *LPPRSELECTSTARSDATE)(long *, ERRSTRUCT *);

typedef void(CALLBACK *LPPRINSERTPERFKEY)(PERFKEY, ERRSTRUCT *);

typedef void(CALLBACK *LPPRUPDATEPERFKEY)(PERFKEY, ERRSTRUCT *);

typedef void(CALLBACK *LPPRDELETEPERFKEY)(long, ERRSTRUCT *);

typedef long(CALLBACK *LPFNMONTHEND)(long);

// typedef void      (CALLBACK* LPPRDELETERTRNSET)(long, long, ERRSTRUCT *);

typedef void(CALLBACK *LPPRPERFSCRIPTHEADER)(PSCRHDR, ERRSTRUCT *);

typedef void(CALLBACK *LPPRPERFSCRIPTDETAIL)(PSCRDET, ERRSTRUCT *);

typedef ERRSTRUCT(CALLBACK *LPFNCALCFLOW)(TRANS, char *, FLOWCALCSTRUCT *);

typedef ERRSTRUCT(CALLBACK *LPFNROLL)(int, char *, char *, long, long, char *,
                                      char *, BOOL, int, long, BOOL);

typedef void(CALLBACK *LPPRINSERTHOLDTOT)(HOLDTOT, ERRSTRUCT *);

typedef void(CALLBACK *LPPRDELETEHOLDTOT)(int, ERRSTRUCT *);

typedef void(CALLBACK *LPPRDELETEHKEYRLTN)(long, int, ERRSTRUCT *);

typedef void(CALLBACK *LPPRINSERTHKEYRLTN)(HKEYRLTN, ERRSTRUCT *);

typedef ERRSTRUCT(CALLBACK *LPPRTESTASSET)(PARTASSET zPartAsset,
                                           PSCRDET *pPerfScrDetail,
                                           int iNumDetail, BOOL bSecCurr,
                                           long lDate, BOOL *pbResult);

typedef ERRSTRUCT(CALLBACK *LPPRINITPERFORMANCE)(long lAsofDate, char *sDBAlias,
                                                 char *sMode, char *sType,
                                                 BOOL bPrepareQueries,
                                                 char *sErrFile);

typedef void(CALLBACK *LPPRDELETEHOLDCASH)(int, char *, char *, char *, char *,
                                           ERRSTRUCT *);

typedef void(CALLBACK *LPPRDELETEHOLDINGS)(int, char *, char *, char *, char *,
                                           long, ERRSTRUCT *);

typedef void(CALLBACK *LPPRSELECTCOMPOSITE)(int, long, int *, ERRSTRUCT *);

typedef void(CALLBACK *LPPRDELETEHOLDINGSHOLDCASH)(int, char *, char *, char *,
                                                   BOOL, ERRSTRUCT *);

typedef BOOL(CALLBACK *LPFNISITAMONTHEND)(long);
typedef long(CALLBACK *LPFNLASTMONTHEND)(long);
typedef void(CALLBACK *LPPRUPDATEPERFDATE)(int, long, long);
#endif // if commonheader is not defined