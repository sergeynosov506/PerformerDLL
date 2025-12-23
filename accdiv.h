/*H*
 *
 * FILENAME: accdiv.h
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
 *
 *H*/
#ifndef ACCDIV_H
#define ACCDIV_H

#include "commonheader.h"

#ifndef NT
#define NT 1
#endif

typedef struct {
  int iID;
  long lTransNo;
  long lDivintNo;
  char sTranType[2 + NT];
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  int iSecID;
  char sSecXtend[2 + NT];
  char sAcctType[1 + NT];
  long lEligDate;
  char sSecSymbol[12 + NT];
  char sDivType[2 + NT];
  double fDivFactor;
  double fUnits;
  double fOrigFace;
  double fPcplAmt;
  double fIncomeAmt;
  long lTrdDate;
  long lStlDate;
  long lEffDate;
  long lEntryDate;
  char sCurrId[4 + NT];
  char sCurrAcctType[1 + NT];
  char sIncCurrId[4 + NT];
  char sIncAcctType[1 + NT];
  char sSecCurrId[4 + NT];
  char sAccrCurrId[4 + NT];
  double fBaseXrate;
  double fIncBaseXrate;
  double fSecBaseXrate;
  double fAccrBaseXrate;
  double fSysXrate;
  double fIncSysXrate;
  double fOrigYld;
  long lEffMatDate;
  double fEffMatPrice;
  char sAcctMthd[1 + NT];
  char sTransSrce[1 + NT];
  char sDrCr[2 + NT];
  char sDtcInclusion[1 + NT];
  char sDtcResolve[2 + NT];
  char sIncomeFlag[2 + NT];
  char sLetterFlag[1 + NT];
  char sLedgerFlag[1 + NT];
  char sCreatedBy[8 + NT];
  long lCreateDate;
  char sCreateTime[8 + NT];
  char sSuspendFlag[1 + NT];
  char sDeleteFlag[1 + NT];
  char sDescription[60 + NT];
} ACCDIV;

typedef struct {
  int iID;
  long lTransNo;
  long lDivintNo;
  char sSecNo[12 + NT];
  char sWi[1 + NT];
  char sSecXtend[2 + NT];
  char sAcctType[1 + NT];
  char sTranType[2 + NT];
  char sDrCr[2 + NT];
  double fUnits;
  double fPcplAmt;
  double fIncomeAmt;
  long lTrdDate;
  long lStlDate;
  long lEffDate;
  char sCurrId[4 + NT];
  char sCurrAcctType[1 + NT];
  char sIncCurrId[4 + NT];
  char sIncAcctType[1 + NT];
  double fBaseXrate;
  double fIncBaseXrate;
  double fSecBaseXrate;
  double fAccrBaseXrate;
  double fUnitsAccountedFor;
  char sDeleteFlag[1 + NT];
} PARTACCDIV;

typedef struct {
  int iNumAccdiv;
  int iAccdivCreated;
  PARTACCDIV *pzAccdiv;
} ACCDIVTABLE;

typedef void(CALLBACK *LPPRACCDIV)(ACCDIV *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTONEACCDIV)(ACCDIV *, int, long, long,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTALLACCDIV)(ACCDIV *, int *, double *, double *,
                                            double *, const char *,
                                            const char *, int, const char *,
                                            const char *, const char *,
                                            const char *, long, long,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPRALLACCDIVFORANACCOUNT)(int, long, long, PARTACCDIV *,
                                                  ERRSTRUCT *);
typedef void(CALLBACK *LPPRPENDINGACCDIVTRANSFORANACCOUNT)(int, long,
                                                           PARTACCDIV *,
                                                           ERRSTRUCT *);
typedef int(CALLBACK *LPFNACCDIVTABLE)(ACCDIVTABLE, int, BOOL, long, long,
                                       char *, char *, char *, char *, long);

#endif // !ACCDIV_H