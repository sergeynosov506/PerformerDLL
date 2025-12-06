/**
* 
* SUB-SYSTEM: pmr payments.h
* 
* FILENAME: payments.h
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
* $Header: 

// 2/11/2010 - VI#43627 Modified logic to create PIs for those accounts, securities that apply  - mk
**/



#include <math.h>
#include <time.h>

#include "commonheader.h"
#include "accdiv.h"
#include "assets.h"
#include "currency.h"
#include "divint.h"
#include "divhist.h"
#include "dtransdesc.h"
#include "equities.h"
#include "fixedinc.h"
#include "fwtrans.h"
#include "holdings.h"
#include "portmain.h"
#include "porttax.h"
#include "priceinfo.h"
#include "sectype.h"
#include "subacct.h"
#include "syssettings.h"
#include "trans.h"
#include "trantype.h"
#include "withhold_rclm.h"

#include "PaymentsIO.h"

typedef void (CALLBACK* LPPRDIVINTUNL)(DILIBSTRUCT *, long, long, char *, char *, int,int, char *, 
																			 char *, char *, char *, long, ERRSTRUCT *);
typedef void (CALLBACK* LPPRMATURITYUNLOAD)(MATSTRUCT *, long, long, char *, int, char *, char *, char *, char *, ERRSTRUCT *);
typedef ERRSTRUCT	(CALLBACK* LPFNTRANALLOC)(TRANS *, TRANTYPE, SECTYPE, ASSETS, 
																						DTRANSDESC[], int, PAYTRAN *, char *, BOOL);

//typedef ERRSTRUCT (CALLBACK* LPFNPRICEYIELD)(PRICEYLD);
typedef void (CALLBACK* LPFNAMUNLSTATEMENT)(char *, char *, char *, char *, char *, char *, long, AMORTSTRUCT *, ERRSTRUCT *);
typedef void (CALLBACK* LPFNSELECTBSCHDDATEPRICE)(long *, double *, char *, char *,ERRSTRUCT *);
typedef void (CALLBACK* LPFNSELECTMINBSCHDDATEPRICE)(long *, double *, char *, char *, long, ERRSTRUCT *);
typedef void (CALLBACK* LPFN1PCHAR1LONG1PCHAR)(char *, long, char *, ERRSTRUCT *);


//valuationIO  function types
typedef void (CALLBACK* LPPRSELECTNEXTCALLDATEPRICE)(char *, char *, long, double *, long *, ERRSTRUCT *);

//amortize IO function types
typedef void (CALLBACK* LPPRAMORTUNLOAD)(char *, int, char *, char *, char *, char *, long, AMORTSTRUCT *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRUPDATEAMORTIZEHOLDINGS)(HOLDINGS, ERRSTRUCT *);
typedef void (CALLBACK* LPPRFORWARDMATURITYUNLOAD)(FMATSTRUCT *, long, long, char *, int, char *, char *, char *, char *, ERRSTRUCT *);

//PI TIPS IO function types
typedef void (CALLBACK* LPPRPITIPSUNLOAD)(char *, long, int, char *, char *, char *, char *, long, 
																					PITIPSSTRUCT *, ERRSTRUCT *);

//these are Delphi dll in DelphiCInterface
typedef double (CALLBACK* LPPRCALCULATEYIELD)(double, char *, char *, char *,  long, int);
typedef double (CALLBACK* LPPRCALCULATEPRICEGIVENYIELD)(double, char *, char *, char *, long, int);
typedef void	 (CALLBACK* LPPRGETPORTFOLIORANGE)(int*,int*,int,int*,ERRSTRUCT *); 

extern LPFNVOID			lpfnStartTransaction, lpfnCommitTransaction, lpfnRollbackTransaction;
extern LPFNVOID			lpfnGetTransCount;
extern LPFN1BOOL			lpfnAbortTransaction;

extern LPPRERRSTRUCT														lpprInitializeErrStruct;
extern LPFNPRINTERROR														lpfnPrintError;
extern LPFN1PCHAR																lpfnSetErrorFileName;
extern LPPRINITTRANS														lpprInitializeTransStruct;
extern LPFN1LONG3PCHAR1BOOL1PCHAR								lpfnInitTranProc;
extern LPFNTRANALLOC														lpfnTranAlloc;
extern LPPRINITTRANSTABLE2											lpprInitTransTable2;
extern LPFNADDTRANSTOTRANSTABLE2								lpfnAddTransToTransTable2;
extern LPFN1INT2PCHAR4LONG2DOUBLE1BOOL1PDOUBLE	lpfnCalculatePhantomIncome;
extern LPFNCALCULATEINFLATIONRATE								lpfnCalculateInflationRate;
extern LPPRPDTRANSDESC													lpprInitializeDtransDesc;
extern LPPRSELECTHOLDINGS												lpprSelectHoldings;
extern LPFN1PCHAR3LONG													lpfnInflationIndexRatio;

extern LPFNRMDYJUL								lpfnrmdyjul;
extern LPFNRJULMDY								lpfnrjulmdy;
extern LPFN1PCHAR1PLONG						lpfnrstrdate;
extern LPFN1LONG2PCHAR1PLONG			lpfnNextBusinessDay;

extern LPPR3LONG									lpprDeleteAccdivOneLot;
extern LPPRACCDIV									lpprInsertAccdiv;
extern LPPRACCDIV                 lpprUpdateAccdiv;
extern LPPRSELECTONEACCDIV				lpprSelectOneAccdiv;
extern LPPRSELECTALLACCDIV				lpprSelectAllAccdiv;
extern LPPR1PCHAR1INT4PHAR1LONG		lpprUpdateAccdivDeleteFlag;
extern LPPR3LONG1PLONG1PINT				lpprGetLastIncomeDate, lpprGetLastIncomeDateMIPS;
extern LPPRFWTRANS								lpprInsertFWTrans;
extern LPPR1INT										lpprDeleteFWTrans;

extern LPPRPPARTCURR							lpprSelectCurrency;
extern LPPRDIVINTUNL							lpprDivintUnload;

extern LPPRDIVHIST								lpprInsertDivhist;
extern LPPRUPDATEDIVHIST					lpprUpdateDivhist;
extern LPPR3LONG									lpprDeleteDivhist;
extern LPPR2PCHAR4LONG1PCHAR			lpprUpdateDivhistOneLot;
extern LPPR2PCHAR3LONG1PCHAR			lpprUpdateDivhistAllLots;

extern LPPRSELECTPARTFINC					lpprSelectFixedInc;
extern LPPRSECPRICE								lpprGetSecurityPrice;
extern LPPRMATURITYUNLOAD					lpprMaturityUnload;

extern LPPRPARTPMAINALL						lpprSelectAllPartPortmain;
extern LPPRPARTPMAINONE						lpprSelectOnePartPortmain;

extern LPPRPORTTAX								lpprSelectPorttax;
extern LPPRALLSECTYPE							lpprSelectAllSectype;

extern LPPR2PCHAR									lpprSelectSubacct;
extern LPPRSELECTSYSSETTINGS			lpprSelectSysSettings;
extern LPPRSELECTSYSVALUES				lpprSelectSysvalues;
extern LPPRASSETS											lpprSelectAsset;


extern LPPR3LONG									lpprUpdateXrefTransNo;
extern LPPRSELECTTRANTYPE					lpprSelectTranType;
extern LPPRWITHHOLDRCLM						lpprSelectWithrclm;
extern LP2FN1LONG									lpfnLastMonthEnd;
extern LPPRGETPORTFOLIORANGE			lpprGetPortfolioRange;

//ValuationIO  functions
extern LPPRSELECTNEXTCALLDATEPRICE	lpprSelectNextCallDatePrice;
extern LPPR2PLONG										lpprSelectStarsDate;
extern LPPRFORWARDMATURITYUNLOAD		lpprForwardMaturityUnload;
//amortize IO functions
extern LPPRAMORTUNLOAD							lpprAmortizeUnload;
extern LPPRCALCULATEYIELD						lpfnCalculateYield;
extern LPPRCALCULATEPRICEGIVENYIELD	lpfnCalculatePriceGivenYield;
extern LPPRUPDATEAMORTIZEHOLDINGS		lpprUpdateAmortizeHohldings;
extern LPPRSELECTTRANS							lpprSelectTrans;

//PI TIPS IO functions
extern LPPRPITIPSUNLOAD								lpprPITIPSUnload;

/* Global Variables */
extern CURRTABLE      zCTable;
extern SECTYPETABLE   zSTTable;
extern SUBACCTTABLE   zSATable;
extern WITHRECLTABLE  zWRTable;
extern TRANTYPE				zTTypeAMDr, zTTypeAMCr;
extern SYSTEM_SETTINGS	zSysSet;



// Prototype of functions defined in amortize.c 
DLLAPI ERRSTRUCT STDCALL WINAPI Amortize(long lValDate, char *sMode, char *sProcessFlag, int iID, 
																	  char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo);
ERRSTRUCT AmortGeneration(PORTTABLE zPMainTable, PINFOTABLE zPInfoTable, SECTYPETABLE zSTable, CURRTABLE zCTable, 
													long lValDate, char *sMode, char *sProcessFlag, char *sSecNo, char *sWi,
                          char *sSecXtend, char *sAcctType, long lTransNo);
ERRSTRUCT AmortUnloadAndSort(long lValDate);
ERRSTRUCT BuildAmortTable(char *sMode, char *sFileName, int iID, char *sSecNo, char *sWi, 
													char *sSecXtend, char *sAcctType, long lTransNo,AMORTTABLE *pzDITable);
ERRSTRUCT AddRecordToAmortTable(AMORTTABLE *pzATab, AMORTSTRUCT zAmortStruct);
ERRSTRUCT AmortCallTranAlloc(AMORTSTRUCT zAmort, double fBaseExrate, double fOldCost,
                             char *sAcctMthd, SECTYPE zSType, long lValDate); 

void      InitializeAmortStruct(AMORTSTRUCT *pzAmort);
void      InitializeAmortTable(AMORTTABLE *pzATable);
BOOL      IsThisAMonthEnd(long lDate);
void			InitializeHoldingsStruct(HOLDINGS *pzHoldings);


// Prototype of functions defined in digenerate1.c 
DLLAPI ERRSTRUCT STDCALL WINAPI GenerateDivInt(long lValDate, char *sMode, char *sType, int iID, char *sSecNo, char *sWi, 
																					char *sSecXtend, char *sAcctType, long lTransNo);
ERRSTRUCT BuildSubacctTable(SUBACCTTABLE *pzSATable);
ERRSTRUCT BuildCurrencyTable(CURRTABLE *pzCTable);
ERRSTRUCT BuildPortmainTable(PORTTABLE *pzPTable, char *sMode, int iID, CURRTABLE zCTable);
ERRSTRUCT BuildPInfoTable(PINFOTABLE *pzPTable, char *sLibName, long lValDate, char *sType);
ERRSTRUCT BuildDITable(char *sMode, char *sType, char *sFileName, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
											 char *sAcctType, long lTransNo, long lStartDate, long lValDate, long lStartPosition, DILIBTABLE *pzDITable);
ERRSTRUCT AddRecordToDILibTable(DILIBTABLE *pzDITab, DILIBSTRUCT zDILStruct);
ERRSTRUCT DivintGeneration(PORTTABLE zPdirTable, PINFOTABLE zPInfoTable, char *sSecNo, char *sWi, char *sSecXtend,
                           char *sAcctType, long lTransNo, long lStartDate, long lValdate, long iForwardDate, char *sMode, 
													 char *sType, SUBACCTTABLE zSTable, CURRTABLE zCTable);
ERRSTRUCT CalculateTotals(DILIBSTRUCT *pzDILS, double *pfTotalUnits, double *pzNewTotUnits, double *pfTotalIncome, 
													double *pfSumTrUnits, double *pfSumRdUnits, double *pfSumIncome, long lBusinessValDate);
ERRSTRUCT VerifyAndCommit(DILIBTABLE zDTable, long lValDate, long lForwardDate, long lBusinessValDate, SUBACCTTABLE zSTable, 
													CURRTABLE zCTable, char *sAcctMethod, int iPortCurrIndx,  double fTotalUnits, double fTotalNUnits,
                          double fTotalPcpl, double fSumTrUnits, double fSumRdUnits, double fSumPcpl, long lDivintNo);
void      CreateDivhistRecord(DILIBSTRUCT zDILS, char *sTranType, DIVHIST *pzDivhist);
ERRSTRUCT CreateAccdivRecord(DILIBSTRUCT zDILS, long lValDate, SUBACCTTABLE zSTable, double fBaseCurExrate,
                             char *sAcctMethod, ACCDIV *pzAccdiv);
ERRSTRUCT CreateFWTransRecord(ACCDIV zAccdiv, int iSecType, FWTRANS *zFWTrans);
void      CalcNewUnits(char *sSplitInd, double fQuotient, double fProduct, 
                      double fOldUnits, double *pzTruncUnits, double *pzRndUnits);
ERRSTRUCT CalcSplitFactor(double fSplitRate, double *pzQuotient,double *pzProduct);
int       FindAcctTypeInSubacctTable(SUBACCTTABLE zSTable, char *sAcctType);
int				FindCurrIdInCurrencyTable(CURRTABLE zCTable, char *sCurrId);


// Prototype of functions defined in digenerate2.c 
DLLAPI ERRSTRUCT STDCALL WINAPI InitPayments(char *sDBPath, char *sType, char *sMode, long lAsofDate, char *sErrFile);
void      InitializeDILibStruct(DILIBSTRUCT *pzDILStruct);
void      InitializeDILibTable(DILIBTABLE *pzDILTable);
void			InitializePInfoTable(PINFOTABLE *pzPITable);
void			InitializePortTable(PORTTABLE *pzPTable);
void			InitializeMaturityTable(MATTABLE *pzMTable);
void      InitializeDivint(DIVINT *pzDI);
void      InitializeDivhist(DIVHIST *pzDH);
void			InitializeAccdiv(ACCDIV *pzAD);
void			InitializeFWTrans(FWTRANS *pzFWTrans);
ERRSTRUCT DivintUnloadAndSort(long lValDate, long lStartDate, char *sType);
BOOL      FileExists(char *sFileName);
char *		MakePricingFileName(long lDate, char *sType, char *sLibName);
long			GetPaymentsEndingDate(long lValDate);
void			FreePayments();

// Prototype of functions defined in dipay.c 
DLLAPI ERRSTRUCT STDCALL WINAPI PayDivInt(long lValDate, char *sMode, char *sProcessFlag, int iID, char *sSecNo, 
																		 char *sWi, char *sSecXtend, char *sAcctType, long lTransNo);
int				FindAcctInPortdirTable(PORTTABLE zPTable, int iID);
int				FindCurrIdInWithRclTable(WITHRECLTABLE zWRTable, char *sCurrId);
ERRSTRUCT BuildWithReclTable(WITHRECLTABLE *pzWRTable);
ERRSTRUCT GetPortmainInfo(PORTTABLE zPortTable, int iID, long lTrdDate, double *pzPortBaseXrate, 
													BOOL *bIncByLot, double *pzSwh, double *pzBwh);       
ERRSTRUCT CreateTransFromAccdiv(TRANS *pzTR, ACCDIV zAccdiv, CURRTABLE zCTable, double zPortBaseXrate,
                                double zExRate, double zIncExRate, char *sPrimaryType, long lValDate);
ERRSTRUCT PostTrades(TRANS zTrans, TRANS zTransWH, TRANS zTransAR, TRANTYPE zTranType, 
										 SECTYPE zSecType, ASSETS zAssets, BOOL bIncByLot);
ERRSTRUCT GetWithholdingsRate(int iID, long lTrdDate, char *sCurrId, int iPortCurrIndex,
															short iFiscalEndMonth, short iFiscalEndDay, double *pfEWhRate, 
															double *pfBWhRate, double *pfERcRate, double *pfBRcRate);
void      CopyFieldsFromTransToAssets(TRANS zTR, double zTrdUnit, ASSETS *pzAssets);


// Prototype of functions defined in forwardmaturity.c 
DLLAPI ERRSTRUCT STDCALL WINAPI GenerateForwardMaturity(long lValDate, char *sMode, int iID, 
																						char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType);
ERRSTRUCT ForwardMaturityGeneration(PORTTABLE zPmainTable, PINFOTABLE zPInfoTable, SUBACCTTABLE zSTable, 
																		CURRTABLE zCTable, char *sMode, char *sSecNo, char *sWi, 
																		char *sSecXtend, char *sAcctType, long lStartDate, long lValDate, long lForwardDate);
ERRSTRUCT BuildForwardMatTable(char *sMode, char *sFileName, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
															 char *sAcctType, long lStartDate, long lValDate, long lStartPosition, FORWARDMATSTRUCT *pzFMTable);
ERRSTRUCT ForwardMaturityUnloadAndSort(long lValDate, long lStartDate);
ERRSTRUCT CreateTrans(FMATSTRUCT zFMat, double fUnits, double fOrigFace, double fBaseCurrExrate, char *sAcctMethod, 
                      char *sIncAcctType, long lValDate,double OpenLiability,double CurrLiability);
void			InitializeForwardMatStruct(FMATSTRUCT* pzTempMat);
ERRSTRUCT AddRecordToForwardMatTable(FORWARDMATSTRUCT *pzMTab, FMATSTRUCT zMatStruct);
void			InitializeForwardMatTable(FORWARDMATSTRUCT *pzMTable);


// Prototype of functions defined in maturity.c
DLLAPI ERRSTRUCT STDCALL WINAPI GenerateMaturity(long lValDate, char *sMode, int iID, char *sSecNo, 
																						char *sWi, char *sSecXtend, char *sAcctType);
ERRSTRUCT MaturityGeneration(PORTTABLE zPmainTable, PINFOTABLE zPInfoTable, SUBACCTTABLE zSTable, CURRTABLE zCTable, 
                             char *sMode, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, 
														 long lStartDate, long lValDate, long lForwardDate);
void      InitializeMatStruct(MATSTRUCT *pzMStruct);
void      InitializeMatTable(MATTABLE *pzMTable);
ERRSTRUCT BuildSecTypeTable(SECTYPETABLE *pzSTable);
ERRSTRUCT MaturityUnloadAndSort(long lValDate, long lStartDate);
ERRSTRUCT BuildMatTable(char *sMode, char *sFileName, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
                        char *sAcctType, long lStartDate, long lvalDate, long lStartingPosition, MATTABLE *pzMatTable);
ERRSTRUCT AddRecordToMatTable(MATTABLE *pzMTab, MATSTRUCT zMatStruct);
ERRSTRUCT CreateTransAndCallTranAlloc(MATSTRUCT zMat, double fUnits, double fTotCost, double fOrigFace, 
																			char *sBaseCurrId, char *sAcctMethod, char *sIncAcctType, long lValDate);
ERRSTRUCT CreateAndInsertFWTrans(TRANS zTrans,char PayType[]);
int				FindSecType(SECTYPETABLE zSTypeTable, int iSType);
