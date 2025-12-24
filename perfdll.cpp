/**
 *
 * SUB-SYSTEM: calcperf
 *
 * FILENAME: perfdll.c
 *
 * DESCRIPTION:
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR: Shobhit Barman (Effron Enterprises, Inc.)
 *
 *
 **/
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2015-11-12 VI# 57941 - Added new functionality to performance engine to merge
// performance from one security to another. 2013-01-14 VI# 51154 Added cleanup
// for orphaned unitvalue entries to be marked as deleted -mk 2010-07-14 VI#
// 44510 Improved daily deletions of unitvalues -mk

#include "perfdll.h"
#include <atlbase.h>
#include <atlcom.h>

static BOOL bInit = FALSE;
HINSTANCE OleDBIODll, StarsUtilsDll, CalcGainLossDll, TransEngineDll;
HINSTANCE InterfaceDll, CalcFlowDll, RollDll, ValuationDll,
    CapicomDll; //, TimerDll;

HRESULT CoCreateCapiCOM(CAPICOM::IHashedDataPtr &obj) {
  HMODULE CapicomDll = LoadLibrarySafe("CapiCom.dll");
  if (!CapicomDll)
    return HRESULT_FROM_WIN32(GetLastError()); // library not found

  BOOL(WINAPI * DllGetClassObject)(REFCLSID, REFIID, LPVOID) = (BOOL(WINAPI *)(
      REFCLSID, REFIID, LPVOID))GetProcAddress(CapicomDll, "DllGetClassObject");

  if (!DllGetClassObject)
    return HRESULT_FROM_WIN32(GetLastError());

  CComPtr<IClassFactory> pClassFactory;
  HRESULT hr = DllGetClassObject(__uuidof(CAPICOM::HashedData),
                                 IID_IClassFactory, &pClassFactory);
  if (FAILED(hr))
    return hr;

  hr = pClassFactory->CreateInstance(NULL, __uuidof(CAPICOM::IHashedData),
                                     (void **)&obj);
  if (FAILED(hr))
    return hr;

  return S_OK;
}

/**
** Function which initializes this Dll. It loads other dlls required by this
** Dll and loads all the functions/procedures used from those dlls.
**/
DLLAPI ERRSTRUCT STDCALL WINAPI InitCalcPerf(long lAsofDate, char *sDBAlias,
                                             char *sMode, char *sType,
                                             BOOL bPrepareQueries,
                                             char *sErrFile) {
  ERRSTRUCT zErr;
  int iError;

  if (!bInit) {
    TransEngineDll = LoadLibrarySafe("TransEngine.dll");
    if (TransEngineDll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    // Load functions from transengine dll
    lpprInitializeErrStruct =
        (LPPRERRSTRUCT)GetProcAddress(TransEngineDll, "InitializeErrStruct");
    if (!lpprInitializeErrStruct) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    lpprInitializeErrStruct(&zErr);

    lpfnSetErrorFileName =
        (LPFN1PCHAR)GetProcAddress(TransEngineDll, "SetErrorFileName");
    if (!lpfnSetErrorFileName) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    lpfnSetErrorFileName(sErrFile);

    lpfnPrintError =
        (LPFNPRINTERROR)GetProcAddress(TransEngineDll, "PrintError");
    if (!lpfnPrintError) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    OleDBIODll = LoadLibrarySafe("oledbio.dll");
    if (OleDBIODll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load OleDBIO Dll", 0, 0, "", iError, 0,
                             0, "CALCPERF INIT1", FALSE));
    }

    StarsUtilsDll = LoadLibrarySafe("StarsUtils.dll");
    if (StarsUtilsDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load StarsUtils Dll", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT2", FALSE));
    }

    // Try to load the "CalcGainLoss.dll" dll created in C
    CalcGainLossDll = LoadLibrarySafe("CalcGainLoss.dll");
    if (CalcGainLossDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CalcGainLoss Dll", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT3", FALSE));
    }

    InterfaceDll = LoadLibrarySafe("DelphiCInterface.dll");
    if (InterfaceDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DelphiCInterface Dll", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT4", FALSE));
    }

    CalcFlowDll = LoadLibrarySafe("CalcFlow.dll");
    if (CalcFlowDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CalcFlow Dll", 0, 0, "", iError, 0,
                             0, "CALCPERF INIT5", FALSE));
    }

    RollDll = LoadLibrarySafe("Roll.dll");
    if (RollDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load Roll Dll", 0, 0, "", iError, 0, 0,
                             "CALCPERF INIT6", FALSE));
    }

    ValuationDll = LoadLibrarySafe("Valuation.dll");
    if (ValuationDll == NULL) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load Valuation Dll", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT7", FALSE));
    }

    // Load the functions from OledbIO dll
    lpprOLEDBIOInit =
        (LPPR3PCHAR1LONG1BOOL)GetProcAddress(OleDBIODll, "InitializeOLEDBIO");
    if (!lpprOLEDBIOInit) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitializeOLEDBIO function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT8", FALSE));
    }

    lpprFreeOLEDBIO = (LPPRVOID)GetProcAddress(OleDBIODll, "FreeOLEDBIO");
    if (!lpprFreeOLEDBIO) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load FreeOLEDBIO function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT8A", FALSE));
    }

    lpfnCommitTransaction =
        (LPFNVOID)GetProcAddress(OleDBIODll, "CommitDBTransaction");
    if (!lpfnCommitTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CommitTransaction function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT9", FALSE));
    }

    lpfnRollbackTransaction =
        (LPFNVOID)GetProcAddress(OleDBIODll, "RollbackDBTransaction");
    if (!lpfnRollbackTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load RollbackTransaction function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT10", FALSE));
    }

    lpfnStartTransaction =
        (LPFNVOID)GetProcAddress(OleDBIODll, "StartDBTransaction");
    if (!lpfnStartTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load StartTransaction function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT11", FALSE));
    }

    lpfnGetTransCount = (LPFNVOID)GetProcAddress(OleDBIODll, "GetTransCount");
    if (!lpfnGetTransCount) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load GetTransCount function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT11a", FALSE));
    }

    lpfnAbortTransaction =
        (LPFN1BOOL)GetProcAddress(OleDBIODll, "AbortDBTransaction");
    if (!lpfnAbortTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load AbortDBTransaction function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT11b", FALSE));
    }

    lpprSelectAllAccdivForAnAccount = (LPPRALLACCDIVFORANACCOUNT)GetProcAddress(
        OleDBIODll, "SelectAllAccdivForAnAccount");
    if (!lpprSelectAllAccdivForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllAccdivForAnAccount Procedure", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT12", FALSE));
    }

    lpprSelectPartAsset =
        (LPPRPARTASSET)GetProcAddress(OleDBIODll, "SelectPartAsset");
    if (!lpprSelectPartAsset) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPartAsset function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT13", FALSE));
    }

    lpprDeleteBankstat =
        (LPPR3LONG)GetProcAddress(OleDBIODll, "DeleteBankstat");
    if (!lpprDeleteBankstat) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteBankstat function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT14", FALSE));
    }

    lpprSelectAllPartCurrencies = (LPPRSELECTALLPARTCURRENCIES)GetProcAddress(
        OleDBIODll, "SelectAllPartCurrencies");
    if (!lpprSelectAllPartCurrencies) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllPartCurrencies", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT15", FALSE));
    }

    lpprSelectAllCountries = (LPPRSELECTALLCOUNTRIES)GetProcAddress(
        OleDBIODll, "SelectAllCountries");
    if (!lpprSelectAllCountries) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllCountries", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT16", FALSE));
    }

    lpprDeleteDailyFlows =
        (LPPR1INT1LONG)GetProcAddress(OleDBIODll, "DeleteDailyFlows");
    if (!lpprDeleteDailyFlows) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteDailyFlows function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT17", FALSE));
    }

    lpprDeleteDailyFlowsByID =
        (LPPR1INT1LONG)GetProcAddress(OleDBIODll, "DeleteDailyFlowsByID");
    if (!lpprDeleteDailyFlowsByID) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteDailyFlowsByID function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT17a", FALSE));
    }

    lpprInsertDailyFlows =
        (LPPRDAILYFLOWS)GetProcAddress(OleDBIODll, "InsertDailyFlows");
    if (!lpprInsertDailyFlows) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertDailyFlows function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT18", FALSE));
    }

    lpprDeleteDSumdata =
        (LPPR3LONG)GetProcAddress(OleDBIODll, "DeleteDSumdata");
    if (!lpprDeleteDSumdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteDSumdata function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT20", FALSE));
    }

    lpprInsertDailySummdata =
        (LPPRSUMMDATA)GetProcAddress(OleDBIODll, "InsertDailySummdata");
    if (!lpprInsertDailySummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertDailySummdata", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT21", FALSE));
    }

    lpprSelectDailySummdata =
        (LPPRPSUMMDATA2LONG)GetProcAddress(OleDBIODll, "SelectDailySummdata");
    if (!lpprSelectDailySummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectDailySummdata", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT22", FALSE));
    }

    lpprUpdateDailySummdata =
        (LPPRSUMMDATA1LONG)GetProcAddress(OleDBIODll, "UpdateDailySummdata");
    if (!lpprUpdateDailySummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateDailySummdata", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT23", FALSE));
    }

    lpprReadAllHoldmap =
        (LPPR8PCHAR1PLONG)GetProcAddress(OleDBIODll, "ReadAllHoldmap");
    if (!lpprReadAllHoldmap) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load ReadAllHoldmap function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT25", FALSE));
    }

    lpprSelectPerformanceHoldings =
        (LPPRSELECTPERFORMANCEHOLDINGS)GetProcAddress(
            OleDBIODll, "SelectPerformanceHoldings");
    if (!lpprSelectPerformanceHoldings) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load SelectPerformanceHoldings function", 0,
                         0, "", iError, 0, 0, "CALCPERF INIT26", FALSE));
    }

    lpprSelectPerformanceHoldcash =
        (LPPRSELECTPERFORMANCEHOLDINGS)GetProcAddress(
            OleDBIODll, "SelectPerformanceHoldcash");
    if (!lpprSelectPerformanceHoldcash) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load lpprSelectPerformanceHoldcash function", 0, 0, "",
          iError, 0, 0, "CALCPERF INIT27", FALSE));
    }

    lpprDeleteMonthSum =
        (LPPR3LONG)GetProcAddress(OleDBIODll, "DeleteMonthSum");
    if (!lpprDeleteMonthSum) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteMonthSum function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT28", FALSE));
    }

    lpprInsertMonthlySummdata =
        (LPPRSUMMDATA)GetProcAddress(OleDBIODll, "InsertMonthlySummdata");
    if (!lpprInsertMonthlySummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertMonthlySummdata function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT29", FALSE));
    }

    lpprSelectPerfctrl =
        (LPPRPPERFCTRL1INT)GetProcAddress(OleDBIODll, "SelectPerfctrl");
    if (!lpprSelectPerfctrl) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPerfctrl function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT30", FALSE));
    }

    lpprUpdatePerfctrl =
        (LPPRPERFCTRL)GetProcAddress(OleDBIODll, "UpdatePerfctrl");
    if (!lpprUpdatePerfctrl) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdatePerfctrl function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT31", FALSE));
    }

    lpprDeletePerfkey = (LPPR1LONG)GetProcAddress(OleDBIODll, "DeletePerfkey");
    if (!lpprDeletePerfkey) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeletePerfkey function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT32", FALSE));
    }

    lpprInsertPerfkey =
        (LPPRPERFKEY)GetProcAddress(OleDBIODll, "InsertPerfkey");
    if (!lpprInsertPerfkey) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertPerfkey function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT33", FALSE));
    }

    lpprSelectPerfkeys =
        (LPPRPPERFKEY1INT)GetProcAddress(OleDBIODll, "SelectPerfkeys");
    if (!lpprSelectPerfkeys) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPerfkeys function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT34", FALSE));
    }

    lpprUpdateNewPerfkey =
        (LPPRPERFKEY)GetProcAddress(OleDBIODll, "UpdateNewPerfkey");
    if (!lpprUpdateNewPerfkey) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateNewPerfkey function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT35", FALSE));
    }

    lpprUpdateOldPerfkey =
        (LPPRPERFKEY)GetProcAddress(OleDBIODll, "UpdateOldPerfkey");
    if (!lpprUpdateOldPerfkey) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateOldPerfkey function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT36", FALSE));
    }

    lpprSelectAllPerfrule =
        (LPPRSELECTALLPERFRULE)GetProcAddress(OleDBIODll, "SelectAllPerfrule");
    if (!lpprSelectAllPerfrule) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllPerfrule function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT37", FALSE));
    }

    lpprSelectOnePartPortmain = (LPPRSELECTONEPARTPORTMAIN)GetProcAddress(
        OleDBIODll, "SelectOnePartPortmain");
    if (!lpprSelectOnePartPortmain) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectOnePartPortmain function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT38", FALSE));
    }

    lpprInsertPerfscriptDetail = (LPPRPERFSCRIPTDETAIL)GetProcAddress(
        OleDBIODll, "InsertPerfscriptDetail");
    if (!lpprInsertPerfscriptDetail) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertPerfscriptDetail function",
                             0, 0, "", iError, 0, 0, "CALCPERF INIT39", FALSE));
    }

    lpprInsertPerfscriptHeader = (LPPRPERFSCRIPTHEADER)GetProcAddress(
        OleDBIODll, "InsertPerfscriptHeader");
    if (!lpprInsertPerfscriptHeader) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertPerfscriptHeader function",
                             0, 0, "", iError, 0, 0, "CALCPERF INIT40", FALSE));
    }

    lpprUpdatePerfscriptHeader = (LPPRPERFSCRIPTHEADER)GetProcAddress(
        OleDBIODll, "UpdatePerfscriptHeader");
    if (!lpprUpdatePerfscriptHeader) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdatePerfscriptHeader function",
                             0, 0, "", iError, 0, 0, "CALCPERF INIT40a",
                             FALSE));
    }

    lpprSelectAllScriptHeaderAndDetails =
        (LPPRSELECTALLSCRIPTHEADERANDDETAILS)GetProcAddress(
            OleDBIODll, "SelectAllScriptHeaderAndDetails");
    if (!lpprSelectAllScriptHeaderAndDetails) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllScriptHeaderAndDetails function", 0, 0, "",
          iError, 0, 0, "CALCPERF INIT41", FALSE));
    }

    lpprSelectAllPorttax =
        (LPPRPORTTAX)GetProcAddress(OleDBIODll, "SelectAllPorttax");
    if (!lpprSelectAllPorttax) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllPorttax function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT42", FALSE));
    }

    lpprSelectOneSegment =
        (LPPR1PCHAR2PINT1INT)GetProcAddress(OleDBIODll, "SelectOneSegment");
    if (!lpprSelectOneSegment) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectOneSegment function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT46", FALSE));
    }

    lpprSelectAllPartSectype = (LPPRSELECTALLPARTSECTYPE)GetProcAddress(
        OleDBIODll, "SelectAllPartSectype");
    if (!lpprSelectAllPartSectype) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllPartSecType function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT47", FALSE));
    }

    lpprSelectStarsDate =
        (LPPR2PLONG)GetProcAddress(OleDBIODll, "SelectStarsDate");
    if (!lpprSelectStarsDate) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectStarsDate function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT48", FALSE));
    }

    lpprSelectCFStartDate =
        (LPPR2PLONG)GetProcAddress(OleDBIODll, "SelectCFStartDate");
    if (!lpprSelectCFStartDate) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectCFStartDate function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT48A", FALSE));
    }

    lpprDeleteSummdata =
        (LPPR3LONG)GetProcAddress(OleDBIODll, "DeleteSummdata");
    if (!lpprDeleteSummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteSummdata function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT49", FALSE));
    }

    lpprInsertPeriodSummdata =
        (LPPRSUMMDATA)GetProcAddress(OleDBIODll, "InsertPeriodSummdata");
    if (!lpprInsertPeriodSummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertPeriodSummdata function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT50", FALSE));
    }

    lpprSelectPeriodSummdata =
        (LPPRPSUMMDATA3LONG)GetProcAddress(OleDBIODll, "SelectPeriodSummdata");
    if (!lpprSelectPeriodSummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPeriodSummdata", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT51", FALSE));
    }

    lpprUpdatePeriodSummdata =
        (LPPRSUMMDATA)GetProcAddress(OleDBIODll, "UpdatePeriodSummdata");
    if (!lpprUpdatePeriodSummdata) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdatePeriodSummdata", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT52", FALSE));
    }

    lpprSelectSyssetng =
        (LPPRSELECTSYSSETTINGS)GetProcAddress(OleDBIODll, "SelectSysSettings");
    if (!lpprSelectSyssetng) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSysSettings", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT52A", FALSE));
    }

    lpprSelectSysvalues =
        (LPPRSELECTSYSVALUES)GetProcAddress(OleDBIODll, "SelectSysvalues");
    if (!lpprSelectSysvalues) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSysvalues", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT52B", FALSE));
    }

    lpprDeleteTaxperf = (LPPR3LONG)GetProcAddress(OleDBIODll, "DeleteTaxperf");
    if (!lpprDeleteTaxperf) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteTaxperf", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT53A", FALSE));
    }

    lpprDeleteTaxperfForSegment =
        (LPPR3LONG)GetProcAddress(OleDBIODll, "DeleteTaxperfForSegment");
    if (!lpprDeleteTaxperfForSegment) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteTaxperfForSegment", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT53B", FALSE));
    }

    lpprInsertTaxperf =
        (LPPRTAXPERF)GetProcAddress(OleDBIODll, "InsertTaxperf");
    if (!lpprInsertTaxperf) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertTaxperf", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT54", FALSE));
    }

    lpprSelectTaxperf =
        (LPPRSELECTTAXPERF)GetProcAddress(OleDBIODll, "SelectTaxperf");
    if (!lpprSelectTaxperf) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectTaxperf", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT55", FALSE));
    }

    lpprSelectAllTemplateHeaderAndDetails =
        (LPPRSELECTALLTEMPLATEDETAILS)GetProcAddress(
            OleDBIODll, "SelectAllTemplateHeaderAndDetails");
    if (!lpprSelectAllTemplateHeaderAndDetails) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllTemplateHeaderAndDetails function", 0, 0, "",
          iError, 0, 0, "CALCPERF INIT56", FALSE));
    }

    lpprInitTrans =
        (LPPRINITTRANS)GetProcAddress(TransEngineDll, "InitializeTransStruct");
    if (!lpprInitTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitializeTransStruct Procedure",
                             0, 0, "", iError, 0, 0, "CALCPERF INIT57", FALSE));
    }

    lpprSelectPerformanceTransaction =
        (LPPRSELECTPERFORMANCETRANSACTION)GetProcAddress(
            OleDBIODll, "SelectPerformanceTransaction");
    if (!lpprSelectPerformanceTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteHoldcash function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT58", FALSE));
    }

    lpprSelectAllPartTrantype = (LPPRSELECTALLPARTTRANTYPE)GetProcAddress(
        OleDBIODll, "SelectAllPartTrantype");
    if (!lpprSelectAllPartTrantype) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllPartTrantype function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT59", FALSE));
    }

    lpprSelectSegmain =
        (LPPRSELECTSEGMAIN)GetProcAddress(OleDBIODll, "SelectSegmain");
    if (!lpprSelectSegmain) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSegmain function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60a", FALSE));
    }

    lpprInsertSegmain =
        (LPPRINSERTSEGMAIN)GetProcAddress(OleDBIODll, "InsertSegmain");
    if (!lpprInsertSegmain) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertSegmain function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60b", FALSE));
    }

    lpprSelectSegtree =
        (LPPRSELECTSEGTREE)GetProcAddress(OleDBIODll, "SelectSegtree");
    if (!lpprSelectSegtree) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSegtree function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60c", FALSE));
    }

    lpprInsertSegtree =
        (LPPRINSERTSEGTREE)GetProcAddress(OleDBIODll, "InsertSegtree");
    if (!lpprInsertSegtree) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertSegtree function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60d", FALSE));
    }

    lpfnSelectSegmentIDFromSegmap = (LPFNSELECTSEGMAP)GetProcAddress(
        OleDBIODll, "SelectSegmentIdFromSegMap");
    if (!lpfnSelectSegmentIDFromSegmap) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load SelectSegmentIDFromSegmap function", 0,
                         0, "", iError, 0, 0, "CALCPERF INIT60e", FALSE));
    }

    lpprSelectSegment =
        (LPPRSELECTSEGMENT)GetProcAddress(OleDBIODll, "SelectSegment");
    if (!lpprSelectSegment) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSegment function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60f", FALSE));
    }

    lpprInsertSegment =
        (LPPRINSERTSEGMENT)GetProcAddress(OleDBIODll, "InsertSegment");
    if (!lpprInsertSegment) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertSegment function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60g", FALSE));
    }

    lpprInsertSegmap =
        (LPPRINSERTSEGMAP)GetProcAddress(OleDBIODll, "InsertSegmap");
    if (!lpprInsertSegmap) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertSegmap function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60h", FALSE));
    }

    lpfnSelectSecSegmap =
        (LPFN2PCHAR)GetProcAddress(OleDBIODll, "SelectSecSegmap");
    if (!lpfnSelectSecSegmap) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSecSegmap function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT60i", FALSE));
    }

    lpprInsertSecSegmap =
        (LPPRINSERTSECSEGMAP)GetProcAddress(OleDBIODll, "InsertSecSegmap");
    if (!lpprInsertSecSegmap) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertSecSegmap function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT60j", FALSE));
    }

    lpprSelectUnitValue =
        (LPPRSELECTUNITVALUE)GetProcAddress(OleDBIODll, "SelectUnitValue");
    if (!lpprSelectUnitValue) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectUnitValue function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT60l", FALSE));
    }

    lpprSelectUnitValueRange2 = (LPPRSELECTUNITVALUERANGE2)GetProcAddress(
        OleDBIODll, "SelectUnitValueRange2");
    if (!lpprSelectUnitValueRange2) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectUnitValueRange2 function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT60l", FALSE));
    }

    lpprInsertUnitValue =
        (LPPRINSERTUNITVALUE)GetProcAddress(OleDBIODll, "InsertUnitValue");
    if (!lpprInsertUnitValue) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertUnitValue function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT60m", FALSE));
    }

    lpprInsertUnitValueBatch = (LPPRINSERTUNITVALUEBATCH)GetProcAddress(
        OleDBIODll, "InsertUnitValueBatch");
    if (!lpprInsertUnitValueBatch) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InsertUnitValueBatch function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT60m1", FALSE));
    }

    lpprDeleteDailyUnitValueForADate =
        (LPPR4LONG)GetProcAddress(OleDBIODll, "DeleteDailyUnitValueForADate");
    if (!lpprDeleteDailyUnitValueForADate) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load DeleteDailyUnitValueForADate function",
                         0, 0, "", iError, 0, 0, "CALCPERF INIT60n", FALSE));
    }

    lpprDeleteUnitValueSince2 =
        (LPPR4LONG)GetProcAddress(OleDBIODll, "DeleteUnitValueSince2");
    if (!lpprDeleteUnitValueSince2) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteUnitValueSince2 function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT60n5", FALSE));
    }

    lpprMarkPeriodUVForADateRangeAsDeleted = (LPPR3LONG)GetProcAddress(
        OleDBIODll, "MarkPeriodUVForADateRangeAsDeleted");
    if (!lpprMarkPeriodUVForADateRangeAsDeleted) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load MarkPeriodUVForADateRangeAsDeleted function", 0, 0,
          "", iError, 0, 0, "CALCPERF INIT60o1", FALSE));
    }

    lpprDeleteMarkedUnitValue =
        (LPPR2LONG)GetProcAddress(OleDBIODll, "DeleteMarkedUnitValue");
    if (!lpprDeleteMarkedUnitValue) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load DeleteMarkedUnitValue function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT60o2", FALSE));
    }

    lpprRecalcDailyUV = (LPPR3LONG)GetProcAddress(OleDBIODll, "RecalcDailyUV");
    if (!lpprRecalcDailyUV) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load RecalcDailyUV function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT60o3", FALSE));
    }

    lpprUpdateUnitValue =
        (LPPRUPDATEUNITVALUE)GetProcAddress(OleDBIODll, "UpdateUnitValue");
    if (!lpprUpdateUnitValue) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateUnitValue function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT60p", FALSE));
    }

    lpprSelectActivePerfReturnType =
        (LPPRSELECTACTIVEPERFRETURNTYPE)GetProcAddress(
            OleDBIODll, "SelectActivePerfReturnType");
    if (!lpprSelectActivePerfReturnType) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load SelectActivePerfReturnType function",
                         0, 0, "", iError, 0, 0, "CALCPERF INIT60q", FALSE));
    }

    lpprSelectPerfAssetMerge =
        (LPPRPERFASSETMERGE)GetProcAddress(OleDBIODll, "SelectPerfAssetMerge");
    if (!lpprSelectPerfAssetMerge) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPerfAssetMerge function", 0,
                             0, "", iError, 0, 0, "CALCPERF INIT60r", FALSE));
    }

    // functions from starsutils dll
    lpfnrmdyjul = (LPFNRMDYJUL)GetProcAddress(StarsUtilsDll, "rmdyjul");
    if (!lpfnrmdyjul) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load rmdyjul function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT61", FALSE));
    }

    lpfnrjulmdy = (LPFNRJULMDY)GetProcAddress(StarsUtilsDll, "rjulmdy");
    if (!lpfnrjulmdy) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load rjulmdy function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT62", FALSE));
    }

    lpfnIsItAMarketHoliday = (LPFNISITAMARKETHOLIDAY)GetProcAddress(
        StarsUtilsDll, "IsItAMarketHoliday");
    if (!lpfnIsItAMarketHoliday) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load IsItAMarketHoliday function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT62a", FALSE));
    }

    lpfnCurrentMonthEnd =
        (LPFN1INT)GetProcAddress(StarsUtilsDll, "CurrentMonthEnd");
    if (!lpfnCurrentMonthEnd) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CurrentMonthEnd function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT63", FALSE));
    }

    lpfnLastMonthEnd = (LPFN1INT)GetProcAddress(StarsUtilsDll, "LastMonthEnd");
    if (!lpfnLastMonthEnd) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load LastMonthEnd function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT64", FALSE));
    }

    lpfnIsItAMonthEnd =
        (LPFN1INT)GetProcAddress(StarsUtilsDll, "IsItAMonthEnd");
    if (!lpfnIsItAMonthEnd) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load IsItAMonthEnd function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT65", FALSE));
    }

    lpfnCurrentDateAndTime =
        (LPFN2VOID)GetProcAddress(StarsUtilsDll, "CurrentDateAndTime");
    if (!lpfnCurrentDateAndTime) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CurrentDateAndTime function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT66", FALSE));
    }

    // Load functions from Calcgainloss Dll
    lpfnCalcGainLoss =
        (LPFNCALCGAINLOSS)GetProcAddress(CalcGainLossDll, "CalcGainLoss");
    if (!lpfnCalcGainLoss) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CalcGainLoss function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT67", FALSE));
    }

    // functions from calcflow dll
    lpfnCalcNetFlow =
        (LPFNCALCFLOW)GetProcAddress(CalcFlowDll, "CalculateNetFlowPerf");
    if (!lpfnCalcNetFlow) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CalcNetFlow function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT71", FALSE));
    }

    lpfnInitCalcFlow = (LPFN1PCHAR)GetProcAddress(CalcFlowDll, "InitCalcFlow");
    if (!lpfnInitCalcFlow) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitCalcFlow function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT72", FALSE));
    }
    zErr.iBusinessError = lpfnInitCalcFlow(sErrFile);
    if (zErr.iBusinessError != 0)
      return zErr;

    // functions from roll dll
    lpfnInitRoll = (LPFN3PCHAR1LONG1PCHAR)GetProcAddress(RollDll, "InitRoll");
    if (!lpfnInitRoll) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitRoll function", 0, 0, "",
                             iError, 0, 0, "CALCPERF INIT73", FALSE));
    }

    lpfnRoll = (LPFNROLL)GetProcAddress(RollDll, "Roll");
    if (!lpfnRoll) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load Roll function", 0, 0, "", iError,
                             0, 0, "CALCPERF INIT74", FALSE));
    }

    lpfnInitValuation = (LPFN1LONG2PCHAR)GetProcAddress(
        ValuationDll, "InitializeAccountValuation");
    if (!lpfnInitValuation) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load InitializeAccountValuation function",
                         0, 0, "", iError, 0, 0, "CALCPERF INIT75", FALSE));
    }

    lpfnValuation = (LPFN1PCHAR1INT1LONG1BOOL)GetProcAddress(
        ValuationDll, "AccountValuation");
    if (!lpfnValuation) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load AccountValuation function", 0, 0,
                             "", iError, 0, 0, "CALCPERF INIT76", FALSE));
    }

    /*		lpfnFindDividendInAccdiv =
       (LPFNACCDIVTABLE)GetProcAddress(ValuationDll, "FindDividendInAccdiv"); if
       (!lpfnFindDividendInAccdiv)
                    {
                            iError = GetLastError();
                            return(lpfnPrintError("Unable To Load
       FindDividendInAccdiv function", 0, 0, "", iError, 0, 0, "CALCPERF
       INIT77", FALSE));
                    }*/

    // create COM object from CAPICOM.DLL
    HRESULT hr = CoCreateCapiCOM(pHashData);
    //			pHashData.CreateInstance(__uuidof(CAPICOM::HashedData));
    if FAILED (hr)
      return (lpfnPrintError("Unable To Create CAPICOM Hashed Data ", 0, 0, "",
                             hr, 0, 0, "CALCPERF INIT78", FALSE));

    pHashData->Algorithm = CAPICOM::CAPICOM_HASH_ALGORITHM_SHA1;

    /*
    TimerDll = LoadLibrarySafe("Timer.dll");
    if (TimerDll == NULL)
    {
            iError = GetLastError();
            return(lpfnPrintError("Unable To Load Timer Dll", 0, 0, "", iError,
                                                                                                    0, 0, "CALCPERF INIT92", FALSE));
            return zErr;
    }

    lpfnTimer =	(LPFN1INT)GetProcAddress(TimerDll, "Timer");
    if (!lpfnTimer)
    {
            iError = GetLastError();
            return(lpfnPrintError("Unable To Load Timer function", 0, 0, "",
    iError, 0, 0, "CALCPERF INIT6", FALSE));
    }

    lpprTimerResult =	(LP2PR1PCHAR)GetProcAddress(TimerDll, "TimerResult");
    if (!lpprTimerResult)
    {
            iError = GetLastError();
            return(lpfnPrintError("Unable To Load TimerResult Procedure", 0, 0,
    "", iError, 0, 0, "CALCPERF INIT6", FALSE));
    }
    */

    bInit = TRUE;
  } // if init is being called the first time

  // initialize starsio
  zErr =
      InitializeAppropriateLibrary(sDBAlias, sErrFile, 'S', lAsofDate, FALSE);

  return zErr;
} // InitTranproc

/**
** Function to select an asset record for the given security
**/
/*
ERRSTRUCT SelectAsset(ASSETS *pzAssets, char *sSecNo, char *sWhenIssue)
{
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

//	lpprTimer(19);
  lpprSelectAsset(pzAssets, sSecNo, sWhenIssue, &zErr);
//	lpprTimer(20);
  if (zErr.iSqlError)
    zErr = lpfnPrintError("Error Reading ASSETS", 0, 0, "", 0, zErr.iSqlError,
                      zErr.iIsamCode, "TPROC SELECT ASSETS", FALSE);

  return zErr;
} /* SelectAsset */

ERRSTRUCT InitializeAppropriateLibrary(char *sDBAlias, char *sErrFile,
                                       char sWhichLibrary, long lAsofDate,
                                       BOOL bOnlyResetErrorFile) {
  ERRSTRUCT zErr;
  static char sDatabase[80] = "";
  static char sErrorFile[80] = "";

  lpprInitializeErrStruct(&zErr);

  if (strcmp(sDBAlias, "") != 0)
    strcpy_s(sDatabase, sDBAlias);

  if (strcmp(sErrFile, "") != 0)
    strcpy_s(sErrorFile, sErrFile);

  if (bOnlyResetErrorFile) {
    lpfnSetErrorFileName(sErrorFile);
    return zErr;
  }

  switch (sWhichLibrary) {
  case 'R': // Roll
    zErr = lpfnInitRoll(sDatabase, "", "", lAsofDate, sErrorFile);
    break;

  case 'V': // Valuation
    zErr = lpfnInitValuation(lAsofDate, sDatabase, sErrorFile);
    break;

  case 'S': // StarsIO - now OLEDBIO
    lpprOLEDBIOInit(sDatabase, (char *)"", (char *)"", lAsofDate, PREPQRY_NONE,
                    &zErr);
    lpfnSetErrorFileName(sErrorFile);
    break;
  }

  return zErr;
} // InitializeAppropriateLibrary

DLLAPI void STDCALL FreePerformanceLibrary() {
  if (bInit) {
    pHashData = nullptr;
    FreeLibrary(CapicomDll);
    lpprFreeOLEDBIO();
    FreeLibrary(StarsUtilsDll);
    FreeLibrary(CalcGainLossDll);
    FreeLibrary(TransEngineDll);
    FreeLibrary(InterfaceDll);
    FreeLibrary(CalcFlowDll);
    FreeLibrary(RollDll);
    FreeLibrary(ValuationDll);
    FreeLibrary(OleDBIODll);
    bInit = FALSE;
  }
}
