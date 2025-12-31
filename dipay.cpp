/**
 *
 * SUB-SYSTEM: payments
 *
 * FILENAME: dipay.c
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

/**
** This is the function called by the user to pay dividend/interest
** transactions.
**/
ERRSTRUCT STDCALL WINAPI PayDivInt(long lValDate, const char *sMode,
                                   const char *sProcessFlag, int iID,
                                   const char *sSecNo, const char *sWi,
                                   const char *sSecXtend, const char *sAcctType,
                                   long lTransNo) {
  PORTTABLE zPmainTable;
  ACCDIV zAccdiv;
  ASSETS zAssets;
  DIVHIST zDivhist;
  PARTFINC zFinc;
  ERRSTRUCT zErr;
  DTRANSDESC zDTr[1];
  TRANS zTrans, zTransWH, zTransAR;
  TRANTYPE zTranType, zTType2;
  BOOL bRollbackRequired, bFirstLot, bNewLot, bInTrans;
  BOOL bUseTrans;
  char sLastSecNo[13], sLastWi[2], sLastSecXtend[3];
  char sLastAcctType[2], sLastTranType[3], sLastDrCr[3];
  double fTradUnit, fExRate, fIncExRate, fPortXrate;
  double fEqtWhRate, fBondWhRate, fEqtRclmRate, fBondRclmRate;
  int iLastID, i, iPIndex, iSecType, iSIndex, iErrors, iNumAccdiv;
  long lXrefTransNo, lLastDivintNo, lBondEligDate, lEndDate;
  short mdy[3];

  memset(&zErr, 0, sizeof(ERRSTRUCT));
  if (!bInit) {
    zErr.iSqlError = -1;
    return zErr;
  }
  lpprInitializeErrStruct(&zErr);
  i = iErrors = zPmainTable.iPmainCreated = 0;
  memset(&zTrans, 0, sizeof(zTrans));
  memset(&zTransWH, 0, sizeof(zTransWH));
  memset(&zTransAR, 0, sizeof(zTransAR));
  memset(&zTranType, 0, sizeof(zTranType));
  memset(&zAssets, 0, sizeof(zAssets));

  /* Build the table with all the branch account */
  zErr = BuildPortmainTable(&zPmainTable, sMode, iID, zCTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    InitializePortTable(&zPmainTable);
    return zErr;
  }

  /*
   ** Set the bond eligibilty date. Any foreign accounts that hold
   ** US securities will only have a witholding generated if the
   ** dated date is <= 7/15/84
   */
  mdy[0] = 7;
  mdy[1] = 15;
  mdy[2] = 1984;
  lpfnrmdyjul(mdy, &lBondEligDate);
  lXrefTransNo = lLastDivintNo = lBondEligDate = lEndDate = 0;
  /*
  ** Fetch all accdiv records, copy fields to Trans & Asset, find related
  ** sectype and trantype and call TranAlloc. Rollback or commit is done as soon
  ** as we get a different security(acct, secno, wi, secxtend and accttype). If
  ** TranAlloc returns an error, for a security, there is no need to process
  ** subsequent records for that security(it will be rollbacked).
  */
  sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = sLastAcctType[0] = '\0';
  sLastTranType[0] = sLastDrCr[0] = '\0';
  iLastID = lLastDivintNo = 0;
  bFirstLot = bNewLot = TRUE;
  bRollbackRequired = FALSE;
  bInTrans = FALSE;
  iPIndex = iSIndex = 0;
  fPortXrate = 0;
  bUseTrans = (lpfnGetTransCount() == 0);
  iNumAccdiv = 0;
  __try {

    while (TRUE) {
      memset(&zAccdiv, 0, sizeof(ACCDIV));
      lEndDate = GetPaymentsEndingDate(lValDate);
      lpprSelectAllAccdiv(&zAccdiv, &iSecType, &fTradUnit, &fExRate,
                          &fIncExRate, sMode, sProcessFlag, iID, sSecNo, sWi,
                          sSecXtend, sAcctType, lTransNo, lEndDate, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break; /* We are done */
      } else if (zErr.iSqlError) {
        InitializePortTable(&zPmainTable);
        return (lpfnPrintError("Error Fetching Accdiv", iLastID, 0, "", 0,
                               zErr.iSqlError, zErr.iIsamCode, "DIPAY PROCESS1",
                               FALSE));
      }

      // If the process flag is set to 'S' - only pay stock dividends and splits
      if (sProcessFlag[0] == 'S' && (strcmp(zAccdiv.sTranType, "SP") != 0 &&
                                     strcmp(zAccdiv.sTranType, "SD") != 0 &&
                                     strcmp(zAccdiv.sTranType, "SX") != 0 &&
                                     strcmp(zAccdiv.sTranType, "SB") != 0 &&
                                     strcmp(zAccdiv.sTranType, "RS") != 0 &&
                                     strcmp(zAccdiv.sTranType, "RX") != 0))
        continue;
      /* If the process flag is set to 'I' - only pay income, or capital gain
       * payments */
      else if (sProcessFlag[0] == 'I' &&
               (strcmp(zAccdiv.sTranType, "AD") != 0 &&
                strcmp(zAccdiv.sTranType, "RD") != 0 &&
                strcmp(zAccdiv.sTranType, "RI") != 0 &&
                strcmp(zAccdiv.sTranType, "LD") != 0 &&
                strcmp(zAccdiv.sTranType, "ST") != 0 &&
                strcmp(zAccdiv.sTranType, "LT") != 0))
        continue;
      /* If the process flag is set to 'L' - only pay liquidating dividends
       * payments */
      else if (sProcessFlag[0] == 'L' && strcmp(zAccdiv.sTranType, "LD") != 0)
        continue;

      // if it is an income, liquidating dividend or capital gain transaction
      // and account is not set to pay income automatically,
      // or income to be generated = 0 (due to small amount of units/rate)
      // ignore the record
      if ((!zPmainTable.pzPmain[i].bIncome &&
           (strcmp(zAccdiv.sTranType, "AD") == 0 ||
            strcmp(zAccdiv.sTranType, "RD") == 0 ||
            strcmp(zAccdiv.sTranType, "RI") == 0 ||
            strcmp(zAccdiv.sTranType, "LD") == 0 ||
            strcmp(zAccdiv.sTranType, "ST") == 0 ||
            strcmp(zAccdiv.sTranType, "LT") == 0)) ||
          (zPmainTable.pzPmain[i].bIncome &&
           (strcmp(zAccdiv.sTranType, "AD") == 0 ||
            strcmp(zAccdiv.sTranType, "RD") == 0 ||
            strcmp(zAccdiv.sTranType, "RI") == 0 ||
            strcmp(zAccdiv.sTranType, "LD") == 0 ||
            strcmp(zAccdiv.sTranType, "ST") == 0 ||
            strcmp(zAccdiv.sTranType, "LT") == 0) &&
           IsValueZero(zAccdiv.fIncomeAmt, 2) &&
           IsValueZero(zAccdiv.fPcplAmt, 2))) {

        // Mark the Accdiv as deleted
        lpprUpdateAccdivDeleteFlag(
            "Y", zAccdiv.iID, zAccdiv.sSecNo, zAccdiv.sWi, zAccdiv.sSecXtend,
            zAccdiv.sAcctType, zAccdiv.lTransNo, zAccdiv.lDivintNo, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          bRollbackRequired = TRUE;
        else
          lpprUpdateDivhistOneLot(zAccdiv.sTranType, "R", 0, zAccdiv.iID,
                                  zAccdiv.lTransNo, zAccdiv.lDivintNo,
                                  zAccdiv.sTranType, &zErr);

        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          bRollbackRequired = TRUE;

        continue;
      }

      // if it is a stock split or stock dividend and account is not set
      // to generate actios entries automatically, ignore the record
      if (!zPmainTable.pzPmain[i].bActions &&
          (strcmp(zAccdiv.sTranType, "SP") == 0 ||
           strcmp(zAccdiv.sTranType, "SD") == 0 ||
           strcmp(zAccdiv.sTranType, "SX") == 0 ||
           strcmp(zAccdiv.sTranType, "SB") == 0 ||
           strcmp(zAccdiv.sTranType, "RS") == 0 ||
           strcmp(zAccdiv.sTranType, "RX") == 0))
        continue;

      // If the Last ID is zero, this is the first record, set all of the "last"
      // variables
      if (iLastID == 0) {
        if (bUseTrans) {
          lpfnStartTransaction();
          bInTrans = TRUE;
        }

        iNumAccdiv++;
        iLastID = zAccdiv.iID;
        strcpy_s(sLastSecNo, zAccdiv.sSecNo);
        strcpy_s(sLastWi, zAccdiv.sWi);
        strcpy_s(sLastSecXtend, zAccdiv.sSecXtend);
        strcpy_s(sLastAcctType, zAccdiv.sAcctType);
        strcpy_s(sLastTranType, zAccdiv.sTranType);
        strcpy_s(sLastDrCr, zAccdiv.sDrCr);
        lLastDivintNo = zAccdiv.lDivintNo;

        lXrefTransNo = 0;
        bNewLot = bFirstLot = TRUE;

        // Set the portfolio's base xrate value - might be required for
        // generating WH/AR transactions
        iPIndex = FindAcctInPortdirTable(zPmainTable, zAccdiv.iID);
        if (iPIndex == -1) {
          lpfnPrintError("Invalid Account", zAccdiv.iID, 0, "T", 14, 0, 0,
                         "DIPAY PROCESS2", FALSE);
          bRollbackRequired = TRUE;
          continue;
        }
        fPortXrate = zCTable.zCurrency[zPmainTable.pzPmain[iPIndex].iCurrIndex]
                         .fCurrExrate;

        // Find the matching sectype row for the security - It may be required
        // for generating WH/AR transactions
        iSIndex = FindSecType(zSTTable, iSecType);
        if (iSIndex < 0) {
          zErr =
              lpfnPrintError("Invalid Sectype", zAccdiv.iID, zAccdiv.lTransNo,
                             "T", 14, 0, 0, "DIPAY PROCESS2", FALSE);
          bRollbackRequired = TRUE;
          continue;
        }
      } // LastID = 0, first record

      /*
      ** Check to see if the account or dividend has changed. If any one of the
      * variables has changed,
      ** check the roll-back required flag.  If the flag is true, then an error
      * occured in processing
      ** the previous lots of this security and the database must roll back all
      * the work, otherwise the
      ** work is committed. If nothing has changed and some error occured
      * previously, don't do the rollback
      ** yet, but skip all subsequent lots without processing(they will be
      * rollback when divint-no changes).
      */
      if (zAccdiv.iID == iLastID && strcmp(zAccdiv.sSecNo, sLastSecNo) == 0 &&
          strcmp(zAccdiv.sWi, sLastWi) == 0 &&
          strcmp(zAccdiv.sSecXtend, sLastSecXtend) == 0 &&
          strcmp(zAccdiv.sAcctType, sLastAcctType) == 0 &&
          strcmp(zAccdiv.sTranType, sLastTranType) == 0 &&
          strcmp(zAccdiv.sDrCr, sLastDrCr) == 0 &&
          lLastDivintNo == zAccdiv.lDivintNo) {
        if (bRollbackRequired)
          continue;
      } // same dividend for same account
      else // either the account or dividend has changed
      {
        if (bRollbackRequired) {
          if (bUseTrans && bInTrans) {
            lpfnRollbackTransaction();
            bInTrans = FALSE;
          }
          bRollbackRequired = FALSE;
        }
        // If it is an income trade (sec-impact = 0) and either account does not
        // pay income by lot, then there must be aggregated income for the
        // previous divint_no, post that trade
        else if (zTranType.lSecImpact == 0 &&
                 (!zPmainTable.pzPmain[iPIndex].bIncByLot)) {
          if (!IsValueZero(zTrans.fIncomeAmt, 2) ||
              !IsValueZero(zTrans.fPcplAmt, 2)) {
            zErr = PostTrades(zTrans, zTransWH, zTransAR, zTranType,
                              zSTTable.zSType[iSIndex], zAssets,
                              zPmainTable.pzPmain[iPIndex].bIncByLot);
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
              if (bUseTrans && bInTrans) {
                lpfnRollbackTransaction();
                bInTrans = FALSE;
              }
              bRollbackRequired = FALSE;
            } else if (bUseTrans && bInTrans) {
              lpfnCommitTransaction();
              bInTrans = FALSE;
            }
          } /* if amount is not zero */
        } // if secimpact = 0 and either inc by lot is false
        else if (bUseTrans &&
                 bInTrans) // no rollback required, and either not an income
        {                  // transaction, or paying incomebylot
          lpfnCommitTransaction();
          bInTrans = FALSE;
        }

        /*
        ** Begin work for the next security/divint_no and set the "last"
        * variables
        ** to the new security and/or account
        */
        if (bUseTrans && !bInTrans) {
          lpfnStartTransaction();
          bInTrans = TRUE;
        }
        iLastID = zAccdiv.iID;
        strcpy_s(sLastSecNo, zAccdiv.sSecNo);
        strcpy_s(sLastWi, zAccdiv.sWi);
        strcpy_s(sLastSecXtend, zAccdiv.sSecXtend);
        strcpy_s(sLastAcctType, zAccdiv.sAcctType);
        strcpy_s(sLastTranType, zAccdiv.sTranType);
        strcpy_s(sLastDrCr, zAccdiv.sDrCr);
        lLastDivintNo = zAccdiv.lDivintNo;
        lXrefTransNo = 0;
        bNewLot = bFirstLot = TRUE;

        // Set the portfolio's base xrate value - might be required for
        // generating WH/AR transactions
        iPIndex = FindAcctInPortdirTable(zPmainTable, zAccdiv.iID);
        if (iPIndex == -1) {
          lpfnPrintError("Invalid Account", zAccdiv.iID, 0, "T", 14, 0, 0,
                         "DIPAY PROCESS2", FALSE);
          bRollbackRequired = TRUE;
          continue;
        }
        fPortXrate = zCTable.zCurrency[zPmainTable.pzPmain[iPIndex].iCurrIndex]
                         .fCurrExrate;

        // Find the matching sectype row for the security - It may be required
        // for generating WH/AR transactions
        iSIndex = FindSecType(zSTTable, iSecType);
        if (iSIndex < 0) {
          zErr =
              lpfnPrintError("Invalid Sectype", zAccdiv.iID, zAccdiv.lTransNo,
                             "T", 14, 0, 0, "DIPAY PROCESS2", FALSE);
          bRollbackRequired = TRUE;
          continue;
        }
      } // if different account or divint

      // Create the transaction and assets records for processing
      if (bNewLot) // either the account is paying income by lot or this is a
                   // split transaction
      {
        bNewLot = FALSE;
        zErr = CreateTransFromAccdiv(
            &zTrans, zAccdiv, zCTable, fPortXrate, fExRate, fIncExRate,
            zSTTable.zSType[iSIndex].sPrimaryType, lValDate);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          bRollbackRequired = TRUE;
          continue;
        }

        CopyFieldsFromTransToAssets(zTrans, fTradUnit, &zAssets);

        // Get the trantype row for the transaction
        lpprSelectTranType(&zTranType, zTrans.sTranType, zTrans.sDrCr, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          bRollbackRequired = TRUE;
          continue;
        }

        /*
        ** Copy the contents to the witholding and accrued reclaim layout if the
        * transactions are
        ** income trades - cash only (RI/RD) - Might need to create WH/AR
        * transactions
        */
        if (zTranType.lCashImpact != 0 && zTranType.lSecImpact == 0) {
          // Set up the witholding trade, flip the dr/cr field depending on
          // setting of original RI/RD trade
          zTransWH = zTrans;
          strcpy_s(zTransWH.sTranType, "WH");
          if (strcmp(zTrans.sDrCr, "CR") == 0)
            strcpy_s(zTransWH.sDrCr, "DR");
          else
            strcpy_s(zTransWH.sDrCr, "CR");

          // Set up the accrued reclaim trade, no changes to dr/cr column
          zTransAR = zTrans;
          strcpy_s(zTransAR.sTranType, "AR");
        } // secimpact = 0, cashimpact != 0
      } // if NewLot is TRUE
      else // account is not paying income by taxlot, aggregate the amount
      {
        zTrans.lTaxlotNo = zAccdiv.lTransNo;
        zTrans.fUnits += zAccdiv.fUnits;
        zTrans.fIncomeAmt += zAccdiv.fIncomeAmt;
      }

      // Mark the Accdiv as deleted
      lpprUpdateAccdivDeleteFlag("Y", zTrans.iID, zTrans.sSecNo, zTrans.sWi,
                                 zTrans.sSecXtend, zTrans.sAcctType,
                                 zTrans.lTaxlotNo, zTrans.lDivintNo, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        bRollbackRequired = TRUE;
        continue;
      }

      /*
      ** For cash impact trades(RI/RD/LD/LT/ST), principal or income amt
      ** should be non zero and for sec impact trades(splits), units should
      ** be non zero, if for some reason that's not the case, ignore the
      * transaction
      */
      if (zTranType.lSecImpact != 0 &&
          IsValueZero(zTrans.fUnits, 5)) { // splits and no units
        bNewLot = TRUE;
        continue;
      }

      if (zTranType.lCashImpact != 0 && IsValueZero(zTrans.fIncomeAmt, 2) &&
          IsValueZero(zTrans.fPcplAmt,
                      2)) { // income (or gain loss) and no money
        bNewLot = TRUE;
        continue;
      }

      /*
      ** If the transaction has a security impact (SP, SD, RS etc), OR income is
      * being paid by lot,
      ** set the xref_trans_no on the trans record to link all matching rows.
      ** Send Transaction to TranAlloc for final processing and posting,
      * aggregate them (if income not being paid by lot)
      */
      if (zTranType.lSecImpact != 0 || zPmainTable.pzPmain[iPIndex].bIncByLot) {
        zTrans.lXrefTransNo = lXrefTransNo;
        zErr = lpfnTranAlloc(&zTrans, zTranType, zSTTable.zSType[iSIndex],
                             zAssets, zDTr, 0, NULL, "C", FALSE);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          bRollbackRequired = TRUE;
          continue;
        }

        if (zTrans.lXrefTransNo == 0) {
          lpprUpdateXrefTransNo(zTrans.lTransNo, zTrans.iID, zTrans.lTransNo,
                                &zErr);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
            bRollbackRequired = TRUE;
            continue;
          }

          lpprUpdateDivhistAllLots(zTrans.sTranType, "T", zTrans.lTransNo,
                                   zTrans.iID, zTrans.lDivintNo,
                                   zAccdiv.sTranType, &zErr);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
            bRollbackRequired = TRUE;
            continue;
          }
        }

        bNewLot = TRUE;
      } // secimpact != 0 or incbylot is true

      if (bFirstLot && (zTranType.lSecImpact != 0 ||
                        zPmainTable.pzPmain[iPIndex].bIncByLot)) {
        lXrefTransNo = zTrans.lTransNo;
        bFirstLot = FALSE;
      }

      // Rest of the code in the loop is dealing with withholding and reclaim
      // processing. If splits/LD, no further processing is required. For RI/RD
      // may need to create WH/AR transactions
      if (zTranType.lSecImpact != 0)
        continue;

      // Witholdings and Reclaims are only for cash instruments (RI/RD)
      zErr = GetWithholdingsRate(zTrans.iID, zTrans.lTrdDate, zTrans.sCurrId,
                                 zPmainTable.pzPmain[iPIndex].iCurrIndex,
                                 zPmainTable.pzPmain[iPIndex].iFiscalMonth,
                                 zPmainTable.pzPmain[iPIndex].iFiscalDay,
                                 &fEqtWhRate, &fBondWhRate, &fEqtRclmRate,
                                 &fBondRclmRate);
      // SB 6/23/2002 - There should be a test to make sure no error is returned
      // from GetWithholdingsRate function, but not adding that check right now
      // because don't know if function is working correctly or not and at this
      // point no withholding rates are set on the currency table.

      // If the security is a bond and the fixed income witholding rate is zero,
      // skip
      if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") == 0 &&
          fBondWhRate == 0.0) {
        zTransWH.fIncomeAmt = zTransAR.fIncomeAmt = 0;
        continue;
      }

      // If the security is an equity and the equity witholding rate is zero,
      // skip
      if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") != 0 &&
          fEqtWhRate == 0.0) {
        zTransWH.fIncomeAmt = zTransAR.fIncomeAmt = 0;
        continue;
      }

      /*
      ** Check for withholding and reclaim rates for the curr id of the trans
      ** If the security is foreign, check the withold reclaims table. If it
      ** is US, check the withold rates on the portdir table
      */
      if (strcmp(zTrans.sCurrId, "USD") == 0) {
        /*
        ** If the security is a tbill, govt bond/note, municipal bond or
        ** municipal unit - no witholding
        */
        if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") == 0 &&
            (strcmp(zSTTable.zSType[iSIndex].sSecondaryType, "T") == 0 ||
             strcmp(zSTTable.zSType[iSIndex].sSecondaryType, "G") == 0 ||
             strcmp(zSTTable.zSType[iSIndex].sSecondaryType, "M") == 0)) {
          zTransWH.fIncomeAmt = zTransAR.fIncomeAmt = 0;
          continue;
        }

        /*
        ** If the security is an equity and the dividend type is coded
        ** to be a capital gain - no witholding
        */
        if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") != 0 &&
            (strcmp(zTrans.sDivType, "27") == 0 ||
             strcmp(zTrans.sDivType, "57") == 0)) {
          zTransWH.fIncomeAmt = zTransAR.fIncomeAmt = 0;
          continue;
        }

        // Get the dated date
        if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") == 0) {
          // strcpy_s(sFixSecNo, zTransWH.sSecNo);
          // strcpy_s(sFixWi, zTransWH.sWi);
          lpprSelectFixedInc(zTransWH.sSecNo, zTransWH.sWi, &zFinc, &zErr);
          if (zErr.iSqlError) {
            zErr = lpfnPrintError("Error Fetching Datd Date", 0, 0, "", 0,
                                  zErr.iSqlError, zErr.iIsamCode, "DIPAY 3",
                                  FALSE);
            bRollbackRequired = TRUE;
            continue;
          }

          if (zFinc.lDatedDate == 0 || zFinc.lDatedDate > lBondEligDate) {
            zTransWH.fIncomeAmt = zTransAR.fIncomeAmt = 0;
            continue;
          }
        } // PrimaryType = 'Bond'
      } // Currency is USD

      /*
      ** Create and Insert a divhist entry. This is necessary as there was no
      * divhist entry for
      ** 'wh' or 'ar' created when the security accrued (in digenerate program)
      */
      InitializeDivhist(&zDivhist);

      zDivhist.iID = zAccdiv.iID;
      strcpy_s(zDivhist.sSecNo, zAccdiv.sSecNo);
      strcpy_s(zDivhist.sWi, zAccdiv.sWi);
      zDivhist.iSecID = zAccdiv.iSecID;
      strcpy_s(zDivhist.sSecXtend, zAccdiv.sSecXtend);
      strcpy_s(zDivhist.sAcctType, zAccdiv.sAcctType);
      zDivhist.lTransNo = zAccdiv.lTransNo;
      zDivhist.lDivintNo = zAccdiv.lDivintNo;

      zDivhist.fUnits = zAccdiv.fUnits;

      strcpy_s(zDivhist.sTranType, "WH");
      strcpy_s(zDivhist.sTranLocation, "A");

      zDivhist.lExDate = zAccdiv.lTrdDate;
      zDivhist.lPayDate = zAccdiv.lStlDate;

      lpprInsertDivhist(&zDivhist, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        bRollbackRequired = TRUE;
        continue;
      }

      // Check the witholding rate by primary type and calculate the withholding
      // amount
      if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") == 0)
        zTransWH.fIncomeAmt = zTrans.fIncomeAmt * fBondWhRate;
      else
        zTransWH.fIncomeAmt = zTrans.fIncomeAmt * fEqtWhRate;

      if (zPmainTable.pzPmain[iPIndex].bIncByLot) {
        /* Get the trantype row for the transaction */
        lpprSelectTranType(&zTType2, zTransWH.sTranType, zTransWH.sDrCr, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          bRollbackRequired = TRUE;
          continue;
        }

        zErr = lpfnTranAlloc(&zTransWH, zTType2, zSTTable.zSType[iSIndex],
                             zAssets, zDTr, 0, NULL, "C", FALSE);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          bRollbackRequired = TRUE;
          continue;
        }
      } // if IncByLot

      // If the currency is USD(there are no US reclaim) or if the reclaim  rate
      // is zero, skip.
      if (strcmp(zTrans.sCurrId, "USD") == 0) {
        zTransAR.fIncomeAmt = 0;
        continue;
      }

      if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") == 0 &&
          fBondRclmRate == 0) {
        zTransAR.fIncomeAmt = 0;
        continue;
      }

      if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") != 0 &&
          fEqtRclmRate == 0) {
        zTransAR.fIncomeAmt = 0;
        continue;
      }

      /* Insert a divhist entry for accrued reclaims */
      strcpy_s(zDivhist.sTranType, "AR");
      lpprInsertDivhist(&zDivhist, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        bRollbackRequired = TRUE;
        continue;
      }

      // Check the accrued reclaim rate by primary type and calculate the
      // reclaim amount
      if (strcmp(zSTTable.zSType[iSIndex].sPrimaryType, "B") == 0)
        zTransAR.fIncomeAmt = zTrans.fIncomeAmt * fBondRclmRate;
      else
        zTransAR.fIncomeAmt = zTrans.fIncomeAmt * fEqtRclmRate;

      if (zPmainTable.pzPmain[iPIndex].bIncByLot) {
        /* Get the trantype row for the transaction */
        lpprSelectTranType(&zTType2, zTransAR.sTranType, zTransAR.sDrCr, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          bRollbackRequired = TRUE;
          continue;
        }

        zErr = lpfnTranAlloc(&zTransAR, zTType2, zSTTable.zSType[iSIndex],
                             zAssets, zDTr, 0, NULL, "C", FALSE);
      } // if IncByLot
    } /* while TRUE */
  } // try
  __except (lpfnAbortTransaction(bUseTrans)) {
  }

  // Rollback or commit the last transaction
  if (bRollbackRequired) {
    if (bUseTrans && bInTrans) {
      lpfnRollbackTransaction();
      bInTrans = FALSE;
    }
  } else {
    /*
    ** If the account is not paying income by single lot - there
    ** may be one last set of trades to book, check and book
    */
    if (iNumAccdiv) {
      if (!zPmainTable.pzPmain[iPIndex].bIncByLot &&
          zTranType.lSecImpact == 0) {
        if (!IsValueZero(zTrans.fIncomeAmt, 2) ||
            !IsValueZero(zTrans.fPcplAmt, 2)) {
          zErr = PostTrades(zTrans, zTransWH, zTransAR, zTranType,
                            zSTTable.zSType[iSIndex], zAssets,
                            zPmainTable.pzPmain[iPIndex].bIncByLot);
          if (bUseTrans && bInTrans) {
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
              lpfnRollbackTransaction();
            else
              lpfnCommitTransaction();

            bInTrans = FALSE;
          }
        } /* if income amt is not zero */
      } /* if not inc by lot */
    }
  } // Rollbackrequired = FALSE

  /* Free up the memory */
  InitializePortTable(&zPmainTable);

  if (bUseTrans && bInTrans) {
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      lpfnRollbackTransaction();
    else
      lpfnCommitTransaction();

    bInTrans = FALSE;
  }

  return zErr;
} /* PayDivint */

/**
** Function to find a portdir id in the portdir table
**/
int FindAcctInPortdirTable(PORTTABLE zPTable, int iID) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (i < zPTable.iNumPmain && iIndex == -1) {
    if (zPTable.pzPmain[i].iID == iID)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findAcctInPortdirTable */

/**
** Function to find a currency id in the currency table
**/
int FindCurrIdInWithRclTable(WITHRECLTABLE zWRTable, char *sCurrId) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (i < zWRTable.iNumWithRecl && iIndex == -1) {
    if (strcmp(zWRTable.zWithRecl[i].sCurrId, sCurrId) == 0)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findCurrIdInWithReclTable */

/**
** Function to build a table of all the currencies witholdings and
** reclaim rates
**/
ERRSTRUCT BuildWithReclTable(WITHRECLTABLE *pzWRTable) {
  ERRSTRUCT zErr;
  char sCurrId[5];
  WITHHOLDRCLM zWR;

  lpprInitializeErrStruct(&zErr);
  pzWRTable->iNumWithRecl = 0;

  while (zErr.iSqlError == 0) {
    lpprSelectWithrclm(&zWR, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return (lpfnPrintError("Error Fetching WithRecl Cursor", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "DIVPAY BUILDWITH1", FALSE));

    if (pzWRTable->iNumWithRecl == NUMWITHRECL)
      return (lpfnPrintError("Withholding Reclaim Table Is Full", 0, 0, "", 997,
                             0, 0, "DIVPAY BUILDWITH2", FALSE));

    strcpy_s(pzWRTable->zWithRecl[pzWRTable->iNumWithRecl].sCurrId, sCurrId);
    pzWRTable->zWithRecl[pzWRTable->iNumWithRecl].fFxWithRate =
        zWR.fFixedincWithhold;
    pzWRTable->zWithRecl[pzWRTable->iNumWithRecl].fFxRclmRate =
        zWR.fFixedincRclm;
    pzWRTable->zWithRecl[pzWRTable->iNumWithRecl].fEqWithRate =
        zWR.fEquityWithhold;
    pzWRTable->zWithRecl[pzWRTable->iNumWithRecl].fEqRclmRate = zWR.fEquityRclm;
    pzWRTable->iNumWithRecl++;

  } /* while no error */

  return zErr;
} /* BuildWithRecltable */

/**
** Function to return the base exchange rate of the portfolio and
** the flag that indicates whether to create income transactions as
** a summary or as a detail (i.e. by lot)
** /
ERRSTRUCT GetPortmainInfo(PORTTABLE zPTable, int iID, char *sBaseCurrId,
                          double *pfPortBaseXrate, BOOL *pbIncByLot)
                          double *pfSwh, double *pfBwh)
{
  int       i, iPortBaseIndex;
  ERRSTRUCT zErr;

  i = -1;
  lpprInitializeErrStruct(&zErr);

  / *
  ** Set the portfolio's base xrate value, if the branch account
  ** does not match to one pointed to in the table, reset the index first
  * /
  i = FindBrAcctInPortdirTable(zPTable, iID);
  if (i == -1)
                return(lpfnPrintError("Invalid Branch Account", iID, 0, "T", 14,
                                                                    0, 0, "DIPAY
PROCESS3", FALSE));

  // Retrieve the combine income flag from the portdir
  *pbIncByLot = zPTable.pzPmain[i].bIncByLot;

  // Get the exchange rate for the base currency of the account
  iPortBaseIndex = zPTable.pzPmain[i].iCurrIndex;
  *pfPortBaseXrate = zCTable.zCurrency[iPortBaseIndex].fCurrExrate;
//  *pfSwh = zPTable.pzPmain[i].fSwh;
//  *pfBwh = zPTable.pzPmain[i].fBwh;

  return zErr;
} // GetPortmainInfo */

/**
** This function is used to populate the transaction record for
** accruing transaction (enteries from the accdiv table)
**/
ERRSTRUCT CreateTransFromAccdiv(TRANS *pzTR, ACCDIV zAccdiv, CURRTABLE zCTable,
                                double fPortBaseXrate, double fExRate,
                                double fIncExRate, char *sPrimaryType,
                                long lValDate) {
  ERRSTRUCT zErr;
  char sTemp[30];
  int iCIndex;
  long lDate, lTrdDate, lPriceDate;

  lpprInitializeErrStruct(&zErr);
  lpprInitializeTransStruct(pzTR);

  /* Set the branch account, sec no, account type and taxlot */
  pzTR->iID = zAccdiv.iID;
  strcpy_s(pzTR->sSecNo, zAccdiv.sSecNo);
  strcpy_s(pzTR->sWi, zAccdiv.sWi);
  pzTR->iSecID = zAccdiv.iSecID;
  strcpy_s(pzTR->sSecXtend, zAccdiv.sSecXtend);
  strcpy_s(pzTR->sAcctType, zAccdiv.sAcctType);
  strcpy_s(pzTR->sSecSymbol, zAccdiv.sSecSymbol);
  pzTR->lTaxlotNo = zAccdiv.lTransNo;

  /* Set the transaction type */
  strcpy_s(pzTR->sTranType, zAccdiv.sTranType);
  strcpy_s(pzTR->sDrCr, zAccdiv.sDrCr);

  /*
  ** If the transaction type is 'AD, set the transaction type to 'RI'
  ** if the security is a bond, otherwise set it to 'RD'.  All other
  ** types remain as they were on the accdiv
  */
  if (strcmp(zAccdiv.sTranType, "AD") == 0) {
    if (strcmp(zAccdiv.sIncomeFlag, "Y") ==
        0) // IncomeFlag - Y means taxlot is held for safekeeping
      strcpy_s(
          pzTR->sTranType,
          "EI"); // only, in that case generate an External Income transaction
    else if (strcmp(sPrimaryType, "B") == 0)
      strcpy_s(pzTR->sTranType, "RI");
    else
      strcpy_s(pzTR->sTranType, "RD");
  }
  strcpy_s(pzTR->sDrCr, zAccdiv.sDrCr);

  /* Set the units and original face */
  pzTR->fUnits = zAccdiv.fUnits;
  pzTR->fOrigFace = zAccdiv.fOrigFace;
  lpprSelectStarsDate(&lTrdDate, &lPriceDate, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /* Set all the dates */
  pzTR->lTrdDate = zAccdiv.lTrdDate;
  pzTR->lStlDate = zAccdiv.lStlDate;
  pzTR->lEffDate = zAccdiv.lEffDate;
  if (strcmp(zAccdiv.sTranType, "ST") == 0)
    pzTR->lOpenTrdDate = pzTR->lTrdDate;

  pzTR->lEntryDate = lTrdDate;
  pzTR->lPostDate = lTrdDate;
  _strdate(sTemp);             // current date in string formot
  lpfnrstrdate(sTemp, &lDate); // change string date to a julian date
  pzTR->lCreateDate = lDate;
  _strtime(pzTR->sCreateTime); // current time

  /* Set the monies */
  pzTR->fPcplAmt = zAccdiv.fPcplAmt;
  pzTR->fIncomeAmt = zAccdiv.fIncomeAmt;

  // 11/12/01 - vay
  // adjust for LT/ST transaction
  if (strcmp(zAccdiv.sTranType, "LT") == 0 ||
      strcmp(zAccdiv.sTranType, "ST") == 0) {
    pzTR->fPcplAmt = zAccdiv.fIncomeAmt;
    pzTR->fIncomeAmt = 0;
  }

  /* Set the currency id and account types */
  strcpy_s(pzTR->sCurrId, zAccdiv.sCurrId);
  strcpy_s(pzTR->sCurrAcctType, zAccdiv.sCurrAcctType);
  strcpy_s(pzTR->sIncCurrId, zAccdiv.sIncCurrId);
  strcpy_s(pzTR->sIncAcctType, zAccdiv.sIncAcctType);

  strcpy_s(pzTR->sSecCurrId, zAccdiv.sSecCurrId);
  strcpy_s(pzTR->sAccrCurrId, zAccdiv.sAccrCurrId);

  /* Set the dividend/split info */
  pzTR->lDivintNo = zAccdiv.lDivintNo;
  strcpy_s(pzTR->sDivType, zAccdiv.sDivType);
  pzTR->fDivFactor = zAccdiv.fDivFactor;

  pzTR->fOrigYld = zAccdiv.fOrigYld;
  pzTR->fEffMatPrice = zAccdiv.fEffMatPrice;

  /* Set remaining information */
  strcpy_s(pzTR->sAcctMthd, zAccdiv.sAcctMthd);
  strcpy_s(pzTR->sTransSrce, zAccdiv.sTransSrce);

  strcpy_s(pzTR->sDtcInclusion, zAccdiv.sDtcInclusion);
  strcpy_s(pzTR->sDtcResolve, zAccdiv.sDtcResolve);

  strcpy_s(pzTR->sIncomeFlag, zAccdiv.sIncomeFlag);
  strcpy_s(pzTR->sLetterFlag, zAccdiv.sLetterFlag);
  strcpy_s(pzTR->sLedgerFlag, zAccdiv.sLedgerFlag);

  strcpy_s(pzTR->sCreatedBy, zAccdiv.sCreatedBy);

  /*
  ** Set the transaction's security and accrual exchange rates,
  ** by dividing the security exchange rate by the exchange rate
  ** of the base currency of the account
  */
  if (!IsValueZero(fPortBaseXrate, 12)) {
    pzTR->fSecBaseXrate = fExRate / fPortBaseXrate;
    pzTR->fAccrBaseXrate = fIncExRate / fPortBaseXrate;
  }

  /*
  ** Find match for curr_id in currency table and set the transaction's
  ** currency exchange rate
  ** This is necessary so that at some point in the future, the system
  ** will be flexible enough to pay a dividend/split in another currency
  ** other than those indicated on the security
  */
  iCIndex = FindCurrIdInCurrencyTable(zCTable, pzTR->sCurrId);
  if (iCIndex == -1)
    return (lpfnPrintError("Invalid Base Currency", pzTR->iID, pzTR->lTaxlotNo,
                           "T", 14, 0, 0, "DIPAY PROCESS4", FALSE));
  if (!IsValueZero(fPortBaseXrate, 12))
    pzTR->fBaseXrate = zCTable.zCurrency[iCIndex].fCurrExrate / fPortBaseXrate;

  pzTR->fSysXrate = zCTable.zCurrency[iCIndex].fCurrExrate;

  /*
  ** If the income currency id is the same as the currency id,
  ** there is no need to search, set the income exchange rates
  ** using the currency exchange rates from above
  */
  if (strcmp(pzTR->sIncCurrId, pzTR->sCurrId) == 0) {
    pzTR->fIncBaseXrate = pzTR->fBaseXrate;
    pzTR->fIncSysXrate = pzTR->fSysXrate;
  } else {
    /*
    ** Find the income currency id on the currency table and assign
    ** the exchange rate to the transaction
    */
    iCIndex = FindCurrIdInCurrencyTable(zCTable, pzTR->sIncCurrId);
    if (iCIndex == -1)
      return (lpfnPrintError("Invalid Base Income Currency", pzTR->iID,
                             pzTR->lTaxlotNo, "T", 121, 0, 0, "DIPAY PROCESS5",
                             FALSE));
    if (!IsValueZero(fPortBaseXrate, 12))
      pzTR->fIncBaseXrate =
          zCTable.zCurrency[iCIndex].fCurrExrate / fPortBaseXrate;
    pzTR->fIncSysXrate = zCTable.zCurrency[iCIndex].fCurrExrate;
  }

  return zErr;
} /* create trans from accdiv */

/**
** Function to post transactions when the account is set to pay only
** one income trade per many lots
**/
ERRSTRUCT PostTrades(TRANS zTrans, TRANS zTransWH, TRANS zTransAR,
                     TRANTYPE zTranType, SECTYPE zSecType, ASSETS zAssets,
                     BOOL bIncomeByLot) {
  ERRSTRUCT zErr;
  TRANTYPE zTType2;
  DTRANSDESC zDTr[1];

  lpprInitializeErrStruct(&zErr);

  // Reset the taxlot number to zero if income is not for a specific lot. Send
  // the trade to tranalloc to post
  if (!bIncomeByLot)
    zTrans.lTaxlotNo = 0;

  zErr = lpfnTranAlloc(&zTrans, zTranType, zSecType, zAssets, zDTr, 0, NULL,
                       "C", FALSE);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /* If there is a withholding trade, send to tranalloc and post */
  if (zTransWH.fIncomeAmt != 0) {
    zTransWH.lTaxlotNo = 0;

    /* Get the trantype row for the transaction */
    lpprSelectTranType(&zTType2, zTransWH.sTranType, zTransWH.sDrCr, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zErr = lpfnTranAlloc(&zTransWH, zTType2, zSecType, zAssets, zDTr, 0, NULL,
                         "C", FALSE);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  /* If there is a accr reclm trade, send to tranalloc and post */
  if (zTransAR.fIncomeAmt != 0.0) {
    zTransAR.lTaxlotNo = 0;

    /* Get the trantype row for the transaction */
    lpprSelectTranType(&zTType2, zTransAR.sTranType, zTransAR.sDrCr, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zErr = lpfnTranAlloc(&zTransAR, zTType2, zSecType, zAssets, zDTr, 0, NULL,
                         "C", FALSE);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  return zErr;
} // PostTrades

/**
** Function to get stock and bond withholding and reclaim rates for a trade. If
* the
** trade's currency is same as portfolio's base currency, try to get portfolio's
** withholding rates for the tax year in which that trade falls. If no
** record is found or if trade's currency is different than portfolio's
** base currency, get the withholding rates for that currency. Get the recliam
** rate of the portfolio from its currency record.
**/
ERRSTRUCT GetWithholdingsRate(int iID, long lTrdDate, char *sCurrId,
                              int iPortCurrIndex, short iFiscalEndMonth,
                              short iFiscalEndDay, double *pfEWhRate,
                              double *pfBWhRate, double *pfERcRate,
                              double *pfBRcRate) {
  static long lStartDate = -1, lEndDate = -1;
  static int iLastID = -1;
  static BOOL bPtaxFound = FALSE;
  static double fEqRate = 0, fBondRate = 0;
  ERRSTRUCT zErr;
  PORTTAX zPR;
  short iMDY[3];
  int iWIndex;

  lpprInitializeErrStruct(&zErr);

  if (iPortCurrIndex < 0 || iPortCurrIndex >= zCTable.iNumCurrency)
    return (lpfnPrintError("Invalid Base Currency Index", iID, 0, "", 999, 0, 0,
                           "DIPAY GETWITHHOLD1", FALSE));

  // Find the currency in the currency table, if not found then all
  // the withholding and reclaim rates are zero. else get the reclaim
  // rate for the currency
  iWIndex = FindCurrIdInWithRclTable(zWRTable, sCurrId);
  if (iWIndex == -1) {
    *pfEWhRate = *pfBWhRate = *pfERcRate = *pfBRcRate = 0;
    return zErr;
  }

  *pfBRcRate = zWRTable.zWithRecl[iWIndex].fFxRclmRate / 100;
  *pfERcRate = zWRTable.zWithRecl[iWIndex].fEqRclmRate / 100;

  /*
  ** If trade's currency is different than portfolio's base currency
  ** then get the withholding rate of the currency, else if a record
  ** for the fiscal year(in which the trade occured) is found, use the
  ** withholding rate on it, else use currency's withholding rate
  */
  if (strcmp(zCTable.zCurrency[iPortCurrIndex].sCurrId, sCurrId) == 0) {
    if (iLastID != iID || lTrdDate < lStartDate || lTrdDate > lEndDate) {
      // Convert trade date into mm, dd, yy
      if (lpfnrjulmdy(lTrdDate, iMDY) < 0)
        return (lpfnPrintError("Invalid Trade Date", iID, 0, "", 999, 0, 0,
                               "DIPAY GETWITHHOLD2", FALSE));

      /*
      ** Use fiscal year end month and date and trade date's year to
      ** make the start date and end date. E.g. if a portfolio's fiscal
      ** year ends on 12/31 and a trade occurs on 4/15/1998, then the
      ** end date will be 12/31/1998(fiscal ending month, day and trade
      ** date's year) and start date will be 1/1/1997( End date - 1 year
      ** + 1 day)
      */
      iMDY[0] = iFiscalEndMonth;
      iMDY[1] = iFiscalEndDay;
      if (lpfnrmdyjul(iMDY, &lEndDate) < 0)
        return (lpfnPrintError("Invalid Fiscal End Month/Date", iID, 0, "", 999,
                               0, 0, "DIPAY GETWITHHOLD3", FALSE));

      // substract 1 year from the end date and then add 1 day to the new date
      iMDY[2]--;
      if (lpfnrmdyjul(iMDY, &lStartDate) < 0)
        return (lpfnPrintError("Invalid Fiscal End Month/Date", iID, 0, "", 999,
                               0, 0, "DIPAY GETWITHHOLD4", FALSE));
      lStartDate++;

      // Read porrtax for this portfolio and this ending date
      lpprSelectPorttax(iID, lEndDate, &zPR, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND)
        bPtaxFound = FALSE;
      else if (zErr.iSqlError != 0)
        return zErr;
      else {
        bPtaxFound = TRUE;
        *pfBWhRate = zPR.fStockWithholdRate / 100;
        *pfEWhRate = zPR.fBondWithholdRate / 100;
        iLastID = iID;
        return zErr;
      }

      iLastID = iID;
    } // if current id not same as lastid or trade date does not fall between
      // start and end dates
  } // if trade's currency is same as portfolio's base currency

  if (strcmp(zCTable.zCurrency[iPortCurrIndex].sCurrId, sCurrId) != 0 ||
      !bPtaxFound) {
    *pfBWhRate = zWRTable.zWithRecl[iWIndex].fFxWithRate / 100;
    *pfEWhRate = zWRTable.zWithRecl[iWIndex].fEqWithRate / 100;
  }

  return zErr;
} // GetWithholdRate

/**
** Function to copy fields from Trans to assets
**/
void CopyFieldsFromTransToAssets(TRANS zTR, double fTrdUnit, ASSETS *pzAssets) {
  strcpy_s(pzAssets->sSecNo, zTR.sSecNo);
  strcpy_s(pzAssets->sWhenIssue, zTR.sWi);
  pzAssets->iID = zTR.iSecID;
  pzAssets->fTradUnit = fTrdUnit;
  strcpy_s(pzAssets->sCurrId, zTR.sCurrId);
} // CopyFieldsFromTransToAssets
