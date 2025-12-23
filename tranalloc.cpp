/**
*
* SUB-SYSTEM: TranAlloc
*
* FILENAME: TranAlloc.c
*
* DESCRIPTION: This routine accepts transactions(from "TranProc" and "Pricing")
*              for posting. It assigns transaction numbers, taxlot numbers,
*              and cost basis and then passes it to UpdateHold.
*
* PUBLIC FUNCTION(S): TranAlloc
*
* NOTES:
*
* USAGE: int TranAlloc(TRANS zTrans, TRANTYPE zTranType, SECTYPE zSecType,
*                      ASSETS zAssets, DTRANSDESC zDTranDesc[],
*                      int iNumDTItems, char *ssPrior)
*         zTrans      is the transaction to be processed
*         zTranType   is the Tran Type record of the transaction type
*         zSecType    is the Security Type record of the security
*         zAssets     is the assets record of the security
*         zDTranDesc  is the dtrasndesc information used for closing trades

*         iNumDTItems is the number of dtransdesc items
*         sCurrPrior  is the flag telling us whether the program is being
*                     run for currenmt date or prior
*
* AUTHOR: John T Griffin (Effron Enterprises, Inc.)
*
* $Header:   J:/Performer/Data/archives/Master/C/tranalloc.c-arc   1.67   27 Jan
2005 16:16:02   YegorovV  $
*
**/
// HISTORY
// 2018-07-31 VI# 61746 Added ability to calculate cost in transengine and G\L
// in calcgainloss for Futures, with secondary type N -mk 2018-03-06 VI# 60490:
// added code to adjust logic for total cost and G\L on futures -mk. 2013-12-11
// VI# 54307: initialize zTIPSAdjustments.iSize=0 - sergeyn 2011-05-10 VI#
// 46071: increase precision for unit cost - sergeyn 2009-03-17 VI# 41539: fixed
// sideeffect to accretion functionality - mk 2009-03-04 VI# 41539: added
// accretion functionality - mk 2007-11-08 Added Vendor id    - yb 3/22/2004 -
// Made POSINFOTABLE a dynamic rather than static array  - vay 3/18/2004 - Don't
// put descriptions on PI/BA - vay

#include "transengine.h"
#include "winbase.h"

/**
** Function to assign transaction numbers, taxlot numbers,
** cost basis info, and pass the transaction to UPDHOLD for posting
**/

DLLAPI ERRSTRUCT STDCALL WINAPI TranAlloc(TRANS *pzTrans, TRANTYPE zTranType,
                                          SECTYPE zSecType, ASSETS zAssets,
                                          DTRANSDESC zDTransDesc[],
                                          int iNumDTItems, PAYTRAN *pzPayTran,
                                          char *sCurrPrior,
                                          BOOL bDoTransaction) {
  ERRSTRUCT zErr, zDeleteErr;
  int i;

  InitializeErrStruct(&zErr);

  bDoTransaction = bDoTransaction && (lpfnGetTransCount() == 0);
  if (bDoTransaction) // if TranAlloc is responsible for DB transactions - start
                      // new one here
  {
    zErr.iBusinessError = lpfnStartTransaction();
    if (zErr.iBusinessError != 0)
      return zErr;
  }

  // Lock the portfolio, by putting an entry in portlock, if a duplicate key
  // error occurs, wait for 5 seconds and try agai, if even after 6 tries(total
  // of 30 seconds), portfolio is still locked, return with an error.
  for (i = 0; i < 6; i++) {
    if (lpfnInsertPortLock(pzTrans->iID, pzTrans->sCreatedBy, &zErr) ==
        9729) // Duplicated Key which means the portfolio is locked
    {
      zErr.iBusinessError = 135;
      Sleep(500); // wait for 500 milisecond (5 seconds)
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      if (bDoTransaction)
        lpfnRollbackTransaction();

      return zErr;
    } else {
      InitializeErrStruct(&zErr);
      break;
    }
  }
  if (zErr.iBusinessError == 135) {
    PrintError("Portfolio locked by another user", pzTrans->iID,
               pzTrans->lTransNo, "T", 135, 0, 0, "TALOC - PORTLOCK", FALSE);
    if (bDoTransaction)
      lpfnRollbackTransaction();

    return zErr;
  }

  __try {
    zErr = DoTranAlloc(pzTrans, zTranType, zSecType, zAssets, zDTransDesc,
                       iNumDTItems, pzPayTran, sCurrPrior, FALSE);
    // in any case unlock the portfolio
    lpprDeletePortlock(pzTrans->iID, &zDeleteErr);
  } __except (lpfnAbortTransaction(bDoTransaction)) {
  }

  if (bDoTransaction) // if TranAlloc is responsible for DB transactions -
                      // commit/rollback it here
  {
    if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
      lpfnCommitTransaction();
    else
      lpfnRollbackTransaction();
  }
  return zErr;
}

ERRSTRUCT DoTranAlloc(TRANS *pzTrans, TRANTYPE zTranType, SECTYPE zSecType,
                      ASSETS zAssets, DTRANSDESC zDTransDesc[], int iNumDTItems,
                      PAYTRAN *pzPayTran, char *sCurrPrior,
                      BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  BOOL bExerciseFlag;
  long lRecNo;
  char sRecType[2];
  double fExerciseUnits;

  InitializeErrStruct(&zErr);
  fExerciseUnits = 0;

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  bExerciseFlag = FALSE;

  /*
  ** All transactions are processed depending on the setting of the
  ** TranCode column in the TranType table
  ** I - Income Transactions
  ** M - Money Transactions
  ** O - Opening Transactions
  ** C - Closing Transactions
  ** A - Adjustment Transactions
  ** S - Split Transactions
  ** X - Transfer Transactions
  ** R - Reversal Transactions
  */
  switch (zTranType.sTranCode[0]) {
  case 'I':
    return (ProcessIncomeInTranAlloc(pzTrans, zDTransDesc, iNumDTItems,
                                     sCurrPrior, bDoTransaction));

  case 'M':
    return (ProcessMoneyInTranAlloc(pzTrans, zDTransDesc, iNumDTItems,
                                    pzPayTran, sCurrPrior, bDoTransaction));

  case 'O':
    zErr = ProcessOpenInTranAlloc(
        pzTrans, zSecType, zAssets, zTranType, zDTransDesc, iNumDTItems,
        pzPayTran, &bExerciseFlag, sCurrPrior, &fExerciseUnits, bDoTransaction);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
    /*
    ** If the exercise flag is set to true, it is necessary
    ** to generate an 'ex' trade
    */
    if (bExerciseFlag)
      zErr = ExerciseOptionInTranAlloc(pzTrans, fExerciseUnits, zTranType,
                                       zSecType, zAssets, zDTransDesc,
                                       iNumDTItems, sCurrPrior, bDoTransaction);
    return zErr;

  case 'C':
    zErr = ProcessCloseInTranAlloc(
        pzTrans, zTranType, zSecType, zAssets, zDTransDesc, iNumDTItems,
        pzPayTran, &bExerciseFlag, sCurrPrior, &fExerciseUnits, bDoTransaction);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /*
    ** If the exercise flag is set to true, it is necessary
    ** to generate an 'ex' trade
    */
    if (bExerciseFlag)
      zErr = ExerciseOptionInTranAlloc(pzTrans, fExerciseUnits, zTranType,
                                       zSecType, zAssets, zDTransDesc,
                                       iNumDTItems, sCurrPrior, bDoTransaction);
    return zErr;

  case 'A':
    return (ProcessAdjustInTranAlloc(pzTrans, zDTransDesc, iNumDTItems,
                                     sCurrPrior, bDoTransaction));

  case 'S':
    return (ProcessSplitInTranAlloc(pzTrans, zDTransDesc, iNumDTItems,
                                    sCurrPrior, bDoTransaction));

  case 'X':
    return (ProcessTransferInTranAlloc(pzTrans, zTranType, zSecType, zAssets,
                                       zDTransDesc, iNumDTItems, sCurrPrior,
                                       bDoTransaction));

  case 'R':
    return (ProcessCancelInTranAlloc(pzTrans, zDTransDesc, iNumDTItems,
                                     sCurrPrior, bDoTransaction));

  default:
    return (PrintError("Invalid TranCode", pzTrans->iID, lRecNo, sRecType, 103,
                       0, 0, "TALOC - TRANCODE SWITCH", FALSE));
  } /*End Switch */
} /* TranAloc */

/**
** Function to process income type of transaction
**/
ERRSTRUCT ProcessIncomeInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                   int iNumDTItems, char *sCurrPrior,
                                   BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  long lRecNo;
  char sRecType[2];

  InitializeErrStruct(&zErr);

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  /*
  ** Check that the principal amount column on the transaction contains a value,
  ** if the value is zero, generate an error message and fail the trade
  */
  /* SB - 1/19/99 No checks anymore
        if (pzTrans->fPcplAmt == 0)
    return(PrintError("Invalid Principal Amount", pzTrans->iID, lRecNo,
                      sRecType, 26, 0, 0, "TALOC - INCOME1", FALSE));*/

  /*
  ** Check that the security number extension column does not contain
  ** a 'TS' or 'TL' value, if so generate an error message and fail the trade
  */
  if (strcmp(pzTrans->sSecXtend, "TL") == 0 ||
      strcmp(pzTrans->sSecXtend, "TS") == 0)
    return (PrintError("Invalid Security Extention", pzTrans->iID, lRecNo,
                       sRecType, 107, 0, 0, "TALOC - INCOME2", FALSE));

  /*
  ** commented out: 11/4/97 - will only be valid if portfolio pays
  ** by lot - waiting for portdir to be change before reactivating
  **
  ** Check if the transaction is an 'RI', 'RD', 'WH', 'AR' or 'RR' - these
  ** transactions are automatically generated by the pricing function and must
  ** carry a tax lot number. If the taxlot number is zero, fail the transaction
  if ((strcmp(pzTrans->sTranType, "RD") == 0 ||
       strcmp(pzTrans->sTranType, "RI") == 0 ||
       strcmp(pzTrans->sTranType, "WH") == 0 ||
       strcmp(pzTrans->sTranType, "AR") == 0 ||
       strcmp(pzTrans->sTranType, "RR") == 0) && pzTrans->lTaxlotNo == 0)
    return(PrintError("Invalid Tax Lot Number", pzTrans->iID, lRecNo,
                      sRecType, 51, 0, 0, "TALOC - INCOME3", FALSE));

  */

  /*
  ** Increase the last trans no in portdir file and assign that number to
  ** TransNo in Trans record
  */
  zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
  if (zErr.iSqlError != 0)
    return zErr;

  pzTrans->lTransNo = zPortmain.lLastTransNo;

  /* Pass the transaction to "updhold" for final processing */
  zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, NULL,
                    sCurrPrior, bDoTransaction);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior, bDoTransaction,
                                ADDANDWRITEVALUE, TRUE);

  return zErr;
} /* End of Income type processing */

/**
** Function to process Money type of transactions
**/
ERRSTRUCT ProcessMoneyInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                  int iNumDTItems, PAYTRAN *pzPayTran,
                                  char *sCurrPrior, BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  char sRecType[2];
  long lRecNo;

  InitializeErrStruct(&zErr);

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  /*
  ** Check that the principal amount column on the transaction contains
  ** a value, if the value is zero, generate an error message and fail the trade
  */
  if (IsValueZero(pzTrans->fPcplAmt, 2) && IsValueZero(pzTrans->fIncomeAmt, 2))
    return (PrintError("Invalid Principal Amount", pzTrans->iID, lRecNo,
                       sRecType, 26, 0, 0, "TALOC - MONEY1", FALSE));

  /*
  ** Increase the last trans no in portdir file and assign that number to
  ** TransNo in Trans record
  */
  zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
  if (zErr.iSqlError != 0)
    return zErr;

  pzTrans->lTransNo = zPortmain.lLastTransNo;

  /* Pass the transaction to "updhold" for final processing */
  zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, pzPayTran,
                    sCurrPrior, bDoTransaction);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior, bDoTransaction,
                                ADDANDWRITEVALUE, TRUE);

  return zErr;
} /* End of Money type processing */

/**
** Function to process Open type of transaction
**/
ERRSTRUCT ProcessOpenInTranAlloc(TRANS *pzTrans, SECTYPE zSecType,
                                 ASSETS zAssets, TRANTYPE zTranType,
                                 DTRANSDESC zDTransDesc[], int iNumDTItems,
                                 PAYTRAN *pzPayTran, BOOL *bExerciseFlag,
                                 char *sCurrPrior, double *pfExerciseUnits,
                                 BOOL bDoTransaction) {
  double fGrossProceeds, fOptPrem;
  int i;
  char sRecType[2];
  long lRecNo;
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  PARTFINC zPFinc;

  i = 0;
  fGrossProceeds = fOptPrem = 0;
  InitializeErrStruct(&zErr);

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  /*
  ** Check if the units on the transaction are zero, if this is the case
  ** return an error, otherwise continue processing
  */
  if (IsValueZero(pzTrans->fUnits, 5))
    return (PrintError("Invalid Units", pzTrans->iID, lRecNo, sRecType, 28, 0,
                       0, "TALOC - OPEN1", FALSE));

  /*
   ** Calculate the gross proceeds of the transaction, if the transaction is
   ** a purchase where
   ** Gross Proceeds   = Prncpl Amt - Commsn - SecFees - MiscFee1 - MiscFee2
   */
  if (zTranType.lCashImpact == -1)
    fGrossProceeds = pzTrans->fPcplAmt - pzTrans->fCommGcr - pzTrans->fSecFees -
                     pzTrans->fMiscFee1 - pzTrans->fMiscFee2;
  else if (zTranType.lCashImpact == 1)
    fGrossProceeds = pzTrans->fPcplAmt + pzTrans->fCommGcr + pzTrans->fSecFees +
                     pzTrans->fMiscFee1 + pzTrans->fMiscFee2;
  else if (zTranType.lCashImpact == 0) // Added on 7/22/99 (to calculate
                                       // UnitCost for FR transactions) - SB
    fGrossProceeds = pzTrans->fTotCost;

  /* If there is a nonzero cash impact, make sure principal amount is non zero*/
  // SB 1/27/1999 : As Per Leno this check is being removed
  /*if (zTranType.lCashImpact != 0 && zTranType.lCashImpact != 2 &&
      pzTrans->fPcplAmt == 0.0 && fGrossProceeds == 0.0)
    return(PrintError("Invalid Principal Amount", pzTrans->iID, lRecNo,
                                                         sRecType, 26, 0, 0,
    "TALOC - OPEN2", FALSE));*/

  /*
  ** If the security type is fixed income, check the original_face column
  ** in fixedinc(in assets db) to determine if the transaction should carry
  ** a value in it's original_face column.  If the fixedinc.original_face
  ** column is 'y' and the trans.original_face has 0 value, the trade fails
  ** otherwise continue processing
  */
  if (zSecType.sPrimaryType[0] == 'B') {
    //		lpprTimer(21);
    lpprSelectPartFixedinc(pzTrans->sSecNo, pzTrans->sWi, &zPFinc, &zErr);
    //		lpprTimer(22);
    if (zErr.iSqlError)
      return (PrintError("Error Selecting Original Face Drom FIXEDINC",
                         pzTrans->iID, lRecNo, sRecType, 0, zErr.iSqlError,
                         zErr.iIsamCode, "TALOC OPEN3", FALSE));

    if (zPFinc.sOriginalFace[0] == 'Y' && IsValueZero(pzTrans->fOrigFace, 3) &&
        IsValueZero(pzTrans->fUnits, 5))
      return (PrintError("Invalid Original Face", pzTrans->iID, lRecNo,
                         sRecType, 29, 0, 0, "TALOC OPEN4", FALSE));

  } /* if fixed income security */

  /*
  ** If there is no unit cost, calculate a new unit cost by using the following
  * formula:
  **   Unitcost = Gross Proceeds / Units, where
  **   Gross Proceeds   = Prncpl Amt - Commsn - SecFees - MiscFee1 - MiscFee2
  **                   OR Prncpl Amt + Commsn + SecFees + MiscFee1 + MiscFee2
  **                   OR Total Cost
  **   Units = Units * TradUnit
  ** If the currency id does not match security currency id, then cost
  ** is first converted into security's local value and then divided by units
  */
  // SB 7/22/99 Even for CashImpact 0, calculate unitcost  if
  // (zTranType.lCashImpact != 0 && pzTrans->fUnitCost == 0)
  if (IsValueZero(pzTrans->fUnitCost, 6)) {
    if (IsValueZero(zAssets.fTradUnit, 3))
      return (PrintError("Invalid Trading Unit", pzTrans->iID, lRecNo, sRecType,
                         21, 0, 0, "TALOC OPEN5", FALSE));

    /*
    ** If the currency id does not match the currency id of the security
    ** convert the values to the security's local value
    */
    if (strcmp(pzTrans->sCurrId, pzTrans->sSecCurrId) != 0)
      pzTrans->fUnitCost =
          ((fGrossProceeds / pzTrans->fBaseXrate) * pzTrans->fSecBaseXrate) /
          (pzTrans->fUnits * zAssets.fTradUnit);
    else
      pzTrans->fUnitCost =
          fGrossProceeds / (pzTrans->fUnits * zAssets.fTradUnit);
  } /* if cashimpact is not zero and unit cost is zero */

  /*
  ** Check if the descriptive information contains any exercise information
  ** if it does, set the exercise flag to true
  */
  *bExerciseFlag = FALSE;
  for (i = 0; i < iNumDTItems; i++) /* # of DtransDesc items */
  {
    if (strcmp(zDTransDesc[i].sCloseType, "AE") == 0)
      *bExerciseFlag = TRUE;
  }

  // Calculate and assign the option premium
  if (*bExerciseFlag) {
    zErr = CalcOptionPremium(*pzTrans, pfExerciseUnits, &fOptPrem);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  }

  // If the short sale, invert the value of the option premium
  if (zTranType.lCashImpact == 1)
    fOptPrem *= -1;
  pzTrans->fOptPrem = fOptPrem;

  /*
  ** If the transaction is purchase (cash impact not = zero) and if the total
  ** cost column is zero, assign the principal value to the total cost column
  */
  if (IsValueZero(pzTrans->fTotCost, 3) && zTranType.lCashImpact != 0) {
    /*
    ** If the security's currency id matches the principal's currency
    ** id, then add the option premium amount to the principal amount
    ** Option premium's are always quoted in terms of the security's currency
    */
    if (strcmp(pzTrans->sCurrId, pzTrans->sSecCurrId) == 0)
      pzTrans->fTotCost = pzTrans->fPcplAmt + pzTrans->fOptPrem;
    else
      /*
      ** If the currency ids do not match, convert the principal amount
      ** to the base currency terms, and then convert the interm amount
      ** to the security's local currency by multiplying by the
      ** security's base exchange rate
      */
      pzTrans->fTotCost =
          ((pzTrans->fPcplAmt / pzTrans->fBaseXrate) * pzTrans->fSecBaseXrate) +
          pzTrans->fOptPrem;
  } /* if purchase and total cost is zero */
  else if (IsValueZero(pzTrans->fTotCost, 3) &&
           zSecType.sPrimaryType[0] == PTYPE_FUTURE &&
           (zSecType.sSecondaryType[0] == STYPE_INDEX ||
            zSecType.sSecondaryType[0] == STYPE_FOREIGN)) {
    if (!IsValueZero(pzTrans->fPcplAmt, 2))
      pzTrans->fTotCost = pzTrans->fPcplAmt;
    else
      pzTrans->fTotCost = pzTrans->fUnits * pzTrans->fPrice * zAssets.fTradUnit;
  } else {
    pzTrans->fTotCost += pzTrans->fOptPrem;
    pzTrans->fOrigCost += pzTrans->fOptPrem;
  }

  /*
  ** If the transaction is purchase (cash impact not = zero) and if the
  * orig_cost
  ** is zero, make it equal to tot_cost
  */
  if (IsValueZero(pzTrans->fOrigCost, 3) && zTranType.lCashImpact != 0)
    pzTrans->fOrigCost = pzTrans->fTotCost;

  /*
  ** Increase the last trans no in portdir file and assign that number to
  ** TransNo in Trans record
  */
  //	lpprTimer(23);
  zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
  //	lpprTimer(24);
  if (zErr.iSqlError != 0)
    return zErr;

  pzTrans->lTransNo = zPortmain.lLastTransNo;

  /*
  ** If it's a cash security, taxlot number is zero(holdcash file does not have
  ** a trans no field), else if it's a single lot security and record for
  ** that security exist (in holdings file), use holdings lot number for the
  ** taxlot number, otherwise always use transaction number for the taxlot #.
  */
  if (zSecType.sPositionInd[0] == 'C')
    pzTrans->lTaxlotNo = 0;
  else if (zSecType.sLotInd[0] == 'S') /* single lot security */
    pzTrans->lTaxlotNo =
        DEFAULTMMLOT; /* assign the number of existing taxlot */
  else
    pzTrans->lTaxlotNo = pzTrans->lTransNo;

  /* Pass the transaction to "updhold" for final processing */
  zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, pzPayTran,
                    sCurrPrior, bDoTransaction);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior, bDoTransaction,
                                ADDANDWRITEVALUE, TRUE);

  return zErr;
} /* End of Open type processing */

/**
** Function to process close type of transaction
**/
ERRSTRUCT STDCALL ProcessCloseInTranAlloc(TRANS *pzTrans, TRANTYPE zTranType,
                                          SECTYPE zSecType, ASSETS zAssets,
                                          DTRANSDESC zDTransDesc[],
                                          int iNumDTItems, PAYTRAN *pzPayTran,
                                          BOOL *bExerciseFlag, char *sCurrPrior,
                                          double *pfExerciseUnits,
                                          BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  TRADEINFO zTIOriginal, zTIRemaining; // Original and remaining Trade Info
  HOLDINGS zHoldings;
  PORTMAIN zPortmain;
  POSINFOTABLE zPosInfo;
  BOOL bMissingLot, bRecFound;
  PARTFINC zPFinc;
  int i, j;
  long lFirstTransNo, lRecNo, lOpenDate;
  char sRecType[2];
  double fTotalSharesRemoved, fRemainingSharesToRemove, fTotalUnits;
  double fGrossProceeds, fTotalCostRemoved, fOptPrem, fSellUnits, fUnitCost,
      fRoundCost;

  fGrossProceeds = fTotalSharesRemoved = fTotalCostRemoved = 0;
  fRemainingSharesToRemove = fOptPrem = fSellUnits = 0;
  bMissingLot = bRecFound = FALSE;

  InitializeErrStruct(&zErr);
  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  i = j = 0;
  /* If the units column contains a zero value, this is an error */
  if (IsValueZero(pzTrans->fUnits, 5))
    return (PrintError("Invalid Units", pzTrans->iID, lRecNo, sRecType, 28, 0,
                       0, "TALOC CLOSE1", FALSE));

  /*
  ** Calculate the gross proceeds of the transaction
  **   Gross Proceeds   = Prncpl Amt + Commsn + SecFees + MiscFee1 + MiscFee2
  *	For CshImpact 1 *
  *		OR	Prncpl Amt - Commsn - SecFees - MiscFee1 - MiscFee2
  *	For CshImpact -1 *
  *		OR  Tot Cost		For CshImpact 0
  */
  if (zTranType.lCashImpact == 1)
    fGrossProceeds = pzTrans->fPcplAmt + pzTrans->fCommGcr + pzTrans->fSecFees +
                     pzTrans->fMiscFee1 + pzTrans->fMiscFee2;
  else if (zTranType.lCashImpact == -1)
    fGrossProceeds = pzTrans->fPcplAmt - pzTrans->fCommGcr - pzTrans->fSecFees -
                     pzTrans->fMiscFee1 - pzTrans->fMiscFee2;
  else if (zTranType.lCashImpact == 0)
    fGrossProceeds = pzTrans->fTotCost;

  /*
   ** If there is a nonzero cash impact, make sure principal amount is non zero,
   ** the only exception are maturities of options which always expire
   ** for zero value
   */
  // SB 1/27/1999 : As Per Leno this check is being removed
  /*if (zSecType.sPrimaryType[0] != 'O')
  {
    if (zTranType.lCashImpact != 0 && zTranType.lCashImpact != 2 &&
        pzTrans->fPcplAmt == 0 && fGrossProceeds == 0)
      return(PrintError("Invalid Principal Amount", pzTrans->iID, lRecNo,
                        sRecType, 26, 0, 0, "TALOC CLOSE2", FALSE));
  }
  else if (strcmp(zTranType.sTranType, "MS") != 0 &&
           strcmp(zTranType.sTranType, "ML") != 0 &&
           zTranType.lCashImpact != 0 && zTranType.lCashImpact != 2 &&
           pzTrans->fPcplAmt == 0 && fGrossProceeds == 0)
     return(PrintError("Invalid Principal Amount", pzTrans->iID, lRecNo,
                       sRecType, 26, 0, 0, "TALOC CLOSE2A", FALSE));*/

  /*
  ** If the transaction is a sale (cash impact not zero) and unitcost has a
  ** value assigned, there is no action, else calculate a new unit
  ** cost by using the following formula:
  **   Unitcost = Cost / Units, where
  **   Gross Proceeds    = Prncpl Amt + Commsn + SecFees + MiscFee1 + MiscFee2
  **   Units = units * TradUnit
  ** If the currency id does not match security currency id, then cost
  ** is first converted into security's local value and then divided by units
  */
  // SB 7/22/99 - Calculate it even for Cash Impact 0 (FD)  if
  // (pzTrans->fUnitCost == 0 && zTranType.lCashImpact != 0)
  if (IsValueZero(pzTrans->fUnitCost, 6)) {
    if (IsValueZero(zAssets.fTradUnit, 3))
      return (PrintError("Invalid Trading Unit", pzTrans->iID, lRecNo, sRecType,
                         21, 0, 0, "TALOC - CLOSE3", FALSE));

    /*
    ** If the currency id does not match the currency id of the security
    ** convert the values to the security's local value
    */
    if (strcmp(pzTrans->sCurrId, pzTrans->sSecCurrId) != 0)
      pzTrans->fUnitCost =
          ((fGrossProceeds / pzTrans->fBaseXrate) * pzTrans->fSecBaseXrate) /
          (pzTrans->fUnits * zAssets.fTradUnit);
    else
      pzTrans->fUnitCost =
          fGrossProceeds / (pzTrans->fUnits * zAssets.fTradUnit);

    // SB 10/19/99 Round unit_cost to 6 decimal places
    pzTrans->fUnitCost = RoundDouble(pzTrans->fUnitCost, 6);
  } // if cashimpact is not zero and unit cost is zero

  /*
   ** If the security is a single lot security there is no checking of available
   ** positions, send trade straight through to UPDHOLD for processing
   */
  if (zSecType.sLotInd[0] == 'S') {
    pzTrans->fTotCost = pzTrans->fUnits;
    pzTrans->fOrigCost = pzTrans->fUnits;

    pzTrans->lTaxlotNo = DEFAULTMMLOT;
    //		lpprTimer(25);
    lpprSelectHoldings(&zHoldings, pzTrans->iID, pzTrans->sSecNo, pzTrans->sWi,
                       pzTrans->sSecXtend, pzTrans->sAcctType,
                       pzTrans->lTaxlotNo, &zErr);
    //		lpprTimer(26);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      bRecFound = FALSE;
    } else if (zErr.iSqlError != 0)
      return zErr;
    else
      bRecFound = TRUE;

    if (bRecFound) {
      pzTrans->fOpenUnitCost = zHoldings.fUnitCost;
      pzTrans->fBaseOpenXrate = zHoldings.fBaseCostXrate;
      pzTrans->fSysOpenXrate = zHoldings.fSysCostXrate;
      pzTrans->lOpenTrdDate = zHoldings.lTrdDate;
      pzTrans->lOpenStlDate = zHoldings.lStlDate;
    } else {
      pzTrans->fOpenUnitCost = pzTrans->fUnitCost;
      pzTrans->fBaseOpenXrate = pzTrans->fBaseXrate;
      pzTrans->fSysOpenXrate = pzTrans->fSysXrate;
      pzTrans->lOpenTrdDate = pzTrans->lTrdDate;
      pzTrans->lOpenStlDate = pzTrans->lStlDate;
    }

    // Increase the last trans no in portdir file and assign that number to
    // TransNo in Trans record
    zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
    if (zErr.iSqlError != 0)
      return zErr;

    pzTrans->lTransNo = zPortmain.lLastTransNo;
    pzTrans->lXrefTransNo = pzTrans->lTransNo;

    // Pass the transaction to "updhold" for final processing
    zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, pzPayTran,
                      sCurrPrior, bDoTransaction);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                  bDoTransaction, ADDANDWRITEVALUE, TRUE);

    return zErr;
  } // if single lot security

  /*
  ** If the transaction is not a 'pay-down', these do not carry original face
  ** and if the security type is fixed income, check the original_face column
  ** in fixedinc(in assets db) to determine if the transaction should carry
  ** a value in it's original_face column.
  */
  if (strcmp(pzTrans->sTranType, "PD") != 0 &&
      zSecType.sPrimaryType[0] == 'B') {
    lpprSelectPartFixedinc(pzTrans->sSecNo, pzTrans->sWi, &zPFinc, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (PrintError("Error Selecting Amort & Original Face From FIXEDINC",
                         pzTrans->iID, lRecNo, sRecType, 0, zErr.iSqlError,
                         zErr.iIsamCode, "TALOC - CLOSE4", FALSE));
  } /* if fixed income security */
  else {
    strcpy_s(zPFinc.sPayType, "N");
    strcpy_s(zPFinc.sOriginalFace, "N");
    // strcpy_s(zPFinc.sAmortFlag, "N");
    // strcpy_s(zPFinc.sAccretFlag, "N");
  }

  /*
   ** If the transaction is not an exericse, check if the descriptive
   ** information contains any exercise information. If it does, set the
   ** exercise flag to true
   */
  *bExerciseFlag = FALSE;
  if (strcmp(pzTrans->sTranType, "EX") != 0 &&
      strcmp(pzTrans->sTranType, "ES") != 0) {
    for (i = 0; i < iNumDTItems; i++) /* # of DtransDesc items */
    {
      if (strcmp(zDTransDesc[i].sCloseType, "AE") == 0)
        *bExerciseFlag = TRUE;
    }
  }

  /* Calculate and assign the option premium */
  if (*bExerciseFlag) {
    zErr = CalcOptionPremium(*pzTrans, pfExerciseUnits, &fOptPrem);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  }

  pzTrans->fOptPrem = fOptPrem;

  /* Initialize the position information table */
  memset(&zPosInfo, 0, sizeof(zPosInfo));
  zErr = InitializePosInfoTable(&zPosInfo, NUMEXTRAELEMENTS);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return zErr;
  }

  /*
  ** Save the original values, this information is used to verify
  ** totals and to provide trade information (dates, exchange rates, etc)
  ** to any sub transaction record that is created by this function
  */
  CopyToTradeInfoFromTrans(*pzTrans, &zTIOriginal);
  CopyToTradeInfoFromTrans(*pzTrans, &zTIRemaining);

  /* The total amount of units being sold/removed */
  fRemainingSharesToRemove = pzTrans->fUnits;

  /* Get the position information table filled in from current holdings table*/
  zErr = GetExistingPosition(*pzTrans, &zPosInfo, &fTotalUnits, zTranType,
                             zSecType, zPFinc.sOriginalFace, zAssets.fTradUnit);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return zErr;
  }

  /*
  ** Check the dtransdesc table for missing tax lot, if there are
  ** missing taxlots, call the decipher function to assign the ids
  ** Then  go through the dtransdesc table. For each entry in dtransdesc table,
  ** try to find the lot in position table(which was retrieved from current
  ** holdings). If the lot is found, mark the lot to be sold(either completely
  ** or partially)
  */
  if (strcmp(pzTrans->sTranType, "EX") != 0 &&
      strcmp(pzTrans->sTranType, "ES") != 0) {
    // SB 7/10/2002 Added following two lines to get rid of warning, these have
    // no logical effect
    fRoundCost = 0;
    lOpenDate = 0;
    for (i = 0; i < iNumDTItems; i++) /* # of DtransDesc Items */
    {
      /* If the close type is not 'VS' - skip */
      if (strcmp(zDTransDesc[i].sCloseType, "VS") != 0)
        continue;

      /*
      ** If the taxlot is zero and the trade is 'WE' or 'WI'
      ** tax lot ids have already been assigned, if this is not the
      ** case, generate an error message
      */
      if (zDTransDesc[i].lTaxlotNo == 0 &&
          (strcmp(pzTrans->sTranType, "WE") == 0 ||
           strcmp(pzTrans->sTranType, "WI") == 0)) {
        bMissingLot = TRUE;
        break;
      }

      if (zDTransDesc[i].lTaxlotNo == 0) {
        // SB - 7/22/99 If taxlot is not identified can not do Versus trade
        bMissingLot = TRUE;
        break;

        fSellUnits = fUnitCost = fRoundCost = 0;
        lOpenDate = 0;

        zErr = DecipherDescInfo(&lOpenDate, &fSellUnits, &fUnitCost,
                                zDTransDesc[i].sDescInfo, pzTrans->iID, lRecNo,
                                sRecType);
        if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
          /* Free the position information table */
          InitializePosInfoTable(&zPosInfo, 0);
          return zErr;
        }

        // Round Unit Cost to 6 decimal places
        fRoundCost = RoundDouble(fUnitCost, 6);
      } // if taxlot = 0

      bMissingLot = TRUE;

      /* Find the match */
      for (j = 0; j < zPosInfo.iCount; j++) /* # of positioninfo items */
      {
        // If the taxlot id is not assigned, search by unitcost and trade date,
        // else search by taxlot id
        if (zDTransDesc[i].lTaxlotNo == 0) {
          if ((zPosInfo.pzPInfo[j].fUnitCostRound == fRoundCost) &&
              (zPosInfo.pzPInfo[j].lTradeDate == lOpenDate)) {
            bMissingLot = FALSE;
            zDTransDesc[i].lTaxlotNo = zPosInfo.pzPInfo[j].lLot;
            zDTransDesc[i].fUnits = fSellUnits;
          } else
            continue; /* not same lot as dtransdesc info - continue */
        } else {
          if (zPosInfo.pzPInfo[j].lLot == zDTransDesc[i].lTaxlotNo)
            bMissingLot = FALSE;
          else
            continue;
        }

        /* same lot found in PosInfo table as in dtransdesc table */
        /*
        ** current lot size is equal to or larger that in dtransdesc info, so
        ** the current lot in PosInfo table may be only partially sold
        */
        if (zPosInfo.pzPInfo[j].fSharesToRemove >= zDTransDesc[i].fUnits) {
          zPosInfo.pzPInfo[j].fSharesToRemove -= zDTransDesc[i].fUnits;
          zPosInfo.pzPInfo[j].fSharesRemoved += zDTransDesc[i].fUnits;
          fRemainingSharesToRemove -= zDTransDesc[i].fUnits;
          fTotalSharesRemoved += zDTransDesc[i].fUnits;

          strcpy_s(zPosInfo.pzPInfo[j].sAcctMthd,
                   "V");                     /*mark the current lot as versus*/
          strcpy_s(pzTrans->sAcctMthd, "V"); /* mark the transaction as versus*/

          break;
        } else {
          bMissingLot = TRUE;
          zDTransDesc[i].lTaxlotNo = 0;
          zDTransDesc[i].fUnits = 0;
        }

        /*
        ** commented out: 11/17/97 to allow for 2 lots with same unitcost
        ** and trade check to work
        ** current lot size is smaller to that in dtransdesc info, so generate
        ** an error message
        else
                return(PrintError("Insufficient Shares for Versus",
        pzTrans->iID, lRecNo, sRecType, 105, 0, 0, "TALOC CLOSE5", FALSE));

        */
      } /* for j loop */

      if (bMissingLot)
        break;
    } /* for i loop */
  } // EX or ES

  if (bMissingLot) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return (PrintError("Unable to Identify TaxLot for Versus", pzTrans->iID,
                       lRecNo, sRecType, 133, 0, 0, "TALOC - CLOSE6", FALSE));
  }

  /* If Total Shares Removed not = Total Shares to sell */
  if (!IsValueZero(zTIOriginal.fUnits - fTotalSharesRemoved, 5))
    RemoveByAcctMethod(zPosInfo, &fRemainingSharesToRemove,
                       &fTotalSharesRemoved, &fTotalCostRemoved);

  /*
  ** If there are still move shares to sell, but no positions to sell them
  ** against, bounce the trade (NO MORE tech-long/tech-short closing
  * transactions)
  */
  if (!IsValueZero(zTIOriginal.fUnits - fTotalSharesRemoved, 5)) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return (PrintError("Insufficient Shares To Sell", pzTrans->iID, lRecNo,
                       sRecType, 134, 0, 0, "TALOC - CLOSE7", FALSE));
  }

  /*
  ** Until now we had only figured out which lots to sell, now generate the
  ** individual closing transactions
  */
  zErr = TransactionProcess(zPosInfo, pzTrans, &zTIOriginal, &zTIRemaining,
                            zDTransDesc, iNumDTItems, pzPayTran, sCurrPrior,
                            &lFirstTransNo, zPFinc.sOriginalFace,
                            zSecType.sPrimaryType, zSecType.sSecondaryType,
                            zPFinc.sPayType, bDoTransaction);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return zErr;
  }

  /*
  ** If there are still move shares to sell, but no positions to sell them
  ** against, generate tech-long/tech-short closing transactions
  */
  /*if (fTotalSharesRemoved != zTIOriginal.fUnits)
          zErr = ProcessTechnicals(pzTrans, zTranType, &zTIOriginal,
     &zTIRemaining, zDTransDesc, iNumDTItems, sCurrPrior, &lFirstTransNo,
     bDoTransaction);*/

  /* Free the position information table */
  InitializePosInfoTable(&zPosInfo, 0);
  return zErr;
} /* End of close type processing */

/**
** Function to parse the text of the dtrans_desc into units, unitcost
** and trade date
**/
ERRSTRUCT DecipherDescInfo(long *plOpenDate, double *pfSellUnits,
                           double *pfUnitCost, char *sDescInfo, int iID,
                           long lRecNo, char *sRecType) {
  char sTradeDate[40 + NT], sSellUnits[40 + NT], sAtSign[40 + NT];
  char sPrice[40 + NT], sDescParse[40 + NT], *pAtSignPtr;
  ERRSTRUCT zErr;

  /* Initialize all variables and structures */
  InitializeErrStruct(&zErr);
  strcpy_s(sDescParse, " ");
  strcpy_s(sPrice, " ");
  strcpy_s(sSellUnits, " ");
  strcpy_s(sAtSign, " ");
  strcpy_s(sTradeDate, " ");

  /* Parse the string */
  strcpy_s(sDescParse, sDescInfo);
  strtok(sDescParse, " ");
  strcpy_s(sTradeDate, strtok(NULL, " "));
  strcpy_s(sSellUnits, strtok(NULL, " "));

  pAtSignPtr = (char *)strtok(NULL, " ");
  strcpy_s(sAtSign, pAtSignPtr);

  /* Check that the string contains a '@' which is always included as
  ** part of the versus trailer.  If there is no '@', fail the trade
  */
  if (*sAtSign != '@')
    return (PrintError("Invalid @ - At Sign on Versus ", iID, lRecNo, sRecType,
                       203, 0, 0, "TALOC - DECIPHER DTRANS", FALSE));

  /* Calculate the unit price */
  pAtSignPtr += 2;
  strcpy_s(sPrice, pAtSignPtr);

  *pfUnitCost = atof(sPrice);
  if (IsValueZero(*pfUnitCost, 6))
    return (PrintError("Invalid Unit Cost on Versus", iID, lRecNo, sRecType,
                       201, 0, 0, "TALOC - DECIPHER DTRANS1", FALSE));

  /* Calculate the trade date of the lot */
  //************* need rstrdate function to do rstrdate(sTradeDate, lOpenDate)
  //****** cw  rstrdate(sTradeDate, lOpenDate);
  if (*plOpenDate == 0)
    return (PrintError("Invalid Trade Date on Versus", iID, lRecNo, sRecType,
                       202, 0, 0, "TALOC - DECIPHER DTRANS2", FALSE));

  /* Calculate the units */
  *pfSellUnits = atof(sSellUnits);
  if (IsValueZero(*pfSellUnits, 5))
    return (PrintError("Invalid Units on Versus ", iID, lRecNo, sRecType, 200,
                       0, 0, "TALOC - DECIPHER DTRANS3", FALSE));

  /* Set the return variables */

  return zErr;
} // DecipherDescInfo

/**
** Function to process an exercise/assignment of an option transaction
**/
ERRSTRUCT ExerciseOptionInTranAlloc(TRANS *pzTrans, double fExerciseUnits,
                                    TRANTYPE zTranType, SECTYPE zSecType,
                                    ASSETS zAssets, DTRANSDESC zDTransDesc[],
                                    int iNumDTItems, char *sCurrPrior,
                                    BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  BOOL bExerciseFlag, bMissingLot;
  TRANS zNTrans;
  char sCallPut[2];
  int i, j, iID;
  long lNewTransNo, lOrgTransNo;

  i = j = 0;
  bExerciseFlag = FALSE;
  bMissingLot = FALSE;

  strcpy_s(sCallPut, " ");

  InitializeErrStruct(&zErr);
  InitializeTransStruct(&zNTrans);

  iID = pzTrans->iID;

  /* Save the original transaction number */
  if (zTranType.sTranCode[0] == 'O')
    lOrgTransNo = pzTrans->lTransNo;
  else
    lOrgTransNo = pzTrans->lXrefTransNo;

  /*
  ** Assign the original security info (secno, wi, acct type, trans no) to the
  ** cross reference columns on the transaction  and vica versa
  */
  zNTrans.iID = pzTrans->iXID;
  zNTrans.iXID = pzTrans->iID;
  zNTrans.fUnits = fExerciseUnits;
  strcpy_s(zNTrans.sSecNo, pzTrans->sXSecNo);
  strcpy_s(zNTrans.sWi, pzTrans->sXWi);
  strcpy_s(zNTrans.sSecXtend, pzTrans->sXSecXtend);
  strcpy_s(zNTrans.sAcctType, pzTrans->sXAcctType);
  zNTrans.iSecID = pzTrans->iSecID;
  strcpy_s(zNTrans.sSecSymbol, zNTrans.sSecNo);
  strcpy_s(zNTrans.sCurrId, pzTrans->sXCurrId);
  strcpy_s(zNTrans.sCurrAcctType, pzTrans->sXCurrAcctType);

  strcpy_s(zNTrans.sXSecNo, pzTrans->sSecNo);
  strcpy_s(zNTrans.sXWi, pzTrans->sWi);
  strcpy_s(zNTrans.sXSecXtend, pzTrans->sSecXtend);
  strcpy_s(zNTrans.sXAcctType, pzTrans->sAcctType);
  strcpy_s(zNTrans.sXCurrId, pzTrans->sCurrId);
  strcpy_s(zNTrans.sXCurrAcctType, pzTrans->sCurrAcctType);
  zNTrans.iXSecID = pzTrans->iSecID;

  zNTrans.lXTransNo = lOrgTransNo;

  /* Assign the remaining information */
  strcpy_s(zNTrans.sBrokerCode, pzTrans->sBrokerCode);
  strcpy_s(zNTrans.sBrokerCode2, pzTrans->sBrokerCode2);
  strcpy_s(zNTrans.sCreatedBy, pzTrans->sCreatedBy);
  //  strcpy_s(zNTrans.sCreateTime, pzTrans->sCreateTime);
  _strtime(zNTrans.sCreateTime);

  zNTrans.lTrdDate = pzTrans->lTrdDate;
  zNTrans.lStlDate = pzTrans->lStlDate;
  zNTrans.lEffDate = pzTrans->lEffDate;
  zNTrans.lEntryDate = pzTrans->lEntryDate;
  zNTrans.lCreateDate = pzTrans->lCreateDate;
  zNTrans.lPostDate = pzTrans->lPostDate;

  /*
  ** Set the accounting method, this must be done in case the accouting
  ** method on the original transaction was set to a 'v'
  */
  lpprSelectAcctMethod(zNTrans.sAcctMthd, zNTrans.iID, &zErr);
  if (zErr.iSqlError)
    return (PrintError("Error Reading Portmain Acct Method", zNTrans.iID, 0, "",
                       0, zErr.iSqlError, zErr.iIsamCode, "TALOC EXERCISE1",
                       FALSE));

  zErr = GetCallPut(zNTrans, sCallPut);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
   ** If the option is a put and the underlying trade is 'PS' or 'SS'
   ** set the tran_type to 'ES' and the dr_cr to 'DR'
   ** Similarily if the option is a call and the underlying trade is a
   ** 'SL' or 'CS', set the tran_type to 'ES' and the dr_cr to 'DR'
   ** The opposite of the above, set the trantype to 'EX' and the
   ** dr_cr to 'CR'
   */
  if ((strcmp(sCallPut, "P") == 0 && (strcmp(pzTrans->sTranType, "PS") == 0 ||
                                      strcmp(pzTrans->sTranType, "CS") == 0)) ||
      (strcmp(sCallPut, "C") == 0 && (strcmp(pzTrans->sTranType, "SL") == 0 ||
                                      strcmp(pzTrans->sTranType, "SS") == 0))) {
    strcpy_s(zNTrans.sTranType, "ES");
    strcpy_s(zNTrans.sDrCr, "DR");
  } else {
    strcpy_s(zNTrans.sTranType, "EX");
    strcpy_s(zNTrans.sDrCr, "CR");
  }

  /*
  ** retrieve the assets, sectype and trantype of the option security
  ** to pass onto the function.
  */
  zErr = GetNewSecurityInfo(zNTrans, &zAssets, &zSecType, &zTranType);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /* Set the currency and exchange rate information */
  strcpy_s(zNTrans.sSecCurrId, zAssets.sCurrId);
  strcpy_s(zNTrans.sAccrCurrId, zAssets.sCurrId);
  strcpy_s(zNTrans.sIncCurrId, zNTrans.sCurrId);
  strcpy_s(zNTrans.sIncAcctType, zNTrans.sCurrAcctType);

  if (strcmp(zNTrans.sCurrId, pzTrans->sCurrId) == 0) {
    zNTrans.fBaseXrate = pzTrans->fBaseXrate;
    zNTrans.fIncBaseXrate = zNTrans.fBaseXrate;
    zNTrans.fSysXrate = pzTrans->fSysXrate;
    zNTrans.fIncSysXrate = zNTrans.fSysXrate;
  } else {
    zNTrans.fBaseXrate = 1;
    zNTrans.fIncBaseXrate = 1;
    zNTrans.fSysXrate = 1;
    zNTrans.fIncSysXrate = 1;
  }

  if (strcmp(zNTrans.sSecCurrId, pzTrans->sSecCurrId) == 0) {
    zNTrans.fSecBaseXrate = pzTrans->fSecBaseXrate;
    zNTrans.fAccrBaseXrate = zNTrans.fSecBaseXrate;
  } else {
    zNTrans.fSecBaseXrate = 1;
    zNTrans.fAccrBaseXrate = 1;
  }

  /*
  ** Upon completion of the new transaction, pass the transaction to either
  ** the closing function for final prepping before it is sent to
  ** 'UPDATEHOLD'
  */
  zErr = ProcessCloseInTranAlloc(&zNTrans, zTranType, zSecType, zAssets,
                                 zDTransDesc, iNumDTItems, NULL, &bExerciseFlag,
                                 sCurrPrior, &fExerciseUnits, bDoTransaction);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  lNewTransNo = zNTrans.lXrefTransNo;

  /*
  ** For each primary transaction that is generated, the original 'ex'
  ** transaction must be updated with the primary transaction's number
  ** and security id
  */
  lpprUpdateXTransNo(lNewTransNo, iID, lOrgTransNo, &zErr);
  if (zErr.iSqlError != 0 || zErr.iSqlError != 0)
    zErr =
        PrintError("Error Updating TRANS", iID, lOrgTransNo, "T", 0,
                   zErr.iSqlError, zErr.iIsamCode, "TALOC - EXERCISE3", FALSE);
  return zErr;
} /* ExerciseOptionInTranAlloc */

/**
** Function to calculate the option premium for exericises and assignments
**/
ERRSTRUCT CalcOptionPremium(TRANS zTrans, double *pfExerciseUnits,
                            double *pfOptPrem) {
  ASSETS zAssets;
  SECTYPE zSecType;
  TRANTYPE zTranType;
  POSINFOTABLE zPosInfo;
  ERRSTRUCT zErr;
  char sOriginalFace[2], sRecType[2], sCallPut[2];
  long lRecNo;
  double fTotalCostRemoved, fRemainingSharesToRemove, fTotalUnits,
      fTotalSharesRemoved;
  int i;

  /* Initialize all temporary variables */
  fTotalCostRemoved = fTotalUnits = fTotalSharesRemoved =
      fRemainingSharesToRemove = 0;
  i = 0;
  strcpy_s(sOriginalFace, " ");
  strcpy_s(sCallPut, " ");
  InitializeErrStruct(&zErr);

  if (zTrans.lDtransNo != 0) {
    lRecNo = zTrans.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTrans.lTransNo;
    strcpy_s(sRecType, "T");
  }

  /* Copy the xsecno and such variables to secno etc */
  strcpy_s(zTrans.sSecNo, zTrans.sXSecNo);
  strcpy_s(zTrans.sWi, zTrans.sXWi);
  strcpy_s(zTrans.sSecXtend, zTrans.sXSecXtend);
  strcpy_s(zTrans.sAcctType, zTrans.sXAcctType);
  zTrans.iSecID = zTrans.iXSecID;

  /* Identify if the option is a call or put */
  zErr = GetCallPut(zTrans, sCallPut);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
   ** If the option is a put and the underlying trade is 'PS' or 'SS'
   ** set the tran_type to 'ES' and the dr_cr to 'DR'
   ** Similarily if the option is a call and the underlying trade is a
   ** 'SL' or 'CS', set the tran_type to 'ES' and the dr_cr to 'DR'
   ** The opposite of the above, set the trantype to 'EX' and the
   ** dr_cr to 'CR'
   */
  if ((strcmp(sCallPut, "P") == 0 && (strcmp(zTrans.sTranType, "PS") == 0 ||
                                      strcmp(zTrans.sTranType, "CS") == 0)) ||
      (strcmp(sCallPut, "C") == 0 && (strcmp(zTrans.sTranType, "SL") == 0 ||
                                      strcmp(zTrans.sTranType, "SS") == 0))) {
    strcpy_s(zTrans.sTranType, "ES");
    strcpy_s(zTrans.sDrCr, "DR");
  } else {
    strcpy_s(zTrans.sTranType, "EX");
    strcpy_s(zTrans.sDrCr, "CR");
  }

  zErr = GetNewSecurityInfo(zTrans, &zAssets, &zSecType, &zTranType);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
  ** Check that the currency id of the option matches the currency id of the
  ** underlying security, if they do not match, generate an error message
  */
  if (strcmp(zTrans.sSecCurrId, zAssets.sCurrId) != 0)
    return (PrintError("Currency Id Mismatch between Option & Security",
                       zTrans.iID, lRecNo, sRecType, 999, 0, 0,
                       "TALOC CALCOPT1", FALSE));

  /* Calculate the number of option units that need to be exercised/assigned */
  *pfExerciseUnits = zTrans.fUnits / zAssets.fTradUnit;
  zTrans.fUnits = *pfExerciseUnits;

  /* Initialize the position information table */
  memset(&zPosInfo, 0, sizeof(zPosInfo));
  zErr = InitializePosInfoTable(&zPosInfo, NUMEXTRAELEMENTS);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return zErr;
  }

  fRemainingSharesToRemove = zTrans.fUnits;

  /* Get the position information table filled in from current holdings table*/
  zErr = GetExistingPosition(zTrans, &zPosInfo, &fTotalUnits, zTranType,
                             zSecType, sOriginalFace, zAssets.fTradUnit);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return zErr;
  }

  /* If there are insufficient shares to remove, generate an error message */
  if (fTotalUnits < fRemainingSharesToRemove) {
    /* Free the position information table */
    InitializePosInfoTable(&zPosInfo, 0);
    return (PrintError("Insufficient Option Units", zTrans.iID, lRecNo,
                       sRecType, 105, 0, 0, "TALOC CALCOPT2", FALSE));
  }

  RemoveByAcctMethod(zPosInfo, &fRemainingSharesToRemove, &fTotalSharesRemoved,
                     &fTotalCostRemoved);
  *pfOptPrem = fTotalCostRemoved;

  /* Free the position information table */
  InitializePosInfoTable(&zPosInfo, 0);
  return zErr;

} /* CalcOptionPremium */

/**
** Function to process adjustment type of transaction
**/
ERRSTRUCT ProcessAdjustInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                   int iNumDTItems, char *sCurrPrior,
                                   BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  HOLDINGS zHoldings;
  long lRecNo;
  char sRecType[2];
  BOOL bRecFound;
  TRANS zTR;

  InitializeErrStruct(&zErr);

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  /*
  ** If the adjustement transaction is a liquidating dividend, amortization
  ** pair off, sakekeeping, update cost, set or unset permanent gain/loss flag,
  ** return of principal, set or unset restriction, update date, purcahse or
  * sell of
  ** basis transaction, the lot number must be provided, else if the transaction
  * is a
  ** 'SE' or AG and balance to adjust is null or blank(just check 1st
  * character), return an error
  */
  if ((strcmp(pzTrans->sTranType, "LD") == 0 ||
       strcmp(pzTrans->sTranType, "AM") == 0 ||
       strcmp(pzTrans->sTranType, "PO") == 0 ||
       strcmp(pzTrans->sTranType, "SK") == 0 ||
       strcmp(pzTrans->sTranType, "SG") == 0 ||
       strcmp(pzTrans->sTranType, "UK") == 0 ||
       strcmp(pzTrans->sTranType, "UG") == 0 ||
       strcmp(pzTrans->sTranType, "UC") == 0 ||
       strcmp(pzTrans->sTranType, "SR") == 0 ||
       strcmp(pzTrans->sTranType, "UR") == 0 ||
       strcmp(pzTrans->sTranType, "RP") == 0 ||
       strcmp(pzTrans->sTranType, "UD") == 0 ||
       strcmp(pzTrans->sTranType, "PZ") == 0 ||
       strcmp(pzTrans->sTranType, "SZ") == 0) &&
      pzTrans->lTaxlotNo == 0)
    return (PrintError("Invalid Taxlot Number", pzTrans->iID, lRecNo, sRecType,
                       51, 0, 0, "TALOC ADJUST1", FALSE));
  else if (strcmp(pzTrans->sTranType, "SE") == 0 ||
           strcmp(pzTrans->sTranType, "AG") == 0) {
    if (pzTrans->sBalToAdjust[0] == ' ' || pzTrans->sBalToAdjust[0] == '\0')
      return (PrintError("Invalid Balance To Adjust", pzTrans->iID, lRecNo,
                         sRecType, 89, 0, 0, "TALOC ADJUST2", FALSE));
    else if (strcmp(pzTrans->sBalToAdjust, "TAX") == 0 ||
             strcmp(pzTrans->sBalToAdjust, "NTAX") == 0)
      return (PrintError("Invalid Balance To Adjust", pzTrans->iID, lRecNo,
                         sRecType, 114, 0, 0, "TALOC ADJUST3", FALSE));
  } /* if SE or SG */
  else if (strcmp(pzTrans->sTranType, "UB") ==
           0) // update broker - needs the original transaction
  {
    lpprSelectOneTrans(pzTrans->iID, pzTrans->lOrigTransNo, &zTR, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND)
      return (PrintError("Invalid Transaction Number", pzTrans->iID,
                         pzTrans->lOrigTransNo, "T", 50, 0, 0, "TALOC ADJUST4",
                         FALSE));
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    if (zTR.lRevTransNo != 0)
      return (PrintError(
          "Invalid Transaction Number. Transaction Has Been Reversed",
          pzTrans->iID, pzTrans->lOrigTransNo, "T", 50, 0, 0, "TALOC ADJUST5",
          FALSE));
  }

  if (strcmp(pzTrans->sTranType, "UD") == 0 &&
      (strcmp(pzTrans->sBalToAdjust, "TRD") != 0 &&
       strcmp(pzTrans->sBalToAdjust, "STL") != 0 &&
       strcmp(pzTrans->sBalToAdjust, "ELIG") != 0))
    return (PrintError("Invalid Date To Update", pzTrans->iID, lRecNo, sRecType,
                       136, 0, 0, "TALOC ADJUST6", FALSE));

  /*
  ** Check that the principal amount/income amt column on the transaction
  * contains
  ** a value, if the value is zero, generate an error message and fail the trade
  */
  if (IsValueZero(pzTrans->fPcplAmt, 2) &&
      IsValueZero(pzTrans->fIncomeAmt, 2) &&
      strcmp(pzTrans->sTranType, "SG") != 0 &&
      strcmp(pzTrans->sTranType, "SK") != 0 &&
      strcmp(pzTrans->sTranType, "UG") != 0 &&
      strcmp(pzTrans->sTranType, "UK") != 0 &&
      strcmp(pzTrans->sTranType, "SE") != 0 &&
      strcmp(pzTrans->sTranType, "UD") != 0 &&
      strcmp(pzTrans->sTranType, "SR") != 0 &&
      strcmp(pzTrans->sTranType, "UR") != 0 &&
      strcmp(pzTrans->sTranType, "UB") != 0)
    return (PrintError("Invalid Principal Amount", pzTrans->iID, lRecNo,
                       sRecType, 26, 0, 0, "TALOC ADJUST7", FALSE));

  /*
   ** If the transaction is a liquidating dividend, get the lot
   ** if the pcpl amt on trade is greater than the total cost or
   ** the total cost is zero, mark the trade as capital gain
   ** SB 7/19/01 RP should also work the same way as LD
   */
  if (strcmp(pzTrans->sTranType, "LD") == 0 ||
      strcmp(pzTrans->sTranType, "RP") == 0) {
    /* Get the holding */
    lpprSelectHoldings(&zHoldings, pzTrans->iID, pzTrans->sSecNo, pzTrans->sWi,
                       pzTrans->sSecXtend, pzTrans->sAcctType,
                       pzTrans->lTaxlotNo, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      bRecFound = FALSE;
    } else if (zErr.iSqlError)
      return zErr;
    else
      bRecFound = TRUE;

    // If the holding is not found, set the opening date to the default date
    if (bRecFound == FALSE) {
      pzTrans->lOpenTrdDate = 0;
      pzTrans->lOpenStlDate = 0;
    }

    if (bRecFound == TRUE) {
      if (zHoldings.fUnits < 0) {
        zHoldings.fTotCost *= -1;
        zHoldings.fOrigCost *= -1;
      }

      /*
       ** If the lot was created by a FR/TS then the trade and settlement dates
       * on the lot might
       ** not be the dates on which trade became effective in the system. In
       * this case the original
       ** trade and settlement dates may be lost(if the lot was sold completely
       * and there is no
       ** holddel information to recreate it). In all the other cases, original
       * trade and settlement
       ** dates will be preserved.
       */
      if (strcmp(zHoldings.sOrigTransType, "FR") == 0 ||
          strcmp(zHoldings.sOrigTransType, "TS") == 0) {
        pzTrans->lOpenTrdDate = zHoldings.lEffDate;
        pzTrans->lOpenStlDate = zHoldings.lEffDate;
      } else {
        pzTrans->lOpenTrdDate = zHoldings.lTrdDate;
        pzTrans->lOpenStlDate = zHoldings.lStlDate;
      }
      pzTrans->fOpenUnitCost = zHoldings.fUnitCost;

      /*
       ** If the principal amount on the trade is greater than
       ** than the cost on the lot, calculate the difference.
       */
      if (pzTrans->fPcplAmt >= zHoldings.fTotCost) {
        // fAdjAmt = pzTrans->fPcplAmt - zHoldings.fTotCost;

        /*
         ** if the trade is a debit and units < 0, the trade will
         ** bring the basis up towards zero and if the trade
         ** is a credit and units > 0, the trade will reduce the
         ** basis towards zero.  If it is not one of these types,
         ** the result will be to increase the cost basis either
         ** positively or negatively and there is no cap gain
         ** so set the tot cost and orig cost equal to the pcpl amt
         */
        if ((strcmp(pzTrans->sDrCr, "DR") == 0 && zHoldings.fUnits < 0) ||
            (strcmp(pzTrans->sDrCr, "CR") == 0 && zHoldings.fUnits > 0)) {
          pzTrans->fTotCost = zHoldings.fTotCost;
          pzTrans->fOrigCost = zHoldings.fOrigCost;
        } /* end of dr/cr check */
        else {
          pzTrans->fTotCost = pzTrans->fPcplAmt;
          pzTrans->fOrigCost = pzTrans->fPcplAmt;
        }
      } else {
        pzTrans->fTotCost = pzTrans->fPcplAmt;
        pzTrans->fOrigCost = pzTrans->fPcplAmt;
      } /* end of total cost check */
    } /* end of brecfound check */
  }

  /*
  ** Get the last transaction number from the portdir table
  ** and assign that value to the transaction number on the trans table
  */
  zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
  if (zErr.iSqlError != 0)
    return zErr;

  pzTrans->lTransNo = zPortmain.lLastTransNo;

  /* Pass the transaction to "updhold" for final processing */
  zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, NULL,
                    sCurrPrior, bDoTransaction);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior, bDoTransaction,
                                ADDANDWRITEVALUE, TRUE);

  return zErr;
} /* End of Adjust type processing */

/**
** Function to process split type of transaction
**/
ERRSTRUCT ProcessSplitInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                  int iNumDTItems, char *sCurrPrior,
                                  BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  char sRecType[2];
  long lRecNo;

  InitializeErrStruct(&zErr);

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  /* If the units are set to zero, return an error */
  if (IsValueZero(pzTrans->fUnits, 5))
    return (PrintError("Invalid Units", pzTrans->iID, lRecNo, sRecType, 28, 0,
                       0, "TALOC SPLIT1", FALSE));

  /*
  ** If the security extension is set to either tech long or tech short
  ** fail the trade
  */
  if (strcmp(pzTrans->sSecXtend, "TL") == 0 ||
      strcmp(pzTrans->sSecXtend, "TS") == 0)
    return (PrintError("Invalid Security Extention", pzTrans->iID, lRecNo,
                       sRecType, 107, 0, 0, "TALOC - SPLIT2", FALSE));

  /* Verify that a tax lot number has been assigned */
  if (pzTrans->lTaxlotNo == 0)
    return (PrintError("Invalid Taxlot Number", pzTrans->iID, lRecNo, sRecType,
                       51, 0, 0, "TALOC - SPLIT3", FALSE));

  /*
  ** Increase the last trans no in portdir file and assign that number to
  ** TransNo in Trans record
  */
  zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
  if (zErr.iSqlError != 0)
    return zErr;

  pzTrans->lTransNo = zPortmain.lLastTransNo;

  if (pzTrans->lXrefTransNo == 0)
    pzTrans->lXrefTransNo = pzTrans->lTransNo;

  /* Pass the transaction to "updhold" for final processing */
  zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, NULL,
                    sCurrPrior, bDoTransaction);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior, bDoTransaction,
                                ADDANDWRITEVALUE, TRUE);

  return zErr;
} /* End of Split type processing */

/**
** Function to process transfer type of transaction
**/
ERRSTRUCT ProcessTransferInTranAlloc(TRANS *pzTrans, TRANTYPE zTranType,
                                     SECTYPE zSecType, ASSETS zAssets,
                                     DTRANSDESC zDTransDesc[], int iNumDTItems,
                                     char *sCurrPrior, BOOL bDoTransaction) {
  PORTMAIN zPortmain;
  ERRSTRUCT zErr;
  BOOL bExerciseFlag;
  char sRecType[2];
  long lRecNo;
  double fExerciseUnits;

  InitializeErrStruct(&zErr);
  fExerciseUnits = 0;

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  if (pzTrans->iID != pzTrans->iXID)
    return (PrintError("Invalid Transfer", pzTrans->iID, lRecNo, sRecType, 7, 0,
                       0, "TALOC - TRANSFER1", FALSE));

  /*
  ** If the transaction is a transfer of currency (sec_impact is zero)
  ** then assign a transaction number and pass the transaction onto
  ** upd_hold for procesing
  ** If the transaction is a transfer of security (sec_impact is one)
  ** then pass the transaction to the process close function for
  ** further processing and posting
  */
  if (zTranType.lSecImpact == 0) {
    /*
    ** Increase the last trans no in portdir file and assign that number to
    ** TransNo in Trans record
    */
    zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
    if (zErr.iSqlError != 0)
      return zErr;

    pzTrans->lTransNo = zPortmain.lLastTransNo;

    /* Pass the transaction to "updhold" for final processing */
    zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, NULL,
                      sCurrPrior, bDoTransaction);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                  bDoTransaction, ADDANDWRITEVALUE, TRUE);
  } else
    /*Pass the transaction to the process close function for further processing
     */
    zErr = ProcessCloseInTranAlloc(
        pzTrans, zTranType, zSecType, zAssets, zDTransDesc, iNumDTItems, NULL,
        &bExerciseFlag, sCurrPrior, &fExerciseUnits, bDoTransaction);

  return zErr;
} /* End of Transfer type processing */

/**
** Function to process cancellation type of transaction
**/
ERRSTRUCT ProcessCancelInTranAlloc(TRANS *pzTrans, DTRANSDESC zDTransDesc[],
                                   int iNumDTItems, char *sCurrPrior,
                                   BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  char sTranCode[2];
  TRANS zTR;
  BOOL bUpDateHoldCalled;

  InitializeErrStruct(&zErr);

  /* Verify that the reversal tranaction number column contains a value */
  if (pzTrans->lRevTransNo == 0)
    return (PrintError("Invalid Reversal Transaction Number", pzTrans->iID,
                       pzTrans->lRevTransNo, "T", 53, 0, 0, "TALOC - CANCEL1",
                       FALSE));

  lpprSelectTrancode(sTranCode, pzTrans->sRevType, pzTrans->sDrCr, &zErr);
  if (zErr.iSqlError)
    return (PrintError("Error Reading TRANTYPE", pzTrans->iID,
                       pzTrans->lRevTransNo, "T", 0, zErr.iSqlError,
                       zErr.iIsamCode, "TALOC - CANCEL2", FALSE));

  /*
  ** If it's a reversal of closing trade (i.e. original trade was a
  ** closing trade) or a transfer or a split or an income transaction
  ** or a money transaction whose
  ** balance to adjust column is set to 'ALL'
  ** cancel all the effected trades(which have same xref_trans_no as the transno
  ** of the current trade) in reverse order of trans_no(or trade date)
  */
  if (pzTrans->lXrefTransNo != 0 &&
      (sTranCode[0] == 'C' || sTranCode[0] == 'X' || sTranCode[0] == 'I' ||
       sTranCode[0] == 'M' || sTranCode[0] == 'S' ||
       (sTranCode[0] == 'A' && (strcmp(pzTrans->sRevType, "LD") == 0 ||
                                strcmp(pzTrans->sRevType, "BA") == 0)))) {
    while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
      /* retrieve the effected trade */
      InitializeTransStruct(&zTR);
      lpprSelectTransForMatchingXref(pzTrans->iID, pzTrans->lXrefTransNo, &zTR,
                                     &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = zErr.iIsamCode = 0;
        break;
      } else if (zErr.iSqlError)
        return zErr;

      /*
      ** This should not happen, a trade which has already been reversed,
      ** being picked up again for reversal, so it must be skipped
      */
      if (strcmp(zTR.sTranType, "RV") == 0 || zTR.lRevTransNo != 0)
        continue;

      /* make the current transaction a reversal and pass it to updatehold */
      zTR.lRevTransNo = zTR.lTransNo;
      strcpy_s(zTR.sRevType, zTR.sTranType);
      zTR.lDtransNo = pzTrans->lDtransNo;
      zTR.lBlockTransNo = pzTrans->lBlockTransNo;

      zErr = IncrementPortmainLastTransNo(&zPortmain, zTR.iID);
      if (zErr.iSqlError != 0)
        return zErr;

      zTR.lTransNo = zPortmain.lLastTransNo;
      strcpy_s(zTR.sTranType, "RV");
      pzTrans->lTransNo = zTR.lTransNo;

      /*
       ** Set the created by, create date and post date to match the
       ** agg trade sent from tranproc
       */
      strcpy_s(zTR.sCreatedBy, pzTrans->sCreatedBy);
      strcpy_s(zTR.sTransSrce, pzTrans->sTransSrce);
      zTR.lCreateDate = pzTrans->lCreateDate;
      zTR.lPostDate = pzTrans->lPostDate;
      zTR.lEntryDate = pzTrans->lEntryDate;

      zErr = UpdateHold(zTR, zPortmain, zDTransDesc, iNumDTItems, NULL,
                        sCurrPrior, bDoTransaction);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      zErr = CurrencySweepProcessor(zTR, zPortmain, sCurrPrior, bDoTransaction,
                                    ADDVALUE, TRUE);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;

      bUpDateHoldCalled = TRUE;

      /* Update the rev_trans_no on the original transaction */
      lpprUpdateRevTransNo(pzTrans->lTransNo, pzTrans->iID, zTR.lRevTransNo,
                           &zErr);
      if (zErr.iSqlError)
        zErr = PrintError("Error Updating TRANS", zTR.iID, zTR.lRevTransNo, "T",
                          0, zErr.iSqlError, zErr.iIsamCode, "TALOC - CANCEL4",
                          FALSE);
    } /* while SqlError = 0 and businesserror = 0 */

    if (bUpDateHoldCalled)
      zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                    bDoTransaction, JUSTWRITEVALUE, FALSE);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  } /* if original trade was a closing trade */
  else {
    /*
    ** Increase the last trans no in portdir file and assign that number to
    ** TransNo in Trans record
    */
    zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
    if (zErr.iSqlError != 0)
      return zErr;

    pzTrans->lTransNo = zPortmain.lLastTransNo;

    /* Pass the transaction to "updhold" for final processing */
    zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, NULL,
                      sCurrPrior, bDoTransaction);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                  bDoTransaction, ADDANDWRITEVALUE, TRUE);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    /* Update the rev_trans_no on the original transaction */
    lpprUpdateRevTransNo(pzTrans->lTransNo, pzTrans->iID, pzTrans->lRevTransNo,
                         &zErr);
    if (zErr.iSqlError)
      zErr = PrintError("Error Updating TRANS", pzTrans->iID, pzTrans->lTransNo,
                        "T", 0, zErr.iSqlError, zErr.iIsamCode,
                        "TALOC - CANCEL4", FALSE);
  }

  return zErr;
} /* End of Cancel type processing */

/**
** Function to create position verification table
**/
ERRSTRUCT GetExistingPosition(TRANS zTrans, POSINFOTABLE *pzPv,
                              double *pfTotalUnits, TRANTYPE zTranType,
                              SECTYPE zSecType, char *sOriginalFace,
                              double fTradingUnit) {
  ERRSTRUCT zErr;
  int i, j;
  double fReductionRatio, fSharesToRemove, fOrigFaceRatio, fSalePrice;
  BOOL bReduceFlag, bShortFlag;
  long lRecNo, lMinTrdDate, lMaxTrdDate;
  char sRecType[2], sAcctType[2];
  HOLDINGS zHoldings;

  InitializeErrStruct(&zErr);

  if (zTrans.lDtransNo != 0) {
    lRecNo = zTrans.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTrans.lTransNo;
    strcpy_s(sRecType, "T");
  }

  i = 0;
  *pfTotalUnits = 0;

  bReduceFlag = FALSE;
  bShortFlag = FALSE;

  /*
  ** If the transaction is an 'ex', all lots are eligible for closing
  ** all other closing transactions are limited by their account type
  */
  if (strcmp(zTrans.sTranType, "EX") == 0 ||
      strcmp(zTrans.sTranType, "ES") == 0)
    strcpy_s(sAcctType, "%");
  else
    strcpy_s(sAcctType, zTrans.sAcctType);

  /*
  ** portfolio's accounting method determines the order in which records are
  * retrived from the
  ** holdings/holdcash table. Records are sorted in ascending order of the
  * effdate for accounting
  ** method "F"(FIFO) and "A"(average-cost), in descending order of effdate for
  * "L"(LIFO), in
  ** descending order of totcost for "H"(High), in ascending order of totcost
  * for "S"(low), in
  ** descending order of totcost and long term and then in descending order of
  * totcost and
  ** short term for M(Minimum Gain), in ascending order of totcost and short
  * term and then in
  ** ascending order of totcost and long term for X(Maximum Gain), in descending
  * order of the
  ** unit cost where unit cost is less or equal to sale price and then
  * descending order of the
  ** unit cost where unit cost is geater than sale price for G(Minimum Gain and
  * Maximum Loss) and
  ** in ascending order of the unit cost where unit cost is greater or equal to
  * sale price and
  ** then descending order of the unit cost where unit cost is less than sale
  * price for O(Minimum Loss and Minimum Gain)
  */
  if (zTrans.sAcctMthd[0] != 'F' && zTrans.sAcctMthd[0] != 'A' &&
      zTrans.sAcctMthd[0] != 'L' && zTrans.sAcctMthd[0] != 'H' &&
      zTrans.sAcctMthd[0] != 'S' && zTrans.sAcctMthd[0] != 'M' &&
      zTrans.sAcctMthd[0] != 'X' && zTrans.sAcctMthd[0] != 'G' &&
      zTrans.sAcctMthd[0] != 'O')
    return (PrintError("Invalid Accounting Method", zTrans.iID, lRecNo,
                       sRecType, 77, 0, 0, "TALOC - GETPOSVER1", FALSE));
  if (zErr.iBusinessError != 0)
    return zErr;

  if (IsValueZero(zTrans.fUnits * fTradingUnit, 7))
    return (PrintError("Invalid Unit/Trading Unit", zTrans.iID, lRecNo,
                       sRecType, 21, 0, 0, "TALOC - GETPOSVER1A", FALSE));
  else
    fSalePrice = zTrans.fPcplAmt / (zTrans.fUnits * fTradingUnit);

  for (j = 0; j < 2; j++) {
    /*
    ** For minimize gain and maximize gain the criteria on cost remains same but
    * the criteria on
    ** date changes (minimize gain first selects long term taxlots and then
    * short term taxlots,
    ** while maximize gain first selects short term and then long term taxlots),
    * for other types
    ** of accounting method, there is no such criteria on dates. So if
    * accounting method is
    ** minimize gain or maximize gain, run the same query twice and figure out
    * correct dates
    ** and for other accounting method, run the query only once.
    */
    if (strcmp(zTrans.sAcctMthd, "M") == 0) // minimize gain
    {
      if (j == 0) // first time, select long term taxlots in order of descending
                  // unit cost
      {
        lMinTrdDate = 0;

        // subtract 12 months from the transaction's trade date
        lpfnNewDateFromCurrent(zTrans.lTrdDate, FALSE, 0, 12, 0, &lMaxTrdDate);

        // subtract 1 more day from it
        lpfnNewDateFromCurrent(lMaxTrdDate, FALSE, 0, 0, 1, &lMaxTrdDate);
      } else if (j == 1) // short term
      {
        lMinTrdDate = lMaxTrdDate + 1;
        lMaxTrdDate = zTrans.lTrdDate;
      }
    } // "M"
    else if (strcmp(zTrans.sAcctMthd, "X") == 0) // maximize gain
    {
      if (j == 0) // first time, select short term taxlots in order of
                  // descending unit cost
      {
        // subtract 12 months from the transaction's trade date
        lpfnNewDateFromCurrent(zTrans.lTrdDate, FALSE, 0, 12, 0, &lMinTrdDate);
        lMaxTrdDate = zTrans.lTrdDate;
      } else if (j == 1) // long term
      {
        lMaxTrdDate = lMinTrdDate - 1;
        lMinTrdDate = 0;
      }
    } //"X"
    else if (strcmp(zTrans.sAcctMthd, "G") != 0 &&
             strcmp(zTrans.sAcctMthd, "O") != 0 && j > 0)
      continue;

    while (!zErr.iSqlError) {
      bReduceFlag = FALSE;
      bShortFlag = FALSE;

      InitializeHoldingsStruct(&zHoldings);

      /*
       ** Get holdings record in the right order based on accounting method.
       ** Make sure NOT to select any cost-required(trd_date = 0) lots.
       */
      if (strcmp(zTrans.sAcctMthd, "F") == 0 ||
          strcmp(zTrans.sAcctMthd, "A") == 0) {
        if (strcmp(zSecType.sPositionInd, "C") == 0)
          lpprHoldcashForFifoAndAvgAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                        zTrans.sWi, zTrans.sSecXtend,
                                        zTrans.sAcctType, 0, &zErr);
        else
          lpprHoldingsForFifoAndAvgAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                        zTrans.sWi, zTrans.sSecXtend,
                                        zTrans.sAcctType, 0, &zErr);
      } // Fifo or Average
      else if (strcmp(zTrans.sAcctMthd, "L") == 0) {
        if (strcmp(zSecType.sPositionInd, "C") == 0)
          lpprHoldcashForLifoAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                  zTrans.sWi, zTrans.sSecXtend,
                                  zTrans.sAcctType, 0, &zErr);
        else
          lpprHoldingsForLifoAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                  zTrans.sWi, zTrans.sSecXtend,
                                  zTrans.sAcctType, 0, &zErr);
      } // Lifo
      else if (strcmp(zTrans.sAcctMthd, "H") == 0) {
        if (strcmp(zSecType.sPositionInd, "C") == 0)
          lpprHoldcashForHighAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                  zTrans.sWi, zTrans.sSecXtend,
                                  zTrans.sAcctType, 0, &zErr);
        else
          lpprHoldingsForHighAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                  zTrans.sWi, zTrans.sSecXtend,
                                  zTrans.sAcctType, 0, &zErr);
      } // High
      else if (strcmp(zTrans.sAcctMthd, "S") == 0) {
        if (strcmp(zSecType.sPositionInd, "C") == 0)
          lpprHoldcashForLowAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                 zTrans.sWi, zTrans.sSecXtend, zTrans.sAcctType,
                                 0, &zErr);
        else
          lpprHoldingsForLowAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                 zTrans.sWi, zTrans.sSecXtend, zTrans.sAcctType,
                                 0, &zErr);
      } // Low
      else if (strcmp(zTrans.sAcctMthd, "M") == 0) {
        if (strcmp(zSecType.sPositionInd, "C") == 0)
          lpprHoldcashForMinimumGainAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                         zTrans.sWi, zTrans.sSecXtend,
                                         zTrans.sAcctType, 0, lMinTrdDate,
                                         lMaxTrdDate, &zErr);
        else
          lpprHoldingsForMinimumGainAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                         zTrans.sWi, zTrans.sSecXtend,
                                         zTrans.sAcctType, 0, lMinTrdDate,
                                         lMaxTrdDate, &zErr);
      } // minimum gain
      else if (strcmp(zTrans.sAcctMthd, "X") == 0) {
        if (strcmp(zSecType.sPositionInd, "C") == 0)
          lpprHoldcashForMaximumGainAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                         zTrans.sWi, zTrans.sSecXtend,
                                         zTrans.sAcctType, 0, lMinTrdDate,
                                         lMaxTrdDate, &zErr);
        else
          lpprHoldingsForMaximumGainAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                         zTrans.sWi, zTrans.sSecXtend,
                                         zTrans.sAcctType, 0, lMinTrdDate,
                                         lMaxTrdDate, &zErr);
      } // maximum gain
      else if (strcmp(zTrans.sAcctMthd, "G") == 0) {
        if (j == 0) {
          /* SB 11/5/02 - For cash position type, since holdcash has no lots, it
           * does not matter how the
           ** records are selected from the table, no need to have a seperate
           * query for acctmethed "G",
           ** use the same query which is used for "F" (fifo)
           */
          if (strcmp(zSecType.sPositionInd, "C") == 0)
            lpprHoldcashForFifoAndAvgAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                          zTrans.sWi, zTrans.sSecXtend,
                                          zTrans.sAcctType, 0, &zErr);
          else // for holdings, first get minimum gain taxlots
            lpprMinimumGainHoldings(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                    zTrans.sWi, zTrans.sSecXtend,
                                    zTrans.sAcctType, 0, fSalePrice, &zErr);
        } // j = 0
        else // j = 1; maximum loss
        {
          if (strcmp(zSecType.sPositionInd, "C") == 0)
            break; // for cash, the record has already been retreived (when j =
                   // 0)
          else
            lpprMaximumLossHoldings(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                    zTrans.sWi, zTrans.sSecXtend,
                                    zTrans.sAcctType, 0, fSalePrice, &zErr);
        } // j = 1
      } // minimum gain and maximum loss
      else // "O"
      {
        if (j == 0) // first get minimum loss taxlots
        {
          /* SB 11/5/02 - For cash position type, since holdcash has no lots, it
           * does not matter how the
           ** records are selected from the table, no need to have a seperate
           * query for acctmethed "O",
           ** use the same query which is used for "F" (fifo)
           */
          if (strcmp(zSecType.sPositionInd, "C") == 0)
            lpprHoldcashForFifoAndAvgAcct(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                          zTrans.sWi, zTrans.sSecXtend,
                                          zTrans.sAcctType, 0, &zErr);
          else
            lpprMinimumLossHoldings(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                    zTrans.sWi, zTrans.sSecXtend,
                                    zTrans.sAcctType, 0, fSalePrice, &zErr);
        } // j = 0
        else // j = 1; minimum gain
        {
          if (strcmp(zSecType.sPositionInd, "C") == 0)
            break; // for cash, the record has already been retreived (when j =
                   // 0)
          else
            lpprMinimumGainHoldings(&zHoldings, zTrans.iID, zTrans.sSecNo,
                                    zTrans.sWi, zTrans.sSecXtend,
                                    zTrans.sAcctType, 0, fSalePrice, &zErr);
        } // j = 1
      } // minimum loss and minimum gain

      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return (PrintError("Error Reading Posver Cursor", zTrans.iID, lRecNo,
                           sRecType, 0, zErr.iSqlError, zErr.iIsamCode,
                           "TALOC - GETPOSVER3", FALSE));

      // 5/23/2003 - Make sure not to close a lot whose settlement date is after
      // the settlement date of the sell
      if (zHoldings.lStlDate > zTrans.lStlDate)
        continue;

      // if it is a future, put curr_libility to tot_cost and open_liability to
      // orig_cost
      if (zSecType.sPrimaryType[0] == PTYPE_FUTURE) {
        zHoldings.fTotCost = zHoldings.fCurLiability;
        zHoldings.fOrigCost = zHoldings.fOpenLiability;
      }

      if (zTranType.lSecImpact == 1 && zHoldings.fUnits > 0)
        continue;

      if (zTranType.lSecImpact == -1 && zHoldings.fUnits < 0)
        continue;

      if (zTrans.lEffDate < zHoldings.lEffDate)
        continue;

      if (IsValueZero(zHoldings.fUnits, 5))
        continue;

      /*
      ** If the position has collateral units, check to see if the whole lot is
      * collaterialized,
      ** if this is true, skip this lot for processing, otherwise set the reduce
      * flag to true.
      */
      if (!IsValueZero(zHoldings.fCollateralUnits, 5)) {
        fSharesToRemove = zHoldings.fUnits - zHoldings.fCollateralUnits;
        if (IsValueZero(fSharesToRemove, 5))
          continue;
        else
          bReduceFlag = TRUE;
      }

      /*
      ** If the units are negative, (this should only happen where the security
      * impact is +1, or 2
      ** and dr_cr column is "dr" or an 'ex' transaction, convert the units and
      * cost columns from
      ** the holdings to a positive value. The reason is that values on the
      * transaction record
      ** are always stated in positive values and failure to convert will result
      * in inaccurate
      ** transactions and positions
      */
      if (zHoldings.fUnits < 0) {
        zHoldings.fUnits *= -1;
        zHoldings.fOrigFace *= -1;
        zHoldings.fTotCost *= -1;
        zHoldings.fOrigCost *= -1;
        zHoldings.fOpenLiability *= -1;
        zHoldings.fCurLiability *= -1;
        zHoldings.fCollateralUnits *= -1;

        bShortFlag = TRUE;
      }

      /*
      ** If the reduce flag is true, reduce the total cost and original cost
      * using a reduction ratio,
      ** this is necessary to maintain the correct cost basis for selling. The
      * units and original face
      ** must also be reduced before inserting them into the array. There is no
      * check for zero divide
      ** as the holdings units can never be zero at this point. The reduction
      * ratio can never be zero
      ** (the whole lot is collateralized) as these lots have already been
      * excluded
      */
      if (bReduceFlag) {
        fReductionRatio = 1 - (zHoldings.fCollateralUnits / zHoldings.fUnits);

        /*
        ** SB 12/29/1999 - If a original face is specified on the trade, that's
        * what should be
        ** used instead of figuring it out based on original face on holdings.
        * In this case put
        ** a
        */
        if (sOriginalFace[0] == 'Y' && !IsValueZero(zHoldings.fOrigFace, 3))
          fOrigFaceRatio = zHoldings.fOrigFace / zHoldings.fUnits;
        else
          fOrigFaceRatio = 0;

        // reduce the units
        zHoldings.fUnits -= zHoldings.fCollateralUnits;

        // calculate the amount of original face
        zHoldings.fOrigFace = zHoldings.fUnits * fOrigFaceRatio;

        // calculate the amount of cost to be reduce  and remove it
        zHoldings.fTotCost -= (zHoldings.fTotCost * fReductionRatio);
        zHoldings.fOrigCost -= (zHoldings.fOrigCost * fReductionRatio);
      } // ReduceFlag = TRUE

      pzPv->iCount++;
      pzPv->pzPInfo[i].lLot = zHoldings.lTransNo;
      pzPv->pzPInfo[i].fShares = zHoldings.fUnits;
      pzPv->pzPInfo[i].fOrigFace = zHoldings.fOrigFace;
      pzPv->pzPInfo[i].fUnitCost = zHoldings.fUnitCost;

      // Round unit cost to 6(originally 3) decimal places for 'VS' trades
      pzPv->pzPInfo[i].fUnitCostRound = RoundDouble(zHoldings.fUnitCost, 6);

      pzPv->pzPInfo[i].fTotCost = zHoldings.fTotCost;
      pzPv->pzPInfo[i].fOrigCost = zHoldings.fOrigCost;
      pzPv->pzPInfo[i].fOpenLiability = zHoldings.fOpenLiability;
      pzPv->pzPInfo[i].fBaseCostXrate = zHoldings.fBaseCostXrate;
      pzPv->pzPInfo[i].fSysCostXrate = zHoldings.fSysCostXrate;
      if (strcmp(zHoldings.sOrigTransType, "FR") == 0 ||
          strcmp(zHoldings.sOrigTransType, "TS") == 0) {
        pzPv->pzPInfo[i].lTradeDate = zHoldings.lEffDate;
        pzPv->pzPInfo[i].lSettleDate = zHoldings.lEffDate;
      } else {
        pzPv->pzPInfo[i].lTradeDate = zHoldings.lTrdDate;
        pzPv->pzPInfo[i].lSettleDate = zHoldings.lStlDate;
      }
      pzPv->pzPInfo[i].fOrgYield = zHoldings.fOrigYield;
      pzPv->pzPInfo[i].lMatDate = zHoldings.lEffMatDate;
      pzPv->pzPInfo[i].fMatPrice = zHoldings.fEffMatPrice;
      pzPv->pzPInfo[i].fCostEffMatYld = zHoldings.fCostEffMatYld;
      pzPv->pzPInfo[i].fCollateralUnits = zHoldings.fCollateralUnits;
      pzPv->pzPInfo[i].fSharesRemoved = 0;
      pzPv->pzPInfo[i].fSharesToRemove = pzPv->pzPInfo[i].fShares;
      strcpy_s(pzPv->pzPInfo[i].sAcctType, zHoldings.sAcctType);
      strcpy_s(pzPv->pzPInfo[i].sAcctMthd, zTrans.sAcctMthd);
      strcpy_s(pzPv->pzPInfo[i].sPermLtFlag, zHoldings.sPermLtFlag);
      strcpy_s(pzPv->pzPInfo[i].sSafekInd, zHoldings.sSafekInd);
      pzPv->pzPInfo[i].iRestrictionCode = zHoldings.iRestrictionCode;

      if (bShortFlag)
        strcpy_s(pzPv->pzPInfo[i].sDrCr, "DR");

      // Add the units to the total units column
      *pfTotalUnits += pzPv->pzPInfo[i].fSharesToRemove;

      i++;
      if (i == pzPv->iSize) {
        zErr = InitializePosInfoTable(pzPv, NUMEXTRAELEMENTS);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return (PrintError("Posver Table Too Small", zTrans.iID, lRecNo,
                             sRecType, zErr.iBusinessError, zErr.iSqlError,
                             zErr.iIsamCode, "TALOC - GETPOSVER4", FALSE));
      }
    } /* End of While Loop  */
  } // for j < 2

  return zErr;
} /* getexistingposition */

/**
** Function to assign shares to holdings lots based on account method
**/
void RemoveByAcctMethod(POSINFOTABLE zPv, double *pfSharesToRemove,
                        double *pfTotalSharesRemoved,
                        double *pfTotalCostRemoved) {
  int i;
  double fReductionRatio, fTotCost;

  fReductionRatio = 0;
  fTotCost = 0;
  i = 0;

  for (i = 0; i < zPv.iCount; i++) {
    /* if Remaining shares to remove = 0  */
    if (IsValueZero(*pfSharesToRemove, 5))
      break;

    /* If Array shares left = Remaining shares to remove */
    if (IsValueZero(zPv.pzPInfo[i].fSharesToRemove - *pfSharesToRemove, 5)) {
      zPv.pzPInfo[i].fSharesRemoved += *pfSharesToRemove;
      *pfTotalSharesRemoved += *pfSharesToRemove;

      zPv.pzPInfo[i].fSharesToRemove = 0;
      *pfSharesToRemove = 0;

    }
    /* if Array shares left > Remaining shares to remove */
    else if (zPv.pzPInfo[i].fSharesToRemove > *pfSharesToRemove) {
      zPv.pzPInfo[i].fSharesRemoved += *pfSharesToRemove;
      *pfTotalSharesRemoved += *pfSharesToRemove;
      zPv.pzPInfo[i].fSharesToRemove -= *pfSharesToRemove;
      *pfSharesToRemove = 0;
    }
    /*if shares left for this lot = 0*/
    else if (IsValueZero(zPv.pzPInfo[i].fSharesToRemove, 5))
      continue;
    /* if Array shares left < Remaining shares to remove */
    else {
      *pfSharesToRemove -= zPv.pzPInfo[i].fSharesToRemove;
      *pfTotalSharesRemoved += zPv.pzPInfo[i].fSharesToRemove;
      zPv.pzPInfo[i].fSharesRemoved += zPv.pzPInfo[i].fSharesToRemove;
      zPv.pzPInfo[i].fSharesToRemove = 0;
    }

    /* Calculate amount of cost removed */
    if (!IsValueZero(zPv.pzPInfo[i].fShares, 5))
      fReductionRatio =
          1 - (zPv.pzPInfo[i].fSharesRemoved / zPv.pzPInfo[i].fShares);
    else
      fReductionRatio = 1;

    if (IsValueZero(fReductionRatio, 8)) {
      if (strcmp(zPv.pzPInfo[i].sDrCr, "DR") == 0)
        *pfTotalCostRemoved -= zPv.pzPInfo[i].fTotCost;
      else
        *pfTotalCostRemoved += zPv.pzPInfo[i].fTotCost;
    } else {
      fTotCost =
          zPv.pzPInfo[i].fTotCost - (zPv.pzPInfo[i].fTotCost * fReductionRatio);

      if (strcmp(zPv.pzPInfo[i].sDrCr, "DR") == 0)
        *pfTotalCostRemoved -= fTotCost;
      else
        *pfTotalCostRemoved += fTotCost;
    }
  } /*End of for */

} /* RemoveByAcctMethod */

/**
** Function to process transactions and pass to updhold
**/
ERRSTRUCT TransactionProcess(POSINFOTABLE zPv, TRANS *pzTrans,
                             TRADEINFO *pzOriginal, TRADEINFO *pzRemaining,
                             DTRANSDESC zDTransDesc[], int iNumDTItems,
                             PAYTRAN *pzPayTran, char *sCurrPrior,
                             long *plFirstTransNo, char *sOriginalFace,
                             char *sPrimaryType, char *sSecondaryType,
                             char *sPayType, BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  TRANSTABLE2 zTIPSTrades, zTIPSAdjustments;
  DTRANSDESC zTrDesc[1];
  TRANS zTr;
  BOOL bFirstTrade, bInit, bUpdateHoldCalled, bCopyOrigFace, bFirstIncome;
  double fReductionRatio, fReduceValue, fPhantomIncome, fDiscountIncome;
  long lLastIncDate, lXrefTransNo;
  int i, iCount;

  InitializeErrStruct(&zErr);
  zTIPSTrades.iSize = 0;
  zTIPSAdjustments.iSize = 0;
  InitializeTransTable2(&zTIPSTrades);
  InitializeTransTable2(&zTIPSAdjustments);
  InitializeDtransDesc(&zTrDesc[0]);

  bFirstTrade = FALSE;
  *plFirstTransNo = 0;
  bInit = TRUE;
  bUpdateHoldCalled = FALSE;

  for (i = 0; i < zPv.iCount; i++) {
    // No more units to remove - we are done
    if (IsValueZero(pzRemaining->fUnits, 5))
      break;

    // this lot has no more shares left - skip it
    if (IsValueZero(zPv.pzPInfo[i].fSharesRemoved, 5))
      continue;

    /*
    ** If Not a Treasury Inflation protected security (TIPS) transaction then
    * get the
    ** next transaction number from portmain and assign it to this transaction,
    * else
    ** (if a TIPS closing transaction) don't do anything with transaction
    * number, this
    ** (and other, if any) transaction will be saved in a table and posted later
    * (because
    ** system needs to generate BA/PI transaction before posting these).
    */
    if (sPrimaryType[0] != PTYPE_BOND || sSecondaryType[0] != STYPE_TBILLS ||
        sPayType[0] != PAYTYPE_INFPROTCTD) {
      /*
      ** Increase the last trans no in portdir file and assign that number to
      ** TransNo in Trans record
      */
      zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
      if (zErr.iSqlError != 0)
        return zErr;

      pzTrans->lTransNo = zPortmain.lLastTransNo;

      /*
      ** If this is the first trade to be generated for the closing/transfer
      * transaction, store the
      ** transaction number in the FirstTransNo column this value will be
      * assigned to the xref_trans_no
      ** column on this transaction and all subsequent transactions that are
      * generated for this trade.
      */
      if (bFirstTrade == FALSE) {
        bFirstTrade = TRUE;
        *plFirstTransNo = zPortmain.lLastTransNo;
      }

      pzTrans->lXrefTransNo = *plFirstTransNo;
    } // Not TIPS

    pzTrans->lTaxlotNo = zPv.pzPInfo[i].lLot;
    pzTrans->lOpenTrdDate = zPv.pzPInfo[i].lTradeDate;
    pzTrans->lOpenStlDate = zPv.pzPInfo[i].lSettleDate;
    pzTrans->fOpenUnitCost = zPv.pzPInfo[i].fUnitCost;
    pzTrans->fBaseOpenXrate = zPv.pzPInfo[i].fBaseCostXrate;
    pzTrans->fSysOpenXrate = zPv.pzPInfo[i].fSysCostXrate;
    pzTrans->fOrigYld = zPv.pzPInfo[i].fOrgYield;
    pzTrans->lEffMatDate = zPv.pzPInfo[i].lMatDate;
    pzTrans->fEffMatPrice = zPv.pzPInfo[i].fMatPrice;
    strcpy_s(pzTrans->sSafekInd, zPv.pzPInfo[i].sSafekInd);
    strcpy_s(pzTrans->sAcctType, zPv.pzPInfo[i].sAcctType);
    pzTrans->iRestrictionCode = zPv.pzPInfo[i].iRestrictionCode;

    /*
    ** Calculate the reduction ratio by which the total and original
    ** cost will be adjusted
    */
    if (!IsValueZero(zPv.pzPInfo[i].fShares, 5))
      fReductionRatio =
          1 - (zPv.pzPInfo[i].fSharesRemoved / zPv.pzPInfo[i].fShares);
    else
      fReductionRatio = 1;

    /*
    ** Calculate the total and original cost for the sale record. If the
    * reduction ratio is zero
    ** (signifies that the whole lot is to be sold) there is no need of a
    * calculation, assign the
    ** whole portion. Otherwise, multiply cost by the ratio, and subtract the
    * result from total
    ** cost to calculate the amount of cost to be assigned to the sale record
    */
    if (IsValueZero(fReductionRatio, 8)) {
      pzTrans->fOrigCost = zPv.pzPInfo[i].fOrigCost;
      pzTrans->fTotCost = zPv.pzPInfo[i].fTotCost;
    } else {
      pzTrans->fOrigCost =
          zPv.pzPInfo[i].fOrigCost -
          RoundDouble(zPv.pzPInfo[i].fOrigCost * fReductionRatio, 3);
      pzTrans->fTotCost =
          zPv.pzPInfo[i].fTotCost -
          RoundDouble(fReductionRatio * zPv.pzPInfo[i].fTotCost, 3);
    }

    /*
     ** Calculate the original face to be associated with the number of units
     * being removed - do this
     ** ONLY if this security should have a original face AND USER HAS NOT
     * specified the original
     ** face amount on the trade. If user has specified an original face amount
     * then that amount is used.
     */
    if (sOriginalFace[0] == 'Y' && !IsValueZero(zPv.pzPInfo[i].fOrigFace, 3) &&
        IsValueZero(pzOriginal->fOrigFace, 3))
      pzTrans->fOrigFace = zPv.pzPInfo[i].fSharesRemoved *
                           (zPv.pzPInfo[i].fOrigFace / zPv.pzPInfo[i].fShares);
    else
      pzTrans->fOrigFace = 0;

    pzTrans->fUnits = zPv.pzPInfo[i].fSharesRemoved;
    pzRemaining->fUnits -= zPv.pzPInfo[i].fSharesRemoved;

    /*
     ** If the security is set up to amortize and the portfolio is set up to
     * amortize, calculate the
     ** amount of amortization for the sale and assign to the amort val column
     * on the trans and reduce
     ** the total cost column by that amount
     */

    // Use rest of accumulated amount if there are zero shares left
    if (IsValueZero(pzRemaining->fUnits, 5)) {
      /*
      ** SB 7/20/01 For a transfer of security or for SL where user has not
      * specified an original
      ** face, it should not get copied from the remaining trade (because it
      * will
      ** always be zero), it should be whatever has been previously calculated.
      */
      if (!IsValueZero(pzOriginal->fOrigFace, 3))
        bCopyOrigFace = TRUE;
      else
        bCopyOrigFace = FALSE;
      CopyToTransFromTradeInfo(*pzRemaining, pzTrans, bCopyOrigFace);
      if (*sPrimaryType == PTYPE_FUTURE) {
        pzTrans->fTotCost = pzRemaining->fTotCost;
        if (strcmp(zPv.pzPInfo[i].sDrCr, "DR") == 0) // short position
        {
          if (pzTrans->fTotCost >= pzTrans->fOrigCost) {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fTotCost - pzTrans->fOrigCost, 2);
            strcpy_s(pzTrans->sDrCr, "DR");
          } else {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fOrigCost - pzTrans->fTotCost, 2);
            strcpy_s(pzTrans->sDrCr, "CR");
          }
        } // short position - closing FS
        else {
          if (pzTrans->fTotCost >= pzTrans->fOrigCost) {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fTotCost - pzTrans->fOrigCost, 2);
            strcpy_s(pzTrans->sDrCr, "CR");
          } else {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fOrigCost - pzTrans->fTotCost, 2);
            strcpy_s(pzTrans->sDrCr, "DR");
          }
        } // long position - closing FP
      } // future
    } // This is the last lot
    else {
      fReduceValue = zPv.pzPInfo[i].fSharesRemoved / pzOriginal->fUnits;

      /*
       ** Calculate amount of commission, copy to trans record, remove that
       ** amount from accumulated value. Do the same for MiscFee1, MiscFee2,
       ** AccrInc, PcplAmt and SecFees
       */
      pzTrans->fCommGcr = RoundDouble(fReduceValue * pzOriginal->fCommGcr, 2);
      pzRemaining->fCommGcr -= pzTrans->fCommGcr;

      pzTrans->fNetComm = RoundDouble(fReduceValue * pzOriginal->fNetComm, 2);
      pzRemaining->fNetComm -= pzTrans->fNetComm;

      pzTrans->fMiscFee1 = RoundDouble(fReduceValue * pzOriginal->fMiscFee1, 2);
      pzRemaining->fMiscFee1 -= pzTrans->fMiscFee1;

      pzTrans->fMiscFee2 = RoundDouble(fReduceValue * pzOriginal->fMiscFee2, 2);
      pzRemaining->fMiscFee2 -= pzTrans->fMiscFee2;

      pzTrans->fAccrInt = RoundDouble(fReduceValue * pzOriginal->fAccrInt, 2);
      pzRemaining->fAccrInt -= pzTrans->fAccrInt;

      pzTrans->fIncomeAmt =
          RoundDouble(fReduceValue * pzOriginal->fIncomeAmt, 2);
      pzRemaining->fIncomeAmt -= pzTrans->fIncomeAmt;

      pzTrans->fPcplAmt = RoundDouble(fReduceValue * pzOriginal->fPcplAmt, 2);
      pzRemaining->fPcplAmt -= pzTrans->fPcplAmt;

      pzTrans->fSecFees = RoundDouble(fReduceValue * pzOriginal->fSecFees, 2);
      pzRemaining->fSecFees -= pzTrans->fSecFees;

      pzTrans->fOptPrem = RoundDouble(fReduceValue * pzOriginal->fOptPrem, 2);
      pzRemaining->fOptPrem -= pzTrans->fOptPrem;

      pzTrans->fBasisAdj = RoundDouble(fReduceValue * pzOriginal->fBasisAdj, 2);
      pzRemaining->fBasisAdj -= pzTrans->fBasisAdj;

      pzTrans->fCommGcr = RoundDouble(fReduceValue * pzOriginal->fCommGcr, 2);
      pzRemaining->fCommGcr -= pzTrans->fCommGcr;

      /*
      ** If the security should have original face and user has specified the
      * original face
      ** on the transaction, use that number.
      */
      if (sOriginalFace[0] == 'Y' && !IsValueZero(pzOriginal->fOrigFace, 3)) {
        // If the calculated amount of original face is more than what the lot
        // has, take as much as the lot has
        if (RoundDouble(fReduceValue * pzOriginal->fOrigFace, 3) >
            RoundDouble(zPv.pzPInfo[i].fOrigFace, 3))
          pzTrans->fOrigFace = RoundDouble(zPv.pzPInfo[i].fOrigFace, 3);
        else
          pzTrans->fOrigFace =
              RoundDouble(fReduceValue * pzOriginal->fOrigFace, 3);

        pzRemaining->fOrigFace -= pzTrans->fOrigFace;
      }
      // For futures TotCost has curr_libility and orig_cost has open libility.
      // Recalculate current liability for this lot and then figure out pcpl_amt
      // by subtraction open_libility from cur_libility. Also make sure dr_cr
      // flag is correct by checking soign of pcpl_amt.
      if (*sPrimaryType == PTYPE_FUTURE) {
        pzTrans->fTotCost = RoundDouble(fReduceValue * pzOriginal->fTotCost, 3);
        if (strcmp(zPv.pzPInfo[i].sDrCr, "DR") == 0) // short position
        {
          if (pzTrans->fTotCost >= pzTrans->fOrigCost) {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fTotCost - pzTrans->fOrigCost, 2);
            strcpy_s(pzTrans->sDrCr, "DR");
          } else {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fOrigCost - pzTrans->fTotCost, 2);
            strcpy_s(pzTrans->sDrCr, "CR");
          }
        } // short position - closing FS
        else {
          if (pzTrans->fTotCost >= pzTrans->fOrigCost) {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fTotCost - pzTrans->fOrigCost, 2);
            strcpy_s(pzTrans->sDrCr, "CR");
          } else {
            pzTrans->fPcplAmt =
                RoundDouble(pzTrans->fOrigCost - pzTrans->fTotCost, 2);
            strcpy_s(pzTrans->sDrCr, "DR");
          }
        } // long position - closing FP
        pzRemaining->fTotCost -= pzTrans->fTotCost;
      }
    } /* End of else part of If last lot */

    /*
    ** Assign the accounting method to the transaction as long as
    ** the accounting method on the transaction is not versus
    */
    if (pzTrans->sAcctMthd[0] != 'V')
      strcpy_s(pzTrans->sAcctMthd, zPv.pzPInfo[i].sAcctMthd);

    /*
     ** If the perm lt flag is set to 'y', set the gain/loss flag on the
     ** trans to 'L' for long term, otherwise leave blank
     */
    if (zPv.pzPInfo[i].sPermLtFlag[0] == 'Y')
      strcpy_s(pzTrans->sGlFlag, "L");

    /*
     ** If the transaction type is a maturity/sale and the pay type is a
     ** discount calculate the discount income and assign to the
     ** accr int and income amt columns on the trans
     */
    if ((strcmp(pzTrans->sTranType, "ML") == 0 ||
         strcmp(pzTrans->sTranType, "SL") == 0) &&
        strcmp(sPayType, "D") == 0) {
      fDiscountIncome =
          RoundDouble(pzTrans->fPcplAmt - RoundDouble(pzTrans->fTotCost, 2), 2);
      if (fDiscountIncome > 0) {
        pzTrans->fAccrInt = fDiscountIncome;
        pzTrans->fIncomeAmt = pzTrans->fAccrInt;
        pzTrans->fPcplAmt = RoundDouble(pzTrans->fTotCost, 2);
      } else
        pzTrans->fAccrInt = pzTrans->fIncomeAmt = 0;

      strcpy_s(pzTrans->sIncomeFlag, "D");
    } // Maturity or sale of a discounted

    /*
    ** If it a TIPS transaction, then add it to a table, to be posted after
    * system
    ** has calculated and posted phantom income and basis adjustment
    * transactions,
    ** else (not a TIPS transaction), post the transaction by calling
    * UpdateHold.
    */
    if (sPrimaryType[0] == PTYPE_BOND && sSecondaryType[0] == STYPE_TBILLS &&
        sPayType[0] == PAYTYPE_INFPROTCTD) {
      InitializeTransStruct(&zTr);
      zTr = *pzTrans;

      // Add the transaction to the TIPS table
      zErr = AddTransToTransTable2(&zTIPSTrades, zTr);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
        InitializeTransTable2(&zTIPSTrades); // cleanup in case of an error
        return zErr;
      }
    } // a TIPS transaction
    else {
      /* Pass the transaction to "updhold" for final processing */
      zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems,
                        pzPayTran, sCurrPrior, bDoTransaction);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;

      zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                    bDoTransaction, ADDVALUE, bInit);
      bInit = FALSE;
      bUpdateHoldCalled = TRUE;
    } // not a TIPS transaction
  } /* End of For i < iNumPIItems loop */

  /*
  ** If this transaction is for a TIPS, for each each taxlot being disposed,
  ** calculate and post phantom income and basis adjustment before posting
  ** actual closing transactiopn (SL/ML).
  */
  if (sPrimaryType[0] == PTYPE_BOND && sSecondaryType[0] == STYPE_TBILLS &&
      sPayType[0] == PAYTYPE_INFPROTCTD) {
    for (i = 0; i < zTIPSTrades.iCount; i++) {
      zTr = zTIPSTrades.pzTrans[i];
      // if system is reversing & rebooking a transaction, don't generate PI/BA
      // again
      if (strcmp(zTr.sCreatedBy, "tranproc") == 0)
        continue;

      // get the latest date on which PI/RI/PS was posted
      lpprGetLastIncomeDate(zTr.iID, zTr.lTaxlotNo, zTr.lStlDate, &lLastIncDate,
                            &iCount, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      if (iCount == 1)
        bFirstIncome = TRUE;
      else
        bFirstIncome = FALSE;

      // calculate the phantom income (equal to increase/decrease in principal
      // value of the position) between last income date and settlement date of
      // this closing trade
      zErr = CalculatePhantomIncome(zTr.iID, zTr.sSecNo, zTr.sWi, zTr.lTaxlotNo,
                                    lLastIncDate, zTr.lStlDate,
                                    zTr.lOpenTrdDate, zTr.fOrigCost, zTr.fUnits,
                                    bFirstIncome, &fPhantomIncome);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      // Post Phantom Income Transaction
      strcpy_s(zTr.sTranType, "PI");
      if (IsValueZero(fPhantomIncome, 2))
        continue; // if no increase in value, no need to generatePI & BA
                  // transaction
      else if (fPhantomIncome > 0)
        strcpy_s(zTr.sDrCr, "CR");
      else {
        strcpy_s(zTr.sDrCr, "DR");
        fPhantomIncome *= -1;
      }

      zTr.fPcplAmt = zTr.fTotCost = zTr.fOrigCost = zTr.fOrigFace =
          zTr.fAccrInt = 0;
      zTr.fCommGcr = zTr.fNetComm = zTr.fMiscFee1 = zTr.fMiscFee2 =
          zTr.fSecFees = 0;
      zTr.fOptPrem = zTr.fBasisAdj = 0;

      zTr.fIncomeAmt = fPhantomIncome;

      zErr = IncrementPortmainLastTransNo(&zPortmain, zTr.iID);
      if (zErr.iSqlError != 0)
        return zErr;

      // Combine all PI with xref trans no
      zTr.lTransNo = zPortmain.lLastTransNo;
      if (i == 0)
        lXrefTransNo = zTr.lTransNo;
      zTr.lXrefTransNo = lXrefTransNo;
      strcpy_s(zTr.sMiscDescInd, "N");

      // Pass the transaction to "updhold" for final processing, no need to call
      // currency sweep processor after PI or BA because none of these have any
      // cash impact
      zErr = UpdateHold(zTr, zPortmain, zTrDesc, 0, NULL, sCurrPrior,
                        bDoTransaction);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;

      // Save the basis adjustment transactions in a table and post them after
      // all the PIs are posted so that all PIs can be combined together with a
      // single xref_trans_no and all BAs are combined together with another
      // xref_trans_no.
      strcpy_s(zTr.sTranType, "BA");
      strcpy_s(zTr.sBalToAdjust, "CURR");
      zTr.fPcplAmt = fPhantomIncome;
      zTr.fIncomeAmt = 0;
      zTr.lTransNo = zTr.lXrefTransNo = 0;
      strcpy_s(zTr.sMiscDescInd, "N");

      // Add the transaction to the TIPS table
      zErr = AddTransToTransTable2(&zTIPSAdjustments, zTr);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
        InitializeTransTable2(&zTIPSAdjustments); // cleanup in case of an error
        return zErr;
      }

      // Adjust the cost on the (closing) transaction by the amount of previous
      // BA which was just created for this taxlot.
      if (strcmp(zTr.sDrCr, "DR") == 0)
        fPhantomIncome *= -1;

      if (strcmp(zTIPSTrades.pzTrans[i].sTranType, "RV") == 0)
        fPhantomIncome *= -1;

      zTIPSTrades.pzTrans[i].fTotCost += fPhantomIncome;
    } // for each lot, generate phantom income

    // now go through table with just save BAs and post them
    for (i = 0; i < zTIPSAdjustments.iCount; i++) {
      zTr = zTIPSAdjustments.pzTrans[i];

      zErr = IncrementPortmainLastTransNo(&zPortmain, zTr.iID);
      if (zErr.iSqlError != 0)
        return zErr;

      // Combine all BA with xref trans no
      zTr.lTransNo = zPortmain.lLastTransNo;
      if (i == 0)
        lXrefTransNo = zTr.lTransNo;
      zTr.lXrefTransNo = lXrefTransNo;

      // Pass the transaction to "updhold" for final processing, no need to call
      // currency sweep processor after PI or BA because none of these have any
      // cash impact
      zErr = UpdateHold(zTr, zPortmain, zTrDesc, 0, NULL, sCurrPrior,
                        bDoTransaction);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    } // post basis adjustments

    // now go through the trade table again, to post the closing trades
    for (i = 0; i < zTIPSTrades.iCount; i++) {
      *pzTrans = zTIPSTrades.pzTrans[i];

      zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
      if (zErr.iSqlError != 0)
        return zErr;

      // Combine all closing transactions with xref trans no
      pzTrans->lTransNo = zPortmain.lLastTransNo;
      if (i == 0)
        *plFirstTransNo = pzTrans->lTransNo;
      pzTrans->lXrefTransNo = *plFirstTransNo;
      // if (bFirstTrade == FALSE)
      //{
      // bFirstTrade = TRUE;
      //*plFirstTransNo = zPortmain.lLastTransNo;
      //}

      // pzTrans->lXrefTransNo = *plFirstTransNo;

      /* Pass the transaction to "updhold" for final processing */
      zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems,
                        pzPayTran, sCurrPrior, bDoTransaction);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;

      zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                    bDoTransaction, ADDVALUE, bInit);
      bInit = FALSE;
      bUpdateHoldCalled = TRUE;
    } // post each closing transaction
  } // TIPS transaction

  if (bUpdateHoldCalled)
    zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior,
                                  bDoTransaction, JUSTWRITEVALUE, bInit);

  return zErr;
} /* End TransactionProcess function */

/**
** Function to process rest of aggregate sell by creating technical positions
**/
ERRSTRUCT ProcessTechnicals(TRANS *pzTrans, TRANTYPE zTranType,
                            TRADEINFO *pzOriginal, TRADEINFO *pzRemaining,
                            DTRANSDESC zDTransDesc[], int iNumDTItems,
                            char *sCurrPrior, long *plFirstTransNo,
                            BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  PORTMAIN zPortmain;
  double fBaseAmt;
  char sRecType[2];
  long lRecNo;
  BOOL bInit = TRUE, bCopyOrigFace;

  InitializeErrStruct(&zErr);

  if (pzTrans->lDtransNo != 0) {
    lRecNo = pzTrans->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTrans->lTransNo;
    strcpy_s(sRecType, "T");
  }

  /*
  ** If the security impact of the transaction is a positive one (add shares)
  ** this indicates a close of a short position, therefore the security
  ** xtend column is set to TL (tech long) otherwise if the security impact
  ** is minus one (subtract shares), set the security xtend column to TS
  ** (tech short)
  */
  if (zTranType.lSecImpact == 1)
    strcpy_s(pzTrans->sSecXtend, "TL");
  else if (zTranType.lSecImpact == -1)
    strcpy_s(pzTrans->sSecXtend, "TS");
  else
    return (PrintError("Invalid Security Impact", pzTrans->iID, lRecNo,
                       sRecType, 47, 0, 0, "TALOC - PRCESSTECH1", FALSE));

  /*
  ** Increase the last trans no in portdir file and assign that number to
  ** TransNo in Trans record
  */

  zErr = IncrementPortmainLastTransNo(&zPortmain, pzTrans->iID);
  if (zErr.iSqlError != 0)
    return zErr;

  /*
  ** Set the transaction number, taxlot number and the cross reference
  ** transaction number.  It is necessary to set thet tax lot number
  ** the same as the transaction number as technical trades are in
  ** effect opening a position
  */
  pzTrans->fUnits = pzRemaining->fUnits;
  pzTrans->lTransNo = zPortmain.lLastTransNo;
  if (*plFirstTransNo != 0)
    pzTrans->lXrefTransNo = *plFirstTransNo;
  else
    pzTrans->lXrefTransNo = pzTrans->lTransNo;

  pzTrans->lTaxlotNo = pzTrans->lTransNo;
  pzTrans->lOpenTrdDate = pzTrans->lTrdDate;
  pzTrans->lOpenStlDate = pzTrans->lStlDate;
  pzTrans->fOpenUnitCost = pzTrans->fUnitCost;
  pzTrans->fBaseOpenXrate = pzTrans->fBaseXrate;
  pzTrans->fSysOpenXrate = pzTrans->fSysXrate;

  /*
  ** SB 7/20/01 For a transfer of security or for SL where user has not
  * specified an original
  ** face, it should not get copied from the remaining trade (because it will
  ** always be zero), it should be whatever has been previously calculated.
  */
  if (!IsValueZero(pzOriginal->fOrigFace, 3))
    bCopyOrigFace = TRUE;
  else
    bCopyOrigFace = FALSE;
  CopyToTransFromTradeInfo(*pzRemaining, pzTrans, bCopyOrigFace);

  /*
  ** Set the cost columns to match the principal amount of the
  ** transaction.  Check the security currency id against the principal
  ** currency and convert the values if necessary
  */
  if (strcmp(pzTrans->sSecCurrId, pzTrans->sCurrId) == 0) {
    pzTrans->fOrigCost = pzTrans->fPcplAmt;
    pzTrans->fTotCost = pzTrans->fOrigCost;
  } else {
    fBaseAmt = pzTrans->fPcplAmt / pzTrans->fBaseXrate;
    pzTrans->fOrigCost = fBaseAmt * pzTrans->fSecBaseXrate;
    pzTrans->fTotCost = pzTrans->fOrigCost;
  }

  /* Pass the transaction to "updhold" for final processing */
  zErr = UpdateHold(*pzTrans, zPortmain, zDTransDesc, iNumDTItems, NULL,
                    sCurrPrior, bDoTransaction);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  zErr = CurrencySweepProcessor(*pzTrans, zPortmain, sCurrPrior, bDoTransaction,
                                ADDANDWRITEVALUE, bInit);

  return zErr;
} /* ProcessTechnicals */

/**
        This routine handles the currency sweep
        iActions = 1 - Add value
                                                 2 - Add and write
                                                 31 - Just write
**/
ERRSTRUCT CurrencySweepProcessor(TRANS zTrans, PORTMAIN zPortmain,
                                 char *sCurrPrior, BOOL bDoTransaction,
                                 int iAction, BOOL bInit) {
  ERRSTRUCT zErr;
  static TRANS zNewTrans;
  static TRANTYPE zTranType;
  static double fTotIncomeAmt = 0;
  ASSETS zAssets;
  DTRANSDESC zDTransDesc;
  int iNumDTItems = 0;
  char sMsg[80];

  InitializeErrStruct(&zErr);

  if (((strcmp(zTrans.sCurrId, zPortmain.sBaseCurrId) != 0 &&
        zTrans.fPcplAmt != 0) ||
       (strcmp(zTrans.sIncCurrId, zPortmain.sBaseCurrId) != 0 &&
        zTrans.fIncomeAmt != 0)) &&
      strcmp(zTrans.sTranType, "TC") != 0 &&
      strcmp(zTrans.sRevType, "TC") != 0 &&
      strcmp(zPortmain.sCurrHandler, "T") == 0) {
    if (strcmp(zTrans.sTranType, "RV") == 0) {
      lpprSelectTrantype(&zTranType, zTrans.sRevType, zTrans.sDrCr, &zErr);
      zTranType.lCashImpact *= -1;
      sprintf_s(sMsg, "Trantype: %s, DrCr: %s Not Found In Trantype Table",
                zTrans.sRevType, zTrans.sDrCr);
    } else {
      lpprSelectTrantype(&zTranType, zTrans.sTranType, zTrans.sDrCr, &zErr);
      sprintf_s(sMsg, "Trantype: %s, DrCr: %s Not Found In Trantype Table",
                zTrans.sTranType, zTrans.sDrCr);
    }
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return (PrintError(sMsg, zTrans.iID, zTrans.lTransNo, "T", 46, 0, 0,
                         "TALOC - CURRPROC1", FALSE));

    /*If the zNewTrans is not initailized before then initialize it*/
    if (bInit) {

      strcpy_s(zNewTrans.sTranType, "TC");
      zNewTrans.iID = zTrans.iID;
      strcpy_s(zNewTrans.sWi, zTrans.sWi);
      strcpy_s(zNewTrans.sXWi, "N");

      /* if its a Open*/
      if (zTranType.lCashImpact == -1) {
        strcpy_s(zNewTrans.sSecNo,
                 zPortmain.sBaseCurrId); // zTrans.sBaseCurrId);
        strcpy_s(zNewTrans.sXSecNo, zTrans.sCurrId);
        zNewTrans.fBaseXrate = 1 / zTrans.fBaseXrate;
        strcpy_s(zNewTrans.sDrCr, "DR");
      }
      /* if its a Close*/
      else if (zTranType.lCashImpact == 1) {
        strcpy_s(zNewTrans.sXSecNo, zPortmain.sBaseCurrId);
        strcpy_s(zNewTrans.sSecNo, zTrans.sCurrId);
        zNewTrans.fBaseXrate = zTrans.fBaseXrate;
        strcpy_s(zNewTrans.sDrCr, "CR");
      } else // no impact or two sided impact, nothing to do
        return zErr;

      lpprSelectAsset(&zAssets, zNewTrans.sSecNo, zNewTrans.sWi, -1, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        if (zErr.iSqlError == SQLNOTFOUND) {
          sprintf_s(sMsg, "Security: %s, Wi: %s Not Found In Asset Table",
                    zNewTrans.sSecNo, zNewTrans.sWi);
          zErr = PrintError(sMsg, zTrans.iID, zTrans.lTransNo, "T", 8, 0, 0,
                            "TALOC - CURRPROC2", FALSE);
        }

        return zErr;
      }
      zNewTrans.iSecID = zAssets.iID;

      lpprSelectAsset(&zAssets, zNewTrans.sXSecNo, zNewTrans.sXWi, -1, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        if (zErr.iSqlError == SQLNOTFOUND) {
          sprintf_s(sMsg, "Security: %s, Wi: %s Not Found In Asset Table",
                    zNewTrans.sXSecNo, zNewTrans.sXWi);
          zErr = PrintError(sMsg, zTrans.iID, zTrans.lTransNo, "T", 8, 0, 0,
                            "TALOC - CURRPROC3", FALSE);
        }

        return zErr;
      }
      zNewTrans.iXSecID = zAssets.iID;

      zNewTrans.fUnitCost = 1.00;
      strcpy_s(zNewTrans.sAcctType, zTrans.sAcctType);
      strcpy_s(zNewTrans.sSecSymbol, zNewTrans.sSecNo);
      zNewTrans.lTrdDate = zTrans.lTrdDate;
      zNewTrans.lStlDate = zTrans.lStlDate;
      zNewTrans.lEffDate = zTrans.lEffDate;
      zNewTrans.lEntryDate = zTrans.lEntryDate;
      zNewTrans.lTaxlotNo = 0;
      zNewTrans.lXrefTransNo = 0;
      zNewTrans.iXID = zTrans.iID;
      strcpy_s(zNewTrans.sSecXtend, "RP");
      strcpy_s(zNewTrans.sAcctMthd, zTrans.sAcctMthd);
      strcpy_s(zNewTrans.sCurrId, zNewTrans.sSecNo);
      strcpy_s(zNewTrans.sXSecXtend, "RP");
      strcpy_s(zNewTrans.sXAcctType, zTrans.sAcctType);
      zNewTrans.fOrigCost = 0;
      // zNewTrans.fTotCost = zTrans.fPcplAmt;
      strcpy_s(zNewTrans.sXCurrId, zNewTrans.sXSecNo);
      strcpy_s(zNewTrans.sCurrAcctType, zTrans.sCurrAcctType);
      strcpy_s(zNewTrans.sIncCurrId, zNewTrans.sCurrId);
      strcpy_s(zNewTrans.sXCurrAcctType, zTrans.sCurrAcctType);
      zNewTrans.fIncBaseXrate = zNewTrans.fBaseXrate;
      zNewTrans.fSecBaseXrate = zNewTrans.fBaseXrate;
      zNewTrans.fAccrBaseXrate = zNewTrans.fBaseXrate;
      zNewTrans.fSysXrate = zNewTrans.fBaseXrate;
      zNewTrans.fIncSysXrate = zNewTrans.fBaseXrate;
      zNewTrans.fBaseOpenXrate = zNewTrans.fBaseXrate;
      zNewTrans.fSysOpenXrate = zNewTrans.fBaseXrate;
      zNewTrans.fIncomeAmt = 0;
      strcpy_s(zNewTrans.sIncAcctType, zTrans.sIncAcctType);
      strcpy_s(zNewTrans.sAccrCurrId, zNewTrans.sIncCurrId);
      strcpy_s(zNewTrans.sSecCurrId, zNewTrans.sCurrId);
      strcpy_s(zNewTrans.sTransSrce, "M"); // zTrans.sTransSrce);
      strcpy_s(zNewTrans.sCreatedBy, "CURRPROC");
      zNewTrans.lCreateDate = zTrans.lCreateDate;
      // strcpy_s(zNewTrans.sCreateTime,zTrans.sCreateTime);
      _strtime(zNewTrans.sCreateTime);
      zNewTrans.lPostDate = zTrans.lPostDate;
      zNewTrans.lBkofSeqNo = zTrans.lBkofSeqNo;
      zNewTrans.lDtransNo = zTrans.lDtransNo;
      strcpy_s(zNewTrans.sBrokerCode, zTrans.sBrokerCode);
      strcpy_s(zNewTrans.sMiscDescInd, "N");
    }

    /*If iAction is add value or addand write save the income amount of trans*/
    if (iAction == ADDVALUE || iAction == ADDANDWRITEVALUE)
      fTotIncomeAmt += zTrans.fIncomeAmt;

    /* if iAction is Add to existing then add the values and go for next trans*/
    if (iAction == ADDVALUE) {
      /* if its a open*/
      if (zTranType.lCashImpact == -1 || zTranType.lCashImpact == 1) {
        zNewTrans.fUnits += zTrans.fPcplAmt;
        zNewTrans.fPcplAmt += zTrans.fPcplAmt;
        //				zNewTrans.fTotCost += zTrans.fPcplAmt;
      }

      return zErr;
    }
    /* if its a close or open */
    if ((zTranType.lCashImpact == 1 || zTranType.lCashImpact == -1) &&
        (iAction == ADDANDWRITEVALUE)) {
      zNewTrans.fUnits = zTrans.fPcplAmt;
      zNewTrans.fPcplAmt = zTrans.fPcplAmt;
      zNewTrans.fTotCost = zNewTrans.fPcplAmt;
    }

    if (zTranType.lCashImpact == 1) {
      zNewTrans.fPcplAmt = zNewTrans.fPcplAmt / zTrans.fBaseXrate;
      zNewTrans.fUnits = zNewTrans.fPcplAmt;
      zNewTrans.fTotCost = zNewTrans.fPcplAmt;
    }
    // If action is add and write or just write then create the TC TRANS
    if (iAction == ADDANDWRITEVALUE || iAction == JUSTWRITEVALUE) {
      if (zNewTrans.fPcplAmt != 0) {
        zErr = IncrementPortmainLastTransNo(&zPortmain, zNewTrans.iID);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

        zNewTrans.lTransNo = zPortmain.lLastTransNo;

        /* Pass the transaction to "updhold" for final processing */
        zErr = UpdateHold(zNewTrans, zPortmain, &zDTransDesc, iNumDTItems, NULL,
                          sCurrPrior, bDoTransaction);
        if (zErr.iSqlError || zErr.iBusinessError)
          return (PrintError("Error Creating TC Trans", zNewTrans.iID, 0, "D",
                             zErr.iBusinessError, zErr.iSqlError,
                             zErr.iIsamCode, "TALOC CURRPROC4", FALSE));
      }

      // II Transaction
      strcpy_s(zNewTrans.sTranType, "TC");
      strcpy_s(zNewTrans.sSecXtend, "IN");
      strcpy_s(zNewTrans.sXSecXtend, "IN");
      zNewTrans.fIncomeAmt = 0;

      if (zTranType.lCashImpact == 1) {
        zNewTrans.fPcplAmt = fTotIncomeAmt / zTrans.fIncBaseXrate;
        zNewTrans.fUnits = zNewTrans.fTotCost = zNewTrans.fPcplAmt;
        strcpy_s(zNewTrans.sDrCr, "CR");
        zNewTrans.fBaseXrate = zTrans.fIncBaseXrate;
      } else {
        zNewTrans.fPcplAmt = fTotIncomeAmt;
        zNewTrans.fUnits = fTotIncomeAmt;
        zNewTrans.fTotCost = fTotIncomeAmt;
        strcpy_s(zNewTrans.sDrCr, "DR");
        zNewTrans.fBaseXrate = 1 / zTrans.fIncBaseXrate;
      }

      if (fTotIncomeAmt != 0) {
        zErr = IncrementPortmainLastTransNo(&zPortmain, zNewTrans.iID);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

        zNewTrans.lTransNo = zPortmain.lLastTransNo;
        zNewTrans.fIncBaseXrate = zNewTrans.fBaseXrate;
        zNewTrans.fSecBaseXrate = zNewTrans.fBaseXrate;
        zNewTrans.fAccrBaseXrate = zNewTrans.fBaseXrate;
        zNewTrans.fSysXrate = zNewTrans.fBaseXrate;
        zNewTrans.fIncSysXrate = zNewTrans.fBaseXrate;
        zNewTrans.fBaseOpenXrate = zNewTrans.fBaseXrate;
        zNewTrans.fSysOpenXrate = zNewTrans.fBaseXrate;
        /* Pass the transaction to "updhold" for final processing */
        zErr = UpdateHold(zNewTrans, zPortmain, &zDTransDesc, iNumDTItems, NULL,
                          sCurrPrior, bDoTransaction);
        if (zErr.iSqlError || zErr.iBusinessError)
          return (PrintError("Error Creating II TC Trans", zNewTrans.iID,
                             zErr.iBusinessError, "D", 0, zErr.iSqlError,
                             zErr.iIsamCode, "TALOC CURRPROC5", FALSE));
      }

      InitializeTransStruct(&zNewTrans);
      fTotIncomeAmt = 0;
    }
  }

  return zErr;
} // CurrencySweepProcessor