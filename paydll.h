#include "payments.h"

//these are functions loaded from OLEDBIO.DLL
// 2/11/2010 - VI#43627 Modified logic to create PIs for those accounts, securities that apply  - mk

LPFNVOID			lpfnStartTransaction, lpfnCommitTransaction, lpfnRollbackTransaction;
LPFNVOID			lpfnGetTransCount;
LPFN1BOOL			lpfnAbortTransaction;

LPPRERRSTRUCT														lpprInitializeErrStruct;
LPFNPRINTERROR													lpfnPrintError;
LPFN1PCHAR															lpfnSetErrorFileName;
LPPRINITTRANS														lpprInitializeTransStruct;
LPFN1LONG3PCHAR1BOOL1PCHAR							lpfnInitTranProc;
LPFNTRANALLOC														lpfnTranAlloc;
LPPRINITTRANSTABLE2											lpprInitTransTable2;
LPFNADDTRANSTOTRANSTABLE2								lpfnAddTransToTransTable2;
LPFN1INT2PCHAR4LONG2DOUBLE1BOOL1PDOUBLE	lpfnCalculatePhantomIncome;
LPFNCALCULATEINFLATIONRATE							lpfnCalculateInflationRate;
LPPRPDTRANSDESC													lpprInitializeDtransDesc;
LPPRSELECTHOLDINGS											lpprSelectHoldings;
LPFN1PCHAR3LONG												lpfnInflationIndexRatio;

LPFNRMDYJUL									lpfnrmdyjul;
LPFNRJULMDY									lpfnrjulmdy;
LPFN1PCHAR1PLONG						lpfnrstrdate;
LPFN1LONG2PCHAR1PLONG				lpfnNextBusinessDay;

LPPR3LONG										lpprDeleteAccdivOneLot;
LPPRACCDIV									lpprInsertAccdiv;
LPPRACCDIV                  lpprUpdateAccdiv;
LPPRSELECTONEACCDIV					lpprSelectOneAccdiv;
LPPRSELECTALLACCDIV					lpprSelectAllAccdiv;
LPPR1PCHAR1INT4PHAR1LONG		lpprUpdateAccdivDeleteFlag;
LPPR3LONG1PLONG1PINT				lpprGetLastIncomeDate, lpprGetLastIncomeDateMIPS;
LPPRFWTRANS									lpprInsertFWTrans;
LPPR1INT										lpprDeleteFWTrans;


LPPRPPARTCURR								lpprSelectCurrency;

LPPRDIVINTUNL								lpprDivintUnload;

LPPRDIVHIST									lpprInsertDivhist;
LPPRUPDATEDIVHIST						lpprUpdateDivhist;
LPPR3LONG										lpprDeleteDivhist;
LPPR2PCHAR4LONG1PCHAR				lpprUpdateDivhistOneLot;
LPPR2PCHAR3LONG1PCHAR				lpprUpdateDivhistAllLots;

LPPRSELECTPARTFINC					lpprSelectFixedInc;

LPPRSECPRICE								lpprGetSecurityPrice;

LPPRMATURITYUNLOAD					lpprMaturityUnload;

LPPRPARTPMAINALL						lpprSelectAllPartPortmain;
LPPRPARTPMAINONE						lpprSelectOnePartPortmain;

LPPRPORTTAX									lpprSelectPorttax;

LPPRALLSECTYPE							lpprSelectAllSectype;

LPPR2PCHAR									lpprSelectSubacct;
LPPRSELECTSYSSETTINGS				lpprSelectSysSettings;
LPPRSELECTSYSVALUES					lpprSelectSysvalues;


LPPR3LONG										lpprUpdateXrefTransNo;

LPPRSELECTTRANTYPE					lpprSelectTranType;

LPPRWITHHOLDRCLM						lpprSelectWithrclm;

LP2FN1LONG									lpfnLastMonthEnd;

LPPRGETPORTFOLIORANGE				lpprGetPortfolioRange;

//ValuationIO  functions
LPPRSELECTNEXTCALLDATEPRICE	lpprSelectNextCallDatePrice;
LPPR2PLONG									lpprSelectStarsDate ;
LPPRFORWARDMATURITYUNLOAD		lpprForwardMaturityUnload;
//amortize IO functions
LPPRAMORTUNLOAD								lpprAmortizeUnload;
LPPRCALCULATEYIELD						lpfnCalculateYield;
LPPRCALCULATEPRICEGIVENYIELD	lpfnCalculatePriceGivenYield;
LPPRUPDATEAMORTIZEHOLDINGS		lpprUpdateAmortizeHohldings;
LPPRASSETS										lpprSelectAsset;
LPPRSELECTTRANS								lpprSelectTrans;


//PI TIPS IO functions
LPPRPITIPSUNLOAD								lpprPITIPSUnload;

/* Global Variables */
CURRTABLE       zCTable;
SECTYPETABLE    zSTTable;
SUBACCTTABLE    zSATable;
WITHRECLTABLE   zWRTable;
TRANTYPE			  zTTypeAMDr, zTTypeAMCr;
SYSTEM_SETTINGS	zSysSet;
