/*H*
*
* SUB-SYSTEM: Valuation
*
* FILENAME: Value1.c
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
*
* USAGE: AccountValuation(char *sMode, int iID, long lPriceDate, BOOL
bPartialValuation)
*
*
* AUTHOR: Shobhit Barman (Effron Enterprises, Inc.)
*
*
2014-03-13 VI# 54712 Fixed division by zero for Primary Type F and Secondary
Type R -mk 2009-03-05 VI# 41233: in case of stl date beyond month-end, adjusted
accrued interest for bonds to be proportional of units on original PS/FR trans
-mk *H*/

#include "valuedll.h"
#include <math.h>

// Main function for the dll
BOOL APIENTRY DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved) {

  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_PROCESS_DETACH:
    FreeValuation();
    break;

  default:
    break;
  }

  return TRUE;
} // DllMain

extern "C" {
DLLAPI ERRSTRUCT STDCALL WINAPI AccountValuation(char *sMode, int iID,
                                                 long lPriceDate,
                                                 BOOL bPartialValuation) {
  ERRSTRUCT zErr;
  PARTPMAIN zPmain;
  char sMsg[80];
  PRICETABLE zPTable;
  CUSTOMPRICETABLE zCPTable;

  if (!bInit) {
    memset(&zErr, 0, sizeof(ERRSTRUCT));
    zErr.iSqlError = -1;
    return zErr;
  }

  lpprInitializeErrStruct(&zErr);

  zPmain.iID = 0;

  /*
  ** If batch mode, force Partial Valuation(valuing just holdings and holdcash)
  * to be FALSE,
  ** else if user wants they can do partial valuation(right now (as of 3/31/99)
  * only performance
  ** will do partial valuation.
  */
  if (strcmp(sMode, "B") == 0)
    bPartialValuation = FALSE;

  if (lStarsTrdDate == 0) {
    lpprSelectStarsDate(&lStarsTrdDate, &lStarsPricingDate, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  // lpfnTimer(1);
  while (TRUE) {
    if (strcmp(sMode, "B") == 0) // batch processing
    {
      lpprSelectAllPmain(&zPmain, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) // we are done
      {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
      /* Can't Value a portfolio before its purge date */
      if (lPriceDate < zPmain.lPurgeDate)
        continue;

    } // batch processing
    else // single account
    {
      if (zPmain.iID == iID) // already processed the account, we are done
        break;
      else {
        lpprSelectOnePmain(&zPmain, iID, &zErr);
        // lpfnTimer(2);
        if (zErr.iSqlError == SQLNOTFOUND)
          return (lpfnPrintError("Portmain Not Found", iID, 0, "", 1,
                                 zErr.iSqlError, 0, "VALUATION ACCVAL1",
                                 FALSE));
        else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
      }
      /* Can't Value a portfolio before its purge date */
      if (lPriceDate < zPmain.lPurgeDate)
        return (lpfnPrintError(
            "Portfolio Has Been Purged. No Transactions Going Back That Far",
            zPmain.iID, 0, "", 604, 0, 0, "VALUATION", FALSE));

    } // single account

    zPTable.iPriceCreated = 0;
    InitializePriceTable(&zPTable);

    zCPTable.iCPriceCreated = 0;
    InitializeCustomPriceTable(&zCPTable);

    zErr = ValueAnAccount(zPmain, &zPTable, &zCPTable, lPriceDate,
                          bPartialValuation);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      sprintf_s(sMsg, "Valuation On Account %s Failed", zPmain.sUniqueName);
      lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iSqlError, "VALUATION ACCVAL2", FALSE);
      if (strcmp(sMode, "B") == 0)
        lpprInitializeErrStruct(&zErr);
    } else {
      sprintf_s(sMsg, "Valuation On Account %s Successful", zPmain.sUniqueName);
      lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iSqlError, "VALUATION ACCVAL3", TRUE);
    }
    // Close any open queries in starsio.dll
    lpprCloseQueries();
  } // while true

  // lpfnTimer(10);
  // lpprTimerResult("sb.log");
  // InitializePhxrefTable(
  InitializePriceTable(&zPTable);
  InitializeCustomPriceTable(&zCPTable);

  return zErr;
} // ValueAccount

ERRSTRUCT ValueAnAccount(PARTPMAIN zPmain, PRICETABLE *pzPTable,
                         CUSTOMPRICETABLE *pzCPTable, long lPriceDate,
                         BOOL bPartialValuation) {
  char sWi[STR1LEN], sSecno[STR12LEN];
  int iSecId;
  double fCurrencyXrate = 1.0;
  ERRSTRUCT zErr;
  PORTBAL zPbal;
  PHXREFTABLE zPXref;
  PBALXTRA zPbxtra;
  PRICEINFO zTempPriceFromHist;
  BOOL bUseTrans;

  lpprInitializeErrStruct(&zErr);

  zPXref.iPhxrefCreated = 0;
  InitializePhxrefTable(&zPXref);
  /*	 Commenting out portbal
  InitializeUpdatableFieldsInPortbal(&zPbal);
  InitializePortbalXtra(&zPbxtra);
  */
  InitializePriceTable(pzPTable);
  InitializeCustomPriceTable(pzCPTable);
  sSecno[0] = sWi[0] = '\0';
  iSecId = 0;

  // Get custom price for all securities in the account for the valuation date
  if (zSystemSettings.bUseCustomSecurityPrice) {
    zErr = GetAllCustomPrice(pzCPTable, zPmain.iID, lPriceDate);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  // select the sec_no and wi for the base currency of the portfolio
  zErr = GetCurrencySecNo(sSecno, sWi, zPmain.sBaseCurrId);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  if (strcmp(zSystemSettings.zSyssetng.sSystemcurrency, zPmain.sBaseCurrId) !=
      0) {
    // get the exchange rate for the base currency of the portfolio
    zErr = GetSecurityPrice(sSecno, sWi, lPriceDate, zSTTable, zPmain.iID,
                            *pzCPTable, pzPTable, &zTempPriceFromHist);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
    fCurrencyXrate = zTempPriceFromHist.fExrate;
  }

  // If complete valuation, get the portbal record and initialize fields which
  // are recalculated
  if (!bPartialValuation) {
    /*	Commenting out portbal
                    lpprSelectPortbal(&zPbal, zPmain.iID, &zErr);
                            //lpfnTimer(3);
                    if (zErr.iSqlError == SQLNOTFOUND)
                            return(lpfnPrintError("Portbal Not Found",
       zPmain.iID, 0, "", 1, zErr.iSqlError, 0, "VALUATION ACCVAL2", FALSE));
                    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                            return zErr;
    */
    zErr = ValueHedgexref(zPmain.iID, lPriceDate, fCurrencyXrate, zSTTable,
                          *pzCPTable, pzPTable, &zPXref);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      InitializePhxrefTable(&zPXref);
      return zErr;
    }
  } // not partial valuation

  // Holdings is always valued
  // 06/12/03 vay - New DB transaction handling architecture
  bUseTrans = (lpfnGetTransCount() == 0);
  if (bUseTrans)
    lpfnStartTransaction();

  __try {
    zErr = ValueHoldings(zPmain, lPriceDate, fCurrencyXrate, zSTTable, zPXref,
                         *pzCPTable, &zPbal, &zPbxtra, pzPTable,
                         bPartialValuation);
    // lpfnTimer(5);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      if (bUseTrans)
        lpfnRollbackTransaction();

      InitializePhxrefTable(&zPXref);
      return zErr;
    }

    // Holdcash is also valued all the time
    zErr =
        ValueHoldcash(zPmain, lPriceDate, fCurrencyXrate, zSTTable, *pzCPTable,
                      &zPbal, &zPbxtra, pzPTable, bPartialValuation);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      if (bUseTrans)
        lpfnRollbackTransaction();

      InitializePhxrefTable(&zPXref);
      return zErr;
    }

    // If doing a full valuation then value payrec, portbal and update valuation
    // date in portmain
    if (!bPartialValuation) {
      zErr = ValuePayrec(zPmain.iID, lPriceDate, fCurrencyXrate, zSTTable,
                         *pzCPTable, pzPTable);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        if (bUseTrans)
          lpfnRollbackTransaction();

        InitializePhxrefTable(&zPXref);
        return zErr;
      }
      /* Coomenting out portbal
      zErr = ValuePortbal(zPmain.iID, lPriceDate, fCurrencyXrate, zSTTable,
      zPbal, zPbxtra, pzPTable); if (zErr.iSqlError != 0 || zErr.iBusinessError
      != 0){ InitializePhxrefTable(&zPXref); return zErr;
      }
*/
      lpprUpdateValDate(zPmain.iID, lPriceDate, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        if (bUseTrans)
          lpfnRollbackTransaction();

        InitializePhxrefTable(&zPXref);
        return zErr;
      }
    }

  } // try
  __except (lpfnAbortTransaction(
      bUseTrans)) //( bUseTrans ? (lpfnRollbackTransaction() ? 0 : 0) : 0)
  {
    // exception filter (above) ensures that db trans is rolled back (if open)
    // and then exception is being passed to the next level (calling proc)
    // w/o any further processing here
  }

  if (bUseTrans)
    lpfnCommitTransaction();

  InitializePhxrefTable(&zPXref);
  return zErr;
} // ValueAnAccount

/*F*
** Function to value hedgexref records for a portfolio on the given date.
*F*/
ERRSTRUCT ValueHedgexref(int iID, long lPriceDate, double fBaseXrate,
                         SECTYPETABLE zlSTTable, CUSTOMPRICETABLE zCPTable,
                         PRICETABLE *pzPTable, PHXREFTABLE *pzPXTable) {
  ERRSTRUCT zErr;
  HEDGEXREF zTempHxref;
  PARTHXREF zAddHxref;
  PRICEINFO zTempPrice;

  lpprInitializeErrStruct(&zErr);
  // free the memory, don't need any record for another account
  InitializePhxrefTable(pzPXTable);

  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    lpprSelectAllHxrefForAnAccount(
        iID, &zTempHxref, &zErr); // get next hedgexref record for this account
    if (zErr.iSqlError == SQLNOTFOUND) // we are done
    {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return zErr;

    zTempHxref.fHedgeValBase = zTempHxref.fHedgeValNative =
        zTempHxref.fHedgeValSystem = 0;
    strcpy_s(zTempHxref.sValuationSrce, "NP"); // no price

    // Get price for hedging security, continue even if security not found
    zErr = GetSecurityPrice(zTempHxref.sSecNo2, zTempHxref.sWi2, lPriceDate,
                            zlSTTable, iID, zCPTable, pzPTable, &zTempPrice);
    if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
      return zErr;

    if (zTempPrice.bIsPriceValid) {
      zTempHxref.fHedgeValNative = zTempHxref.fHedgeUnits *
                                   zTempPrice.fClosePrice *
                                   zTempPrice.fTradUnit;
      strcpy_s(zTempHxref.sValuationSrce, zTempPrice.sPriceSource);
    }

    if (zTempPrice.bIsExrateValid) {
      if (!IsValueZero(fBaseXrate, 12))
        zTempHxref.fHedgeValBase =
            zTempHxref.fHedgeValNative / (zTempPrice.fExrate / fBaseXrate);
      zTempHxref.fHedgeValSystem =
          zTempHxref.fHedgeValNative / zTempPrice.fExrate;
    } // no error in getting price

    // update the hedgexref record
    zTempHxref.lAsofDate = lPriceDate;
    lpprUpdateHedgexref(zTempHxref, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    // Add the record to memory table for later use(by ValueHoldings function)
    zAddHxref.iID = zTempHxref.iID;
    strcpy_s(zAddHxref.sSecNo, zTempHxref.sSecNo2);
    strcpy_s(zAddHxref.sWi, zTempHxref.sWi2);
    strcpy_s(zAddHxref.sSecXtend, zTempHxref.sSecXtend2);
    strcpy_s(zAddHxref.sAcctType, zTempHxref.sAcctType2);
    zAddHxref.lTransNo = zTempHxref.lTransNo2;
    zAddHxref.fHedgeValNative = zTempHxref.fHedgeValNative;
    zAddHxref.fHedgeValSystem = zTempHxref.fHedgeValSystem;
    zErr = AddHedgexrefToTable(pzPXTable, zAddHxref);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    zAddHxref.fHedgeValNative = zAddHxref.fHedgeValSystem = 0;
    // Get price for hedged security, continue even if the price not found
    zErr = GetSecurityPrice(zTempHxref.sSecNo, zTempHxref.sWi, lPriceDate,
                            zlSTTable, iID, zCPTable, pzPTable, &zTempPrice);
    if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
      return zErr;

    if (zTempPrice.bIsPriceValid) {
      zAddHxref.fHedgeValNative = zTempHxref.fHedgeUnits2 *
                                  zTempPrice.fClosePrice * zTempPrice.fTradUnit;
      if (!IsValueZero(zTempPrice.fExrate, 12))
        zAddHxref.fHedgeValSystem =
            zAddHxref.fHedgeValNative / zTempPrice.fExrate;
    } // no error in getting price

    // Add the record to memory table for later use(by ValueHoldings function)
    strcpy_s(zAddHxref.sSecNo, zTempHxref.sSecNo);
    strcpy_s(zAddHxref.sWi, zTempHxref.sWi);
    strcpy_s(zAddHxref.sSecXtend, zTempHxref.sSecXtend);
    strcpy_s(zAddHxref.sAcctType, zTempHxref.sAcctType);
    zAddHxref.lTransNo = zTempHxref.lTransNo;
    zErr = AddHedgexrefToTable(pzPXTable, zAddHxref);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  } // while no error
  return zErr;
} // ValueHedgexref

/*F*
** Function to value holdings records for a portfolio on the given date.
*F*/
ERRSTRUCT ValueHoldings(PARTPMAIN zPmain, long lPriceDate, double fBaseXrate,
                        SECTYPETABLE zlocSTTable, PHXREFTABLE zPXTable,
                        CUSTOMPRICETABLE zCPTable, PORTBAL *pzPbal,
                        PBALXTRA *pzPbxtra, PRICETABLE *pzPTable,
                        BOOL bPartialValuation) {
  ERRSTRUCT zErr;
  HOLDINGS zTempHold;
  PRICEINFO zTempPrice;
  TRANS zTrans, zTempTrans;
  PARTFINC zTempBond;
  ASSETS zAssets;

  // GK 2001-3-6
  ACCDIVTABLE zADTable;
  ASSETTABLE zATable;

  int i, j;
  long lSecImpact, lCallDate, lStlDate, lIndexID;
  double fSTGL, fMTGL, fLTGL, fTotGL, fPrice, fYield, fCallPrice,
      fLastDayInterest, fTotalUnits;
  char sMsg[100], sLastSecNo[13], sLastWi[2], sLastDivSecNo[13], sLastDivWi[2],
      sLastDivSecXtend[3], sLastDivAcctType[2];
  BOOL bUseCallDate, bCallYTC = FALSE;

  lpprInitializeErrStruct(&zErr);
  fSTGL = fMTGL = fLTGL = fTotGL = fPrice = fYield = fCallPrice =
      fLastDayInterest = fTotalUnits = 0;
  bUseCallDate = bCallYTC = FALSE;
  lSecImpact = lCallDate = lStlDate = lIndexID = 0;
  memset(&zTempBond, 0, sizeof(zTempBond));
  memset(&zTempHold, 0, sizeof(zTempHold));
  memset(&zTempPrice, 0, sizeof(zTempPrice));
  memset(&zTrans, 0, sizeof(zTrans));
  memset(&zTempTrans, 0, sizeof(zTempTrans));
  // GK 2001-3-6
  zADTable.iAccdivCreated = 0;
  InitializeAccdivTable(&zADTable);

  zErr = FillAccdivTable(zPmain.iID, zPmain.bIncome, zPmain.bIncByLot,
                         lPriceDate, &zATable, &zADTable);
  if (zErr.iSqlError || zErr.iBusinessError) // we are done
    return zErr;

  zATable.iAssetCreated = 0;
  InitializeAssetTable(&zATable);
  // GK 2001-3-6 down to here

  sLastSecNo[0] = sLastWi[0] = '\0';
  sLastDivSecNo[0] = sLastDivWi[0] = sLastDivSecXtend[0] = sLastDivAcctType[0] =
      '\0';
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    // lpfnTimer(11);
    lpprInitHoldings(&zTempHold);
    lpprSelectAllHoldingsForAnAccount(
        zPmain.iID, &zTempHold,
        &zErr); // get next holdings record for this account
    // lpfnTimer(12);
    if (zErr.iSqlError == SQLNOTFOUND) // we are done
    {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return zErr;

    // Change from 3 to 5 since can have 5 decimal places for units
    // VI# 58465 - SB 2/3/2016 commented out the following test - now we should
    // value taxlot even if it has zero units
    /*		if (IsValueZero(zTempHold.fUnits, 5))
                            continue;*/

    zTempHold.lAsofDate = lPriceDate;
    strcpy_s(zTempHold.sValuationSrce, "NP"); // Not priced

    // if security not same as the last lot, get price and other required
    // information
    if (strcmp(zTempHold.sSecNo, sLastSecNo) != 0 ||
        strcmp(zTempHold.sWi, sLastWi) != 0) {
      // Get price for the security, return if any error(including not finding
      // the security) occurs
      // lpfnTimer(13);
      zErr = GetSecurityPrice(zTempHold.sSecNo, zTempHold.sWi, lPriceDate,
                              zlocSTTable, zPmain.iID, zCPTable, pzPTable,
                              &zTempPrice);
      // lpfnTimer(14);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      // if the security is a bond or money market, get fixed income record
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND ||
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_MMARKET) {
        // lpfnTimer(15);
        lpprSelectFixedinc(zTempHold.sSecNo, zTempHold.sWi, &zTempBond, &zErr);
        // lpfnTimer(16);
        if (zErr.iSqlError == SQLNOTFOUND) {
          sprintf_s(sMsg,
                    "Security - **%s**, **%s** Not Found In Fixedinc Table",
                    zTempHold.sSecNo, zTempHold.sWi);
          lpfnPrintError(sMsg, zTempHold.iID, zTempHold.lTransNo, "T", 128, 0,
                         0, "VALUATION VALHOLD2", TRUE);
          zErr.iSqlError = 0;
          zTempBond.bRecordFound = FALSE;
        } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
        else
          zTempBond.bRecordFound = TRUE;
      }

      // SB 1/22/08 - if the security is a future contract, get assets record to
      // find out its primary currency
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_FUTURE &&
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
              STYPE_CONTRACTS) {
        lpprSelectAsset(&zAssets, zTempHold.sSecNo, zTempHold.sWi, -1, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          if (zErr.iSqlError == SQLNOTFOUND) {
            sprintf_s(sMsg, "Security: %s, Wi: %s Not Found In Asset Table",
                      zTempHold.sSecNo, zTempHold.sWi);
            lpfnPrintError(sMsg, zTempHold.iID, zTempHold.lTransNo, "T", 128, 0,
                           0, "VALUATION VALHOLD2A", TRUE);
          }

          return zErr;
        } // error selecting assets record
      } // if future contract

      // 9/24/02 SB - If TIPS and variablerate (inflation index ratio) is
      // invalid, calculate inflation index ratio
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND &&
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
              STYPE_TBILLS &&
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPayType[0] ==
              PAYTYPE_INFPROTCTD) {
        if (zTempPrice.fVariablerate <= 0) {
          if (zTempBond.bRecordFound)
            lIndexID = zTempBond.lInflationIndexID;
          else
            lIndexID = 0;

          zTempPrice.fVariablerate = lpfnInflationIndexRatio(
              zTempHold.sSecNo, zTempBond.lIssueDate, lPriceDate, lIndexID);
          if (zTempPrice.fVariablerate <= 0) {
            sprintf_s(sMsg,
                      "Invalid Inflation Ratio %f, for Security %s, Date: %d",
                      zTempPrice.fVariablerate, zTempHold.sSecNo, lPriceDate);
            return (lpfnPrintError(sMsg, zTempHold.iID, zTempHold.lTransNo, "L",
                                   999, 0, 0, "VALUATION VALHOLD2B", FALSE));
          }
        } // if couldn't calculate ratio
      } // if TIPS

      strcpy_s(sLastSecNo, zTempHold.sSecNo);
      strcpy_s(sLastWi, zTempHold.sWi);
    } // if security changed

    // call function to fugure out whether the gain/loss is short term or long
    // term
    /*zErr = IsGainSTMTOrLT() */

    // Calculate Current Liability and Market Value
    if (zTempPrice.bIsPriceValid) {
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
          PTYPE_FUTURE) {
        if (IsValueZero(zTempPrice.fClosePrice, 5))
          zTempHold.fCurLiability = zTempHold.fOpenLiability;
        else
          zTempHold.fCurLiability =
              zTempHold.fUnits * zTempPrice.fClosePrice * zTempPrice.fTradUnit;

        if ((zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
             STYPE_CONTRACTS) &&
            !IsValueZero(zTempHold.fCurLiability, 5) &&
            !IsValueZero(zTempPrice.fClosePrice, 5))
          zTempHold.fMktVal =
              (zTempHold.fCurLiability - zTempHold.fOpenLiability) /
              zTempPrice.fClosePrice;
        else
          zTempHold.fMktVal =
              zTempHold.fCurLiability - zTempHold.fOpenLiability;

      } // if future
      else {
        zTempHold.fCurLiability = 0;
        zTempHold.fMktVal =
            zTempHold.fUnits * zTempPrice.fClosePrice * zTempPrice.fTradUnit;

        // 9/24/02 SB - If TIPS, multiply market value by inflation index ratio
        if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                PTYPE_BOND &&
            zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
                STYPE_TBILLS &&
            zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPayType[0] ==
                PAYTYPE_INFPROTCTD)

          zTempHold.fMktVal *= zTempPrice.fVariablerate;
      } // if not future
      // strcpy_s(zTempHold.sValuationSrce, "NE"); // No exchange rate
      strcpy_s(zTempHold.sValuationSrce, zTempPrice.sPriceSource);
    } // if price is valid
    else {
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
          PTYPE_FUTURE) {
        zTempHold.fCurLiability = zTempHold.fOpenLiability;
        zTempHold.fMktVal = zTempHold.fCurLiability - zTempHold.fOpenLiability;
      } // if future
      else {
        zTempHold.fCurLiability = 0;
        if (IsValueZero(zTempHold.fTotCost, 3) && zTempHold.fOpenLiability > 0)
          zTempHold.fMktVal =
              zTempHold.fCurLiability - zTempHold.fOpenLiability;
        else
          zTempHold.fMktVal = zTempHold.fTotCost;
      } // if not future
    } // if price is not valid

    // Market Value base and system exchange rates
    if (zTempPrice.bIsExrateValid) {
      if (!IsValueZero(fBaseXrate, 12)) {
        if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                PTYPE_FUTURE &&
            zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
                STYPE_CONTRACTS &&
            zTempPrice.bIsPriceValid &&
            !IsValueZero(zTempPrice.fClosePrice, 5)) {
          zTempHold.fMvBaseXrate = (1 / zTempPrice.fClosePrice);

          // SB 1/22/08 - Up until now exchange rate was always being divided by
          // portfolio's base exchange rate, however that's wrong if the
          // portfolio's base currency is same as the hedge currency of the
          // contract, in this case no need to divide it further by the exchange
          // rate between portfolio and system currency
          if (strcmp(zPmain.sBaseCurrId, zAssets.sCurrId) != 0)
            zTempHold.fMvBaseXrate = zTempHold.fMvBaseXrate / fBaseXrate;
        } else
          zTempHold.fMvBaseXrate = zTempPrice.fExrate / fBaseXrate;

        zTempHold.fMvSysXrate = zTempPrice.fExrate;
      } /*!IsValueZero*/
    }

    // Calculate accrued interest
    // The statement below was last "else if" case of accrual calculation,
    // however, now we need to try to calculate accruals for fixed income mutual
    // fund from accdiv if the first attemp to calculate it based on accrual
    // factor results in zero accrual. So, the different cases for accrual
    // calculation can't be part of one single if ..else if statement, they will
    // have to be broken out in separate if statements.
    if (lPriceDate >= zTempHold.lStlDate)
      zTempHold.fAccrInt = 0;

    // SB 12/12/2003 Added a new sectype, discounted securities with accruals
    if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
            PTYPE_BOND &&
        zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPayType[0] == 'D' &&
        zlocSTTable.zSType[zTempPrice.iSTypeIdx].iAccrualSched == 100)
      zTempHold.fAccrInt = StraightLineDiscountedAccrual(
          zTempHold.fTotCost, zTempHold.fUnits, zTempHold.lStlDate,
          zTempBond.lMaturityDate, lPriceDate);
    else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                 PTYPE_BOND &&
             zTempPrice.bRecordFound && strcmp(zTempBond.sFlatCode, "Y") != 0) {
      if (lPriceDate <= zTempHold.lStlDate) {
        // first, try to find linked transacton - vay
        memset(&zTempTrans, 0, sizeof(TRANS));
        lpprSelectTrans(zTempHold.iID, zTempHold.lTransNo, &zTempTrans, &zErr);
        if ((zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND) ||
            zErr.iBusinessError != 0) {
          sprintf_s(sMsg, "Error Selecting Trans for Security %s ** %s",
                    zTempHold.sSecNo, zTempHold.sWi);
          lpfnPrintError(sMsg, zTempHold.iID, zTempHold.lTransNo, "T", 0,
                         zErr.iSqlError, zErr.iBusinessError,
                         "VALUATION VALHOLD3", TRUE);
          return zErr;
        }

        if (zTempHold.fUnits < zTempTrans.fUnits &&
            !IsValueZero(zTempTrans.fUnits, 5))
          zTempHold.fAccrInt =
              (zTempTrans.fAccrInt * zTempHold.fUnits) / zTempTrans.fUnits;
        else
          zTempHold.fAccrInt = zTempTrans.fAccrInt;
      }

      if ((lPriceDate > zTempHold.lStlDate) ||
          (zErr.iSqlError ==
           SQLNOTFOUND)) // ... or no trans found - 04/06/01 vay

      {
        if ((lPriceDate <=
             zTempBond.lMaturityDate) || // if security is not matured
            (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
             STYPE_MFUND)) // or it is a Mutual Fund
        {
          zTempHold.fAccrInt =
              zTempHold.fUnits * zTempPrice.fAccrInt * zTempPrice.fTradUnit;
          // if accrual is zero and the system is set to calculate accrauls till
          // the next business day (or next day), see if it(next business day)
          // is the pay date for the security, if it is then the security should
          // get full accraul amount.
          if (IsValueZero(zTempPrice.fAccrInt, 7)) {
            if (zTempPrice.sPriceSource[0] != 'M' ||
                !lpfnItsManualPriceSecurity(zTempHold.sSecNo, zTempHold.sWi,
                                            lPriceDate, &zErr)) {
              if (_stricmp(zSystemSettings.zSyssetng.sDateforaccrualflag,
                           "B") == 0 ||
                  (_stricmp(zSystemSettings.zSyssetng.sDateforaccrualflag,
                            "D") == 0 &&
                   lPriceDate < lJan312001))
                zTempPrice.fAccrInt = GetAccruedInterest(
                    lPriceDate, zTempHold, zCPTable, pzPTable, TRUE);
              else if (_stricmp(zSystemSettings.zSyssetng.sDateforaccrualflag,
                                "N") == 0)
                zTempPrice.fAccrInt = GetAccruedInterest(
                    lPriceDate, zTempHold, zCPTable, pzPTable, FALSE);
            }

            zTempHold.fAccrInt =
                zTempHold.fUnits * zTempPrice.fAccrInt * zTempPrice.fTradUnit;
          } // zTempHold.fAccrInt == 0
          else if (_stricmp(zSystemSettings.zSyssetng.sDateforaccrualflag,
                            "B") == 0 ||
                   (_stricmp(zSystemSettings.zSyssetng.sDateforaccrualflag,
                             "D") == 0 &&
                    lPriceDate < lJan312001)) {
            // if the accr_int is not manual, calc the Accr_int on previous
            // business day, if previous business day accrual is more than
            // today, then add that to today's accrual
            if (zTempPrice.sPriceSource[0] != 'M' ||
                !lpfnItsManualPriceSecurity(zTempHold.sSecNo, zTempHold.sWi,
                                            lPriceDate, &zErr)) {
              fLastDayInterest =
                  GetAccruedInterestForLastBusinessDay(lPriceDate, zTempHold);
              fLastDayInterest *= zTempHold.fUnits * zTempPrice.fTradUnit;
              if (fabs(fLastDayInterest) - fabs(zTempHold.fAccrInt) > 0.0000001)
                zTempHold.fAccrInt += fLastDayInterest;
            } // if not manual accrual
          } // if Dateforaccrualflag - B (calculating acrrual upto the next
            // business day)

          // (11/13/99) Offit funds (sectype = 14) are defined as Mutual Funds
          // and they have special logic Federated have some securities defined
          // as sectype 14 but they don't calculate accruals on them, so this is
          // still OK
          if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
              STYPE_MFUND)
            zTempHold.fAccrInt *= 0.1;

          // 9/24/02 SB - If TIPS, multiply market value by inflation index
          // ratio
          if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                  PTYPE_BOND &&
              zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
                  STYPE_TBILLS &&
              zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPayType[0] ==
                  PAYTYPE_INFPROTCTD)

            zTempHold.fAccrInt *= zTempPrice.fVariablerate;
        } // if price date <= Maturity date
        else
          zTempHold.fAccrInt = 0;
      } // price date > settlement date or no transaction found
    } // If bond

    /*
    ** PerformanceType in Syssetng table can have following values
    **    M - Calculate Performance with Market Value Only
    **		I - Calculate Performance with Market Value + Accrued Interest
    **		D - Calculate Performance with Market Value + Accrued Dividends
    **    A - Calculate Performance with Market Value + Accrued Interest +
    * Accrued Dividends
    ** else - default to I
    ** If this value is D or A then need to get accrued dividends (from ACCDIV
    * and TRANS), else not
    **
    ** SB 11/11/10 - if we are doing valuation for performance
    * (bPartialValuation = TRUE) then no need to worry about
    ** accrual as performance gets pending equity accrual from transaction and
    * not from holdings
    **
    ** SB 10/29/2013 VI# 54033 - There are cases where some fixed income mutual
    * fund have pending income like
    ** equities. On the reverse side there are also cases where fixed income
    * mutual funds have accrual factor
    ** defined in histfinc. To take care of both of these cases, continue
    * calculating accrual on holdings (in the previous steps)
    ** based on accrual factor, however, if the accrual factor is zero then do
    * an additional step by looking
    ** for pending income in accdiv.
    */
    if ((zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
             PTYPE_EQUITY ||
         (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND &&
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
              STYPE_MFUND &&
          IsValueZero(zTempHold.fAccrInt, 2))) &&
        !bPartialValuation &&
        (strcmp(zSystemSettings.zSyssetng.sPerformanceType, "D") == 0 ||
         strcmp(zSystemSettings.zSyssetng.sPerformanceType, "A") == 0)) {
      zTempHold.fAccrInt = 0;
      for (j = 0; j < zADTable.iNumAccdiv; j++) {
        /* JTG 1/29/2007 put in change to not use LT or ST transaction type for
         * accruals */
        if (strcmp(zADTable.pzAccdiv[j].sSecNo, zTempHold.sSecNo) == 0 &&
            strcmp(zADTable.pzAccdiv[j].sWi, zTempHold.sWi) == 0 &&
            strcmp(zADTable.pzAccdiv[j].sSecXtend, zTempHold.sSecXtend) == 0 &&
            strcmp(zADTable.pzAccdiv[j].sAcctType, zTempHold.sAcctType) == 0 &&
            zADTable.pzAccdiv[j].lTrdDate <= lPriceDate &&
            zADTable.pzAccdiv[j].lStlDate > lPriceDate &&
            zTempHold.lEligDate < zADTable.pzAccdiv[j].lTrdDate &&
            strcmp(zADTable.pzAccdiv[j].sTranType, "LT") != 0 &&
            strcmp(zADTable.pzAccdiv[j].sTranType, "ST") != 0) {
          if (!IsValueZero(zADTable.pzAccdiv[j].fUnits, 5)) {
            if (zADTable.pzAccdiv[j].lTransNo == zTempHold.lTransNo) {
              if (zTempHold.fUnits > zADTable.pzAccdiv[j].fUnits)
                zTempHold.fAccrInt += zADTable.pzAccdiv[j].fIncomeAmt;
              else
                zTempHold.fAccrInt +=
                    (zTempHold.fUnits * zADTable.pzAccdiv[j].fIncomeAmt /
                     zADTable.pzAccdiv[j].fUnits);
            }
            // SB 5/29/01 - some accounts by mistake had incorrect incbylot
            // causing transno on accdiv (actually when accdiv becomes trans) to
            // be zero. If that's the condition, then divide the accrual based
            // on number of units
            else if (zADTable.pzAccdiv[j].lTransNo == 0 &&
                     zTempHold.fUnits <= zADTable.pzAccdiv[j].fUnits) {
              double fHoldUnits = 0.0;
              if (zADTable.pzAccdiv[j].fUnits -
                      zADTable.pzAccdiv[j].fUnitsAccountedFor >=
                  zTempHold.fUnits)
                fHoldUnits = zTempHold.fUnits;
              else
                fHoldUnits = zADTable.pzAccdiv[j].fUnits -
                             zADTable.pzAccdiv[j].fUnitsAccountedFor;
              if (fHoldUnits < 0)
                fHoldUnits = 0;
              zADTable.pzAccdiv[j].fUnitsAccountedFor += fHoldUnits;
              zTempHold.fAccrInt +=
                  (fHoldUnits * zADTable.pzAccdiv[j].fIncomeAmt /
                   zADTable.pzAccdiv[j].fUnits);
            }
          } // if units in accdiv != 0
          // SB 11/11/10 - Now for many clients, system is automatically
          // generating pending accrual but those pending
          //  accruals are not posted as RD. RD is being posted from feed (and
          //  revisions are done manually). When system is not generating RD
          //  (bIncome = FALSE), units on the record will be zero and taxlot
          //  number will not be filled. In cases like these, we need to
          //  calculate correct units (based on holdings & trans) and divide
          //  accrual amount proportionately to all the taxlots
          else {
            if (strcmp(zTempHold.sSecNo, sLastDivSecNo) != 0 ||
                strcmp(zTempHold.sWi, sLastDivWi) != 0 ||
                strcmp(zTempHold.sSecXtend, sLastDivSecXtend) != 0 ||
                strcmp(zTempHold.sAcctType, sLastDivAcctType) != 0) {
              // SB 11/11/10 - Get total units (held + sold) eligible for this
              // dividend. To figure out eligibility, we need record date and
              // pay date of the dividend, however record date is not available
              // either on trans or accdiv record, so best that can be done is
              // ex_date (trd_date) and pay date (stl date). This is not perfect
              // but the best that can be done at this time
              zErr = GetTotalUnitsEligibleForDividend(
                  zPmain.iID, zTempHold.sSecNo, zTempHold.sWi,
                  zTempHold.sSecXtend, zTempHold.sAcctType, lPriceDate,
                  zADTable.pzAccdiv[j].lTrdDate, zADTable.pzAccdiv[j].lStlDate,
                  &fTotalUnits);
              if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                return zErr;
              strcpy_s(sLastDivSecNo, zTempHold.sSecNo);
              strcpy_s(sLastDivWi, zTempHold.sWi);
              strcpy_s(sLastDivSecXtend, zTempHold.sSecXtend);
              strcpy_s(sLastDivAcctType, zTempHold.sAcctType);
            } // if haven't yet retrieve units held for this security

            // It shouldn't happen that held units come back as zero or less
            // than units in current taxlot, but if it does happen then assign
            // the whole accrual to first lot of matching security and don't
            // assign any accrual to subsequent lots, if any.
            if (IsValueZero(fTotalUnits, 5) || fTotalUnits < zTempHold.fUnits) {
              if (strcmp(zTempHold.sSecNo, sLastSecNo) != 0 ||
                  strcmp(zTempHold.sWi, sLastWi) !=
                      0) // first lot of this security
                zTempHold.fAccrInt += zADTable.pzAccdiv[j].fIncomeAmt;
            } else // total held units are greater than zero and less than or
                   // equal to that held in current taxlot
              zTempHold.fAccrInt +=
                  (zTempHold.fUnits * zADTable.pzAccdiv[j].fIncomeAmt /
                   fTotalUnits);
          } // units on accdiv = 0
        } // if security matches and is eligible for dividend
      } // end of while loop
    } // if equity or fixed income mutal fund

    // Accr int base and system echange rates
    if (zTempPrice.bIsExrateValid) {
      if (!IsValueZero(fBaseXrate, 12))
        zTempHold.fAiBaseXrate = zTempPrice.fIncExrate / fBaseXrate;
      zTempHold.fAiSysXrate = zTempPrice.fIncExrate;
    }

    // Do the following calculations only if doing full valuation
    if (!bPartialValuation) {
      // Calculate Original Yield, if required
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND &&
          IsValueZero(zTempHold.fOrigYield, 3) &&
          !IsValueZero(zTempHold.fUnits, 5)) {
        fPrice =
            zTempHold.fOrigCost / (zTempHold.fUnits * zTempPrice.fTradUnit);
        lStlDate = zTempHold.lStlDate;
        zTempHold.fOrigYield = -999; // NAVALUE;
        zTempHold.fOrigYield =
            lpfnCalculateYield(fPrice, zTempHold.sSecNo, zTempHold.sWi,
                               const_cast<char *>("C"), lStlDate, 2);
        // if function fails to calculate the yield
        if (zTempHold.fOrigYield < 0)
          zTempHold.fOrigYield = 0.0;
      } // OrigYield

      // Calculate yield at cost of a bond
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
          PTYPE_BOND) {
        bCallYTC = FALSE;

        if (IsValueZero(zTempHold.fCostEffMatYld, 3) &&
            !IsValueZero(zTempHold.fUnits, 5)) {
          bCallYTC = TRUE;

          lpprSelectNextCallDatePrice(zTempHold.sSecNo, zTempHold.sWi,
                                      zTempHold.lStlDate, &fCallPrice,
                                      &lCallDate, &zErr);
          if (zErr.iBusinessError != 0 ||
              (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND))
            return zErr;
          else if (zErr.iSqlError !=
                   SQLNOTFOUND) // if there is a call date, that's effective
                                // maturity date
            bUseCallDate = TRUE;
          else
            bUseCallDate = FALSE;

          // If the lot was created using FR and the cost has been adjusted,
          // use effective date else use original settlement date
          lStlDate = GetStlDate(zTempHold);

          // SB 7/18/2014. VI# 55500
          // If the taxlot was created by FR and was already amortized at the
          // time of FR, GetStlDate function returns effective date, in other
          // cases it returns settlement date. If date returned is the
          // settlement date then original unit cost should be used for yield
          // calculation, else (if free received lot was already amortized) get
          // the original transaction and use amortized unit cost for yield
          // calculation.ade the sme change in amortization unit where some
          // times recalculation takes place.
          if (lStlDate == zTempHold.lStlDate)
            fPrice =
                zTempHold.fOrigCost / (zTempHold.fUnits * zTempPrice.fTradUnit);
          else {
            lpprSelectTrans(zTempHold.iID, zTempHold.lTransNo, &zTempTrans,
                            &zErr);
            if (zErr.iSqlError == 0 && zErr.iBusinessError == 0 &&
                strcmp(zTempTrans.sTranType, "FR") == 0 &&
                !IsValueZero(zTempTrans.fTotCost, 3) &&
                !IsValueZero(zTempTrans.fUnits, 3))
              fPrice = zTempTrans.fTotCost /
                       (zTempTrans.fUnits * zTempPrice.fTradUnit);
            else
              fPrice = zTempHold.fTotCost /
                       (zTempHold.fUnits * zTempPrice.fTradUnit);
          }
        } else if (zTempHold.lEffMatDate <= lPriceDate) // just pass a call date
        {
          bCallYTC = TRUE;

          lpprSelectNextCallDatePrice(zTempHold.sSecNo, zTempHold.sWi,
                                      lPriceDate, &fCallPrice, &lCallDate,
                                      &zErr);
          if (zErr.iBusinessError != 0 ||
              (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND))
            return zErr;
          else if (zErr.iSqlError != SQLNOTFOUND) // a future call date found
            bUseCallDate = TRUE;
          else
            bUseCallDate = FALSE;

          lStlDate = lPriceDate;
          if (!IsValueZero(zTempHold.fUnits, 5))
            fPrice =
                zTempHold.fTotCost / (zTempHold.fUnits * zTempPrice.fTradUnit);
          else
            fPrice = 0;
        } // if effmatdate <= pricedate

        if (bCallYTC == TRUE && !IsValueZero(fPrice, 2)) {
          fYield = lpfnCalculateYield(fPrice, zTempHold.sSecNo, zTempHold.sWi,
                                      const_cast<char *>("C"), lStlDate, 2);

          if (fYield > 0) {
            zTempHold.fCostEffMatYld = fYield;
          } // successfully calculated yield

          // even if can't calculate yield, update params which have been used
          // for calculation
          if (bUseCallDate) {
            zTempHold.lEffMatDate = lCallDate;
            zTempHold.fEffMatPrice = fCallPrice;
          } else {
            zTempHold.lEffMatDate = zTempBond.lMaturityDate;
            zTempHold.fEffMatPrice = zTempBond.fRedemptionPrice;
          }

        } // if YTC needs to be calculated
      } // if bond
      else
        zTempHold.fCostEffMatYld = 0;

      // Annual income
      zTempHold.fAnnualIncome = 0;
      // if the security is a cash, option or future security or if the lot
      // is a tech-short or tech-long, no annual income
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] !=
              PTYPE_CASH &&
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] !=
              PTYPE_OPTION &&
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] !=
              PTYPE_FUTURE &&
          strcmp(zTempHold.sSecXtend, "TS") != 0 &&
          strcmp(zTempHold.sSecXtend, "TL") != 0) {
        if ((zlocSTTable.zSType[zTempPrice.iSTypeIdx].iIntcalc == 3 ||
             zlocSTTable.zSType[zTempPrice.iSTypeIdx].iIntcalc == 4 ||
             (zTempBond.bRecordFound && zTempBond.sZeroCoupon[0] == 'Y')) &&
            zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                PTYPE_BOND &&
            zTempPrice.bIsPriceValid) {
          if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
                  STYPE_GOVTBONDNOTES &&
              !IsValueZero(zTempHold.fUnits, 5))
            zTempHold.fAnnualIncome =
                (100.0 - ((zTempHold.fMktVal / zTempHold.fUnits) * 100.0)) *
                zTempHold.fUnits;
          else
            zTempHold.fAnnualIncome =
                zTempPrice.fCurYtm * zTempHold.fUnits / 100.0;
        } // if yield = C
        else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sYield[0] == 'C') {
          if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                  PTYPE_EQUITY &&
              zTempPrice.bIsPriceValid)
            zTempHold.fAnnualIncome =
                zTempPrice.fEps * zTempHold.fUnits / 100.0;
          else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                       PTYPE_BOND &&
                   zTempPrice.bIsPriceValid)
            zTempHold.fAnnualIncome =
                zTempPrice.fCurYtm * zTempHold.fUnits / 100.0;
          else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                       PTYPE_MMARKET &&
                   zTempPrice.bIsPriceValid)
            zTempHold.fAnnualIncome =
                zTempPrice.fCurYld * zTempHold.fUnits / 100.0;
        } // if yield = C
        else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sYield[0] == 'M') {
          if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                  PTYPE_EQUITY &&
              zTempPrice.bIsPriceValid)
            zTempHold.fAnnualIncome =
                zTempPrice.fEps * zTempHold.fMktVal / 100.0;
          else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                       PTYPE_BOND &&
                   zTempPrice.bIsPriceValid)
            zTempHold.fAnnualIncome =
                zTempPrice.fCurYtm * zTempHold.fMktVal / 100.0;
        } // if yield = M
        else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sYield[0] == 'D' &&
                 zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                     PTYPE_BOND) {
          if (zTempBond.bRecordFound && zTempBond.lMaturityDate != 0 &&
              zTempPrice.bIsPriceValid && zTempBond.lMaturityDate != lPriceDate)
            zTempHold.fAnnualIncome = ((zTempHold.fUnits - zTempHold.fMktVal) /
                                       (zTempBond.lMaturityDate - lPriceDate)) *
                                      360;
        } else if (zTempPrice.bRecordFound) {
          zTempHold.fAnnualIncome =
              zTempPrice.fAnnDivCpn * zTempHold.fUnits * zTempPrice.fTradUnit;

          // Right now (8/11/99) only Offit funds (sectype = 14) are defined as
          // Mutual Funds and they have special logic Now (11/13/99) even
          // federated have some securities defined as sectype 14 but they don't
          // calculate est ann income, so this is still OK
          if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
              STYPE_MFUND)
            zTempHold.fAnnualIncome *= 0.1;
        }
      } // calc ann inc if not cash, option, future security or a tech-short or
        // tech-long lot

      strcpy_s(zTempHold.sPrimaryType,
               zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType);
      // currency and security gain/loss
      CreateTransFromHoldings(zTempHold, &zTrans, lPriceDate, &lSecImpact);
      zTrans.fOptPrem = 0;
      zErr.iBusinessError = lpfnCalcGainLoss(
          zTrans, const_cast<char *>("C"), lSecImpact,
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType,
          zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType,
          zPmain.sBaseCurrId, &zTempHold.fCurrencyGl, &zTempHold.fSecurityGl,
          &fSTGL, &fMTGL, &fLTGL, &fTotGL);
      if (zErr.iBusinessError != 0)
        return zErr;

      // Accrual gain loss
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND &&
          zTempBond.bRecordFound && strcmp(zTempBond.sPayType, "D") == 0) {

      } else
        zTempHold.fAccrualGl = 0;

      // Market effective maturity yield
      if (zTempPrice.fCurYtm < 0)
        zTempPrice.fCurYtm;

      // SB 9/1/2013, VI# 53725
      // For calculating yield, treat fixed income mutual fund as equity not
      // like other bonds - it should have dividend yield rather than yield to
      // maturity/worst.

      // SB 2/10/2014 VI# 54503
      // Previous change (VI 53725) caused issues for Federated. They want to
      // calculate it themselves and override yield for fixed income mutual
      // fund, rather than using dividend yield. Now change the logic to still
      // use dividend yield as long as it is not zero, but if it zero then use
      // YTM.
      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND &&
          zTempPrice.bIsPriceValid) {
        if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType[0] ==
                STYPE_MFUND &&
            !IsValueZero(zTempPrice.fClosePrice, 5) &&
            !IsValueZero(zTempPrice.fAnnDivCpn, 7))
          zTempHold.fMktEffMatYld =
              (zTempPrice.fAnnDivCpn / zTempPrice.fClosePrice) * 10.0;
        else
          zTempHold.fMktEffMatYld = zTempPrice.fCurYtm;
      } else if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
                     PTYPE_EQUITY &&
                 zTempPrice.bIsPriceValid &&
                 !IsValueZero(zTempPrice.fClosePrice, 5))
        zTempHold.fMktEffMatYld =
            (zTempPrice.fAnnDivCpn / zTempPrice.fClosePrice) * 100.0;
      else
        zTempHold.fMktEffMatYld = 0;

      // market current yield
      if (zTempPrice.fCurYld < 0)
        zTempPrice.fCurYld = 0;

      if (zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] ==
              PTYPE_BOND &&
          zTempPrice.bRecordFound)
        zTempHold.fMktCurYld = zTempPrice.fCurYld;
      else if (!IsValueZero(zTempHold.fMktVal, 2))
        zTempHold.fMktCurYld =
            (zTempHold.fAnnualIncome / zTempHold.fMktVal) * 100;
      else
        zTempHold.fMktCurYld = 0;

      // hedge value
      zTempHold.fHedgeValue = 0;
      for (i = 0; i < zPXTable.iNumPhxref; i++) {
        if (zPXTable.pzPhxref[i].iID == zTempHold.iID &&
            strcmp(zPXTable.pzPhxref[i].sSecNo, zTempHold.sSecNo) == 0 &&
            strcmp(zPXTable.pzPhxref[i].sWi, zTempHold.sWi) == 0 &&
            strcmp(zPXTable.pzPhxref[i].sSecXtend, zTempHold.sSecXtend) == 0 &&
            strcmp(zPXTable.pzPhxref[i].sAcctType, zTempHold.sAcctType) == 0 &&
            zPXTable.pzPhxref[i].lTransNo == zTempHold.lTransNo)
          zTempHold.fHedgeValue += zPXTable.pzPhxref[i].fHedgeValNative;
      }

      // Recalculate Unit Cost
      if (!IsValueZero(zTempHold.fUnits, 5) &&
          !IsValueZero(zTempPrice.fTradUnit, 3))
        zTempHold.fUnitCost =
            zTempHold.fTotCost / (zTempHold.fUnits * zTempPrice.fTradUnit);
      else
        lpfnPrintError("HOLDINGS UNIT COST DIV BY ZERO", zTempHold.iID,
                       zTempHold.lTransNo, "T", 999, 0, 0,
                       "VALUATION CALCUNITCOST", TRUE);

      // Set benchmark security
      strcpy_s(zTempHold.sBenchmarkSecNo, zTempPrice.sBenchmarkSecNo);
      // update extra portbal buckets using this holdings record if doing a full
      // valuation
      /*	Commenting out portbal
                      if (!bPartialValuation)
                                      UpdatePBFromHoldings(zTempHold,
         zlocSTTable.zSType[zTempPrice.iSTypeIdx], zPmain.bCAvail,
         zPmain.bFAvail, pzPbal, pzPbxtra);
      */
    } // if doing full valuation

    // update the holdings lot
    // lpfnTimer(21);
    lpprUpdateHoldings(zTempHold, &zErr);
    // lpfnTimer(22);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } // while no error in fetching Holdings

  InitializeAccdivTable(&zADTable); // Release these
  InitializeAssetTable(&zATable);   // Release these

  return zErr;
} // ValueHoldings

/*F*
** Function to value holdcash records for a portfolio on the given date.
*F*/
ERRSTRUCT ValueHoldcash(PARTPMAIN zPmain, long lPriceDate, double fBaseXrate,
                        SECTYPETABLE zlocSTTable, CUSTOMPRICETABLE zCPTable,
                        PORTBAL *pzPbal, PBALXTRA *pzPbxtra,
                        PRICETABLE *pzPTable, BOOL bPartialValuation) {
  ERRSTRUCT zErr;
  HOLDCASH zTempHcash;
  PRICEINFO zTempPrice;
  long lSecImpact;
  TRANS zTrans;
  double fCurrGl, fSecGL, fSTGL, fMTGL, fLTGL, fTotGL;
  char sLastSecNo[13], sLastWi[2];

  lpprInitializeErrStruct(&zErr);
  memset(&zTempPrice, 0, sizeof(zTempPrice));
  sLastSecNo[0] = sLastWi[0] = '\0';
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    // lpfnTimer(23);
    lpprInitHoldcash(&zTempHcash);
    lpprSelectAllHoldcashForAnAccount(
        zPmain.iID, &zTempHcash,
        &zErr); // get next payrec record for this account
    // lpfnTimer(24);
    if (zErr.iSqlError == SQLNOTFOUND) // we are done
    {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return zErr;

    zTempHcash.lAsofDate = lPriceDate;
    if (strcmp(zTempHcash.sSecNo, sLastSecNo) != 0 ||
        strcmp(zTempHcash.sWi, sLastWi) != 0) {
      // Get price for the security, return if any error(including not finding
      // the security) occurs
      // lpfnTimer(25);
      zErr = GetSecurityPrice(zTempHcash.sSecNo, zTempHcash.sWi, lPriceDate,
                              zlocSTTable, zPmain.iID, zCPTable, pzPTable,
                              &zTempPrice);
      // lpfnTimer(26);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      strcpy_s(sLastSecNo, zTempHcash.sSecNo);
      strcpy_s(sLastWi, zTempHcash.sWi);
    } // security changed

    if (zTempPrice.bIsPriceValid)
      zTempHcash.fMktVal =
          zTempHcash.fUnits * zTempPrice.fClosePrice * zTempPrice.fTradUnit;
    else
      zTempHcash.fMktVal = zTempHcash.fTotCost;

    if (zTempPrice.bIsExrateValid) {
      if (!IsValueZero(fBaseXrate, 12))
        zTempHcash.fMvBaseXrate = zTempPrice.fExrate / fBaseXrate;
      zTempHcash.fMvSysXrate = zTempPrice.fExrate;
    }

    CreateTransFromHoldcash(zTempHcash, &zTrans, lPriceDate, &lSecImpact);
    zErr.iBusinessError = lpfnCalcGainLoss(
        zTrans, const_cast<char *>("C"), lSecImpact,
        zlocSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType,
        zlocSTTable.zSType[zTempPrice.iSTypeIdx].sSecondaryType,
        zPmain.sBaseCurrId, &fCurrGl, &fSecGL, &fSTGL, &fMTGL, &fLTGL, &fTotGL);
    if (zErr.iBusinessError != 0)
      return zErr;

    if (!(IsValueZero(zTempHcash.fTotCost, 3)) &&
        !(IsValueZero(zTempHcash.fUnits, 3)) &&
        !(IsValueZero(zTempPrice.fTradUnit, 3)))
      zTempHcash.fUnitCost =
          zTempHcash.fTotCost / zTempHcash.fUnits / zTempPrice.fTradUnit;
    else
      lpfnPrintError("HOLDCASH UNIT COST DIV BY ZERO", zTempHcash.iID, 0, "", 0,
                     0, 0, "VALUATION", FALSE);

    // update holdcash record
    lpprUpdateHoldcash(zTempHcash, &zErr);
    // lpfnTimer(27);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    // update extra portbal buckets using this holdcash record if doing a full
    // valuation
    /** Commenting out portbal
                    if (!bPartialValuation)
                            UpdatePBFromHoldcash(zTempHcash, zPmain.bCAvail,
    pzPbal, pzPbxtra);
    **/
  } // while no error in fetching holdcash

  return zErr;
} // ValueHoldcash

/*F*
** Function to value payrec records for a portfolio on the given date.
*F*/
ERRSTRUCT ValuePayrec(int iID, long lPriceDate, double fBaseXrate,
                      SECTYPETABLE zlocSTTable, CUSTOMPRICETABLE zCPTable,
                      PRICETABLE *pzPTable) {
  ERRSTRUCT zErr;
  PAYREC zTempPrec;
  PRICEINFO zTempPrice;
  char sLastSecNo[13], sLastWi[2];

  lpprInitializeErrStruct(&zErr);
  memset(&zTempPrice, 0, sizeof(zTempPrice));
  sLastSecNo[0] = sLastWi[0] = '\0';
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    lpprSelectAllPayrecForAnAccount(
        iID, &zTempPrec, &zErr); // get next payrec record for this account
    if (zErr.iSqlError == SQLNOTFOUND) // we are done
    {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return zErr;

    zTempPrec.lAsofDate = lPriceDate;
    strcpy_s(zTempPrec.sValuationSrce, "NP"); // No price

    if (strcmp(zTempPrec.sSecNo, sLastSecNo) != 0 ||
        strcmp(zTempPrec.sWi, sLastWi) != 0) {
      // Get price for the security, continue even if security not found
      zErr =
          GetSecurityPrice(zTempPrec.sSecNo, zTempPrec.sWi, lPriceDate,
                           zlocSTTable, iID, zCPTable, pzPTable, &zTempPrice);
      if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
        return zErr;
    } // if different security

    if (zTempPrice.bIsPriceValid) {
      zTempPrec.fCurVal =
          zTempPrec.fUnits * zTempPrice.fClosePrice * zTempPrice.fTradUnit;
      //			strcpy_s(zTempPrec.sValuationSrce, "NE"); // no
      // exchange rate
      strcpy_s(zTempPrec.sValuationSrce,
               zTempPrice.sPriceSource); // no exchange rate
    }

    if (zTempPrice.bIsExrateValid) {
      if (!IsValueZero(fBaseXrate, 12))
        zTempPrec.fMvBaseXrate = zTempPrice.fExrate / fBaseXrate;

      zTempPrec.fMvSysXrate = zTempPrice.fExrate;
      //	  if (zTempPrice.bIsPriceValid)
      //				strcpy_s(zTempPrec.sValuationSrce,
      //"OK"); // Everything ok
    }

    lpprUpdatePayrec(zTempPrec, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } // while no error in fetching payrec

  return zErr;
} // ValuePayrec

/*F*
** Function to value portbal records for a portfolio on the given date.
*F*/
/*ERRSTRUCT ValuePortbal(int iID, long lPriceDate, double fBaseXrate,
SECTYPETABLE zSTTable, PORTBAL zPbal, PBALXTRA zPbxtra, PRICETABLE *pzPTable)
{
        ERRSTRUCT	zErr;
        long			lStarsTrdDate, lStarsPricingDate;
        DIVHIST		zDivhist;
  ACCDIV		zAccdiv;
        TRANS			zTrans;

        lpprInitializeErrStruct(&zErr);

        // Don't care about portbal
        return zErr;

        zPbal.lAsofDate = lPriceDate;

        lpprSelectStarsDate(&lStarsTrdDate, &lStarsPricingDate, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                return zErr;

        // Calculate accrued interest, in-house dividend and out-of-house
dividend if (lPriceDate == lStarsPricingDate)
        {
                //lpfnTimer(28);
                while (TRUE)
                {
                        lpprSelectAllAccdivForAnAccount(iID, &zAccdiv, &zErr);
                        if (zErr.iSqlError == SQLNOTFOUND)
                        {
                                zErr.iSqlError = 0;
                                break;
                        }
                        else if (zErr.iSqlError != 0)
                                return zErr;
/ * Commenting out Portbal
                        zErr = UpdatePBFromAccdiv(zAccdiv, lPriceDate,
fBaseXrate, zSTTable, pzPTable, &zPbal); if (zErr.iSqlError != 0 ||
zErr.iBusinessError != 0) return zErr;
* /
                } // while true
                //lpfnTimer(29);
        } // if current valuation
        else
        {
                while (TRUE)
                {
                        lpprSelectAllDivhistForAnAccount(iID, &zDivhist, &zErr);
                        if (zErr.iSqlError == SQLNOTFOUND)
                        {
                                zErr.iSqlError = 0;
                                break;
                        }
                        else if (zErr.iSqlError != 0)
                                return zErr;

                        if (zDivhist.lExDate > lPriceDate || zDivhist.lPayDate
<= lPriceDate) continue; else if (strcmp(zDivhist.sTranLocation, "A") == 0) //
in accdiv
                        {
                                // select accdiv record
                                lpprSelectAccdiv(&zAccdiv, zDivhist.iID,
zDivhist.lDivintNo, zDivhist.lTransNo, &zErr); if (zErr.iSqlError ==
SQLNOTFOUND) return (lpfnPrintError("Accdiv For Divhist Not Found",
zDivhist.iID, zDivhist.lTransNo, "T", 0, 100, 0, "VALUATION VALDIVHIST1",
FALSE)); else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) return zErr;

/ *Commenting out portbal
                                // update portbal using this accdiv
                                zErr = UpdatePBFromAccdiv(zAccdiv, lPriceDate,
fBaseXrate, zSTTable, pzPTable, &zPbal); if (zErr.iSqlError != 0 ||
zErr.iBusinessError != 0) return zErr;
* /
                        } // in accdiv
                        else if (strcmp(zDivhist.sTranLocation, "T") == 0) // in
transaction
                        {
                                // select trans record
                                lpprSelectTrans(zDivhist.iID,
zDivhist.lDivTransNo, &zTrans, &zErr); if (zErr.iSqlError != 0 ||
zErr.iBusinessError != 0)
                                {
                                        lpfnPrintError("Trans For Divhist Not
Found", zDivhist.iID, zDivhist.lDivTransNo, "T", 999, 100, 0, "VALUATION
VALDIVHIST2", TRUE); return zErr;
                                }
/ ** Commenting out portbal
                                zErr = UpdatePBFromTrans(zTrans, lPriceDate,
fBaseXrate,zSTTable, pzPTable, &zPbal); if (zErr.iSqlError != 0 ||
zErr.iBusinessError != 0) return zErr;
** /
                        } // in trans
                        else
                                lpfnPrintError("Invalid Trans Location In
Divhist Record", zDivhist.iID, zDivhist.lTransNo, "T", 999, 0, 0, "VALUATION
VALPBAL1", TRUE); } // while TRUE } // if not current valuation

        // When Issue
        zPbal.fWiCalc = zPbxtra.fWhenIssued;

        // Mark to market
        zPbal.fMarkToMarket = zPbxtra.fShortCash + zPbxtra.fShortHoldings;

        // Cash Available
        zPbal.fCashAvail = zPbal.fWiCalc + zPbal.fMarkToMarket +
zPbxtra.fLongCash;

        // Funds Available
        zPbal.fFundsAvail = zPbal.fCashAvail + zPbxtra.fTBillMinusHedge +
zPbxtra.fMMarket;

        // Current Equity
        zPbal.fCurEquity =	zPbal.fAccrInc + zPbal.fIhDiv + zPbal.fOhDiv +
                                                                                        zPbal.fCashVal + zPbal.fMoneymktLval + zPbal.fMoneymktSval +
                                                                                        zPbal.fBondLval + zPbal.fBondSval + zPbal.fEquityLval +
                                                                                        zPbal.fEquitySval + zPbal.fOptionLval + zPbal.fOptionSval +
                                                                                        zPbal.fMfundEqtyLval + zPbal.fMfundEqtySval +
                                                                                        zPbal.fMfundBondLval + zPbal.fMfundBondSval +
                                                                                        zPbal.fPreferredLval + zPbal.fPreferredSval +
                                                                                        zPbal.fCvtBondLval + zPbal.fCvtBondSval +
                                                                                        zPbal.fTbillLval + zPbal.fTbillSval +
                                                                                        zPbal.fStInstrLval + zPbal.fStInstrSval;

        // Annual Income
/ *	if (zPbal.fCashAvail < 0.0)
                zPbal.fAnnualInc = zPbal.fCashAvail * fCrRate;
        else
                zPbal.fAnnualInc = zPbal.fCashAvail * fDrRate;

        // Yield
        if (zPbal.fCurEquity == 0)
                zPbal.fYield = 0;
        else
                zPbal.fYield = (zPbal.fAnnualInc / zPbal.fCurEquity ) * 100;* /

/ * Commenting out portbal
        lpprUpdatePortbal(zPbal, &zErr);
* /
        //lpfnTimer(30);

        return zErr;
} // ValuePortbal*/

/*F*
** This function updates some fields on a portbal record (and some temporary
** fieds on a seperate record using which some other fields on the
** portbal records are updated later) using the given holdings lot.
*F*/
/*void UpdatePBFromHoldings(HOLDINGS zHold, SECTYPE zSType, BOOL bCAvail,
                                                                                                        BOOL bFAvail, PORTBAL *pzPbal, PBALXTRA *pzPbxtra)
{
        double		fTempMV, fTempAI;

        // if tech-short or tech-long, return without doing anything
        if (strcmp(zHold.sSecXtend, "TS") == 0 || strcmp(zHold.sSecXtend, "TL")
== 0) return;

        // Calc the market value and accr int in base currency of the portfolio
        if (!IsValueZero(zHold.fMvBaseXrate,12))
                fTempMV = zHold.fMktVal / zHold.fMvBaseXrate;
        if (!IsValueZero(zHold.fAiBaseXrate,12))
                fTempAI = zHold.fAccrInt / zHold.fAiBaseXrate;

        // Update non supervised value if it is an unsupervised lot
        if (strcmp(zHold.sSecXtend, "UP") == 0)
                pzPbal->fNonSupVal += fTempMV;

        // Update accrued income
        pzPbal->fAccrInc += fTempAI;

        // If it is a tbill and portfolio is set to include Tbill in funds
available,
        // store the value(MV - Hedge value) in portbal extension record.
        if (bFAvail == TRUE && zSType.sPrimaryType[0] == PTYPE_BOND &&
zSType.sSecondaryType[0] == STYPE_TBILLS) pzPbxtra->fTBillMinusHedge += (fTempMV
- zHold.fHedgeValue);

        // if it is a money market lot and cash avaliable is TRUE or account
type
        // is not in-house income, store the value in portbal extension record
        if (zSType.sMktValInd[0] == 'M' && (bCAvail || zHold.sAcctType[0] !=
ACCTYPE_IHINCOME)) pzPbxtra->fMMarket += fTempMV;

        // update appropriate field depending on market value indicator and
whether it is short or long if (zHold.fUnits >= 0)
        {
                switch (zSType.sMktValInd[0])
                {
                        case 'M':	pzPbal->fMoneymktLval += fTempMV;
                                                                break;

                        case 'B':	pzPbal->fBondLval += fTempMV;
                                                                break;

                        case 'E': pzPbal->fEquityLval += fTempMV;
                                                                break;

                        case 'O':	pzPbal->fOptionLval += fTempMV;
                                                                break;

                        case 'Q': pzPbal->fMfundEqtyLval += fTempMV;
                                                                break;

                        case 'F': pzPbal->fMfundBondLval += fTempMV;
                                                                break;

                        case 'P': pzPbal->fPreferredLval += fTempMV;
                                                                break;

                        case 'V': pzPbal->fCvtBondLval += fTempMV;
                                                                break;

                        case 'T':	pzPbal->fTbillLval += fTempMV;
                                                                break;

                        case 'S':	pzPbal->fStInstrLval += fTempMV;
                                                                break;

                        default	:	break;
                }
        } // if units >= 0
        else
        {
                switch (zSType.sMktValInd[0])
                {
                        case 'M':	pzPbal->fMoneymktSval += fTempMV;
                                                                break;

                        case 'B':	pzPbal->fBondSval += fTempMV;
                                                                break;

                        case 'E': pzPbal->fEquitySval += fTempMV;
                                                                break;

                        case 'O':	pzPbal->fOptionSval += fTempMV;
                                                                break;

                        case 'Q': pzPbal->fMfundEqtySval += fTempMV;
                                                                break;

                        case 'F': pzPbal->fMfundBondSval += fTempMV;
                                                                break;

                        case 'P': pzPbal->fPreferredSval += fTempMV;
                                                                break;

                        case 'V': pzPbal->fCvtBondSval += fTempMV;
                                                                break;

                        case 'T':	pzPbal->fTbillSval += fTempMV;
                                                                break;

                        case 'S':	pzPbal->fStInstrSval += fTempMV;
                                                                break;

                        default	:	break;
                }
        } // if units < 0

        // liability value
        if (zSType.sMktValInd[0] == 'L' && !IsValueZero(zHold.fMvBaseXrate,12))
                pzPbal->fLiabilityVal += zHold.fCurLiability /
zHold.fMvBaseXrate;

        // when issue
        if (zHold.sWi[0] == 'Y' && !IsValueZero(zHold.fBaseCostXrate,12))
                pzPbxtra->fWhenIssued += zHold.fTotCost / zHold.fBaseCostXrate;
        else if (zHold.sAcctType[0] == ACCTYPE_IHSHORT || zHold.sAcctType[0] ==
ACCTYPE_OHSHORT) pzPbxtra->fShortHoldings += fTempMV;

        pzPbal->fAnnualInc += zHold.fAnnualIncome;


  return;
} // UpdatePBFromHoldings
*/

/*F*
** This function updates some fields on a portbal record (and some temporary
** fieds on a seperate record using which some other fields on the
** portbal records are updated later) using the given holdcash lot.
*F*/
/*void UpdatePBFromHoldcash(HOLDCASH zHcash, BOOL bCAvail, PORTBAL *pzPbal,
                                                                                                        PBALXTRA *pzPbxtra)
{
        double	fTempMV;

        // if a tech-short or tech-long lot or if account type is non-purpose
loan, nothing to do if (strcmp(zHcash.sSecXtend, "TS") == 0 ||
strcmp(zHcash.sSecXtend, "TL") == 0 || zHcash.sAcctType[0] ==
ACCTYPE_NONPURPOSELOAN) return; if (!IsValueZero(zHcash.fMvBaseXrate,12))
                fTempMV = zHcash.fMktVal / zHcash.fMvBaseXrate;

        if (strcmp(zHcash.sSecXtend, "UP") == 0)
                pzPbal->fNonSupVal += fTempMV;

        / *
        ** If account type is not in (out-of-house income, in-house short,
        ** out-of-house short and  when and if issued) and (cash available is
        ** TRUE or account type is not in-house income), store the value in long
        ** cash field on portbal extension record
        * /
        if (zHcash.sAcctType[0] != ACCTYPE_OHINCOME && zHcash.sAcctType[0] !=
ACCTYPE_IHSHORT && zHcash.sAcctType[0] != ACCTYPE_OHSHORT && zHcash.sAcctType[0]
!= ACCTYPE_WHENISSUE && (bCAvail || zHcash.sAcctType[0] != ACCTYPE_IHINCOME))
                pzPbxtra->fLongCash += fTempMV;

        // update cash value
        pzPbal->fCashVal += fTempMV;

        // if account type is '5'(in-house short) or 'B'(out-of-house short) and
        // it is not a when issued security, update short cash on portbal
extension record if ((zHcash.sAcctType[0] == ACCTYPE_IHSHORT ||
zHcash.sAcctType[0] == ACCTYPE_OHSHORT) && strcmp(zHcash.sWi, "N") == 0)
                pzPbxtra->fShortCash += fTempMV;

        return;
} // UpdatePBFromHoldcash
*/

/*F*
** This function updates some fields(accrued interest/in-house dividend/
** out-of-house dividend) on a portbal record using the given accdiv lot.
*F*/
/*ERRSTRUCT UpdatePBFromAccdiv(ACCDIV zAccdiv, long lPriceDate, double
fBaseXrate, SECTYPETABLE zSTTable, PRICETABLE *pzPTable, PORTBAL *pzPbal)
{
        ERRSTRUCT	zErr;
        PRICEINFO	zTempPrice;
        double		fTempMV;

        lpprInitializeErrStruct(&zErr);

        // if an unsupervised security, nothing to do
        if (strcmp(zAccdiv.sSecXtend, "UP") == 0)
                return zErr;

        // Get price for the security, return if any error(including not finding
the security) occurs zErr = GetSecurityPrice(zAccdiv.sSecNo, zAccdiv.sWi,
lPriceDate, zSTTable, pzPTable, &zTempPrice); if (zErr.iSqlError != 0) return
zErr;

        // market value
        if(!IsValueZero(fBaseXrate,12))
        {
                if (zTempPrice.bIsExrateValid)
                        fTempMV = zAccdiv.fPcplAmt / (zTempPrice.fIncExrate /
fBaseXrate); else fTempMV = zAccdiv.fPcplAmt / (zAccdiv.fIncBaseXrate /
fBaseXrate);
        }
  if (strcmp(zAccdiv.sDrCr, "DR") == 0)
                fTempMV *= -1;

        if (zSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] == PTYPE_BOND)
                pzPbal->fAccrInc += fTempMV;
        else
        {
                if (zAccdiv.sAcctType[0] == ACCTYPE_IHINCOME)
                        pzPbal->fIhDiv += fTempMV;
                else
                        pzPbal->fOhDiv += fTempMV;
        }

        return zErr;
} // UpdatePBFromAccdiv
*/
/*F*
** This function updates some fields(accrued interest/in-house dividend/
** out-of-house dividend) on a portbal record using the given transaction.
*F*/
/*ERRSTRUCT UpdatePBFromTrans(TRANS zTrans, long lPriceDate, double fBaseXrate,
                                                                                                                SECTYPETABLE zSTTable, PRICETABLE *pzPTable, PORTBAL *pzPbal)
{
        ERRSTRUCT zErr;
        PRICEINFO	zTempPrice;
        double		fTempAccr;

        lpprInitializeErrStruct(&zErr);

        // if an unsupervised security, nothing to do
        if (strcmp(zTrans.sSecXtend, "UP") == 0)
                return zErr;

        // Get price for the security, return if any error(including not finding
the security) occurs zErr = GetSecurityPrice(zTrans.sSecNo, zTrans.sWi,
lPriceDate, zSTTable, pzPTable, &zTempPrice); if (zErr.iSqlError != 0) return
zErr;

        // market value
        if (!IsValueZero(fBaseXrate,12))
        {
                if (zTempPrice.bIsExrateValid)
                        fTempAccr = zTrans.fPcplAmt / (zTempPrice.fIncExrate /
fBaseXrate); else fTempAccr = zTrans.fPcplAmt / (zTrans.fIncBaseXrate /
fBaseXrate);
        }

  if (strcmp(zTrans.sDrCr, "DR") == 0)
                fTempAccr *= -1;

        if (zSTTable.zSType[zTempPrice.iSTypeIdx].sPrimaryType[0] == PTYPE_BOND)
                pzPbal->fAccrInc += fTempAccr;
        else
        {
                if (zTrans.sAcctType[0] == ACCTYPE_IHINCOME)
                        pzPbal->fIhDiv += fTempAccr;
                else
                        pzPbal->fOhDiv += fTempAccr;
        }

        return zErr;
} // UpdatePBFromTrans
*/

double GetAccruedInterest(const long lPriceDate, HOLDINGS zTempHold,
                          CUSTOMPRICETABLE zCPTable, PRICETABLE *pzPTable,
                          BOOL bLastBusinessDay) {
  long lTempDate, lLastBusinessDay = 0, lNextBusinessDay = 0;
  double dInterest;
  BOOL bInterestPaid;
  PRICEINFO zTempPriceFromHist;
  ERRSTRUCT zErr;

  if (bLastBusinessDay) {
    // Read last business day from current pricing date, if any error reading
    // it, return
    if (lpfnLastBusinessDay(lPriceDate, const_cast<char *>("USA"),
                            const_cast<char *>("M"), &lLastBusinessDay) != 0)
      return 0;

    /*
    ** if the current pricing day is a holiday then the previous record is not
    * what we are
    ** interested in, we are interested in the previous business day(e.g. if a
    * bond pays
    ** on 5/1/2000, records for 4/28/2000(Friday), 4/29/2000(Saturday) and
    * 4/30/2000(Sunday)
    ** will have accr_int as 0(because all of them are calculating accrual on
    * 5/1). If pricing
    ** on 4/28, last business day will be 4/27 and that's the record we want,
    * but on 4/30, last
    ** business record will be 4/28 which is not what we want, if get the
    * previous business day
    ** from 4/28 then we will get the required record on 4/27.
    ** SB 10/30/00 - There is an exception to the above rule, if the security
    * pays after last
    ** business day and on or prior to pricing date then we should go back only
    * to prior business
    ** day instead of prior to prior business day, e.g. if bond pays on
    * 9/30/00(Saturday), and
    ** the price date is 9/30/2000, if we go to previous(9/29/2000) to
    * previous(9/28/2000) date and
    ** then that accrual will be almost the full accrual and if that accrual is
    * taken then we
    ** will double count (accrual as well as cash we already got on 9/30/2000),
    * so in this situation,
    ** we need to go back only one day(9/29/2000)
    */
    if (lpfnIsItAMarketHoliday(lPriceDate, const_cast<char *>("USA")) == 1) {
      // check if security paid interest between last business day + 1 and
      // pricing date
      bInterestPaid = FALSE;
      for (lTempDate = lLastBusinessDay + 1; lTempDate <= lPriceDate;
           lTempDate++) {
        lpfnSelectDivint(zTempHold.sSecNo, zTempHold.sWi, lTempDate, &zErr);
        if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
          bInterestPaid = TRUE;
      }

      // if interest was not paid then get previous business date
      if (!bInterestPaid &&
          lpfnLastBusinessDay(lLastBusinessDay, const_cast<char *>("USA"),
                              const_cast<char *>("M"), &lLastBusinessDay) != 0)
        return 0;
    }

    if (lpfnNextBusinessDay(lPriceDate, const_cast<char *>("USA"),
                            const_cast<char *>("M"), &lNextBusinessDay) != 0)
      return 0;
  } else
    lLastBusinessDay = lPriceDate - 1;

  // See if the security had any accrual on the last business day (or last day -
  // depending on the flag)
  zErr = GetSecurityPrice(zTempHold.sSecNo, zTempHold.sWi, lLastBusinessDay,
                          zSTTable, zTempHold.iID, zCPTable, pzPTable,
                          &zTempPriceFromHist);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return 0;

  // If the security has a valid accrual for the last business day (or last day
  // - depending on the flag) then look in divint table for the accrual rate on
  // next business day (or next day)
  if (!IsValueZero(zTempPriceFromHist.fAccrInt, 7)) {
    if (bLastBusinessDay)
      dInterest = lpfnSelectDivint(zTempHold.sSecNo, zTempHold.sWi,
                                   lNextBusinessDay, &zErr);
    else
      dInterest = lpfnSelectDivint(zTempHold.sSecNo, zTempHold.sWi,
                                   lPriceDate + 1, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return 0;

    return dInterest / 10;
  } else
    return 0;
} // GetAccruedInterest

// The routine gets the accrual for Last Business day
double GetAccruedInterestForLastBusinessDay(const long lPriceDate,
                                            HOLDINGS zTempHold) {
  long lTempDate, lLastBusinessDay = 0, lNextBusinessDay = 0;
  double dInterest;
  BOOL bInterestPaid;
  ERRSTRUCT zErr;

  if (lpfnLastBusinessDay(lPriceDate, const_cast<char *>("USA"),
                          const_cast<char *>("M"), &lLastBusinessDay) != 0)
    return 0;

  /*
  ** if the current pricing day is a holiday then the previous record is not
  * what we are
  ** interested in, we are interested in the previous business day(e.g. if a
  * bond pays
  ** on 5/1/2000, records for 4/28/2000(Friday), 4/29/2000(Saturday) and
  * 4/30/2000(Sunday)
  ** will have accr_int as 0(because all of them are calculating accrual on
  * 5/1). If pricing
  ** on 4/28, last business day will be 4/27 and that's the record we want, but
  * on 4/30, last
  ** business record will be 4/28 which is not what we want, if get the previous
  * business day
  ** from 4/28 then we will get the required record on 4/27.
  ** SB 10/30/00 - There is an exception to the above rule, if the security pays
  * after last
  ** business day and on or prior to pricing date then we should go back only to
  * prior business
  ** day instead of prior to prior business day, e.g. if bond pays on
  * 9/30/00(Saturday), and
  ** the price date is 9/30/2000, if we go to previous(9/29/2000) to
  * previous(9/28/2000) date and
  ** then that accrual will be almost the full accrual and if that accrual is
  * taken then we
  ** will double count (accrual as well as cash we already got on 9/30/2000), so
  * in this situation,
  ** we need to go back only one day(9/29/2000)
  */
  if (lpfnIsItAMarketHoliday(lPriceDate, const_cast<char *>("USA")) == 1) {
    // check if security paid interest between last business day + 1 and pricing
    // date
    bInterestPaid = FALSE;
    for (lTempDate = lLastBusinessDay + 1; lTempDate <= lPriceDate;
         lTempDate++) {
      lpfnSelectDivint(zTempHold.sSecNo, zTempHold.sWi, lTempDate, &zErr);
      if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
        bInterestPaid = TRUE;
    }

    // if interest was not paid then get previous business date
    if (!bInterestPaid &&
        lpfnLastBusinessDay(lLastBusinessDay, const_cast<char *>("USA"),
                            const_cast<char *>("M"), &lLastBusinessDay) != 0)
      return 0;
  }

  // See if the security had any accrual on the last business day (or last day -
  // depending on the flag)
  dInterest = lpfnSelectSecurityRate(zTempHold.sSecNo, zTempHold.sWi,
                                     lLastBusinessDay, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return 0;

  return dInterest;
} /*GetAccruedInterestForLastBusinessDay*/

//---------------------------------------------------------------------------------
////
// GK Added this downto bottom
// // 2001-3-6 For Holdings <Accr_int>
//---------------------------------------------------------------------------------
////

void InitializeAccdivTable(ACCDIVTABLE *pzADTable) {
  if (pzADTable->iAccdivCreated > 0 && pzADTable->pzAccdiv != NULL)
    free(pzADTable->pzAccdiv);

  pzADTable->pzAccdiv = NULL;
  pzADTable->iAccdivCreated = pzADTable->iNumAccdiv = 0;
} // initializeaccdivtable

void InitializeAssetTable(ASSETTABLE *pzATable) {
  if (pzATable->iAssetCreated > 0 && pzATable->pzAsset != NULL)
    free(pzATable->pzAsset);

  pzATable->pzAsset = NULL;
  pzATable->iAssetCreated = pzATable->iNumAsset = 0;
} /* initializeassettable */

/**
** This function gets all undeleted accdiv and adds them to the accdiv table.
**/
ERRSTRUCT FillAccdivTable(int iID, BOOL bIncome, BOOL bIncByLot, long lValDate,
                          ASSETTABLE *pzATable, ACCDIVTABLE *pzAccdivTable) {
  ERRSTRUCT zErr;
  PARTACCDIV zTempAccdiv;

  lpprInitializeErrStruct(&zErr);

  // Select all 'pending' (as of valdate) RD trans
  while (!zErr.iSqlError) {
    lpprInitializeErrStruct(&zErr);
    InitializePartialAccdiv(&zTempAccdiv);

    lpprSelectPendingAccdivTransForAnAccount(iID, lValDate, &zTempAccdiv,
                                             &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zTempAccdiv.fUnitsAccountedFor = 0;

    zErr = AddAccdivToTable(pzAccdivTable, zTempAccdiv);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  } /* while no error */

  // Select all pending accdiv (even deleted one) as of valdate
  while (!zErr.iSqlError) {
    lpprInitializeErrStruct(&zErr);
    InitializePartialAccdiv(&zTempAccdiv);

    lpprSelectAllAccdivForAnAccount(iID, lValDate, lValDate, &zTempAccdiv,
                                    &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zTempAccdiv.fUnitsAccountedFor = 0;
    // Don't add it to table if we already found the same dividend for the same
    // taxlot in accdiv
    if (FindDividendInAccdiv(*pzAccdivTable, iID, bIncome,
                             zTempAccdiv.lDivintNo, zTempAccdiv.lTransNo,
                             zTempAccdiv.sSecNo, zTempAccdiv.sWi,
                             zTempAccdiv.sSecXtend, zTempAccdiv.sAcctType,
                             zTempAccdiv.lEffDate) < 0) {
      zErr = AddAccdivToTable(pzAccdivTable, zTempAccdiv);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    }
  } /* while no error */

  return zErr;
} // fillaccdivtable

void InitializePartialAccdiv(PARTACCDIV *pzPAccdiv) {
  pzPAccdiv->iID = pzPAccdiv->lTransNo = pzPAccdiv->lDivintNo = 0;
  pzPAccdiv->sSecNo[0] = pzPAccdiv->sWi[0] = pzPAccdiv->sSecXtend[0] = '\0';
  pzPAccdiv->sAcctType[0] = pzPAccdiv->sTranType[0] = pzPAccdiv->sDrCr[0] =
      '\0';
  pzPAccdiv->fPcplAmt = pzPAccdiv->fIncomeAmt = pzPAccdiv->fUnits = 0;
  pzPAccdiv->lTrdDate = pzPAccdiv->lStlDate = pzPAccdiv->lEffDate = 0;
  pzPAccdiv->sCurrId[0] = pzPAccdiv->sCurrAcctType[0] =
      pzPAccdiv->sIncCurrId[0] = pzPAccdiv->sIncAcctType[0] = '\0';
  pzPAccdiv->fBaseXrate = pzPAccdiv->fIncBaseXrate = pzPAccdiv->fSecBaseXrate =
      pzPAccdiv->fAccrBaseXrate = 1.0;
  pzPAccdiv->fUnitsAccountedFor = 0;
} // initializepartialaccdiv

/**
** This function adds an accdiv record to the accdiv table. This function does
* not
** check whether the passed accdiv record already exist in the table or not.
**/
ERRSTRUCT AddAccdivToTable(ACCDIVTABLE *pzAccdivTable, PARTACCDIV zPAccdiv) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  // If table is full to its limit, allocate more space
  if (pzAccdivTable->iAccdivCreated == pzAccdivTable->iNumAccdiv) {
    pzAccdivTable->iAccdivCreated += EXTRAACCDIV;
    pzAccdivTable->pzAccdiv = (PARTACCDIV *)realloc(
        pzAccdivTable->pzAccdiv,
        pzAccdivTable->iAccdivCreated * sizeof(PARTACCDIV));
    if (pzAccdivTable->pzAccdiv == NULL)
      return (lpfnPrintError("Insufficient Memory For HoldTable", 0, 0, "", 997,
                             0, 0, "CALCPERF ADDHOLD", FALSE));
  }

  pzAccdivTable->pzAccdiv[pzAccdivTable->iNumAccdiv] = zPAccdiv;
  pzAccdivTable->iNumAccdiv++;

  return zErr;
} // AddAccdivtotable

int FindDividendInAccdiv(ACCDIVTABLE zAccdivTable, int iID, BOOL bIncome,
                         long lDivintNo, long lTransNo, char *sSecNo, char *sWi,
                         char *sSecXtend, char *sAcctType, long lEffDate) {
  int i, iResult;

  iResult = -1;
  for (i = 0; i < zAccdivTable.iNumAccdiv; i++) {
    // SB 11/11/10 - if RD transactions are generated in Accdiv not posted as
    // trans, by the system, taxlot number and dividend number will not be
    // filled. In this case, if pay date has passed (greater than or equal to
    // system pricing date), then just do a match on pay date. Even for pay
    // date, give a garce period of 3 days.

    // SB 12/12/11 - There are cases where settlement dates falls on the wekend
    // but we don't get RD transaction until next business days. In these cases
    // we should still take pending accrual from accdiv. When first such record
    // would come from accdiv, the first condition below would not be satisfied
    // and that first lot would be added to the table, however, for all
    // subsequent lots from accdiv first condition was being satisfied (as this
    // condition doesn't on purpose match lot number) and they were being
    // skipped. To avoid this case, added another condition (accdiv.ltransno ==
    // 0) to first if statement.This addition will make sure that first
    // condition applies only to those pending dividend that are from
    // transaction and came on the feed, hence posted without a taxlot number.
    if (!bIncome && zAccdivTable.pzAccdiv[i].lEffDate < lStarsTrdDate &&
        zAccdivTable.pzAccdiv[i].lTransNo == 0) {
      if (zAccdivTable.pzAccdiv[i].iID == iID &&
          strcmp(zAccdivTable.pzAccdiv[i].sSecNo, sSecNo) == 0 &&
          strcmp(zAccdivTable.pzAccdiv[i].sWi, sWi) == 0 &&
          strcmp(zAccdivTable.pzAccdiv[i].sSecXtend, sSecXtend) == 0 &&
          strcmp(zAccdivTable.pzAccdiv[i].sAcctType, sAcctType) == 0 &&
          abs(zAccdivTable.pzAccdiv[i].lEffDate - lEffDate) <= 3) {
        iResult = i;
        break;
      }
    } else if (zAccdivTable.pzAccdiv[i].iID == iID &&
               zAccdivTable.pzAccdiv[i].lDivintNo == lDivintNo &&
               zAccdivTable.pzAccdiv[i].lTransNo == lTransNo) {
      iResult = i;
      break;
    }
  }

  return iResult;
} // FindDividendInAccdiv

double StraightLineDiscountedAccrual(double fTotCost, double fParAmount,
                                     long lStlDate, long lMaturityDate,
                                     long lPricingDate) {
  long lDaysToMaturity, lDaysHeld;
  double fAccrualAmount;

  fAccrualAmount = 0;
  lDaysToMaturity = lDaysHeld = 0;

  // Should not happen, discounted securities shouldn't be bought for more than
  // par amount but if it does, there is no accrual
  if (fParAmount <= fTotCost)
    return fAccrualAmount;

  // This shouldn't happen either, settlement date can't be after maturity date
  // but if it does, there is no accrual
  if (lStlDate >= lMaturityDate)
    return fAccrualAmount;
  else
    lDaysToMaturity = lMaturityDate - lStlDate;

  // If pricing date is same or earlier than settlement date, no accrual
  if (lStlDate >= lPricingDate)
    return fAccrualAmount;
  else
    lDaysHeld = lPricingDate - lStlDate;

  fAccrualAmount = (fParAmount - fTotCost) * lDaysHeld / lDaysToMaturity;

  return fAccrualAmount;
}

ERRSTRUCT GetTotalUnitsEligibleForDividend(int iID, char *sSecNo, char *sWi,
                                           char *sSecXtend, char *sAcctType,
                                           long lValDate, long lRecDate,
                                           long lPayDate, double *pfUnits)

{
  ERRSTRUCT zErr;
  double fUnits = 0;

  lpprInitializeErrStruct(&zErr);
  *pfUnits = fUnits = 0;

  // Get sum of units from holdings
  lpprSelectUnitsHeldForASecurity(iID, sSecNo, sWi, sSecXtend, sAcctType,
                                  lRecDate, lPayDate, &fUnits, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  *pfUnits = fUnits;

  fUnits = 0;
  // get sum of units from trans, if any, w
  lpprSelectUnitsForASoldSecurity(iID, sSecNo, sWi, sSecXtend, sAcctType,
                                  lValDate, lRecDate, lPayDate, &fUnits, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  *pfUnits += fUnits;

  return zErr;
}
} // extern "C"
