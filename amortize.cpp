/*H*
 *
 * SUB-SYSTEM: payments
 *
 * FILENAME: amortize.c
 *
 * DESCRIPTION:
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR: Shobhit Barman (Effron Enterprises, Inc)
 *
 *H*/
// History.
// 2018-11-16 J# PER-9268 Wells Audit - Initialize variables, free
// memory,deprecated string function  - sergeyn 2009-03-04 VI# 41539: added
// accretion functionality - mk 10/19/2005 Added logic to recalculate yield -
// sergeyn

#include "paydll.h"
#include "stdio.h"

extern "C" {

/*F*
** Function called by the user to do amortization processing. Valid Modes are
** B(batch), A(single account) and S(single security in an account).
** Process Flag can be M(amortize muni), O(amortize other - everything except
** munis) and A(All).
*F*/
ERRSTRUCT STDCALL WINAPI Amortize(long lValDate, const char *sMode,
                                  const char *sProcessFlag, int iID,
                                  const char *sSecNo, const char *sWi,
                                  const char *sSecXtend, const char *sAcctType,
                                  long lTransNo) {
  ERRSTRUCT zErr;
  PINFOTABLE zPInfoTable;
  PORTTABLE zPmainTable;
  //	long         lStartDate, lForwardDate;

  lpprInitializeErrStruct(&zErr);
  zPmainTable.iPmainCreated = 0;
  zPInfoTable.iPICreated = 0;
  InitializePInfoTable(&zPInfoTable);
  //  lStartDate = lValDate - PREVIOUSDAYS;
  //  lForwardDate = lValDate + FORWARDDAYS;

  /* If in batch mode, create the file with data from holdings, divint, etc. */
  if (sMode[0] == 'B') {
    zErr = AmortUnloadAndSort(lValDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    /* Build PInfo table for dividends */
    zErr = BuildPInfoTable(&zPInfoTable, "A", lValDate, "");
    if (zErr.iBusinessError != 0)
      return zErr;
  } else {
    zPInfoTable.iPICreated = 1;
    zPInfoTable.pzPRec = (PINFO *)realloc(zPInfoTable.pzPRec, sizeof(PINFO));
    if (zPInfoTable.pzPRec == NULL)
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                             "DIVINT1", FALSE));

    zPInfoTable.pzPRec[0].iID = iID;
    zPInfoTable.pzPRec[0].lStartPosition = 0;
    zPInfoTable.iNumPI = 1;
  }

  /* Build the table with all the branch account */
  zErr = BuildPortmainTable(&zPmainTable, sMode, iID, zCTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    InitializePInfoTable(&zPInfoTable); /* Free up the memory */
    return zErr;
  }

  /* Call the function which does actual work, Added lForwardDate to do Forward
   * Transactions */
  zErr = AmortGeneration(zPmainTable, zPInfoTable, zSTTable, zCTable, lValDate,
                         sMode, sProcessFlag, sSecNo, sWi, sSecXtend, sAcctType,
                         lTransNo);

  /* Free up the memory */
  InitializePInfoTable(&zPInfoTable);
  InitializePortTable(&zPmainTable);
  return zErr;
} /* Amortize */

/*F*
** Function that does main processing of amortization for all the
** accounts in the supplied porttable.
*F*/
ERRSTRUCT AmortGeneration(PORTTABLE zPMainTable, PINFOTABLE zPInfoTable,
                          SECTYPETABLE zSTable, CURRTABLE zCTable,
                          long lValDate, const char *sMode,
                          const char *sProcessFlag, const char *sSecNo,
                          const char *sWi, const char *sSecXtend,
                          const char *sAcctType, long lTransNo) {
  ERRSTRUCT zErr;
  AMORTTABLE zATable;
  int i, j, k, m, iLastID;
  char sLastSecNo[13], sLastWi[2], sLastSecXtend[3], sLastAcctType[3];
  char sFileName[30], sErrMsg[100];
  long lLastTransNo, lTempDate, lStlDate;
  double fTempCost;
  double fTemp, fNewTotCost, fOldCost;
  double fTempPrice;
  BOOL bMonthEnd, bAmortelig, bAccretelig;
  HOLDINGS zHoldings;
  TRANS zTempTrans;

  lpprInitializeErrStruct(&zErr);
  zATable.iAmortCreated = 0;
  InitializeAmortTable(&zATable);

  /* Process Flag. M - muni, O - other(everything except muni), A - all */
  if (sProcessFlag[0] != 'M' && sProcessFlag[0] != 'O' &&
      sProcessFlag[0] != 'A')
    return (
        lpfnPrintError("Process Flag Can Only Be M(amort muni), O(amort other) "
                       "Or A(amort all)",
                       0, 0, "", 999, 0, 0, "AMORT GENRT1", FALSE));

  /* Filename for sorted file on valdate, the "S" parameter does not do anything
   * here */
  strcpy_s(sFileName, MakePricingFileName(lValDate, "", "A"));

  /* Is the val date a month end date */
  bMonthEnd = IsThisAMonthEnd(lValDate);

  zATable.iAmortCreated = 0;
  /* Do processing for all the branch accounts in the port table */
  for (i = 0; i < zPMainTable.iNumPmain; i++) {
    /*
    ** If portfolio is setup not to amortize muni and other automatically,
    ** nothing to do. If it's setup not to amortize muni and we are processing
    ** only muni(process flag - 'M'), nothing to do. In the third scenario, if
    ** if the portfolio is setup not to amortize other and we are processing
    ** only other, we have nothing to do for this portfolio.
    */
    if (!zPMainTable.pzPmain[i].bAmortMuni &&
        !zPMainTable.pzPmain[i].bAccretMuni) {
      if (!zPMainTable.pzPmain[i].bAmortOther &&
          !zPMainTable.pzPmain[i].bAccretOther)
        continue;
      else if (sProcessFlag[0] == 'M')
        continue;
    } else if (!zPMainTable.pzPmain[i].bAmortOther &&
               !zPMainTable.pzPmain[i].bAccretOther && sProcessFlag[0] == 'O')
      continue;

    /*If portfolio's amortize start date is later than val date, nothing to do*/
    if (zPMainTable.pzPmain[i].lAmortStart > lValDate)
      continue;

    /*
    ** Begin a database transaction. Transactions are done on a security basis,
    ** i.e. either all the transactions of a security(br_acct, sec_no, wi,
    ** sec_xtend and acct_type) are commited(no error) or rolled back(error)
    */

    /* Build a table of all the records for current branch account */
    zErr = BuildAmortTable(sMode, sFileName, zPMainTable.pzPmain[i].iID, sSecNo,
                           sWi, sSecXtend, sAcctType, lTransNo, &zATable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      lpfnPrintError("Amortization Could Not Be Processed",
                     zPMainTable.pzPmain[i].iID, 0, "", 999, 0, 0,
                     "AMORT GENRT2", TRUE);
      lpprInitializeErrStruct(&zErr);
      continue;
    }

    /* Process All the records for current iID */
    sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = '\0';
    sLastAcctType[0] = '\0';
    iLastID = 0;
    lLastTransNo = 0;
    for (j = 0; j < zATable.iNumAmort; j++) {
      /*
      ** On a month end, processing is done for all the lots, otherwise only
      ** those lots are processed for which lValuation Date >= eff_mat_date
      */
      if (!bMonthEnd && lValDate > zATable.pzAmort[j].lEffMatDate)
        continue;

      k = FindSecType(zSTable, zATable.pzAmort[j].iSecType);
      if (k < 0) {
        InitializeAmortTable(&zATable);
        return (lpfnPrintError("Invalid Sectype", zPMainTable.pzPmain[i].iID, 0,
                               "", 14, 0, 0, "AMORT GENRT3", FALSE));
      }

      // If a discounted security, no amortization
      if (strcmp(zATable.pzAmort[j].sPayType, "D") == 0)
        continue;

      // if processing only muni and this is not a muni, skip it
      if (strcmp(sProcessFlag, "M") == 0 &&
          strcmp(zSTable.zSType[k].sSecondaryType, "M") != 0)
        continue;

      // if processing everything except muni and this is a muni, skip it
      if (strcmp(sProcessFlag, "O") == 0 &&
          strcmp(zSTable.zSType[k].sSecondaryType, "M") == 0)
        continue;

      // if portfolio is not setup to amortize muni and this is a muni security,
      // skip it
      if (!zPMainTable.pzPmain[i].bAmortMuni &&
          !zPMainTable.pzPmain[i].bAccretMuni &&
          strcmp(zSTable.zSType[k].sSecondaryType, "M") == 0)
        continue;

      // if portfolio is not setup to amortize other(than muni) and this is a
      // non muni security, skip it
      if (!zPMainTable.pzPmain[i].bAmortOther &&
          !zPMainTable.pzPmain[i].bAccretOther &&
          strcmp(zSTable.zSType[k].sSecondaryType, "M") != 0)
        continue;

      // if effective date on the lot is greater than the current pricing date,
      // skip it
      if (lValDate < zATable.pzAmort[j].lEligDate)
        continue;

      /*
      ** If the security is same as last one processed and if there was an
      ** error in last processing, don't do any processing(all the lots are
      ** going to be rolled back). If on the other hand security is not same
      ** as last one, finish the current database transaction with a commit(no
      ** errors for any lot) or a rollback and then start a new transaction.
      */
      if (zATable.pzAmort[j].iID == iLastID &&
          strcmp(zATable.pzAmort[j].sSecNo, sLastSecNo) == 0 &&
          strcmp(zATable.pzAmort[j].sWi, sLastWi) == 0 &&
          strcmp(zATable.pzAmort[j].sSecXtend, sLastSecXtend) == 0 &&
          strcmp(zATable.pzAmort[j].sAcctType, sLastAcctType) == 0) {
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          // print error message
          continue;
        }
      } else {
        if (iLastID != 0) /* Not the First Record */
        {
          if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
            lpfnPrintError(sErrMsg, iLastID, 0, "", 999, 0, 0, "AMORT GENRT4",
                           TRUE);
            lpprInitializeErrStruct(&zErr);
          }
        } /* Not the first record */

        iLastID = zATable.pzAmort[j].iID;
        strcpy_s(sLastSecNo, zATable.pzAmort[j].sSecNo);
        strcpy_s(sLastWi, zATable.pzAmort[j].sWi);
        strcpy_s(sLastSecXtend, zATable.pzAmort[j].sSecXtend);
        strcpy_s(sLastAcctType, zATable.pzAmort[j].sAcctType);
      } /* Different security than last time */

      // pre-fill the holdings record
      InitializeHoldingsStruct(&zHoldings);
      zHoldings.fOrigYield = zATable.pzAmort[j].fOrigYield;
      zHoldings.lEffMatDate = zATable.pzAmort[j].lEffMatDate;
      zHoldings.fEffMatPrice = zATable.pzAmort[j].fEffMatPrice;
      zHoldings.fCostEffMatYld = zATable.pzAmort[j].fCostEffMatYld;
      zHoldings.fTotCost = zATable.pzAmort[j].fTotCost;
      zHoldings.fOrigCost = zATable.pzAmort[j].fOrigCost;
      zHoldings.lEffDate = zATable.pzAmort[j].lEffDate;
      zHoldings.lEligDate = zATable.pzAmort[j].lEligDate;
      zHoldings.lStlDate = zATable.pzAmort[j].lStlDate;
      strcpy_s(zHoldings.sOrigTransType, zATable.pzAmort[j].sOrigTransType);

      zHoldings.iID = zATable.pzAmort[j].iID;

      strcpy_s(zHoldings.sSecNo, zATable.pzAmort[j].sSecNo);
      strcpy_s(zHoldings.sWi, zATable.pzAmort[j].sWi);
      strcpy_s(zHoldings.sSecXtend, zATable.pzAmort[j].sSecXtend);
      strcpy_s(zHoldings.sAcctType, zATable.pzAmort[j].sAcctType);
      zHoldings.lTransNo = zATable.pzAmort[j].lTransNo;

      /*
      ** If cost eff mat yield is zero, no AM transaction can be generated. But,
      ** if the holdings has changed, update it before continuing on the next
      ** record. Also, before updating, make sure to calculate a new cost eff
      ** mat yield, if effective mat date is less than or equal to val date.
      */

      // if database option SYSVALUES.RecalcCostEffMatYld is set to TRUE,
      // try to calculate cost eff mat yield and compare it to what holdings
      // have if they don't match - update holdings record with newly calculated
      // value (most likely, new call date has been introduced so we need to
      // recalc yield)

      if (zSysSet.bRecalcCostEffMatYld) {
        lpprSelectNextCallDatePrice(zATable.pzAmort[j].sSecNo,
                                    zATable.pzAmort[j].sWi, lValDate,
                                    &fTempPrice, &lTempDate, &zErr);
        if (zErr.iBusinessError != 0 ||
            (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND))
          continue;

        lStlDate = GetStlDate(zHoldings);
        // SB 7/18/2014. VI# 55500
        // If the taxlot was created by FR and was already amortized at the time
        // of FR, GetStlDate function returns effective date, in other cases it
        // returns settlement date. If date returned is the settlement date then
        // original unit cost should be used for yield calculation, else (if
        // free received lot was already amortized) get the original transaction
        // and use amortized unit cost for yield calculation.ade the sme change
        // in valuation where initial calculation takes place.
        if (lStlDate == zHoldings.lStlDate)
          fTempCost =
              zATable.pzAmort[j].fOrigCost /
              (zATable.pzAmort[j].fUnits * zATable.pzAmort[j].fTradUnit);
        else {
          lpprSelectTrans(zHoldings.iID, zHoldings.lTransNo, &zTempTrans,
                          &zErr);
          if (zErr.iSqlError == 0 && zErr.iBusinessError == 0 &&
              strcmp(zTempTrans.sTranType, "FR") == 0 &&
              !IsValueZero(zTempTrans.fTotCost, 3) &&
              !IsValueZero(zTempTrans.fUnits, 3))
            fTempCost = zTempTrans.fTotCost /
                        (zTempTrans.fUnits * zATable.pzAmort[j].fTradUnit);
          else
            fTempCost =
                zATable.pzAmort[j].fOrigCost /
                (zATable.pzAmort[j].fUnits * zATable.pzAmort[j].fTradUnit);
        }

        fTemp = lpfnCalculateYield(fTempCost, zATable.pzAmort[j].sSecNo,
                                   zATable.pzAmort[j].sWi, "C", lStlDate, 2);

        if (fTemp > 0.0 &&
            !IsValueZero(zATable.pzAmort[j].fCostEffMatYld - fTemp, 6)) {
          zATable.pzAmort[j].fCostEffMatYld = fTemp;
          zHoldings.lEffMatDate = lTempDate;
          zHoldings.fEffMatPrice = fTempPrice;
          zHoldings.fCostEffMatYld = fTemp;
          lpprUpdateAmortizeHohldings(zHoldings, &zErr);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            continue;
        }
      }

      bAmortelig = FALSE;
      bAccretelig = FALSE;
      if (zPMainTable.pzPmain[i].bAmortMuni ||
          zPMainTable.pzPmain[i].bAmortOther) {
        // if TotCost is already below Eff_Mat_Cost, do not amortize
        bAmortelig =
            ((zATable.pzAmort[j].fTotCost >
              zATable.pzAmort[j].fUnits * zATable.pzAmort[j].fTradUnit *
                  zATable.pzAmort[j].fEffMatPrice));
        // this is only needed if we want to do accretion
        //&& (zATable.pzAmort[j].fTotCost = zATable.pzAmort[j].fOrigCost))
        // continue;
      }

      if (zPMainTable.pzPmain[i].bAccretMuni ||
          zPMainTable.pzPmain[i].bAccretOther) {
        // if TotCost is already above Eff_Mat_Cost, do not accrete
        bAccretelig =
            ((zATable.pzAmort[j].fTotCost <
              zATable.pzAmort[j].fUnits * zATable.pzAmort[j].fTradUnit *
                  zATable.pzAmort[j].fEffMatPrice));
        // this is only needed if we want to do accretion
        //&& (zATable.pzAmort[j].fTotCost = zATable.pzAmort[j].fOrigCost))
        // continue;
      }
      if (!bAmortelig && !bAccretelig)
        continue;

      // calculate new price
      fTemp = lpfnCalculatePriceGivenYield(
          zATable.pzAmort[j].fCostEffMatYld, zATable.pzAmort[j].sSecNo,
          zATable.pzAmort[j].sWi, "C", lValDate, 2);
      if (fTemp < 0) {
        lpfnPrintError("Calculate new price failed", iLastID, 0, "A", 999, 0, 0,
                       "AMORT GENRT4.3", TRUE);
        continue;
      }

      fNewTotCost = RoundDouble(
          fTemp * zATable.pzAmort[j].fUnits * zATable.pzAmort[j].fTradUnit, 3);

      bAmortelig = FALSE;
      bAccretelig = FALSE;
      // if New TotCost is greater than TotCost, do nothing (we don't post
      // accretions)
      if (zPMainTable.pzPmain[i].bAmortMuni ||
          zPMainTable.pzPmain[i].bAmortOther) {
        bAmortelig = (fNewTotCost < zATable.pzAmort[j].fTotCost);
        // continue;
      }

      if (zPMainTable.pzPmain[i].bAccretMuni ||
          zPMainTable.pzPmain[i].bAccretOther) {
        bAccretelig = (fNewTotCost > zATable.pzAmort[j].fTotCost);
        // continue;
      }

      if (!bAmortelig && !bAccretelig)
        continue;

      // if New TotCost is within 1 dollar difference to TotCost, nothing to do
      if (fabs(fNewTotCost - zATable.pzAmort[j].fTotCost) < 1.0)
        continue;

      // fNewTotCost should be always greater than or equal to the
      // call(redemption) value
      fTempCost = zATable.pzAmort[j].fTotCost /
                  (zATable.pzAmort[j].fUnits * zATable.pzAmort[j].fTradUnit);
      if (bAmortelig) {
        if ((fNewTotCost < zATable.pzAmort[j].fUnits *
                               zATable.pzAmort[j].fTradUnit *
                               zATable.pzAmort[j].fEffMatPrice) &&
            (fTempCost > zATable.pzAmort[j].fEffMatPrice)) {
          // this is only needed if we want to do accretion
          //&&  (fNewTotCost > zATable.pzAmort[j].fTotCost))
          fNewTotCost = RoundDouble(zATable.pzAmort[j].fUnits *
                                        zATable.pzAmort[j].fTradUnit *
                                        zATable.pzAmort[j].fEffMatPrice,
                                    3);
        }
      }

      if (bAccretelig) {
        if ((fNewTotCost > zATable.pzAmort[j].fUnits *
                               zATable.pzAmort[j].fTradUnit *
                               zATable.pzAmort[j].fEffMatPrice) &&
            (fTempCost < zATable.pzAmort[j].fEffMatPrice)) {
          // this is only needed if we want to do accretion
          //&&  (fNewTotCost > zATable.pzAmort[j].fTotCost))
          fNewTotCost = RoundDouble(zATable.pzAmort[j].fUnits *
                                        zATable.pzAmort[j].fTradUnit *
                                        zATable.pzAmort[j].fEffMatPrice,
                                    3);
        }
      }

      // if New TotCost is within 1 dollar difference to TotCost, nothing to do
      if (fabs(fNewTotCost - zATable.pzAmort[j].fTotCost) < 1.0)
        continue;

      fOldCost = zATable.pzAmort[j].fTotCost;
      zATable.pzAmort[j].fTotCost = fNewTotCost;

      m = zPMainTable.pzPmain[i].iCurrIndex;

      // now add AM transaction to trans table
      zErr = AmortCallTranAlloc(
          zATable.pzAmort[j], zCTable.zCurrency[m].fCurrExrate, fOldCost,
          zPMainTable.pzPmain[i].sAcctMethod, zSTable.zSType[k], lValDate);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        continue;

      /*			zHoldings.fTotCost = fNewTotCost;
                              lpprUpdateAmortizeHohldings(zHoldings, &zErr);
                              if (zErr.iSqlError != 0 || zErr.iBusinessError !=
         0)
                              {
                                      lpfnPrintError("Can not update
         amortization holdings", iLastID, 0, "",  999, 0, 0, "AMORT GENRT5",
         FALSE); lpprInitializeErrStruct(&zErr); continue;
                              }*/
    } /* for j < iNumAmort */

    /* For the last lot of each branch account */
    zErr.iSqlError = zErr.iBusinessError = 0;
  } /* i < numpdir */
  InitializeAmortTable(&zATable);
  return zErr;
} /* AmortGeneration */

/*F*
** Function to create the unload file for the entire firm's data required for
** creating amortization transactions for the given date. This function
** also creates a sorted file out of the unloaded file. It first checks if the
** file amort.unl.dddd (dddd = given date) already exists, if it does, unload
** is skipped. Then it checks if amort.srt.dddd exists, if it does sorting is
** also skipped(obviously, if a new unload file is created, no need to check if
** sort file exists or not, sorting is always done).
*F*/
ERRSTRUCT AmortUnloadAndSort(long lValDate) {
  ERRSTRUCT zErr;
  char sFName[30], sUnlStr[500];
  AMORTSTRUCT zTempAmort;
  FILE *fp;

  lpprInitializeErrStruct(&zErr);

  strcpy_s(sFName, MakePricingFileName(lValDate, "", "A"));
  /* Create, if required, and open the file for writing */
  fp = fopen(sFName, "w");
  if (fp == NULL)
    zErr = lpfnPrintError("Error Opening File", 0, 0, "", 999, 0, 0,
                          "AMORT UNLOAD1", FALSE);

  /* Fetch all the records and write them to the file */
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    InitializeAmortStruct(&zTempAmort);
    lpprAmortizeUnload("B", 0, "", "", "", "", 0, &zTempAmort, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      break;

    if (strcmp(zTempAmort.sSecNo, "") == 0)
      strcpy_s(zTempAmort.sSecNo, " ");
    if (strcmp(zTempAmort.sWi, "") == 0)
      strcpy_s(zTempAmort.sWi, " ");
    if (strcmp(zTempAmort.sSecXtend, "") == 0)
      strcpy_s(zTempAmort.sSecXtend, " ");
    if (strcmp(zTempAmort.sSecSymbol, "") == 0)
      strcpy_s(zTempAmort.sSecSymbol, " ");
    if (strcmp(zTempAmort.sAcctType, "") == 0)
      strcpy_s(zTempAmort.sAcctType, " ");
    if (strcmp(zTempAmort.sOrigTransType, "") == 0)
      strcpy_s(zTempAmort.sOrigTransType, " ");
    if (strcmp(zTempAmort.sCurrId, "") == 0)
      strcpy_s(zTempAmort.sCurrId, " ");
    if (strcmp(zTempAmort.sIncCurrId, "") == 0)
      strcpy_s(zTempAmort.sIncCurrId, " ");
    if (strcmp(zTempAmort.sPayType, "") == 0)
      strcpy_s(zTempAmort.sPayType, " ");
    if (strcmp(zTempAmort.sDefeased, "") == 0)
      strcpy_s(zTempAmort.sDefeased, " ");
    if (strcmp(zTempAmort.sAmortFlag, "") == 0)
      strcpy_s(zTempAmort.sAmortFlag, " ");
    if (strcmp(zTempAmort.sAccretFlag, "") == 0)
      strcpy_s(zTempAmort.sAccretFlag, " ");

    /* Get all the fields in a string */
    sprintf_s(
        sUnlStr,
        "%d|%s|%s|%s|%s|%s|%ld|%d|%f|%ld|%ld|%ld|%ld|%f|%f|%f|%f|%f|%f|%f"
        "|%ld|%f|%s|%d|%f|%s|%s|%ld|%f|%s|%s|%d|%ld|%ld|%s|%s\n",
        zTempAmort.iID, zTempAmort.sSecNo, zTempAmort.sWi, zTempAmort.sSecXtend,
        zTempAmort.sSecSymbol, zTempAmort.sAcctType, zTempAmort.lTransNo,
        zTempAmort.iSecID, zTempAmort.fUnits, zTempAmort.lTrdDate,
        zTempAmort.lStlDate, zTempAmort.lEffDate, zTempAmort.lEligDate,
        zTempAmort.fOrigFace, zTempAmort.fOrigYield, zTempAmort.fTotCost,
        zTempAmort.fOrigCost, zTempAmort.fCostEffMatYld, zTempAmort.fCurExrate,
        zTempAmort.fCurIncExrate, zTempAmort.lEffMatDate,
        zTempAmort.fEffMatPrice, zTempAmort.sOrigTransType, zTempAmort.iSecType,
        zTempAmort.fTradUnit, zTempAmort.sCurrId, zTempAmort.sIncCurrId,
        zTempAmort.lMaturityDate, zTempAmort.fRedemptionPrice,
        zTempAmort.sPayType, zTempAmort.sDefeased, zTempAmort.iAccrualSched,
        zTempAmort.lFirstCpnDate, zTempAmort.lLastCpnDate,
        zTempAmort.sAmortFlag, zTempAmort.sAccretFlag);

    /* Write the string to the file */
    if (fputs(sUnlStr, fp) == EOF)
      zErr = lpfnPrintError("Error Writing To The File", 0, 0, "", 999, 0, 0,
                            "AMORT UNLOAD2", FALSE);

  } /* while no error */
  fclose(fp);
  return zErr;
} /* AmortUnloadAndSort */

/*F*
** Function to build a memory table of all the records belonging to an account.
** The function assumes that the file is sorted (by branch_account, sec_no, wi,
** trans_no, etc) and it has records in AMORTSTRUCT format, each field seperated
** by '|' character. Write now, this function works only for a single account,
** it later may be extended to accept a list of accounts.
*F*/
ERRSTRUCT BuildAmortTable(const char *sMode, const char *sFileName, int iID,
                          const char *sSecNo, const char *sWi,
                          const char *sSecXtend, const char *sAcctType,
                          long lTransNo, AMORTTABLE *pzATable) {
  ERRSTRUCT zErr;
  AMORTSTRUCT zTempAmort;
  FILE *fp;
  char sStr1[301], *sStr2;
  char *next_token1 = NULL;
  int i;

  lpprInitializeErrStruct(&zErr);

  InitializeAmortTable(pzATable);

  if (sMode[0] == 'B') {
    fp = fopen(sFileName, "r");
    if (fp == NULL)
      return (lpfnPrintError("Error Opening File", 0, 0, "", 999, 0, 0,
                             "AMORT BUILDTABLE1", FALSE));

    /*
    ** Read records from the file. For the record size, we need to give the size
    ** of the biggest record, because fgets stops as soon as it gets a new line
    ** character(or has read the given number of character, whichever is first),
    ** so use 300 as the size, even though the length of records will be much
    ** lower than that.
    */
    while ((zErr.iSqlError == 0 && zErr.iBusinessError == 0) &&
           (fgets(sStr1, 300, fp) != NULL)) {
      InitializeAmortStruct(&zTempAmort);

      /*
      ** If fetched account is not same as the passed account, there are two
      ** possibility, first is that the last account processed is not null,
      ** which means that we are done(since file is sorted by account), and
      ** second is that last account processed is NULL which means we have not
      ** found records of the the passed account yet, in that case read next
      ** record. If we have found a record of the passed account, then use
      ** strtok to get all the fields out of it into a AMORTSTRUCt variable and
      ** add that to the table.
      */
      sStr2 = strtok_s(sStr1, "|", &next_token1);
      i = atoi(sStr2);
      if (i > iID)
        break;
      else if (i < iID)
        continue;

      zTempAmort.iID = i;

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sSecNo, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sWi, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sSecXtend, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sSecSymbol, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sAcctType, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lTransNo = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.iSecID = atoi(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fUnits = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lTrdDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lStlDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lEffDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lEligDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fOrigFace = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fOrigYield = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fTotCost = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fOrigCost = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fCostEffMatYld = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fCurExrate = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fCurIncExrate = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lEffMatDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fEffMatPrice = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sOrigTransType, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.iSecType = atoi(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fTradUnit = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sCurrId, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sIncCurrId, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lMaturityDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.fRedemptionPrice = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sPayType, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sDefeased, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.iAccrualSched = atoi(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lFirstCpnDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempAmort.lLastCpnDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sAmortFlag, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempAmort.sAccretFlag, sStr2);

      /* Add the record to the given table */
      zErr = AddRecordToAmortTable(pzATable, zTempAmort);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        break;
    } /* while not(eof) */
    fclose(fp);
  } /* if batch mode */
  else {
    while (zErr.iSqlError == 0) {
      lpprAmortizeUnload(sMode, iID, sSecNo, sWi, sSecXtend, sAcctType,
                         lTransNo, &zTempAmort, &zErr);

      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0)
        break;

      zErr = AddRecordToAmortTable(pzATable, zTempAmort);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        break;
    } /* No error */

  } /* Not in batch mode */

  return zErr;
} /* BuildAmortTable */

/*F*
** Function to add a record(of AMORTSTRUCT type) to AMORTTABLE.
*F*/
ERRSTRUCT AddRecordToAmortTable(AMORTTABLE *pzATab, AMORTSTRUCT zAmortStruct) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzATab->iAmortCreated == pzATab->iNumAmort) {
    pzATab->iAmortCreated += NUMAMORT;
    pzATab->pzAmort = (AMORTSTRUCT *)realloc(
        pzATab->pzAmort, pzATab->iAmortCreated * sizeof(AMORTSTRUCT));
    if (pzATab->pzAmort == NULL)
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                             "AMORT ADDAMREC", FALSE));
  }

  pzATab->pzAmort[pzATab->iNumAmort++] = zAmortStruct;

  return zErr;
} /* AddRecordToAmortTable */

ERRSTRUCT AmortCallTranAlloc(AMORTSTRUCT zAmort, double fBaseExrate,
                             double fOldCost, char *sAcctMthd, SECTYPE zSType,
                             long lValDate) {
  ERRSTRUCT zErr;
  TRANS zTR;
  ASSETS zAssets;
  DTRANSDESC zDTr[1];
  long lTrdDate, lPriceDate;
  char sTemp[30];

  lpprInitializeErrStruct(&zErr);
  lpprInitializeTransStruct(&zTR);

  if (fBaseExrate <= 0)
    return (lpfnPrintError("Invalid Base Exrate", zAmort.iID, zAmort.lTransNo,
                           "T", 67, 0, 0, "AMORT CALLTALLOC1", FALSE));

  zTR.iID = zAmort.iID;
  zTR.lTransNo = zAmort.lTransNo;
  strcpy_s(zTR.sSecNo, zAmort.sSecNo);
  strcpy_s(zTR.sWi, zAmort.sWi);
  strcpy_s(zTR.sSecXtend, zAmort.sSecXtend);
  strcpy_s(zTR.sAcctType, zAmort.sAcctType);
  strcpy_s(zTR.sSecSymbol, zAmort.sSecSymbol);
  strcpy_s(zTR.sTranType, "AM");
  zTR.iSecID = zAmort.iSecID;
  if (zAmort.fUnits < 0) {
    zTR.fUnits = zAmort.fUnits * -1;
    strcpy_s(zTR.sDrCr, "DR");
  } else {
    zTR.fUnits = zAmort.fUnits;
    strcpy_s(zTR.sDrCr, "CR");

    // if accretion - change DR_CR flag

    if ((fOldCost - zAmort.fTotCost) < 0)
      strcpy_s(zTR.sDrCr, "DR");
  }
  lpprSelectStarsDate(&lTrdDate, &lPriceDate, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zTR.fOrigFace = zAmort.fOrigFace;
  zTR.fPcplAmt = TruncateDouble(fabs(fOldCost - zAmort.fTotCost), 2);
  zTR.fAmortVal = TruncateDouble(fabs(fOldCost - zAmort.fTotCost), 2);
  zTR.lTrdDate = zTR.lStlDate = zTR.lEffDate = lValDate;
  zTR.lEntryDate = lTrdDate;
  zTR.lPostDate = lTrdDate;
  zTR.lTaxlotNo = zAmort.lTransNo;

  strcpy_s(zTR.sCurrId, zAmort.sCurrId);
  strcpy_s(zTR.sCurrAcctType, zAmort.sAcctType);
  strcpy_s(zTR.sIncCurrId, zAmort.sCurrId);
  strcpy_s(zTR.sIncAcctType, zAmort.sAcctType);
  strcpy_s(zTR.sSecCurrId, zAmort.sCurrId);
  strcpy_s(zTR.sAccrCurrId, zAmort.sIncCurrId);

  zTR.fBaseXrate = zAmort.fCurExrate / fBaseExrate;
  zTR.fIncBaseXrate = zAmort.fCurIncExrate / fBaseExrate;
  zTR.fSecBaseXrate = zAmort.fCurExrate / fBaseExrate;
  zTR.fAccrBaseXrate = zAmort.fCurIncExrate / fBaseExrate;

  zTR.fSysXrate = zAmort.fCurExrate;
  zTR.fIncSysXrate = zAmort.fCurIncExrate;
  strcpy_s(zTR.sAcctMthd, sAcctMthd);
  strcpy_s(zTR.sTransSrce, "P");
  strcpy_s(zTR.sCreatedBy, "PRICING");
  _strdate(sTemp);
  lpfnrstrdate(sTemp, &zTR.lCreateDate);
  _strtime(zTR.sCreateTime);

  strcpy_s(zAssets.sSecNo, zAmort.sSecNo);
  strcpy_s(zAssets.sWhenIssue, zAmort.sWi);
  zAssets.iSecType = zAmort.iSecType;
  zAssets.fTradUnit = zAmort.fTradUnit;
  strcpy_s(zAssets.sCurrId, zAmort.sCurrId);

  // TranAlloc (...TRUE) allows usage of DB Transactions
  if (zAmort.fUnits < 0)
    zErr = lpfnTranAlloc(&zTR, zTTypeAMDr, zSType, zAssets, zDTr, 0, NULL, "C",
                         TRUE); // FALSE);
  else
    zErr = lpfnTranAlloc(&zTR, zTTypeAMCr, zSType, zAssets, zDTr, 0, NULL, "C",
                         TRUE); // FALSE);

  return zErr;
} /* AmortCallTranAlloc */

void InitializeAmortStruct(AMORTSTRUCT *pzAmort) {
  pzAmort->iID = 0;
  pzAmort->sSecNo[0] = pzAmort->sWi[0] = '\0';
  pzAmort->sSecXtend[0] = pzAmort->sSecSymbol[0] = pzAmort->sAcctType[0] = '\0';
  pzAmort->lTransNo = pzAmort->lTrdDate = pzAmort->lStlDate =
      pzAmort->lEffDate = 0;
  pzAmort->lEligDate = pzAmort->iSecID = 0;
  pzAmort->fUnits = pzAmort->fOrigFace = pzAmort->fOrigYield = 0;
  pzAmort->fTotCost = pzAmort->fOrigCost = pzAmort->fCostEffMatYld = 0;
  pzAmort->lEffMatDate = 0;
  pzAmort->fEffMatPrice = 0;
  pzAmort->sOrigTransType[0] = '\0';
  pzAmort->iSecType = 0;
  pzAmort->fTradUnit = 0;
  pzAmort->sCurrId[0] = pzAmort->sIncCurrId[0] = '\0';
  pzAmort->fCurExrate = pzAmort->fCurIncExrate = 1;
  pzAmort->sPayType[0] = '\0';
  pzAmort->iAccrualSched = 0;
  pzAmort->fRedemptionPrice = 0;
  pzAmort->lFirstCpnDate = pzAmort->lLastCpnDate = 0;
  pzAmort->sAmortFlag[0] = pzAmort->sDefeased[0] = '\0';
  pzAmort->sAccretFlag[0] = pzAmort->sDefeased[0] = '\0';
  pzAmort->iAccrualSched = 0;
} /* InitializeAmortStruct */

void InitializeAmortTable(AMORTTABLE *pzATable) {
  if (pzATable->iAmortCreated > 0 && pzATable->pzAmort != NULL)
    free(pzATable->pzAmort);

  pzATable->pzAmort = NULL;
  pzATable->iAmortCreated = pzATable->iNumAmort = 0;
}

BOOL IsThisAMonthEnd(long lDate) {
  short iMDY1[3], iMDY2[3];

  if (lpfnrjulmdy(lDate, iMDY1) < 0 || lpfnrjulmdy(lDate + 1, iMDY2) < 0)
    return FALSE;

  if (iMDY1[0] == iMDY2[0])
    return FALSE;
  else
    return TRUE;
}

void InitializeHoldingsStruct(HOLDINGS *pzHoldings) {
  pzHoldings->iID = 0;
  pzHoldings->sSecNo[0] = pzHoldings->sWi[0] = pzHoldings->sSecXtend[0] = '\0';
  pzHoldings->sAcctType[0] = '\0';
  pzHoldings->lTransNo = 0;
  pzHoldings->iSecID = 0;
  pzHoldings->lAsofDate = 0;
  pzHoldings->sSecSymbol[0] = '\0';
  pzHoldings->fUnits = pzHoldings->fOrigFace = pzHoldings->fTotCost = 0;
  pzHoldings->fUnitCost = pzHoldings->fOrigCost = pzHoldings->fOpenLiability =
      0;
  pzHoldings->fBaseCostXrate = pzHoldings->fSysCostXrate = 0;
  pzHoldings->lTrdDate = pzHoldings->lEffDate = pzHoldings->lEligDate =
      pzHoldings->lStlDate = 0;
  pzHoldings->fOrigYield = 0;
  pzHoldings->lEffMatDate = 0;
  pzHoldings->fEffMatPrice = 0;
  pzHoldings->fCostEffMatYld = 0;
  pzHoldings->lAmortStartDate = 0;
  pzHoldings->sOrigTransType[0] = pzHoldings->sOrigTransSrce[0] = '\0';
  pzHoldings->sLastTransType[0] = '\0';
  pzHoldings->lLastTransNo = 0;
  pzHoldings->sLastTransSrce[0] = '\0';
  pzHoldings->lLastPmtDate = 0;
  pzHoldings->sLastPmtType[0] = '\0';
  pzHoldings->lLastPmtTrNo = 0;
  pzHoldings->lNextPmtDate = 0;
  pzHoldings->fNextPmtAmt = 0;
  pzHoldings->lLastPdnDate = 0;
  pzHoldings->sLtStInd[0] = '\0';
  pzHoldings->fMktVal = pzHoldings->fCurLiability = 0;
  pzHoldings->fMvBaseXrate = pzHoldings->fMvSysXrate = 0;
  pzHoldings->fAccrInt = 0;
  pzHoldings->fAiBaseXrate = pzHoldings->fAiSysXrate = 0;
  pzHoldings->fAnnualIncome = pzHoldings->fAccrualGl = 0;
  pzHoldings->fCurrencyGl = pzHoldings->fSecurityGl = 0;
  pzHoldings->fMktEffMatYld = pzHoldings->fMktCurYld = 0;
  pzHoldings->sSafekInd[0] = '\0';
  pzHoldings->fCollateralUnits = pzHoldings->fHedgeValue = 0;
  pzHoldings->sBenchmarkSecNo[0] = pzHoldings->sPermLtFlag[0] = '\0';
  pzHoldings->sValuationSrce[0] = pzHoldings->sPrimaryType[0] = '\0';
}}
