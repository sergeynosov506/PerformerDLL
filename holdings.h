/*H*
 *
 * FILENAME: holdings.h
 *
 * DESCRIPTION:
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR:
 *
 *
 *H*/
// 2010-11-19 VI# 45216 Added cost exchange rate for Book Value -mk
// 2010-06-30 VI# 44433 Added calculation of estimated annual income -mk
// 2007-11-08 Update callback()- yb
#ifndef HOLDINGS_H
#define HOLDINGS_H

#include "assets.h"
#include "commonheader.h"


#ifndef NT
#define NT 1
#endif

typedef struct {
  int iID;
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  char sSecXtend[2 + NT];
  char sAcctType[1 + NT];
  long lTransNo;
  int iSecID;
  long lAsofDate;
  char sSecSymbol[12 + NT];
  double fUnits;
  double fOrigFace;
  double fTotCost;
  double fUnitCost;
  double fOrigCost;
  double fOpenLiability;
  double fBaseCostXrate;
  double fSysCostXrate;
  long lTrdDate;
  long lEffDate;
  long lEligDate;
  long lStlDate;
  double fOrigYield;
  long lEffMatDate;
  double fEffMatPrice;
  double fCostEffMatYld;
  long lAmortStartDate;
  char sOrigTransType[2 + NT];
  char sOrigTransSrce[1 + NT];
  char sLastTransType[2 + NT];
  long lLastTransNo;
  char sLastTransSrce[1 + NT];
  long lLastPmtDate;
  char sLastPmtType[2 + NT];
  long lLastPmtTrNo;
  long lNextPmtDate;
  double fNextPmtAmt;
  long lLastPdnDate;
  char sLtStInd[2 + NT];
  double fMktVal;
  double fCurLiability;
  double fMvBaseXrate;
  double fMvSysXrate;
  double fAccrInt;
  double fAiBaseXrate;
  double fAiSysXrate;
  double fAnnualIncome;
  double fAccrualGl;
  double fCurrencyGl;
  double fSecurityGl;
  double fMktEffMatYld;
  double fMktCurYld;
  char sSafekInd[1 + NT];
  double fCollateralUnits;
  double fHedgeValue;
  char sBenchmarkSecNo[12 + NT];
  char sPermLtFlag[1 + NT];
  char sValuationSrce[2 + NT];
  char sPrimaryType[1 + NT];
  int iRestrictionCode;
} HOLDINGS;

typedef struct {
  int iID;
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  char sSecXtend[2 + NT];
  long lTransNo;
  double fTotCost;
  double fMktVal;
  double fMvBaseXrate;
  double fBaseCostXrate;
  double fAccrInt;
  double fAiBaseXrate;
  char sPrimaryType[1 + NT];
  BOOL bHoldCash; // Currently Unused, TRUE-from holdcash,FALSE-from holdings
  int iAssetIndex;
  double fAnnualIncome;
} PARTHOLDING;

typedef struct {
  long lHoldDate; /* date on which holdings table is rolled to */
  int iNumHolding;
  int iHoldingCreated;
  PARTHOLDING *pzHold;
} HOLDINGTABLE;

typedef void(CALLBACK *LPPRINITHOLDINGS)(HOLDINGS *);
typedef void(CALLBACK *LPPRHOLDINGS)(HOLDINGS, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHOLDINGS)(HOLDINGS *, int, char *, char *,
                                           char *, char *, long, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHOLDINGS2)(HOLDINGS *, int, char *, char *,
                                            char *, char *, long, long, long,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTHOLDINGS3)(HOLDINGS *, int, char *, char *,
                                            char *, char *, long, double,
                                            ERRSTRUCT *);
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

typedef void(CALLBACK *LPPRSELECTPERFORMANCEHOLDINGS)(PARTHOLDING *,
                                                      PARTASSET2 *, LEVELINFO *,
                                                      int, char *, int,
                                                      ERRSTRUCT *);
// typedef void  (CALLBACK* LPPRSELECTPERFORMANCEHOLDCASH)(PARTHOLDING
// *,PARTASSET *, LEVELINFO *, int, char *, int, ERRSTRUCT *); typedef void
// (CALLBACK* LPPRDELETEHOLDINGS)(int, char *, char *, char *, char *, long,
// ERRSTRUCT *); typedef void  (CALLBACK* LPPRDELETEHOLDINGSHOLDCASH)(int, char
// *, char *, char *, BOOL, ERRSTRUCT *);

long GetStlDate(HOLDINGS zHold);
void GetStlDateAndPrice(HOLDINGS zHold, double fTradUnit, long *plStlDate,
                        double *pfPrice);

#endif // !HOLDINGS_H
