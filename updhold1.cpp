/**
 *
 * SUB-SYSTEM: UpdateHold
 *
 * FILENAME: updhold1.c
 *
 * DESCRIPTION: This file has the library function to update Holdings and
 *              Holdcash files based on the supplied Transaction information.
 *              Some of the functions used by this program are in updhold2.c
 *              (mostly databse routines to select, delete, update tables) file.
 *              This function also calls UpdateBalances(another library fnctn)
 *              to update portbal and perbal tables
 *
 * PUBLIC FUNCTION(S): UpdateHold
 *
 * NOTES: This function is called from transaction processor or pricing.
 *        The calling routine should have BEGIN TRANSACTION and END TRANSACTION.
 *
 * USAGE: int UpdateHold(TRANS zTR, PORTMAIN zPR, char *sCurPrior,
 *                       DTRANSDESC zDtransDesc[], int iNumDtransDesc)
 *           zTR            is the transaction record filled with the trade to
 *                          be processed
 *           zPR            is the portdir record, this is needed becuase this
 *                          routine is responsible to update LastTransNo(which
 * is incremented by TranAlloc function) in portdir table. sCurPrior      is a
 * flag telling us which program is calling this routine. Transaction Processor
 * will call it with "C"(current) and roll will call it with "P"(prior).
 *           zDtransDesc[]  is an array of dtrans desc info
 *           iNumDtransDesc is the number of elements in zDtransDesc array
 *
 * AUTHOR: Shobhit Barman ( EFFRON ENTERPRISES, INC. )
 *
 * $Header:   J:/Performer/Data/archives/Master/C/UPDHOLD1.C-arc   1.64   07 Sep
 * 2005 15:21:06   GriffinJ  $
 *
 *  VI#54726 Fixed to prevent negative Orig Face - mk
 **/

#include "transengine.h"

/**
** This is the library function to update Holdings and Holdcash files based on
** the supplied Transaction information. This function will be called by
** "transaction processor" and "roll". This need five arguments, the actual
** transaction record, portdir record, a flag telling us which program is
** calling this routine(transaction processor calls it with "C"(current) and
** roll calls it with "P"(prior), an array of dtransdesc entries and a count of
** number of entries in that arry.
** Following are the types of transaction this routine can handle:
**   M - Money Transaction
**   I - Income Transaction
**   O - Opening Transactions
**   C - Closing Transactions
**   A - Adjustment Transactions
**   S - Split Transactions
**   X - Transfer Transactions - valid only for Intra-account transfers.
**                               Inter-account transfers are never sent to this
**                               routine as one transaction rather they are
**                               broken by the calling function into a closing
**                               and an opening transaction.
**  Reversal transaction are also handled by this function
**
** Security positions and cash balances are stored in separate tables,
** positions reside in the HOLDINGS tables while cash balances are stored
** in the HOLDCASH tables.
**
History
// 2020-01-20 J#PER-10497 Fixed error for failing transaction when holdcash
overflow occurs -mk.
// 2019-01-08 J#PER-9403 Fail transaction when holdcash overflow occurs -mk.
// 2015-04-23 VI#57131 Missed a quote -mk.
// 2015-04-22 VI#57131 Set a variable to zero -mk.
*/
ERRSTRUCT STDCALL WINAPI UpdateHold(TRANS zTR, PORTMAIN zPR,
                                    DTRANSDESC zDtransDesc[],
                                    int iNumDtransDesc, PAYTRAN *pzPayTran,
                                    char *sCurPrior, BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  BOOL bReversal;
  int i;
  long lRecNo, lFirstOfMonth;
  short mdy[3];
  char sRecType[2];
  TRANTYPE zTranType;

  InitializeErrStruct(&zErr);
  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  if (sCurPrior[0] != 'C' && sCurPrior[0] != 'P')
    return (PrintError("Invalid Current/Prior Indicator", zTR.iID, lRecNo,
                       sRecType, 102, 0, 0, "UPDATE HOLD1", FALSE));

  /*
  ** The trantype table provides the processing parameters for a transaction
  ** security imapct and cash impact, the table is accessed using the
  ** transaction type from the trans table.  If the transaction is a reversal
  ** use the rev type column on the trans table for access, this column
  ** contains the original transaction type
  */
  if (strcmp(zTR.sTranType, "RV") == 0) {
    bReversal = TRUE;
    strcpy_s(zTranType.sTranType, zTR.sRevType);
  } else {
    bReversal = FALSE;
    strcpy_s(zTranType.sTranType, zTR.sTranType);
  }

  /* Get the trantype row for the transaction type */
  strcpy_s(zTranType.sDrCr, zTR.sDrCr);

  lpprSelectTrantype(&zTranType, zTranType.sTranType, zTranType.sDrCr, &zErr);
  if (zErr.iSqlError)
    return (PrintError("Error Selecting TRANTYPE Record", zTR.iID, lRecNo,
                       sRecType, 0, zErr.iSqlError, zErr.iIsamCode,
                       "UPDH SELECT TRANTYPE", FALSE));

  // If updathold is responsible for doing a database transaction, start a new
  // transaction
  bDoTransaction = bDoTransaction && (lpfnGetTransCount() == 0);
  if (bDoTransaction) {
    zErr.iBusinessError = lpfnStartTransaction();
    if (zErr.iBusinessError != 0)
      return zErr;
  }

  __try {

    //	lpfnTimer(9);
    /*
    ** If the transaction code is 'M'(money), run the process cash
    ** function only, if it is income, check for a entry in the divhist table
    ** and if necessary add/delete, then call process cash as
    ** both of these transaction codes have no security impact
    */
    if (zTranType.sTranCode[0] == 'M')
      zErr = ProcessCash(zTR, zTranType.lCashImpact, bReversal, sCurPrior,
                         zPR.bIncByLot);
    else if (zTranType.sTranCode[0] == 'I') {
      /*
      ** Only perform the process income function when the current
      ** prior column is set to 'C' and the autogen column from the
      ** the trantype is set to 'Y' - (RD, RI transactions).
      ** The reason is that this function maintains the divhist table
      ** which is only updated/maintained for current processing
      */
      zErr = ProcessIncome(zTR, zPR.bIncByLot, sCurPrior, bReversal);
      if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
        zErr = ProcessCash(zTR, zTranType.lCashImpact, bReversal, sCurPrior,
                           zPR.bIncByLot);
    } else if (zTranType.sTranCode[0] == 'O')
    // Note that a tech-short or a tech-long closing also goes to opening
    /*  else if (zTranType.sTranCode[0] == 'O' || (zTranType.sTranCode[0] == 'C'
       && (strcmp(zTR.sSecXtend, "TS") == 0 || strcmp(zTR.sSecXtend, "TL") ==
       0))) */
    {
      zErr = ProcessOpeningInHoldings(
          zTR, zPR, zTranType.lSecImpact, zTranType.lCashImpact,
          zTranType.iTradeDateInd, bReversal, sCurPrior);
      if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
        zErr = ProcessCash(zTR, zTranType.lCashImpact, bReversal, sCurPrior,
                           zPR.bIncByLot);
    } else if (zTranType.sTranCode[0] == 'C') {
      zErr = ProcessClosingInHoldings(
          zTR, zPR.sBaseCurrId, zTranType.lSecImpact, bReversal, sCurPrior);
      if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
        zErr = ProcessCash(zTR, zTranType.lCashImpact, bReversal, sCurPrior,
                           zPR.bIncByLot);
    } else if (zTranType.sTranCode[0] == 'S') /* Split Transaction */
      zErr = ProcessSplitInHoldings(zTR, zTranType.lSecImpact,
                                    zTranType.sAutoGen, bReversal, sCurPrior);
    else if (zTranType.sTranCode[0] == 'A') /* Adjustment Transaction */
    {
      zErr = ProcessAdjustmentInHoldings(&zTR, zPR, zTranType.lSecImpact,
                                         zTranType.lCashImpact, bReversal,
                                         sCurPrior);
      if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
        zErr = ProcessCash(zTR, zTranType.lCashImpact, bReversal, sCurPrior,
                           zPR.bIncByLot);
    } else if (zTranType.sTranCode[0] == 'X') /* Transfer Transaction */
    {
      if (zTR.iID != zTR.iXID) {
        if (bDoTransaction) // in case of an error, rollback transaction
          lpfnRollbackTransaction();
        return (PrintError("Inter Account Transfers Are Not Valid", zTR.iID,
                           lRecNo, sRecType, 7, 0, 0, "UPDATE HOLD2", FALSE));
      }

      zErr = ProcessTransferInHoldings(
          zTR, zPR, zPR.sBaseCurrId, zTranType.lSecImpact,
          zTranType.lCashImpact, zTranType.iTradeDateInd, bReversal, sCurPrior);
      if (zErr.iBusinessError == 0 && zErr.iSqlError == 0 &&
          zTranType.lCashImpact != 0)
        zErr = ProcessTransferInHoldcash(zTR, zPR, zTranType.lCashImpact,
                                         zTranType.iTradeDateInd, bReversal,
                                         sCurPrior);
    } else {
      zErr = PrintError("Invalid TranCode", zTR.iID, lRecNo, sRecType, 103, 0,
                        0, "UPDATE HOLD3", FALSE);
      if (bDoTransaction) // in case of an error, rollback transaction
        lpfnRollbackTransaction();
      return zErr;
    } // invalid trancode

    //	lpfnTimer(10);

    /*
    ** Set the perf date on the transaction record. The default is effective
    ** date, unless the trade is an 'as-of' for the prior month, then the
    ** setting is the first date of the current month -
    ** use post date (which should be equal to Starsdat.TradeDate - this is
    * important
    ** for trans import app when run for last business day of the month
    ** calendar eom falls on weekend, e.g. 9/29/06 file gets processed on
    * 9/30/06
    ** and starsdate is (9/29/96, 10/2/06)
    ** If post date = 0 (not filled in by calling app), then use Entry Date
    */
    if (zTR.lPostDate != 0)
      lpfnrjulmdy(zTR.lPostDate, mdy);
    else
      lpfnrjulmdy(zTR.lEntryDate, mdy);

    mdy[1] = 01;
    lpfnrmdyjul(mdy, &lFirstOfMonth);

    if (zTR.lEffDate < lFirstOfMonth)
      zTR.lPerfDate = lFirstOfMonth;
    else
      zTR.lPerfDate = zTR.lEffDate;

    /*
    ** If no error has occured and we are dealing with current positions, then
    ** insert the trans, trans_desc and PayTran records, if any
    */
    if (zErr.iBusinessError == 0 && zErr.iSqlError == 0 &&
        sCurPrior[0] == 'C') {
      lpprInsertTrans(zTR, &zErr);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
        if (bDoTransaction) // in case of an error, rollback transaction
          lpfnRollbackTransaction();
        return zErr;
      }

      // lpfnTimer(11);
      if ((pzPayTran) && (pzPayTran->iID != 0)) {
        // overwrite DtransNo with actual number assigned to the trans
        pzPayTran->lTransNo = zTR.lTransNo;
        lpprInsertPayTran(*pzPayTran, &zErr);
        if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
          if (bDoTransaction) // in case of an error, rollback transaction
            lpfnRollbackTransaction();
          return zErr;
        }
      }

      for (i = 0; i < iNumDtransDesc; i++) {
        zDtransDesc[i].lDtransNo = zTR.lTransNo;
        zDtransDesc[i].iSeqno = i + 1;
        if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
          lpprInsertTransDesc(zDtransDesc[i], &zErr);
        else
          break;
      } /* for i < NumDtransDesc */
    } /* if no sql or business error and we are processing current positions */

    /*
    ** If no error has occured and we are dealing with current positions, then
    ** update the last transaction number column on the portdir(tranproc
    ** increaments this number but this routine is responsible for actually
    ** updating it), the value should be identical to the value stored in the
    ** transaction number on the trans table
    */
    if (zErr.iBusinessError == 0 && zErr.iSqlError == 0 &&
        sCurPrior[0] == 'C') {
      // lpfnTimer(12);
      lpprUpdatePortmainLastTransNo(zPR.lLastTransNo, zPR.iID, &zErr);
      // lpfnTimer(13);
      if (zErr.iSqlError || zErr.iBusinessError) {
        zErr = PrintError("Error updating PORTMAIN Record", zTR.iID, lRecNo,
                          sRecType, 0, zErr.iSqlError, zErr.iIsamCode,
                          "UPDATE HOLD4", FALSE);
        if (bDoTransaction) // in case of an error, rollback transaction
          lpfnRollbackTransaction();
        return zErr;
      }
    } // no error and current prior flag is "C"

    /*
    ** If there have been sucessful updates to the holdings, holdcash, trans and
    ** portmain tables, the final step is to update the values on the portbal
    * table
    */
    // SB 9/28/1999 - Not doing portbal anymore
    //  if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
    //  zErr = UpdateBalances(zTR, zPR);

    // If updatehold is responsible for database transaction, rollback or commit
    // depending on whether there was an error in any of the previous steps or
    // not
  } // try
  __except (lpfnAbortTransaction(bDoTransaction)) {
  }

  if (bDoTransaction) {
    if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
      lpfnCommitTransaction();
    else
      lpfnRollbackTransaction();
  } // if bDoProcessing

  return zErr;
} /* UpdateHold */

/**
** This function applies the cash impact of the transaction.  The function
** uses a combination of currency id and currency account type from the
** transaction to determine the correct cash balance to adjust.
** Once the correct balance has been identified, the ProcessMoneyinHoldings
** function is invoked to update the balance
**/
ERRSTRUCT ProcessCash(TRANS zTR, long lCashImpact, BOOL bReversal,
                      char *sCurPrior, BOOL bIncByLot) {
  ERRSTRUCT zErr;
  long lRecNo;
  char sRecType[2];
  BOOL bUseSecXRate;

  InitializeErrStruct(&zErr);
  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  /* Tech Short or Tech Long Closing Transactions have no cash impact */
  if (strcmp(zTR.sSecXtend, "TL") == 0 || strcmp(zTR.sSecXtend, "TS") == 0)
    lCashImpact = 0;

  /*
  ** If there is no cash impact, return without doing anything, else if the
  ** cash impact is anything other than 1 or -1, return an error
  */
  if (lCashImpact == 0 || lCashImpact == 2)
    return zErr;
  else if (lCashImpact != 1 && lCashImpact != -1)
    return (PrintError("Invalid Cash Impact", zTR.iID, lRecNo, sRecType, 48, 0,
                       0, "UPDH PROCESS CASH1", FALSE));

  /*
  ** SB 10/3/2012 - Opening part (cash impact = 1) for TC/DR should use sec
  * exrate and all other transactions should use *
  *				base exrate. The decision regarding whether to
  * use base exrate or sec exrate was being made later in *
  *					the function. In case of reversal of
  * TC/DR, it was too late becuase that decision was being made *
  *						after multiplying cash impact by
  * -1. So, reversal of TC/DR used to end up using base exrate rather *
  *						than sec exrate, which was
  * messing up the exchange rate on the holdcash record. So, make and save *
  *							the decision about which
  * exchange rate to use before modifying cash impact in any way.
  */
  if (!bReversal && strcmp(zTR.sTranType, "TC") == 0 &&
      strcmp(zTR.sDrCr, "DR") == 0 && lCashImpact == 1)
    bUseSecXRate = TRUE;
  else if (bReversal && strcmp(zTR.sRevType, "TC") == 0 &&
           strcmp(zTR.sDrCr, "DR") == 0 && lCashImpact == 1)
    bUseSecXRate = TRUE;
  else
    bUseSecXRate = FALSE;

  /*
  ** Cash impact is based on the type of transaction, a reversal
  ** will have the opposite cash impact of the original transaction
  */
  if (bReversal)
    lCashImpact *= -1;

  /*
  ** Using the zTr.sCurrId field, access the currency table to obtain the
  ** correct security number for the principal currency
  */
  //	lpprTimer(39);
  lpprCurrencySecno(zTR.sSecNo, zTR.sWi, &zTR.iSecID, zTR.sCurrId, &zErr);
  //	lpprTimer(40);
  if (zErr.iSqlError)
    return (PrintError("Error Reading CURRENCY", zTR.iID, lRecNo, sRecType, 0,
                       zErr.iSqlError, zErr.iIsamCode, "UPDH PROCESS CASH2",
                       FALSE));

  // SB - Changed on 10/19/98 so that secxtend gets copied with "RP" if it is
  // not IN
  if (strcmp(zTR.sSecXtend, "IN") != 0)
    strcpy_s(zTR.sSecXtend, "RP");
  /* Process the principal portion of the trade */
  /* If TC DR trade use security exchange rate if it is the foreign currency
   * (cashimpart = 1) */
  if (bUseSecXRate)
    zErr = ProcessMoneyInHoldings(zTR, lCashImpact, zTR.fPcplAmt,
                                  zTR.fSecBaseXrate, zTR.fSysXrate, sCurPrior,
                                  bReversal, bIncByLot);
  else
    zErr =
        ProcessMoneyInHoldings(zTR, lCashImpact, zTR.fPcplAmt, zTR.fBaseXrate,
                               zTR.fSysXrate, sCurPrior, bReversal, bIncByLot);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
  ** Process the income portion of the trade, only if the income amount
  ** is not zero, if the income currency id does not match the principal
  ** currency id, retrieve the correct security id
  */
  if (!IsValueZero(zTR.fIncomeAmt, 2)) {
    if (strcmp(zTR.sCurrId, zTR.sIncCurrId) != 0) {
      lpprCurrencySecno(zTR.sSecNo, zTR.sWi, &zTR.iSecID, zTR.sIncCurrId,
                        &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return (PrintError("Error Reading CURRENCY", zTR.iID, lRecNo, sRecType,
                           0, zErr.iSqlError, zErr.iIsamCode,
                           "UPDH PROCESS CASH3", FALSE));
    } /* If CurrId != IncCurrId */

    strcpy_s(zTR.sCurrAcctType, zTR.sIncAcctType);
    strcpy_s(zTR.sSecXtend, "IN"); // force sec_xtend to be IN for income amout
    if (bUseSecXRate)
      zErr = ProcessMoneyInHoldings(zTR, lCashImpact, zTR.fIncomeAmt,
                                    zTR.fSecBaseXrate, zTR.fIncSysXrate,
                                    sCurPrior, bReversal, bIncByLot);
    else
      zErr = ProcessMoneyInHoldings(zTR, lCashImpact, zTR.fIncomeAmt,
                                    zTR.fIncBaseXrate, zTR.fIncSysXrate,
                                    sCurPrior, bReversal, bIncByLot);
  } /* Income amount is not zero */

  return zErr;
} /* processcash */

/**
** This function is responsible for adjusting the values in the cash balance
** record by the cash adjustment(in some cases it will be pcplamt on the
** transaction, in other cases it will be pcplamt + accr int on the transaction)
**/
ERRSTRUCT ProcessMoneyInHoldings(TRANS zTR, long lCashImpact, double fCashAdj,
                                 double fBaseXrate, double fSysXrate,
                                 char *sCurPrior, BOOL bReversal,
                                 BOOL bIncByLot) {
  BOOL bRecFound;
  ERRSTRUCT zErr;
  HOLDCASH zHCR;

  InitializeErrStruct(&zErr);

  if (fCashAdj == 0.0)
    return zErr;

  /* Check and see if there is an existing cash record */
  InitializeHoldcashStruct(&zHCR);
  //	lpprTimer(41);
  lpprSelectHoldcash(&zHCR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                     zTR.sCurrAcctType, &zErr);
  //	lpprTimer(42);
  if (zErr.iSqlError == SQLNOTFOUND) {
    bRecFound = FALSE;
    zErr.iSqlError = 0;
  } else if (zErr.iSqlError == 0)
    bRecFound = TRUE;
  else
    return zErr;

  /* Calculate the new Base and System exchange rates */
  CalculateNewCostXRate(zHCR.fTotCost, fCashAdj, zHCR.fBaseCostXrate,
                        fBaseXrate, &zHCR.fBaseCostXrate);
  CalculateNewCostXRate(zHCR.fTotCost, fCashAdj, zHCR.fSysCostXrate, fSysXrate,
                        &zHCR.fSysCostXrate);

  /*
  ** Multiply the cash to be adjusted(in most cases it will be principal amount
  ** on the transaction. Check processcsh function to see when it is different)
  ** by the value of the cash impact. The result of this computation
  ** is that the resulting cash adjustment can always be added to cash
  ** balance and there is no need to check the Cash Impact value and
  ** decide whether to add or subtract the cash adjustment
  */
  fCashAdj = RoundDouble(fCashAdj * lCashImpact, 2);

  zHCR.fUnits += fCashAdj;
  zHCR.fTotCost += fCashAdj;
  zHCR.fMktVal += fCashAdj;

  // Round the units and cost to two decimal places
  zHCR.fUnits = RoundDouble(zHCR.fUnits, 2);
  zHCR.fTotCost = RoundDouble(zHCR.fTotCost, 2);

  if (!IsValueZero(zHCR.fUnits, 2))
    zHCR.fUnitCost = zHCR.fTotCost / zHCR.fUnits;
  else
    zHCR.fUnitCost = 0;

  zHCR.fMvBaseXrate = fBaseXrate;
  zHCR.fMvSysXrate = fSysXrate;

  zHCR.lTrdDate = zTR.lTrdDate;
  zHCR.lStlDate = zTR.lStlDate;
  zHCR.lEffDate = zTR.lEffDate;
  zHCR.lLastTransNo = zTR.lTransNo;

  /* Calculate the current currency gain and loss */
  CalculateCurrencyGainLoss(zHCR.fTotCost, zHCR.fBaseCostXrate, zHCR.fMktVal,
                            zHCR.fMvBaseXrate, &zHCR.fCurrencyGl);

  /* if record already exist, update it, else add a new record */
  if (bRecFound) {
    if (IsValueZero(zHCR.fUnits, 2))
      lpprDeleteHoldcash(zHCR.iID, zHCR.sSecNo, zHCR.sWi, zHCR.sSecXtend,
                         zHCR.sAcctType, &zErr);
    else {

      lpprUpdateHoldcash(zHCR, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return (PrintError("Error Updating Cash CURRENCY. Please contact "
                           "Client Support for an immediate fix.",
                           zTR.iID, 0, "C", 138, zErr.iSqlError, zErr.iIsamCode,
                           "UPDC PROCESS CASH5", FALSE));
    }
  } /* if recfound */
  else {
    zHCR.iID = zTR.iID;
    strcpy_s(zHCR.sSecNo, zTR.sSecNo);
    strcpy_s(zHCR.sWi, zTR.sWi);
    //  SB 10/19/99 Cash should only be Principal or Income
    if (strcmp(zTR.sSecXtend, "IN") == 0)
      strcpy_s(zHCR.sSecXtend, zTR.sSecXtend);
    else
      strcpy_s(zHCR.sSecXtend, "RP");
    strcpy_s(zHCR.sAcctType, zTR.sCurrAcctType);
    strcpy_s(zHCR.sSecSymbol, zTR.sSecNo);
    zHCR.iSecID = zTR.iSecID;
    zHCR.lAsofDate = zTR.lEffDate;

    if (!IsValueZero(zHCR.fUnits, 2))
      lpprInsertHoldcash(zHCR, &zErr);
  } /* Rec not found*/

  // 11/12/01 Added by VAY
  // LT/ST trans now can come through payment processor
  // so, we have to update divhist/accdiv properly as it is done for Income
  // trans Code below was borrowed from ProcessIncome and just tries to update
  // divhist/accdiv
  char sMoney[] = "M";
  zErr = UpdateDivhistAccdiv(zTR, bIncByLot, bReversal, sCurPrior, sMoney);

  return zErr;
} /* processmoney */

/**
** This function applies the impact of the income transaction against
** the divhist table.
**/
ERRSTRUCT ProcessIncome(TRANS zTR, BOOL bIncByLot, char *sCurPrior,
                        BOOL bReversal) {
  HOLDINGS zHR;
  PAYREC zPY;
  BOOL bRecFound = FALSE;
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  /*
  ** Update the following columns on the holdings lot :
  **    last payment date = trans.stl_date
  **    last payment type = trans.div_type
  **    last payment trans_no = trans.trans_no
  */

  /* Select the lot to be adjusted */
  if (zTR.lTaxlotNo != 0) {
    InitializeHoldingsStruct(&zHR);
    lpprSelectHoldings(&zHR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      bRecFound = FALSE;
    } else if (zErr.iSqlError == 0)
      bRecFound = TRUE;
    else
      return zErr;
  }

  if (bRecFound) {
    zHR.lLastPmtTrNo = zTR.lTransNo;
    zHR.lLastPmtDate = zTR.lEffDate;
    strcpy_s(zHR.sLastPmtType, zTR.sDivType);

    lpprUpdateHoldings(zHR, &zErr);
    if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
      return zErr;
  } /* Rec found */

  /*
  ** Update the Divhist row associated with the income payment
  ** If the transactions is a reversal of a payment, delete the divhist
  ** row, otherwise update the tran_location to 'T' and assign the transaction
  ** number to div_trans_no.
  */
  char sIncome[] = "I";
  zErr = UpdateDivhistAccdiv(zTR, bIncByLot, bReversal, sCurPrior, sIncome);
  if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
    return zErr;

  /* If the transaction is an accrued reclaim, add or delete a payrec row */
  if (strcmp(zTR.sTranType, "AR") == 0) {
    if (bReversal == FALSE) {
      CopyFieldsFromTransToPayrec(&zPY, zTR);
      lpprInsertPayrec(zPY, &zErr);
    } else
      lpprDeletePayrec(zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, zTR.lDivintNo, &zErr);
  } /* AR Trade */
  else if (strcmp(zTR.sTranType, "RR") == 0) {
    if (bReversal == FALSE)
      lpprDeletePayrec(zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, zTR.lDivintNo, &zErr);
    else {
      CopyFieldsFromTransToPayrec(&zPY, zTR);
      lpprInsertPayrec(zPY, &zErr);
    }
  } /* RR Trade */

  return zErr;
} /* ProcessIncome */

/**
** Function to process opening trade transactions
**/
ERRSTRUCT ProcessOpeningInHoldings(TRANS zTR, PORTMAIN zPR, long lSecImpact,
                                   long lCashImpact, int iDateInd,
                                   BOOL bReversal, char *sCurPrior) {
  BOOL bRecFound;
  ERRSTRUCT zErr;
  double fTradingUnits, fSysCost, fSysXrate, fIncSysXrate;
  char sPrimaryType[2], sSecondaryType[2], sPositionInd[2], sLotInd[2];
  char sCostInd[2], sLotExistsInd[2], sAvgInd[2], sCurrId[6], sRecType[2];
  long lRecNo;
  HOLDINGS zHR;
  HOLDCASH zHCR;

  InitializeErrStruct(&zErr);

  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  //	lpprTimer(43);
  zErr = GetSecurityCharacteristics(
      zTR.sSecNo, zTR.sWi, sPrimaryType, sSecondaryType, sPositionInd, sLotInd,
      sCostInd, sLotExistsInd, sAvgInd, &fTradingUnits, sCurrId);
  //	lpprTimer(44);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
  ** If the position indicator of the sec is'C' - cash, select the row
  ** holdcash table, otherwise always select the row from the HOLDINGS table.
  ** Once holdcash record is selected(if CASH), its contents are copied into
  ** a Holdings variable, so that at every step, we don't have to check whether
  ** we are trading currency or a regular security and do same processing twice.
  ** After all the processing is done and we have to add a new record, if we are
  ** processing Cash(currency), the required fields from Holdings are copied
  ** back to Holdcash structure and a holdcash record is inserted.
  */
  if (sPositionInd[0] == 'C') {
    InitializeHoldcashStruct(&zHCR);
    //		lpprTimer(45);
    lpprSelectHoldcash(&zHCR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, &zErr);
    // lpprTimer(46);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound = FALSE;
      zErr.iSqlError = 0;
    } else if (zErr.iSqlError == 0)
      bRecFound = TRUE;
    else
      return zErr;

    CopyFieldsFromHoldcashToHoldings(&zHR, zHCR);
  } /* if position indicator is set to 'C' for cash */
  else {
    InitializeHoldingsStruct(&zHR);
    //		lpprTimer(45);
    lpprSelectHoldings(&zHR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, &zErr);
    // lpprTimer(46);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound = FALSE;
      zErr.iSqlError = 0;
    } else if (zErr.iSqlError == 0)
      bRecFound = TRUE;
    else
      return zErr;
  } /* Not cash */

  /*
  ** This should not happen. If it's a reversal trade and LotExistsInd is "Y"
  ** then original Lot should always exist, if it doesn't then return with an
  ** error
  */
  if (bReversal && bRecFound == FALSE && sLotExistsInd[0] == 'Y')
    return (PrintError("Incorrect Or Missing Taxlot Number", zTR.iID, lRecNo,
                       sRecType, 101, 0, 0, "UPDH PROCESSOPEN1", FALSE));

  /* Calculate the sys cost xrate */
  if (lCashImpact != 0)
    fSysCost = zTR.fPcplAmt / zTR.fSysXrate;
  else
    fSysCost = zTR.fTotCost;

  if (!IsValueZero(fSysCost, 12))
    fSysXrate = zTR.fTotCost / fSysCost;
  else
    fSysXrate = 1.0;

  /* Calculate the accrual's system cost exchange rate */
  fSysCost = zTR.fIncomeAmt / zTR.fIncSysXrate;

  if (!IsValueZero(fSysCost, 12))
    fIncSysXrate = zTR.fAccrInt / fSysCost;
  else
    fIncSysXrate = 1;

  /* If the transaction is being reversed, flip the security impact */
  if (bReversal)
    lSecImpact *= -1;

  /*
  ** Adjust the transaction fields by the security impacts that to update the
  ** columns on the Holdings table, the routine need only perform an addition
  */
  zTR.fUnits *= lSecImpact;
  zTR.fTotCost *= lSecImpact;
  zTR.fPcplAmt *= lSecImpact;
  zTR.fOrigCost *= lSecImpact;
  zTR.fAccrInt *= lSecImpact;
  zTR.fOrigFace *= lSecImpact;

  /*
  ** For a regular trade, if it's a single lot security, don't add a
  ** new taxlot unless one already doesn't exists. For a reversal trade delete
  ** the record if after subtracting current trade, number of units become zero
  */

  /*
  ** If it's a single-lot security and record found, do the required processing
  ** and return
  */
  if (strcmp(sLotInd, "S") == 0 && bRecFound) {
    zHR.fUnits += zTR.fUnits;
    zHR.fTotCost += zTR.fTotCost;
    zHR.fMktVal += zTR.fPcplAmt;

    if (IsValueZero(zTR.fOrigCost, 3))
      zHR.fOrigCost += zTR.fTotCost;
    else
      zHR.fOrigCost += zTR.fOrigCost;

    /*
    ** if it's a reversal and after reversing trade, total units become zero,
    ** delete the record from holding table
    */
    if (bReversal && IsValueZero(zHR.fUnits, 5)) {
      if (sPositionInd[0] == 'C')
        lpprDeleteHoldcash(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                           zHR.sAcctType, &zErr);
      else
        lpprDeleteHoldings(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                           zHR.sAcctType, zHR.lTransNo, &zErr);

      return zErr;
    } /* if reversal & number of units = 0 */

    /*
    ** If the transaction is a contribution or transfer - CashImpact = 0
    ** pass the opening xrates from trade to function otherwise pass
    ** the market exchange rate
    */
    if (lCashImpact == 0) {
      CalculateNewCostXRate(zHR.fTotCost, zTR.fTotCost, zHR.fBaseCostXrate,
                            zTR.fBaseOpenXrate, &zHR.fBaseCostXrate);
      CalculateNewCostXRate(zHR.fTotCost, zTR.fTotCost, zHR.fSysCostXrate,
                            zTR.fSysOpenXrate, &zHR.fSysCostXrate);
    } /* No Cash Impact */
    else {
      CalculateNewCostXRate(zHR.fTotCost, zTR.fTotCost, zHR.fBaseCostXrate,
                            zTR.fSecBaseXrate, &zHR.fBaseCostXrate);
      CalculateNewCostXRate(zHR.fTotCost, zTR.fTotCost, zHR.fSysCostXrate,
                            fSysXrate, &zHR.fSysCostXrate);
    } /* Cash impact is not zero */

    /*
    ** If the transaction is a opening trade for a cash security, copy
    ** the columns from the HOLDINGS to the HOLDCASH row and update
    ** the holdcash table, otherwise update the HOLDINGS table
    */
    if (sPositionInd[0] == 'C') {
      CopyFieldsFromHoldingsToHoldcash(&zHCR, zHR);
      if (!IsValueZero(zHCR.fUnits, 5)) {
        lpprUpdateHoldcash(zHCR, &zErr);
        return zErr;
      } else // unlikely case of money market units were -ve before the
             // transaction and this buy is making it zero
      {
        lpprDeleteHoldcash(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                           zHR.sAcctType, &zErr);
        return zErr;
      }
    } /* Cash */
    else {
      lpprUpdateHoldings(zHR, &zErr);
      return zErr;
    }
  } /* if a money market or cash security and recfound = TRUE*/

  /*
  ** If the transaction is not a reversal, a lot does not already exist
  ** and the lot exists indicator is set to 'N', then format a holdings
  ** row based upon the values in the transaction record
  */
  /* CHECK AGAIN  MLS */
  if (bReversal == FALSE || (bRecFound == FALSE && sLotExistsInd[0] == 'N')) {
    CopyFieldsFromTransToHoldings(&zHR, zTR);
    if (IsValueZero(zHR.fOrigCost, 3))
      zHR.fOrigCost = zHR.fTotCost;

    /*
    ** If the opening transaction is part of transfer or a contribution, set the
    **  cost exchange rates equal to opening exchange rates on the transactions.
    */
    if (lCashImpact == 0) {
      zHR.fBaseCostXrate = zTR.fBaseOpenXrate;
      zHR.fSysCostXrate = zTR.fSysOpenXrate;
    }
  } /* Reversal is FALSE or (LotExistInd is N and rec not found) */

  /*
  ** If the portfolio is an average cost portfolio and the security is not
  ** a money market or a future, futures are not averaged and money markets
  ** and cash are always held in one lot
  ** Upon returning from the averge cost function, assign the new
  ** cost basis to the holding
  */
  if (zPR.sAcctMethod[0] == 'A' && sAvgInd[0] == 'Y') {
    zErr = ProcessAverageInHoldings(&zTR, fTradingUnits, &fSysXrate, bReversal,
                                    sCostInd);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    zHR.fUnitCost = zTR.fUnitCost; // SB 10/19/99 Changed so that all the lots
                                   // get new unit_cost
    zHR.fTotCost = zTR.fTotCost;
    zHR.fOrigCost = zTR.fOrigCost;
    zHR.fBaseCostXrate = zTR.fBaseXrate;
    zHR.fSysCostXrate = fSysXrate;
  }

  /*
  ** If it's a reversal trade then delete the taxlot else if it's a regular
  ** trade, finish formatting the record and add the lot
  */
  if (bReversal && bRecFound) {
    lpprDeleteHoldings(zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, &zErr);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    if (sCurPrior[0] == 'C') {
      // delete accruing (pending, i.e. not processed yet) divhist and accdiv
      // also
      lpprDeleteAccruingDivhistOneLot(zTR.iID, zTR.sSecNo, zTR.sWi,
                                      zTR.sSecXtend, zTR.sAcctType,
                                      zTR.lTaxlotNo, &zErr);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;

      lpprDeleteAccruingAccdivOneLot(zTR.iID, zTR.sSecNo, zTR.sWi,
                                     zTR.sSecXtend, zTR.sAcctType,
                                     zTR.lTaxlotNo, &zErr);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    } // if posting transactions
  } // if reversal & taxlot found
  else {
    if (sCostInd[0] == 'L') /* Futures */
    {
      zHR.fOpenLiability = zHR.fTotCost;
      zHR.fCurLiability = zHR.fOpenLiability;
      zHR.fTotCost = zHR.fOrigCost = zHR.fMktVal = zHR.fAccrInt = 0;
    }

    /* date indicator - 2, use settlement date else trade date
    if (iDateInd == 2)
      zHR.lEligDate = zTR.lStlDate;
    else
      zHR.lEligDate = zTR.lTrdDate;
     */

    // For a bonds, eligibility date for payments should be the settlement date,
    // unless the settmenet date is less than the effective date (e.g. in case
    // of FR or TS transactions), in which case it should be effective date. For
    // other type of security, eligibility date should be effective date.
    if (sPrimaryType[0] == 'B' && zTR.lStlDate > zTR.lEffDate)
      zHR.lEligDate = zTR.lStlDate;
    else
      zHR.lEligDate = zTR.lEffDate;

    /* if cash insert a holdcash record, else insert a holdings record */
    if (sPositionInd[0] == 'C') {
      CopyFieldsFromHoldingsToHoldcash(&zHCR, zHR);
      if (!IsValueZero(zHCR.fUnits, 2))
        lpprInsertHoldcash(zHCR, &zErr);
    } /* Cash */
    else {
      strcpy_s(zHR.sPrimaryType, sPrimaryType);
      lpprInsertHoldings(zHR, &zErr);
    }
  } /* if regular trade */

  return zErr;
} /* processopening */

/**
** Function to process closing trade transactions
**/
ERRSTRUCT ProcessClosingInHoldings(TRANS zTR, char *sBaseCurr, long lSecImpact,
                                   BOOL bReversal, char *sCurPrior) {
  BOOL bRecFound, bRecFound1;
  ERRSTRUCT zErr;
  double fCGL, fSGL, fTotGL, fSTGL, fMTGL, fLTGL;
  double fTradingUnits, fUnitPrice, fUnitAccrual, fUnitGL;
  double fMvBaseXrate, fMvSysXrate, fAiBaseXrate, fAiSysXrate;
  char sPrimaryType[3], sSecondaryType[3], sPositionInd[3], sLotInd[3];
  char sCostInd[3], sLotExistsInd[3], sAvgInd[3], sCurrId[6], sRecType[3];
  long lRecNo, lProcessDate;
  HOLDINGS zHR;
  HOLDCASH zHCR;
  HOLDDEL zHD;

  InitializeErrStruct(&zErr);
  fTradingUnits = fUnitPrice = fUnitAccrual = fUnitGL = 0;
  fMvBaseXrate = fMvSysXrate = fAiBaseXrate = fAiSysXrate = 0;
  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  zErr = GetSecurityCharacteristics(
      zTR.sSecNo, zTR.sWi, sPrimaryType, sSecondaryType, sPositionInd, sLotInd,
      sCostInd, sLotExistsInd, sAvgInd, &fTradingUnits, sCurrId);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
  ** If the closing trade is for a cash position, check the holdcash table for
  ** an existing position, otherwise if it is a non cash security check the
  ** holdings table
  ** If the transaction is not a cancel and the lot does exist, format
  ** the holddel record using the holdings record, set the create date and
  ** transaction number using the transaction's effective date and it's
  ** transaction number and insert into the holddel table
  */
  if (sPositionInd[0] == 'C') {
    InitializeHoldcashStruct(&zHCR);
    lpprSelectHoldcash(&zHCR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound = FALSE;
      zErr.iSqlError = 0;
    } else if (zErr.iSqlError == 0)
      bRecFound = TRUE;
    else
      return zErr;

    CopyFieldsFromHoldcashToHoldings(&zHR, zHCR);
  } /* Cash */
  else {
    InitializeHoldingsStruct(&zHR);
    lpprSelectHoldings(&zHR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound = FALSE;
      zErr.iSqlError = 0;
    } else if (zErr.iSqlError == 0)
      bRecFound = TRUE;
    else
      return zErr;
  } /* if postind is not "C"(cash) */

  if (bRecFound && sCurPrior[0] == 'C' && sLotInd[0] != 'S') {
    CopyFieldsFromHoldingsToHolddel(&zHD, zHR);
    zHD.lCreateDate = zTR.lEffDate;
    zHD.lCreateTransNo = zTR.lTransNo;

    /*
    ** The rev_trans_no from the transaction will only contain a
    ** value for cancel transactions, in all other cases it will be zero
    */
    zHD.lRevTransNo = zTR.lRevTransNo;

    lpprInsertHolddel(zHD, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      zErr.iBusinessError = GetLastError();
      return zErr;
    }

    /*
    ** If the transaction is a reversal, it is necessary to update
    ** the rev_trans_no on the original holddel row
    */
    if (bReversal) {
      lpprHolddelUpdate(zTR.iID, zTR.lTransNo, zTR.lRevTransNo, zTR.lEffDate,
                        &zErr);
      if (zErr.iSqlError) // Send A Warning Message
        PrintError("WARNING !!! Holddel Record Not Found-Potential Problms "
                   "With Dividend Payments",
                   zTR.iID, lRecNo, sRecType, 1001, 0, 0, "UPDH PROCESSCLOSE1",
                   TRUE);
    } /* Reversal */
  } /* If current processing and rec found */

  /* if it's a non cash trade and the record not found, return with an error */
  if (bReversal == FALSE && bRecFound == FALSE && sLotExistsInd[0] == 'Y')
    return (PrintError("Incorrect Or Missing Taxlot Number", zTR.iID, lRecNo,
                       sRecType, 100, 0, 0, "UPDH PROCESSCLOSE2", FALSE));

  /*
  ** If its a money market/cash trade and the lot does not exist and
  ** its not a reversal, format the holdings record
  */
  if (bReversal == FALSE && bRecFound == FALSE && sLotExistsInd[0] == 'N') {
    CopyFieldsFromTransToHoldings(&zHR, zTR);
    zHR.fUnits = zHR.fTotCost = zHR.fOrigCost = 0;
    zHR.fUnitCost = zTR.fUnitCost;
    zHR.fBaseCostXrate = zTR.fBaseOpenXrate;
    zHR.fSysCostXrate = zTR.fSysOpenXrate;
    zHR.lEffDate = zTR.lOpenTrdDate;
    zHR.lEligDate = zTR.lOpenStlDate;
    zHR.lTrdDate = zTR.lOpenTrdDate;
    zHR.lStlDate = zTR.lOpenStlDate;
  }

  /*
  ** If the transaction is being reversed, flip the security impact
  ** Unless the originating transaction is an exercise whose debit/credit
  ** column is set to DB, then leave the security impact as is
  ** Also, if the closing transaction is not a reversal and it is an
  ** exercise of a short positions (trantype = 'ex' and drcr = 'dr')
  ** flip the security impact.  Security Impacts for 'ex' transactions
  ** is set to -1 and in this case it must be +1
  */
  if (bReversal)
    lSecImpact *= -1;

  /*
  ** Calculate the unit price and unit accrual as this will be used
  ** to adjust the market value and accrual columns on the holdings
  ** Also, save  the market value and accrual exchange rates to update
  ** the holdings exchange rates following a cancel.  This is necessary
  ** due to the use of the holddel to restore lots to their before image
  */
  if (bRecFound) {
    if (!IsValueZero(zHR.fUnits, 5) && !IsValueZero(fTradingUnits, 3)) {
      fUnitAccrual = (zHR.fAccrInt / zHR.fUnits) / fTradingUnits;

      fMvBaseXrate = zHR.fMvBaseXrate;
      fMvSysXrate = zHR.fMvSysXrate;
      fAiBaseXrate = zHR.fAiBaseXrate;
      fAiSysXrate = zHR.fAiSysXrate;

      if (sCostInd[0] != 'L')
        fUnitPrice = (zHR.fMktVal / zHR.fUnits) / fTradingUnits;
      else {
        fUnitPrice = (zHR.fCurLiability / zHR.fUnits) / fTradingUnits;
        fUnitGL = (zHR.fMktVal / zHR.fUnits) / fTradingUnits;
      }
    } // if units and trading units are non-zero
    else {
      fUnitPrice = 0;
      fUnitAccrual = 0;

      fMvBaseXrate = 1;
      fAiBaseXrate = 1;
      fMvSysXrate = 1;
      fAiSysXrate = 1;
    }
  } /* if brecFound */

  /*
  ** If the transaction is a cancel of a closing transaction
  ** format the holdings record using the holddel record
  */
  if (bReversal) {
    if (sCurPrior[0] != 'C') // Read from holddel only for current processing
      zErr.iSqlError = SQLNOTFOUND;
    else {
      lpprSelectHolddel(&zHD, zTR.lRevTransNo, zTR.lEffDate, zTR.iID,
                        zTR.sSecNo, zTR.sWi, zTR.sSecXtend, zTR.sAcctType,
                        zTR.lTaxlotNo, &zErr);
      if (zErr.iSqlError == 0) {
        /*
        ** SB - 7/17/00 There is a unique situtaion in which the holddel record
        * might
        ** not be right. e.g. consider following trnsaction:
        **     1  PS  1500 units on 10/1/1999		resulting in 1500 units
        * on holdings
        **     2  SL   600 units on 7/7/2000		resulting in 900 units
        * on holdings
        **     3  SP   450 units on 6/15/2000		resulting in 1350 units
        * on holdings
        **  As of SL   500 units on 7/2/2000		resulting in 850 units
        * on holdings
        ** When asof SL happens, trans # 2(SL of 600units) will be reversed and
        * rebooked. When
        ** reversing trans # 2, the Holddel will have 1500 units which is wrong,
        * it should have
        ** 1350 + 600 = 1950 units. In situation like that, the existing holddel
        * should not be used.
        */
        if (RoundDouble(zHR.fUnits + (zTR.fUnits * lSecImpact) - zHD.fUnits,
                        3) != 0)
          zErr.iSqlError = SQLNOTFOUND;
      } // if holddel found
    } // doing current processing

    /*
     ** If holddel record not found, then the lot may not look exactly like it
     ** was before, so copy all the information from trans record and generate a
     * warning.
     */
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound1 = FALSE;

      if (bRecFound == FALSE) {
        CopyFieldsFromTransToHoldings(&zHR, zTR);
        zHR.fUnits = zHR.fTotCost = zHR.fOrigCost = zHR.fOrigFace =
            zHR.fMktVal = 0;
        zHR.fUnitCost = zTR.fOpenUnitCost;
        zHR.fBaseCostXrate = zTR.fBaseOpenXrate;
        zHR.fSysCostXrate = zTR.fSysOpenXrate;
        zHR.lEffDate = zTR.lOpenTrdDate;
        zHR.lEligDate = zTR.lOpenStlDate;
        zHR.lTrdDate = zTR.lOpenTrdDate;
        zHR.lStlDate = zTR.lOpenStlDate;
      } // if holdings/haldcash record not found

      // Send A Warning Message in case of current processing
      if (sCurPrior[0] == 'C')
        PrintError(
            "WARNING !!! Holddel Record Not Found-Potential Problms With Lot",
            zTR.iID, lRecNo, sRecType, 1000, 0, 0, "UPDH PROCESSCLOSE3", TRUE);
      zErr.iSqlError = 0;
    } // if holddel record not found
    else if (zErr.iSqlError == 0) {
      bRecFound1 = TRUE;
      CopyFieldsFromHolddelToHoldings(&zHR, zHD);
    } else
      return zErr;

    /*
    ** SB 6/22/00 - If it is a reversal, it is necessary to update the
    * rev_trans_no on the original
    ** holddel record. If the lot still exists (original closing transaction was
    * a partial sell),
    ** then the holddel will be updated above (inside the condition "bRecFound
    * && sCurPrior[0] == 'C' && sLotInd[0] != 'S'"),
    ** but if the lot was completely sold, holddel wil not get updated. So do
    * that here.
    */
    if (sCurPrior[0] == 'C' && !bRecFound && sLotInd[0] != 'S') {
      lpprHolddelUpdate(zTR.iID, zTR.lTransNo, zTR.lRevTransNo, zTR.lEffDate,
                        &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    }
  } /* Reversal Trade */

  /*
  ** If the transaction is not a reversal or it is a reversal and
  ** the holddel record was not found, process as usual
  */
  if (bReversal == FALSE || (bReversal && bRecFound1 == FALSE)) {
    /*
    ** Adjust the transaction fields so that to update the columns
    ** on the Holdings table, the routine need only perform an addition
    */
    zTR.fUnits *= lSecImpact;
    zTR.fTotCost *= lSecImpact;
    zTR.fOrigCost *= lSecImpact;
    zTR.fPcplAmt *= lSecImpact;
    zTR.fAccrInt *= lSecImpact;
    zTR.fOrigFace *= lSecImpact;

    zHR.fUnits += zTR.fUnits;

    // SB - 1/10/1999 Recalculate the market value of the security after
    // principal amount has been multipled by sec impact
    if (strcmp(zTR.sCurrId, zTR.sSecCurrId) != 0)
      zHR.fMktVal += (zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate;
    else
      zHR.fMktVal += zTR.fPcplAmt;

    if (sCostInd[0] == 'C') {
      zHR.fTotCost += zTR.fTotCost;
      zHR.fOrigCost += zTR.fOrigCost;
      if (zHR.fOrigFace + zTR.fOrigFace < 0)
        zHR.fOrigFace = 0;
      else
        zHR.fOrigFace += zTR.fOrigFace;
    } /* Cost indicator is 'C' */
    // SB 4/22/2008 - For liability security, use total cost on transaction
    // rather than calculating it based on open unit cost since open unit cost
    // for liability securities is zero. SB 9/9/09 - For Liability, cost should
    // be zero, liabilities don't have cost they have only liabilities,
    // furthermore use original cost from the transaction, not the total cost.
    // Original cost on transaction has open liability amount and it has current
    // proceeds on total cost.
    else if (sCostInd[0] == 'L') {
      zHR.fOpenLiability += zTR.fOrigCost;
      zHR.fTotCost = zHR.fOrigCost = zHR.fOrigFace = 0;
    } else
      return (PrintError("Invalid Cost Indicator", zTR.iID, lRecNo, sRecType,
                         100, 0, 0, "UPDH PROCESSCLOSE4", FALSE));
  } /* Reversal is FALSE or (reversal TRUE and Holddel rec not found */

  strcpy_s(zHR.sLastTransType, zTR.sTranType);
  zHR.lLastTransNo = zTR.lTransNo;
  strcpy_s(zHR.sLastTransSrce, zTR.sTransSrce);

  /*
  ** If current processing, then forcibly delete any
  ** pending accdiv/divhist entries, overwise payment may be
  ** posted for wrong number of units (i.e. SL's trd_date < ex_date < stl_date
  */
  if ((bRecFound) && (sCurPrior[0] == 'C') && (sPositionInd[0] != 'C')) {
    lpprDeleteAccruingDivhistOneLot(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                                    zHR.sAcctType, zHR.lTransNo, &zErr);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    lpprDeleteAccruingAccdivOneLot(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                                   zHR.sAcctType, zHR.lTransNo, &zErr);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  }

  /*
  ** If the units have been reduced to zero, then delete the lot
  ** otherwise update the exchange rates and call the update holdings function
  */
  if (IsValueZero(zHR.fUnits, 5)) {
    if (bRecFound == FALSE)
      return (PrintError("Invalid Or Missing Taxlot Number", zTR.iID, lRecNo,
                         sRecType, 100, 0, 0, "UPDH PROCESSCLOSE5", FALSE));

    if (sPositionInd[0] == 'C')
      lpprDeleteHoldcash(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                         zHR.sAcctType, &zErr);
    else
      lpprDeleteHoldings(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                         zHR.sAcctType, zHR.lTransNo, &zErr);

    return zErr;
  } /* If zHR.units = zero */

  /*
  ** Update the market value and accrued interest columns with the
  ** correct market value and accrued interest
  ** If the original lot had been found, recalculate the values
  ** using the unit values calculated earlier, otherwise (lot not found)
  ** use the values from the transaction record
  */
  if (bRecFound) {
    zHR.fAccrInt = zHR.fUnits * fUnitAccrual * fTradingUnits;

    if (sCostInd[0] != 'L')
      zHR.fMktVal = zHR.fUnits * fUnitPrice * fTradingUnits;
    else {
      zHR.fCurLiability = zHR.fUnits * fUnitPrice * fTradingUnits;
      zHR.fMktVal = zHR.fUnits * fUnitGL * fTradingUnits;
    }

    /*
    ** if there was a cancellation and the lot was restored using
    ** the holddel record, reset the exchange rates
    */
    if (bReversal && bRecFound1) {
      zHR.fMvBaseXrate = fMvBaseXrate;
      zHR.fAiBaseXrate = fAiBaseXrate;
      zHR.fMvSysXrate = fMvSysXrate;
      zHR.fAiSysXrate = fAiSysXrate;
    }
  } /* brecFound */
  else if (bRecFound1 == FALSE && strcmp(sCostInd, "L") == 0) {
    zHR.fCurLiability = zHR.fMktVal;
    zHR.fMktVal = 0;
  }
  // If it is a paydown transaction, update last paydown date on holdings
  if (strcmp(zTR.sTranType, "PD") == 0 && bReversal == FALSE)
    zHR.lLastPdnDate = zTR.lTrdDate;

  /* Recalculate the currency and security gain/loss columns in the holdings */
  fTotGL = fSGL = fCGL = fSTGL = fMTGL = fLTGL = 0;

  lProcessDate = zTR.lTrdDate;
  CopyFieldsFromHoldingsToTrans(&zTR, zHR, sPrimaryType);

  /* Set the currency id and the trade, settlement and eff date */
  strcpy_s(zTR.sCurrId, sCurrId);
  strcpy_s(zTR.sSecCurrId, sCurrId);
  zTR.lTrdDate = zTR.lStlDate = zTR.lEffDate = lProcessDate;
  char sClose[] = "C";
  zErr.iBusinessError = lpfnCalcGainLoss(
      zTR, sClose, lSecImpact, sPrimaryType, sSecondaryType, sBaseCurr, &fCGL,
      &fSGL, &fSTGL, &fMTGL, &fLTGL, &fTotGL);
  if (zErr.iBusinessError)
    return zErr;

  if (sPositionInd[0] == 'C') {
    zHR.fCurrencyGl = fCGL + fSGL;
    zHR.fSecurityGl = 0;
  } else {
    zHR.fCurrencyGl = fCGL;
    zHR.fSecurityGl = fSGL;
  }

  if (bRecFound) {
    if (sPositionInd[0] == 'C') {
      CopyFieldsFromHoldingsToHoldcash(&zHCR, zHR);
      if (zHCR.fUnits != 0.0)
        lpprUpdateHoldcash(zHCR, &zErr);
    } else
      lpprUpdateHoldings(zHR, &zErr);
  } /* RecFound */
  else {
    if (sPositionInd[0] == 'C') {
      CopyFieldsFromHoldingsToHoldcash(&zHCR, zHR);
      if (zHCR.fUnits != 0.0)
        lpprInsertHoldcash(zHCR, &zErr);
    } else {
      strcpy_s(zHR.sPrimaryType, sPrimaryType);
      lpprInsertHoldings(zHR, &zErr);
    }
  } /* recfound = false */

  return zErr;
} /* processclosing */

/**
** Function to process split trade transactions
**/
ERRSTRUCT ProcessSplitInHoldings(TRANS zTR, long lSecImpact, char *sAutoGen,
                                 BOOL bReversal, char *sCurPrior) {
  BOOL bRecFound, bRecFound1;
  ERRSTRUCT zErr;
  double fTrueCost, fTradingUnits, fAccrInt, fMktVal;
  double fMvBaseXrate, fMvSysXrate, fAiBaseXrate, fAiSysXrate;
  char sPrimaryType[2], sSecondaryType[2], sPositionInd[2], sLotInd[2];
  char sCostInd[2], sLotExistsInd[2], sAvgInd[2], sCurrId[6], sRecType[2];
  long lRecNo;
  HOLDINGS zHR;
  HOLDDEL zHD;

  InitializeErrStruct(&zErr);

  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  /*  Verify that the original lot does exist */
  InitializeHoldingsStruct(&zHR);
  lpprSelectHoldings(&zHR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                     zTR.sAcctType, zTR.lTaxlotNo, &zErr);
  if (zErr.iSqlError == SQLNOTFOUND) {
    bRecFound = FALSE;
    zErr.iSqlError = 0;
  } else if (zErr.iSqlError == 0)
    bRecFound = TRUE;
  else
    return zErr;

  // if it's a non cash trade and the record not found, return with an error
  if (bReversal == FALSE && bRecFound == FALSE)
    return (PrintError("Incorrect Or Missing Taxlot Number", zTR.iID, lRecNo,
                       sRecType, 100, 0, 0, "UPDH PROCESSSPLIT1", FALSE));

  // If the transaction is being reversed, flip the security impact
  if (bReversal)
    lSecImpact *= -1;

  // Insert the before image of the lot into the HOLDDEL table
  if (sCurPrior[0] == 'C') {
    CopyFieldsFromHoldingsToHolddel(&zHD, zHR);
    zHD.lCreateDate = zTR.lEffDate;
    zHD.lCreateTransNo = zTR.lTransNo;

    /*
    ** The rev_trans_no from the transaction will only contain a
    ** value for cancel transactions, in all other cases it will be zero
    */
    zHD.lRevTransNo = zTR.lRevTransNo;
    lpprInsertHolddel(zHD, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /*
    ** If the transaction is a reversal, it is necessary to update
    ** the rev_trans_no on the original holddel row
    */
    if (bReversal) {
      lpprHolddelUpdate(zTR.iID, zTR.lTransNo, zTR.lRevTransNo, zTR.lEffDate,
                        &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* reversal */
  } /* Current */

  /*
  ** If the transaction is a cancel of a split transaction
  ** format the holdings record using the holddel record
  ** First move the current market values, accrual and all relevant
  ** to temp. variables, this is necessary to correctly reset the
  ** values on the lot.
  */
  if (bReversal) {
    fMktVal = zHR.fMktVal;
    fAccrInt = zHR.fAccrInt;
    fMvBaseXrate = zHR.fMvBaseXrate;
    fMvSysXrate = zHR.fMvSysXrate;
    fAiBaseXrate = zHR.fAiBaseXrate;
    fAiSysXrate = zHR.fAiSysXrate;

    if (sCurPrior[0] != 'C')
      zErr.iSqlError = SQLNOTFOUND;
    else
      lpprSelectHolddel(&zHD, zTR.lRevTransNo, zTR.lEffDate, zTR.iID,
                        zTR.sSecNo, zTR.sWi, zTR.sSecXtend, zTR.sAcctType,
                        zTR.lTaxlotNo, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound1 = FALSE;
      zErr.iSqlError = 0;
    } else if (zErr.iSqlError == 0) {
      /*
      ** SB - 7/17/00 There is a unique situtaion in which the holddel record
      * might
      ** not be right. e.g. consider following trnsaction:
      **     1  PS  1500 units on 10/1/1999		resulting in 1500 units
      * on holdings
      **     2  SL   600 units on 7/7/2000		resulting in 900 units
      * on holdings
      **     3  SP   450 units on 6/15/2000		resulting in 1350 units
      * on holdings
      **  As of SL   500 units on 7/2/2000		resulting in 850 units
      * on holdings
      ** When asof SL happens, trans # 2(SL of 600units) will be reversed and
      * rebooked. When
      ** reversing trans # 2, the Holddel will have 1500 units which is wrong,
      * it should have
      ** 1350 + 600 = 1950 units. In situation like that, the existing holddel
      * should not be used.
      */
      if (RoundDouble(zHR.fUnits + (zTR.fUnits * lSecImpact) - zHD.fUnits, 3) !=
          0)
        bRecFound1 = FALSE;
      else
        bRecFound1 = TRUE;
    } // if holddel found
    else
      return zErr;

    if (bRecFound1) {
      CopyFieldsFromHolddelToHoldings(&zHR, zHD);
      zHR.fMktVal = fMktVal;
      zHR.fAccrInt = fAccrInt;
      zHR.fMvBaseXrate = fMvBaseXrate;
      zHR.fMvSysXrate = fMvSysXrate;
      zHR.fAiBaseXrate = fAiBaseXrate;
      zHR.fAiSysXrate = fAiSysXrate;
    }
  } /* Reversal Trade */

  /* Security Impact should be either +1 or -1 */
  if (lSecImpact != 1 && lSecImpact != -1)
    return (PrintError("Invalid Security Impact", zTR.iID, lRecNo, sRecType, 47,
                       0, 0, "UPDH PROCESSSPLIT", FALSE));

  if (bReversal == FALSE || (bReversal && bRecFound1 == FALSE)) {
    zErr = GetSecurityCharacteristics(
        zTR.sSecNo, zTR.sWi, sPrimaryType, sSecondaryType, sPositionInd,
        sLotInd, sCostInd, sLotExistsInd, sAvgInd, &fTradingUnits, sCurrId);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* calc the original cost as it will be needed to recalculate unit cost */
    fTrueCost = zHR.fUnits * zHR.fUnitCost * fTradingUnits;

    /*
    ** Adjust the transaction fields so that to update the columns
    ** on the Holdings table, the routine need only perform an addition
    */
    zTR.fUnits *= lSecImpact;
    zHR.fUnits += zTR.fUnits;

    /* recalculate the unit cost  */
    if (!IsValueZero(zHR.fUnits, 5) && !IsValueZero(fTradingUnits, 3)) {
      zHR.fUnitCost = (fTrueCost / zHR.fUnits) / fTradingUnits;
      if (zHR.fUnits < 0 && zHR.fUnitCost < 0)
        zHR.fUnitCost *= -1;

      // SB 10/19/99 Round unit_cost to 6 decimal places
      RoundDouble(zHR.fUnitCost, 6);
    } else
      zHR.fUnitCost = 0;
  }

  strcpy_s(zHR.sLastTransType, zTR.sTranType);
  zHR.lLastTransNo = zTR.lTransNo;
  strcpy_s(zHR.sLastTransSrce, zTR.sTransSrce);

  /*
  ** VI # 58465 If the units have been reduced to zero (can happen if
  * transaction was created with wrong units), then delete
  ** the taxlot otherwise update the taxlot
  */
  if (IsValueZero(zHR.fUnits, 5)) {
    if (sPositionInd[0] == 'C')
      lpprDeleteHoldcash(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                         zHR.sAcctType, &zErr);
    else
      lpprDeleteHoldings(zHR.iID, zHR.sSecNo, zHR.sWi, zHR.sSecXtend,
                         zHR.sAcctType, zHR.lTransNo, &zErr);
  } /* If zHR.units = zero */
  else
    lpprUpdateHoldings(zHR, &zErr);

  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  if (zTR.lDivintNo != 0 && sCurPrior[0] == 'C') {
    if (bReversal == FALSE)
      lpprUpdateDivhistOneLot(zTR.sTranType, (char *)"T", zTR.lTransNo, zTR.iID,
                              zTR.lTaxlotNo, zTR.lDivintNo, zTR.sTranType,
                              &zErr);
    else {
      lpprDeleteDivhistOneLot(zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo,
                              zTR.lRevTransNo, zTR.sRevType, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      lpprDeleteAccdivOneLot(zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo, &zErr);
    }
  } /* DivNo != 0 */

  return zErr;
} /* processsplit */

/**
** Function to process adjustment trade transactions
**/
ERRSTRUCT ProcessAdjustmentInHoldings(TRANS *pzTR, PORTMAIN zPR,
                                      long lSecImpact, long lCashImpact,
                                      BOOL bReversal, char *sCurPrior) {
  BOOL bRecFound, bRecFound1, bRecFound2;
  ERRSTRUCT zErr;
  char sTCode[3], sPrimaryType[2], sSecondaryType[2], sPositionInd[2];
  char sLotInd[2], sCostInd[2], sLotExistsInd[2], sAvgInd[2], sCurrId[6];
  double fTrueCost, fTradingUnits, fMktVal, fAccrInt, fSysXrate;
  double fMvBaseXrate, fMvSysXrate, fAiBaseXrate, fAiSysXrate;
  char sRecType[2];
  long lRecNo;
  HOLDINGS zHR;
  HEDGEXREF zHG;
  HOLDDEL zHD;
  TRANS zTR, zTR2;

  /*
   ** SB 10/17/2000 Untile now the TRANS record was being passed by value and
   * this function was
   ** temporarily changing some of the values (for convenience) which SHOULD NOT
   * get reflected
   ** in the calling functions. Now, in case of BA trades, we want to fill total
   * cost and the
   ** original cost in the transaction, so TRANS is being passed as a pointer.
   * So, make a copy
   ** of the transaction and make changes in the original and use the copy for
   * temporary record.
   */
  zTR = *pzTR;

  // The only adjustment transactions which affect holding File are SK, UC, AM,
  // LD, SG ,PO, BA, RP
  InitializeErrStruct(&zErr);

  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  // If the transaction is a reversal, use the reversal transaction type to
  // identify the original trade
  if (bReversal)
    strcpy_s(sTCode, zTR.sRevType);
  else
    strcpy_s(sTCode, zTR.sTranType);

  /*
  ** Set/unset safekeeping, update cost, amortization, liquidating dividends
  * set/unset gain/loss, pair offs,
  ** basis adjustment, return of principal, set/unset restrictions, update date,
  * update broker,
  ** purchase/sale of basis transactions are the only valid transactions for
  * this function
  */
  if (strcmp(sTCode, "SK") != 0 && strcmp(sTCode, "UC") != 0 &&
      strcmp(sTCode, "AM") != 0 && strcmp(sTCode, "LD") != 0 &&
      strcmp(sTCode, "PO") != 0 && strcmp(sTCode, "SG") != 0 &&
      strcmp(sTCode, "BA") != 0 && strcmp(sTCode, "UK") != 0 &&
      strcmp(sTCode, "UG") != 0 && strcmp(sTCode, "RP") != 0 &&
      strcmp(sTCode, "SR") != 0 && strcmp(sTCode, "UR") != 0 &&
      strcmp(sTCode, "UD") != 0 && strcmp(sTCode, "UB") != 0 &&
      strcmp(sTCode, "PZ") != 0 && strcmp(sTCode, "SZ") != 0)
    return zErr;

  // If the transction is an 'LD', 'BA', 'AM', 'RP', 'PZ', 'SZ' modify the pcpl
  // amt by the security impact
  if (strcmp(sTCode, "BA") == 0 || strcmp(sTCode, "LD") == 0 ||
      strcmp(sTCode, "AM") == 0)
    zTR.fPcplAmt *= lSecImpact;
  else if (strcmp(sTCode, "RP") == 0) {
    zTR.fPcplAmt *= lSecImpact;
    zTR.fTotCost *= lSecImpact;
    zTR.fOrigCost *= lSecImpact;
  } else if (strcmp(sTCode, "PZ") == 0 || strcmp(sTCode, "SZ") == 0) {
    zTR.fPcplAmt *= lSecImpact;
    zTR.fTotCost = zTR.fOrigCost =
        zTR.fPcplAmt; // for purchase/sale of basis cost has to be same as
                      // principal amount
    zTR.fUnits = 0;   // No units for this purchase/sale
  }

  // If the transaction is to be reversed, invert the principal amount and units
  if (bReversal) {
    zTR.fPcplAmt *= -1;
    zTR.fUnits *= -1;
    if (strcmp(sTCode, "RP") == 0 || strcmp(sTCode, "PZ") == 0 ||
        strcmp(sTCode, "SZ") == 0) {
      zTR.fOrigCost *= -1;
      zTR.fTotCost *= -1;
    } // return of principal, purchase/sale of basis
  }

  // Update broker effects transaction only, it does not effect
  // holdings/holdcash
  if (strcmp(sTCode, "UB") == 0) {
    // Select the transaction that needs to be modified
    lpprSelectOneTrans(zTR.iID, zTR.lOrigTransNo, &zTR2, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND)
      return (PrintError("Invalid Transaction Number", zTR.iID,
                         zTR.lOrigTransNo, "T", 50, 0, 0, "UPDH PROCESSADJUST1",
                         FALSE));
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    // if transaction has ben reversed, can't update broker code
    if (zTR2.lRevTransNo != 0)
      return (PrintError(
          "Invalid Transaction Number. Transaction Has Been Reversed", zTR.iID,
          zTR.lOrigTransNo, "T", 50, 0, 0, "UPDH PROCESSADJUST2", FALSE));

    /*
    ** In case of a regular(non-reversal) trade, before updating the broker
    * code, store the old
    ** value in broker code2 column (both on effected transaction and UB
    * transaction). In case
    ** of reversal use the value from broker code2 to restore the old value of
    * broker code.
    */
    if (bReversal)
      strcpy_s(zTR2.sBrokerCode, zTR.sBrokerCode2);
    else {
      strcpy_s(zTR2.sBrokerCode2, zTR2.sBrokerCode);
      strcpy_s(zTR.sBrokerCode2, zTR2.sBrokerCode);
      strcpy_s(zTR2.sBrokerCode, zTR.sBrokerCode);
    }

    lpprUpdateBrokerInTrans(zTR2, &zErr);
    return zErr;
  } // Update broker

  /*
  ** This function will return all of the characteristics of the security
  ** primary currency, secondary currency, trading unit, postion file to update,
  * etc
  */
  zErr = GetSecurityCharacteristics(
      zTR.sSecNo, zTR.sWi, sPrimaryType, sSecondaryType, sPositionInd, sLotInd,
      sCostInd, sLotExistsInd, sAvgInd, &fTradingUnits, sCurrId);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /* Select the lot to be adjusted */
  InitializeHoldingsStruct(&zHR);
  lpprSelectHoldings(&zHR, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                     zTR.sAcctType, zTR.lTaxlotNo, &zErr);
  if (zErr.iSqlError == SQLNOTFOUND) {
    bRecFound = FALSE;
    zErr.iSqlError = 0;
  } else if (zErr.iSqlError == 0)
    bRecFound = TRUE;
  else
    return zErr;

  /*
  ** If unable to find the lot and the transaction is a liquidating dividend,
  ** or reversal of set/unset restriction or update date,
  ** return without an error, for all other transaction type,
  ** return the error
  */
  if (!bRecFound) {
    if (strcmp(sTCode, "LD") == 0 ||
        (bReversal && (strcmp(sTCode, "SR") == 0 || strcmp(sTCode, "UR") == 0 ||
                       strcmp(sTCode, "UD") == 0)))
      return zErr;
    else
      return (PrintError("Invalid Or Missing Taxlot Number", zTR.iID, lRecNo,
                         sRecType, 51, 0, 0, "UPDH PROCESSADJUST3", FALSE));
  } /* Rec Not Found */

  /* Insert the before image of the lot into the HOLDDEL table */
  if (sCurPrior[0] == 'C') {
    CopyFieldsFromHoldingsToHolddel(&zHD, zHR);
    zHD.lCreateDate = zTR.lEffDate;
    zHD.lCreateTransNo = zTR.lTransNo;

    /*
    ** The rev_trans_no from the transaction will only contain a
    ** value for cancel transactions, in all other cases it will be zero
    */
    zHD.lRevTransNo = zTR.lRevTransNo;
    lpprInsertHolddel(zHD, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /*
    ** If the transaction is a reversal, it is necessary to update
    ** the rev_trans_no on the original holddel row
    */
    if (bReversal) {
      lpprHolddelUpdate(zTR.iID, zTR.lTransNo, zTR.lRevTransNo, zTR.lEffDate,
                        &zErr);
      if (zErr.iSqlError)
        PrintError("WARNING !!!! Holddel Record Not Found-Potential Problms "
                   "With Dividend Payments",
                   zTR.iID, lRecNo, sRecType, 1001, 0, 0, "UPDH PROCESSADJUST4",
                   TRUE);
    } // Reversal
  } // Current

  /*
  ** If the transaction is a cancel of a adjustment transaction format the
  * holdings record
  ** using the holddel record. First move the current market values, accrual and
  * all relevant
  ** exrates to temp. variables, this is necessary to correctly reset the values
  * on the lot.
  */
  if (bReversal) {
    if (strcmp(zPR.sAcctMethod, "A") == 0)
      bRecFound2 = FALSE;
    else {
      fMktVal = zHR.fMktVal;
      fAccrInt = zHR.fAccrInt;
      fMvBaseXrate = zHR.fMvBaseXrate;
      fMvSysXrate = zHR.fMvSysXrate;
      fAiBaseXrate = zHR.fAiBaseXrate;
      fAiSysXrate = zHR.fAiSysXrate;

      if (sCurPrior[0] != 'C') // use holddel only for current
        zErr.iSqlError = SQLNOTFOUND;
      else
        lpprSelectHolddel(&zHD, zTR.lRevTransNo, zTR.lEffDate, zTR.iID,
                          zTR.sSecNo, zTR.sWi, zTR.sSecXtend, zTR.sAcctType,
                          zTR.lTaxlotNo, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        bRecFound2 = FALSE;
        zErr.iSqlError = 0;
      } else if (zErr.iSqlError == 0)
        bRecFound2 = TRUE;
      else
        return zErr;

      if (bRecFound2) {
        CopyFieldsFromHolddelToHoldings(&zHR, zHD);
        zHR.fMktVal = fMktVal;
        zHR.fAccrInt = fAccrInt;
        zHR.fMvBaseXrate = fMvBaseXrate;
        zHR.fMvSysXrate = fMvSysXrate;
        zHR.fAiBaseXrate = fAiBaseXrate;
        zHR.fAiSysXrate = fAiSysXrate;
        if (strcmp(sTCode, "BA") == 0)
          zTR.fTotCost = zTR.fOrigCost = 0;
      }
    } // Not Average Cost
  } /* Reversal trade */

  if (bReversal == FALSE || (bReversal && bRecFound2 == FALSE)) {
    if (strcmp(sTCode, "SR") == 0)
      zHR.iRestrictionCode = zTR.iRestrictionCode;

    if (strcmp(sTCode, "UR") == 0)
      zHR.iRestrictionCode = 0;

    if (strcmp(sTCode, "SK") == 0 || strcmp(sTCode, "UK") == 0)
      strcpy_s(zHR.sSafekInd, zTR.sSafekInd);

    if (strcmp(sTCode, "SG") == 0 || strcmp(sTCode, "UG") == 0)
      strcpy_s(zHR.sPermLtFlag, zTR.sGlFlag);

    if (strcmp(sTCode, "UC") == 0) {
      // SB 7/19/01 UC should not update original trade date
      zHR.fUnitCost = zTR.fUnitCost;
      zHR.fBaseCostXrate = zTR.fBaseXrate;
      zHR.fSysCostXrate = zTR.fSysXrate;

      zHR.fOrigCost = zTR.fPcplAmt;
      if (strcmp(zTR.sBalToAdjust, "ORIG") != 0)
        zHR.fTotCost = zTR.fPcplAmt;
    } /* if UC */
    else if (strcmp(sTCode, "BA") == 0) {
      // SB 10/17/2000 - don't care if the old cost was zero or not, apply the
      // adjustments anyway
      /*
      ** SB 6/15/2002 - Until now the test was if BalToAdjust == "ORIG" which
      * was
      ** not being used (no calling program was populating it), now changed it b
      ** if BalToAdjust == "CURR", fron end will now have to options with basis
      * adjustment
      ** transactions, adjust both current and origianl cost (only option until
      * now) or
      ** adjust just current cost (now default in FrontEnd).
      */
      if (strcmp(zTR.sBalToAdjust, "CURR") == 0)
        zHR.fTotCost += zTR.fPcplAmt;
      else {
        zHR.fOrigCost += zTR.fPcplAmt;
        zHR.fTotCost += zTR.fPcplAmt;
      } // Updating both total and original cost.(BalToAdjust != "CURR")

      pzTR->fOrigCost = zHR.fTotCost - zTR.fPcplAmt; // cost prior to adjustment
      pzTR->fTotCost = zHR.fTotCost;                 // cost after adjustment

      // recalculate the unit cost
      fTrueCost = (zHR.fUnits * zHR.fUnitCost * fTradingUnits) + zTR.fPcplAmt;

      if (!IsValueZero(zHR.fUnits, 5) && !IsValueZero(fTradingUnits, 3)) {
        zHR.fUnitCost = (fTrueCost / zHR.fUnits) / fTradingUnits;

        if (zHR.fUnits < 0 && zHR.fUnitCost < 0)
          zHR.fUnitCost = zHR.fUnitCost * -1;

        // SB 10/19/99 Round unit_cost to 6 decimal places
        RoundDouble(zHR.fUnitCost, 6);
      } else
        zHR.fUnitCost = 0;

      // If the unit cost falls below zero, force it to be one cent
      if (zHR.fUnitCost < 0.0)
        zHR.fUnitCost = 0.01;
    } /* if BA */
    else if (strcmp(sTCode, "LD") == 0) {
      /* SB 9/18/98 Changed it to reflect the changes in N&B code after the
original code was downloaded zHR.fTotCost += zTR.fPcplAmt; zHR.fOrigCost +=
zTR.fPcplAmt;

// recalculate the unit cost
fTrueCost = (zHR.fUnitCost * zHR.fUnits * fTradingUnits) - zTR.fPcplAmt;

      if (zHR.fUnits != 0.0 && fTradingUnits != 0.0)
zHR.fUnitCost = (fTrueCost / zHR.fUnits) / fTradingUnits;
else
zHR.fUnitCost = 0;*/
      // Adjust if total cost is not zero
      if (!IsValueZero(zHR.fTotCost, 3)) {
        if (zHR.fTotCost + zTR.fPcplAmt >= 0.0) {
          zHR.fTotCost += zTR.fPcplAmt;
          zHR.fOrigCost += zTR.fPcplAmt;

          // recalculate the unit cost
          fTrueCost =
              (zHR.fUnitCost * zHR.fUnits * fTradingUnits) + zTR.fPcplAmt;

          if (!IsValueZero(zHR.fUnits, 5) && !IsValueZero(fTradingUnits, 3)) {
            zHR.fUnitCost = (fTrueCost / zHR.fUnits) / fTradingUnits;

            // if units < 0 and unit cost is negative, make it positive
            if (zHR.fUnits < 0.0 && zHR.fUnitCost < 0.0)
              zHR.fUnitCost *= -1;

            // SB 10/19/99 Round unit_cost to 6 decimal places
            RoundDouble(zHR.fUnitCost, 6);
          } else
            zHR.fUnitCost = 0;

          // If the unit cost falls below zero, force it to be one cent
          if (zHR.fUnitCost < 0.0)
            zHR.fUnitCost = 0.01;
        } // if zHR.fTotCost + zTR.fPcplAmt >= 0
      } // if total cost is not zero
    } /* if LD */
    else if (strcmp(sTCode, "AM") == 0)
      zHR.fTotCost += zTR.fPcplAmt;
    else if (strcmp(sTCode, "PO") == 0)
      zHR.fCollateralUnits += zTR.fUnits;
    else if (strcmp(sTCode, "RP") == 0 || strcmp(sTCode, "PZ") == 0 ||
             strcmp(sTCode, "SZ") ==
                 0) // return of principal, purchase/sale of basis
    {
      zHR.fTotCost += zTR.fTotCost;
      zHR.fOrigCost += zTR.fOrigCost;

      if (!IsValueZero(zHR.fUnits, 3))
        zHR.fUnitCost = zHR.fTotCost / (zHR.fUnits * fTradingUnits);
    } else if (strcmp(sTCode, "UD") == 0) // update date
    {
      if (!bReversal) {
        if (strcmp(zTR.sBalToAdjust, "TRD") == 0)
          zHR.lTrdDate = zTR.lTrdDate;
        else if (strcmp(zTR.sBalToAdjust, "STL") == 0)
          zHR.lStlDate = zTR.lStlDate;
        else if (strcmp(zTR.sBalToAdjust, "ELIG") == 0)
          zHR.lEligDate = zTR.lTrdDate;
      } else {
        if (strcmp(zTR.sBalToAdjust, "TRD") == 0)
          zHR.lTrdDate = zTR.lOpenTrdDate;
        else if (strcmp(zTR.sBalToAdjust, "STL") == 0)
          zHR.lStlDate = zTR.lOpenStlDate;
        else if (strcmp(zTR.sBalToAdjust, "ELIG") == 0)
          zHR.lEligDate = zTR.lOpenTrdDate;
      }
    }
  } /* Reversal is FALSE or (Reversal is TRUE and RecFound is FALSE) */

  /* If the transaction is a pair-off transaction, update the HEDGEXREF table */
  if (strcmp(sTCode, "PO") == 0) {
    lpprSelectHedgxref(&zHG, zTR.iID, zTR.sSecNo, zTR.sWi, zTR.sSecXtend,
                       zTR.sAcctType, zTR.lTaxlotNo, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bRecFound1 = FALSE;
      CopyFieldsFromTransToHedgeXref(&zHG, zTR);
      zErr.iSqlError = 0;
    } else if (zErr.iSqlError == 0)
      bRecFound1 = TRUE;
    else
      return zErr;

    zHG.fHedgeUnits += zTR.fUnits;
    zHG.fHedgeUnits2 += zTR.fUnits;

    if (bRecFound1) {
      if (zHG.fHedgeUnits == 0)
        lpprDeleteHedgxref(zHG.iID, zHG.sSecNo, zHG.sWi, zHG.sSecXtend,
                           zHG.sAcctType, zHG.lTransNo, &zErr);
      else
        lpprUpdateHedgxref(zHG, &zErr);
    } else
      lpprInsertHedgxref(zHG, &zErr);
    if (zErr.iSqlError != 0 || zErr.iSqlError != 0)
      return zErr;
  } /* PO */

  strcpy_s(zHR.sLastTransType, zTR.sTranType);
  zHR.lLastTransNo = zTR.lTransNo;
  strcpy_s(zHR.sLastTransSrce, zTR.sTransSrce);

  lpprUpdateHoldings(zHR, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  if (zTR.lDivintNo != 0 && sCurPrior[0] == 'C') {
    if (bReversal == FALSE)
      lpprUpdateDivhistOneLot(zTR.sTranType, (char *)"T", zTR.lTransNo, zTR.iID,
                              zTR.lTaxlotNo, zTR.lDivintNo, zTR.sTranType,
                              &zErr);
    else {
      lpprDeleteDivhistOneLot(zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo,
                              zTR.lRevTransNo, zTR.sRevType, &zErr);
      if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
        return zErr;

      lpprDeleteAccdivOneLot(zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo, &zErr);
      if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
        return zErr;
    } // Reversal
  } // if Current and DivintNo != 0

  /*
  ** If the transaction type is AM, UC LD, BA, RP, PZ, SZ and the portfolio is
  ** using average cost accounting and the security is eligible for averaging
  */
  fSysXrate = zTR.fSysXrate;

  if ((strcmp(sTCode, "AM") == 0 || strcmp(sTCode, "UC") == 0 ||
       strcmp(sTCode, "LD") == 0 || strcmp(sTCode, "BA") == 0 ||
       strcmp(sTCode, "RP") == 0 || strcmp(sTCode, "PZ") == 0 ||
       strcmp(sTCode, "SZ") == 0) &&
      strcmp(zPR.sAcctMethod, "A") == 0 && strcmp(sAvgInd, "Y") == 0) {
    zTR.fTotCost = zTR.fOrigCost = zTR.fUnits = 0;
    zErr = ProcessAverageInHoldings(&zTR, fTradingUnits, &fSysXrate, bReversal,
                                    sCostInd);
  }

  return zErr;
} /* processadjustment */

/**
** Function to process transfer of securities within the same account.  Inter
** account transfers are broken into two transactions and linked by use of the
** xref_trans_no.  A transfer comprises of two pieces, a closing portion and an
** opening portion.  This function first performs the closing piece using the
** primary account information (br_acct, sec_no, etc) and then the opening
** portion using the secondary account information (xbr_acct, xsec_no etc).
**/
ERRSTRUCT ProcessTransferInHoldings(TRANS zTR, PORTMAIN zPR, char *sBaseCurr,
                                    long lSecImpact, long lCashImpact,
                                    int iDateInd, BOOL bReversal,
                                    char *sCurPrior) {
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  if (lSecImpact != 2)
    return zErr;
  /*
  ** Given that a transfer reduces units on one side and increases units
  ** on the opening side, it is necessary to reset the security impact
  ** field depending on which action is taking place.  The closing portion
  ** is always processed first, so therefore the security impact is -1,
  ** unless the position in question is short, then it is +1.  Short positions
  ** are identified by the debit/credit field which will be set to 'DR' for
  ** short positions and 'CR' for long positions
  */
  /*  if (strcmp(zTR.sDrCr, "DR") == 0)
      lSecImpact = 1;
    else
      lSecImpact = -1;*/

  /*
  ** SB - Changed the logic of secimpact on 12/31/1999 - We are using "DrCr" to
  * determine
  ** whether it is "UP', "PL' or "RP", instead on whether it is short/long. So
  * closing will
  ** have security impact -1. Transfer of Short positions will be handled later
  * on.
  */
  lSecImpact = -1;
  zErr = ProcessClosingInHoldings(zTR, zPR.sBaseCurrId, lSecImpact, bReversal,
                                  sCurPrior);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /*
  ** Reset the branch account information and the tax lot number using the
  ** cross branch account information
  */
  strcpy_s(zTR.sSecNo, zTR.sXSecNo);
  strcpy_s(zTR.sWi, zTR.sXWi);
  strcpy_s(zTR.sSecXtend, zTR.sXSecXtend);
  strcpy_s(zTR.sAcctType, zTR.sXAcctType);
  zTR.iSecID = zTR.iXSecID;
  zTR.lTrdDate = zTR.lOpenTrdDate;
  zTR.lStlDate = zTR.lOpenStlDate;

  if (bReversal)
    zTR.lTaxlotNo = zTR.lRevTransNo;
  else
    zTR.lTaxlotNo = zTR.lTransNo;

  /* Flip the security impact column for open processing */
  lSecImpact *= -1;

  return (ProcessOpeningInHoldings(zTR, zPR, lSecImpact, lCashImpact, iDateInd,
                                   bReversal, sCurPrior));
} /* processtransferinholdings */

/**
** Function to process transfers of cash within the same account.  Inter
** account transfers are broken into two transactions and linked by use of the
** xref_trans_no.  A transfer comprises of two pieces, a closing portion and an
** opening portion.  This function first performs the closing piece using the
** primary account information (br_acct, curr_id, etc) and then the opening
** portion using the secondary account information (xbr_acct, xcurr_id etc).
**/
ERRSTRUCT ProcessTransferInHoldcash(TRANS zTR, PORTMAIN zPR, long lCashImpact,
                                    int iDateInd, BOOL bReversal,
                                    char *sCurPrior) {
  ERRSTRUCT zErr;
  char sRecType[2];
  long lRecNo;
  double fExrate;

  InitializeErrStruct(&zErr);

  if (zTR.lDtransNo != 0) {
    lRecNo = zTR.lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = zTR.lTransNo;
    strcpy_s(sRecType, "T");
  }

  /* If the cash impact on the transaction is not set to 2, exit the function */
  if (lCashImpact != 2)
    return (PrintError("Invalid Cash Impact", zTR.iID, lRecNo, sRecType, 48, 0,
                       0, "UPDH PROCESSXFERINHCASH", FALSE));

  /*
  ** Given that a transfer reduces units on one side and increases units
  ** on the opening side, it is necessary to reset the cash impact
  ** field depending on which action is taking place.  The closing portion
  ** is always processed first, so therefore the cash impact is -1,
  ** the opening portion will have a cash impact of 1.  There is no need
  ** to invert the cash impact for reversal transactions in this funtion as
  ** process cash function already handles such a case
  */
  lCashImpact = -1;

  /* If 'IS' type of transaction, no need to change pcplamt by exrate because
     it is the same currency that is being swept (same for reversal of 'IS' */
  if ((strcmp(zTR.sTranType, "IS") != 0) && ((strcmp(zTR.sRevType, "IS") != 0)))
    zTR.fPcplAmt *= zTR.fBaseXrate;

  fExrate = zTR.fBaseXrate;
  if (strcmp(zTR.sCurrId, zPR.sBaseCurrId) == 0)
    zTR.fBaseXrate = 1;

  zErr = ProcessCash(zTR, lCashImpact, bReversal, sCurPrior, zPR.bIncByLot);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  /*
  ** Reset the currency id and account type using the cross currency information
  ** Recalculate the principal impact so that it is stated in terms of the
  ** cross currency id
  */
  strcpy_s(zTR.sCurrId, zTR.sXCurrId);
  strcpy_s(zTR.sCurrAcctType, zTR.sXCurrAcctType);

  strcpy_s(zTR.sSecXtend, zTR.sXSecXtend);
  strcpy_s(zTR.sAcctType, zTR.sXAcctType);
  zTR.fBaseXrate = fExrate;

  /*
  ** Set the Cash Impact and the Principal Amount fields
  ** BaseXrate is not being checked before decdiv - because it should never
  ** be zero, if program has come so far with a zero basexrate, it SHOULD cause
  ** a memory fault and the calling programs should be corrected so that they
  ** never send a zero basexrate
  */
  lCashImpact = 1;

  /* If 'IS' type of transaction, no need to change pcplamt by exrate because
     it is the same currency that is being swept (same for reversal of 'IS' */
  if ((strcmp(zTR.sTranType, "IS") != 0) && ((strcmp(zTR.sRevType, "IS") != 0)))
    zTR.fPcplAmt /= zTR.fBaseXrate;

  if (strcmp(zTR.sCurrId, zPR.sBaseCurrId) == 0)
    zTR.fBaseXrate = 1;

  return (ProcessCash(zTR, lCashImpact, bReversal, sCurPrior, zPR.bIncByLot));
} /* processtransferinholdcash */

/**
** Function to recalculate original and total cost for existing positions
** in an average cost portfolio
** Positions whose trade date is set to 19010101 will not be averaged
** as they have insufficient cost information
**/
ERRSTRUCT ProcessAverageInHoldings(TRANS *pzTR, double fTradingUnits,
                                   double *pfSysXrate, BOOL bReversal,
                                   char *sCostInd) {
  char sRecType[2];
  long lRecNo;
  double fTotalUnits, fTotalTotCost, fTotalOrigCost, fUnitOrigCost,
      fUnitTotCost, fTotalBaseTotCost;
  double fTotalSysTotCost, fUnitLiability, fTotalOpenLiability,
      fTotalBaseOLiability, fTotalSysOLiability;
  double fNewBaseXrate, fNewSysXrate;
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  if (pzTR->lDtransNo != 0) {
    lRecNo = pzTR->lDtransNo;
    strcpy_s(sRecType, "D");
  } else {
    lRecNo = pzTR->lTransNo;
    strcpy_s(sRecType, "T");
  }

  fTotalUnits = fTotalTotCost = fTotalOrigCost = fUnitOrigCost = fUnitTotCost =
      fUnitLiability = 0;
  fTotalBaseTotCost = fTotalSysTotCost = fTotalOpenLiability =
      fTotalBaseOLiability = fTotalSysOLiability = 0;
  fNewBaseXrate = fNewSysXrate = 1.0;

  lpprHsumSelect(&fTotalUnits, &fTotalTotCost, &fTotalOrigCost,
                 &fTotalBaseTotCost, &fTotalSysTotCost, &fTotalOpenLiability,
                 &fTotalBaseOLiability, &fTotalSysOLiability, pzTR->iID,
                 pzTR->sSecNo, pzTR->sWi, pzTR->sSecXtend, pzTR->sAcctType, 0,
                 &zErr);
  if ((zErr.iSqlError != SQLNOTFOUND && zErr.iSqlError != 0) ||
      zErr.iBusinessError != 0)
    return (PrintError("Error Fetching Sum From HOLDINGS", pzTR->iID, lRecNo,
                       sRecType, 0, zErr.iSqlError, zErr.iIsamCode,
                       "UPDH FETCH HSUM", FALSE));

  // Adjust the summarized values by the transaction values
  fTotalUnits += pzTR->fUnits;

  // SB 1/17/08 - If this is a liability security then open liability needs to
  // be averaged, if not then cost needs to be averaged
  if (sCostInd[0] == 'L') {
    fTotalOpenLiability += pzTR->fTotCost;

    /*
    ** Calculate the open liability in terms of the base exchange rate and the
    ** system's exchange rate and add the result to the necessary totals
    */
    fTotalBaseOLiability += (pzTR->fTotCost / pzTR->fBaseXrate);
    fTotalSysOLiability += (pzTR->fTotCost / *pfSysXrate);

    /* recalculate open liability for the lot */
    if (!IsValueZero(fTotalUnits, 5) && !IsValueZero(fTradingUnits, 3))
      fUnitLiability =
          RoundDouble((fTotalOpenLiability / fTotalUnits) / fTradingUnits, 10);

    pzTR->fTotCost = pzTR->fUnits * fUnitLiability * fTradingUnits;
    pzTR->fTotCost = RoundDouble(pzTR->fTotCost, 2);

    /* Recalculate the averaged exchange rates */
    if (!IsValueZero(fTotalBaseOLiability, 2))
      fNewBaseXrate = fTotalOpenLiability / fTotalBaseOLiability;
    else
      fNewBaseXrate = 1;

    if (IsValueZero(fNewBaseXrate, 12)) {
      // Liability(and hence xrate) becomes zero in case of Reversal of a single
      // lot. In this case the lot will be deleted any way, no need to give this
      // warning
      if (!bReversal)
        PrintError("WARNING !!! Defaulting Base Exrate To 1", pzTR->iID, lRecNo,
                   sRecType, 68, 0, 0, "UPDH AVERAGE1A", TRUE);
      fNewBaseXrate = 1;
    }

    pzTR->fBaseXrate = fNewBaseXrate;

    if (!IsValueZero(fTotalSysOLiability, 2))
      fNewSysXrate = fTotalOpenLiability / fTotalSysOLiability;
    else
      fNewSysXrate = 1;

    if (IsValueZero(fNewSysXrate, 12)) {
      // Liability(and hence xrate) becomes zero in case of Reversal of a single
      // lot. In this case the lot will be deleted any way, no need to give this
      // warning
      if (!bReversal)
        PrintError("WARNING !!! Defaulting Sys Exrate To 1", pzTR->iID, lRecNo,
                   sRecType, 68, 0, 0, "UPDH AVERAGE2A", TRUE);
      fNewSysXrate = 1;
    }

    *pfSysXrate = fNewSysXrate;
  } // if liability
  else {
    fTotalTotCost += pzTR->fTotCost;

    /*
    ** if orig cost is zero, add total cost - SB 7/10/02 Changed that logic, for
    * BA transactions
    ** which affect only total cost, it will be incorrect to add orig cost.
    */
    //	if (!IsValueZero(pzTR->fOrigCost, 3))
    fTotalOrigCost += pzTR->fOrigCost;
    //  else
    //  fTotalOrigCost += pzTR->fTotCost;

    /*
    ** Calculate the total cost and original cost in terms of the base exchange
    * rate and the system's exchange rate and
    ** add the result to the necessary totals
    */
    fTotalBaseTotCost += (pzTR->fTotCost / pzTR->fBaseXrate);
    fTotalSysTotCost += (pzTR->fTotCost / *pfSysXrate);

    /* recalculate total cost for the lot */
    if (!IsValueZero(fTotalUnits, 5) && !IsValueZero(fTradingUnits, 3))
      pzTR->fUnitCost = (fTotalTotCost / fTotalUnits) / fTradingUnits;
    //  else
    //  fTotUnitCost = 0;
    fUnitTotCost = RoundDouble(pzTR->fUnitCost, 10);
    pzTR->fUnitCost = RoundDouble(pzTR->fUnitCost, 6); // SB added on 10/19

    pzTR->fTotCost = pzTR->fUnits * fUnitTotCost * fTradingUnits;
    pzTR->fTotCost = RoundDouble(pzTR->fTotCost, 3); // SB added on 10/19

    /* recalculate original cost for the lot */
    if (!IsValueZero(fTotalUnits, 5) && !IsValueZero(fTradingUnits, 3))
      fUnitOrigCost = (fTotalOrigCost / fTotalUnits) / fTradingUnits;
    else
      fUnitOrigCost = 0;
    fUnitOrigCost = RoundDouble(fUnitOrigCost, 10); // SB added on 10/19

    pzTR->fOrigCost = pzTR->fUnits * fUnitOrigCost * fTradingUnits;
    pzTR->fOrigCost = RoundDouble(pzTR->fOrigCost, 3); // SB added on 10/19

    /* Recalculate the averaged exchange rates */
    if (!IsValueZero(fTotalBaseTotCost, 12))
      fNewBaseXrate = fTotalTotCost / fTotalBaseTotCost;
    else
      fNewBaseXrate = 1;

    if (IsValueZero(fNewBaseXrate, 12)) {
      // Cost(and hence xrate) becomes zero in case of Reversal of a single lot.
      // In this case the lot will be deleted any way, no need to give this
      // warning
      if (!bReversal)
        PrintError("WARNING !!! Defaulting Base Exrate To 1", pzTR->iID, lRecNo,
                   sRecType, 68, 0, 0, "UPDH AVERAGE1B", TRUE);
      fNewBaseXrate = 1;
    }

    pzTR->fBaseXrate = fNewBaseXrate;

    if (!IsValueZero(fTotalSysTotCost, 12))
      fNewSysXrate = fTotalTotCost / fTotalSysTotCost;
    else
      fNewSysXrate = 1;

    if (IsValueZero(fNewSysXrate, 12)) {
      // Cost(and hence xrate) becomes zero in case of Reversal of a single lot.
      // In this case the lot will be deleted any way, no need to give this
      // warning
      if (!bReversal)
        PrintError("WARNING !!! Defaulting Sys Exrate To 1", pzTR->iID, lRecNo,
                   sRecType, 68, 0, 0, "UPDH AVERAGE2B", TRUE);
      fNewSysXrate = 1;
    }

    *pfSysXrate = fNewSysXrate;
  } // if not liability

  // update unit_cost, tot_cost and orig_cost in all the previous lots of the
  // same security
  lpprHsumUpdate(fUnitTotCost, fTradingUnits, fUnitOrigCost, fUnitLiability,
                 fNewBaseXrate, fNewSysXrate, pzTR->sTranType, pzTR->lTaxlotNo,
                 pzTR->sTransSrce, pzTR->iID, pzTR->sSecNo, pzTR->sWi,
                 pzTR->sSecXtend, pzTR->sAcctType, 0, &zErr);
  if (zErr.iSqlError)
    return (PrintError("Error Updating Records In HOLDINGS", pzTR->iID, lRecNo,
                       sRecType, 0, zErr.iSqlError, zErr.iIsamCode,
                       "UPDH HSUM UPDATE", FALSE));

  return zErr;
} /* processaverage */

/*
** Update the Divhist row associated with the income payment
** If the transactions is a reversal of a payment, delete the divhist
** row, otherwise update the tran_location to 'T' and assign the transaction
** number to div_trans_no.
*/
ERRSTRUCT UpdateDivhistAccdiv(TRANS zTR, BOOL bIncByLot, BOOL bReversal,
                              char *sCurPrior, char *sProcessType) {
  char sType[3];
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  if (zTR.lDivintNo != 0 && sCurPrior[0] == 'C') {
    if (bReversal)
      strcpy_s(sType, zTR.sRevType);
    else if (sProcessType[0] == 'M')
      strcpy_s(sType, zTR.sTranType);
    else if (sProcessType[0] == 'I') {
      if (strcmp(zTR.sTranType, "WH") == 0 ||
          strcmp(zTR.sTranType, "AR") == 0 || strcmp(zTR.sTranType, "RR") == 0)
        strcpy_s(sType, zTR.sTranType);
      else
        strcpy_s(sType, "AD");
    }

    if (bReversal == FALSE) {
      if (bIncByLot)
        lpprUpdateDivhistOneLot(zTR.sTranType, (char *)"T", zTR.lTransNo,
                                zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo, sType,
                                &zErr);
      else
        lpprUpdateDivhistAllLots(zTR.sTranType, (char *)"T", zTR.lTransNo,
                                 zTR.iID, zTR.lDivintNo, sType, &zErr);

      if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
        return zErr;
    } else {
      if (bIncByLot)
        lpprDeleteDivhistOneLot(zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo,
                                zTR.lRevTransNo, sType, &zErr);
      else
        lpprDeleteDivhistAllLots(zTR.iID, zTR.lDivintNo, zTR.lRevTransNo, sType,
                                 &zErr);
      if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
        return zErr;

      if ((sProcessType[0] == 'M' &&
           ((strcmp(sType, "LT") == 0 || strcmp(sType, "ST") == 0))) ||
          (sProcessType[0] == 'I' &&
           ((strcmp(sType, "RD") == 0 || strcmp(sType, "RI") == 0)))) {
        if (bIncByLot)
          lpprDeleteAccdivOneLot(zTR.iID, zTR.lTaxlotNo, zTR.lDivintNo, &zErr);
        else
          lpprDeleteAccdivAllLots(zTR.iID, zTR.lDivintNo, &zErr);
        if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
          return zErr;
      } /* transaction is a cancel of a LT/ST or RD/RI*/
    } /* reversal = true */
  } /* DivNo is not zero */

  return zErr;
} // UpdateDivhistAccdiv