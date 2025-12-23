// 2015-03-23 VI# 56949  Fixed size of fields - sergeyn
// 2009-03-17 VI# 41539: fixed sideeffect to accretion functionality - mk
// 2009-03-04 VI# 41539: added accretion functionality - mk

#include "assets.h"
#include "commonheader.h"
#include "dtransdesc.h"
#include "fixedinc.h"
#include "hedgexref.h"
#include "holdcash.h"
#include "holddel.h"
#include "holdings.h"
#include "payrec.h"
#include "portbal.h"
#include "portmain.h"
#include "priceinfo.h"
#include "sectype.h"
#include "trans.h"
#include "trantype.h"
#include <time.h>

#define MAXDTRANDESC 50
#define NUMEXTRAELEMENTS 10
#define ADDVALUE 1
#define ADDANDWRITEVALUE 2
#define JUSTWRITEVALUE 3

typedef struct {
  long lLot;
  char sAcctType[1 + NT];
  double fShares;
  double fOrigFace;
  double fUnitCost;
  double fTotCost;
  double fOrigCost;
  double fOpenLiability;
  double fBaseCostXrate;
  double fSysCostXrate;
  long lTradeDate;
  long lSettleDate;
  double fOrgYield;
  double fCostEffMatYld;
  long lMatDate;
  double fMatPrice;
  double fCollateralUnits;
  double fSharesToRemove;
  double fSharesRemoved;
  double fAmortUnit;
  char sSafekInd[1 + NT];
  char sAcctMthd[1 + NT];
  char sDrCr[2 + NT];
  char sPermLtFlag[1 + NT];
  double fUnitCostRound;
  int iRestrictionCode;
} POSINFO;

typedef struct {
  POSINFO *pzPInfo;
  int iSize;    /* # of pzPInfo elements created using malloc */
  int iCount;   /* actual # of taxlots in pzPInfo */
} POSINFOTABLE; /* Position Info Table Structure */

/*
** following structure stores some of the trade info which needs to be passed
** between different functions
*/
typedef struct {
  double fUnits;
  double fOrigFace;
  double fPcplAmt;
  double fNetComm;
  double fSecFees;
  double fMiscFee1;
  double fMiscFee2;
  double fAccrInt;
  double fIncomeAmt;
  double fOptPrem;
  double fBasisAdj;
  double fCommGcr;
  double fTotCost;
} TRADEINFO;

typedef struct {
  TRANS zTrans;
  BOOL bAsOfFlag;   /* Is this transaction an AsOf */
  BOOL bReversable; /* Only For CancelTable - Is it eligible for reversal */
  BOOL bRebookFlag; /* Only For CancelTable - Does it need to be rebooked */
  TRANTYPE zTranType;
  char sRevTranCode[2];
  char sSellAcctType[2];
  long lDtransNo;
  long lBlockTransNo;
} TRANSINFO; /* Transaction and related info */

typedef struct {
  TRANSINFO *pzTInfo;
  int iSize;  /* # of pzInfo elements created using malloc */
  int iCount; /* # of actual transactions in pzInfo->zTrans */
} PTABLE;     /* Process Table Structure */

typedef int(CALLBACK *LPFNINSERTPORTLOCK)(int, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRDELETEPORTLOCK)(int, ERRSTRUCT *);

// Global Variables
// extern FILE *fp;
extern char sErrorFile[80]; // error file name used by Printerror routine

typedef void(CALLBACK *LPPRTRANSSELECT)(TRANS *, TRANTYPE *, int, long, long,
                                        char *, char *, ERRSTRUCT *);

// All the I/O functions defined in StarsIO.DLL are dynamically loaded,
// define all the EXTERN function pointer variables
extern LPFNVOID lpfnStartTransaction, lpfnCommitTransaction,
    lpfnRollbackTransaction;
extern LPFNVOID lpfnGetTransCount;
extern LPFN1BOOL lpfnAbortTransaction;
// extern LPFN1INT lpfnTimer;
extern LPPR1INT1LONG lpprDeleteAccdivAllLots, lpprUpdatePortmainLastTransNo;
extern LPPR1INT3LONG1PCHAR lpprDeleteDivhistOneLot;
extern LPPR1INT2LONG1PCHAR lpprDeleteDivhistAllLots;
extern LPPR1LONG4PCHAR lpprDeleteHoldcash;
extern LPPR1INT4PCHAR1LONG lpprDeleteHedgxref, lpprDeleteHoldings,
    lpprDeleteAccruingAccdivOneLot, lpprDeleteAccruingDivhistOneLot;
extern LPPR1INT4PCHAR2LONG lpprDeletePayrec;
extern LPPR1PLONG1PCHAR2LONG lpprSelectRevNoAndCode;
extern LPPR2PLONG lpprSelectStarsDate;
extern LPPR3LONG lpprDeleteAccdivOneLot, lpprUpdateXTransNo,
    lpprUpdateNewTransNo, lpprUpdateRevTransNo;
extern LPPR3LONG1PLONG1PINT lpprGetLastIncomeDate;
extern LPPR4LONG lpprHolddelUpdate;

extern LPPR4LONG2PCHAR2PLONG2PDOUBLE lpprGetIncomeForThePeriod;

extern LPPR1PCHAR1LONG lpprSelectAcctMethod;
extern LPFN1PCHAR1PLONG lpfnrstrdate;
// extern LP2PR1PCHAR
// lpprTimerResult;
extern LPPR2PCHAR lpprSelectSellAcctType;
extern LPPR2PCHAR1PINT1PCHAR lpprCurrencySecno;
extern LPPR2PCHAR3LONG1PCHAR lpprUpdateDivhistAllLots;
extern LPPR2PCHAR4LONG1PCHAR lpprUpdateDivhistOneLot;
extern LPPR3PCHAR lpprSelectCallPut, lpprSelectTrancode;
extern LPPR3PCHAR1LONG1INT lpprStarsIOInit;
extern LPPRASSETS lpprSelectAsset;
extern LPFNCALCGAINLOSS lpfnCalcGainLoss;
extern LPPRDTRANSSELECT lpprSelectDtrans;
extern LPPRDTRANSUPDATE lpprUpdateDtrans;
extern LPPRDTRANSDESCSELECT lpprSelectDtransDesc, lpprSelectTransDesc;
extern LPPRSELECTPARTFINC lpprSelectPartFixedinc;
extern LPPRSELECTHEDGEXREF lpprSelectHedgxref;
extern LPPRHEDGEXREF lpprInsertHedgxref, lpprUpdateHedgxref;
extern LPPRSELECTHOLDCASH lpprSelectHoldcash;
extern LPPRHOLDCASH lpprInsertHoldcash, lpprUpdateHoldcash;
extern LPPRHOLDINGS lpprInsertHoldings, lpprUpdateHoldings;
extern LPPRSELECTHOLDINGS lpprHoldcashForFifoAndAvgAcct,
    lpprHoldcashForLifoAcct, lpprHoldcashForHighAcct, lpprHoldcashForLowAcct,
    lpprSelectHoldings, lpprHoldingsForFifoAndAvgAcct, lpprHoldingsForLifoAcct,
    lpprHoldingsForHighAcct, lpprHoldingsForLowAcct;
extern LPPRSELECTHOLDINGS2 lpprHoldcashForMinimumGainAcct,
    lpprHoldcashForMaximumGainAcct, lpprHoldingsForMinimumGainAcct,
    lpprHoldingsForMaximumGainAcct;
extern LPPRSELECTHOLDINGS3 lpprMinimumGainHoldings, lpprMinimumLossHoldings,
    lpprMaximumLossHoldings;
extern LPPRHSUMSELECT lpprHsumSelect;
extern LPPRHSUMUPDATE lpprHsumUpdate;
extern LPPRSELECTHOLDDEL lpprSelectHolddel;
extern LPPRHOLDDEL lpprInsertHolddel;
extern LPFN1PCHAR3LONG lpfnInflationIndexRatio;
extern LPFNNEWDATE lpfnNewDateFromCurrent;
extern LPPRPAYREC lpprInsertPayrec;
extern LPPRSELECTPAYREC lpprSelectPayrec;
extern LPPRSECPRICE lpprGetSecurityPrice;
// extern LPPRSELECTPORTBAL
// lpprSelectPortbal; extern LPPRUPDATEPORTBAL
// lpprUpdatePortbal;
extern LPPRPORTMAIN lpprSelectPortmain;
extern LPFNINSERTPORTLOCK lpfnInsertPortLock;
extern LPPRDELETEPORTLOCK lpprDeletePortlock;
extern LPFNRMDYJUL lpfnrmdyjul;
extern LPFNRJULMDY lpfnrjulmdy;
extern LPPRSECCHAR lpprSecCharacteristics;
extern LPPRSECTYPE lpprSelectSectype;
extern LPPRTRANS lpprInsertTrans, lpprUpdateBrokerInTrans;
extern LPPRSELECTTRANS lpprSelectTransForMatchingXref, lpprSelectOneTrans;
extern LPPRTRANSSELECT lpprSelectTrans;
extern LPPRTRANSDESC lpprInsertTransDesc; // use DtransDesc as TransDesc
extern LPPRSELECTTRANTYPE lpprSelectTrantype;

extern LPPRSELECTPAYTRAN lpprSelectPayTran, lpprSelectDPayTran;
extern LPPRINSERTPAYTRAN lpprInsertPayTran;

/* Prototype of functions defined in updhold1.c file  */
DLLAPI ERRSTRUCT STDCALL WINAPI UpdateHold(TRANS zTR, PORTMAIN zPR,
                                           DTRANSDESC zDtransDesc[],
                                           int iNumDtransDesc,
                                           PAYTRAN *pzPayTran, char *sCurPrior,
                                           BOOL bDoTransaction);
ERRSTRUCT ProcessCash(TRANS zTR, long lCashImpact, BOOL bReversal,
                      char *sCurPrior, BOOL bIncByLot);
ERRSTRUCT ProcessMoneyInHoldings(TRANS zTR, long lCashImpact, double fCashAdj,
                                 double fBaseXrate, double fSysXrate,
                                 char *sCurPrior, BOOL bReversal,
                                 BOOL bIncByLot);
ERRSTRUCT ProcessIncome(TRANS zTR, BOOL bIncByLot, char *sCurPrior,
                        BOOL bReversal);
ERRSTRUCT ProcessOpeningInHoldings(TRANS zTR, PORTMAIN zPR, long lSecImpact,
                                   long lCashImpact, int iDateInd,
                                   BOOL bReversal, char *sCurPrior);

ERRSTRUCT ProcessClosingInHoldings(TRANS zTR, char *sBaseCurr, long lSecImpact,
                                   BOOL bReversal, char *sCurPrior);
ERRSTRUCT ProcessSplitInHoldings(TRANS zTR, long lSecImpact, char *sAutoGen,
                                 BOOL bReversal, char *sCurPrior);
ERRSTRUCT ProcessAdjustmentInHoldings(TRANS *pzTR, PORTMAIN zPR,
                                      long lSecImpact, long lCashImpact,
                                      BOOL bReversal, char *sCurPrior);
ERRSTRUCT ProcessTransferInHoldings(TRANS zTR, PORTMAIN zPR, char *sBaseCurr,
                                    long lSecImpact, long lCashImpact,
                                    int iDateInd, BOOL bReversal,
                                    char *sCurPrior);
ERRSTRUCT ProcessTransferInHoldcash(TRANS zTR, PORTMAIN zPR, long lCashImpact,
                                    int iDateInd, BOOL bReversal,
                                    char *sCurPrior);
ERRSTRUCT ProcessAverageInHoldings(TRANS *pzTR, double fTradingUnits,
                                   double *pzSysXrate, BOOL bReversal,
                                   char *sCostInd);
ERRSTRUCT UpdateDivhistAccdiv(TRANS zTR, BOOL bIncByLot, BOOL bReversal,
                              char *sCurPrior, char *sProcessType);

/* Prototype of functions defined in updhold2.c file  */
DLLAPI void STDCALL WINAPI InitializeHoldingsStruct(HOLDINGS *pzHoldings);
DLLAPI void STDCALL WINAPI InitializeHoldcashStruct(HOLDCASH *pzHoldcash);
DLLAPI void STDCALL WINAPI InitializeTransStruct(TRANS *pzTrans);
void InitializeHedgeXrefStruct(HEDGEXREF *pzHedgeXref);
void InitializePayrecStruct(PAYREC *pzPayrec);
void InitializeHolddelStruct(HOLDDEL *pzHolddel);
DLLAPI void STDCALL WINAPI InitializeErrStruct(ERRSTRUCT *pzErr);
void CopyFieldsFromTransToHoldcash(HOLDCASH *pzHCR, TRANS zTR);
void CopyFieldsFromTransToHoldings(HOLDINGS *pzHR, TRANS zTR);
void CopyFieldsFromHoldcashToHoldings(HOLDINGS *pzHR, HOLDCASH zHCR);
void CopyFieldsFromHoldingsToHoldcash(HOLDCASH *pzHCR, HOLDINGS zHR);
void CopyFieldsFromHolddelToHoldings(HOLDINGS *pzHR, HOLDDEL zHD);
void CopyFieldsFromHoldingsToHolddel(HOLDDEL *pzHD, HOLDINGS zHR);
void CopyFieldsFromHoldingsToTrans(TRANS *pzTR, HOLDINGS zHR,
                                   char *sPrimaryType);
void CopyFieldsFromTransToHedgeXref(HEDGEXREF *pzHG, TRANS zTR);
void CopyFieldsFromTransToPayrec(PAYREC *pzPYR, TRANS zTR);
void CopyFieldsFromHoldingsToHolddel(HOLDDEL *pzHD, HOLDINGS zHR);
void CopyFieldsFromHolddelToHoldings(HOLDINGS *pzHR, HOLDDEL zHD);
void CalculateNewCostXRate(double fHoldCost, double fTransCost, double fHoldCXR,
                           double fTransCXR, double *pfNewXrate);
void CalculateCurrencyGainLoss(double fTCost, double fBaseCXR, double fMV,
                               double fMVBaseCXR, double *pfCurrencyGL);
ERRSTRUCT GetSecurityCharacteristics(char *sSecNo, char *sWi, char *sPType,
                                     char *sSType, char *sPostnInd,
                                     char *sLotInd, char *sCostInd,
                                     char *sLotExistInd, char *sAvgInd,
                                     double *pfTrdUnit, char *sCurrId);
DLLAPI int STDCALL WINAPI SetErrorFileName(char *sErrFile);
DLLAPI ERRSTRUCT STDCALL WINAPI PrintError(const char *sErrMsg, int,
                                           long lRecNo, const char *sRecType,
                                           int iBusinessErr, int iSqlErr,
                                           int iIsamCode,
                                           const char *sMsgIdentifier,
                                           BOOL bWarning);

/* Prototype of functions defined in tranalloc.c file */
DLLAPI ERRSTRUCT STDCALL WINAPI TranAlloc(TRANS *pzTrans, TRANTYPE zTranType,
                                          SECTYPE zSecType, ASSETS zAssets,
                                          DTRANSDESC zDTranDesc[],
                                          int iNumDTItems, PAYTRAN *pzPayTran,
                                          char *sCurrPrior,
                                          BOOL bDoTransaction);
ERRSTRUCT ProcessOpenInTranAlloc(TRANS *pzTrans2, SECTYPE zSecType,
                                 ASSETS zAssets, TRANTYPE zTranType,
                                 DTRANSDESC zDTransDesc[], int iNumDTItems,
                                 PAYTRAN *pzPayTran, BOOL *bExerciseFlag,
                                 char *sCurrPrior, double *pzExerciseUnits,
                                 BOOL bDoTransaction);

ERRSTRUCT ProcessCloseInTranAlloc(TRANS *pzTrans, TRANTYPE zTranType,
                                  SECTYPE zSecType, ASSETS zAssets,
                                  DTRANSDESC zDTransDesc[], int iNumDTItems,
                                  PAYTRAN *pzPayTran, BOOL *bExerciseFlag,
                                  char *sCurrPrior, double *pzExerciseUnits,
                                  BOOL bDoTransaction);

ERRSTRUCT ProcessAdjustInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                   int iNumDTItems, char *sCurrPrior,
                                   BOOL bDoTransaction);

ERRSTRUCT ProcessSplitInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                  int iNumDTItems, char *sCurrPrior,
                                  BOOL bDoTransaction);

ERRSTRUCT ProcessIncomeInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                   int iNumDTItems, char *sCurrPrior,
                                   BOOL bDoTransaction);

ERRSTRUCT ProcessMoneyInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                  int iNumDTItems, PAYTRAN *pzPayTran,
                                  char *sCurrPrior, BOOL bDoTransaction);

ERRSTRUCT ProcessCancelInTranAlloc(TRANS *pzTrans3, DTRANSDESC zDTransDesc[],
                                   int iNumDTItems, char *sCurrPrior,
                                   BOOL bDoTransaction);

ERRSTRUCT ProcessTransferInTranAlloc(TRANS *pzTrans, TRANTYPE zTranType,
                                     SECTYPE zSecType, ASSETS zAssets,
                                     DTRANSDESC zDTransDesc[], int iNumDTItems,
                                     char *sCurrPrior, BOOL bDoTransaction);

ERRSTRUCT DecipherDescInfo(long *lOpenDate, double *pzSellUnits,
                           double *pzUnitCost, char *sDescInfo, int iID,
                           long lRecNo, char *sRecType);
double nbcFrac2Num(char *);
ERRSTRUCT ExerciseOptionInTranAlloc(TRANS *pzTrans, double fExerciseUnits,
                                    TRANTYPE zTranType, SECTYPE zSecType,
                                    ASSETS zAssets, DTRANSDESC zDTransDesc[],
                                    int iNumDTItems, char *sCurrPrior,
                                    BOOL bDoTransaction);
ERRSTRUCT CalcOptionPremium(TRANS zTrans, double *pzExerciseUnits,
                            double *pzOptPrem);
ERRSTRUCT TransactionProcess(POSINFOTABLE zPv, TRANS *pzTrans,
                             TRADEINFO *pzOriginal, TRADEINFO *pzRemaining,
                             DTRANSDESC zDTransDesc[], int iNumDTItems,
                             PAYTRAN *pzPayTran, char *sCurrPrior,
                             long *plFirstTransNo, char *sOriginalFace,
                             char *sPrimaryType, char *sSecondaryType,
                             char *sPayType, BOOL bDoTransaction);
ERRSTRUCT ProcessTechnicals(TRANS *pzTrans, TRANTYPE zTranType,
                            TRADEINFO *pzOriginal, TRADEINFO *pzRemaining,
                            DTRANSDESC zDTransDesc[], int iNumDTItems,
                            char *sCurrPrior, long *plFirstTransNo,
                            BOOL bDoTransaction);
ERRSTRUCT GetExistingPosition(TRANS zTrans, POSINFOTABLE *pzPv,
                              double *pzTotalUnits, TRANTYPE zTranType,
                              SECTYPE zSectype, char *sOriginalFace,
                              double fTradingUnit);
void RemoveByAcctMethod(POSINFOTABLE zPv, double *pzRemainingSharesToRemove,
                        double *pzTotalSharesRemoved,
                        double *pzTotalCostRemoved);
ERRSTRUCT CurrencySweepProcessor(TRANS zTrans, PORTMAIN zPortmain,
                                 char *sCurrPrior, BOOL bDoTransaction,
                                 int iAction, BOOL bInit);
ERRSTRUCT DoTranAlloc(TRANS *pzTrans, TRANTYPE zTranType, SECTYPE zSecType,
                      ASSETS zAssets, DTRANSDESC zDTransDesc[], int iNumDTItems,
                      PAYTRAN *pzPayTran, char *sCurrPrior,
                      BOOL bDoTransaction);

/* Prototype of functions defined in tadbrtns.c file */
ERRSTRUCT GetNewSecurityInfo(TRANS zTrans, ASSETS *pzAssets, SECTYPE *pzSecType,
                             TRANTYPE *pzTranType);
ERRSTRUCT GetCallPut(TRANS zTrans, char *sCallPut);
ERRSTRUCT IncrementPortmainLastTransNo(PORTMAIN *pzPR, int iID);
ERRSTRUCT SelectHoldingsTransNo(int iID1, char *sSecNo, char *sWi,
                                char *sSecXtend, char *sAcctType,
                                long *plTransNo);
void InitializeTradeInfoStruct(TRADEINFO *pzTI);
void InitializePositionInfoStruct(POSINFO *pzPV);
ERRSTRUCT InitializePosInfoTable(POSINFOTABLE *pzPITable, int iSize);
void CopyToTradeInfoFromTrans(TRANS zTrans, TRADEINFO *pzTI);
void CopyToTransFromTradeInfo(TRADEINFO zTI, TRANS *pzTrans,
                              BOOL bCopyOrigFace);
DLLAPI void STDCALL WINAPI InitializeTransTable2(TRANSTABLE2 *pzTTable2);
DLLAPI ERRSTRUCT STDCALL WINAPI AddTransToTransTable2(TRANSTABLE2 *pzTTable2,
                                                      TRANS zTrans);

/* Prototype of functions defined in TranProc.c file */
ERRSTRUCT InternalTranProc(int iID, char *sSecNo, char *sWi, long lTagNo,
                           char *sCurrPrior, BOOL bReversal, char *sFileName,
                           BOOL bDoTransaction);
ERRSTRUCT LockPortAndProcess(PTABLE zProTable, ASSETS zAssts, PORTMAIN zPDir,
                             TRANTYPE zTrType, SECTYPE zSType, long lCurrDate,
                             BOOL bAnyAsOfs, char *sCurrPrior, BOOL bReversal,
                             char *sFileName, BOOL bDoTransaction);
ERRSTRUCT ProcessAsOfTrade(char *sAcctMethod, TRANS zTrans, char *sOrgTranCode,
                           char *sRevTranCode, char *sAutoGen,
                           char *sSellAcctType, char *sLotInd, char *sCurrPrior,
                           BOOL bRebook, PTABLE *pzCancelTable);
ERRSTRUCT ProcessAsOfAdjustment(TRANS zTrans, PTABLE *pzCancelTable, int i,
                                char *sOrgTranCode, char *sRevTranCode,
                                char *sCurrPrior, BOOL bRebook);
ERRSTRUCT ProcessAsOfIncome(TRANS zTrans, PTABLE *pzCancelTable, int i,
                            char *sOrgTranCode, char *sRevTranCode,
                            char *sAutoGen, char *sCurrPrior, BOOL bRebook);
ERRSTRUCT ProcessAsOfMoney(TRANS zTrans, PTABLE *pzCancelTable, int i,
                           char *sOrgTranCode, char *sRevTranCode);
ERRSTRUCT ProcessAsOfOpen(char *sAcctMethod, TRANS zTrans,
                          PTABLE *pzCancelTable, int i, char *sOrgTranCode,
                          char *sRevTranCode, char *sCurrPrior, BOOL bRebook);
ERRSTRUCT ProcessAsOfOthers(TRANS zTrans, PTABLE *pzCancelTable, int i,
                            char *sOrgTranCode, char *sRevTranCode,
                            char *sCurrPrior, BOOL bRebook);
ERRSTRUCT AddTradeToCancelTable(TRANS zTrans, PTABLE *pzCancelTable,
                                char *sTranCode, int iTradeSort, BOOL bRebook);
ERRSTRUCT AggregateTrades(PTABLE zPTable, int iPStartIndex, int *piPEndIndex,
                          TRANS *pzTrans);
ERRSTRUCT CreateMergeTable(PTABLE zProcecssTab, PTABLE zCancelTab,
                           PTABLE *pzMergeTab);
ERRSTRUCT ProcessCancelTable(PTABLE zCanTable, long lCurrentDate,
                             char *sCurrPrior, BOOL bDoTransaction);
ERRSTRUCT ProcessOpeninTranproc(TRANS *pzHTrans, ASSETS zAst, PORTMAIN zPdir,
                                TRANTYPE zTtype, long lcurrdate);
ERRSTRUCT ProcessCloseinTranproc(TRANS *pzHTrans, ASSETS zAst, PORTMAIN zPmain,
                                 TRANTYPE zTtype, long lcurrdate);
ERRSTRUCT ProcessTransferinTranproc(TRANS *pzHTrans, ASSETS zAst,
                                    PORTMAIN zPdir, TRANTYPE zTtype,
                                    SECTYPE zStype, DTRANSDESC zDtransDes[],
                                    int iDtransDesCount, long lcurrdate,
                                    char *sCurrPrior);
ERRSTRUCT VerifyTransaction(TRANS zTrns, TRANTYPE *pzTranType,
                            char *psRevTranCode, char *psSellAcctType);
ERRSTRUCT SetDefaultDatainTranproc(TRANS *pzTrans, PORTMAIN zPORTMAIN,
                                   ASSETS zAssets, TRANTYPE zTranType,
                                   long lPriceDate, long lTradeDate,
                                   char *sPrimaryType);

ERRSTRUCT GetDtransDescript(int iID, long lTransNo, DTRANSDESC *pzDTranDes,
                            int *piDTranDesCount, BOOL bRebookFlag);
ERRSTRUCT GetDtransPayee(int iID, long lTransNo, PAYTRAN *pzPayTran,
                         BOOL bRebookFlag);

ERRSTRUCT AddTransToPTable(PTABLE *pzPTable, TRANSINFO zTInfo);
ERRSTRUCT FillOutCancelTable(PTABLE *pzCancelTable, int iID, long lEffDate,
                             long lTransNo, long lDtransNo, long lBlockTransNo,
                             char *sSecNo, char *sWI);
ERRSTRUCT WriteReversalInformation(PTABLE CancelTable, char *sFileName,
                                   char *sPortfolioName);

/* Prototype of functions defined in tpdbrtns.c file */
DLLAPI void STDCALL WINAPI InitializeDtransDesc(DTRANSDESC *pzDtransDesc);
void InitializeTransInfo(TRANSINFO *pzTInfo);
void InitializePTable(PTABLE *pzPTable);
ERRSTRUCT PrepareForTranAlloc(TRANTYPE *pzTranType, ASSETS *pzAssets,
                              SECTYPE *pzSecType, TRANS zTrans, BOOL bAllFlag);
ERRSTRUCT CheckTradeToCancelTable(TRANS zTrans, PTABLE zCancelTable[],
                                  int *piCancelCount, char *sTranCode,
                                  char *sOrgTranCode, char *sRevTranCode,
                                  char *sAutoGen, int iTradeSort,
                                  BOOL *bAddFlag);
ERRSTRUCT ProcessXTransNoTrade(char *sAcctMethod, PTABLE *pzCancelTable,
                               int iCIndex, char *sCurrPrior, BOOL bRebook);
ERRSTRUCT FetchXrefTransRecord(TRANS *pzXrefTR, BOOL *bRecFound);
ERRSTRUCT GetProcessDate(long *plDate, long *plDate2);
ERRSTRUCT SelectAsset(ASSETS *pzAssets, char *sSecNo, int iVendorID,
                      char *sWhenIssue);
char BoolToChar(BOOL bValue);
void FreeTranProc();

/* Prototype of functions defined in updbalances.c file */
ERRSTRUCT UpdateBalances(TRANS zTR, PORTMAIN zPR);
ERRSTRUCT ProcessIncomeInUpdbal(TRANS zTR, long lFiscal, char *sPSType,
                                char *sSSType, char *sPayType, char *sTax,
                                long lCashImpact, long lSecImpact,
                                long lPortbalImpact);
ERRSTRUCT ProcessMoneyInUpdbal(TRANS zTR, long lFiscal, long lCashImpact,
                               long lSecImpact, long lPortbalImpact,
                               char *sPerfImpact);
ERRSTRUCT ProcessOpeningInUpdbal(TRANS zTR, long lFiscal, char *sPSType,
                                 char *sSSType, char *sMktValInd,
                                 long lCashImpact, long lSecImpact,
                                 long lPortbalImpact, char *sPayType,
                                 char *sTax, BOOL bReversal);
ERRSTRUCT ProcessClosingInUpdbal(TRANS zTR, long lFiscal, char *sPSType,
                                 char *sSSType, char *sMktValInd,
                                 long lCashImpact, long lSecImpact,
                                 long lPortbalImpact, char *sBaseCurr,
                                 char *sPayType, char *sTax, BOOL bReversal);
ERRSTRUCT ProcessAdjustmentInUpdbal(TRANS zTR, long lFiscal, char *sPSType,
                                    char *sSSType, char *sMktValInd,
                                    long lCashImpact, long lSecImpact,
                                    long lPortbalImpact, BOOL bReversal);
ERRSTRUCT ProcessTransferInUpdbal(TRANS zTR, long lFiscal, char *sPSType,
                                  char *sSSType, char *sMktValInd,
                                  char *sPerfImpact, long lCashImpact,
                                  long lSecImpact, long lPortbalImpact,
                                  char *sBaseCurr, char *sPayType,
                                  char *sTaxStat, BOOL bReversal);
ERRSTRUCT CommonOpenCloseInUpdbal(TRANS zTR, long lFiscal, char *sPSType,
                                  char *sSSType, char *sMktValInd,
                                  long lCashImpact, long lSecImpact,
                                  long lPortbalImpact, char *sPayType,
                                  char *sTax, PORTBAL *pzPBR, BOOL bReversal,
                                  char *sTranCode);
void InitializePortbalStruct(PORTBAL *zPortbal);

/* Prototype of functions defined in TIPSProcessing.c file */
DLLAPI ERRSTRUCT STDCALL WINAPI CalculatePhantomIncome(
    int iID, char *sSecNo, char *sWi, long lTaxlotNo, long lPreviousIncomeDate,
    long lCurrentStlDate, long lOriginalTrdDate, double fOrigTotCost,
    double fUnits, BOOL bFirstIncome, double *pfCalculatedPI);
DLLAPI ERRSTRUCT STDCALL WINAPI CalculateInflationRate(
    int iID, char *sSecNo, char *sWi, long lTaxlotNo, long lPreviousIncomeDate,
    long lCurrentStlDate, long lOriginalTrdDate, BOOL bSaveErrorToFile,
    BOOL bFirstIncome, double *pfCalculatedRate);
/* End of Function Prototypes */