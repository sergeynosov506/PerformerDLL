/**
 *
 * SUB-SYSTEM: TranProc
 *
 * FILENAME: tpdbrtns.ec
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
// History
// 2013-06-11 VI#52871 Removed references to StarsIO -mk.
// 2007-11-08 Added Vendor id    - yb
// 2007-11-21 Put back erroneously deleted selectsectype in
// Preparefortranalloc() - yb.

#include "transengine.h"
HINSTANCE hOledbIODll, hStarsUtilsDll, hGainLossDll, hDelphiCInterfaceDll;
BOOL bTProcInitialized = FALSE;

/**
** Function which initializes this Dll. It loads other dlls required by this
** Dll and loads all the functions/procedures used from those dlls.
**/
DLLAPI ERRSTRUCT STDCALL WINAPI InitTranProc(long lAsofDate, char *sDBAlias,
                                             char *sMode, char *sType,
                                             BOOL bPrepareQueries,
                                             char *sErrFile) {
  ERRSTRUCT zErr;
  int iError = 0;

  InitializeErrStruct(&zErr);

  zErr.iBusinessError = SetErrorFileName(sErrFile);
  if (zErr.iBusinessError != 0)
    return (PrintError("Invalid Error File Name ", 0, 0, "",
                       zErr.iBusinessError, 0, 0, "TPROC INIT0", FALSE));

  if (!bTProcInitialized) {

    // Try to load the "oledbio.dll" dll created in C
    hOledbIODll = LoadLibrarySafe((LPCTSTR) "oledbio.dll");
    if (hOledbIODll == NULL) {
      iError = GetLastError();
      return (PrintError("Unable To Load oledbio.dll", 0, 0, "", iError, 0, 0,
                         "TPROC INIT1b", FALSE));
    }

    // Try to load the "StarsUtils.dll" dll created in Delphi
    hStarsUtilsDll = LoadLibrarySafe("StarsUtils.dll");
    if (hStarsUtilsDll == NULL) {
      iError = GetLastError();
      return (PrintError("Unable To Load StarsUtils.dll", 0, 0, "", iError, 0,
                         0, "TPROC INIT2", FALSE));
    }

    // Try to load the "CalcGainLoss.dll" dll created in C
    hGainLossDll = LoadLibrarySafe("CalcGainLoss.dll");
    if (hGainLossDll == NULL) {
      iError = GetLastError();
      return (PrintError("Unable To Load CalcGainLoss.dll", 0, 0, "", iError, 0,
                         0, "TPROC INIT3", FALSE));
    }

    // Try to load the "DelphiCInterface.dll" dll created in Delphi
    hDelphiCInterfaceDll = LoadLibrarySafe("DelphiCInterface.dll");
    if (hDelphiCInterfaceDll == NULL) {
      iError = GetLastError();
      return (PrintError("Unable To Load DelphiCInterfaceDll.dll", 0, 0, "",
                         iError, 0, 0, "TPROC INIT3A", FALSE));
    }

    // lpprStarsIOInit =
    // (LPPR3PCHAR1LONG1BOOL)GetProcAddress(hStarsIODll, "InitializeStarsIO");
    lpprStarsIOInit =
        (LPPR3PCHAR1LONG1BOOL)GetProcAddress(hOledbIODll, "InitializeOLEDBIO");
    if (!lpprStarsIOInit) {
      iError = GetLastError();
      return (PrintError("Unable To Load InitializeOlEDBIO function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT4", FALSE));
    }

    lpfnStartTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "StartDBTransaction");
    if (!lpfnStartTransaction) {
      iError = GetLastError();
      return (PrintError("Unable To Load StartTransaction function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT5", FALSE));
    }

    lpfnCommitTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "CommitDBTransaction");
    if (!lpfnCommitTransaction) {
      iError = GetLastError();
      return (PrintError("Unable To Load CommitTransaction function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT6", FALSE));
    }

    lpfnRollbackTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "RollbackDBTransaction");
    if (!lpfnRollbackTransaction) {
      iError = GetLastError();
      return (PrintError("Unable To Load RollbackTransaction function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT7", FALSE));
    }

    lpfnGetTransCount = (LPFNVOID)GetProcAddress(hOledbIODll, "GetTransCount");
    if (!lpfnGetTransCount) {
      iError = GetLastError();
      return (PrintError("Unable To Load GetTransCount function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT7a", FALSE));
    }

    lpfnAbortTransaction =
        (LPFN1BOOL)GetProcAddress(hOledbIODll, "AbortDBTransaction");
    if (!lpfnAbortTransaction) {
      iError = GetLastError();
      return (PrintError("Unable To Load AbortDBTransaction function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT7b", FALSE));
    }

    lpprDeleteAccdivOneLot =
        (LPPR3LONG)GetProcAddress(hOledbIODll, "DeleteAccdivOneLot");
    if (!lpprDeleteAccdivOneLot) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteAccdivOneLot function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT8", FALSE));
    }

    lpprDeleteAccruingAccdivOneLot = (LPPR1INT4PCHAR1LONG)GetProcAddress(
        hOledbIODll, "DeleteAccruingAccdivOneLot");
    if (!lpprDeleteAccruingAccdivOneLot) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteAccruingAccdivOneLot function",
                         0, 0, "", iError, 0, 0, "TPROC INIT8", FALSE));
    }

    lpprDeleteAccdivAllLots =
        (LPPR1INT1LONG)GetProcAddress(hOledbIODll, "DeleteAccdivAllLots");
    if (!lpprDeleteAccdivAllLots) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteAccdivAllLots function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT9", FALSE));
    }

    lpprSelectAsset = (LPPRASSETS)GetProcAddress(hOledbIODll, "SelectAsset");
    if (!lpprSelectAsset) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectAsset function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT10", FALSE));
    }

    lpprCurrencySecno = (LPPR2PCHAR1PINT1PCHAR)GetProcAddress(
        hOledbIODll, "SelectCurrencySecno");
    if (!lpprCurrencySecno) {
      iError = GetLastError();
      return (PrintError("Unable To Load CurrencuSecNofunction", 0, 0, "",
                         iError, 0, 0, "TPROC INIT11", FALSE));
    }

    lpprSelectCallPut =
        (LPPR3PCHAR)GetProcAddress(hOledbIODll, "SelectCallPut");
    if (!lpprSelectCallPut) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectCallPut function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT12", FALSE));
    }

    lpprUpdateDivhistOneLot = (LPPR2PCHAR4LONG1PCHAR)GetProcAddress(
        hOledbIODll, "UpdateDivhistOneLot");
    if (!lpprUpdateDivhistOneLot) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateDivhistOneLot function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT13", FALSE));
    }

    lpprUpdateDivhistAllLots = (LPPR2PCHAR3LONG1PCHAR)GetProcAddress(
        hOledbIODll, "UpdateDivhistAllLots");
    if (!lpprUpdateDivhistAllLots) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateDivhistAllLots function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT13a", FALSE));
    }

    lpprDeleteDivhistOneLot =
        (LPPR1INT3LONG1PCHAR)GetProcAddress(hOledbIODll, "DeleteDivhistOneLot");
    if (!lpprDeleteDivhistOneLot) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteDivhistOneLot function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT14", FALSE));
    }

    lpprDeleteAccruingDivhistOneLot = (LPPR1INT4PCHAR1LONG)GetProcAddress(
        hOledbIODll, "DeleteAccruingDivhistOneLot");
    if (!lpprDeleteAccruingDivhistOneLot) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteAccruingDivhistOneLot function",
                         0, 0, "", iError, 0, 0, "TPROC INIT14", FALSE));
    }

    lpprDeleteDivhistAllLots = (LPPR1INT2LONG1PCHAR)GetProcAddress(
        hOledbIODll, "DeleteDivhistAllLots");
    if (!lpprDeleteDivhistAllLots) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteDivhistAllLots function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT14a", FALSE));
    }

    lpprSelectDtrans =
        (LPPRDTRANSSELECT)GetProcAddress(hOledbIODll, "SelectDtrans");
    if (!lpprSelectDtrans) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectDtrans function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT15", FALSE));
    }

    lpprUpdateDtrans =
        (LPPRDTRANSUPDATE)GetProcAddress(hOledbIODll, "UpdateDtrans");
    if (!lpprUpdateDtrans) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateDtrans function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT16", FALSE));
    }

    lpprSelectDtransDesc =
        (LPPRDTRANSDESCSELECT)GetProcAddress(hOledbIODll, "SelectDtransDesc");
    if (!lpprSelectDtransDesc) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectDtransDesc function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT17", FALSE));
    }

    lpprSelectPartFixedinc =
        (LPPRSELECTPARTFINC)GetProcAddress(hOledbIODll, "SelectPartFixedinc");
    if (!lpprSelectPartFixedinc) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectPartFixedinc function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT18", FALSE));
    }

    lpprSelectHedgxref =
        (LPPRSELECTHEDGEXREF)GetProcAddress(hOledbIODll, "SelectHedgxref");
    if (!lpprSelectHedgxref) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectHedgxref function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT19", FALSE));
    }

    lpprUpdateHedgxref =
        (LPPRHEDGEXREF)GetProcAddress(hOledbIODll, "UpdateHedgxref");
    if (!lpprUpdateHedgxref) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateHedgxref function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT20", FALSE));
    }

    lpprInsertHedgxref =
        (LPPRHEDGEXREF)GetProcAddress(hOledbIODll, "InsertHedgxref");
    if (!lpprInsertHedgxref) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertHedgeXref function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT21", FALSE));
    }

    lpprDeleteHedgxref =
        (LPPR1INT4PCHAR1LONG)GetProcAddress(hOledbIODll, "DeleteHedgxref");
    if (!lpprDeleteHedgxref) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteHedgxref function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT22", FALSE));
    }

    lpprGetSecurityPrice =
        (LPPRSECPRICE)GetProcAddress(hOledbIODll, "GetSecurityPrice");
    if (!lpprGetSecurityPrice) {
      iError = GetLastError();
      return (PrintError("Unable To Load GetSecurityPrice Procedure", 0, 0, "",
                         iError, 0, 0, "TPROC INIT22A", FALSE));
    }

    lpprSelectHoldcash =
        (LPPRSELECTHOLDCASH)GetProcAddress(hOledbIODll, "SelectHoldcash");
    if (!lpprSelectHoldcash) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectHoldcash function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT23", FALSE));
    }

    lpprUpdateHoldcash =
        (LPPRHOLDCASH)GetProcAddress(hOledbIODll, "UpdateHoldcash");
    if (!lpprUpdateHoldcash) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateHoldcash function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT24", FALSE));
    }

    lpprInsertHoldcash =
        (LPPRHOLDCASH)GetProcAddress(hOledbIODll, "InsertHoldcash");
    if (!lpprInsertHoldcash) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertHoldcash function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT25", FALSE));
    }

    lpprDeleteHoldcash =
        (LPPR1LONG4PCHAR)GetProcAddress(hOledbIODll, "DeleteHoldcash");
    if (!lpprDeleteHoldcash) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteHoldcash function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT26", FALSE));
    }

    lpprHoldcashForFifoAndAvgAcct = (LPPRSELECTHOLDINGS)GetProcAddress(
        hOledbIODll, "HoldcashForFifoAndAvgAccts");
    if (!lpprHoldcashForFifoAndAvgAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldcashForFifoAndAvgAccts function",
                         0, 0, "", iError, 0, 0, "TPROC INIT27", FALSE));
    }

    lpprHoldcashForLifoAcct =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "HoldcashForLifoAccts");
    if (!lpprHoldcashForLifoAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldcashForLifoAccts function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT28", FALSE));
    }

    lpprHoldcashForHighAcct =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "HoldcashForHighAccts");
    if (!lpprHoldcashForHighAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldcashForHighAccts function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT29", FALSE));
    }

    lpprHoldcashForLowAcct =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "HoldcashForLowAccts");
    if (!lpprHoldcashForLowAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldcashForLowAccts function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT30", FALSE));
    }

    lpprHoldcashForMinimumGainAcct = (LPPRSELECTHOLDINGS2)GetProcAddress(
        hOledbIODll, "HoldcashForMinimumGainAccts");
    if (!lpprHoldcashForMinimumGainAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldcashForMinimumGainAccts function",
                         0, 0, "", iError, 0, 0, "TPROC INIT30A", FALSE));
    }

    lpprHoldcashForMaximumGainAcct = (LPPRSELECTHOLDINGS2)GetProcAddress(
        hOledbIODll, "HoldcashForMaximumGainAccts");
    if (!lpprHoldcashForMaximumGainAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldcashForMaximumGainAccts function",
                         0, 0, "", iError, 0, 0, "TPROC INIT30B", FALSE));
    }

    lpprSelectHolddel =
        (LPPRSELECTHOLDDEL)GetProcAddress(hOledbIODll, "SelectHolddel");
    if (!lpprSelectHolddel) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectHolddel function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT31", FALSE));
    }

    lpprInsertHolddel =
        (LPPRHOLDDEL)GetProcAddress(hOledbIODll, "InsertHolddel");
    if (!lpprInsertHolddel) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertHolddel function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT32", FALSE));
    }

    lpprHolddelUpdate = (LPPR4LONG)GetProcAddress(hOledbIODll, "HolddelUpdate");
    if (!lpprHolddelUpdate) {
      iError = GetLastError();
      return (PrintError("Unable To Load HolddelUpdate function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT33", FALSE));
    }

    lpprSelectHoldings =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "SelectHoldings");
    if (!lpprSelectHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectHoldings function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT34", FALSE));
    }

    lpprUpdateHoldings =
        (LPPRHOLDINGS)GetProcAddress(hOledbIODll, "UpdateHoldings");
    if (!lpprUpdateHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateHoldings function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT35", FALSE));
    }

    lpprInsertHoldings =
        (LPPRHOLDINGS)GetProcAddress(hOledbIODll, "InsertHoldings");
    if (!lpprInsertHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertHoldings function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT36", FALSE));
    }

    lpprDeleteHoldings =
        (LPPR1INT4PCHAR1LONG)GetProcAddress(hOledbIODll, "DeleteHoldings");
    if (!lpprDeleteHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeleteHoldings function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT37", FALSE));
    }

    lpprHoldingsForFifoAndAvgAcct = (LPPRSELECTHOLDINGS)GetProcAddress(
        hOledbIODll, "HoldingsForFifoAndAvgAccts");
    if (!lpprHoldingsForFifoAndAvgAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsForFifoAndAvgAccts function",
                         0, 0, "", iError, 0, 0, "TPROC INIT38", FALSE));
    }

    lpprHoldingsForLifoAcct =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "HoldingsForLifoAccts");
    if (!lpprHoldingsForLifoAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsForLifoAccts function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT39", FALSE));
    }

    lpprHoldingsForHighAcct =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "HoldingsForHighAccts");
    if (!lpprHoldingsForHighAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsForHighAccts function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT40", FALSE));
    }

    lpprHoldingsForLowAcct =
        (LPPRSELECTHOLDINGS)GetProcAddress(hOledbIODll, "HoldingsForLowAccts");
    if (!lpprHoldingsForLowAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsForLowAccts function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT41", FALSE));
    }

    lpprHoldingsForMinimumGainAcct = (LPPRSELECTHOLDINGS2)GetProcAddress(
        hOledbIODll, "HoldingsForMinimumGainAccts");
    if (!lpprHoldingsForMinimumGainAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsForMinimumGainAccts function",
                         0, 0, "", iError, 0, 0, "TPROC INIT41A", FALSE));
    }

    lpprHoldingsForMaximumGainAcct = (LPPRSELECTHOLDINGS2)GetProcAddress(
        hOledbIODll, "HoldingsForMaximumGainAccts");
    if (!lpprHoldingsForMaximumGainAcct) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsForMaximumGainAccts function",
                         0, 0, "", iError, 0, 0, "TPROC INIT41B", FALSE));
    }

    lpprMinimumGainHoldings =
        (LPPRSELECTHOLDINGS3)GetProcAddress(hOledbIODll, "MinimumGainHoldings");
    if (!lpprMinimumGainHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load MinimumGainHoldings function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT41C", FALSE));
    }

    lpprMinimumLossHoldings =
        (LPPRSELECTHOLDINGS3)GetProcAddress(hOledbIODll, "MinimumLossHoldings");
    if (!lpprMinimumLossHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load MinimumLossHoldings function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT41D", FALSE));
    }

    lpprMaximumLossHoldings =
        (LPPRSELECTHOLDINGS3)GetProcAddress(hOledbIODll, "MaximumLossHoldings");
    if (!lpprMaximumLossHoldings) {
      iError = GetLastError();
      return (PrintError("Unable To Load MaximumLossHoldings function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT41E", FALSE));
    }

    lpprHsumSelect =
        (LPPRHSUMSELECT)GetProcAddress(hOledbIODll, "HoldingsSumSelect");
    if (!lpprHsumSelect) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsSumSelect function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT42", FALSE));
    }

    lpprHsumUpdate =
        (LPPRHSUMUPDATE)GetProcAddress(hOledbIODll, "HoldingsSumUpdate");
    if (!lpprHsumUpdate) {
      iError = GetLastError();
      return (PrintError("Unable To Load HoldingsSumUpdate function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT43", FALSE));
    }

    lpprSelectPayrec =
        (LPPRSELECTPAYREC)GetProcAddress(hOledbIODll, "SelectPayrec");
    if (!lpprSelectPayrec) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectPayrec function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT44", FALSE));
    }

    lpprInsertPayrec = (LPPRPAYREC)GetProcAddress(hOledbIODll, "InsertPayrec");
    if (!lpprInsertPayrec) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertPayrec function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT45", FALSE));
    }

    lpprDeletePayrec =
        (LPPR1INT4PCHAR2LONG)GetProcAddress(hOledbIODll, "DeletePayrec");
    if (!lpprDeletePayrec) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeletePayrec function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT46", FALSE));
    }

    lpfnInsertPortLock =
        (LPFNINSERTPORTLOCK)GetProcAddress(hOledbIODll, "InsertPortLock");
    if (!lpfnInsertPortLock) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertPortLock function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT47", FALSE));
    }

    lpprDeletePortlock =
        (LPPRDELETEPORTLOCK)GetProcAddress(hOledbIODll, "DeletePortLock");
    if (!lpprDeletePortlock) {
      iError = GetLastError();
      return (PrintError("Unable To Load DeletePortLock function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT48", FALSE));
    }

    lpprSelectPortmain =
        (LPPRPORTMAIN)GetProcAddress(hOledbIODll, "SelectPortmain");
    if (!lpprSelectPortmain) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectPortmain function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT49", FALSE));
    }

    lpprSelectAcctMethod =
        (LPPR1PCHAR1LONG)GetProcAddress(hOledbIODll, "SelectAcctMethod");
    if (!lpprSelectAcctMethod) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectAcctMethod function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT50", FALSE));
    }

    lpprUpdatePortmainLastTransNo =
        (LPPR1INT1LONG)GetProcAddress(hOledbIODll, "UpdatePortmainLastTransNo");
    if (!lpprUpdatePortmainLastTransNo) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdatePortmainLastTransNo function", 0,
                         0, "", iError, 0, 0, "TPROC INIT51", FALSE));
    }

    lpprSelectSectype =
        (LPPRSECTYPE)GetProcAddress(hOledbIODll, "SelectSectype");
    if (!lpprSelectSectype) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectSectype function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT52", FALSE));
    }

    lpprSecCharacteristics =
        (LPPRSECCHAR)GetProcAddress(hOledbIODll, "SecurityCharacteristics");
    if (!lpprSecCharacteristics) {
      iError = GetLastError();
      return (PrintError("Unable To Load SecCharacteristics function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT53", FALSE));
    }

    lpprSelectStarsDate =
        (LPPR2PLONG)GetProcAddress(hOledbIODll, "SelectStarsDate");
    if (!lpprSelectStarsDate) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectStarsDate function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT54", FALSE));
    }

    lpprSelectSellAcctType =
        (LPPR2PCHAR)GetProcAddress(hOledbIODll, "SelectSellAcctType");
    if (!lpprSelectSellAcctType) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectSellAcctType function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT55", FALSE));
    }

    lpprSelectTrans =
        (LPPRTRANSSELECT)GetProcAddress(hOledbIODll, "SelectTrans");
    if (!lpprSelectTrans) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectTrans function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT56", FALSE));
    }

    lpprSelectRevNoAndCode = (LPPR1PLONG1PCHAR2LONG)GetProcAddress(
        hOledbIODll, "SelectRevTransNoAndCode");
    if (!lpprSelectRevNoAndCode) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectRevNoAndCode function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT57", FALSE));
    }

    lpprSelectTransForMatchingXref = (LPPRSELECTTRANS)GetProcAddress(
        hOledbIODll, "SelectTransForMatchingXref");
    if (!lpprSelectTransForMatchingXref) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectRevNoAndCode function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT58", FALSE));
    }

    lpprSelectOneTrans =
        (LPPRSELECTTRANS)GetProcAddress(hOledbIODll, "SelectOneTrans");
    if (!lpprSelectOneTrans) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectOneTrans function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT58A", FALSE));
    }

    lpprInsertTrans = (LPPRTRANS)GetProcAddress(hOledbIODll, "InsertTrans");
    if (!lpprInsertTrans) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertTrans function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT59", FALSE));
    }

    lpprUpdateNewTransNo =
        (LPPR3LONG)GetProcAddress(hOledbIODll, "UpdateNewTransNo");
    if (!lpprUpdateNewTransNo) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateNewTransNo function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT60", FALSE));
    }

    lpprUpdateRevTransNo =
        (LPPR3LONG)GetProcAddress(hOledbIODll, "UpdateRevTransNo");
    if (!lpprUpdateRevTransNo) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateRevTransNo function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT61", FALSE));
    }

    lpprUpdateXTransNo =
        (LPPR3LONG)GetProcAddress(hOledbIODll, "UpdateXTransNo");
    if (!lpprUpdateXTransNo) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateXtransNo function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT62", FALSE));
    }

    lpprUpdateBrokerInTrans =
        (LPPRTRANS)GetProcAddress(hOledbIODll, "UpdateBrokerInTrans");
    if (!lpprUpdateBrokerInTrans) {
      iError = GetLastError();
      return (PrintError("Unable To Load UpdateBrokerInTrans function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT62A", FALSE));
    }

    lpprSelectTransDesc =
        (LPPRDTRANSDESCSELECT)GetProcAddress(hOledbIODll, "SelectTransDesc");
    if (!lpprSelectTransDesc) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectTransDesc function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT63", FALSE));
    }

    lpprInsertTransDesc =
        (LPPRTRANSDESC)GetProcAddress(hOledbIODll, "InsertTransDesc");
    if (!lpprInsertTransDesc) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertTransDesc function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT64", FALSE));
    }

    lpprSelectTrantype =
        (LPPRSELECTTRANTYPE)GetProcAddress(hOledbIODll, "SelectTrantype");
    if (!lpprSelectTrantype) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectTranType function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT65", FALSE));
    }

    lpprSelectTrancode =
        (LPPR3PCHAR)GetProcAddress(hOledbIODll, "SelectTrancode");
    if (!lpprSelectTrancode) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectTrancode function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT66", FALSE));
    }

    lpprGetLastIncomeDate =
        (LPPR3LONG1PLONG1PINT)GetProcAddress(hOledbIODll, "GetLastIncomeDate");
    if (!lpprGetLastIncomeDate) {
      iError = GetLastError();
      return (PrintError("Unable To Load GetLastIncomeDate function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT66A", FALSE));
    }

    lpprGetIncomeForThePeriod = (LPPR4LONG2PCHAR2PLONG2PDOUBLE)GetProcAddress(
        hOledbIODll, "GetIncomeForThePeriod");
    if (!lpprGetIncomeForThePeriod) {
      iError = GetLastError();
      return (PrintError("Unable To Load GetIncomeForThePeriod function", 0, 0,
                         "", iError, 0, 0, "TPROC INIT66B", FALSE));
    }

    lpprSelectDPayTran =
        (LPPRSELECTPAYTRAN)GetProcAddress(hOledbIODll, "SelectDPayTran");
    if (!lpprSelectDPayTran) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectDPayTran function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT66C", FALSE));
    }

    lpprSelectPayTran =
        (LPPRSELECTPAYTRAN)GetProcAddress(hOledbIODll, "SelectPayTran");
    if (!lpprSelectPayTran) {
      iError = GetLastError();
      return (PrintError("Unable To Load SelectPayTran function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT66D", FALSE));
    }

    lpprInsertPayTran =
        (LPPRINSERTPAYTRAN)GetProcAddress(hOledbIODll, "InsertPayTran");
    if (!lpprInsertPayTran) {
      iError = GetLastError();
      return (PrintError("Unable To Load InsertPayTran function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT66E", FALSE));
    }

    lpfnrmdyjul = (LPFNRMDYJUL)GetProcAddress(hStarsUtilsDll, "rmdyjul");
    if (!lpfnrmdyjul) {
      iError = GetLastError();
      return (PrintError("Unable To Load rmdyjul function", 0, 0, "", iError, 0,
                         0, "TPROC INIT67", FALSE));
    }

    lpfnrjulmdy = (LPFNRJULMDY)GetProcAddress(hStarsUtilsDll, "rjulmdy");
    if (!lpfnrjulmdy) {
      iError = GetLastError();
      return (PrintError("Unable To Load rjulmdy function", 0, 0, "", iError, 0,
                         0, "TPROC INIT68", FALSE));
    }

    lpfnNewDateFromCurrent =
        (LPFNNEWDATE)GetProcAddress(hStarsUtilsDll, "NewDateFromCurrent");
    if (!lpfnNewDateFromCurrent) {
      iError = GetLastError();
      return (PrintError("Unable To Load NewDateFromCurrent function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT69", FALSE));
    }

    lpfnrstrdate = (LPFN1PCHAR1PLONG)GetProcAddress(hStarsUtilsDll, "rstrdate");
    if (!lpfnrstrdate) {
      iError = GetLastError();
      return (PrintError("Error Loading rstrdate Function", 0, 0, "", iError, 0,
                         0, "TPROC INIT70", FALSE));
    }

    lpfnCalcGainLoss =
        (LPFNCALCGAINLOSS)GetProcAddress(hGainLossDll, "CalcGainLoss");
    if (!lpfnCalcGainLoss) {
      iError = GetLastError();
      return (PrintError("Unable To Load CalcGainLoss function", 0, 0, "",
                         iError, 0, 0, "TPROC INIT71", FALSE));
    }

    lpfnInflationIndexRatio = (LPFN1PCHAR3LONG)GetProcAddress(
        hDelphiCInterfaceDll, "CheckForInflationIndexRatio_c");
    if (!lpfnInflationIndexRatio) {
      iError = GetLastError();
      return (PrintError("Unable To Load CheckForInflationIndexRatio function",
                         0, 0, "", iError, 0, 0, "TPROC INIT72", FALSE));
    }

    /*		TimerDll = LoadLibrarySafe("Timer.dll");
                    if (TimerDll == NULL)
                    {
                            iError = GetLastError();
                            return(PrintError("Unable To Load Timer Dll", 0, 0,
       "", iError, 0, 0, "TPROC INIT92", FALSE)); return zErr;
                    }

                    lpfnTimer =	(LPFN1INT)GetProcAddress(TimerDll, "Timer");
                    if (!lpfnTimer)
                    {
                            iError = GetLastError();
                            return(PrintError("Unable To Load Timer function",
       0, 0, "", iError, 0, 0, "TPROC INIT6", FALSE));
                    }

                    lpprTimerResult =	(LP2PR1PCHAR)GetProcAddress(TimerDll,
       "TimerResult"); if (!lpprTimerResult)
                    {
                            iError = GetLastError();
                            return(PrintError("Unable To Load TimerResult
       Procedure", 0, 0, "", iError, 0, 0, "TPROC INIT6", FALSE));
                    }*/
  } // if not previouly initialized

  // initialize the queries in tprocio dll.
  if (bPrepareQueries)
    lpprStarsIOInit(sDBAlias, sMode, sType, lAsofDate, PREPQRY_TRANSENGINE,
                    &zErr);
  else
    lpprStarsIOInit(sDBAlias, sMode, sType, lAsofDate, PREPQRY_NONE, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  // bTProcInitialized = true;
  // lLastDate = lAsofDate;
  // strcpy_s(sLastAlias, sDBAlias);
  // strcpy_s(sLastMode, sMode);
  // strcpy_s(sLastType, sType);

  return zErr;
} // InitTranproc

/**
** Function to initialize Dtransdesc structure defined in dtransdesc.h file
**/
void STDCALL InitializeDtransDesc(DTRANSDESC *pzDTranDesc) {
  pzDTranDesc->iID = 0;
  pzDTranDesc->lDtransNo = 0;
  pzDTranDesc->iSeqno = 0;
  sprintf_s(pzDTranDesc->sCloseType, "%-*s",
            (int)(sizeof(pzDTranDesc->sCloseType) - 1), "");
  pzDTranDesc->lTaxlotNo = 0;
  pzDTranDesc->fUnits = 0;
  sprintf_s(pzDTranDesc->sDescInfo, "%-*s",
            (int)(sizeof(pzDTranDesc->sDescInfo) - 1), "");
} /* initdtransdesc */

/**
** Function to initialize TRANSINFO structure
**/
void InitializeTransInfo(TRANSINFO *pzTInfo) {
  InitializeTransStruct(&pzTInfo->zTrans);

  pzTInfo->bAsOfFlag = FALSE;
  pzTInfo->bReversable = FALSE;
  pzTInfo->bRebookFlag = FALSE;
  memset(&pzTInfo->zTranType, 0, sizeof(pzTInfo->zTranType));
  pzTInfo->sRevTranCode[0] = '\0';
  pzTInfo->sSellAcctType[0] = '\0';
  pzTInfo->lDtransNo = pzTInfo->lBlockTransNo = 0;
}

/**
** Function to initialize DYNAMICPTABLE structure
**/
void InitializePTable(PTABLE *pzDPTable) {
  if (pzDPTable->iSize != 0 && pzDPTable->pzTInfo != NULL)
    free(pzDPTable->pzTInfo);

  pzDPTable->pzTInfo = NULL;
  pzDPTable->iSize = 0;
  pzDPTable->iCount = 0;
}

/**
** This function will retrieve the assets, trantype and sec_type records
** for calling the tranalloc function
*/
ERRSTRUCT PrepareForTranAlloc(TRANTYPE *pzTranType, ASSETS *pzAssets,
                              SECTYPE *pzSecType, TRANS zTrans, BOOL bAllFlag) {
  ERRSTRUCT zErr;
  char sMsg[80];

  InitializeErrStruct(&zErr);

  lpprSelectTrantype(pzTranType, zTrans.sTranType, zTrans.sDrCr, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError) {
    sprintf_s(sMsg, "Error Reading **%s**, **%s** From Trantype Table",
              zTrans.sTranType, zTrans.sDrCr);
    return (PrintError(sMsg, zTrans.iID, zTrans.lDtransNo, "D",
                       zErr.iBusinessError, zErr.iSqlError, zErr.iIsamCode,
                       "TPROC SELECTTRANTYPE", FALSE));
  }

  /*
   ** If the boolean variable bAllFlag is set to FALSE, return.  This mean
   ** that there is no need to retrieve the assets and sec type information
   ** because they have not changed since the previous retrieval
   */
  if (bAllFlag == FALSE)
    return zErr;

  zErr = SelectAsset(pzAssets, zTrans.sSecNo, -1, zTrans.sWi);

  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  lpprSelectSectype(pzSecType, pzAssets->iSecType, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError) {
    sprintf_s(sMsg, "Error Reading **%d** From Sectype Table",
              pzAssets->iSecType);
    zErr = PrintError(sMsg, zTrans.iID, zTrans.lDtransNo, "D",
                      zErr.iBusinessError, zErr.iSqlError, zErr.iIsamCode,
                      "TPROC SELECT SECTYPE", FALSE);
  }

  return zErr;
} /* PrepareForTranAlloc */

/**
** Function to select an asset record for the given security
**/
ERRSTRUCT SelectAsset(ASSETS *pzAssets, char *sSecNo, int iVendorID,
                      char *sWhenIssue) {
  ERRSTRUCT zErr;
  char sMsg[80];

  InitializeErrStruct(&zErr);

  sprintf_s(sMsg, "Error Reading **%s**, **%s** From Assets Table", sSecNo,
            sWhenIssue);
  lpprSelectAsset(pzAssets, sSecNo, sWhenIssue, -1, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    zErr = PrintError(sMsg, 0, 0, "", zErr.iBusinessError, zErr.iSqlError,
                      zErr.iIsamCode, "TPROC SELECTASSETS", FALSE);

  return zErr;
} /* SelectAsset */

char BoolToChar(BOOL bValue) {
  if (bValue)
    return 'Y';
  else
    return 'N';
} // BoolToChar

void FreeTranProc() {
  if (bTProcInitialized) {
    FreeLibrary(hOledbIODll);
    FreeLibrary(hStarsUtilsDll);
    FreeLibrary(hGainLossDll);
    // FreeLibrary(hStarsIODll);
    FreeLibrary(hDelphiCInterfaceDll);

    bTProcInitialized = FALSE;
  }
} // FreeTranProc