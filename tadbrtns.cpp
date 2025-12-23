/**
 *
 * SUB-SYSTEM: TranAlloc
 *
 * FILENAME: tadbrtns.ec
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
 * $Header:   J:/Performer/Data/archives/Master/C/TADBRTNS.C-arc   1.21   22 Mar
 * 2004 14:04:04   vay  $
 *
 **/

// HISTORY
// 2009-03-04 VI# 41539: added accretion functionality - mk
// 2007-11-08 Added Vendor id    - yb
// 3/22/2004 - Made POSINFOTABLE a dynamic rather than static array  - vay

#include "transdll.h"

/**
** Function to initialize Portmain structure defined in portdir.h file
**/
void STDCALL InitializePortmainStruct(PORTMAIN *pzPortmain) {
  pzPortmain->iID = 0;
  sprintf_s(pzPortmain->sUniqueName, "%-*s",
            (int)(sizeof(pzPortmain->sUniqueName) - 1), "");
  sprintf_s(pzPortmain->sAbbrev, "%-*s", (int)(sizeof(pzPortmain->sAbbrev) - 1),
            "");
  sprintf_s(pzPortmain->sDescription, "%-*s",
            (int)(sizeof(pzPortmain->sDescription) - 1), "");
  pzPortmain->lDateHired = 0;
  pzPortmain->fIndividualMinAnnualFee = pzPortmain->fIndividualMinAcctSize = 0;
  pzPortmain->fTotalAssetsManaged = 0;
  pzPortmain->iInvestmentStyle = pzPortmain->iScope =
      pzPortmain->iDecisionMaking = 0;
  pzPortmain->iDefaultReturnType = pzPortmain->iProductType = 0;
  pzPortmain->fExpenseRatio = 0;
  pzPortmain->iMarketCap = pzPortmain->iMaturity = 0;
  pzPortmain->lAsofDate = 0;
  pzPortmain->iFiscalYearEndMonth = pzPortmain->iFiscalYearEndDay = 0;
  sprintf_s(pzPortmain->sPeriodType, "%-*s",
            (int)(sizeof(pzPortmain->sPeriodType) - 1), "");
  pzPortmain->lInceptionDate = 0;
  pzPortmain->bUserInceptionDate = FALSE;
  sprintf_s(pzPortmain->sPortfolioType, "%-*s",
            (int)(sizeof(pzPortmain->sPortfolioType) - 1), "");
  sprintf_s(pzPortmain->sAdministrator, "%-*s",
            (int)(sizeof(pzPortmain->sAdministrator) - 1), "");
  sprintf_s(pzPortmain->sManager, "%-*s",
            (int)(sizeof(pzPortmain->sManager) - 1), "");
  sprintf_s(pzPortmain->sAddress1, "%-*s",
            (int)(sizeof(pzPortmain->sAddress1) - 1), "");
  sprintf_s(pzPortmain->sAddress2, "%-*s",
            (int)(sizeof(pzPortmain->sAddress2) - 1), "");
  sprintf_s(pzPortmain->sAddress3, "%-*s",
            (int)(sizeof(pzPortmain->sAddress3) - 1), "");
  sprintf_s(pzPortmain->sAcctMethod, "%-*s",
            (int)(sizeof(pzPortmain->sAcctMethod) - 1), "");
  sprintf_s(pzPortmain->sTax, "%-*s", (int)(sizeof(pzPortmain->sTax) - 1), "");
  sprintf_s(pzPortmain->sBaseCurrId, "%-*s",
            (int)(sizeof(pzPortmain->sBaseCurrId) - 1), "");
  pzPortmain->bIncome = pzPortmain->bActions = FALSE;
  pzPortmain->bMature = pzPortmain->bCAvail = pzPortmain->bFAvail = FALSE;
  sprintf_s(pzPortmain->sAlloc, "%-*s", (int)(sizeof(pzPortmain->sAlloc) - 1),
            "");
  pzPortmain->fMaxEqPct = pzPortmain->fMaxFiPct = pzPortmain->fMinCashPct = 0;
  pzPortmain->iEqLotSize = pzPortmain->iFiLotSize = 0;
  pzPortmain->lValDate = pzPortmain->lDeleteDate = 0;
  pzPortmain->bIsInactive = FALSE;
  sprintf_s(pzPortmain->sCurrHandler, "%-*s",
            (int)(sizeof(pzPortmain->sCurrHandler) - 1), "");
  pzPortmain->bAmortMuni = pzPortmain->bAmortOther = pzPortmain->bAccreteDisc =
      FALSE;
  pzPortmain->bAccretMuni = pzPortmain->bAccretOther = FALSE;
  pzPortmain->lAmortStartDate = 0;
  pzPortmain->bIncByLot = pzPortmain->bDiscretionaryAuthority = FALSE;
  pzPortmain->bVotingAuthority = pzPortmain->bSpecialArrangements = FALSE;
  pzPortmain->iIncomeMoneyMarketFund = pzPortmain->iPrincipalMoneyMarketFund =
      FALSE;
  sprintf_s(pzPortmain->sIncomeProcessing, "%-*s",
            (int)(sizeof(pzPortmain->sIncomeProcessing) - 1), "");
  pzPortmain->lPricingEffectiveDate = pzPortmain->lLastTransNo =
      pzPortmain->lPurgeDate = 0;
  pzPortmain->lLastActivity = pzPortmain->lRollDate = 0;
} /* initportdir */

/**
** This function selects a portdir record from the table and then increments
** the LastTransNo in it (in most of the cases, this is used as new transno for
** the transaction). If UpdateHold is successful in processing the current
** transaction, it writes back this incremented lLastTransNo to the portdir
**/
ERRSTRUCT IncrementPortmainLastTransNo(PORTMAIN *pzPR, int iID) {
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  lpprSelectPortmain(pzPR, iID, &zErr);
  if (zErr.iSqlError)
    return (PrintError("Error Reading PORTMAIN", iID, 0, "", 0, zErr.iSqlError,
                       zErr.iIsamCode, "TPROC INCPORTMAINLASTTRANS", FALSE));

  /* Increment the last_trans_no column by 1 */
  pzPR->lLastTransNo++;

  return zErr;
} /* IncrementPortmainLastTransNo */

/**
** This function will the put call indicator from the derivatives
** table
*/
ERRSTRUCT GetCallPut(TRANS zTrans, char *sCallPut) {
  ERRSTRUCT zErr;
  char sRecType[2];
  long lRecNo;

  InitializeErrStruct(&zErr);

  if (zTrans.lDtransNo != 0) {
    lRecNo = zTrans.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTrans.lTransNo;
    strcpy_s(sRecType, "T");
  }

  lpprSelectCallPut(sCallPut, zTrans.sSecNo, zTrans.sWi, &zErr);
  if (zErr.iSqlError)
    return (PrintError("Error Reading DERIVAT", 0, 0, "", 0, zErr.iSqlError,
                       zErr.iIsamCode, "TALOC SELECTCALLPUT", FALSE));

  return zErr;

} // GetCallPut

/**
** This function will retrieve the assets, trantype and sec_type records
** for calling the tranalloc function
*/
ERRSTRUCT GetNewSecurityInfo(TRANS zTrans, ASSETS *pzAssets, SECTYPE *pzSecType,
                             TRANTYPE *pzTranType) {
  ERRSTRUCT zErr;
  char sMsg[80], sRecType[2];
  long lRecNo;

  InitializeErrStruct(&zErr);

  if (zTrans.lDtransNo != 0) {
    lRecNo = zTrans.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTrans.lTransNo;
    strcpy_s(sRecType, "T");
  }

  //	lpprTimer(29);
  lpprSelectTrantype(pzTranType, zTrans.sTranType, zTrans.sDrCr, &zErr);
  //	lpprTimer(30);
  if (zErr.iSqlError) {
    sprintf_s(sMsg, "Error Reading Trantype Table For TType - %s, DrCr - %s",
              zTrans.sTranType, zTrans.sDrCr);
    return (PrintError(sMsg, 0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                       "TALOC SELECT TRANTYPE", FALSE));
  }

  //	lpprTimer(31);
  lpprSelectAsset(pzAssets, zTrans.sSecNo, zTrans.sWi, -1, &zErr);
  //	lpprTimer(32);
  if (zErr.iSqlError != 0) {
    sprintf_s(sMsg, "Error Reading Assets - %s", zTrans.sSecNo);
    return (PrintError(sMsg, 0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                       "TALOC SELECT ASSET", FALSE));
  }

  //	lpprTimer(33);
  lpprSelectSectype(pzSecType, pzAssets->iSecType, &zErr);
  //	lpprTimer(34);
  if (zErr.iSqlError) {
    sprintf_s(sMsg, "Error Reading Sectype - %d", pzAssets->iSecType);
    return (PrintError(sMsg, 0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                       "TALOC SELECT SECTYPE", FALSE));
  }

  return zErr;

} /* GetNewSecurityInfo */

/**
** function to initialize tradeinfo structure
**/
void InitializeTradeInfoStruct(TRADEINFO *pzTI) {
  pzTI->fUnits = pzTI->fOrigFace = pzTI->fPcplAmt = pzTI->fNetComm =
      pzTI->fSecFees = 0;
  pzTI->fMiscFee1 = pzTI->fMiscFee2 = pzTI->fAccrInt = pzTI->fIncomeAmt =
      pzTI->fTotCost = 0;
} /* initializeTradeinfostruct */

/**
** function to initialize positioninfo structure
**/
void InitializePositionInfoStruct(POSINFO *pzPV) {
  pzPV->fShares = pzPV->fOrigFace = pzPV->fUnitCost = pzPV->fTotCost =
      pzPV->fOrigCost = 0;
  pzPV->fOpenLiability = pzPV->fBaseCostXrate = pzPV->fSysCostXrate =
      pzPV->fOrgYield = 0;
  pzPV->fMatPrice = pzPV->fCollateralUnits = pzPV->fSharesRemoved =
      pzPV->fSharesToRemove = 0;
  pzPV->fUnitCostRound = 0;
  pzPV->lLot = 0;
  pzPV->lTradeDate = pzPV->lSettleDate = pzPV->lMatDate = 0;

} /* initializepositioninfostruct */

/**
** Function to initialize, add items and free POSINFOTABLE
**/
ERRSTRUCT InitializePosInfoTable(POSINFOTABLE *pzPITable, int iSize) {
  int i, iStart;
  ERRSTRUCT zErr;
  InitializeErrStruct(&zErr);

  if (iSize > 0) // if requesting table expansion
  {
    // If the table is full, create more space
    if (pzPITable->iCount == pzPITable->iSize) {
      pzPITable->iSize += iSize;
      pzPITable->pzPInfo = (POSINFO *)realloc(
          pzPITable->pzPInfo, sizeof(POSINFO) * pzPITable->iSize);
      if (pzPITable->pzPInfo ==
          NULL) { // error allocating more memory - exit reporting problem
        pzPITable->iSize = pzPITable->iCount = 0;
        zErr = PrintError("Insufficient Memory To Create Table", 0, 0, "", 997,
                          0, 0, "TALLOC InitializePosInfoTable", FALSE);
      } else { // memory reallocated, initialize array
        iStart = pzPITable->iCount;
        if (iStart > 0)
          iStart++;

        for (i = iStart; i < pzPITable->iSize; i++)
          InitializePositionInfoStruct(&pzPITable->pzPInfo[i]);
      }
    }
  } else // requesting table destruction
  {
    if (pzPITable->iSize != 0 && pzPITable->pzPInfo != NULL)
      free(pzPITable->pzPInfo);

    pzPITable->pzPInfo = NULL;
    pzPITable->iSize = pzPITable->iCount = 0;
  }

  return zErr;
}

/**
** function to copy fields from Trans to TradeInfo structure
**/
void CopyToTradeInfoFromTrans(TRANS zTrans, TRADEINFO *pzTI) {
  InitializeTradeInfoStruct(pzTI);
  pzTI->fUnits = zTrans.fUnits;
  pzTI->fOrigFace = zTrans.fOrigFace;
  pzTI->fPcplAmt = zTrans.fPcplAmt;
  pzTI->fNetComm = zTrans.fNetComm;
  pzTI->fSecFees = zTrans.fSecFees;
  pzTI->fMiscFee1 = zTrans.fMiscFee1;
  pzTI->fMiscFee2 = zTrans.fMiscFee2;
  pzTI->fAccrInt = zTrans.fAccrInt;
  pzTI->fIncomeAmt = zTrans.fIncomeAmt;
  pzTI->fOptPrem = zTrans.fOptPrem;
  pzTI->fBasisAdj = zTrans.fBasisAdj;
  pzTI->fCommGcr = zTrans.fCommGcr;
  pzTI->fTotCost = zTrans.fTotCost; // required only for forwards
  /* strcpy_s(pzTI->sAcctMthd, zTrans.sAcctMthd);*/
} /*copytotradeinfofromtrans*/

/**
** function to copy fields from tradeinfo structure to trans
**/
void CopyToTransFromTradeInfo(TRADEINFO zTI, TRANS *pzTrans,
                              BOOL bCopyOrigFace) {
  pzTrans->fPcplAmt = zTI.fPcplAmt;
  pzTrans->fNetComm = zTI.fNetComm;
  pzTrans->fSecFees = zTI.fSecFees;
  pzTrans->fMiscFee1 = zTI.fMiscFee1;
  pzTrans->fMiscFee2 = zTI.fMiscFee2;
  pzTrans->fAccrInt = zTI.fAccrInt;
  pzTrans->fIncomeAmt = zTI.fIncomeAmt;
  pzTrans->fOptPrem = zTI.fOptPrem;
  pzTrans->fBasisAdj = zTI.fBasisAdj;
  pzTrans->fCommGcr = zTI.fCommGcr;
  if (bCopyOrigFace)
    pzTrans->fOrigFace = zTI.fOrigFace;
  /*  strcpy_s(pzTrans->sAcctMthd, zTI.sAcctMthd);*/
} /* copytotransfromtradeinfo */

/**
** Function to initialize dynamic TRANSTABLE2 structure
**/
void STDCALL InitializeTransTable2(TRANSTABLE2 *pzTTable2) {
  if (pzTTable2->iSize != 0 && pzTTable2->pzTrans != NULL)
    free(pzTTable2->pzTrans);

  pzTTable2->pzTrans = NULL;
  pzTTable2->iSize = 0;
  pzTTable2->iCount = 0;
} // InitializeTransTable2

/**
** This function adds the given Transaction to the passed Dynamic TransTable2.
** If the table is full, it uses realloc to allocate more space before adding
* trans
*/
ERRSTRUCT STDCALL AddTransToTransTable2(TRANSTABLE2 *pzTTable2, TRANS zTrans) {
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  // If the table is full, create more space
  if (pzTTable2->iCount == pzTTable2->iSize) {
    pzTTable2->iSize += NUMEXTRAELEMENTS;
    pzTTable2->pzTrans =
        (TRANS *)realloc(pzTTable2->pzTrans, sizeof(TRANS) * pzTTable2->iSize);
    if (pzTTable2->pzTrans == NULL)
      return (PrintError("Insufficient Memory To Create Table", 0, 0, "", 997,
                         0, 0, "TALLOC ADDTRANSTOTRANSTABLE2", FALSE));
  }

  pzTTable2->pzTrans[pzTTable2->iCount++] = zTrans;

  return zErr;
} // AddTransToTransTable2