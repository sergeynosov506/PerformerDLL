/*H*
 *
 * FILENAME: Roll.c
 *
 * DESCRIPTION: The purpose of this function is to recreate security positions
 *              and cash balances for a given portfolio on a given date. The
 *              function is capable of not only accepting a single portfolio,
 * but also the whole firm, a list of accounts, composites, managers, securities
 * and transactions. It sould be noted that if user has supplied a(list of)
 * securities or transactions then the positions(and cash balances) created by
 * this function does not represent the whole portfolio.
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR: Shobhit Barman(Effron Enterprises, Inc.)
 *
 * 2022-05-20 J#PER12177 Fixed logic for pmperf - mk.
 * 2022-03-31 J#PER12156 Adjusted logic for earliest holdmap date to roll
 * transactions back to start - mk. 2022-03-31 J#PER12156 Adjusted logic for
 * earliest holdmap date to roll transactions back to inception - mk.
 *H*/

#include "roll.h"
#include "commonheader.h"

HINSTANCE hTEngineDll, hOledbIODll, hStarsUtilsDll;
static BOOL bInit = FALSE;

BOOL APIENTRY DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_PROCESS_DETACH:
    FreeRoll();
    break;

  default:
    break;
  }

  return TRUE;

} // DllMain

/*F*
** Roll library function. The function to recreate security positions and
** cash balances for a portfolio on a given date.
** Possible values for alphaflag:
**  F - whole firm
**  B - Branch Account, ID has the account name
**  S - Security, ID has account, AlphaId1 has sec_no and
**      AlphaId2 has when_issue
**  C - Composite, ID has the composite ID
**  M - Manager, AlphaId1 has the manager code
** FS - Firm Wide by security, AlphaId1 has sec_no and AlphaId2 has when_iss
**
** Numeric flag can be used in addition to AlphaFlag, its possible values are:
**  T - Transaction number, numericid1 and numericid2 define the range. This
**      can be used only if alphaid is 'B'(branch account).
** TD - Trade date, numericid1 and numericid2 define the date range
** ED - Effective date, numericid1 and id2 define the date range
** ND - Entry date, numericid1 and id2 define the range
**
** For partial roll, destination is always adhoc data set but for full roll if
** a data set exist for the roll date, that is used as destination. User can
** overwrite it to make Adhoc data set or the reorg data set as the destination
** even though a full roll is requested(& a data set for roll date exists).
** WhichDataSet :  1 - Roll
**                 2 - Perform
**              else - Depends on Date
*F*/
DLLAPI ERRSTRUCT STDCALL WINAPI Roll(int iID, char *sAlphaId1, char *sAlphaId2,
                                     long lNumericId1, long lNumericId2,
                                     char *sAlphaFlag, char *sNumericFlag,
                                     BOOL bInitDataSet, int iWhichDataSet,
                                     long lRollDate, int iResetPerfDate) {
  ERRSTRUCT zErr;
  long lSrceDate = 0, lDestDate, lTransStartDate, lTransEndDate, lTempLong;
  BOOL bPartialRoll, bPortmainSelected, bMultipleAccnts, bUseTrans;
  char sSecNo[13], sWi[2], sMsg[70];
  int iAccntProcessed;
  PORTMAIN zPR;
  short iMDY[3];

  lpprInitializeErrStruct(&zErr);
  lTransStartDate = lTransEndDate = 0;
  strcpy_s(sSecNo, "");
  strcpy_s(sWi, "");
  lpprInitializePortmainStruct(&zPR);
  bPortmainSelected = bMultipleAccnts = FALSE;

  // If not rolling for a month end date, don't reset the perf dates even if
  // user has asked for it
  if (!lpfnIsItAMonthEnd(lRollDate))
    iResetPerfDate = 0;
  /*
  ** User MUST use AlphaFlag to indicate what the program is supposed to do,
  ** in addition, NumericFlag can also be used(in no case it can be used alone).
  ** Check both flags to figure out what user want to do.
  */
  if (strcmp(sNumericFlag, "T") == 0 || strcmp(sNumericFlag, "TD") == 0 ||
      strcmp(sNumericFlag, "ED") == 0 || strcmp(sNumericFlag, "ND") == 0) {
    bPartialRoll = TRUE;
    if (strcmp(sNumericFlag, "T") == 0 && strcmp(sAlphaFlag, "B") != 0)
      return (lpfnPrintError("Invalid Alpha and Numeric Flag Cmbination", 0, 0,
                             "", 600, 0, 0, "ROLL1", FALSE));
  } else {
    if (sNumericFlag[0] != '\0')
      return (lpfnPrintError("Invalid Numeric Flag", 0, 0, "", 601, 0, 0,
                             "ROLL2", FALSE));

    if (strcmp(sAlphaFlag, "S") == 0 || strcmp(sAlphaFlag, "FS") == 0)
      bPartialRoll = TRUE;
    else if (strcmp(sAlphaFlag, "F") == 0 || strcmp(sAlphaFlag, "B") == 0 ||
             strcmp(sAlphaFlag, "C") == 0 || strcmp(sAlphaFlag, "M") == 0)
      bPartialRoll = FALSE;
    else
      return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0, "ROLL3",
                             FALSE));
  } /* NumericFlag is not used */

  /*
  ** Cursor that gets all the firm account reads all the required data from
  ** portdir table, so there is no need to read portdir again for these
  ** accounts.
  */
  if (strcmp(sAlphaFlag, "F") == 0)
    bPortmainSelected = TRUE;
  else
    bPortmainSelected = FALSE;

  /*
  ** If we are doing roll for multiple accounts, cursors for source and
  ** destination should be initialized only once.
  */
  if (strcmp(sAlphaFlag, "B") == 0 || strcmp(sAlphaFlag, "S") == 0)
    bMultipleAccnts = FALSE;
  else
    bMultipleAccnts = TRUE;

  /*
  ** If doing a partial roll, or if user has specifically asked for the adhoc
  ** data set, results always go in the Ad Hoc data set. When doing full roll,
  ** initially, set the destdate to roll date, "AccntDeletePrepare" function
  ** will reset this date to adhoc date if no record on roll date is found
  ** in holdmap file.
  ** 09/20/2005 - allow settlement tables for Unit Accounting - JTG
  */
  if (iWhichDataSet != 1 && iWhichDataSet != 2 && iWhichDataSet != 3)
    iWhichDataSet = 0;

  zErr = FindOutDestinationDataSet(lRollDate, iWhichDataSet, bPartialRoll,
                                   &lDestDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // Initialize tranproc with the new dates
  zErr = CallInitTranProc(lDestDate, const_cast<char *>(""),
                          const_cast<char *>(""), const_cast<char *>(""),
                          const_cast<char *>(""));
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /* Initialize all delete cursors */
  /*if (bMultipleAccnts == FALSE)
  {
zErr = AccntDestInit(pzPR->iID, sSecNo, sWi, -1, lDestDate, FALSE);
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
       return zErr;
  }*/

  /* If doing roll of multiple accounts, find out a common source data set */
  if (bMultipleAccnts) {
    zErr = FindOutSourceDataSet(
        zPR, const_cast<char *>(""), const_cast<char *>(""), lRollDate,
        &lSrceDate, &lTempLong, &lTransStartDate, &lTransEndDate,
        bMultipleAccnts, FALSE, FALSE, &bInitDataSet, iWhichDataSet);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } else if (strcmp(sAlphaFlag, "B") == 0) {
    zPR.iID = iID;
    strcpy_s(sSecNo, "");
    strcpy_s(sWi, "");
  } else if (strcmp(sAlphaFlag, "S") == 0) {
    zPR.iID = iID;
    strcpy_s(sSecNo, sAlphaId1);
    strcpy_s(sWi, sAlphaId2);
  } else if (strcmp(sAlphaFlag, "FS") == 0) {
    strcpy_s(sSecNo, sAlphaId1);
    strcpy_s(sWi, sAlphaId2);

    /*
    ** Create a cursor to get all the branch account having the given security
    ** on the roll date.
    */
    zErr = CreateFSecBrAcctCursor(sSecNo, sWi, lRollDate, lSrceDate, lDestDate);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Initialize Destination Data Set(copy from source) */
    zErr = FirmDestInit(sSecNo, sWi, lSrceDate, lDestDate);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } else
    return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0, "ROLL5",
                           FALSE));

  /* Do the required processing, one account at a time */
  iAccntProcessed = 0;
  for (;;) {
    if (strcmp(sAlphaFlag, "F") == 0) {
      zErr = GetNextAccount(const_cast<char *>("F"), &zPR);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break; /* We are done */
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* "F" - Whole Firm */
    if (strcmp(sAlphaFlag, "B") == 0) {
      if (iAccntProcessed > 0)
        break;
    } /* "B" - branch account */
    else if (strcmp(sAlphaFlag, "S") == 0) {
      if (iAccntProcessed > 0)
        break;
    } /* "S" - security */
    else if (strcmp(sAlphaFlag, "C") == 0) {
      zErr = GetNextAccount(const_cast<char *>("C"), &zPR);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* "C" - composite */
    else if (strcmp(sAlphaFlag, "M") == 0) {
      zErr = GetNextAccount(const_cast<char *>("M"), &zPR);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* "M" - Manager */
    else if (strcmp(sAlphaFlag, "FS") == 0) {
      zErr = GetFSecBrAcct(&zPR);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* "FS" - Whole Firm By Security*/
    else
      return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0, "ROLL6",
                             FALSE));

    // start DB trans before doing actual processing
    bUseTrans = (lpfnGetTransCount() == 0);
    if (bUseTrans)
      lpfnStartDBTransaction();

    __try {

      /*
      ** Call the function which does the actual processing. If we are rolling
      * for more than account,
      ** don't stop the processing, reset the error and continue, unless it is a
      * firmwide roll for a
      ** securiy(write now reorg is doing that and it wants to stop if any error
      * occurs in any of the account).
      */
      zErr = RollAnAccount(&zPR, sSecNo, sWi, bPartialRoll, bInitDataSet,
                           lRollDate, lSrceDate, lDestDate, lTransStartDate,
                           lTransEndDate, bPortmainSelected, bMultipleAccnts,
                           iResetPerfDate, iWhichDataSet, FALSE, FALSE);
      /*	  zErr = RollAnAccount(&zPR, sSecNo, sWi, bPartialRoll,
         sNumericFlag, lNumericId1, lNumericId2, bInitDataSet, lRollDate,
         lSrceDate, lDestDate, lTransStartDate, lTransEndDate,
         bPortmainSelected, bMultipleAccnts,iResetPerfDate,FALSE);*/
    } __except (lpfnAbortDBTransaction(bUseTrans)) {
    }

    lpfnrjulmdy(lRollDate, iMDY);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
      // roll failed - rollback any database changes
      if (bUseTrans)
        lpfnRollbackDBTransaction();

      sprintf_s(sMsg, "Roll For Portfolio %s On %d/%d/%d Failed",
                zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
      lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iIsamCode, "ROLL7", TRUE);
    } else {
      // roll succeeded - commit changes
      if (bUseTrans)
        lpfnCommitDBTransaction();

      sprintf_s(sMsg, "Portfolio %s Successfully Rolled On %d/%d/%d",
                zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
      lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iIsamCode, "ROLL8", TRUE);
    }
    if ((zErr.iSqlError != 0 || zErr.iBusinessError != 0) && bMultipleAccnts &&
        strcmp(sAlphaFlag, "FS") != 0)
      zErr.iSqlError = zErr.iBusinessError = 0;

    iAccntProcessed++;
  } /* while no error */

  return zErr;
} /* Roll */

/*F*
** This routine rolls an acount with current holdings always being the source
*F*/
DLLAPI ERRSTRUCT STDCALL WINAPI RollFromCurrent(int iID, char *sAlphaFlag,
                                                BOOL bInitDataSet,
                                                int iWhichDataSet,
                                                long lRollDate,
                                                int iResetPerfDate) {
  ERRSTRUCT zErr;
  PORTMAIN zPR;
  BOOL bPortmainSelected, bMultipleAccnts, bUseTrans;
  long lDestDate, lSrceDate, lTempLong, lTransStartDate, lTransEndDate;
  char sSecNo[13], sWi[2], sMsg[70];
  int iAccntProcessed;
  short iMDY[3];

  if (!bInit) {
    FILE *f = fopen("C:\\Users\\Sergey\\.gemini\\roll_debug.log", "a");
    if (f) {
      fprintf(f, "RollFromCurrent: bInit is FALSE. Returning error.\n");
      fclose(f);
    }
    memset(&zErr, 0, sizeof(ERRSTRUCT));
    zErr.iSqlError = -1;
    return zErr;
  }

  FILE *f = fopen("C:\\Users\\Sergey\\.gemini\\roll_debug.log", "a");
  if (f) {
    fprintf(f, "RollFromCurrent: bInit is TRUE.\n");
    fclose(f);
  }

  lpprInitializeErrStruct(&zErr);
  lDestDate = lSrceDate = lTempLong = lTransStartDate = lTransEndDate = 0;
  strcpy_s(sSecNo, "");
  strcpy_s(sWi, "");
  lpprInitializePortmainStruct(&zPR);
  bPortmainSelected = bMultipleAccnts = FALSE;

  // If not rolling for a month end date, don't reset the perf dates even if
  // user has asked for it
  if (!lpfnIsItAMonthEnd(lRollDate))
    iResetPerfDate = 0;

  // Calling program MUST use AlphaFlag to indicate what the program is supposed
  // to do,
  if (strcmp(sAlphaFlag, "F") != 0 && strcmp(sAlphaFlag, "B") != 0)
    return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0,
                           "ROLLFROMCURRENT1", FALSE));

  // For firm, all portmain will be selected here, else not
  if (strcmp(sAlphaFlag, "F") == 0) {
    bPortmainSelected = TRUE;
    bMultipleAccnts = TRUE;
  } else {
    bPortmainSelected = FALSE;
    bMultipleAccnts = FALSE;
  }

  /*
  ** if not adhoc(1) or performance(2), destination is same as roll date. If the
  * destination
  ** does not exists in holdmap, "FindOutDestnationDataSet" function resets
  * it(by reseting lDestDate)
  ** to be adhoc data set.
  */
  if (iWhichDataSet != 1 && iWhichDataSet != 2)
    iWhichDataSet = 0;

  zErr = FindOutDestinationDataSet(lRollDate, iWhichDataSet, FALSE, &lDestDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // Initialize tranproc for the destination data set
  zErr = CallInitTranProc(lDestDate, const_cast<char *>(""),
                          const_cast<char *>(""), const_cast<char *>(""),
                          const_cast<char *>(""));
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // If doing roll of multiple accounts, find out a common source data set
  if (bMultipleAccnts) {
    zErr = FindOutSourceDataSet(
        zPR, const_cast<char *>(""), const_cast<char *>(""), lRollDate,
        &lSrceDate, &lTempLong, &lTransStartDate, &lTransEndDate,
        bMultipleAccnts, TRUE, FALSE, &bInitDataSet, iWhichDataSet);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } else if (strcmp(sAlphaFlag, "B") == 0) {
    zPR.iID = iID;
    strcpy_s(sSecNo, "");
    strcpy_s(sWi, "");
  } else
    return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0,
                           "ROLLFROMCURRENT2", FALSE));

  /* Do the required processing, one account at a time */
  iAccntProcessed = 0;
  for (;;) {
    if (strcmp(sAlphaFlag, "F") == 0) {
      zErr = GetNextAccount(const_cast<char *>("F"), &zPR);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break; /* We are done */
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* "F" - Whole Firm */
    else if (strcmp(sAlphaFlag, "B") == 0) {
      if (iAccntProcessed > 0) // processed the account of interest
        break;
    } /* "B" - branch account */
    else
      return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0,
                             "ROLLFROMCURRENT3", FALSE));

    // start DB trans before doing actual processing
    bUseTrans = (lpfnGetTransCount() == 0);
    if (bUseTrans)
      lpfnStartDBTransaction();

    __try {
      /*
      ** Call the function which does the actual processing.
      ** If we are rolling for more than accounts, don't stop the processing,
      ** reset the error and continue.
      */
      /*	  zErr = RollAnAccount(&zPR, sSecNo, sWi, FALSE, "", 0, 0,
         bInitDataSet, lRollDate, lSrceDate, lDestDate, lTransStartDate,
         lTransEndDate, bPortmainSelected, bMultipleAccnts, iResetPerfDate,
         TRUE);*/
      zErr = RollAnAccount(&zPR, sSecNo, sWi, FALSE, bInitDataSet, lRollDate,
                           lSrceDate, lDestDate, lTransStartDate, lTransEndDate,
                           bPortmainSelected, bMultipleAccnts, iResetPerfDate,
                           iWhichDataSet, TRUE, FALSE);
    } __except (lpfnAbortDBTransaction(bUseTrans)) {
    }

    lpfnrjulmdy(lRollDate, iMDY);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
      // roll failed - rollback any changes
      if (bUseTrans)
        lpfnRollbackDBTransaction();

      sprintf_s(sMsg, "Roll For Portfolio %s On %d/%d/%d Failed",
                zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
      lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iIsamCode, "ROLLFROMCURRENT4", TRUE);
    } else {
      // roll succeeded - commit changes
      if (bUseTrans)
        lpfnCommitDBTransaction();

      sprintf_s(sMsg, "Portfolio %s Successfully Rolled On %d/%d/%d",
                zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
      lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iIsamCode, "ROLLFROMCURRENT5", TRUE);
    }
    if ((zErr.iSqlError != 0 || zErr.iBusinessError != 0) && bMultipleAccnts &&
        strcmp(sAlphaFlag, "FS") != 0)
      zErr.iSqlError = zErr.iBusinessError = 0;

    iAccntProcessed++;
  } /* while no error */

  return zErr;
} /* RollFromCurrent */

DLLAPI ERRSTRUCT STDCALL WINAPI SettlementRoll(int iID, char *sAlphaFlag,
                                               long lRollDate,
                                               BOOL bDoTransaction) {
  ERRSTRUCT zErr;
  long lSrceDate = 0, lDestDate, lTempLong;
  BOOL bPortmainSelected, bMultipleAccnts;
  char sDataType[2], sMsg[70], sTemp[STR80LEN];
  int iAccntProcessed, iWhichDataSet;
  PORTMAIN zPR;
  short iMDY[3];

  lpprInitializeErrStruct(&zErr);

  lpprInitializePortmainStruct(&zPR);
  bPortmainSelected = bMultipleAccnts = FALSE;

  // Calling program MUST use AlphaFlag to indicate what the program is supposed
  // to do,
  if (strcmp(sAlphaFlag, "F") != 0 && strcmp(sAlphaFlag, "B") != 0)
    return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0,
                           "SETTLEMENT ROLL1", FALSE));

  /*
  ** Cursor that gets all the firm account reads all the required data from
  ** portdir table, so there is no need to read portdir again for these
  ** accounts.
  */
  if (strcmp(sAlphaFlag, "F") == 0) {
    bPortmainSelected = TRUE;
    bMultipleAccnts = TRUE;
  } else {
    bPortmainSelected = FALSE;
    bMultipleAccnts = FALSE;
  }

  // Results aalways go to settlement date tables.
  iWhichDataSet = 3;
  lDestDate = SETTLEMENT_DATE;

  // Initialize tranproc with the new dates
  zErr = CallInitTranProc(lDestDate, const_cast<char *>(""),
                          const_cast<char *>(""), const_cast<char *>(""),
                          const_cast<char *>(""));
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  lSrceDate = -1;
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    lpprSelectAllHoldmap(sTemp, sTemp, sTemp, sTemp, sTemp, sTemp, sTemp,
                         sDataType, &lTempLong, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Reading Holdmap", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode, "SETTLEMENT ROLL2",
                             FALSE));

    if (strcmp(sDataType, "C") != 0)
      continue; // Even though we found an answer, let the query read all the
                // entries so that the cursor can be proerply closed

    lSrceDate = lTempLong;
  } /* while no error */

  if (lSrceDate == -1)
    return (lpfnPrintError("No current Holdings Found", 0, 0, "", 0,
                           zErr.iSqlError, zErr.iIsamCode, "SETTLEMENT ROLL3",
                           FALSE));
  if (strcmp(sAlphaFlag, "B") == 0)
    zPR.iID = iID;

  /* Do the required processing, one account at a time */
  iAccntProcessed = 0;
  for (;;) {
    if (strcmp(sAlphaFlag, "F") == 0) {
      zErr = GetNextAccount(const_cast<char *>("F"), &zPR);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break; /* We are done */
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* "F" - Whole Firm */
    else if (strcmp(sAlphaFlag, "B") == 0) {
      if (iAccntProcessed > 0)
        break;
    } /* "B" - branch account */
    else
      return (lpfnPrintError("Invalid Alpha Flag", 0, 0, "", 602, 0, 0,
                             "SETTLEMENT ROLL5", FALSE));

    // start DB trans before doing actual processing
    bDoTransaction = bDoTransaction && (lpfnGetTransCount() == 0);
    if (bDoTransaction)
      lpfnStartDBTransaction();

    __try {

      /*
      ** Call the function which does the actual processing. If we are rolling
      * for more than account,
      ** don't stop the processing, reset the error and continue.
      */
      zErr = SettlementDateRollAnAccount(&zPR, lRollDate, lSrceDate, lDestDate,
                                         !bMultipleAccnts);
    } __except (lpfnAbortDBTransaction(bDoTransaction)) {
    }

    lpfnrjulmdy(lRollDate, iMDY);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
      // roll failed - rollback any database changes
      if (bDoTransaction)
        lpfnRollbackDBTransaction();

      sprintf_s(sMsg, "Roll For Portfolio %s On %d/%d/%d Failed",
                zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
      lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iIsamCode, "ROLL7", TRUE);

      if (bMultipleAccnts)
        zErr.iSqlError = zErr.iBusinessError = 0;
    } else {
      // roll succeeded - commit changes
      if (bDoTransaction)
        lpfnCommitDBTransaction();

      sprintf_s(sMsg, "Portfolio %s Successfully Rolled On %d/%d/%d",
                zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
      lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                     zErr.iIsamCode, "ROLL8", TRUE);
    } // no error

    iAccntProcessed++;
  } /* while no error */

  return zErr;
} /* SettlementRoll */

/*F*
** This routine rolls an acount completely from inception, doesn't use any
* existing data as source
*F*/
DLLAPI ERRSTRUCT STDCALL WINAPI RollFromInception(int iID, int iWhichDataSet,
                                                  long lRollDate,
                                                  int iResetPerfDate) {
  ERRSTRUCT zErr;
  PORTMAIN zPR;
  BOOL bUseTrans;
  long lDestDate;
  char sMsg[70];
  short iMDY[3];

  lpprInitializeErrStruct(&zErr);

  lpprInitializePortmainStruct(&zPR);

  /*
  ** if not adhoc(1) or performance(2), destination is same as roll date. If the
  * destination
  ** does not exists in holdmap, "FindOutDestnationDataSet" function resets
  * it(by reseting lDestDate)
  ** to be adhoc data set.
  */
  if (iWhichDataSet != 1 && iWhichDataSet != 2)
    iWhichDataSet = 0;

  zErr = FindOutDestinationDataSet(lRollDate, iWhichDataSet, FALSE, &lDestDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // Initialize tranproc for the destination data set
  zErr = CallInitTranProc(lDestDate, const_cast<char *>(""),
                          const_cast<char *>(""), const_cast<char *>(""),
                          const_cast<char *>(""));
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zPR.iID = iID;

  // start DB trans before doing actual processing
  bUseTrans = (lpfnGetTransCount() == 0);
  if (bUseTrans)
    lpfnStartDBTransaction();

  __try {
    /*
    ** Call the function which does the actual processing.
    */
    zErr = RollAnAccount(&zPR, const_cast<char *>(""), const_cast<char *>(""),
                         FALSE, TRUE, lRollDate, 0, lDestDate, 0, 0, FALSE,
                         FALSE, iResetPerfDate, iWhichDataSet, FALSE, TRUE);
  } __except (lpfnAbortDBTransaction(bUseTrans)) {
  }

  lpfnrjulmdy(lRollDate, iMDY);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    // roll failed - rollback any changes
    if (bUseTrans)
      lpfnRollbackDBTransaction();

    sprintf_s(sMsg, "Roll For Portfolio %s On %d/%d/%d Failed", zPR.sUniqueName,
              iMDY[0], iMDY[1], iMDY[2]);
    lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                   zErr.iIsamCode, "ROLLFROMINCEPTION1", TRUE);
  } else {
    // roll succeeded - commit changes
    if (bUseTrans)
      lpfnCommitDBTransaction();

    sprintf_s(sMsg, "Portfolio %s Successfully Rolled On %d/%d/%d",
              zPR.sUniqueName, iMDY[0], iMDY[1], iMDY[2]);
    lpfnPrintError(sMsg, zPR.iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                   zErr.iIsamCode, "ROLLFROMINCEPTION2", TRUE);
  }

  return zErr;
} /* RollFromInception */

ERRSTRUCT RollAnAccount(PORTMAIN *pzPR, char *sSecNo, char *sWi,
                        BOOL bPartialRoll, BOOL bInitDataSet, long lRollDate,
                        long lSrceDate, long lDestDate, long lTransStartDate,
                        long lTransEndDate, BOOL bPortmainAlreadySelected,
                        BOOL bMultipleAccnts, BOOL iResetPerfDate,
                        int iWhichDataSet, BOOL bRollFromCurrent,
                        BOOL bRollFromInception) {
  static int iLastID = 0;

  ERRSTRUCT zErr;
  long lSrceLastTransNo, lNewLastTransNo;
  BOOL bBackwardRoll, bSpecificSecNo, bInitPmainPbal, bRollDateUpdated;
  TRANS zTR;
  long lLastMonthEnd;

  lpprInitializeErrStruct(&zErr);
  lLastMonthEnd = lpfnLastMonthEnd(lRollDate);

  if (sSecNo[0] == '\0') {
    bSpecificSecNo = FALSE;
    bInitPmainPbal = TRUE;
  } else {
    bSpecificSecNo = TRUE;
    if (pzPR->iID == iLastID)
      bInitPmainPbal = FALSE;
    else
      bInitPmainPbal = TRUE;
  }

  if (!bPortmainAlreadySelected) {
    lpprSelectPortmain(pzPR, pzPR->iID, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } /* No need to select portmain */

  /*
  ** If doing roll for multiple accounts and the account is deleted/purged/did
  ** not exist on the roll date then no need to send an error message, else send
  * an error.
  */
  if (bMultipleAccnts) {
    if ((pzPR->lDeleteDate != 0 && lRollDate > pzPR->lDeleteDate) ||
        lRollDate < pzPR->lInceptionDate || lRollDate < pzPR->lPurgeDate)
      return zErr;
  } /* multiple accounts */
  else {
    /* Can't roll a portfolio on a date after its delete date */
    if (pzPR->lDeleteDate != 0 && lRollDate > pzPR->lDeleteDate)
      return (lpfnPrintError("Portfolio Has Been Deleted Before The Roll Date",
                             pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT1",
                             FALSE));

    /* Can't roll a portfolio before its inception date */
    if (lRollDate < pzPR->lInceptionDate)
      return (lpfnPrintError("Portfolio Didn't Exist On The Roll Date",
                             pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT2",
                             FALSE));

    /* Can't roll a portfolio before its purge date */
    if (lRollDate < pzPR->lPurgeDate)
      return (lpfnPrintError(
          "Portfolio Has Been Purged. No Transactions Going Back That Far",
          pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT3", FALSE));
  } /* Single Account */

  /*
  ** There are two flags bRollFromCurrent and bRollFromInception, they both
  * can't be true at the same time,
  ** if both of them have been set to true, make brollFromCurrent FALSE
  */
  if (bRollFromCurrent && bRollFromInception)
    bRollFromCurrent = FALSE;

  /*
  ** If doing multiple accounts roll, source date must have already been
  ** set, else if destination date is same as roll date then source date is the
  ** roll date, else find out the source(base) data which we are going to use.
  */
  if (bMultipleAccnts) {
    zErr = GetLastTransNo(pzPR->iID, lSrceDate, &lSrceLastTransNo, TRUE);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      lSrceLastTransNo = LASTTRANSNO;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } else // single account
  {
    if (lDestDate == lRollDate && bRollFromCurrent == FALSE &&
        bRollFromInception == FALSE) {
      lSrceDate = lRollDate;
      zErr = GetLastTransNo(pzPR->iID, lSrceDate, &lSrceLastTransNo, TRUE);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        lSrceLastTransNo = LASTTRANSNO;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
      lTransStartDate = lTransEndDate = lRollDate;
    } else {
      zErr = FindOutSourceDataSet(
          *pzPR, sSecNo, sWi, lRollDate, &lSrceDate, &lSrceLastTransNo,
          &lTransStartDate, &lTransEndDate, bMultipleAccnts, bRollFromCurrent,
          bRollFromInception, &bInitDataSet, iWhichDataSet);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* if destination date is not roll date */
  } /* Single account roll */

  /* If srce date <= destination date then a forward roll, else backward */
  if (lSrceDate <= lRollDate)
    bBackwardRoll = FALSE;
  else
    bBackwardRoll = TRUE;

  /*
  ** If a full roll for a single account, destination data set initialization
  ** (deletion of old data and copying source over it) is required. For a
  ** partial roll, initialize only if user wants to(InitDataSet is TRUE). If
  ** source and destination are same, no initialization is needed.
  */
  if ((!bMultipleAccnts && lSrceDate != lDestDate &&
       (!bPartialRoll || bInitDataSet)) ||
      bRollFromInception) {
    if (!(iWhichDataSet == 2 && !bInitDataSet))
      zErr = AccntDestInit(pzPR->iID, sSecNo, sWi, lSrceDate, lDestDate,
                           bInitPmainPbal, bRollFromInception);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    iLastID = pzPR->iID;
  } // if destinatin needs to be initialized for a single account

  bRollDateUpdated = FALSE;
  // process all asof transactions, as long as we are not rolling from inception
  // forward
  lNewLastTransNo = lSrceLastTransNo;
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0 &&
         !bRollFromInception) {
    lpprAsofTrans(pzPR->iID, lSrceDate, lSrceLastTransNo, sSecNo, sWi,
                  bSpecificSecNo, &zTR, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;

    __try {
      // SB 4/2/03 - It's wrong to change srcelasttransno, because the next time
      // it is sent to lpprAsofTrans query, it's going to re open the query
      // because one of the argument will now be different, so send another
      // variable to CallUpdateHold function zErr = CallUpdateHold(zTR, *pzPR,
      // FALSE, &lSrceLastTransNo, iWhichDataSet, lRollDate, lDestDate,
      // &bRollDateUpdated);
      zErr = CallUpdateHold(zTR, *pzPR, FALSE, &lNewLastTransNo, iWhichDataSet,
                            lRollDate, lDestDate, &bRollDateUpdated);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } __except (zErr.iSqlError != 0 ? 1 : 0) {
    }
  } // while no error in processing asof transactions

  // If RollFromInception is trye, make source date to be a day less than what
  // it is (inception date), so the inception date transactions are not missed
  if (bRollFromInception)
    lTransStartDate -= 1;

  // process all regular transactions
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    if (bBackwardRoll)
      lpprBackwardTrans(pzPR->iID, lTransStartDate, lTransEndDate, sSecNo, sWi,
                        bSpecificSecNo, &zTR, &zErr);
    else
      lpprForwardTrans(pzPR->iID, lTransStartDate, lTransEndDate, sSecNo, sWi,
                       bSpecificSecNo, &zTR, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;

    __try {

      zErr = CallUpdateHold(zTR, *pzPR, bBackwardRoll, &lNewLastTransNo,
                            iWhichDataSet, lRollDate, lDestDate,
                            &bRollDateUpdated);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } __except (zErr.iSqlError != 0 ? 1 : 0) {
    }
  } // loop for regular transactions

  /*
  ** update the roll date and lasttransno if it has not been updated yet (For
  * DataSet 0,
  ** this gets updated after every single transaction - to avoid the problem of
  * effect of
  ** transaction being applied to holdings without portmain reflecting it).
  */
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0 && !bRollDateUpdated)
    zErr = UpdateRollDate(pzPR->iID, lRollDate, lNewLastTransNo, lDestDate);

  // Reset perf_date for all asof transactions
  if ((iResetPerfDate) && zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    if (iResetPerfDate == 1)
      lpprUpdatePerfDate(pzPR->iID, lLastMonthEnd, lRollDate, &zErr);
    else
      lpprUpdatePerfDate(pzPR->iID, 1, lRollDate, &zErr);
  }

  return zErr;
} // RollAnAccount

ERRSTRUCT SettlementDateRollAnAccount(PORTMAIN *pzPR, long lRollDate,
                                      long lSrceDate, long lDestDate,
                                      BOOL bNeedInitialization) {
  ERRSTRUCT zErr;
  long lSrceLastTransNo = 0;
  BOOL bRollDateUpdated;
  TRANS zTR;

  lpprInitializeErrStruct(&zErr);

  // Select portfolio if it has not already been selected
  if (bNeedInitialization) {
    lpprSelectPortmain(pzPR, pzPR->iID, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  /* Can't roll a portfolio on a date after its delete date */
  if (pzPR->lDeleteDate != 0 && lRollDate > pzPR->lDeleteDate)
    return (lpfnPrintError("Portfolio Has Been Deleted Before The Roll Date",
                           pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT1",
                           FALSE));

  /* Can't roll a portfolio before its inception date */
  if (lRollDate < pzPR->lInceptionDate)
    return (lpfnPrintError("Portfolio Didn't Exist On The Roll Date", pzPR->iID,
                           0, "", 604, 0, 0, "ROLL ROLLACCT2", FALSE));

  /* Can't roll a portfolio before its purge date */
  if (lRollDate < pzPR->lPurgeDate)
    return (lpfnPrintError(
        "Portfolio Has Been Purged. No Transactions Going Back That Far",
        pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT3", FALSE));

  if (bNeedInitialization) {
    zErr =
        AccntDestInit(pzPR->iID, const_cast<char *>(""), const_cast<char *>(""),
                      lSrceDate, lDestDate, TRUE, FALSE);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  bRollDateUpdated = FALSE;

  // get all the transactions with settlement date greater than roll date
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    lpprSelectTransBySettlementDate(pzPR->iID, lRollDate, &zTR, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;

    // For free recive use effective date instead of settlement date
    if (strcmp(zTR.sTranType, "FR") == 0 || strcmp(zTR.sTranType, "FB") == 0) {
      if (zTR.lEffDate <= lRollDate)
        continue;
    } // Free receive

    // Call update hold to reverse the transaction with settlement date is
    // greater than roll date
    zErr = CallUpdateHold(zTR, *pzPR, TRUE, &lSrceLastTransNo, 3, lRollDate,
                          lDestDate, &bRollDateUpdated);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } // loop for transactions

  /*
  ** update the roll date and lasttransno if it has not been updated yet (For
  * DataSet 0,
  ** this gets updated after every single transaction - to avoid the problem of
  * effect of
  ** transaction being applied to holdings without portmain reflecting it).
  */
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0 && !bRollDateUpdated)
    zErr = UpdateRollDate(pzPR->iID, lRollDate, lSrceLastTransNo, lDestDate);

  return zErr;
} // SettlementDateRollAnAccount

/*
** Function that does actual rolling.
*
ERRSTRUCT RollAnAccount(PORTMAIN *pzPR, char *sSecNo, char *sWi, BOOL
bPartialRoll, char *sNumericFlag, long lNumericId1, long lNumericId2, BOOL
bInitDataSet, long lRollDate, long lSrceDate, long lDestDate, long
lTransStartDate, long lTransEndDate, BOOL bPortmainAlreadySelected, BOOL
bMultipleAccnts,BOOL iResetPerfDate,BOOL bRollFromCurrent)
{
  static int  iLastID = 0;

  ERRSTRUCT   zErr;
  long        lSrceLastTransNo;
  BOOL        bBackwardRoll, bSpecificSecNo, bInitPmainPbal;
  TRANS       zTR;
  long		  lLastMonthEnd;

  lpprInitializeErrStruct(&zErr);
        lLastMonthEnd = lpfnLastMonthEnd(lRollDate);

  if (sSecNo[0] == '\0')
  {
    bSpecificSecNo = FALSE;
    bInitPmainPbal = TRUE;
  }
  else
  {
    bSpecificSecNo = TRUE;
    if (pzPR->iID == iLastID)
      bInitPmainPbal = FALSE;
    else
      bInitPmainPbal = TRUE;
  }

  if (!bPortmainAlreadySelected)
  {
    lpprSelectPortmain(pzPR, pzPR->iID, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } // No need to select portmain

  / *
  ** If doing roll for multiple accounts and the account is deleted/purged/did
  ** not exist on the roll date then no need to send an error message, else send
an error.
  * /
  if (bMultipleAccnts)
  {
    if ((pzPR->lDeleteDate != 0 && lRollDate > pzPR->lDeleteDate) ||
        lRollDate < pzPR->lInceptionDate || lRollDate < pzPR->lPurgeDate)
      return zErr;
  } // multiple accounts
  else
  {
    // Can't roll a portfolio on a date after its delete date
    if (pzPR->lDeleteDate != 0 && lRollDate > pzPR->lDeleteDate)
      return(lpfnPrintError("Portfolio Has Been Deleted Before The Roll Date",
                                                                                                    pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT1", FALSE));

    // Can't roll a portfolio before its inception date
    if (lRollDate < pzPR->lInceptionDate)
      return(lpfnPrintError("Portfolio Didn't Exist On The Roll Date",
pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT2", FALSE));

    // Can't roll a portfolio before its purge date
    if (lRollDate < pzPR->lPurgeDate)
      return(lpfnPrintError("Portfolio Has Been Purged. No Transactions Going
Back That Far", pzPR->iID, 0, "", 604, 0, 0, "ROLL ROLLACCT3", FALSE));
        }// Single Account

        / *
        ** if any of the numeric flag is being used, then clean the destination
        ** (there is no source, nothing to copy) and process all the
transactions.
        * /
  if (sNumericFlag[0] != '\0')
  {
                if (sNumericFlag != "T" && sNumericFlag != "TD" && sNumericFlag
!= "ED" &&sNumericFlag != "ND") return(lpfnPrintError("Invalid Numeric Flag",
pzPR->iID, 0, "", 601, 0, 0, "ROLL ROLLACCT4", FALSE));

    / *
    ** If bInitDataSet is TRUE and rolling for a single account, then delete
    ** destination, but there is no source to copy to destination, so pass the
    ** source date as -1 to initialize destination function.
    * /
    if (bInitDataSet && !bMultipleAccnts)
    {
      zErr = AccntDestInit(pzPR->iID, sSecNo, sWi, -1, lDestDate, TRUE);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                          return zErr;
      iLastID = pzPR->iID;
    } // if bInitDataSet is TRUE

    while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
    {
      if (strcmp(sNumericFlag, "T") == 0)
        lpfnTransNoTrans(pzPR->iID, lNumericId1, lNumericId2, sSecNo, sWi,
bSpecificSecNo, &zTR, &zErr); else if (strcmp(sNumericFlag, "TD") == 0)
        lpfnTradeDateTrans(pzPR->iID, lNumericId1, lNumericId2, sSecNo,sWi,
bSpecificSecNo, &zTR, &zErr); else if (strcmp(sNumericFlag, "ED") == 0)
        lpfnEffectDateTrans(pzPR->iID, lNumericId1, lNumericId2, sSecNo,sWi,
bSpecificSecNo, &zTR, &zErr); else lpfnEntryDateTrans(pzPR->iID, lNumericId1,
lNumericId2, sSecNo,sWi, bSpecificSecNo, &zTR, &zErr); if (zErr.iSqlError ==
SQLNOTFOUND)
      {
        zErr.iSqlError = 0;
        break;
      }
      else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;


          // Send the transaction to updatehold for processing
      zErr = CallUpdateHold(zTR, *pzPR, FALSE, &lSrceLastTransNo);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } // while no error

    return zErr;
  } // If numeric flag is being used

  / *
  ** If control comes here, it means numeric flag is not being used.
  ** If doing multiple accounts roll, source date must have already been
  ** set, else if destination date is same as roll date then source date is the
  ** roll date, else find out the source(base) data which we are going to use.
  * /
  if (bMultipleAccnts)
  {
    zErr = GetLastTransNo(pzPR->iID, lSrceDate, &lSrceLastTransNo, TRUE);
    if (zErr.iSqlError == SQLNOTFOUND)
    {
      zErr.iSqlError = 0;
      lSrceLastTransNo = LASTTRANSNO;
    }
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }
  else // single account
  {
    if (lDestDate == lRollDate && bRollFromCurrent == FALSE)
    {
      lSrceDate = lRollDate;
      zErr = GetLastTransNo(pzPR->iID, lSrceDate, &lSrceLastTransNo, TRUE);
      if (zErr.iSqlError == SQLNOTFOUND)
      {
        zErr.iSqlError = 0;
        lSrceLastTransNo = LASTTRANSNO;
      }
      else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
                        lTransStartDate = lTransEndDate = lRollDate;
    }
    else
    {
      zErr = FindOutSourceDataSet(*pzPR, sSecNo, sWi, lRollDate, &lSrceDate,
&lSrceLastTransNo, &lTransStartDate, &lTransEndDate, bMultipleAccnts,
bRollFromCurrent); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) return
zErr; } // if destination date is not roll date } // Single account roll

  // If srce date <= destination date then a forward roll, else backward
  if (lSrceDate <= lRollDate)
    bBackwardRoll = FALSE;
  else
    bBackwardRoll = TRUE;

  / *
  ** If a full roll for a single account, destination data set initialization
  ** (deletion of old data and copying source over it) is required. For a
  ** partial roll, initialize only if user wants to(InitDataSet is TRUE). If
  ** source and destination are same, no initialization is needed.
  * /
  if (!bMultipleAccnts && lSrceDate != lDestDate && (!bPartialRoll ||
bInitDataSet))
  {
    zErr = AccntDestInit(pzPR->iID, sSecNo, sWi, lSrceDate, lDestDate,
bInitPmainPbal); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) return
zErr;

    iLastID = pzPR->iID;
  }  // if destinatin needs to be initialized for a single account

        // process all asof transactions
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
  {
    lpfnAsofTrans(pzPR->iID, lSrceDate, lSrceLastTransNo, sSecNo,
sWi,bSpecificSecNo, &zTR, &zErr); if (zErr.iSqlError == SQLNOTFOUND)
    {
      zErr.iSqlError = 0;
      break;
    }
    else if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;

                zErr = CallUpdateHold(zTR, *pzPR, FALSE, &lSrceLastTransNo,
iWhichDataSet); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) return
zErr;

  } // while no error in processing asof transactions

        // process all regular transactions
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
  {
    if (bBackwardRoll)
      lpfnBackwardTrans(pzPR->iID, lTransStartDate, lTransEndDate, sSecNo, sWi,
bSpecificSecNo, &zTR, &zErr); else lpfnForwardTrans(pzPR->iID, lTransStartDate,
lTransEndDate, sSecNo, sWi, bSpecificSecNo, &zTR, &zErr); if (zErr.iSqlError ==
SQLNOTFOUND)
    {
      zErr.iSqlError = 0;
      break;
    }
    else if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;

    zErr = CallUpdateHold(zTR, *pzPR, bBackwardRoll, &lSrceLastTransNo,
iWhichDataSet); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) return
zErr;

  } // loop for regular transactions

        / *
        ** update the roll date in the destination portmain if rolling to adhoc
tables (For DataSet 0,
  ** this gets updated after every single transaction - to avoid the problem of
effect of transaction
        ** being applied to holdings without portmain reflecting it. For DataSet
2(performance) - no need
        ** to update dates).
        * /
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0 && iWhichDataSet == 1)
    zErr = UpdateRollDate(pzPR->iID, lRollDate, lSrceLastTransNo, lDestDate);

        // Reset perf_date for all asof transactions
        if ((iResetPerfDate) && zErr.iSqlError == 0 && zErr.iBusinessError == 0)
                lpprUpdatePerfDate(pzPR->iID, lLastMonthEnd, lRollDate, &zErr);

  return zErr;
} // RollAnAccount */

/*F*
** Function that calls UpdateHold and updates lasttransno, if required
*F*/
ERRSTRUCT CallUpdateHold(TRANS zTR, PORTMAIN zPR, BOOL bReversal,
                         long *plLastTransNo, int iWhichDataSet, long lRollDate,
                         long lDestinationDate, BOOL *pbRollDateUpdated) {
  ERRSTRUCT zErr;
  DTRANSDESC zTempTrnDesc, zDTranDesc[1];

  lpprInitializeErrStruct(&zErr);

  /*
  ** When rolling forward, transactions are sent to upd_hold library as they
  ** are. When rolling backward, as-of transactions, if any, are sent as they
  ** are and regular transactions are sent as reversals.
  */
  if (bReversal) {
    if (strcmp(zTR.sTranType, "RV") == 0) // if reversing a reversal
    {
      strcpy_s(zTR.sTranType, zTR.sRevType);
      strcpy_s(zTR.sRevType, "RV");
      zTR.lTransNo = zTR.lRevTransNo;
    } else {
      strcpy_s(zTR.sRevType, zTR.sTranType);
      strcpy_s(zTR.sTranType, "RV");
      zTR.lRevTransNo = zTR.lTransNo;
    }
  }

  if (strcmp(zTR.sAcctMthd, "V") == 0) {
    while (!zErr.iSqlError) {
      lpprSelectTrndesc(&zTempTrnDesc, zTR.iID, zTR.lTransNo, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        lpprInitializeErrStruct(&zErr);
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      if (strcmp(zTempTrnDesc.sCloseType, "VS") == 0)
        zDTranDesc[0] =
            zTempTrnDesc; // even though we found the record we were looking
                          // for, continue until SQLNOTFOUND, so that the query
                          // can be closed properly
    } // while no error
  } // if versus trade

  // Call update hold and make sure (6th flag - TRUE) update hold starts and
  // commit/rollback the effect of that transaction depending on if there was no
  // error/error.
  zErr = lpfnUpdateHold(zTR, zPR, zDTranDesc, 0, NULL, const_cast<char *>("P"),
                        FALSE);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // If rolling to an existing(monthend) table set, update lasttransno (if
  // grearter than that in appropriate portmain)
  if (iWhichDataSet == 0 && zTR.lTransNo > *plLastTransNo) {
    zErr = UpdateRollDate(zPR.iID, lRollDate, zTR.lTransNo, lDestinationDate);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    *pbRollDateUpdated = TRUE;
  } // if rolling to an existing (monthend) table set

  if (*plLastTransNo < zTR.lTransNo)
    *plLastTransNo = zTR.lTransNo;

  return zErr;
} /* CallUpdateHold */

/*
** Function to read last_trans_no from a portdir table. Although, this function
** can be used to read record even from the current portdir, generally, it is
** used to read record from a prior portdir. The name of the file is read from
** holdmap table.
*/
ERRSTRUCT GetLastTransNo(int iID, long lDate, long *plLastTransNo,
                         BOOL bIsNotFoundAnError) {
  ERRSTRUCT zErr;
  char sTemp[STR80LEN], sPortmain[STR80LEN];

  lpprInitializeErrStruct(&zErr);

  if (lDate == PERFORM_DATE)
    strcpy_s(sPortmain, "pmperf");
  else if (lDate == SETTLEMENT_DATE)
    strcpy_s(sPortmain, "pmstlmnt");
  else {
    zErr = ReadHoldingsMapForADate(lDate, sTemp, sTemp, sTemp, sPortmain, sTemp,
                                   sTemp, sTemp, FALSE);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  lpprSrcePortmain(iID, sPortmain, plLastTransNo, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError) {
    if (zErr.iSqlError == SQLNOTFOUND && !bIsNotFoundAnError)
      ;
    else
      zErr = lpfnPrintError("Error Fetching Portmain cursor", 0, 0, "", 0,
                            zErr.iSqlError, zErr.iIsamCode,
                            "ROLL GETLASTTRANS2", FALSE);
  }

  return zErr;
} /* getlasttransno */

/*
** Function to figure out destination data set. If the user has asked for ADHOC,
* Performance or
** settlement data set then that's the data set they get, else this function
* will check holdmap
** table, if a record exists for the requested date (and data type is not 'C'
* (current)) that is
** the destination data set else adhoc data set is the destination(if even that
* does not exist,
** it's an error).
*/
ERRSTRUCT FindOutDestinationDataSet(long lRollDate, int iWhichDataSet,
                                    BOOL bPartialRoll, long *plDestDate) {
  ERRSTRUCT zErr;
  char sTemp[STR80LEN], sHoldings[STR80LEN];

  lpprInitializeErrStruct(&zErr);

  if (iWhichDataSet == 2) {
    *plDestDate = PERFORM_DATE;
    return zErr;
  } else if (iWhichDataSet == 3) {
    *plDestDate = SETTLEMENT_DATE;
    return zErr;
  } else if (bPartialRoll || iWhichDataSet == 1)
    *plDestDate = ADHOC_DATE;
  else
    *plDestDate = lRollDate;

  zErr = ReadHoldingsMapForADate(*plDestDate, sHoldings, sTemp, sTemp, sTemp,
                                 sTemp, sTemp, sTemp, FALSE);
  if (zErr.iSqlError == SQLNOTFOUND || _stricmp(sHoldings, "holdings") == 0) {
    /*
    ** If the passed date was the ADHOC date, return with an error, else read
    ** holdmap again for the ADHOC_DATE.
    */
    if (*plDestDate == ADHOC_DATE)
      return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0, 0,
                             "ROLL FINDOUTDESTINATION1", FALSE));
    else {
      *plDestDate = ADHOC_DATE;
      zErr = ReadHoldingsMapForADate(*plDestDate, sTemp, sTemp, sTemp, sTemp,
                                     sTemp, sTemp, sTemp, FALSE);
      if (zErr.iSqlError == SQLNOTFOUND)
        return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0, 0,
                               "ROLL FINDOUTDESTINATION2", FALSE));
      else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* if passed date was not the ad hoc date */
  } /* if record not found for the passed date */

  return zErr;
} // FindoutDestinationDataSet

long GetRollDate(long iID, char *pName, long lDefEndDate) {
  ERRSTRUCT zErr;
  long lRolDate = 0;
  lpprSelectRollDate(iID, pName, &lRolDate, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    lRolDate = lDefEndDate;
  return lRolDate;
}

/*
** Function to figure out source data set. This is done by examining two data
** sets closest to the data set on roll date and checking which one will need
** less number of transactions to process.
*/
ERRSTRUCT FindOutSourceDataSet(PORTMAIN zPR, char *sSecNo, char *sWi,
                               long lRollDate, long *plSourceDate,
                               long *plSourceLastTransNo,
                               long *plTransStartDate, long *plTransEndDate,
                               BOOL bMultipleAccnts, BOOL bRollFromCurrent,
                               BOOL bRollFromInception, BOOL *bInitDataSet,
                               int iWhichDataSet) {
  ERRSTRUCT zErr;
  BOOL bSpecificSecNo, bSourceFound;
  char sTemp[STR80LEN], sDataType[2];
  long lAsofDate, lStartDate, lEndDate, lStartTransNo, lEndTransNo;
  int iRegForwardCount, iRegBackwardCount, iAsofForwardCount,
      iAsofBackwardCount;
  BOOL bIsStartCurrent, bIsEndCurrent, bIsSourceCurrent;
  long lTempMaxDate = MAXDATE; // to use in IF conditions

  lpprInitializeErrStruct(&zErr);
  *plSourceLastTransNo = LASTTRANSNO;
  bSourceFound = bIsStartCurrent = bIsEndCurrent = bIsSourceCurrent = FALSE;

  if (sSecNo[0] == '\0')
    bSpecificSecNo = FALSE;
  else
    bSpecificSecNo = TRUE;

  /*
  ** Find the latest date which is less than the roll date(start date) and
  ** the earliest date which is greater than the the roll date(end date).
  */
  lStartDate = lStartTransNo = lEndTransNo = -1;
  lEndDate = MAXDATE; /* 12/31/2078 */
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    lpprSelectAllHoldmap(sTemp, sTemp, sTemp, sTemp, sTemp, sTemp, sTemp,
                         sDataType, &lAsofDate, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Reading Holdmap", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode, "ROLL FINDSOURCE1",
                             FALSE));

    // Data in adhoc and performance tables are temporary, cann't use them as
    // source
    if (lAsofDate == ADHOC_DATE || lAsofDate == PERFORM_DATE)
      continue;

    /* If running for single account, make sure date is >= incept date */
    if (!bMultipleAccnts && lAsofDate < zPR.lInceptionDate)
      continue;

    if (bRollFromCurrent == TRUE && (strcmp(sDataType, "C") != 0))
      continue;

    if (lAsofDate <= lRollDate && lAsofDate > lStartDate) {
      lStartDate = lAsofDate;
      if (strcmp(sDataType, "C") == 0)
        bIsStartCurrent = TRUE;
      else
        bIsStartCurrent = FALSE;
    } else if (lAsofDate >= lRollDate && lAsofDate < lEndDate) {
      lEndDate = lAsofDate;
      if (strcmp(sDataType, "C") == 0)
        bIsEndCurrent = TRUE;
      else
        bIsEndCurrent = FALSE;
    }
  } /* while no error */

  /*
  ** If no record is found in the holdmap which is less than the roll date
  ** and no record is found which is greater than roll date, we can not do
  ** anything.
  */
  if (lStartDate == -1 && lEndDate == lTempMaxDate)
    return (lpfnPrintError("No Source Data to Be Considered As Base Point", 0,
                           0, "", 603, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL FINDSOURCE2", FALSE));

  if (lRollDate == lStartDate || bRollFromInception) {
    bSourceFound = TRUE;
    *plSourceDate = lStartDate;
    bIsSourceCurrent = bIsStartCurrent;
  } else if (lRollDate == lEndDate) {
    bSourceFound = TRUE;
    *plSourceDate = lEndDate;
    bIsSourceCurrent = bIsEndCurrent;
  }

  /*
  ** If we are doing a roll for more than one account, select the date closest
  ** to roll date as the source date, else select the date which will result
  ** in the minimum number of transactions being applied for the account.
  */
  if (bMultipleAccnts) {
    if (!bSourceFound) {
      if (lStartDate == -1 ||
          (lEndDate - lRollDate <= lRollDate - lStartDate)) {
        *plSourceDate = lEndDate;
        bIsSourceCurrent = bIsEndCurrent;
      } else {
        *plSourceDate = lStartDate;
        bIsSourceCurrent = bIsStartCurrent;
      }
    } // soruce not found

    zErr = GetTransactionDateRange(lRollDate, *plSourceDate, bIsSourceCurrent,
                                   plTransStartDate, plTransEndDate);
    return zErr;
  } /* if multiple accounts are being rolled */
  else if (bSourceFound) {
    if (bRollFromInception) {
      *plTransStartDate = zPR.lInceptionDate;
      *plTransEndDate = lRollDate;
      *plSourceLastTransNo = 0;
    } else {
      zErr = GetLastTransNo(zPR.iID, *plSourceDate, &lStartTransNo, FALSE);
      if (zErr.iSqlError == SQLNOTFOUND)
        zErr.iSqlError = 0;
      else
        *plSourceLastTransNo = lStartTransNo;

      *plTransStartDate = lRollDate;
      *plTransEndDate = lRollDate;
    }

    return zErr;
  }

  /*
  ** If no record was found in holdmap file for a date less than the roll
  ** date, source is the end date and we have to do a backward roll, otherwise
  ** read last transno for the account on the start date.
  */
  if (lStartDate == -1) {
    zErr = GetLastTransNo(zPR.iID, lEndDate, plSourceLastTransNo, FALSE);
    *bInitDataSet = FALSE;
    if (bRollFromCurrent || bRollFromInception)
      *plSourceDate = lEndDate;
    else {
      /* that code for Roll from performance*/
      if (iWhichDataSet == 2)
        *plSourceDate =
            GetRollDate(zPR.iID, const_cast<char *>("pmperf"), lEndDate);
      else
        *plSourceDate = lEndDate;
    }
    if (*plSourceDate > lRollDate) {
      *plSourceDate = lEndDate;
      *bInitDataSet = TRUE;
    }
    if ((zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND) ||
        zErr.iBusinessError != 0)
      return zErr;

    zErr = GetTransactionDateRange(lRollDate, *plSourceDate, bIsEndCurrent,
                                   plTransStartDate, plTransEndDate);
    return zErr;
  } else {
    zErr = GetLastTransNo(zPR.iID, lStartDate, &lStartTransNo, FALSE);
    if (zErr.iSqlError == SQLNOTFOUND) {
      *plSourceDate = lEndDate;
      zErr = GetTransactionDateRange(lRollDate, *plSourceDate, bIsStartCurrent,
                                     plTransStartDate, plTransEndDate);
      return zErr;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } // valid start date

  /*
  ** If no record was found in holdmap file for a date greater than the
  ** roll date, source is the start date and we have to do a forward roll
  */
  if (lEndDate == lTempMaxDate) {
    *plSourceDate = lStartDate;
    zErr = GetLastTransNo(zPR.iID, *plSourceDate, plSourceLastTransNo, FALSE);
    if ((zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND) ||
        zErr.iBusinessError != 0)
      return zErr;

    zErr = GetTransactionDateRange(lRollDate, *plSourceDate, bIsStartCurrent,
                                   plTransStartDate, plTransEndDate);
    return zErr;
  }

  if (lEndTransNo == -1) {
    zErr = GetLastTransNo(zPR.iID, lEndDate, &lEndTransNo, FALSE);
    if (zErr.iSqlError == SQLNOTFOUND) {
      *plSourceDate = lStartDate;
      zErr = GetTransactionDateRange(lRollDate, *plSourceDate, bIsStartCurrent,
                                     plTransStartDate, plTransEndDate);

      return zErr;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  iRegForwardCount = iRegBackwardCount = iAsofForwardCount =
      iAsofBackwardCount = 0;
  /*
  ** Now get the count of transaction, we will have to process if we do a
  ** forward roll, starting with data set on startDate as the base data. First
  ** condition on trade_date and transno will give any asofs, after that data
  ** set was created and second condition on trade date will give actual trans
  ** between start date(excluded) and roll date(included).
  ** First temporarily set the *plStartEffDate and *plEndEffDate to the
  * appropriate
  ** dates as if we are doing a forward roll. And then reset these variables as
  * if we
  ** are doing a backward roll.
  */
  zErr = GetTransactionDateRange(lRollDate, lStartDate, bIsStartCurrent,
                                 plTransStartDate, plTransEndDate);
  // RegTransCount "Forward"
  lpprRegTransCount(zPR.iID, *plTransStartDate, *plTransEndDate, sSecNo, sWi,
                    bSpecificSecNo, &iRegForwardCount, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Reading Regular Forward Count From Trans", 0,
                           0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL FINDSOURCE3A", FALSE));

  // AsofTransCount "Forward"
  lpprAsofTransCount(zPR.iID, lStartDate, lStartTransNo, sSecNo, sWi,
                     bSpecificSecNo, &iAsofForwardCount, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Reading Asof Forward Count From Trans", 0, 0,
                           "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL FINDSOURCE4A", FALSE));

  zErr = GetTransactionDateRange(lRollDate, lEndDate, bIsEndCurrent,
                                 plTransStartDate, plTransEndDate);
  // RegTransCount "Backward"
  lpprRegTransCount(zPR.iID, *plTransStartDate, *plTransEndDate, sSecNo, sWi,
                    bSpecificSecNo, &iRegBackwardCount, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Reading Regular Backward Count From Trans", 0,
                           0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL FINDSOURCE5A", FALSE));

  // AsofTransCount "Backward"
  lpprAsofTransCount(zPR.iID, *plTransStartDate, *plTransEndDate, sSecNo, sWi,
                     bSpecificSecNo, &iAsofBackwardCount, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Reading Asof Backward Count From Trans", 0, 0,
                           "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL FINDSOURCE6A", FALSE));

  /*
  ** If iBackwardCount < iForwardCount then BackwardRoll will be done on the
  ** data set on End Date, by reversing transactions between roll date and
  ** end date, else a forward roll wll be done on the data set on start date,
  ** by applying transactions bewteen Start Date and Roll Date.
  */
  if (iRegBackwardCount + iAsofBackwardCount <
      iRegForwardCount + iAsofForwardCount) {
    *plSourceDate = lEndDate;
    *plSourceLastTransNo = lEndTransNo;
    bIsSourceCurrent = bIsEndCurrent;
  } // Backward Roll
  else {
    *plSourceDate = lStartDate;
    *plSourceLastTransNo = lStartTransNo;
    bIsSourceCurrent = bIsStartCurrent;
  } //

  zErr = GetTransactionDateRange(lRollDate, *plSourceDate, bIsSourceCurrent,
                                 plTransStartDate, plTransEndDate);

  return zErr;
} /* findsourcedataset */

/*
** This function is called from findsourcedataset function to figures out which
* transaction
** set(based on eff_date range) to use based on roll date and source date. When
* this function
** is called source date has already been identified and also it has been
* determined
** whether the source is current or not. The asof_date for the
* "current"(data_type = "C")
** holdings set is the pricing date (which is the last business date). Holdings
* on that date
** will have trades with effective date later than asof_date, so if current is
* used as source,
** we have to get trade_date from starsdate and use that date instead of source
* date.
*/
ERRSTRUCT GetTransactionDateRange(long lRollDate, long lSourceDate,
                                  BOOL bIsSourceCurrent, long *plTransStartDate,
                                  long *plTransEndDate) {
  ERRSTRUCT zErr;
  long lTradeDate, lPricingDate;

  lpprInitializeErrStruct(&zErr);

  lpprSelectStarsDate(&lTradeDate, &lPricingDate, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  if (lSourceDate <= lRollDate) // forward roll
  {
    if (bIsSourceCurrent) // this is very unlikely, rolling a portfolio to a
                          // date greater than current
      *plTransStartDate = lTradeDate;
    else
      *plTransStartDate = lSourceDate;
    *plTransEndDate = lRollDate;
  } else // backward roll
  {
    *plTransStartDate = lRollDate;
    if (bIsSourceCurrent)
      *plTransEndDate = lTradeDate;
    else
      *plTransEndDate = lSourceDate;
  } // backward roll

  return zErr;
} // GetTransactionDateRange

/*
** Function to update roll_date in the portdir table.
*/
ERRSTRUCT UpdateRollDate(int iID, long lRollDate, long lLastTransNo,
                         long lDestDate) {
  ERRSTRUCT zErr;
  char sPortmain[STR80LEN], sTemp[STR80LEN];

  lpprInitializeErrStruct(&zErr);
  if (lDestDate == PERFORM_DATE)
    strcpy_s(sPortmain, "pmperf");
  else if (lDestDate == SETTLEMENT_DATE)
    strcpy_s(sPortmain, "pmstlmnt");
  else {
    zErr = ReadHoldingsMapForADate(lDestDate, sTemp, sTemp, sTemp, sPortmain,
                                   sTemp, sTemp, sTemp, FALSE);
    if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;
  }
  if (lLastTransNo > 0 && lLastTransNo != LASTTRANSNO)
    lpprDestUpd1Portmain(iID, lRollDate, lLastTransNo, sPortmain, &zErr);
  else
    lpprDestUpd2Portmain(iID, lRollDate, sPortmain, &zErr);

  return zErr;
} /* Updaterolldate */

/*
** Function to prepare the destination data set for an account by copying source
** over it. If InitPortmainPbal is TRUE only then portdir and portbal are
* deleted.
** This option is used when an account is rolled for a list of securities,
** then, the first time this function is called portdir and portbal records for
** the account are deleted(and reinserted from the source) and at all subsequent
** times, they are skipped.
*/
ERRSTRUCT AccntDestInit(int iID, char *sSecNo, char *sWi, long lSrcDate,
                        long lDestDate, BOOL bInitPmainPbal,
                        BOOL bRollFromInception) {
  ERRSTRUCT zErr;
  BOOL bSrcExist, bSecSpecific, bSecInHoldings;
  char sHoldings1[STR80LEN], sHoldcash1[STR80LEN], sPortbal1[STR80LEN],
      sPortmain1[STR80LEN], sHedgeXref1[STR80LEN], sPayrec1[STR80LEN],
      sHoldtot1[STR80LEN];
  char sHoldings2[STR80LEN], sHoldcash2[STR80LEN], sPortbal2[STR80LEN],
      sPortmain2[STR80LEN], sHedgeXref2[STR80LEN], sPayrec2[STR80LEN],
      sHoldtot2[STR80LEN];

  static BOOL bLastSecSpecific, bDestInit, bSrceInit = FALSE;
  static BOOL bLastSpecificSecNo;
  static long lLastDest = -100;
  static long lLastSrce = -100;

  lpprInitializeErrStruct(&zErr);
  bSecInHoldings = FALSE;
  /* If source and destination are same, return without doing anything */
  if (lSrcDate == lDestDate && bRollFromInception == FALSE)
    return zErr;

  /*
   ** If Passed SecNo is not null, initialization is being done for a security.
   ** If initialization is done for a specific security, find out if the
   * security
   ** is in holdings (or holdcash).
   */
  if (sSecNo[0] == '\0')
    bSecSpecific = FALSE;
  else {
    bSecSpecific = TRUE;
    zErr = IsSecurityInHoldings(sSecNo, sWi, &bSecInHoldings);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  } /* Init for a specific security */

  /*
  ** If destination has been initialized in the past for different conditions
  ** (destination date is different or whether it is for a specific security
  ** or not is different), unprepare (previously prepared)destination queries.
  */
  if (bDestInit) {
    if (lLastDest != lDestDate || bLastSecSpecific != bSecSpecific)
      lpfnUnprepareRollQueries(const_cast<char *>(""),
                               UNPREP_ACCNTDESTINATIONQUERIES);

    bDestInit = FALSE;
  }

  if (lDestDate == PERFORM_DATE) {
    strcpy_s(sHoldings1, "hdperf");
    strcpy_s(sHoldcash1, "hcperf");
    strcpy_s(sPortbal1, "pbperf");
    strcpy_s(sPortmain1, "pmperf");
    strcpy_s(sHedgeXref1, "hxperf");
    strcpy_s(sPayrec1, "prperf");
    strcpy_s(sHoldtot1, "hdadhoc");
  } else if (lDestDate == SETTLEMENT_DATE) {
    strcpy_s(sHoldings1, "hdstlmnt");
    strcpy_s(sHoldcash1, "hcstlmnt");
    strcpy_s(sPortbal1, "pbstlmnt");
    strcpy_s(sPortmain1, "pmstlmnt");
    strcpy_s(sHedgeXref1, "hxstlmnt");
    strcpy_s(sPayrec1, "prstlmnt");
    strcpy_s(sHoldtot1, "htstlmnt");
  } else {
    // Destination should never be holdings itself.
    zErr = ReadHoldingsMapForADate(lDestDate, sHoldings1, sHoldcash1, sPortbal1,
                                   sPortmain1, sHedgeXref1, sPayrec1, sHoldtot1,
                                   TRUE);
    if (zErr.iSqlError == SQLNOTFOUND ||
        _stricmp(sHoldings1, "holdings") == 0) {
      /*
      ** If the passed date was the ADHOC date, return with an error, else read
      ** holdmap again for the ADHOC_DATE.
      */
      if (lDestDate == ADHOC_DATE)
        return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0, 0,
                               "ROLL ACCNTDEL1", FALSE));
      else {
        lDestDate = ADHOC_DATE;
        zErr = ReadHoldingsMapForADate(lDestDate, sHoldings1, sHoldcash1,
                                       sPortbal1, sPortmain1, sHedgeXref1,
                                       sPayrec1, sHoldtot1, TRUE);
        if (zErr.iSqlError == SQLNOTFOUND)
          return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0,
                                 0, "ROLL ACCNTDEL2", FALSE));
        else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
      } /* if passed date was not the ad hoc date */
    } /* if record not found for the passed date */
  } /* if lDestDate is not PERFORM_DATE */
  zErr = CallInitTranProc(lDestDate, const_cast<char *>(""),
                          const_cast<char *>(""), const_cast<char *>(""),
                          const_cast<char *>(""));
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /*
   ** Delete records from all the destination tables. If initializing for a
   * specific
   ** security then delete records only for that security, else delete records
   ** for the whole account. Delete records from portdir and portbal only if
   ** InitPortmainPbal is TRUE.
   */
  if (bSecSpecific) {
    /*
    ** If doing security specific roll, security will either be in holdings or
    ** holdcash, but not in both the tables, so initialize only the table
    ** where the security is.
    */
    if (bSecInHoldings) {
      /* Delete Holdings */
      lpprAccountDeleteHoldings(iID, sSecNo, sWi, sHoldings1, bSecSpecific,
                                &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return zErr;
    } else {
      /* delete holdcash */
      lpprAccountDeleteHoldcash(iID, sSecNo, sWi, sHoldcash1, bSecSpecific,
                                &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return zErr;
    } /* Security in holdcash */
  } else {
    /* Delete Holdings */
    lpprAccountDeleteHoldings(iID, sSecNo, sWi, sHoldings1, bSecSpecific,
                              &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;

    /* Delete holdcash */
    lpprAccountDeleteHoldcash(iID, sSecNo, sWi, sHoldcash1, bSecSpecific,
                              &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;
  }

  /* delete hedgxref */
  lpprAccountDeleteHedgxref(iID, sSecNo, sWi, sHedgeXref1, bSecSpecific, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return zErr;

  /* delete payrec */
  lpprAccountDeletePayrec(iID, sSecNo, sWi, sPayrec1, bSecSpecific, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return zErr;

  /*
  ** SB 11/14/2012 - Earlier this function was not called when source and
  * destination dates were same but now, for
  ** roll from inception, this function may be called even when source and
  * destination are same (mainly to delete
  ** all data except portmain record). So, put this additional check to make
  * sure portmain record doesn't get
  ** deleted if soruce and destination are same.
  */
  if (bInitPmainPbal && lSrcDate != lDestDate) {

    /* delete portbal */
    /** Commenting out Portbal
       lpprAccountDeletePortbal(iID, sPortbal1, &zErr);
        if (zErr.iSqlError || zErr.iBusinessError)
          return zErr;
    **/
    /* delete portmain */
    lpprAccountDeletePortmain(iID, sPortmain1, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return zErr;
  } /* InitPortmain and pbal */
  bDestInit = TRUE;
  lLastDest = lDestDate;

  /*
  ** If source has been initialized in the past for different conditions
  ** (destination/source date is different or whether it is for a specific
  * security
  ** or not is different), unprepare (previously prepared)source queries.
  */
  if (bSrceInit) {
    if (lLastSrce != lSrcDate || lLastDest != lDestDate ||
        bLastSpecificSecNo != bSecSpecific)
      lpfnUnprepareRollQueries(const_cast<char *>(""),
                               UNPREP_ACCNTSOURCEQUERIES);

    bSrceInit = FALSE;
  } // if souce has been initialized

  /*
  ** Read the holdmap file for the source date, if no record is found, it is
  ** not an error, but nothing else to do, return.
  */
  zErr = ReadHoldingsMapForADate(lSrcDate, sHoldings2, sHoldcash2, sPortbal2,
                                 sPortmain2, sHedgeXref2, sPayrec2, sHoldtot2,
                                 FALSE);
  if (zErr.iSqlError == SQLNOTFOUND) {
    bSrcExist = FALSE;
    zErr.iSqlError = 0;
    return zErr;
  } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  else
    bSrcExist = TRUE;

  if (!bRollFromInception) {
    if (bSecSpecific) {
      if (bSecInHoldings) {
        /* Copy source holdings on to destination holdings */
        lpprAccountInsertHoldings(iID, sSecNo, sWi, sHoldings1, sHoldings2,
                                  bSecSpecific, &zErr);
        if (zErr.iSqlError || zErr.iBusinessError)
          return (lpfnPrintError("Error Inserting Record Into Holdings", 0, 0,
                                 "", 0, zErr.iSqlError, zErr.iIsamCode,
                                 "ROLL ACCNTINITDEST7.0", FALSE));
      } else {
        /* Copy source holdcash on to destination holdcash */
        lpprAccountInsertHoldcash(iID, sSecNo, sWi, sHoldcash1, sHoldcash2,
                                  bSecSpecific, &zErr);
        if (zErr.iSqlError || zErr.iBusinessError)
          return (lpfnPrintError("Error Inserting Record Into HoldCash", 0, 0,
                                 "", 0, zErr.iSqlError, zErr.iIsamCode,
                                 "ROLL ACCNTINITDEST8.0", FALSE));
      } /* In holdcash */
    } else {
      /* Copy source holdings on to destination holdings */
      lpprAccountInsertHoldings(iID, sSecNo, sWi, sHoldings1, sHoldings2,
                                bSecSpecific, &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Inserting Record Into Holdings", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL ACCNTINITDEST7.1", FALSE));

      /* Copy source holdcash on to destination holdcash */
      lpprAccountInsertHoldcash(iID, sSecNo, sWi, sHoldcash1, sHoldcash2,
                                bSecSpecific, &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Inserting Record Into HoldCash", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL ACCNTINITDEST8.1", FALSE));
    }

    /* Copy source hedgxref on to destination hedgxref */
    lpprAccountInsertHedgxref(iID, sSecNo, sWi, sHedgeXref1, sHedgeXref2,
                              bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL ACCNTINITDEST9.0", FALSE));

    /* Copy source payrec on to destination payrec */
    lpprAccountInsertPayrec(iID, sSecNo, sWi, sPayrec1, sPayrec2, bSecSpecific,
                            &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL ACCNTINITDEST10.0", FALSE));
  } // not roll from inception

  if (bInitPmainPbal) {
    /* Copy source portbal on to destination portbal */
    /** Commenting out portbal
                    lpprAccountInsertPortbal(iID, sPortbal1, sPortbal2, &zErr);
        if (zErr.iSqlError || zErr.iBusinessError)
          return(lpfnPrintError("Error Inserting Record Into Portbal", 0, 0, "",
                                                                                                                    0, zErr.iSqlError, zErr.iIsamCode,
                                                                                                                    "ROLL ACCNTINITDEST11", FALSE));
    **/
    /* Copy source portdir on to destination portdir */
    /*
    ** SB 11/14/2012 - Added this additional check to make sure portmain record
    * doesn't get inserted when it's
    ** already there (soruce and destination are same)
    */
    if (lSrcDate != lDestDate) {
      lpprAccountInsertPortmain(iID, sPortmain1, sPortmain2, &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Inserting Record Into Portdir", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL ACCNTINITDEST12", FALSE));
    }

    // if rolling from inception, set the last trans no in destination table to
    // 0.
    if (bRollFromInception) {
      lpprUpdatePortmainLastTransNo(iID, 0, &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Inserting Record Into Portdir", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL ACCNTINITDEST13", FALSE));
    } // if rolling from inception
  } /* InitPortmain and pbal */

  bSrceInit = TRUE;
  lLastSrce = lSrcDate;

  return zErr;
} /* AccntDestInit */

/*
** Function to figure out if the passed security is in Holdings table or
* Holdcash table.
*/
ERRSTRUCT IsSecurityInHoldings(char *sSecNo, char *sWi, BOOL *pbInHoldings) {
  ERRSTRUCT zErr;
  char sMsg[100];
  char sPositionInd[2];

  lpprInitializeErrStruct(&zErr);

  lpprSelectPositionIndicator(sSecNo, sWi, sPositionInd, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError) {
    sprintf_s(sMsg, "Error Reading Assets & SecTypes For Sec No - %s, Wi - %s",
              sSecNo, sWi);
    return (lpfnPrintError(sMsg, 0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL ASSETS", FALSE));
  }

  if (sPositionInd[0] == 'C') /* Hold cash */
    *pbInHoldings = FALSE;
  else
    *pbInHoldings = TRUE;

  return zErr;
} /* IsSecurityInHoldings */

/*
** Function to check if a holdmap record exist for the given date. This
** function is called with an option to save results(name of holdings, holdcash,
** portbal, etc). in static variables defined in the function, and if this
** function is called again for the same date, the saved results are returned,
** instead of reading them again from the table. This is done becuase name of
** the destination data set is required at many different places, so instead of
** passing all the names to different cursors again and again just a date is
** passed. This function is called many times with different dates but we don't
** want to sve the results every time. Since destination date remains same
** through out the program, first time this function is called with destination
** date, results should be saved, they should not be saved any other time.
*/
ERRSTRUCT ReadHoldingsMapForADate(long lDate, char *sHoldings, char *sHoldcash,
                                  char *sPortbal, char *sPortmain,
                                  char *sHedgxref, char *sPayrec,
                                  char *sHoldtot, BOOL bSaveResult) {
  ERRSTRUCT zErr;
  static long lResultDate = -100;
  static char sLastHoldings[STR80LEN], sLastHoldcash[STR80LEN],
      sLastPortbal[STR80LEN], sLastPortmain[STR80LEN];
  static char sLastHedgxref[STR80LEN], sLastPayrec[STR80LEN],
      sLastHoldtot[STR80LEN];

  lpprInitializeErrStruct(&zErr);

  if (lResultDate == lDate) {
    strcpy_s(sHoldings, STR80LEN, sLastHoldings);
    strcpy_s(sPayrec, STR80LEN, sLastPayrec);
    strcpy_s(sHoldcash, STR80LEN, sLastHoldcash);
    strcpy_s(sPortmain, STR80LEN, sLastPortmain);
    strcpy_s(sPortbal, STR80LEN, sLastPortbal);
    strcpy_s(sHedgxref, STR80LEN, sLastHedgxref);
    return zErr;
  }

  lpprSelectOneHoldmap(lDate, sHoldings, sHoldcash, sPortmain, sPortbal,
                       sPayrec, sHedgxref, sHoldtot, &zErr);
  if (zErr.iSqlError == SQLNOTFOUND)
    return zErr;
  else if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Reading Holdmap File", 0, 0, "", 0,
                           zErr.iSqlError, zErr.iIsamCode, "ROLL READHMAP",
                           FALSE));

  if (bSaveResult) {
    strcpy_s(sLastHoldings, sHoldings);
    strcpy_s(sLastHoldcash, sHoldcash);
    strcpy_s(sLastPortbal, sPortbal);
    strcpy_s(sLastPortmain, sPortmain);
    strcpy_s(sLastHedgxref, sHedgxref);
    strcpy_s(sLastPayrec, sPayrec);
    strcpy_s(sLastHoldtot, sHoldtot);
    lResultDate = lDate;
  }

  return zErr;
} /* ReadHoldingsMapForADate */

/*
** Function to initialize roll library. It loads all the dlls and the functions
** defined in them that are required anywhere in roll.
*/

DLLAPI ERRSTRUCT STDCALL WINAPI InitRoll(char *sDBPath, char *sType,
                                         char *sMode, long lAsofDate,
                                         char *sErrFile) {
  int iError;
  ERRSTRUCT zErr;
  memset(&zErr, 0, sizeof(ERRSTRUCT));

  static char sLastAlias[80] = "";
  static long lLastDate = -1;

  if (!bInit) {
    FILE *fDebug = fopen("C:\\Users\\Sergey\\.gemini\\roll_debug.log", "a");
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Starting initialization (bInit=FALSE)\n");
    }

    // Load "TransEngine" dll created in C
    hTEngineDll = LoadLibrarySafe("TransEngine.dll");
    if (hTEngineDll == NULL) {
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to load TransEngine.dll (Error %d)\n",
                GetLastError());
        fclose(fDebug);
      }
      zErr.iBusinessError = GetLastError();
      zErr.iSqlError = -1; // Explicitly set it so it's not garbage
      return zErr;
    }
    if (fDebug)
      fprintf(fDebug, "InitRoll: Loaded TransEngine.dll\n");

    // Load "StarsIO" dll created in Delphi
    // hOledbIODll = LoadLibrarySafe("StarsIO.dll");

    // Load "OLEDBIO" dll created in C - debug - vay
    hOledbIODll = LoadLibrarySafe("OLEDBIO.dll");
    if (hOledbIODll == NULL) {
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to load OLEDBIO.dll (Error %d)\n",
                GetLastError());
        fclose(fDebug);
      }
      zErr.iBusinessError = GetLastError();
      zErr.iSqlError = -1;
      return zErr;
    }
    if (fDebug) {
      fprintf(
          fDebug,
          "InitRoll: Loaded OLEDBIO.dll. Setting hStarsUtilsDll = NULL...\n");
      fflush(fDebug);
    }

    /* StarsUtils.dll is 32-bit Delphi. We now use OLEDBIO.dll which contains
     * 64-bit versions of these functions. */
    hStarsUtilsDll = NULL;

    if (fDebug) {
      fprintf(fDebug,
              "InitRoll: hStarsUtilsDll set to NULL. Finding "
              "InitializeErrStruct (hTEngineDll=%p)...\n",
              hTEngineDll);
      fflush(fDebug);
    }
    lpprInitializeErrStruct =
        (LPPRERRSTRUCT)GetProcAddress(hTEngineDll, "InitializeErrStruct");
    if (!lpprInitializeErrStruct) {
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to find InitializeErrStruct\n");
        fflush(fDebug);
        fclose(fDebug);
      }
      zErr.iBusinessError = GetLastError();
      zErr.iSqlError = -1;
      return zErr;
    }
    lpprInitializeErrStruct(&zErr);
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found InitializeErrStruct.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding InitializePortmainStruct...\n");
      fflush(fDebug);
    }
    lpprInitializePortmainStruct = (LPPRPMAINPOINTER)GetProcAddress(
        hTEngineDll, "InitializePortmainStruct");
    if (!lpprInitializePortmainStruct) {
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to find InitializePortmainStruct\n");
        fflush(fDebug);
        fclose(fDebug);
      }
      zErr.iBusinessError = GetLastError();
      zErr.iSqlError = -1;
      return zErr;
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found InitializePortmainStruct.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding PrintError...\n");
      fflush(fDebug);
    }
    lpfnPrintError = (LPFNPRINTERROR)GetProcAddress(hTEngineDll, "PrintError");
    if (!lpfnPrintError) {
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to find PrintError\n");
        fflush(fDebug);
        fclose(fDebug);
      }
      return zErr;
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found PrintError.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding InitTranProc...\n");
      fflush(fDebug);
    }
    lpfnInitTranProc =
        (LPFN1LONG3PCHAR1BOOL1PCHAR)GetProcAddress(hTEngineDll, "InitTranProc");
    if (!lpfnInitTranProc) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to find InitTranProc (Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading InitTranProc Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT2", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found InitTranProc.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding UpdateHold...\n");
      fflush(fDebug);
    }
    lpfnUpdateHold = (LPFNUPDATEHOLD)GetProcAddress(hTEngineDll, "UpdateHold");
    if (!lpfnUpdateHold) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to find UpdateHold (Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading UpdateHold Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT3", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found UpdateHold.\n");
      fflush(fDebug);
    }

    // Load functions from OLEDBIO.dll (replacing legacy StarsUtils.dll)
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding rmdyjul in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpfnrmdyjul = (LPFNRMDYJUL)GetProcAddress(hOledbIODll, "rmdyjul");
    if (!lpfnrmdyjul) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find rmdyjul in OLEDBIO (Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading rmdyjul Function from OLEDBIO", 0,
                             0, "", iError, 0, 0, "ROLL INIT4", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found rmdyjul.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding rjulmdy in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpfnrjulmdy = (LPFNRJULMDY)GetProcAddress(hOledbIODll, "rjulmdy");
    if (!lpfnrjulmdy) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find rjulmdy in OLEDBIO (Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading rjulmdy Function from OLEDBIO", 0,
                             0, "", iError, 0, 0, "ROLL INIT5", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found rjulmdy.\n");
      fflush(fDebug);
    }

    // Load functions from StarsIO.dll
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding StartDBTransaction in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpfnStartDBTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "StartDBTransaction");
    if (!lpfnStartDBTransaction) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find StartDBTransaction in OLEDBIO (Error "
                "%d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading StartDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "ROLL INIT6", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found StartDBTransaction.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding CommitDBTransaction in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpfnCommitDBTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "CommitDBTransaction");
    if (!lpfnCommitDBTransaction) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find CommitDBTransaction in OLEDBIO "
                "(Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading CommitDBTransaction Function", 0, 0,
                             "", iError, 0, 0, "ROLL INIT7", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found CommitDBTransaction.\n");
      fflush(fDebug);
    }

    if (fDebug) {
      fprintf(fDebug,
              "InitRoll: Finding RollbackDBTransaction in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpfnRollbackDBTransaction =
        (LPFNVOID)GetProcAddress(hOledbIODll, "RollbackDBTransaction");
    if (!lpfnRollbackDBTransaction) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find RollbackDBTransaction in OLEDBIO "
                "(Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading RollbackDBTransaction Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT8", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found RollbackDBTransaction.\n");
      fflush(fDebug);
    }

    lpfnGetTransCount = (LPFNVOID)GetProcAddress(hOledbIODll, "GetTransCount");
    if (!lpfnGetTransCount) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug, "InitRoll: Failed to find GetTransCount in OLEDBIO\n");
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading GetTransCount Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT8a", FALSE));
    }

    lpfnAbortDBTransaction =
        (LPFN1BOOL)GetProcAddress(hOledbIODll, "AbortDBTransaction");
    if (!lpfnAbortDBTransaction) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find AbortDBTransaction in OLEDBIO\n");
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading AbortTransaction Function", 0, 0,
                             "", iError, 0, 0, "ROLL INIT8b", FALSE));
    }

    lpfnUnprepareRollQueries =
        (LPFN1PCHAR1INT)GetProcAddress(hOledbIODll, "UnprepareRollQueries");
    if (!lpfnUnprepareRollQueries) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find UnprepareRollQueries in OLEDBIO\n");
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading UnprepareRollQueries Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT9", FALSE));
    }

    if (fDebug) {
      fprintf(fDebug, "InitRoll: Finding SelectPortmain in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpprSelectPortmain =
        (LPPRPORTMAIN)GetProcAddress(hOledbIODll, "SelectPortmain");
    if (!lpprSelectPortmain) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(
            fDebug,
            "InitRoll: Failed to find SelectPortmain in OLEDBIO (Error %d)\n",
            iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (lpfnPrintError("Error Loading SelectPortmain Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT10", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found SelectPortmain.\n");
      fflush(fDebug);
    }

    lpprAccountInsertHoldings = (LPPR1LONG4PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountInsertHoldings");
    if (!lpprAccountInsertHoldings) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountInsertHold Function", 0, 0,
                             "", iError, 0, 0, "ROLL INIT11", FALSE));
    }

    lpprAccountInsertHoldcash = (LPPR1LONG4PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountInsertHoldcash");
    if (!lpprAccountInsertHoldcash) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountInsertHoldcash Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT12", FALSE));
    }

    lpprAccountInsertHedgxref = (LPPR1LONG4PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountInsertHedgxref");
    if (!lpprAccountInsertHedgxref) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountInsertHedgxref Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT13", FALSE));
    }

    lpprAccountInsertPayrec = (LPPR1LONG4PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountInsertPayrec");
    if (!lpprAccountInsertPayrec) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountInsertPayrec Function", 0, 0,
                             "", iError, 0, 0, "ROLL INIT14", FALSE));
    }

    /*lpprAccountInsertPortbal = (LPPR1LONG2PCHAR)GetProcAddress(hOledbIODll,
    "AccountInsertPortbal"); if(!lpprAccountInsertPortbal)
    {
            iError = GetLastError();
      return(lpfnPrintError("Error Loading AccountInsertPortbal Function", 0, 0,
    "", iError, 0, 0, "ROLL INIT8", FALSE));
    }*/

    lpprAccountInsertPortmain =
        (LPPR1LONG2PCHAR)GetProcAddress(hOledbIODll, "AccountInsertPortmain");
    if (!lpprAccountInsertPortmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountInsertPortmain Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT15", FALSE));
    }

    lpprAccountDeleteHoldings = (LPPR1LONG3PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountDeleteHoldings");
    if (!lpprAccountDeleteHoldings) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountDeleteHoldings Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT16", FALSE));
    }

    lpprAccountDeleteHoldcash = (LPPR1LONG3PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountDeleteHoldcash");
    if (!lpprAccountDeleteHoldcash) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountDeleteHoldcash Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT17", FALSE));
    }

    lpprAccountDeleteHedgxref = (LPPR1LONG3PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountDeleteHedgxref");
    if (!lpprAccountDeleteHedgxref) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountDeleteHedgxref Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT18", FALSE));
    }

    lpprAccountDeletePayrec = (LPPR1LONG3PCHAR1BOOL)GetProcAddress(
        hOledbIODll, "AccountDeletePayrec");
    if (!lpprAccountDeletePayrec) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountDeletePayrec Function", 0, 0,
                             "", iError, 0, 0, "ROLL INIT19", FALSE));
    }

    if (fDebug) {
      fprintf(fDebug,
              "InitRoll: Finding UpdatePortmainLastTransNo in OLEDBIO...\n");
      fflush(fDebug);
    }
    lpprUpdatePortmainLastTransNo =
        (LPPR1INT1LONG)GetProcAddress(hOledbIODll, "UpdatePortmainLastTransNo");
    if (!lpprUpdatePortmainLastTransNo) {
      iError = GetLastError();
      if (fDebug) {
        fprintf(fDebug,
                "InitRoll: Failed to find UpdatePortmainLastTransNo in OLEDBIO "
                "(Error %d)\n",
                iError);
        fflush(fDebug);
        fclose(fDebug);
      }
      return (
          lpfnPrintError("Unable To Load UpdatePortmainLastTransNo function", 0,
                         0, "", iError, 0, 0, "ROLL INIT20", FALSE));
    }
    if (fDebug) {
      fprintf(fDebug, "InitRoll: Found UpdatePortmainLastTransNo.\n");
      fflush(fDebug);
    }

    /*lpprAccountDeletePortbal = (LPPR1LONG1PCHAR) GetProcAddress(hOledbIODll,
    "AccountDeletePortbal"); if(!lpprAccountDeletePortbal)
    {
            iError = GetLastError();
      return(lpfnPrintError("Error Loading AccountDeletePortbal Function", 0, 0,
    "", iError, 0, 0, "ROLL INIT8", FALSE));
    }*/

    lpprAccountDeletePortmain =
        (LPPR1LONG1PCHAR)GetProcAddress(hOledbIODll, "AccountDeletePortmain");
    if (!lpprAccountDeletePortmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AccountDeletePortmain Function", 0,
                             0, "", iError, 0, 0, "ROLL INIT20", FALSE));
    }

    lpprSelectAllHoldmap =
        (LPPR8PCHAR1PLONG)GetProcAddress(hOledbIODll, "ReadAllHoldmap");
    if (!lpprSelectAllHoldmap) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading FetchHMapCursor Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT21", FALSE));
    }

    lpprAsofTransCount = (LPPR3LONG2PCHAR1BOOL1PINT)GetProcAddress(
        hOledbIODll, "SelectAsofTransCount");
    if (!lpprAsofTransCount) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AsofTransCount", 0, 0, "", iError,
                             0, 0, "ROLL INIT22", FALSE));
    }

    lpprBackwardTrans = (LPPR3LONG2PCHAR1BOOL1PTRANS)GetProcAddress(
        hOledbIODll, "SelectBackwardTrans");
    if (!lpprBackwardTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading BackwardTrans Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT23", FALSE));
    }

    lpprForwardTrans = (LPPR3LONG2PCHAR1BOOL1PTRANS)GetProcAddress(
        hOledbIODll, "SelectForwardTrans");
    if (!lpprForwardTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading ForwardTrans Function", 0, 0, "",
                             iError, 0, 0, "ROLL INIT24", FALSE));
    }

    lpprRegTransCount = (LPPR3LONG2PCHAR1BOOL1PINT)GetProcAddress(
        hOledbIODll, "SelectRegularTransCount");
    if (!lpprRegTransCount) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading RegTransCount", 0, 0, "", iError, 0,
                             0, "ROLL INIT25", FALSE));
    }

    lpprDestUpd1Portmain = (LPPR3LONG1PCHAR)GetProcAddress(
        hOledbIODll, "UpdateRollDateAndLastTransNo");
    if (!lpprDestUpd1Portmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DestUpd1Portmain", 0, 0, "", iError,
                             0, 0, "ROLL INIT26", FALSE));
    }

    lpprDestUpd2Portmain =
        (LPPR2LONG1PCHAR)GetProcAddress(hOledbIODll, "UpdateRollDate");
    if (!lpprDestUpd2Portmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading DestUpd2Portmain", 0, 0, "", iError,
                             0, 0, "ROLL INIT27", FALSE));
    }

    lpprSelectOneHoldmap =
        (LPPR1LONG7PCHAR)GetProcAddress(hOledbIODll, "SelectHoldmap");
    if (!lpprSelectOneHoldmap) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectHoldmap", 0, 0, "", iError, 0,
                             0, "ROLL INIT28", FALSE));
    }

    lpprSelectStarsDate =
        (LPPR2PLONG)GetProcAddress(hOledbIODll, "SelectStarsDate");
    if (!lpprSelectStarsDate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectStarsDate", 0, 0, "", iError,
                             0, 0, "ROLL INIT29", FALSE));
    }

    lpprAsofTrans = (LPPR3LONG2PCHAR1BOOL1PTRANS)GetProcAddress(
        hOledbIODll, "SelectAsofTrans");
    if (!lpprAsofTrans) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading AsofTrans", 0, 0, "", iError, 0, 0,
                             "ROLL INIT30", FALSE));
    }

    lpprSrcePortmain =
        (LPPR1LONG1PCHAR1PLONG)GetProcAddress(hOledbIODll, "SelectLastTransNo");
    if (!lpprSrcePortmain) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SrcePortmain", 0, 0, "", iError, 0,
                             0, "ROLL INIT31", FALSE));
    }

    lpprSelectRollDate =
        (LPPR1LONG1PCHAR1PLONG)GetProcAddress(hOledbIODll, "SelectRollDate");
    if (!lpprSelectRollDate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SrcePortmain", 0, 0, "", iError, 0,
                             0, "ROLL INIT31A", FALSE));
    }

    lpprSelectPositionIndicator =
        (LPPR3PCHAR)GetProcAddress(hOledbIODll, "SelectPositionIndicator");
    if (!lpprSelectPositionIndicator) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectPositionIndicator", 0, 0, "",
                             iError, 0, 0, "ROLL INIT32", FALSE));
    }

    lpfnIsItAMonthEnd = (LPFN1LONG)GetProcAddress(hOledbIODll, "IsItAMonthEnd");
    if (!lpfnIsItAMonthEnd) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading IsItAMonthEnd from OLEDBIO", 0, 0,
                             "", iError, 0, 0, "ROLL INIT33", FALSE));
    }

    lpfnLastMonthEnd = (LP2FN1LONG)GetProcAddress(hOledbIODll, "LastMonthEnd");
    if (!lpfnLastMonthEnd) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading LastMonthEnd from OLEDBIO", 0, 0,
                             "", iError, 0, 0, "ROLL INIT34", FALSE));
    }

    lpprUpdatePerfDate =
        (LPPR3LONG)GetProcAddress(hOledbIODll, "UpdatePerfDate");
    if (!lpprUpdatePerfDate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading UpdatePerfDate", 0, 0, "", iError,
                             0, 0, "ROLL INIT35", FALSE));
    }

    lpprSelectTrndesc =
        (LPPRDTRANSDESCSELECT)GetProcAddress(hOledbIODll, "SelectTransDesc");
    if (!lpprSelectTrndesc) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectTrndesc", 0, 0, "", iError, 0,
                             0, "ROLL INIT36", FALSE));
    }

    lpprSelectTransBySettlementDate = (LPPRSELECTTRANS)GetProcAddress(
        hOledbIODll, "SelectTransBySettlementDate");
    if (!lpprSelectTransBySettlementDate) {
      iError = GetLastError();
      return (lpfnPrintError("Error Loading SelectTransBySettlementDate", 0, 0,
                             "", iError, 0, 0, "ROLL INIT37", FALSE));
    }

    /*lpfnCompInsHedgxref = (LPFNCOMPINSHOLD)GetProcAddress(hOledbIODll,
       "CompInsHedgxref"); if(!lpfnCompInsHedgxref)
                {
                        iError = GetLastError();
                return(lpfnPrintError("Error Loading CompInsHedgxref Function",
       0, 0, "", iError, 0, 0, "ROLL INIT8", FALSE));
                }
                lpfnCompInsPortmain  =
       (LPFNCOMPINSHOLD)GetProcAddress(hOledbIODll, "CompInsPortmain");
                if(!lpfnCompInsPortmain)
                {
                        iError = GetLastError();
                return(lpfnPrintError("Error Loading CompInsPortmain Function",
       0, 0, "", iError, 0, 0, "ROLL INIT8", FALSE));
                }
                lpfnCompInsPrec  = (LPFNCOMPINSHOLD)GetProcAddress(hOledbIODll,
       "CompInsPrec"); if(!lpfnCompInsPayrec)
                {
                        iError = GetLastError();
                return(lpfnPrintError("Error Loading CompInsPrec Function", 0,
       0, "", iError, 0, 0, "ROLL INIT8", FALSE));
                }
                lpfnCompInsPbal  = (LPFNCOMPINSHOLD)GetProcAddress(hOledbIODll,
       "CompInsPbal"); if(!lpfnCompInsPortbal)
                {
                        iError = GetLastError();
                return(lpfnPrintError("Error Loading CompInsPbal Function", 0,
       0, "", iError, 0, 0, "ROLL INIT8", FALSE));
                }
                lpfnCompInsHcash = (LPFNCOMPINSHOLD)GetProcAddress(hOledbIODll,
       "CompInsHcash"); if(!lpfnCompInsHoldcash)
                {
                        iError = GetLastError();
                return(lpfnPrintError("Error Loading CompInsHcash Function", 0,
       0, "", iError, 0, 0, "ROLL INIT8", FALSE));
                }
                lpfnCompInsHold  = (LPFNCOMPINSHOLD)GetProcAddress(hOledbIODll,
       "CompInsHold"); if(!lpfnCompInsHoldings)
                {
                        iError = GetLastError();
                return(lpfnPrintError("Error Loading CompInsHold Function", 0,
       0, "", iError, 0, 0, "ROLL INIT8", FALSE));

                }*/

    bInit = TRUE;
  } // If never initialized before

  zErr = CallInitTranProc(lAsofDate, sDBPath, sMode, sType, sErrFile);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  return zErr;
} // InitRoll

/*
** Tranproc needs to be initialized a couple of times with different
** as of date, rest of the parameters(dbalias, mode, etc.) don't change. When
** first time this function is called(from InitRoll), calling function has
** all the required parameters but later on it is likely that the calling
** program will only have a new as of date. So, the first time this function
** is called all the variables are stored in static memory and later on those
** stored values are used to initialize tranproc again.
*/
ERRSTRUCT CallInitTranProc(long lAsofDate, char *sDBAlias, char *sMode,
                           char *sType, char *sErrFile) {
  ERRSTRUCT zErr;

  static long lLastDate = -1;
  static char sLastDBAlias[80] = "";
  static char sLastMode[2] = "";
  static char sLastType[2] = "";
  static char sLastErrFile[80] = "";
  static BOOL bInit = FALSE;

  lpprInitializeErrStruct(&zErr);

  if (!bInit) {
    strcpy_s(sLastDBAlias, sDBAlias);
    strcpy_s(sLastMode, sMode);
    strcpy_s(sLastType, sType);
    strcpy_s(sLastErrFile, sErrFile);
  }

  zErr = lpfnInitTranProc(lAsofDate, sLastDBAlias, sLastMode, sLastType, FALSE,
                          sLastErrFile);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    bInit = FALSE;
    return zErr;
  }

  lLastDate = lAsofDate;
  bInit = TRUE;

  return zErr;
} // CallInitTranProc

/*
** Function to Get Next Account
*/
ERRSTRUCT GetNextAccount(char *sSourceFlag, PORTMAIN *pzPR) {
  ERRSTRUCT zErr;
  //  char sTemp[80];

  lpprInitializeErrStruct(&zErr);

  /* This function is defined in the tranproc library */
  // lpprInitializePortdirStruct(pzPR);

  if (strcmp(sSourceFlag, "F") == 0) {
    //		lpfnFirmAcct( &pzPR, sTemp, &zErr);
  } else if (strcmp(sSourceFlag, "C") == 0)
    ; // EXEC SQL FETCH COMP_ACCT INTO :pzPR->iID;
      // Note: The ; is only for compiler. Otherwise we will get an error msg.
  else if (strcmp(sSourceFlag, "M") == 0)
    ; // EXEC SQL FETCH MGR_ACCT INTO :pzPR->iID;
      // Note: The ; is only for compiler. Otherwise we will get an error msg.
  else
    return (lpfnPrintError("Invalid Flag", pzPR->iID, 0, "", 602, 0, 0,
                           "ROLL GETACCT1", FALSE));
  if (zErr.iSqlError && zErr.iSqlError != SQLNOTFOUND)
    zErr =
        lpfnPrintError("Error Fetching Account", pzPR->iID, 0, "", 0,
                       zErr.iSqlError, zErr.iIsamCode, "ROLL GETACCT2", FALSE);

  return zErr;
} /* getnextaccount */

/*
** Function to prepare the destination data set for the whole firm by copying
** source over it.
*/
ERRSTRUCT FirmDestInit(char *sSecNo, char *sWi, long lSrcDate, long lDestDate) {
  static BOOL bLastSecSpecific;
  static long lLastDate = -1, lLastSrc = -1, lLastDest = -1;
  static int iLastSecStatus;

  ERRSTRUCT zErr;
  BOOL bSrcExist, bSecSpecific, bSecInHoldings;
  int iSecStatus;
  char sHoldings[STR80LEN], sHoldcash[STR80LEN], sPortbal[STR80LEN],
      sPortmain[STR80LEN];
  char sHedgeXref[STR80LEN], sPayrec[STR80LEN], sHoldtot[STR80LEN];
  char sHoldings1[STR80LEN], sHoldcash1[STR80LEN], sPortbal1[STR80LEN],
      sPortmain1[STR80LEN];
  char sHedgeXref1[STR80LEN], sPayrec1[STR80LEN], sHoldtot1[STR80LEN],
      sHoldings2[STR80LEN], sHoldcash2[STR80LEN];
  char sPortbal2[STR80LEN], sPortmain2[STR80LEN], sHedgeXref2[STR80LEN],
      sPayrec2[STR80LEN], sHoldtot2[STR80LEN];

  lpprInitializeErrStruct(&zErr);
  bSecInHoldings = FALSE;
  /* If Passed SecNo is not null, initialization is being done for a security */
  if (sSecNo[0] == '\0') {
    bSecSpecific = FALSE;
    iSecStatus = 0;
  } else {
    bSecSpecific = TRUE;
    zErr = IsSecurityInHoldings(sSecNo, sWi, &bSecInHoldings);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    if (bSecInHoldings)
      iSecStatus = 1;
    else
      iSecStatus = 2;
  } /* Init for a specific security */

  if (lDestDate == PERFORM_DATE) {
    strcpy_s(sHoldings, "hdperf");
    strcpy_s(sHoldcash, "hcperf");
    strcpy_s(sPortbal, "pbperf");
    strcpy_s(sPortmain, "pmperf");
    strcpy_s(sHedgeXref, "hxperf");
    strcpy_s(sPayrec, "prperf");
    strcpy_s(sHoldtot, "htadhoc");
  } else if (lDestDate == SETTLEMENT_DATE) {
    strcpy_s(sHoldings, "hdstlmnt");
    strcpy_s(sHoldcash, "hcstlmnt");
    strcpy_s(sPortbal, "pbstlmnt");
    strcpy_s(sPortmain, "pmstlmnt");
    strcpy_s(sHedgeXref, "hxstlmnt");
    strcpy_s(sPayrec, "prstlmnt");
    strcpy_s(sHoldtot, "htstlmnt");
  } else {
    /* Prepare delete cursors if they are not prepared yet */
    zErr =
        ReadHoldingsMapForADate(lDestDate, sHoldings, sHoldcash, sPortbal,
                                sPortmain, sHedgeXref, sPayrec, sHoldtot, TRUE);
    if (zErr.iSqlError == SQLNOTFOUND ||
        strncmp(sHoldings, "holdings", 8) == 0) {
      /*
      ** If the passed date was the ADHOC or REORG date, return with an error,
      ** else read holdmap again for the ADHOC_DATE.
      */
      if (lDestDate == ADHOC_DATE || lDestDate == PERFORM_DATE)
        return (lpfnPrintError("No Adhoc/Reorg Data Set Exists", 0, 0, "", 603,
                               0, 0, "ROLL FIRMDEL1", FALSE));
      else {
        lDestDate = ADHOC_DATE;
        zErr = ReadHoldingsMapForADate(lDestDate, sHoldings, sHoldcash,
                                       sPortbal, sPortmain, sHedgeXref, sPayrec,
                                       sHoldtot, TRUE);
        if (zErr.iSqlError == SQLNOTFOUND)
          return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0,
                                 0, "ROLL FIRMDEL2", FALSE));
        else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
      } /* if passed date was not the adhoc or reorg date */
    } /* if record not found for the passed date */
  } /* if lDestDate is not PERFORM_DATE  */
  /*
  ** Delete records from all the tables. If initializing for a specific
  ** security then delete records only for that security, else delete records
  ** for the whole account.
  */
  if (bSecSpecific) {
    /*
    ** If doing security specific roll, security will either be in holdings or
    ** holdcash, but not in both the tables, so initialize only that table
    ** where the security is.
    */
    if (bSecInHoldings) {
      /* Delete Holdings */
      //      lpfnFirmDelHold(sSecNo, sWi, sHoldings, bSecSpecific, &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Deleting Records From Holdings", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL FIRMINITDEST1.0", FALSE));
    } else {
      /* delete holdcash */
      //    lpfnFirmDelHcash ( sSecNo, sWi, sHoldcash, bSecSpecific, &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Deleting Record From HoldCash", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL FIRMINITDEST2.0", FALSE));
    } /* Security in holdcash */

    /* delete hedge_xref */
    // lpfnFirmDelHedgxref ( sSecNo, sWi, sHedgeXref, bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST3.0", FALSE));

    /* delete payrec */
    // lpfnFirmDelPrec ( sSecNo, sWi, sPayrec, bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST4.0", FALSE));
  } else {
    /* Delete Holdings */
    // lpfnFirmDelHold ( sSecNo, sWi, sHoldings, bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Records From Holdings", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST1.1", FALSE));

    /* Delete holdcash */
    // lpfnFirmDelHcash ( sSecNo, sWi, sHoldcash, bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HoldCash", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST2.1", FALSE));

    /* delete hedge_xref */
    // lpfnFirmDelHedgxref ( sSecNo, sWi, sHedgeXref, bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST3.1", FALSE));

    /* delete payrec */
    // lpfnFirmDelPrec ( sSecNo, sWi, sPayrec, bSecSpecific, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST4.1", FALSE));
  }

  /* delete portbal */
  // lpfnFirmDelPbal(sPortbal, bSecSpecific, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Deleting Record From Portbal", 0, 0, "", 0,
                           zErr.iSqlError, zErr.iIsamCode, "ROLL FIRMINITDEST5",
                           FALSE));
  /* delete portdir */
  // lpfnFirmDelPortmain(sPortmain, bSecSpecific, &zErr);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Deleting Record From Portdir", 0, 0, "", 0,
                           zErr.iSqlError, zErr.iIsamCode, "ROLL FIRMINITDEST6",
                           FALSE));

  /* If source and destination are same, return nothing to copy */
  if (lSrcDate == lDestDate)
    return zErr;
  if (lDestDate == PERFORM_DATE) {
    strcpy_s(sHoldings1, "hdperf");
    strcpy_s(sHoldcash1, "hcperf");
    strcpy_s(sPortbal1, "pbperf");
    strcpy_s(sPortmain1, "pmperf");
    strcpy_s(sHedgeXref1, "hxperf");
    strcpy_s(sPayrec1, "prperf");
    strcpy_s(sHoldtot1, "holdtot");
  } else if (lDestDate == SETTLEMENT_DATE) {
    strcpy_s(sHoldings, "hdstlmnt");
    strcpy_s(sHoldcash, "hcstlmnt");
    strcpy_s(sPortbal, "pbstlmnt");
    strcpy_s(sPortmain, "pmstlmnt");
    strcpy_s(sHedgeXref, "hxstlmnt");
    strcpy_s(sPayrec, "prstlmnt");
    strcpy_s(sHoldtot, "htstlmnt");
  } else {
    /* Now prepare insert cursors. If no source data set exists, nothing to do
     */
    zErr = ReadHoldingsMapForADate(lDestDate, sHoldings1, sHoldcash1, sPortbal1,
                                   sPortmain1, sHedgeXref1, sPayrec1, sHoldtot1,
                                   FALSE);
    if (zErr.iSqlError == SQLNOTFOUND)
      return (lpfnPrintError("No Destination Data Set Exists", 0, 0, "", 603, 0,
                             0, "ROLL FIRMINSERT1", FALSE));
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }

  /* Read table for source data set */
  zErr = ReadHoldingsMapForADate(lSrcDate, sHoldings2, sHoldcash2, sPortbal2,
                                 sPortmain2, sHedgeXref2, sPayrec2, sHoldtot2,
                                 FALSE);
  if (zErr.iSqlError == SQLNOTFOUND) {
    bSrcExist = FALSE;
    zErr.iSqlError = 0;
    return zErr;
  } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  else
    bSrcExist = TRUE;

  if (bSecSpecific) {
    if (bSecInHoldings) {
      /* Copy source holdings on to destination holdings */
      //      lpfnFirmInsHold(sSecNo, sWi, iSecStatus, sHoldings1, sHoldings2,
      //      &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Inserting Record Into Holdings", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL FIRMINITDEST7.0", FALSE));
    } else {
      /* Copy source holdcash on to destination holdcash */
      //    lpfnFirmInsHcash(sSecNo, sWi, iSecStatus, sHoldcash1, sHoldcash2,
      //    &zErr);
      if (zErr.iSqlError || zErr.iBusinessError)
        return (lpfnPrintError("Error Inserting Record Into HoldCash", 0, 0, "",
                               0, zErr.iSqlError, zErr.iIsamCode,
                               "ROLL FIRMINITDEST8.0", FALSE));
    } /* In holdcash */

    /* Copy source hedge_xref on to destination hedge_xref */
    //  lpfnFirmInsHedgxref(sSecNo, sWi, iSecStatus, sHedgeXref1, sHedgeXref2,
    //  &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST9.0", FALSE));

    /* Copy source payrec on to destination payrec */
    // lpfnFirmInsPrec(sSecNo, sWi, iSecStatus, sPayrec1, sPayrec2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST10.0", FALSE));

    /* Copy source portbal on to destination portbal */
    // lpfnFirmInsPbal(iSecStatus, sPortbal1, sPortbal2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portbal", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL ACCNTINITDEST11.0", FALSE));

    /* Copy source portdir on to destination portdir */
    // lpfnFirmInsPortmain(iSecStatus, sPortmain1, sPortmain2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portdir", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL ACCNTINITDEST12.0", FALSE));
  } /* If bsecspecific = TRUE */
  else {
    /* Copy source holdings on to destination holdings */
    // lpfnFirmInsHold(sSecNo, sWi, iSecStatus, sHoldings1, sHoldings2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Holdings", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST7.1", FALSE));

    /* Copy source holdcash on to destination holdcash */
    // lpfnFirmInsHcash(sSecNo, sWi, iSecStatus, sHoldcash1, sHoldcash2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HoldCash", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST8.1", FALSE));
    /* Copy source hedge_xref on to destination hedge_xref */
    // lpfnFirmInsHedgxref(sSecNo, sWi, iSecStatus, sHedgeXref1, sHedgeXref2,
    // &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST9.1", FALSE));

    /* Copy source payrec on to destination payrec */
    // lpfnFirmInsPrec(sSecNo, sWi, iSecStatus, sPayrec1, sPayrec2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL FIRMINITDEST10.1", FALSE));

    /* Copy source portbal on to destination portbal */
    // lpfnFirmInsPbal(iSecStatus, sPortbal1, sPortbal2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portbal", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL ACCNTINITDEST11", FALSE));

    /* Copy source portdir on to destination portdir */
    // lpfnFirmInsPortmain(iSecStatus, sPortmain1, sPortmain2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portdir", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL ACCNTINITDEST12", FALSE));
  } /* If rolling whole account */

  return zErr;
} /* FirmDestInit */

/*****************************************************************************
** Function to prepare the destination data set for a composite by copying  **
** source over it.                                                          **
******************************************************************************/
ERRSTRUCT CompDestInit(int iID, long lSrcDate, long lDestDate) {
  ERRSTRUCT zErr;
  BOOL bSrcExist;
  //  static BOOL bInit = FALSE;
  static long lLastDate = -1, lLastSrc = -1;
  char sHoldings[STR80LEN], sHoldcash[STR80LEN], sPortbal[STR80LEN],
      sPortmain[STR80LEN];
  char sHedgeXref[STR80LEN], sPayrec[STR80LEN], sHoldtot[STR80LEN];
  char sHoldings1[STR80LEN], sHoldcash1[STR80LEN], sPortbal1[STR80LEN],
      sPortmain1[STR80LEN];
  char sHedgeXref1[STR80LEN], sPayrec1[STR80LEN], sHoldtot1[STR80LEN],
      sHoldings2[STR80LEN], sHoldcash2[STR80LEN];
  char sPortbal2[STR80LEN], sPortmain2[STR80LEN], sHedgeXref2[STR80LEN],
      sPayrec2[STR80LEN], sHoldtot2[STR80LEN];

  lpprInitializeErrStruct(&zErr);
  if (lDestDate == PERFORM_DATE) {
    strcpy_s(sHoldings, "hdperf");
    strcpy_s(sHoldcash, "hcperf");
    strcpy_s(sPortbal, "pbperf");
    strcpy_s(sPortmain, "pmperf");
    strcpy_s(sHedgeXref, "hxperf");
    strcpy_s(sPayrec, "prperf");
    strcpy_s(sHoldtot, "holdtot");
  } else if (lDestDate == SETTLEMENT_DATE) {
    strcpy_s(sHoldings, "hdstlmnt");
    strcpy_s(sHoldcash, "hcstlmnt");
    strcpy_s(sPortbal, "pbstlmnt");
    strcpy_s(sPortmain, "pmstlmnt");
    strcpy_s(sHedgeXref, "hxstlmnt");
    strcpy_s(sPayrec, "prstlmnt");
    strcpy_s(sHoldtot, "htstlmnt");
  } else {
    /* Prepare delete cursors if they are not prepared yet */
    zErr =
        ReadHoldingsMapForADate(lDestDate, sHoldings, sHoldcash, sPortbal,
                                sPortmain, sHedgeXref, sPayrec, sHoldtot, TRUE);
    if (zErr.iSqlError == SQLNOTFOUND ||
        strncmp(sHoldings, "holdings", 8) == 0) {
      /*
      ** If the passed date was the ADHOC or REORG date, return with an error,
      ** else read holdmap again for the ADHOC_DATE.
      */
      if (lDestDate == ADHOC_DATE || lDestDate == PERFORM_DATE)
        return (lpfnPrintError("No Adhoc/Reorg Data Set Exists", 0, 0, "", 603,
                               0, 0, "ROLL COMPDEL1", FALSE));
      else {
        lDestDate = ADHOC_DATE;
        zErr = ReadHoldingsMapForADate(lDestDate, sHoldings, sHoldcash,
                                       sPortbal, sPortmain, sHedgeXref, sPayrec,
                                       sHoldtot, TRUE);
        if (zErr.iSqlError == SQLNOTFOUND)
          return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0,
                                 0, "ROLL COMPDEL2", FALSE));
        else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
      } /* if passed date was not the adhoc or reorg date */
    } /* if record not found for the passed date */

    /* Delete Holdings */
    //  lpfnCompDelHold(iID,sHoldings, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Records From Holdings", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST1", FALSE));

    /* Delete holdcash */
    // lpfnCompDelHcash(iID, sHoldcash, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HoldCash", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST2", FALSE));

    /* delete hedge_xref */
    // lpfnCompDelHedgxref(iID, sHedgeXref, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST3", FALSE));

    /* delete payrec */
    // lpfnCompDelPrec(iID, sPayrec, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST4", FALSE));

    /* delete portbal */
    // lpfnCompDelPbal(iID, sPortbal, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Portbal", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST5", FALSE));

    /* delete portdir */
    // lpfnCompDelPortmain(iID, sPortmain, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Portdir", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST6", FALSE));

    /* If source and destination are same, return without copying anything */
    if (lSrcDate == lDestDate)
      return zErr;

    /* Now prepare insert cursors. If no source data set exists, nothing to do
     */
    zErr = ReadHoldingsMapForADate(lDestDate, sHoldings1, sHoldcash1, sPortbal1,
                                   sPortmain1, sHedgeXref1, sPayrec1, sHoldtot1,
                                   TRUE);
    if (zErr.iSqlError == SQLNOTFOUND)
      return (lpfnPrintError("No Destination Data Set Exists", 0, 0, "", 603, 0,
                             0, "ROLL COMPINSERT1", FALSE));
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Read table for source data set */
    zErr = ReadHoldingsMapForADate(lSrcDate, sHoldings2, sHoldcash2, sPortbal2,
                                   sPortmain2, sHedgeXref2, sPayrec2, sHoldtot2,
                                   FALSE);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bSrcExist = FALSE;
      zErr.iSqlError = 0;
      return zErr;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
    else
      bSrcExist = TRUE;

    /* Copy source holdings on to destination holdings */
    // lpfnCompInsHold(iID,sHoldings1, sHoldings2, &zErr);;
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Holdings", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST7", FALSE));

    /* Copy source holdcash on to destination holdcash */
    // lpfnCompInsHcash(iID, sHoldcash1, sHoldcash2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HoldCash", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST8", FALSE));

    /* Copy source hedge_xref on to destination hedge_xref */
    // lpfnCompInsHedgxref(iID, sHedgeXref1,sHedgeXref2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST9", FALSE));

    /* Copy source payrec on to destination payrec */
    // lpfnCompInsPrec(iID, sPayrec1,sPayrec2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST10", FALSE));

    /* Copy source portbal on to destination portbal */
    // lpfnCompInsPbal(iID, sPortbal1,sPortbal2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portbal", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST11", FALSE));

    /* Copy source portdir on to destination portdir */
    // lpfnCompInsPortmain(iID, sPortmain1,sPortmain2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portdir", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL COMPINITDEST12", FALSE));
  } /* if lDestDate is not PERFORM_DATE*/
  return zErr;
} /* CompDestInit */

/**************************************************************************
** Function to prepare the destination data set for a manager by copying **
** source over it.                                                       **
** ---------------------------- MgrDestInit ---------------------------- **
***************************************************************************/

ERRSTRUCT MgrDestInit(char *sMgr, long lSrcDate, long lDestDate) {
  ERRSTRUCT zErr;
  BOOL bSrcExist;
  //	static BOOL bInit = FALSE;
  static long lLastSrc = -1, lLastDest = -1;

  char sHoldings[STR80LEN], sHoldcash[STR80LEN], sPortbal[STR80LEN],
      sPortmain[STR80LEN];
  char sHedgeXref[STR80LEN], sPayrec[STR80LEN], sHoldtot[STR80LEN];
  char sHoldings1[STR80LEN], sHoldcash1[STR80LEN], sPortbal1[STR80LEN],
      sPortmain1[STR80LEN];
  char sHedgeXref1[STR80LEN], sPayrec1[STR80LEN], sHoldtot1[STR80LEN],
      sHoldings2[STR80LEN], sHoldcash2[STR80LEN];
  char sPortbal2[STR80LEN], sPortmain2[STR80LEN], sHedgeXref2[STR80LEN],
      sPayrec2[STR80LEN], sHoldtot2[STR80LEN];
  //  char sStatement[150];
  lpprInitializeErrStruct(&zErr);

  /* Prepare delete cursors if they are not prepared yet */
  if (lDestDate == PERFORM_DATE) {
    strcpy_s(sHoldings, "hdperf");
    strcpy_s(sHoldcash, "hcperf");
    strcpy_s(sPortbal, "pbperf");
    strcpy_s(sPortmain, "pmperf");
    strcpy_s(sHedgeXref, "hxperf");
    strcpy_s(sPayrec, "prperf");
    strcpy_s(sHoldtot, "holdtot");
  } else if (lDestDate == SETTLEMENT_DATE) {
    strcpy_s(sHoldings, "hdstlmnt");
    strcpy_s(sHoldcash, "hcstlmnt");
    strcpy_s(sPortbal, "pbstlmnt");
    strcpy_s(sPortmain, "pmstlmnt");
    strcpy_s(sHedgeXref, "hxstlmnt");
    strcpy_s(sPayrec, "prstlmnt");
    strcpy_s(sHoldtot, "htstlmnt");
  } else {
    zErr =
        ReadHoldingsMapForADate(lDestDate, sHoldings, sHoldcash, sPortbal,
                                sPortmain, sHedgeXref, sPayrec, sHoldtot, TRUE);
    if (zErr.iSqlError == SQLNOTFOUND ||
        strncmp(sHoldings, "holdings", 8) == 0) {
      /*
      ** If the passed date was the ADHOC or REORG date, return with an error,
      ** else read holdmap again for the ADHOC_DATE.
      */
      if (lDestDate == ADHOC_DATE || lDestDate == PERFORM_DATE)
        return (lpfnPrintError("No Adhoc/Reorg Data Set Exists", 0, 0, "", 603,
                               0, 0, "ROLL MGRDEL1", FALSE));
      else {
        lDestDate = ADHOC_DATE;
        zErr = ReadHoldingsMapForADate(lDestDate, sHoldings, sHoldcash,
                                       sPortbal, sPortmain, sHedgeXref, sPayrec,
                                       sHoldtot, TRUE);
        if (zErr.iSqlError == SQLNOTFOUND)
          return (lpfnPrintError("No Ad Hoc Data Set Exists", 0, 0, "", 603, 0,
                                 0, "ROLL MGRDEL2", FALSE));
        else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
      } /* if passed date was not the adhoc or reorg date */
    } /* if record not found for the passed date */
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Delete Holdings */
    /* Call to function MgrDelHold */
    // lpfnMgrDelHold(sMgr, sHoldings, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Records From Holdings", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST1", FALSE));

    /* Delete holdcash */
    /* Call to function MgrDelHcash */
    // lpfnMgrDelHcash(sMgr, sHoldcash, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HoldCash", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST2", FALSE));

    /* delete hedge_xref */
    /* Call to function MgrDelHedgxref*/
    // lpfnMgrDelHedgxref(sMgr, sHedgeXref, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST3", FALSE));

    /* delete payrec */
    /* Call to function MgrDelPrec */
    // lpfnMgrDelPrec(sMgr, sPayrec, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST4", FALSE));

    /* delete portbal */
    /* Call to functiom MgrDelPbal */
    // lpfnMgrDelPbal(sMgr, sPortbal, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Portbal", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST5", FALSE));

    /* delete portdir */
    /* Call to function MgrDelPortmain */
    // lpfnMgrDelPortmain(sMgr, sPortmain, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Deleting Record From Portdir", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST6", FALSE));

    /* If source and destination are same, return without copying anything */
    if (lSrcDate == lDestDate)
      return zErr;

    /* Now prepare insert cursors. If no source data set exists, nothing to do
     */
    /*zErr = MgrInsertPrepare(lSrcDate, lDestDate, &bSrcExist);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;
    else if (bSrcExist == FALSE)
            return zErr;*/
    zErr = ReadHoldingsMapForADate(lDestDate, sHoldings1, sHoldcash1, sPortbal1,
                                   sPortmain1, sHedgeXref1, sPayrec1, sHoldtot1,
                                   TRUE);
    if (zErr.iSqlError == SQLNOTFOUND)
      return (lpfnPrintError("No Destination Data Set Exists", 0, 0, "", 603, 0,
                             0, "ROLL MGRINSERT1", FALSE));
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /* Read table for source data set */
    zErr = ReadHoldingsMapForADate(lSrcDate, sHoldings2, sHoldcash2, sPortbal2,
                                   sPortmain2, sHedgeXref2, sPayrec2, sHoldtot2,
                                   FALSE);
    if (zErr.iSqlError == SQLNOTFOUND) {
      bSrcExist = FALSE;
      zErr.iSqlError = 0;
      return zErr;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
    else
      bSrcExist = TRUE;

    /* Copy source holdings on to destination holdings */
    // lpfnMgrInsHold(sMgr, sHoldings1, sHoldings2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Holdings", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST7", FALSE));

    /* Copy source holdcash on to destination holdcash */
    // lpfnMgrInsHcash(sMgr, sHoldcash1, sHoldcash2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HoldCash", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST8", FALSE));

    /* Copy source hedge_xref on to destination hedge_xref */
    // lpfnMgrInsHedgxref(sMgr, sHedgeXref1, sHedgeXref2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into HedgeXref", 0, 0, "",
                             0, zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST9", FALSE));

    /* Copy source payrec on to destination payrec */
    // lpfnMgrInsPrec(sMgr, sPayrec1, sPayrec2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Payrec", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST10", FALSE));

    /* Copy source portbal on to destination portbal */
    // lpfnMgrInsPbal(sMgr, sPortbal1, sPortbal2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portbal", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST11", FALSE));

    /* Copy source portdir on to destination portdir */
    // lpfnMgrInsPortmain(sMgr, sPortmain1, sPortmain2, &zErr);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Inserting Record Into Portdir", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "ROLL MGRINITDEST12", FALSE));
  } /*if lDestDate is not same as PERFORM_DATE*/
  return zErr;
} /* MgrDestInit */

/*********************************************************************************
** Function to read next br_acct from the temp table (which is declared to have
* *
** all the branch account in the whole firm having the given security on a date)
* *
**********************************************************************************/
ERRSTRUCT GetFSecBrAcct(PORTMAIN *pzPR) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  // EXEC SQL FETCH FSEC_ACCT_CURSOR INTO :pzPR->sBrAcct;
  if (zErr.iSqlError == SQLNOTFOUND) {
    zErr.iSqlError = SQLNOTFOUND;
    // EXEC SQL CLOSE FSEC_ACCT_CURSOR;
  } else if (zErr.iSqlError || zErr.iBusinessError)
    zErr =
        lpfnPrintError("Error Fetching BrAcct From Cursor", 0, 0, "", 0,
                       zErr.iSqlError, zErr.iIsamCode, "ROLL GTBRACCT", FALSE);

  return zErr;

} /* GetFirmBrAcctForASecurity */

/*******************************************************************************
** Function to create a cursor to insert a list of branch account interested in*
** a particular security, into a temp table. This function is used whenever a  *
** firm wide roll of a security is done.                                       *
********************************************************************************/
ERRSTRUCT CreateFSecBrAcctCursor(char *sSecNo, char *sWi, long lRollDate,
                                 long lSrceDate, long lDestDate) {
  ERRSTRUCT zErr;
  BOOL bSecInHoldings;
  static BOOL bTableExists = FALSE;

  long lStartDate, lEndDate;
  char sHoldings1[STR80LEN], sHoldcash1[STR80LEN], sHoldings2[STR80LEN],
      sHoldcash2[STR80LEN];
  char sStatement[200], sTemp[STR80LEN];

  lpprInitializeErrStruct(&zErr);

  /* Read table for dest data set */
  if (lDestDate == PERFORM_DATE) {
    strcpy_s(sHoldings1, "hdperf");
    strcpy_s(sHoldcash1, "hcperf");
  } else {
    zErr = ReadHoldingsMapForADate(lDestDate, sHoldings1, sHoldcash1, sTemp,
                                   sTemp, sTemp, sTemp, sTemp, FALSE);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }
  /* Read table for source data set */

  zErr = ReadHoldingsMapForADate(lSrceDate, sHoldings2, sHoldcash2, sTemp,
                                 sTemp, sTemp, sTemp, sTemp, FALSE);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /* Find out if security is in holdings or holdcash */

  zErr = IsSecurityInHoldings(sSecNo, sWi, &bSecInHoldings);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /* If the table does not exists, create it, else empty it. */

  if (bTableExists) {
    /*
    ** br_acct in holdings is char(8), so it will never find an
    ** account = 'zzzzzzzzzz' and hence an empty temp table(with right
    ** structure) will be created.
    */
    // EXEC SQL SELECT br_acct FROM pmr:holdings
    //           WHERE br_acct = 'zzzzzzzzzz' INTO TEMP FSEC_ACCT_TABLE;
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Creating Temp Table", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode, "ROLL CREATEACCT1",
                             FALSE));

    // EXEC SQL CREATE INDEX idx1 ON FSEC_ACCT_TABLE (br_acct);
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Creating Index On Temp Table", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode, "ROLL CREATEACCT2",
                             FALSE));

    bTableExists = TRUE;

  } /* Table doesn't yet exist */

  else {
    // EXEC SQL DELETE FROM FSEC_ACCT_TABLE;
    if (zErr.iSqlError || zErr.iBusinessError)
      return (lpfnPrintError("Error Emptying Temp Table", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode, "ROLL CREATEACCT3",
                             FALSE));
  } /* Table already exist */

  /*
  ** Get all the branch account holding given security on source date in the
  ** temp table(named FSEC_ACCT_TABLE)
  */
  if (bSecInHoldings)
    sprintf_s(sStatement,
              "INSERT INTO FSEC_ACCT_TABLE SELECT br_acct FROM pmr:%s"
              " WHERE sec_no = ? AND wi = ?",
              sHoldings2);
  else
    sprintf_s(sStatement,
              "INSERT INTO FSEC_ACCT_TABLE SELECT br_acct FROM pmr:%s"
              " WHERE sec_no = ? AND wi = ? ",
              sHoldcash2);

  // EXEC SQL PREPARE FSEC_ACCT_ST1 FROM :sStatement;
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Preparing Statement To Read Holdings Table ",
                           0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT4", FALSE));

  // EXEC SQL EXECUTE FSEC_ACCT_ST1 USING :sSecNo, :sWi;
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Executing Statement To Read Holdings Table ",
                           0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT5", FALSE));

  /*
  ** Add branch accounts from trans which have trades between source date
  ** and roll date.
  */
  sprintf_s(sStatement, "INSERT INTO FSEC_ACCT_TABLE SELECT br_acct "
                        "FROM trans:trans WHERE sec_no = ? AND wi = ? AND "
                        "eff_date > ? AND eff_date <= ? AND rev_trans_no = 0");
  // EXEC SQL PREPARE FSEC_ACCT_ST2 FROM :sStatement;
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Preparing Statement To Read Trans Table", 0,
                           0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT6", FALSE));

  if (lSrceDate < lRollDate) {
    lStartDate = lSrceDate;
    lEndDate = lRollDate;
  } else {
    lStartDate = lRollDate;
    lEndDate = lSrceDate;
  }

  // EXEC SQL EXECUTE FSEC_ACCT_ST2 USING :sSecNo, :sWi, :lStartDate, :lEndDate;
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Executing Statement To Read Trans Table", 0,
                           0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT7", FALSE));

  // EXEC SQL UPDATE STATISTICS FOR TABLE FSEC_ACCT_TABLE (br_acct);
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Updating Statistics For Temp Table", 0, 0, "",
                           0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT8", FALSE));

  /* Declare the cursor to read unique branch account from the temp table */

  sprintf_s(sStatement, "SELECT br_acct FROM FSEC_ACCT_TABLE GROUP BY br_acct");

  // EXEC SQL PREPARE FSEC_ACCT_ST3 FROM :sStatement;
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Preparing Statement To Read Temp Table", 0, 0,
                           "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT9", FALSE));

  // EXEC SQL DECLARE FSEC_ACCT_CURSOR CURSOR FOR FSEC_ACCT_ST3;

  /* Open the cursor */

  // EXEC SQL OPEN FSEC_ACCT_CURSOR;
  if (zErr.iSqlError || zErr.iBusinessError)
    return (lpfnPrintError("Error Opening Temp Table To Read Branch Accounts",
                           0, 0, "", 0, zErr.iSqlError, zErr.iIsamCode,
                           "ROLL CREATEACCT10", FALSE));

  return zErr;
} /* CreateFSecBrAcctCursor */

void FreeRoll() {
  FILE *f = fopen("C:\\Users\\Sergey\\.gemini\\roll_debug.log", "a");
  if (f) {
    fprintf(f, "FreeRoll: Setting bInit to FALSE.\n");
    fclose(f);
  }
  bInit = FALSE;
  FreeLibrary(hTEngineDll);
  FreeLibrary(hOledbIODll);
  FreeLibrary(hStarsUtilsDll);
} // FreeRoll
