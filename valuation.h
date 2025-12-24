#ifndef VALUATION_H
#define VALUATION_H

#include "accdiv.h"
#include "assets.h"
#include "commonheader.h"
#include "currency.h"
#include "divhist.h"
#include "equities.h"
#include "fixedinc.h"
#include "hedgexref.h"
#include "holdcash.h"
#include "holdings.h"
#include "payrec.h"
#include "portbal.h"
#include "portmain.h"
#include "priceinfo.h"
#include "sectype.h"
#include "syssettings.h"
#include "trans.h"

#define EXTRAPRICE 100
#define EXTRACUSTOMPRICE 10
#define EXTRAHXREF 10
#define lJan312001 36922
#define EXTRAACCDIV 25
#define EXTRASOLDTRANS 10

#ifdef __cplusplus
extern "C" {
#endif

static long lStarsTrdDate = 0;
static long lStarsPricingDate = 0;
extern BOOL bInit;

// extra fields which are not part of portbal record but are required
// to calculate some of the fields in portbal. These are calculated
// using holdings/holdcash/trans records.
typedef struct {
  int iID;
  double fLongCash;
  double fTBillMinusHedge;
  double fShortCash;
  double fShortHoldings;
  double fMMarket;
  double fWhenIssued;
} PBALXTRA;

typedef void(CALLBACK *LPPRSELECTSYSSETTINGS)(SYSSETTING *, ERRSTRUCT *);
typedef double(CALLBACK *LPFNSELECTDIVINT)(char *, char *, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTCURRENCYSECNO)(char *, char *, int *, char *,
                                                ERRSTRUCT *);
typedef BOOL(CALLBACK *LPFNITSMANUALPRICESECURITY)(char *, char *, long,
                                                   ERRSTRUCT *);

extern SECTYPETABLE zSTTable;
extern CURRTABLE zCurrencyTable;
extern SYSTEM_SETTINGS zSystemSettings;

// function variables
extern LPPR3PCHAR1LONG1INT lpprStarsIOInit;
extern LPFNVOID lpfnStartTransaction, lpfnRollbackTransaction,
    lpfnCommitTransaction;
extern LPFNVOID lpfnGetTransCount;
extern LPFN1BOOL lpfnAbortTransaction;
extern LPPRVOID lpprCloseQueries;

extern LPPRSELECTONEACCDIV lpprSelectAccdiv;
extern LPPRALLACCDIVFORANACCOUNT lpprSelectAllAccdivForAnAccount;
extern LPPRPENDINGACCDIVTRANSFORANACCOUNT
    lpprSelectPendingAccdivTransForAnAccount;
extern LPPRACCDIV lpprUpdateAccdiv;

extern LPPRALLDIVHISTFORANACCOUNT lpprSelectAllDivhistForAnAccount;
extern LPPRDIVHIST lpprUpdateDivhist;

extern LPPR2PCHAR1LONG1PDOUBLE1PLONG lpprSelectNextCallDatePrice;

extern LPPRERRSTRUCT lpprInitializeErrStruct;

extern LPPRSELECTPARTEQTY lpprSelectEquity;
extern LPFN1PCHAR lpfnSetErrorFile;
extern LPPRSELECTPARTFINC lpprSelectFixedinc;
extern LPFNCALCGAINLOSS lpfnCalcGainLoss;
extern LPPRSECPRICE lpprGetSecurityPrice;

extern LPPRALLHXREFFORANACCOUNT lpprSelectAllHxrefForAnAccount;
extern LPPRHEDGEXREF lpprUpdateHedgexref;

extern LPPRALLHOLDCASHFORANACCOUNT lpprSelectAllHoldcashForAnAccount;
extern LPPRHOLDCASH lpprUpdateHoldcash;
extern LPPRINITHOLDCASH lpprInitHoldcash;

extern LPPRALLHOLDINGSFORANACCOUNT lpprSelectAllHoldingsForAnAccount;
extern LPPRHOLDINGS lpprUpdateHoldings;
extern LPPRINITHOLDINGS lpprInitHoldings;

extern LPPRALLPAYRECFORANACCOUNT lpprSelectAllPayrecForAnAccount;
extern LPPRPAYREC lpprUpdatePayrec;

extern LPFNPRINTERROR lpfnPrintError;

// extern LPPRSELECTPORTBAL
// lpprSelectPortbal; extern LPPRUPDATEPORTBAL
// lpprUpdatePortbal;

extern LPPRPARTPMAINONE lpprSelectOnePmain;
extern LPPRPARTPMAINALL lpprSelectAllPmain;
extern LPPR1INT1LONG lpprUpdateValDate;

extern LPPRALLSECTYPES lpprSelectAllSectypes;
extern LPPR2PLONG lpprSelectStarsDate;
extern LPPRSELECTTRANS lpprSelectTrans;

extern LPFN1DOUBLE3PCHAR1LONGINT lpfnCalculateYield;

extern LPPRSELECTSYSSETTINGS lpprSelectSysSettings;
extern LPPRSELECTSYSVALUES lpprSelectSysvalues;
extern LPFN1LONG2PCHAR1PLONG lpfnLastBusinessDay, lpfnNextBusinessDay;
extern LPFN1LONG1PCHAR lpfnIsItAMarketHoliday;
extern LPFNSELECTDIVINT lpfnSelectDivint;
extern LPPRSELECTCURRENCYSECNO lpprSelectCurrencySecno;

extern LPPRPPARTCURR lpprSelectPartCurrency;
extern LPFNSELECTDIVINT lpfnSelectSecurityRate;
extern LPFNITSMANUALPRICESECURITY lpfnItsManualPriceSecurity;
// LPFN1INT
// lpfnTimer; LP2PR1PCHAR
// lpprTimerResult;
extern LPFN1PCHAR3LONG lpfnInflationIndexRatio;

extern LPPRASSETS lpprSelectAsset;

extern LPPR1INT4PCHAR2LONG1PDOUBLE lpprSelectUnitsHeldForASecurity;
extern LPPR1INT4PCHAR3LONG1PDOUBLE lpprSelectUnitsForASoldSecurity;
extern LPPRCUSTOMPRICE lpprSelectCustomPriceForAnAccount;

// Prototype of functions defined in value1.c
ERRSTRUCT ValueAnAccount(PARTPMAIN zPmain, PRICETABLE *pzPTable,
                         CUSTOMPRICETABLE *pzCPTable, long lPriceDate,
                         BOOL bPartialValuation);
ERRSTRUCT ValueHedgexref(int iID, long lPriceDate, double fBaseXrate,
                         SECTYPETABLE zSTTable, CUSTOMPRICETABLE zCPTable,
                         PRICETABLE *pzPTable, PHXREFTABLE *pzPXTable);
ERRSTRUCT ValueHoldings(PARTPMAIN zPmain, long lPriceDate, double fBaseXrate,
                        SECTYPETABLE zSTTable, PHXREFTABLE zPXTable,
                        CUSTOMPRICETABLE zCPTable, PORTBAL *pzPbal,
                        PBALXTRA *pzPbxtra, PRICETABLE *pzPTable,
                        BOOL bPartialValuation);
ERRSTRUCT ValueHoldcash(PARTPMAIN zPmain, long lPriceDate, double fBaseXrate,
                        SECTYPETABLE zSTTable, CUSTOMPRICETABLE zCPTable,
                        PORTBAL *pzPbal, PBALXTRA *pzPbxtra,
                        PRICETABLE *pzPTable, BOOL bPartialValuation);
ERRSTRUCT ValuePayrec(int iID, long lPriceDate, double fBaseXrate,
                      SECTYPETABLE zSTTable, CUSTOMPRICETABLE zCPTable,
                      PRICETABLE *pzPTable);
ERRSTRUCT ValuePortbal(int iID, long lPriceDate, double fBaseXrate,
                       SECTYPETABLE zSTTable, PORTBAL zPbal, PBALXTRA zPbxtra,
                       CUSTOMPRICETABLE zCPTable, PRICETABLE *pzPTable);
void UpdatePBFromHoldings(HOLDINGS zHold, SECTYPE zSType, BOOL bCAvail,
                          BOOL bFAvail, PORTBAL *pzPbal, PBALXTRA *pzPbxtra);
void UpdatePBFromHoldcash(HOLDCASH zHcash, BOOL bCAvail, PORTBAL *pzPbal,
                          PBALXTRA *pzPbxtra);
ERRSTRUCT UpdatePBFromAccdiv(ACCDIV zAccdiv, long lPriceDate, double fBaseXrate,
                             SECTYPETABLE zSTTable, PRICETABLE *pzPTable,
                             PORTBAL *pzPbal);
ERRSTRUCT UpdatePBFromTrans(TRANS zTrans, long lPriceDate, double fBaseXrate,
                            SECTYPETABLE zSTTable, PRICETABLE *pzPTable,
                            PORTBAL *pzPbal);
double GetAccruedInterest(const long lPriceDate, HOLDINGS zTempHold,
                          CUSTOMPRICETABLE zCPTable, PRICETABLE *pzPTable,
                          BOOL bLastBusinessDay);
int FindDividendInAccdiv(ACCDIVTABLE zAccdivTable, int iID, BOOL bIncome,
                         long lDivintNo, long lTransNo, char *sSecNo, char *sWi,
                         char *sSecXtend, char *sAcctType, long lEffDate);
double StraightLineDiscountedAccrual(double fTotCost, double fParAmount,
                                     long lStlDate, long lMaturityDate,
                                     long lPricingDate);
ERRSTRUCT GetTotalUnitsEligibleForDividend(int iID, char *sSecNo, char *sWi,
                                           char *sSecXtend, char *sAcctType,
                                           long lValDate, long lRecDate,
                                           long lPayDate, double *pfUnits);

// Prototype of functions defined in value2.c
void InitializeParthxref(PARTHXREF *pzPH);
void InitializePhxrefTable(PHXREFTABLE *pzPTable);
void InitializeUpdatableFieldsInPortbal(PORTBAL *pzPbal);
void InitializePortbalXtra(PBALXTRA *pzPbXtra);
void InitializePriceTable(PRICETABLE *pzPTable);
ERRSTRUCT BuildSecTypeTable(SECTYPETABLE *pzSTable);
int FindSectype(SECTYPETABLE zSTable, int iSecType);
int FindPrice(char *sSecNo, char *sWi, long lPriceDate, PRICETABLE zPTable);
ERRSTRUCT GetSecurityPrice(char *sSecNo, char *sWi, long lPriceDate,
                           SECTYPETABLE zSTable, int iID,
                           CUSTOMPRICETABLE zCPTable, PRICETABLE *pzPTable,
                           PRICEINFO *pzPrice);
ERRSTRUCT AddHedgexrefToTable(PHXREFTABLE *pzHxrefTable, PARTHXREF zAddHxref);
ERRSTRUCT AddPriceToTable(PRICETABLE *pzPTable, PRICEINFO zPInfo);
void CreateTransFromHoldings(HOLDINGS zHold, TRANS *pzTrans, long lPriceDate,
                             long *plSecImpact);
void CreateTransFromHoldcash(HOLDCASH zHcash, TRANS *pzTrans, long lPriceDate,
                             long *plSecImpact);
ERRSTRUCT LoadSystemSettings(SYSTEM_SETTINGS *pzSysSetng);
ERRSTRUCT LoadPartCurrencyTable(CURRTABLE *pzCurrTable);
ERRSTRUCT GetCurrencySecNo(char *SecNo, char *Wi, char sBaseCurrId[]);
double GetAccruedInterestForLastBusinessDay(const long lPriceDate,
                                            HOLDINGS zTempHold);
void InitializeCustomPriceTable(CUSTOMPRICETABLE *pzCPTable);
ERRSTRUCT AddCustomPriceToTable(CUSTOMPRICETABLE *pzCPTable,
                                CUSTOMPRICEINFO zCPInfo);
ERRSTRUCT GetAllCustomPrice(CUSTOMPRICETABLE *pzCPTable, int iCustomTypeID,
                            long lPriceDate);
void FreeValuation();

/*void InitializeSoldTransTable(SOLDTRANSTABLE *pzSTTable);
ERRSTRUCT FillSoldTransTable(int iID, long lValDate, SOLDTRANSTABLE *pzSTTable);
void InitializeSoldTrans(SOLDTRANS *pzSTrans);
ERRSTRUCT AddSoldTransToTable(SOLDTRANSTABLE *pzSTTable, SOLDTRANS zSTrans);
*/

// 2001-3-6 Added for 0 Pricing and Accr_int in the future - GK
void InitializeAccdivTable(ACCDIVTABLE *pzADTable);
void InitializeAssetTable(ASSETTABLE *pzATable);
void InitializePartialAccdiv(PARTACCDIV *pzPAccdiv);
ERRSTRUCT AddAccdivToTable(ACCDIVTABLE *pzAccdivTable, PARTACCDIV zPAccdiv);
ERRSTRUCT FillAccdivTable(int iID, BOOL bIncome, BOOL bIncByLot,
                          long lPriceDate, ASSETTABLE *pzATable,
                          ACCDIVTABLE *pzAccdivTable);
// down to here

#ifdef __cplusplus
}
#endif

#endif // VALUATION_H
