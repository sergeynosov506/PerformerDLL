#include "valuation.h"

HINSTANCE hOledbIODll, hTransEngineDll, hGainLossDll, hDelphiCInterfaceDll,
    hStarsUtilsDll; //, TimerDll;
static BOOL bInit = FALSE;

extern "C" {
DLLAPI ERRSTRUCT STDCALL WINAPI InitializeAccountValuation(long lAsofDate,
                                                           char *sDBAlias,
                                                           char *sErrFile) {
  ERRSTRUCT zErr;
  int iError;
  long iTemp;

  zErr.iBusinessError = zErr.iSqlError = 0;

  // Load the Dll and all the functions only the first time
  if (!bInit) {
    // Try to load the "oledbio.dll" dll created in C
    hOledbIODll = LoadLibrarySafe("OLEDBIO.dll");
    if (hOledbIODll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    // Try to load the "StarsUtils.dll" dll created in Delphi
    hTransEngineDll = LoadLibrarySafe("TransEngine.dll");
    if (hTransEngineDll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    // Try to load the "CalcGainLoss.dll" dll created in C
    hGainLossDll = LoadLibrarySafe("CalcGainLoss.dll");
    if (hGainLossDll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    hDelphiCInterfaceDll = LoadLibrarySafe("DelphiCInterface.dll");
    if (hDelphiCInterfaceDll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    hStarsUtilsDll = LoadLibrarySafe("StarsUtils.DLL");
    if (hStarsUtilsDll == NULL) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    lpprInitializeErrStruct =
        (LPPRERRSTRUCT)GetProcAddress(hTransEngineDll, "InitializeErrStruct");
    if (!lpprInitializeErrStruct) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }
    lpprInitializeErrStruct(&zErr);

    lpfnPrintError =
        (LPFNPRINTERROR)GetProcAddress(hTransEngineDll, "PrintError");
    if (!lpfnPrintError) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    lpfnSetErrorFile =
        (LPFN1PCHAR)GetProcAddress(hTransEngineDll, "SetErrorFileName");
    if (!lpfnSetErrorFile) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    zErr.iBusinessError = lpfnSetErrorFile(sErrFile);
    if (zErr.iBusinessError != 0)
      return (lpfnPrintError("Invalid Error File Name ", 0, 0, "",
                             zErr.iBusinessError, 0, 0, "ACCTVALUATION INIT1",
                             FALSE));

    // lpprStarsIOInit =	(LPPR3PCHAR1LONG1INT)GetProcAddress(hOledbIODll,
    // "InitializeStarsIO");
    lpprStarsIOInit =
        (LPPR3PCHAR1LONG1INT)GetProcAddress(hOledbIODll, "InitializeOLEDBIO");
    if (!lpprStarsIOInit) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitializeStarsIO Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT2", FALSE));
    }

    lpfnStartTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "StartDBTransaction");
    if (!lpfnStartTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load StartDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT3", FALSE));
    }

    lpfnCommitTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "CommitDBTransaction");
    if (!lpfnCommitTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CommitDBTransaction Function", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT4",
                             FALSE));
    }

    lpfnRollbackTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "RollbackDBTransaction");
    if (!lpfnRollbackTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load RollbackTransaction Function", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT5",
                             FALSE));
    }

    lpfnGetTransCount = (LPFNVOID)GetProcAddress(hOledbIODll, "GetTransCount");
    if (!lpfnGetTransCount) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load GetTransCount Function", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT5a", FALSE));
    }

    lpfnAbortTransaction =
        (LPFN1BOOL)GetProcAddress(hOledbIODll, "AbortDBTransaction");
    if (!lpfnAbortTransaction) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load AbortDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT5b", FALSE));
    }

    lpprCloseQueries =
        (LPPRVOID)GetProcAddress(hOledbIODll, "CloseAllOpenQueries");
    if (!lpprCloseQueries) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CloseAllOpenQueries function", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT5A",
                             FALSE));
    }

    lpprSelectAccdiv =
        (LPPRSELECTONEACCDIV)GetProcAddress(hOledbIODll, "SelectOneAccdiv");
    if (!lpprSelectAccdiv) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectOneAccdiv Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT6", FALSE));
    }

    lpprSelectAllAccdivForAnAccount = (LPPRALLACCDIVFORANACCOUNT)GetProcAddress(
        hOledbIODll, "SelectAllAccdivForAnAccount");
    if (!lpprSelectAllAccdivForAnAccount) {
      iError = GetLastError();
      return (
          lpfnPrintError("Unable To Load SelectAllAccdivForAnAccount Procedure",
                         0, 0, "", iError, 0, 0, "ACCTVALUATION INIT7", FALSE));
    }

    lpprSelectPendingAccdivTransForAnAccount =
        (LPPRPENDINGACCDIVTRANSFORANACCOUNT)GetProcAddress(
            hOledbIODll, "SelectPendingAccdivTransForAnAccount");
    if (!lpprSelectPendingAccdivTransForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectPendingAccdivTransForAnAccount Procedure", 0, 0,
          "", iError, 0, 0, "ACCTVALUATION INIT7", FALSE));
    }

    lpprUpdateAccdiv = (LPPRACCDIV)GetProcAddress(hOledbIODll, "UpdateAccdiv");
    if (!lpprUpdateAccdiv) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateAccdiv Procedure", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT8", FALSE));
    }

    lpprSelectNextCallDatePrice = (LPPR2PCHAR1LONG1PDOUBLE1PLONG)GetProcAddress(
        hOledbIODll, "SelectNextCallDatePrice");
    if (!lpprSelectNextCallDatePrice) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectNextCallDatePrice Function",
                             0, 0, "", iError, 0, 0, "ACCTVALUATION INIT8A",
                             FALSE));
    }

    lpprSelectAllDivhistForAnAccount =
        (LPPRALLDIVHISTFORANACCOUNT)GetProcAddress(
            hOledbIODll, "SelectAllDivhistForAnAccount");
    if (!lpprSelectAllDivhistForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllDivhistForAnAccount Procedure", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT9", FALSE));
    }

    lpprUpdateDivhist =
        (LPPRDIVHIST)GetProcAddress(hOledbIODll, "UpdateDivhist");
    if (!lpprUpdateDivhist) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateDivhist Procedure", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT10", FALSE));
    }

    lpprSelectEquity =
        (LPPRSELECTPARTEQTY)GetProcAddress(hOledbIODll, "SelectPartEquities");
    if (!lpprSelectEquity) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectEquity Procedure", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT11", FALSE));
    }

    lpprSelectFixedinc =
        (LPPRSELECTPARTFINC)GetProcAddress(hOledbIODll, "SelectPartFixedinc");
    if (!lpprSelectFixedinc) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPartFixedinc Procedure", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT12",
                             FALSE));
    }

    lpfnCalcGainLoss =
        (LPFNCALCGAINLOSS)GetProcAddress(hGainLossDll, "CalcGainLoss");
    if (!lpfnCalcGainLoss) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CalcGainLoss Function", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT13", FALSE));
    }

    lpprGetSecurityPrice =
        (LPPRSECPRICE)GetProcAddress(hOledbIODll, "GetSecurityPrice");
    if (!lpprGetSecurityPrice) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load GetSecurityPrice Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT14", FALSE));
    }

    lpprSelectAllHxrefForAnAccount = (LPPRALLHXREFFORANACCOUNT)GetProcAddress(
        hOledbIODll, "SelectAllHedgxrefForAnAccount");
    if (!lpprSelectAllHxrefForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllHedgxrefForAnAccount Procedure", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT15", FALSE));
    }

    lpprUpdateHedgexref =
        (LPPRHEDGEXREF)GetProcAddress(hOledbIODll, "UpdateHedgxref");
    if (!lpprUpdateHedgexref) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateHedgeXref Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT16", FALSE));
    }

    lpprInitHoldcash = (LPPRINITHOLDCASH)GetProcAddress(
        hTransEngineDll, "InitializeHoldcashStruct");
    if (!lpprInitHoldcash) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitHoldcash Procedure", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT16A", FALSE));
    }

    lpprSelectAllHoldcashForAnAccount =
        (LPPRALLHOLDCASHFORANACCOUNT)GetProcAddress(
            hOledbIODll, "SelectAllHoldcashForAnAccount");
    if (!lpprSelectAllHoldcashForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllHoldcashForAnAccount Procedure", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT17", FALSE));
    }

    lpprUpdateHoldcash =
        (LPPRHOLDCASH)GetProcAddress(hOledbIODll, "UpdateHoldcash");
    if (!lpprUpdateHoldcash) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateHoldcash Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT18", FALSE));
    }

    lpprInitHoldings = (LPPRINITHOLDINGS)GetProcAddress(
        hTransEngineDll, "InitializeHoldingsStruct");
    if (!lpprInitHoldings) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load InitHoldings Procedure", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT18A", FALSE));
    }

    lpprSelectAllHoldingsForAnAccount =
        (LPPRALLHOLDINGSFORANACCOUNT)GetProcAddress(
            hOledbIODll, "SelectAllHoldingsForAnAccount");
    if (!lpprSelectAllHoldingsForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllHoldingsForAnAccount Procedure", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT19", FALSE));
    }

    lpprUpdateHoldings =
        (LPPRHOLDINGS)GetProcAddress(hOledbIODll, "UpdateHoldings");
    if (!lpprUpdateHoldings) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdateHoldings Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT20", FALSE));
    }

    lpprSelectAllPayrecForAnAccount = (LPPRALLPAYRECFORANACCOUNT)GetProcAddress(
        hOledbIODll, "SelectAllPayrecForAnAccount");
    if (!lpprSelectAllPayrecForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectAllPayrecForAnAccount Procedure", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT21", FALSE));
    }

    lpprUpdatePayrec = (LPPRPAYREC)GetProcAddress(hOledbIODll, "UpdatePayrec");
    if (!lpprUpdatePayrec) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdatePayrec Procedure", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT22", FALSE));
    }

    lpprSelectOnePmain =
        (LPPRPARTPMAINONE)GetProcAddress(hOledbIODll, "SelectOnePartPortmain");
    if (!lpprSelectOnePmain) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPartPortmainOne Procedure",
                             0, 0, "", iError, 0, 0, "ACCTVALUATION INIT23",
                             FALSE));
    }

    lpprSelectAllPmain =
        (LPPRPARTPMAINALL)GetProcAddress(hOledbIODll, "SelectAllPartPortmain");
    if (!lpprSelectAllPmain) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectPartPortmainAll Procedure",
                             0, 0, "", iError, 0, 0, "ACCTVALUATION INIT24",
                             FALSE));
    }

    lpprUpdateValDate =
        (LPPR1INT1LONG)GetProcAddress(hOledbIODll, "UpdatePortmainValDate");
    if (!lpprUpdateValDate) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load UpdatePortmainValDate Procedure",
                             0, 0, "", iError, 0, 0, "ACCTVALUATION INIT25",
                             FALSE));
    }

    lpprSelectAllSectypes =
        (LPPRALLSECTYPES)GetProcAddress(hOledbIODll, "SelectAllSectypes");
    if (!lpprSelectAllSectypes) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAllSectypes Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT26", FALSE));
    }

    lpprSelectStarsDate =
        (LPPR2PLONG)GetProcAddress(hOledbIODll, "SelectStarsDate");
    if (!lpprSelectStarsDate) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectStarsDate Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT27", FALSE));
    }

    lpprSelectTrans =
        (LPPRSELECTTRANS)GetProcAddress(hOledbIODll, "SelectOneTrans");
    if (!lpprSelectTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectOneTrans Procedure", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT28", FALSE));
    }

    lpfnCalculateYield = (LPFN1DOUBLE3PCHAR1LONGINT)GetProcAddress(
        hDelphiCInterfaceDll, "CalulateYield");
    if (!lpfnCalculateYield) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load CalulateYield Function", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT29", FALSE));
    }

    lpfnInflationIndexRatio = (LPFN1PCHAR3LONG)GetProcAddress(
        hDelphiCInterfaceDll, "CheckForInflationIndexRatio_c");
    if (!lpfnInflationIndexRatio) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load CheckForInflationIndexRatio function", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT29A", FALSE));
    }

    lpprSelectSysSettings =
        (LPPRSELECTSYSSETTINGS)GetProcAddress(hOledbIODll, "SelectSysSettings");
    if (!lpprSelectSysSettings) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load lpprSelectSysSettings Function", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT30",
                             FALSE));
    }

    lpfnSelectDivint =
        (LPFNSELECTDIVINT)GetProcAddress(hOledbIODll, "SelectDivint");
    if (!lpfnSelectDivint) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectDivint Function", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT31", FALSE));
    }

    lpfnLastBusinessDay = (LPFN1LONG2PCHAR1PLONG)GetProcAddress(
        hStarsUtilsDll, "LastBusinessDay");
    if (!lpfnLastBusinessDay) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load LastBusinessDay Function", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT32", FALSE));
    }

    lpfnNextBusinessDay = (LPFN1LONG2PCHAR1PLONG)GetProcAddress(
        hStarsUtilsDll, "NextBusinessDay");
    if (!lpfnNextBusinessDay) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load NextBusinessDay Function", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT33", FALSE));
    }

    lpfnIsItAMarketHoliday =
        (LPFN1LONG1PCHAR)GetProcAddress(hStarsUtilsDll, "IsItAMarketHoliday");
    if (!lpfnIsItAMarketHoliday) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load IsItAMarketHolidayDay Function", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT34",
                             FALSE));
    }

    lpprSelectCurrencySecno = (LPPRSELECTCURRENCYSECNO)GetProcAddress(
        hOledbIODll, "SelectCurrencySecno");
    if (!lpprSelectCurrencySecno) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load lpprSelectCurrencySecno Function",
                             0, 0, "", iError, 0, 0, "ACCTVALUATION INIT35",
                             FALSE));
    }

    lpprSelectPartCurrency =
        (LPPRPPARTCURR)GetProcAddress(hOledbIODll, "SelectAllPartCurrencies");
    if (!lpprSelectPartCurrency) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load lpprSelectPartCurrency", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT36", FALSE));
    }

    lpfnSelectSecurityRate =
        (LPFNSELECTDIVINT)GetProcAddress(hOledbIODll, "SelectSecurityRate");
    if (!lpfnSelectSecurityRate) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load lpfnSelectSecurityRate", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT37", FALSE));
    }

    lpfnItsManualPriceSecurity = (LPFNITSMANUALPRICESECURITY)GetProcAddress(
        hOledbIODll, "ItsManualPriceSecurity");
    if (!lpfnItsManualPriceSecurity) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load lpfnItsManualPriceSecurity", 0, 0,
                             "", iError, 0, 0, "ACCTVALUATION INIT38", FALSE));
    }

    lpprSelectAsset = (LPPRASSETS)GetProcAddress(hOledbIODll, "SelectAsset");
    if (!lpprSelectAsset) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectAsset function", 0, 0, "",
                             iError, 0, 0, "ACCTVALUATION INIT39", FALSE));
    }

    lpprSelectUnitsHeldForASecurity =
        (LPPR1INT4PCHAR2LONG1PDOUBLE)GetProcAddress(
            hOledbIODll, "SelectUnitsHeldForASecurity");
    if (!lpprSelectUnitsHeldForASecurity) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectUnitsHeldForASecurity function", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT40", FALSE));
    }

    lpprSelectUnitsForASoldSecurity =
        (LPPR1INT4PCHAR3LONG1PDOUBLE)GetProcAddress(
            hOledbIODll, "SelectUnitsForASoldSecurity");
    if (!lpprSelectUnitsForASoldSecurity) {
      iError = GetLastError();
      return (lpfnPrintError(
          "Unable To Load SelectUnitsForSoldSecurity function", 0, 0, "",
          iError, 0, 0, "ACCTVALUATION INIT41", FALSE));
    }

    lpprSelectSysvalues =
        (LPPRSELECTSYSVALUES)GetProcAddress(hOledbIODll, "SelectSysvalues");
    if (!lpprSelectSysvalues) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectSysvalues", 0, 0, "", iError,
                             0, 0, "ACCTVALUATION INIT42", FALSE));
    }

    lpprSelectCustomPriceForAnAccount = (LPPRCUSTOMPRICE)GetProcAddress(
        hOledbIODll, "SelectCustomPricesForAnAccount");
    if (!lpprSelectCustomPriceForAnAccount) {
      iError = GetLastError();
      return (lpfnPrintError("Unable To Load SelectCustomPriceForAnAccount", 0,
                             0, "", iError, 0, 0, "ACCTVALUATION INIT43",
                             FALSE));
    }

    // Build the global tables
    zErr = BuildSecTypeTable(&zSTTable);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    memset(&zSystemSettings, 0, sizeof(SYSTEM_SETTINGS));
    zErr = LoadSystemSettings(&zSystemSettings);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    zErr = LoadPartCurrencyTable(&zCurrencyTable);
    if (zErr.iSqlError != SQLNOTFOUND || zErr.iBusinessError != 0)
      return zErr;

    /*
    ** SB - 1/4/2000. VERY VERY IMPORTANT - DO NOT DELETE FOLLOWING CALL
    ** Call a function in StarsUtils so that that Dll gets initialized once. If
    * the Dll is
    ** not initialized in the begining and later on any of the function is
    * required from that
    ** Dll, it will get initialized with default date (-1) and if that's the
    * date for which
    ** Valuation is not initialized - valuation result will go into wrong data
    * sets.
    ** Date has to be greater than 0 and the day should not be Saturday or
    * Sunday in order to initialize starsio.dll
    */
    lpfnLastBusinessDay(36867, const_cast<char *>("USA"),
                        const_cast<char *>("M"), &iTemp);

    /*		TimerDll = LoadLibrarySafe("Timer.dll");
                    if (TimerDll == NULL)
                    {
                            iError = GetLastError();
                            return(lpfnPrintError("Unable To Load Timer Dll", 0,
       0, "", iError, 0, 0, "ACCTVALUATION INIT92", FALSE));
                    }

                    lpfnTimer =	(LPFN1INT)GetProcAddress(TimerDll, "Timer");
                    if (!lpfnTimer)
                    {
                            iError = GetLastError();
                            return(lpfnPrintError("Unable To Load Timer
       function", 0, 0, "", iError, 0, 0, "ACCTVALUATION INIT6", FALSE));
                    }

                    lpprTimerResult =	(LP2PR1PCHAR)GetProcAddress(TimerDll,
       "TimerResult"); if (!lpprTimerResult)
                    {
                            iError = GetLastError();
                            return(lpfnPrintError("Unable To Load TimerResult
       Procedure", 0, 0, "", iError, 0, 0, "ACCTVALUATION INIT6", FALSE));
                    }*/
  } // If routine was not initialized before

  // Initialize starsio for the given alias and asof date
  lpprStarsIOInit(sDBAlias, const_cast<char *>(""), const_cast<char *>(""),
                  lAsofDate, 0, &zErr);
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
    bInit = TRUE;

  return zErr;
} // InitializeAccountValuation

void InitializeParthxref(PARTHXREF *pzPH) {
  pzPH->iID = 0;
  pzPH->sSecNo[0] = pzPH->sWi[0] = pzPH->sSecXtend[0] = pzPH->sAcctType[0] =
      '\0';
  pzPH->lTransNo = 0;
  pzPH->fHedgeValNative = pzPH->fHedgeValSystem = 0;
}

void InitializePhxrefTable(PHXREFTABLE *pzPTable) {
  if (pzPTable->iPhxrefCreated > 0 && pzPTable->pzPhxref != NULL)
    free(pzPTable->pzPhxref);

  pzPTable->pzPhxref = NULL;
  pzPTable->iPhxrefCreated = pzPTable->iNumPhxref = 0;
}

void InitializePriceTable(PRICETABLE *pzPTable) {
  if (pzPTable->iPriceCreated > 0 && pzPTable->pzPrice != NULL)
    free(pzPTable->pzPrice);

  pzPTable->pzPrice = NULL;
  pzPTable->iNumPrice = pzPTable->iPriceCreated = 0;
} // InitializePriceTable

// This function initialzes those fields in Portbal which are recalculated by
// valuation
void InitializeUpdatableFieldsInPortbal(PORTBAL *pzPbal) {
  pzPbal->fAccrInc = pzPbal->fIhDiv = pzPbal->fOhDiv = pzPbal->fAnnualInc = 0;
  pzPbal->fYield = pzPbal->fCurEquity = pzPbal->fCashAvail =
      pzPbal->fFundsAvail = 0;
  pzPbal->fCashVal = pzPbal->fMoneymktLval = pzPbal->fBondLval =
      pzPbal->fEquityLval = 0;
  pzPbal->fOptionLval = pzPbal->fMfundEqtyLval = pzPbal->fMfundBondLval = 0;
  pzPbal->fPreferredLval = pzPbal->fCvtBondLval = pzPbal->fTbillLval =
      pzPbal->fStInstrLval = 0;
  pzPbal->fMoneymktSval = pzPbal->fBondSval = pzPbal->fEquitySval =
      pzPbal->fOptionSval = 0;
  pzPbal->fMfundEqtySval = pzPbal->fMfundBondSval = pzPbal->fPreferredSval =
      pzPbal->fCvtBondSval = 0;
  pzPbal->fTbillSval = pzPbal->fStInstrSval = pzPbal->fWiCalc =
      pzPbal->fMarkToMarket = 0;
  pzPbal->fLiabilityVal = pzPbal->fNonSupVal = 0;
}

void InitializePortbalXtra(PBALXTRA *pzPbXtra) {
  pzPbXtra->fLongCash = pzPbXtra->fTBillMinusHedge = pzPbXtra->fShortCash = 0;
  pzPbXtra->fShortHoldings = pzPbXtra->fMMarket = pzPbXtra->fWhenIssued = 0;
}

ERRSTRUCT BuildSecTypeTable(SECTYPETABLE *pzSTable) {
  ERRSTRUCT zErr;
  SECTYPE zTempST;

  lpprInitializeErrStruct(&zErr);

  pzSTable->iNumSType = 0;
  while (zErr.iSqlError == 0) {
    lpprSelectAllSectypes(&zTempST, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return (lpfnPrintError("Error Fetching Sec Type Cursor", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "VALUATION BUILDSEC1", FALSE));

    if (pzSTable->iNumSType == NUMSECTYPES)
      return (lpfnPrintError("Sec Type Table Is Full", 0, 0, "", 997, 0, 0,
                             "VALUATION BUILDSEC2", FALSE));

    pzSTable->zSType[pzSTable->iNumSType++] = zTempST;
  } /* No error in fetching sec type cursor */

  return zErr;
} /* BuildSecTypeTable */

int FindSectype(SECTYPETABLE zSTable, int iSecType) {
  int i, iIdx;

  iIdx = -1;
  i = 0;
  while (i < zSTable.iNumSType && iIdx == -1) {
    if (zSTable.zSType[i].iSecType == iSecType)
      iIdx = i;

    i++;
  }

  return iIdx;
} // FindSecType

int FindPrice(char *sSecNo, char *sWi, long lPriceDate, PRICETABLE zPTable) {
  int i, iIdx;

  iIdx = -1;
  i = 0;
  while (i < zPTable.iNumPrice && iIdx == -1) {
    if (strcmp(zPTable.pzPrice[i].sSecNo, sSecNo) == 0 &&
        strcmp(zPTable.pzPrice[i].sWi, sWi) == 0 &&
        lPriceDate == zPTable.pzPrice[i].lPriceDate)

      iIdx = i;

    i++;
  }

  return iIdx;
} // FindPrice

ERRSTRUCT GetSecurityPrice(char *sSecNo, char *sWi, long lPriceDate,
                           SECTYPETABLE zSTable, int iID,
                           CUSTOMPRICETABLE zCPTable, PRICETABLE *pzPTable,
                           PRICEINFO *pzPrice) {
  ERRSTRUCT zErr;
  int i, j;
  char sMsg[80];
  BOOL bCustomPriceFound;

  lpprInitializeErrStruct(&zErr);

  i = FindPrice(sSecNo, sWi, lPriceDate, *pzPTable);
  if (i >= 0 && i < pzPTable->iNumPrice) {
    *pzPrice = pzPTable->pzPrice[i];
    return zErr;
  }

  lpprGetSecurityPrice(sSecNo, sWi, lPriceDate, pzPrice, &zErr);
  if (zErr.iSqlError)
    return zErr;
  else {
    pzPrice->iSTypeIdx = FindSectype(zSTable, pzPrice->iSecType);
    if (pzPrice->iSTypeIdx == -1) {
      sprintf_s(sMsg, "Invalid Sectype %d For Security %s, %s",
                pzPrice->iSecType, pzPrice->sSecNo, pzPrice->sWi);
      return (lpfnPrintError(sMsg, 0, 0, "", 18, 0, 0, "VALUATION GETPRICE",
                             FALSE));
    }

    zErr = AddPriceToTable(pzPTable, *pzPrice);
  }

  // SB 7/20/2012 If this client uses custom prices and there is a custom price
  // for this security, overwrite the system prce with the custom price.
  if (zSystemSettings.bUseCustomSecurityPrice) {
    bCustomPriceFound = FALSE;
    for (j = 0; j < zCPTable.iNumCPrice; j++) {
      if (zCPTable.pzCPrice[j].iCustomItemID =
              iID && zCPTable.pzCPrice[j].lPriceDate == lPriceDate &&
              strcmp(sSecNo, zCPTable.pzCPrice[j].sSecNo) == 0 &&
              strcmp(sWi, zCPTable.pzCPrice[j].sWi) == 0) {
        pzPrice->fClosePrice = zCPTable.pzCPrice[j].fClosePrice;
        pzPrice->bIsPriceValid = TRUE;
        break;
      }
    }
  }

  return zErr;
} // GetSecurityPrice

ERRSTRUCT AddHedgexrefToTable(PHXREFTABLE *pzHxrefTable, PARTHXREF zAddHxref) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzHxrefTable->iPhxrefCreated == pzHxrefTable->iNumPhxref) {
    pzHxrefTable->iPhxrefCreated += EXTRAHXREF;
    pzHxrefTable->pzPhxref =
        (PARTHXREF *)realloc(pzHxrefTable->pzPhxref,
                             sizeof(PARTHXREF) * pzHxrefTable->iPhxrefCreated);

    if (pzHxrefTable->pzPhxref == NULL)
      return (lpfnPrintError("Insufficient Memory", zAddHxref.iID, 0, "", 999,
                             0, 0, "VALUATION ADDHXREF", FALSE));
  }

  pzHxrefTable->pzPhxref[pzHxrefTable->iNumPhxref++] = zAddHxref;

  return zErr;
} // AddHedgexrefToTable

ERRSTRUCT AddPriceToTable(PRICETABLE *pzPTable, PRICEINFO zPInfo) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzPTable->iPriceCreated == pzPTable->iNumPrice) {
    pzPTable->iPriceCreated += EXTRAPRICE;
    pzPTable->pzPrice = (PRICEINFO *)realloc(
        pzPTable->pzPrice, sizeof(PRICEINFO) * pzPTable->iPriceCreated);

    if (pzPTable->pzPrice == NULL)
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 999, 0, 0,
                             "VALUATION ADDPRICE", FALSE));
  }

  pzPTable->pzPrice[pzPTable->iNumPrice++] = zPInfo;

  return zErr;
} // AddPriceToTable

void CreateTransFromHoldings(HOLDINGS zHold, TRANS *pzTrans, long lPriceDate,
                             long *plSecImpact) {
  pzTrans->fTotCost = zHold.fTotCost;
  pzTrans->fSecBaseXrate = zHold.fMvBaseXrate;
  pzTrans->fBaseXrate = zHold.fMvBaseXrate;
  pzTrans->fBaseOpenXrate = zHold.fBaseCostXrate;
  pzTrans->lTrdDate = lPriceDate;
  pzTrans->lEffDate = lPriceDate;
  pzTrans->lOpenTrdDate = zHold.lTrdDate;
  pzTrans->fPcplAmt = zHold.fMktVal;
  pzTrans->fAccrInt = zHold.fAccrInt;
  //	strcpy_s(pzTrans->sCurrId, zHold.sCurrId);
  if (strcmp(zHold.sPermLtFlag, "Y") == 0)
    strcpy_s(pzTrans->sGlFlag, "L");
  else
    strcpy_s(pzTrans->sGlFlag, " ");
  pzTrans->lRevTransNo = 0;
  if (zHold.fUnits < 0.0) {
    strcpy_s(pzTrans->sTranType, "CS");
    pzTrans->fTotCost *= -1;
    pzTrans->fPcplAmt *= -1;
    pzTrans->fAccrInt *= -1;
    *plSecImpact = 1;
  } else {
    strcpy_s(pzTrans->sTranType, "SL");
    *plSecImpact = -1;
  }
} // CreateTransFromHoldings

void CreateTransFromHoldcash(HOLDCASH zHcash, TRANS *pzTrans, long lPriceDate,
                             long *plSecImpact) {
  pzTrans->fTotCost = zHcash.fTotCost;
  pzTrans->fSecBaseXrate = zHcash.fMvBaseXrate;
  pzTrans->fBaseXrate = zHcash.fMvBaseXrate;
  pzTrans->fBaseOpenXrate = zHcash.fBaseCostXrate;
  pzTrans->lTrdDate = lPriceDate;
  pzTrans->lEffDate = lPriceDate;
  pzTrans->lOpenTrdDate = zHcash.lTrdDate;
  pzTrans->fPcplAmt = zHcash.fMktVal;
  pzTrans->fAccrInt = 0;
  //	strcpy_s(pzTrans->sCurrId, zHcash.sCurrId);
  strcpy_s(pzTrans->sGlFlag, " ");
  pzTrans->lRevTransNo = 0;
  if (zHcash.fUnits < 0.0) {
    strcpy_s(pzTrans->sTranType, "CS");
    pzTrans->fTotCost *= -1;
    pzTrans->fPcplAmt *= -1;
    pzTrans->fAccrInt *= -1;
    *plSecImpact = 1;
  } else {
    strcpy_s(pzTrans->sTranType, "SL");
    *plSecImpact = -1;
  }
} // CreateTransFromHoldcash

ERRSTRUCT LoadSystemSettings(SYSTEM_SETTINGS *pzSysSetng) {
  ERRSTRUCT zErr;
  SYSVALUES zSysValues;

  lpprInitializeErrStruct(&zErr);
  memset(pzSysSetng, sizeof(*pzSysSetng), 0);
  lpprSelectSysSettings(&pzSysSetng->zSyssetng, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  strcpy_s(zSysValues.sName, "UseCustomSecurityPrice");
  lpprSelectSysvalues(&zSysValues, &zErr);
  if (zErr.iSqlError == SQLNOTFOUND) {
    pzSysSetng->bUseCustomSecurityPrice = FALSE;
    zErr.iSqlError = 0;
  } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  else if (_stricmp(zSysValues.sValue, "1") == 0 ||
           stricmp(zSysValues.sValue, "Y") == 0)
    pzSysSetng->bUseCustomSecurityPrice = TRUE;
  else
    pzSysSetng->bUseCustomSecurityPrice = FALSE;

  return zErr;
}

void InitializePartCurrencyTable(CURRTABLE *pzCurrTable) {
  memset(pzCurrTable, 0, NUMCURRENCY);
}

ERRSTRUCT LoadPartCurrencyTable(CURRTABLE *pzCurrTable) {
  ERRSTRUCT zErr;
  int i = 0;
  PARTCURR zTempCurrency;

  lpprInitializeErrStruct(&zErr);
  InitializePartCurrencyTable(pzCurrTable);

  while (zErr.iSqlError == 0) {
    memset(&zTempCurrency, 0, sizeof(PARTCURR));
    lpprSelectPartCurrency(&zTempCurrency, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    pzCurrTable->zCurrency[i++] = zTempCurrency;
  } /*while*/
  return zErr;
}

ERRSTRUCT GetCurrencySecNo(char *sSecNo, char *sWi, char *sBaseCurrId) {
  int i;
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);
  sSecNo[0] = sWi[0] = '\0';

  for (i = 0; i < NUMCURRENCY; i++) {
    if (_stricmp(sBaseCurrId, zCurrencyTable.zCurrency[i].sCurrId) == 0) {
      strcpy_s(sSecNo, STR12LEN, zCurrencyTable.zCurrency[i].sSecNo);
      strcpy_s(sWi, STR1LEN, zCurrencyTable.zCurrency[i].sWi);

      return zErr;
    } /*if*/
  } /*for*/

  zErr.iSqlError = SQLNOTFOUND;
  return zErr;
}

void InitializeCustomPriceTable(CUSTOMPRICETABLE *pzCPTable) {
  if (pzCPTable->iCPriceCreated > 0 && pzCPTable->pzCPrice != NULL)
    free(pzCPTable->pzCPrice);

  pzCPTable->pzCPrice = NULL;
  pzCPTable->iNumCPrice = pzCPTable->iCPriceCreated = 0;
} // InitializeCustomPriceTable

ERRSTRUCT AddCustomPriceToTable(CUSTOMPRICETABLE *pzCPTable,
                                CUSTOMPRICEINFO zCPInfo) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzCPTable->iCPriceCreated == pzCPTable->iNumCPrice) {
    pzCPTable->iCPriceCreated += EXTRACUSTOMPRICE;
    pzCPTable->pzCPrice = (CUSTOMPRICEINFO *)realloc(
        pzCPTable->pzCPrice,
        sizeof(CUSTOMPRICEINFO) * pzCPTable->iCPriceCreated);

    if (pzCPTable->pzCPrice == NULL)
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 999, 0, 0,
                             "VALUATION ADDCUSTOMPRICE", FALSE));
  }

  pzCPTable->pzCPrice[pzCPTable->iNumCPrice++] = zCPInfo;

  return zErr;
} // AddCustomPriceToTable

ERRSTRUCT GetAllCustomPrice(CUSTOMPRICETABLE *pzCPTable, int iCustomTypeID,
                            long lPriceDate) {
  CUSTOMPRICEINFO zCPInfo;
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  InitializeCustomPriceTable(pzCPTable);

  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    lpprSelectCustomPriceForAnAccount(iCustomTypeID, lPriceDate, &zCPInfo,
                                      &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return (lpfnPrintError("Error Fetching Custom price", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "VALUATION GETALLCUSTOMPRICE1", FALSE));

    zErr = AddCustomPriceToTable(pzCPTable, zCPInfo);
  } /* No error in fetching sec type cursor */

  return zErr;
}

void FreeValuation() {
  if (bInit) {
    FreeLibrary(hOledbIODll);
    FreeLibrary(hTransEngineDll);
    FreeLibrary(hGainLossDll);
    FreeLibrary(hDelphiCInterfaceDll);
    FreeLibrary(hStarsUtilsDll);

    bInit = FALSE;
  }
} // freeValuation
} // extern "C"
