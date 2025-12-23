/**
 *
 * SUB-SYSTEM: payments
 *
 * FILENAME: digenerate2.c
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
 **/

#include "payments.h"

HINSTANCE hDelphiCDll, hStarsIODll, hStarsUtilsDll, hTEngineDll;
static BOOL bInit = FALSE;

ERRSTRUCT STDCALL WINAPI InitPayments(const char *sDBPath, const char *sType,
                                      const char *sMode, long lAsofDate,
                                      const char *sErrFile) {
  int iError;
  ERRSTRUCT zErr;
  SYSVALUES zSysvalues;

  if (!bInit) {
    // Load "TransEngine" dll created in C
    hTEngineDll = LoadLibrarySafe("TransEngine.dll");
    if (hTEngineDll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    lpprInitializeErrStruct =
        (LPPRERRSTRUCT)GetProcAddress(hTEngineDll, "InitializeErrStruct");
    if (!lpprInitializeErrStruct) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    lpprInitializeErrStruct(&zErr);

    lpfnPrintError = (LPFNPRINTERROR)GetProcAddress(hTEngineDll, "PrintError");
    if (!lpfnPrintError) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    lpfnSetErrorFileName =
        (LPFN1PCHAR)GetProcAddress(hTEngineDll, "SetErrorFileName");
    if (!lpfnSetErrorFileName) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    zErr.iBusinessError = lpfnSetErrorFileName((char *)sErrFile);
    if (zErr.iBusinessError != 0)
      return zErr;

    // Load "DelphiCInterface" dll created in Delphi
    hDelphiCDll = LoadLibrarySafe("DelphiCInterface.dll");
    if (hDelphiCDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DelphiCInterface Dll", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT1", FALSE));
    }

    // Load "OLEDBIO" dll created in C
    hStarsIODll = LoadLibrarySafe("OLEDBIO.dll");
    if (hStarsIODll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading StarsIO DLL", 0, 0, "", iError, 0,
                             0, "PAYMENTS INIT2", FALSE));
    }

    // Load "StasUtils" dll created in Delphi
    hStarsUtilsDll = LoadLibrarySafe("StarsUtils.dll");
    if (hStarsUtilsDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading StarsUtils Dll", 0, 0, "", iError,
                             0, 0, "PAYMENTS INIT3", FALSE));
    }

    // Load functions from DelphiCInterface.Dll
    lpfnCalculatePriceGivenYield = (LPPRCALCULATEPRICEGIVENYIELD)GetProcAddress(
        hDelphiCDll, "CalulatePriceGivenYield");
    if (!lpfnCalculatePriceGivenYield) {
      iError = GetLastError();
      return (
          lpfnPrintError("Error Loading lpfnCalculatePriceGivenYield Function",
                         0, 0, "", iError, 0, 0, "PAYMENTS INIT4", FALSE));
    }

    lpfnCalculateYield =
        (LPPRCALCULATEYIELD)GetProcAddress(hDelphiCDll, "CalulateYield");
    if (!lpfnCalculateYield) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading lpfnCalculateYield Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT5", FALSE));
    }

    lpfnInflationIndexRatio = (LPFN1PCHAR3LONG)GetProcAddress(
        hDelphiCDll, "CheckForInflationIndexRatio_c");
    if (!lpfnInflationIndexRatio) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load CheckForInflationIndexRatio function",
                         0, 0, "", iError, 0, 0, "PAYMENTS INIT5A", FALSE));
    }

    // Load functions from StarsIO.Dll
    lpprDeleteAccdivOneLot =
        (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteAccdivOneLot");
    if (!lpprDeleteAccdivOneLot) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DeleteAccdivOneLot Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT6", FALSE));
    }

    lpprInsertAccdiv = (LPPRACCDIV)GetProcAddress(hStarsIODll, "InsertAccdiv");
    if (!lpprInsertAccdiv) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading InsertAccdiv Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT7", FALSE));
    }

    lpprSelectOneAccdiv =
        (LPPRSELECTONEACCDIV)GetProcAddress(hStarsIODll, "SelectOneAccdiv");
    if (!lpprSelectOneAccdiv) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectOneAccdiv Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT8", FALSE));
    }

    lpprSelectAllAccdiv =
        (LPPRSELECTALLACCDIV)GetProcAddress(hStarsIODll, "SelectAllAccdiv");
    if (!lpprSelectAllAccdiv) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectAllAccdiv Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT9", FALSE));
    }

    lpprUpdateAccdiv = (LPPRACCDIV)GetProcAddress(hStarsIODll, "UpdateAccdiv");
    if (!lpprUpdateAccdiv) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading UpdateAccdiv Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT10", FALSE));
    }

    lpprUpdateAccdivDeleteFlag = (LPPR1PCHAR1INT4PHAR1LONG)GetProcAddress(
        hStarsIODll, "UpdateAccdivDeleteFlag");
    if (!lpprUpdateAccdivDeleteFlag) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading UpdateAccdivDeleteFlag Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT11", FALSE));
    }

    lpprAmortizeUnload =
        (LPPRAMORTUNLOAD)GetProcAddress(hStarsIODll, "AmortizeUnload");
    if (!lpprAmortizeUnload) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AmortizeUnload Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT12", FALSE));
    }

    lpprPITIPSUnload =
        (LPPRPITIPSUNLOAD)GetProcAddress(hStarsIODll, "PITIPSUnload");
    if (!lpprPITIPSUnload) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading PITIPSUnload Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT12a", FALSE));
    }

    lpprUpdateAmortizeHohldings = (LPPRUPDATEAMORTIZEHOLDINGS)GetProcAddress(
        hStarsIODll, "UpdateAmortizeHoldings");
    if (!lpprUpdateAmortizeHohldings) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading UpdateAmortizeHoldings Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT13", FALSE));
    }

    lpprSelectCurrency =
        (LPPRPPARTCURR)GetProcAddress(hStarsIODll, "SelectPartCurrency");
    if (!lpprSelectCurrency) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectCurrency Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT14", FALSE));
    }

    lpfnCommitTransaction =
        (LPFNVOID)GetProcAddress(hStarsIODll, "CommitDBTransaction");
    if (!lpfnCommitTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading CommitDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT15", FALSE));
    }

    lpfnRollbackTransaction =
        (LPFNVOID)GetProcAddress(hStarsIODll, "RollbackDBTransaction");
    if (!lpfnRollbackTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading RollbackDBTransaction Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT16", FALSE));
    }

    lpfnStartTransaction =
        (LPFNVOID)GetProcAddress(hStarsIODll, "StartDBTransaction");
    if (!lpfnStartTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading StartDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT17", FALSE));
    }

    lpfnGetTransCount = (LPFNVOID)GetProcAddress(hStarsIODll, "GetTransCount");
    if (!lpfnGetTransCount) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading GetTransCount Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT17a", FALSE));
    }

    lpfnAbortTransaction =
        (LPFN1BOOL)GetProcAddress(hStarsIODll, "AbortDBTransaction");
    if (!lpfnAbortTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AbortDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT17b", FALSE));
    }

    lpprSelectNextCallDatePrice = (LPPRSELECTNEXTCALLDATEPRICE)GetProcAddress(
        hStarsIODll, "SelectNextCallDatePrice");
    if (!lpprSelectNextCallDatePrice) {
      iError = GetLastError();
      return (
          lpfnPrintError("Error Loading lpprSelectNextCallDatePrice Function",
                         0, 0, "", iError, 0, 0, "PAYMENTS INIT18", FALSE));
    }

    lpprDivintUnload =
        (LPPRDIVINTUNL)GetProcAddress(hStarsIODll, "DivintUnload");
    if (!lpprDivintUnload) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DivintUnload Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT19", FALSE));
    }

    lpprDeleteDivhist = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteDivhist");
    if (!lpprDeleteDivhist) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DeleteDivhist Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT20", FALSE));
    }

    lpprInsertDivhist =
        (LPPRDIVHIST)GetProcAddress(hStarsIODll, "InsertDivhist");
    if (!lpprInsertDivhist) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading InsertDivhist Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT21", FALSE));
    }

    lpprUpdateDivhist =
        (LPPRUPDATEDIVHIST)GetProcAddress(hStarsIODll, "UpdateDivhist");
    if (!lpprUpdateDivhist) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading UpdateDivhist Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT22", FALSE));
    }

    lpprUpdateDivhistOneLot = (LPPR2PCHAR4LONG1PCHAR)GetProcAddress(
        hStarsIODll, "UpdateDivhistOneLot");
    if (!lpprUpdateDivhistOneLot) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateDivhistOneLot function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT22a", FALSE));
    }

    lpprUpdateDivhistAllLots = (LPPR2PCHAR3LONG1PCHAR)GetProcAddress(
        hStarsIODll, "UpdateDivhistAllLots");
    if (!lpprUpdateDivhistAllLots) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateDivhistAllLots function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT22b", FALSE));
    }

    lpprSelectFixedInc =
        (LPPRSELECTPARTFINC)GetProcAddress(hStarsIODll, "SelectPartFixedinc");
    if (!lpprSelectFixedInc) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectFixedInc Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT23", FALSE));
    }

    lpprDeleteFWTrans = (LPPR1INT)GetProcAddress(hStarsIODll, "DeleteFWTrans");
    if (!lpprDeleteFWTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DeleteFWTrans Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT24", FALSE));
    }

    lpprInsertFWTrans =
        (LPPRFWTRANS)GetProcAddress(hStarsIODll, "InsertFWTrans");
    if (!lpprInsertFWTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading InsertFWTrans Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT25", FALSE));
    }

    lpprForwardMaturityUnload = (LPPRFORWARDMATURITYUNLOAD)GetProcAddress(
        hStarsIODll, "ForwardMaturityUnload");
    if (!lpprForwardMaturityUnload) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading lpprForwardMaturityUnload Function",
                             0, 0, "", iError, 0, 0, "PAYMENTS INIT26", FALSE));
    }

    lpprGetSecurityPrice =
        (LPPRSECPRICE)GetProcAddress(hStarsIODll, "GetSecurityPrice");
    if (!lpprGetSecurityPrice) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load GetSecurityPrice Procedure", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT26A", FALSE));
    }

    lpprMaturityUnload =
        (LPPRMATURITYUNLOAD)GetProcAddress(hStarsIODll, "MaturityUnload");
    if (!lpprMaturityUnload) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading MaturityUnload Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT27", FALSE));
    }

    lpprSelectAllPartPortmain =
        (LPPRPARTPMAINALL)GetProcAddress(hStarsIODll, "SelectAllPartPortmain");
    if (!lpprSelectAllPartPortmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectAllPartPortmain Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT28", FALSE));
    }

    lpprSelectOnePartPortmain =
        (LPPRPARTPMAINONE)GetProcAddress(hStarsIODll, "SelectOnePartPortmain");
    if (!lpprSelectOnePartPortmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectOnePartPortmain Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT29", FALSE));
    }

    lpprGetPortfolioRange =
        (LPPRGETPORTFOLIORANGE)GetProcAddress(hStarsIODll, "GetPortfolioRange");
    if (!lpprGetPortfolioRange) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading lpprGetPortfolioRange Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT30", FALSE));
    }

    lpprSelectPorttax =
        (LPPRPORTTAX)GetProcAddress(hStarsIODll, "SelectPorttax");
    if (!lpprSelectPorttax) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectPorttax Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT31", FALSE));
    }

    lpprSelectAllSectype =
        (LPPRALLSECTYPE)GetProcAddress(hStarsIODll, "SelectAllSectypes");
    if (!lpprSelectAllSectype) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectAllSectypes Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT32", FALSE));
    }

    lpprSelectStarsDate =
        (LPPR2PLONG)GetProcAddress(hStarsIODll, "SelectStarsDate");
    if (!lpprSelectStarsDate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading lpprSelectStarsDate Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT33", FALSE));
    }

    lpprSelectSubacct =
        (LPPR2PCHAR)GetProcAddress(hStarsIODll, "SelectAllSubacct");
    if (!lpprSelectSubacct) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectSubacct Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT34", FALSE));
    }

    lpprSelectSysSettings =
        (LPPRSELECTSYSSETTINGS)GetProcAddress(hStarsIODll, "SelectSysSettings");
    if (!lpprSelectSysSettings) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading lpprSelectSysSettings Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT35", FALSE));
    }

    lpprUpdateXrefTransNo =
        (LPPR3LONG)GetProcAddress(hStarsIODll, "UpdateXrefTransNo");
    if (!lpprUpdateXrefTransNo) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading UpdateXrefTransNo Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT36", FALSE));
    }

    lpprSelectTranType =
        (LPPRSELECTTRANTYPE)GetProcAddress(hStarsIODll, "SelectTrantype");
    if (!lpprSelectTranType) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectTranType Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT37", FALSE));
    }

    lpprSelectWithrclm =
        (LPPRWITHHOLDRCLM)GetProcAddress(hStarsIODll, "SelectWithrclm");
    if (!lpprSelectWithrclm) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectWithrclm Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT38", FALSE));
    }

    lpprGetLastIncomeDate =
        (LPPR3LONG1PLONG1PINT)GetProcAddress(hStarsIODll, "GetLastIncomeDate");
    if (!lpprGetLastIncomeDate) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load GetLastIncomeDate function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT38A", FALSE));
    }

    lpprGetLastIncomeDateMIPS = (LPPR3LONG1PLONG1PINT)GetProcAddress(
        hStarsIODll, "GetLastIncomeDateMIPS");
    if (!lpprGetLastIncomeDateMIPS) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load GetLastIncomeDateMIPS function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT38B", FALSE));
    }

    lpprSelectTrans =
        (LPPRSELECTTRANS)GetProcAddress(hStarsIODll, "SelectOneTrans");
    if (!lpprSelectTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectOneTrans Procedure", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT38C", FALSE));
    }

    lpprInitializeDtransDesc =
        (LPPRPDTRANSDESC)GetProcAddress(hTEngineDll, "InitializeDtransDesc");
    if (!lpprInitializeDtransDesc) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitializeDtransDesc function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT38D", FALSE));
    }

    // Load functions from StarsUtils.Dll
    lpfnrmdyjul = (LPFNRMDYJUL)GetProcAddress(hStarsUtilsDll, "rmdyjul");
    if (!lpfnrmdyjul) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading rmdyjul Function", 0, 0, "", iError,
                             0, 0, "PAYMENTS INIT39", FALSE));
    }

    lpfnrjulmdy = (LPFNRJULMDY)GetProcAddress(hStarsUtilsDll, "rjulmdy");
    if (!lpfnrjulmdy) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading rjulmdy Function", 0, 0, "", iError,
                             0, 0, "PAYMENTS INIT40", FALSE));
    }

    lpfnrstrdate = (LPFN1PCHAR1PLONG)GetProcAddress(hStarsUtilsDll, "rstrdate");
    if (!lpfnrstrdate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading rstrdate Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT41", FALSE));
    }

    lpfnNextBusinessDay = (LPFN1LONG2PCHAR1PLONG)GetProcAddress(
        hStarsUtilsDll, "NextBusinessDay");
    if (!lpfnNextBusinessDay) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading NextBusinessDay Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT42", FALSE));
    }

    lpfnLastMonthEnd =
        (LP2FN1LONG)GetProcAddress(hStarsUtilsDll, "LastMonthEnd");
    if (!lpfnLastMonthEnd) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading lpfnLastMonthEnd Function", 0, 0,
                             "", iError, 0, 0, "PAYMENTS INIT43", FALSE));
    }

    // Load Rest of the functions from TransEngine.Dll
    lpfnInitTranProc =
        (LPFN1LONG3PCHAR1BOOL1PCHAR)GetProcAddress(hTEngineDll, "InitTranProc");
    if (!lpfnInitTranProc) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading InitTranProc Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT44", FALSE));
    }

    lpfnTranAlloc = (LPFNTRANALLOC)GetProcAddress(hTEngineDll, "TranAlloc");
    if (!lpfnTranAlloc) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading TranAlloc Function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT45", FALSE));
    }

    lpprInitializeTransStruct =
        (LPPRINITTRANS)GetProcAddress(hTEngineDll, "InitializeTransStruct");
    if (!lpprInitializeTransStruct) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading InitializeTransStruct Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT46", FALSE));
    }

    lpprInitTransTable2 = (LPPRINITTRANSTABLE2)GetProcAddress(
        hTEngineDll, "InitializeTransTable2");
    if (!lpprInitTransTable2) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading InitializeTransTable2 Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT47", FALSE));
    }

    lpfnAddTransToTransTable2 = (LPFNADDTRANSTOTRANSTABLE2)GetProcAddress(
        hTEngineDll, "AddTransToTransTable2");
    if (!lpfnAddTransToTransTable2) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AddTransToTransTable2 Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT48", FALSE));
    }

    lpfnCalculatePhantomIncome =
        (LPFN1INT2PCHAR4LONG2DOUBLE1BOOL1PDOUBLE)GetProcAddress(
            hTEngineDll, "CalculatePhantomIncome");
    if (!lpfnCalculatePhantomIncome) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading CalculatePhantomIncome Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT49", FALSE));
    }

    lpfnCalculateInflationRate = (LPFNCALCULATEINFLATIONRATE)GetProcAddress(
        hTEngineDll, "CalculateInflationRate");
    if (!lpfnCalculateInflationRate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading CalculateInflationRate Function", 0,
                             0, "", iError, 0, 0, "PAYMENTS INIT49a", FALSE));
    }

    lpprSelectHoldings =
        (LPPRSELECTHOLDINGS)GetProcAddress(hStarsIODll, "SelectHoldings");
    if (!lpprSelectHoldings) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectHoldings function", 0, 0, "",
                             iError, 0, 0, "PAYMENTS INIT50", FALSE));
    }

    lpprSelectSysvalues =
        (LPPRSELECTSYSVALUES)GetProcAddress(hStarsIODll, "SelectSysvalues");
    if (!lpprSelectSysvalues) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSysvalues", 0, 0, "", iError,
                             0, 0, "PAYMENTS INIT51", FALSE));
    }

    /* Build the global table of all the currencies in the system */
    zErr = BuildCurrencyTable(&zCTable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Build the global table of all sectypes in the system */
    zErr = BuildSecTypeTable(&zSTTable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Build the global table of all the subaccoubts in the system */
    zErr = BuildSubacctTable(&zSATable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Build the global table of all witholdings/reclaims in the system */
    zErr = BuildWithReclTable(&zWRTable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* load tran type record for amortization Dr */
    lpprSelectTranType(&zTTypeAMDr, "AM", "DR", &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* load tran type record for amortization Cr */
    lpprSelectTranType(&zTTypeAMCr, "AM", "CR", &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    // load system settings
    memset(&zSysSet, sizeof(zSysSet), 0);
    lpprSelectSysSettings(&zSysSet.zSyssetng, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    // by default, boolean options are set to FALSE
    // in the database, they have to be set as 1 or 0 (i.e. numbers)
    // not as strings/characters (i.e. T, Y, TRUE, etc would mean 0 -> FALSE)
    strcpy_s(zSysvalues.sName, "RecalcCostEffMatYld");
    lpprSelectSysvalues(&zSysvalues, &zErr);
    if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
      zSysSet.bRecalcCostEffMatYld = atoi(zSysvalues.sValue);
    } else if (zErr.iSqlError == SQLNOTFOUND) {
      lpprInitializeErrStruct(&zErr);
    } else
      return zErr;

    // don't want to go too far back for generating payments
    if (zSysSet.zSyssetng.iPaymentsStartDate <= 0 ||
        zSysSet.zSyssetng.iPaymentsStartDate >= 365)
      zSysSet.zSyssetng.iPaymentsStartDate = 90;

    bInit = TRUE;
  } // if never initialized before
  else
    lpprInitializeErrStruct(&zErr);

  // Initialize Tranproc
  zErr = lpfnInitTranProc(lAsofDate, (char *)sDBPath, (char *)"", (char *)"",
                          FALSE, (char *)sErrFile);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  return zErr;
} // InitPayments

/**
** Function to initialize divintlibstruct
**/
void InitializeDILibStruct(DILIBSTRUCT *pzDILStruct) {
  pzDILStruct->sTableName[0] = '\0';
  pzDILStruct->iID = 0;
  pzDILStruct->sSecNo[0] = pzDILStruct->sWi[0] = '\0';
  pzDILStruct->sSecXtend[0] = pzDILStruct->sSecSymbol[0] = '\0';
  pzDILStruct->sAcctType[0] = '\0';
  pzDILStruct->lTransNo = 0;
  pzDILStruct->fUnits = 0;
  pzDILStruct->lEffDate = pzDILStruct->lEligDate = pzDILStruct->lStlDate = 0;
  pzDILStruct->fOrigFace = pzDILStruct->fOrigYield = 0;
  pzDILStruct->lEffMatDate = 0;
  pzDILStruct->fEffMatPrice = 0;
  pzDILStruct->sOrigTransType[0] = '\0';
  pzDILStruct->lCreateDate = 0;
  pzDILStruct->sSafekInd[0] = '\0';
  pzDILStruct->iSecID = 0;
  pzDILStruct->fTrdUnit = 0;
  pzDILStruct->iSecType = 0;
  pzDILStruct->sAutoAction[0] = pzDILStruct->sAutoDivint[0] = '\0';
  pzDILStruct->sCurrId[0] = pzDILStruct->sIncCurrId[0] = '\0';
  pzDILStruct->fCurExrate = pzDILStruct->fCurIncExrate = 1;
  pzDILStruct->lDivintNo = 0;
  pzDILStruct->sDivType[0] = '\0';
  pzDILStruct->lExDate = pzDILStruct->lRecDate = pzDILStruct->lPayDate = 0;
  pzDILStruct->fDivRate = 0;
  pzDILStruct->sPostStatus[0] = '\0';
  pzDILStruct->lModifyDate = pzDILStruct->lFwdDivintNo =
      pzDILStruct->lPrevDivintNo = 0;
  pzDILStruct->sDeleteFlag[0] = '\0';
  pzDILStruct->sProcessFlag[0] = pzDILStruct->sProcessType[0] = '\0';
  pzDILStruct->sShortSettle[0] = pzDILStruct->sTranType[0] = '\0';
  pzDILStruct->sInclUnits[0] = pzDILStruct->sSplitInd[0] = '\0';
  pzDILStruct->lDivTransNo = 0;
  pzDILStruct->sTranLocation[0] = '\0';
  pzDILStruct->bNullDivhist = TRUE;
  pzDILStruct->bProcessLot = FALSE;
  pzDILStruct->sTruncRnd[0] = '\0';
  pzDILStruct->fRndUnits = 0;
  pzDILStruct->fTruncUnits = 0;
  pzDILStruct->fIncomeAmt = 0;
} /* InitializeDilibStruct */

/**
** Function to initialize DILibtable
**/
void InitializeDILibTable(DILIBTABLE *pzDILTable) {
  if (pzDILTable->iDIRecCreated > 0 && pzDILTable->pzDIRec != NULL)
    free(pzDILTable->pzDIRec);

  pzDILTable->pzDIRec = NULL;
  pzDILTable->iDIRecCreated = pzDILTable->iNumDIRec = 0;
} /* InitializeDilibtable */

/**
** Function to initialize PInfoTable
**/
void InitializePInfoTable(PINFOTABLE *pzPITable) {
  if (pzPITable->iPICreated > 0 && pzPITable->pzPRec != NULL)
    free(pzPITable->pzPRec);

  pzPITable->pzPRec = NULL;
  pzPITable->iPICreated = pzPITable->iNumPI = 0;
} /* InitializePInfoTable */

/**
** Function to initialize porttable
**/
void InitializePortTable(PORTTABLE *pzPTable) {
  if (pzPTable->iPmainCreated > 0 && pzPTable->pzPmain != NULL)
    free(pzPTable->pzPmain);

  pzPTable->pzPmain = NULL;
  pzPTable->iNumPmain = pzPTable->iPmainCreated = 0;
} /* InitializePortTable */

/**
** Function to initialize MATTABLE
**/
void InitializeMaturityTable(MATTABLE *pzMTable) {
  if (pzMTable->iMatCreated > 0 && pzMTable->pzMaturity != NULL)
    free(pzMTable->pzMaturity);

  pzMTable->pzMaturity = NULL;
  pzMTable->iNumMat = pzMTable->iMatCreated = 0;
}

/**
** Function to initialize divint structure
**/
void InitializeDivint(DIVINT *pzDI) {
  pzDI->lDivintNo = 0;
  pzDI->sSecNo[0] = pzDI->sWi[0] = '\0';
  pzDI->sDivType[0] = '\0';
  pzDI->lExDate = pzDI->lRecDate = pzDI->lPayDate = 0;
  pzDI->sCurrId[0] = 0;
  pzDI->fDivRate = 0;
  pzDI->sPostStatus[0] = '\0';
  pzDI->lCreateDate = 0;
  pzDI->sCreatedBy[0] = '\0';
  pzDI->lModifyDate = 0;
  pzDI->lFwdDivintNo = 0;
  pzDI->lPrevDivintNo = 0;
} /* Initializedivint */

/**
** Function to initialize divhist structure
**/
void InitializeDivhist(DIVHIST *pzDH) {
  pzDH->iID = 0;
  pzDH->sSecNo[0] = '\0';
  pzDH->sWi[0] = '\0';
  pzDH->sSecXtend[0] = '\0';
  pzDH->sAcctType[0] = '\0';
  pzDH->lTransNo = pzDH->lDivintNo = 0;
  pzDH->sTranType[0] = '\0';
  pzDH->fUnits = 0;
  pzDH->lDivTransNo = 0;
  pzDH->lExDate = 0;
  pzDH->lPayDate = 0;
  pzDH->sTranLocation[0] = '\0';
} /* Initializedivhist */

/**
** Function to initialize accdiv structure
**/
void InitializeAccdiv(ACCDIV *pzAD) {
  pzAD->iID = 0;
  pzAD->sSecNo[0] = pzAD->sWi[0] = pzAD->sSecXtend[0] = '\0';
  pzAD->sAcctType[0] = '\0';
  pzAD->lTransNo = pzAD->lDivintNo = 0;
  pzAD->sSecSymbol[0] = pzAD->sTranType[0] = '\0';
  pzAD->sDivType[0] = 0;
  pzAD->fDivFactor = pzAD->fUnits = pzAD->fOrigFace = 0;
  pzAD->fPcplAmt = pzAD->fIncomeAmt = 0;
  pzAD->lTrdDate = pzAD->lStlDate = pzAD->lEffDate = pzAD->lEntryDate = 0;
  pzAD->sCurrId[0] = pzAD->sCurrAcctType[0] = pzAD->sIncCurrId[0] = '\0';
  pzAD->sSecCurrId[0] = pzAD->sAccrCurrId[0] = '\0';
  pzAD->fBaseXrate = pzAD->fIncBaseXrate = pzAD->fSecBaseXrate = 1;
  pzAD->fAccrBaseXrate = pzAD->fSysXrate = pzAD->fIncSysXrate = 1;
  pzAD->fOrigYld = 0;
  pzAD->lEffMatDate = 0;
  pzAD->fEffMatPrice = 0;
  pzAD->sAcctMthd[0] = pzAD->sTransSrce[0] = pzAD->sDrCr[0] = '\0';
  pzAD->sDtcInclusion[0] = pzAD->sDtcResolve[0] = pzAD->sIncomeFlag[0] = '\0';
  pzAD->sLetterFlag[0] = pzAD->sLedgerFlag[0] = pzAD->sCreatedBy[0] = '\0';
  pzAD->lCreateDate = 0;
  pzAD->sCreateTime[0] = pzAD->sSuspendFlag[0] = pzAD->sDeleteFlag[0] = '\0';
} /* InitializeAccdiv */

/**
** Function to initialize FWTrans structure
**/
void InitializeFWTrans(FWTRANS *pzFWTrans) {
  pzFWTrans->iID = pzFWTrans->iSecID = 0;
  pzFWTrans->sSecNo[0] = pzFWTrans->sWi[0] = pzFWTrans->sSecXtend[0] = '\0';
  pzFWTrans->sAcctType[0] = '\0';
  pzFWTrans->lTransNo = pzFWTrans->lDivintNo = 0;
  pzFWTrans->sTranType[0] = '\0';
  pzFWTrans->sDivType[0] = 0;
  pzFWTrans->fDivFactor = pzFWTrans->fUnits = 0;
  pzFWTrans->fPcplAmt = pzFWTrans->fIncomeAmt = 0;
  pzFWTrans->lTrdDate = pzFWTrans->lStlDate = pzFWTrans->lEffDate = 0;
  pzFWTrans->sCurrId[0] = pzFWTrans->sIncCurrId[0] = '\0';
  pzFWTrans->sSecCurrId[0] = pzFWTrans->sAccrCurrId[0] = '\0';
  pzFWTrans->fBaseXrate = pzFWTrans->fIncBaseXrate = pzFWTrans->fSecBaseXrate =
      1;
  pzFWTrans->fAccrBaseXrate = pzFWTrans->fSysXrate = pzFWTrans->fIncSysXrate =
      1;
  pzFWTrans->sDrCr[0] = '\0';
  pzFWTrans->lCreateDate = 0;
  pzFWTrans->sCreateTime[0] = '\0';
} /* InitializeAccdiv */

/**
** Function to create the unload file for the entire firm's data required for
** generating dividend and interest payments for the given date. This function
** also creates a sorted file out of the unloaded file. It first checks if the
** file divint.unl.dddd (dddd = given date) already exists, if it does, unload
** is skipped. Then it checks if divint.srt.dddd exists, if it does sorting is
** also skipped(obviously, if a new unload file is created, no need to check if
** sort file exists or not, sorting is always done).
**/
ERRSTRUCT DivintUnloadAndSort(long lValDate, long lStartDate,
                              const char *sType) {
  ERRSTRUCT zErr;
  char sFName[90], sUnlStr[500], sTempStr[50];
  DILIBSTRUCT zTempDILS;
  FILE *fp;
  int iLine = 0;
  int iStartPortfolio, iEndPortfolio = 0;
  int iNoMoreRecords = 0;
  int iPortProcessed = 0;

  lpprInitializeErrStruct(&zErr);
  strcpy_s(sFName, MakePricingFileName(lValDate, sType, "D"));

  if (FileExists(sFName))
    DeleteFile(sFName);

  /* Create and open file for writing */

  /* Fetch all the records and write them to the file */
  lValDate = GetPaymentsEndingDate(lValDate);
  while (iNoMoreRecords != -1) {
    fp = fopen(sFName, "a+");
    if (fp == NULL)
      return (lpfnPrintError("Error Opening File", 0, 0, "", 0, zErr.iSqlError,
                             zErr.iIsamCode, "DIVINT UNLOAD1", FALSE));

    lpprGetPortfolioRange(&iStartPortfolio, &iEndPortfolio, 300,
                          &iNoMoreRecords, &zErr);
    iPortProcessed += 300;
    while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
      lpprDivintUnload(&zTempDILS, lStartDate, lValDate, "B", sType,
                       iStartPortfolio, iEndPortfolio, "", "", "", "", 0,
                       &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        fclose(fp);
        return zErr;
      }

      if (strcmp(zTempDILS.sSecNo, "") == 0)
        strcpy_s(zTempDILS.sSecNo, " ");
      if (strcmp(zTempDILS.sWi, "") == 0)
        strcpy_s(zTempDILS.sWi, " ");
      if (strcmp(zTempDILS.sSecXtend, "") == 0)
        strcpy_s(zTempDILS.sSecXtend, " ");
      if (strcmp(zTempDILS.sSecSymbol, "") == 0)
        strcpy_s(zTempDILS.sSecSymbol, " ");
      if (strcmp(zTempDILS.sAcctType, "") == 0)
        strcpy_s(zTempDILS.sAcctType, " ");
      if (strcmp(zTempDILS.sOrigTransType, "") == 0)
        strcpy_s(zTempDILS.sOrigTransType, " ");
      if (strcmp(zTempDILS.sSafekInd, "") == 0)
        strcpy_s(zTempDILS.sSafekInd, " ");
      if (strcmp(zTempDILS.sAutoAction, "") == 0)
        strcpy_s(zTempDILS.sAutoAction, " ");
      if (strcmp(zTempDILS.sAutoDivint, "") == 0)
        strcpy_s(zTempDILS.sAutoDivint, " ");
      if (strcmp(zTempDILS.sCurrId, "") == 0)
        strcpy_s(zTempDILS.sCurrId, " ");
      if (strcmp(zTempDILS.sIncCurrId, "") == 0)
        strcpy_s(zTempDILS.sIncCurrId, " ");
      if (strcmp(zTempDILS.sDivType, "") == 0)
        strcpy_s(zTempDILS.sDivType, " ");
      if (strcmp(zTempDILS.sPostStatus, "") == 0)
        strcpy_s(zTempDILS.sPostStatus, " ");
      if (strcmp(zTempDILS.sDeleteFlag, "") == 0)
        strcpy_s(zTempDILS.sDeleteFlag, " ");
      if (strcmp(zTempDILS.sProcessFlag, "") == 0)
        strcpy_s(zTempDILS.sProcessFlag, " ");
      if (strcmp(zTempDILS.sProcessType, "") == 0)
        strcpy_s(zTempDILS.sProcessType, " ");
      if (strcmp(zTempDILS.sShortSettle, "") == 0)
        strcpy_s(zTempDILS.sShortSettle, " ");
      if (strcmp(zTempDILS.sInclUnits, "") == 0)
        strcpy_s(zTempDILS.sInclUnits, " ");
      if (strcmp(zTempDILS.sSplitInd, "") == 0)
        strcpy_s(zTempDILS.sSplitInd, " ");
      if (strcmp(zTempDILS.sTranType, "") == 0)
        strcpy_s(zTempDILS.sTranType, " ");
      if (strcmp(zTempDILS.sTranLocation, "") == 0)
        strcpy_s(zTempDILS.sTranLocation, " ");

      /* Get all the fields except those coming from divhist */
      sprintf_s(
          sUnlStr,
          "%s|%d|%s|%s|%s|%s|%s|%ld|%f|%ld|%ld|%ld|%f|%f|%ld|%f|%s|%ld|%s|"
          "%d|%f|%d|%s|%s|%s|%s|%f|%f|"
          "%ld|%s|%ld|%ld|%ld|%f|%s|%ld|%ld|%ld|%ld|%s|%s|"
          "%s|%s|%s|%s",
          zTempDILS.sTableName, zTempDILS.iID, zTempDILS.sSecNo, zTempDILS.sWi,
          zTempDILS.sSecXtend, zTempDILS.sSecSymbol, zTempDILS.sAcctType,
          zTempDILS.lTransNo, zTempDILS.fUnits, zTempDILS.lEffDate,
          zTempDILS.lEligDate, zTempDILS.lStlDate, zTempDILS.fOrigFace,
          zTempDILS.fOrigYield, zTempDILS.lEffMatDate, zTempDILS.fEffMatPrice,
          zTempDILS.sOrigTransType, zTempDILS.lCreateDate, zTempDILS.sSafekInd,

          zTempDILS.iSecID, zTempDILS.fTrdUnit, zTempDILS.iSecType,
          zTempDILS.sAutoAction, zTempDILS.sAutoDivint, zTempDILS.sCurrId,
          zTempDILS.sIncCurrId, zTempDILS.fCurExrate, zTempDILS.fCurIncExrate,

          zTempDILS.lDivintNo, zTempDILS.sDivType, zTempDILS.lExDate,
          zTempDILS.lRecDate, zTempDILS.lPayDate, zTempDILS.fDivRate,
          zTempDILS.sPostStatus, zTempDILS.lDivCreateDate,
          zTempDILS.lModifyDate, zTempDILS.lFwdDivintNo,
          zTempDILS.lPrevDivintNo, zTempDILS.sDeleteFlag,
          zTempDILS.sProcessFlag,

          zTempDILS.sProcessType, zTempDILS.sShortSettle, zTempDILS.sInclUnits,
          zTempDILS.sSplitInd);
      /* If divhist record is not null, add divhist fields to unload string */
      if (!zTempDILS.bNullDivhist) {
        sprintf_s(sTempStr, "|%s|%ld|%s", zTempDILS.sTranType,
                  zTempDILS.lDivTransNo, zTempDILS.sTranLocation);
        strcat_s(sUnlStr, sTempStr);
      }
      strcat_s(sUnlStr, "\n");

      /* Write the string to the file) */
      if (fputs(sUnlStr, fp) == EOF) {
        fclose(fp);
        return (lpfnPrintError("Error Writing To The File", 0, 0, "", 999, 0, 0,
                               "DIVINT UNLOAD2", FALSE));
      }
    } /* while no error */

    fclose(fp);
  } // No more records

  /*
  ** Sort the file in the following order :
  ** br_acct(1), sec_no(2), wi(3), sec_xtend(4), acct_type(6), trans_no(7),
  ** table_name(0) and create_date(16)
  */
  /*sprintf_s(sUnlStr, "sort -o%s -t'|' +1 -2 +2 -3 +3 -4 +4 -5 +6 -7 +7 -8 +0
  -1 "
                   "+16 -17 %s", sFName2, sFName);
  if (system(sUnlStr) != 0)
    zErr = lpfnPrintError("Error Sorting File", 0, 0, "", 999, 0, 0,
                                                                                                        "DIVINT UNLOAD3", FALSE);*/

  return zErr;
} /* DivintUnloadAndSort */

/**
** This function checks if the given file exists. It uses opn to open the file,
** if open is successful, file exists, else not.
**/
BOOL FileExists(const char *sFileName) {
  int fd;

  fd = _open(sFileName, O_RDONLY, 0);
  if (fd == -1)
    return FALSE;

  _close(fd);

  return TRUE;
} /* FileExists */

/**
** Function that makes the filename for the given date and file type. Valid
** File Type are 'U'(nload) and 'S'(orted). Valid LibraryName are "D"(dividend)
** "M"(maturity) and "A"(amort).
**/
char *MakePricingFileName(long lDate, const char *sType, const char *sLibName) {
  static char sFileName[50];

  /* Add the file type to the file name */
  if (sLibName[0] == 'D') {
    if (sType[0] == 'I')
      sprintf_s(sFileName, "divint.unl.%ld", lDate);
    else if (sType[0] == 'S')
      sprintf_s(sFileName, "divspl.unl.%ld", lDate);
    else if (sType[0] == 'T')
      sprintf_s(sFileName, "tips.unl.%ld", lDate);
    else
      sprintf_s(sFileName, "dividend.unl.%ld", lDate);
  } else if (sLibName[0] == 'M')
    sprintf_s(sFileName, "maturity.unl.%ld", lDate);
  else if (sLibName[0] == 'A')
    sprintf_s(sFileName, "amort.unl.%ld", lDate);
  else if (sLibName[0] == 'F')
    sprintf_s(sFileName, "ForwardMaturity.unl.%ld", lDate);
  else
    return (char *)"";

  return sFileName;
} /* MakePricingFileName */

/*
** This function is used to get the ending date upto which
* dividends/interest/maturity should
** get generated/paid. Typically it is the valuation date but in case the
* valuation date is a
** Friday(or the next day is a holiday), this date should be Sunday's date(or
* the holday date).
*/
long GetPaymentsEndingDate(long lValDate) {
  long lEndDate;

  // Get the next business day, reduce it by 1, the new date should be the
  // ending date
  if (lpfnNextBusinessDay(lValDate, (char *)"USA", (char *)"M", &lEndDate) ==
      0) {
    lEndDate--;

    // This should not happen, but if somehow ending date comes out to be
    // earlier than valdate, use valdate  instead
    if (lEndDate < lValDate)
      lEndDate = lValDate;
  }

  return lEndDate;
} // GetPaymentsEndingDate

void FreePayments() {
  if (bInit) {
    FreeLibrary(hDelphiCDll);
    FreeLibrary(hStarsIODll);
    FreeLibrary(hStarsUtilsDll);
    FreeLibrary(hTEngineDll);
    bInit = FALSE;
  } // bInit
} // FreePayments
