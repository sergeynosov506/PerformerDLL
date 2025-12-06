#include "commonheader.h"
#include "valuation.h"

// struct variables
SECTYPETABLE	zSTTable;
CURRTABLE		zCurrencyTable;
SYSTEM_SETTINGS	zSystemSettings;

/// function variables
LPPR3PCHAR1LONG1INT					lpprStarsIOInit;
LPFNVOID							lpfnStartTransaction, lpfnRollbackTransaction, lpfnCommitTransaction;
LPFNVOID							lpfnGetTransCount;
LPFN1BOOL							lpfnAbortTransaction;
LPPRVOID							lpprCloseQueries;

LPPRSELECTONEACCDIV					lpprSelectAccdiv;
LPPRALLACCDIVFORANACCOUNT			lpprSelectAllAccdivForAnAccount;
LPPRPENDINGACCDIVTRANSFORANACCOUNT	lpprSelectPendingAccdivTransForAnAccount;
LPPRACCDIV							lpprUpdateAccdiv;

LPPRALLDIVHISTFORANACCOUNT			lpprSelectAllDivhistForAnAccount;
LPPRDIVHIST							lpprUpdateDivhist;

LPPR2PCHAR1LONG1PDOUBLE1PLONG		lpprSelectNextCallDatePrice;

LPPRERRSTRUCT						lpprInitializeErrStruct;

LPPRSELECTPARTEQTY					lpprSelectEquity;

LPFN1PCHAR							lpfnSetErrorFile;

LPPRSELECTPARTFINC					lpprSelectFixedinc;

LPFNCALCGAINLOSS					lpfnCalcGainLoss;

LPPRSECPRICE						lpprGetSecurityPrice;

LPPRALLHXREFFORANACCOUNT			lpprSelectAllHxrefForAnAccount;
LPPRHEDGEXREF						lpprUpdateHedgexref;

LPPRALLHOLDCASHFORANACCOUNT			lpprSelectAllHoldcashForAnAccount;
LPPRHOLDCASH						lpprUpdateHoldcash;
LPPRINITHOLDCASH					lpprInitHoldcash;

LPPRALLHOLDINGSFORANACCOUNT			lpprSelectAllHoldingsForAnAccount;
LPPRHOLDINGS						lpprUpdateHoldings;
LPPRINITHOLDINGS					lpprInitHoldings;

LPPRALLPAYRECFORANACCOUNT			lpprSelectAllPayrecForAnAccount;
LPPRPAYREC							lpprUpdatePayrec;

LPFNPRINTERROR						lpfnPrintError;

//LPPRSELECTPORTBAL					lpprSelectPortbal;
//LPPRUPDATEPORTBAL					lpprUpdatePortbal;

LPPRPARTPMAINONE					lpprSelectOnePmain;
LPPRPARTPMAINALL					lpprSelectAllPmain;
LPPR1INT1LONG						lpprUpdateValDate;

LPPRALLSECTYPES						lpprSelectAllSectypes;

LPPR2PLONG							lpprSelectStarsDate;

LPPRSELECTTRANS						lpprSelectTrans;

LPFN1DOUBLE3PCHAR1LONGINT			lpfnCalculateYield;

LPPRSELECTSYSSETTINGS				lpprSelectSysSettings;
LPPRSELECTSYSVALUES					lpprSelectSysvalues;
LPFN1LONG2PCHAR1PLONG				lpfnLastBusinessDay, lpfnNextBusinessDay;
LPFN1LONG1PCHAR						lpfnIsItAMarketHoliday;
LPFNSELECTDIVINT					lpfnSelectDivint;
LPPRSELECTCURRENCYSECNO				lpprSelectCurrencySecno;

LPPRPPARTCURR						lpprSelectPartCurrency;
LPFNSELECTDIVINT					lpfnSelectSecurityRate;
LPFNITSMANUALPRICESECURITY			lpfnItsManualPriceSecurity;
//LPFN1INT							lpfnTimer;
//LP2PR1PCHAR						lpprTimerResult;
LPFN1PCHAR3LONG						lpfnInflationIndexRatio;
LPPRASSETS							lpprSelectAsset;
LPPR1INT4PCHAR2LONG1PDOUBLE			lpprSelectUnitsHeldForASecurity;
LPPR1INT4PCHAR3LONG1PDOUBLE			lpprSelectUnitsForASoldSecurity;
LPPRCUSTOMPRICE						lpprSelectCustomPriceForAnAccount;
