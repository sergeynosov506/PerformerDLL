#include "transengine.h"
// Global Variables
// FILE *fp;
char sErrorFile[80]; // error file name used by Printerror routine

// All the I/O functions defined in StarsIO.DLL are dynamically loaded,
// define all the function pointer variables
LPFNVOID								lpfnStartTransaction, lpfnCommitTransaction, lpfnRollbackTransaction;
LPFNVOID								lpfnGetTransCount;
LPFN1BOOL								lpfnAbortTransaction;
//LPFN1INT								lpfnTimer;
LPPR1INT1LONG							lpprDeleteAccdivAllLots, lpprUpdatePortmainLastTransNo;
LPPR1INT3LONG1PCHAR						lpprDeleteDivhistOneLot;
LPPR1INT2LONG1PCHAR						lpprDeleteDivhistAllLots;
LPPR1LONG4PCHAR							lpprDeleteHoldcash;
LPPR1INT4PCHAR1LONG						lpprDeleteHedgxref, lpprDeleteHoldings, 
                      				lpprDeleteAccruingAccdivOneLot, lpprDeleteAccruingDivhistOneLot;
LPPR1INT4PCHAR2LONG						lpprDeletePayrec;
LPPR1PLONG1PCHAR2LONG					lpprSelectRevNoAndCode;
LPPR2PLONG										lpprSelectStarsDate;
LPPR3LONG											lpprDeleteAccdivOneLot, lpprUpdateXTransNo,
															lpprUpdateNewTransNo, lpprUpdateRevTransNo;
LPPR3LONG1PLONG1PINT					lpprGetLastIncomeDate;
LPPR4LONG											lpprHolddelUpdate;
LPPR4LONG2PCHAR2PLONG2PDOUBLE	lpprGetIncomeForThePeriod;
LPPR1PCHAR1LONG								lpprSelectAcctMethod; 
LPFN1PCHAR1PLONG							lpfnrstrdate;
//LP2PR1PCHAR									lpprTimerResult;
LPPR2PCHAR										lpprSelectSellAcctType;
LPPR2PCHAR1PINT1PCHAR					lpprCurrencySecno;
LPPR2PCHAR3LONG1PCHAR					lpprUpdateDivhistAllLots;
LPPR2PCHAR4LONG1PCHAR					lpprUpdateDivhistOneLot;
LPPR3PCHAR										lpprSelectCallPut, lpprSelectTrancode;
LPPR3PCHAR1LONG1INT						lpprStarsIOInit;
LPPRASSETS										lpprSelectAsset;
LPFNCALCGAINLOSS							lpfnCalcGainLoss; 
LPPRDTRANSSELECT							lpprSelectDtrans;
LPPRDTRANSUPDATE							lpprUpdateDtrans;
LPPRDTRANSDESCSELECT					lpprSelectDtransDesc, lpprSelectTransDesc;
LPPRSELECTPARTFINC						lpprSelectPartFixedinc;   
LPPRSELECTHEDGEXREF						lpprSelectHedgxref;
LPPRHEDGEXREF									lpprInsertHedgxref, lpprUpdateHedgxref;
LPPRSELECTHOLDCASH						lpprSelectHoldcash;
LPPRHOLDCASH									lpprInsertHoldcash, lpprUpdateHoldcash;
LPPRHOLDINGS									lpprInsertHoldings, lpprUpdateHoldings; 
LPPRSELECTHOLDINGS						lpprHoldcashForFifoAndAvgAcct, lpprHoldcashForLifoAcct, lpprHoldcashForHighAcct, 
															lpprHoldcashForLowAcct, lpprSelectHoldings, lpprHoldingsForFifoAndAvgAcct, 
															lpprHoldingsForLifoAcct, lpprHoldingsForHighAcct, lpprHoldingsForLowAcct;
LPPRSELECTHOLDINGS2						lpprHoldcashForMinimumGainAcct, lpprHoldcashForMaximumGainAcct,
															lpprHoldingsForMinimumGainAcct, lpprHoldingsForMaximumGainAcct; 
LPPRSELECTHOLDINGS3						lpprMinimumGainHoldings, lpprMinimumLossHoldings, lpprMaximumLossHoldings; 
LPPRHSUMSELECT								lpprHsumSelect;
LPPRHSUMUPDATE								lpprHsumUpdate;
LPPRSELECTHOLDDEL							lpprSelectHolddel;
LPPRHOLDDEL										lpprInsertHolddel;
LPFN1PCHAR3LONG				 				lpfnInflationIndexRatio;
LPFNNEWDATE										lpfnNewDateFromCurrent;
LPPRPAYREC										lpprInsertPayrec;
LPPRSELECTPAYREC							lpprSelectPayrec;
LPPRSECPRICE									lpprGetSecurityPrice;
//LPPRSELECTPORTBAL						lpprSelectPortbal;
//LPPRUPDATEPORTBAL						lpprUpdatePortbal;
LPPRPORTMAIN									lpprSelectPortmain;
LPFNINSERTPORTLOCK						lpfnInsertPortLock;
LPPRDELETEPORTLOCK						lpprDeletePortlock;
LPFNRMDYJUL										lpfnrmdyjul;
LPFNRJULMDY										lpfnrjulmdy;
LPPRSECCHAR										lpprSecCharacteristics;
LPPRSECTYPE										lpprSelectSectype; 
LPPRTRANS											lpprInsertTrans, lpprUpdateBrokerInTrans;
LPPRSELECTTRANS								lpprSelectTransForMatchingXref, lpprSelectOneTrans;
LPPRTRANSSELECT								lpprSelectTrans;
LPPRTRANSDESC									lpprInsertTransDesc;//use DtransDesc as TransDesc
LPPRSELECTTRANTYPE						lpprSelectTrantype;

LPPRSELECTPAYTRAN							lpprSelectPayTran, lpprSelectDPayTran;
LPPRINSERTPAYTRAN							lpprInsertPayTran;
