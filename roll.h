
/*H*
* 
* SUB-SYSTEM: pmr nbcRoll  
* 
* FILENAME: nbcRoll.h
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
* $Header: /sysprog/lib/pmr/rcs/nbcRoll.h,v 21.1 98/03/11 15:06:33 sxb prod Locker: mxs2 $
*
* 2022-05-20 J#PER12177 Fixed logic for pmperf - mk.
*H*/

#include "commonheader.h"
#include "dtransdesc.h"
#include "portmain.h"
#include "assets.h"
#include "trans.h"


#define LASTTRANSNO											9999999
#define UNPREP_THISQUERY                1
#define UNPREP_TRANSQUERIES             2
#define UNPREP_ACCNTSOURCEQUERIES       3
#define UNPREP_ACCNTDESTINATIONQUERIES  4
#define UNPREP_FIRMSOURCEQUERIES        5
#define UNPREP_FIRMDESTINATIONQUERIES   6
#define UNPREP_MGRSOURCEQUERIES         7
#define UNPREP_MGRDESTINATIONQUERIES    8
#define UNPREP_COMPSOURCEQUERIES        9
#define UNPREP_COMPDESTINATIONQUERIES   10


typedef ERRSTRUCT	(CALLBACK* LPFNUPDATEHOLD)(TRANS, PORTMAIN, 
																						 DTRANSDESC[], int, 
																						 PAYTRAN *, char *, BOOL);


/* Prototype of functions defined in roll.c */
ERRSTRUCT RollAnAccount(PORTMAIN *pzPR, char *sSecNo, char *sWi, BOOL bPartialRoll, BOOL bInitDataSet, long lRollDate, 
												long lSrceDate, long lDestDate, long lTransStartDate, long lTransEndDate, BOOL bPortmainAlreadySelected,
												BOOL bMultipleAccnts,BOOL bResetPerfDate, int iWhichDataSet, BOOL bRollFromCurrent, BOOL bRollFromInception);
ERRSTRUCT SettlementDateRollAnAccount(PORTMAIN *pzPR, long lRollDate, long lSrceDate, 
																			long lDestDate, BOOL bNeedInitialization);
ERRSTRUCT CallUpdateHold(TRANS zTR, PORTMAIN zPR, BOOL bReversal, long *plLastTransNo, int iWhichDataSet, 
												 long lRollDate, long lDestinationDate, BOOL *bRollDateUpdated);
ERRSTRUCT GetLastTransNo(int iID, long lDate, long *pllastTransNo, BOOL bIsNotFoundAnError);
ERRSTRUCT ReadHoldingsMapForADate(long lDate, char *sHoldings, char *sHoldcash, char *sPortbal, 
																	char *sPortmain, char *sHedgxref, char *sPayrec, char *sHoldtot, BOOL bSaveResults);
ERRSTRUCT FindOutDestinationDataSet(long lRollDate, int iWhichDataSet, BOOL bPartialRoll, long *plDestDate);
ERRSTRUCT FindOutSourceDataSet(PORTMAIN zPR, char *sSecNo, char *sWi, long lRollDate, long *plSourceDate, 
                               long *plSourceLastTransNo, long *plTransStartDate, long *plTransEndDate, 
								BOOL bMultipleAccnts,BOOL bRollFromCurrent, BOOL bRollFromInception, BOOL *bInitDataSet, int iWhichDataSet);
ERRSTRUCT GetTransactionDateRange(long lRollDate, long lSourceDate, BOOL bIsSourceCurrent,
																 long *plTransStartDate, long *plTransEndDate);
ERRSTRUCT UpdateRollDate(int iID, long lRollDate, long lLastTransNo, long lDestDate);
ERRSTRUCT IsSecurityInHoldings(char *sSecNo, char *sWi, BOOL *pbInHoldings);
ERRSTRUCT AccntDestInit(int iID, char *sSecNo, char *sWi, long lSrcDate, long lDestDate, BOOL bInitPmainPbal, 
												BOOL bRollFromInception);
ERRSTRUCT CallInitTranProc(long lAsofDate, char *sDBAlias, char *sMode, char *sType, char *sErrFile);
ERRSTRUCT FirmDestInit(char *sSecNo1, char *sWi1, long lSrcDate,long lDestDate);
ERRSTRUCT CompDestInit(int iID, long lSrcDate, long lDestDate);
ERRSTRUCT MgrDestInit(char *sMgr, long lSrcDate, long lDestDate);
ERRSTRUCT GetNextAccount(char *sSourceFlag, PORTMAIN *pzPR);
ERRSTRUCT GetNextListItem(char *sSourceFlag, int iID, char *sItemId2);
ERRSTRUCT CreateFSecBrAcctCursor(char *sSecNo, char *sWi, long lRollDate,
                                 long lSrceDate, long lDestDate);
ERRSTRUCT GetFSecBrAcct(PORTMAIN *pzPR);
void			FreeRoll();

LPFNVOID										lpfnStartDBTransaction, lpfnCommitDBTransaction, lpfnRollbackDBTransaction;
LPFNVOID										lpfnGetTransCount;
LPFN1BOOL										lpfnAbortDBTransaction;
LPPRVOID										lpprFreeStrasIO, lpprCloseQueries;
LPFN1PCHAR1INT							lpfnUnprepareRollQueries;

LPFNRMDYJUL									lpfnrmdyjul;
LPFNRJULMDY									lpfnrjulmdy;

LPPRDTRANSDESCSELECT        lpprSelectTrndesc;

LPPRERRSTRUCT								lpprInitializeErrStruct;

LPPR1LONG3PCHAR1BOOL				lpprAccountDeleteHoldings, lpprAccountDeleteHoldcash;
LPPR1LONG3PCHAR1BOOL				lpprAccountDeleteHedgxref, lpprAccountDeletePayrec;
/*LPPR1LONG2PCHAR							lpprAccountInsertPortbal, lpprAccountInsertPortmain;*/
LPPR1LONG2PCHAR							lpprAccountInsertPortmain;

LPPR1LONG4PCHAR1BOOL				lpprAccountInsertHoldings, lpprAccountInsertHoldcash;
LPPR1LONG4PCHAR1BOOL				lpprAccountInsertHedgxref, lpprAccountInsertPayrec;
/*LPPR1LONG1PCHAR							lpprAccountDeletePortbal, lpprAccountDeletePortmain;*/
LPPR1LONG1PCHAR							lpprAccountDeletePortmain;

LPPR3PCHAR									lpprSelectPositionIndicator;

LPPRPMAINPOINTER						lpprInitializePortmainStruct;
LPFNPRINTERROR							lpfnPrintError;

LPPR1INT1LONG								lpprUpdatePortmainLastTransNo;
LPPR1LONG7PCHAR							lpprSelectOneHoldmap;
LPPR8PCHAR1PLONG						lpprSelectAllHoldmap;

LPPRPORTMAIN								lpprSelectPortmain;

LPPR2PLONG									lpprSelectStarsDate;


LPFNUPDATEHOLD							lpfnUpdateHold;

LPFN1LONG3PCHAR1BOOL1PCHAR	lpfnInitTranProc;

LPPR3LONG2PCHAR1BOOL1PTRANS lpprForwardTrans,lpprBackwardTrans, lpprAsofTrans, lpprTransNoTrans;
LPPR3LONG2PCHAR1BOOL1PTRANS lpprTradeDateTrans, lpprEffectDateTrans, lpprEntryDateTrans;
LPPRSELECTTRANS							lpprSelectTransBySettlementDate;

LPPR3LONG1PCHAR							lpprDestUpd1Portmain;
LPPR2LONG1PCHAR							lpprDestUpd2Portmain;
LPPR1LONG1PCHAR1PLONG				lpprSrcePortmain;
LPPR1LONG1PCHAR1PLONG				lpprSelectRollDate;

LPPR3LONG2PCHAR1BOOL1PINT		lpprAsofTransCount, lpprRegTransCount;

LPFN1LONG										lpfnIsItAMonthEnd;
LP2FN1LONG									lpfnLastMonthEnd;
LPPR3LONG										lpprUpdatePerfDate;
//LPPR1LONG1PCHAR1PLONG				lpfnSrcePortmain;  



/*
LPFNMGRDELPREC             lpfnMgrDelHedgxref,lpfnMgrDelPortmain,lpfnMgrDelPayrec,lpfnMgrDelPortbal,
                           lpfnMgrDelHoldcash,lpfnMgrDelHold;
LPFNMGRINSHOLD             lpfnMgrInsHoldingslpfnMgrInsHoldcash, lpfnMgrInsPortbal, lpfnMgrInsPortmain,
                           lpfnMgrInsHedgxref, lpfnMgrInsPrec; 
LPFNCOMPDELHOLD            lpfnCompDelHedgxref,lpfnCompDelPortmain,lpfnCompDelPayrec,lpfnCompDelPortbal,
                           lpfnCompDelHoldcash,lpfnCompDelHoldings;
LPFNCOMPINSHOLD            lpfnCompInsHedgxref,lpfnCompInsPortmain,lpfnCompInsPayrec,lpfnCompInsPortbal,
                           lpfnCompInsHoldcash,lpfnCompInsHoldings;
LPNFFIRMINSHOLD            lpfnFirmInsHoldings, lpfnFirmInsHoldcash, lpfnFirmInsHedgxref, lpfnFirmInsPrec;
LPNFFIRMINSPBAL            lpfnFirmInsPortbal, lpfnFirmInsPortmain;
LPFNFIRMDELHOLD            lpfnFirmDelHoldings, lpfnFirmDelHoldcash, lpfnFirmDelHedgxref, lpfnFirmDelPrec;
LPFNFIRMDELPBAL            lpfnFirmDelPortbal, lpfnFirmDelPortmain; 
LPFNMGRACCNT               lpfnMgrAcct;
LPFNFIRMACCNT              lpfnFirmAcct;
*/
