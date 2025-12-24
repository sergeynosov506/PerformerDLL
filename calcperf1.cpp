/**
 *
 * SUB-SYSTEM: calcperf
 *
 * FILENAME: calcperf.c
 *
 * DESCRIPTION: This is the main file of the "Performance Calculation" library
 * routine.
 *
 * PUBLIC FUNCTION(S): CalcPerf
 *
 * NOTES:
 *       1) Currently it has only one interface, namely CalcPerf which
 *          calculates performance on the current system date for an account. In
 *          future more interfaces may be added, where user can give a specific
 *          date range, more than one account, etc.
 *       2) Although system is designed to handle both static and dynamic keys,
 *          it is a good idea to generate all the keys dynamically. The reason
 *          being that if a static key is declared and at some point the account
 *          sells all security belonging to that key, the key will cease to
 * exist but in future if a new security is bought which belongs to that key,
 *          there is no way to automatically regenarate a matching perfkey, user
 *          will have to remember to add a new static key.
 *       3) It is a requirement of the system that if there are rules defining
 *          parent-child(ern) relationship(Parent - Aggregate, Children - Base
 *          or Local with the rest of the rule same as Parent), Parent rule
 *          should be declared before a child's rule(For the reason check the
 *          "CreateNewPerfkey" function).
 *
 * USAGE: CalcPerf(iID, lCurrentDate, lStartDate, lAnchorDate,
 * lEarliestPerfDate) where iID                portfolio id for which
 * performance needs to be calculated lCurrentDate       performance is
 * calculated for Start date to lStartDate         current date range.
 *               lAnchorDate        No record should be changed prior to this
 *               lEarliestPerfDate  Ignore transactions with perf date prior to
 * this date.
 *
 * AUTHOR: Shobhit Barman(Effron Enterprises, Inc.)
 *
 *
 **/
// History
// 2023-11-14 J#PER12960 Modified logic to handle Notional Flows correctly- mk.
// 2023-10-13 J#PER12923 Modified logic to exclude standard FI accruals when
// flag is M - mk. 2023-06-04 J#PER12738 Added ability to weigh for Value
// Dependent option - mk. 2023-04-08 J#PER11602 Delete perf date when inception
// date moves within a month - mk. 2021-06-01 J#PER11638 Broke out weighted fees
// out from weighted fees - mk. 2021-03-11 J# PER 11415 Restored changes based
// on new FeesOut logic - mk. 2021-03-03 J# PER-11415 Rolled back changes - mk
// 2021-02-26 J# PER-11415 Adjustments for feesout based on perf.dlland
// reporting - mk. 2020-11-11 J# PER-11247 Changed logic on NCF returns -mk.
// 2020-11-02 J# PER-11229 Round daily flows up to 2 decimals when storing -mk.
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2020-04-17 J# PER-10655 Added CN transactions -mk.
// 2019-09-11 J# PER-10145 Repaired logic for monthsum to not accumulate flows
// after terminating a segment -mk. 2019-07-26 J# PER-10090 Fixed issue
// w/zeroing out WFs for terminating segments -mk. 2018-11-16 J# PER-9268 Wells
// Audit - Initialize variables, free memory,deprecated string function  -
// sergeyn 2018-07-23 VI# 61941 Adjusted logic for WH transactions to populate
// contributions or withdrwawals based on dr_cr -mk. 2017-09-06 VI# 60426 Run
// performance in 12 month periods if running for periods over a year -mk.
// 2017-03-02 VI# 59981 Fixed issues w/IPVs and deleted daily flows -mk
// 2016-03-03 VI# 58607 Missed prior version's changes -mk
// 2016-03-02 VI# 58607 Fixed issue w/zero net flow but needed IPV for net
// return -mk 2013-04-12 VI# 52068 Fixed sideeffect w/Calcselected -mk
// 2013-04-02 VI# 52037 Undid change till further notice -mk
// 2013-03-26 VI# 52037 Fixed FAs not being captured in first day of month -mk
// 2013-03-13 VI# 51848 Added returns for terminating flows w/no MV where
// applicable -mk 2010-06-30 VI# 44433 Added calculation of estimated annual
// income -mk 2010-06-16 VI# 42903 Added TodayFeesOut into DAILYINFO - sergeyn
// 2010-06-10 VI# 42903 Added option calculate performance on selected rule -
// sergeyn 2009-10-27 VI#43232 Fixed duplicate record in daily flows - mk
// 2007-11-08 Added Vendor id    - yb
// 2/6/04 - Now the flows stored in the SUMMDATA/DSUMDATA/MONTHSUM include fees
// - vay

#include "calcperf.h"
DllExport void heapdump(void);

/**
** Main function for the dll
**/
BOOL APIENTRY DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    break;
  case DLL_PROCESS_DETACH:
    CalcPerfCleanUp();
    FreePerformanceLibrary();
    break;
  default:
    break;
  }

  return TRUE;
} // DllMain

/**
** This is the function which is called by the user(calling program), it
** initializes the library, figures out current perf date and last date, create
** table of all the perfkeys for the account and then calls CalculatePerformance
** function which does the actual work.
**/
extern "C" DllExport ERRSTRUCT CalcPerf(int iID, long lCurrentDate,
                                        long lStartDate, long lAnchorDate,
                                        long lEarliestPerfDate) {
  ERRSTRUCT zErr;
  PKEYTABLE zPKTable;
  PERFCTRL zPCtrl;
  PARTPMAIN zPmain;
  PERFRULETABLE zRuleTable;
  static long lCurrentPerfDate = 0;
  long lLeastLastDate, lLeastLastMEnd, lBiggestLastDate, lLastPerfDate,
      lPricingDate;
  int i, iNumDeleted;
  short iMDY[3];
  char sMsg[80];
  BOOL bUseTrans;

  lpprInitializeErrStruct(&zErr);
  zRuleTable.iCapacity = 0;

  /*	fp = fopen("sb.log", "a+");
  if (fp == NULL)
  return(lpfnPrintError("Error Creating File",
  iID, 0, "", 999, 0, 0, "CALCPERF", FALSE));*/

  // lpfnTimer(0);
  /*
  ** If user has not given any date, get the date(ProcessDate from
  ** nbcontrol:aims_date table) for which we are doing performance. The function
  ** is defined in tranproc library routine.
  */
  if (lCurrentDate != 0)
    lCurrentPerfDate = lCurrentDate;
  else if (lCurrentPerfDate == 0) {
    lpprSelectStarsDate(&lCurrentPerfDate, &lPricingDate, &zErr);
    if (lCurrentPerfDate < 0)
      return (
          lpfnPrintError("Invalid Performance Date Returned By SelectStarsDate",
                         iID, 0, "", 509, 0, 0, "CALCPERF1", FALSE));
  }

  zErr = InitializeCalcPerfLibrary();
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = InitializeAppropriateLibrary("", "", 'S', -1, FALSE);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  lpprSelectOnePartPortmain(&zPmain, iID, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    if (zErr.iSqlError == SQLNOTFOUND)
      lpfnPrintError("Portfolio Not Found", iID, 0, "", 1, zErr.iSqlError, 0,
                     "CALCPERF2", FALSE);

    return zErr;
  }

  /* Can't run performance on portfolio before its purge date */
  if (lStartDate < zPmain.lPurgeDate) {
    lpfnrjulmdy(lStartDate, iMDY);
    sprintf_s(sMsg,
              "Portfolio Has Been Purged. Performance will start on %d/%d/%d "
              "Or Later",
              iMDY[0], iMDY[1], iMDY[2]);
    lpfnPrintError(sMsg, zPmain.iID, 0, "", 604, 0, 0, "PERFORMANCE", TRUE);
    lStartDate = zPmain.lPurgeDate;
  }

  /* get all the perfkeys defined for the account */
  zPKTable.iCapacity = 0;
  zErr = GetAllPerfkeys(iID, &zPKTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /*
  ** Program is not going to effect any performance record(which already exist)
  ** before the anchor date.
  */
  if (lAnchorDate == 0)
    lAnchorDate = lCurrentPerfDate;

  if (lAnchorDate > lCurrentPerfDate)
    return (
        lpfnPrintError("Not Allowed To Change Any Data, Anchor Date Is Greater "
                       "Than The Current Perf Date",
                       iID, 0, "", 519, 0, 0, "CALCPERF4", FALSE));

  /*
  ** Figure out Last Performance Date if user has not specified it. If the
  ** perfkey table has any active key in it then it's LastPerfdate should be
  ** the Last Performance Date. If the table has more than one key, all of them
  ** should have same LastPerfDate, but if for some odd reason, they have
  ** different dates, then choose the earliest LastPerfDate and run performance
  ** since then.
  */
  if (lStartDate != 0)
    lLastPerfDate = lStartDate;
  else {
    lLeastLastMEnd = lLeastLastDate = lCurrentPerfDate;
    iNumDeleted = 0;
    for (i = 0; i < zPKTable.iCount; i++) {
      if (IsKeyDeleted(zPKTable.pzPKey[i].zPK.lDeleteDate)) {
        iNumDeleted++;
        continue;
      }

      if (lLeastLastDate > zPKTable.pzPKey[i].zPK.lLastPerfDate &&
          zPKTable.pzPKey[i].zPK.lLastPerfDate > 0)
        lLeastLastDate = zPKTable.pzPKey[i].zPK.lLastPerfDate;

      if (lLeastLastMEnd >= zPKTable.pzPKey[i].zPK.lMePerfDate) {
        if (zPKTable.pzPKey[i].zPK.lMePerfDate > 0 &&
            zPKTable.pzPKey[i].zPK.lMePerfDate != lCurrentPerfDate)
          lLeastLastMEnd = zPKTable.pzPKey[i].zPK.lMePerfDate;
      }
    } /* for i < zPKTable.iCount */

    /*
    ** If no key exists in the table then the performance needs to be run
    ** since account inception. If last perf date is not less than the current
    ** perf date, print a message and return
    */
    if (zPKTable.iCount == 0)
      lLastPerfDate = zPmain.lInceptionDate - 1;
    else if (lLeastLastDate >= lCurrentPerfDate) {
      if (iNumDeleted == zPKTable.iCount) {
        /*
        ** If all the keys in the accounts are deleted, get the biggest date on
        ** which a key(in this account) was deleted. If this date is earlier
        ** than LastMonthEnd(compare to current perf date) and earlier than the
        ** anchor date, return with an error(becuase it will cause atleast one
        ** month end(permanent record in monthly file) but the program is not
        ** allowed to write anything before the anchor date).
        */
        lBiggestLastDate = 0;
        for (i = 0; i < zPKTable.iCount; i++) {
          if (lBiggestLastDate < zPKTable.pzPKey[i].zPK.lDeleteDate)
            lBiggestLastDate = zPKTable.pzPKey[i].zPK.lDeleteDate;
        }

        if (lBiggestLastDate < lAnchorDate)
          return (lpfnPrintError(
              "All the Keys In The Account Are Deleted Before The Anchor Date, "
              "There Is No Valid Starting Point",
              iID, 0, "", 999, 0, 0, "CALCPERF5", FALSE));
        else if (lBiggestLastDate != lCurrentPerfDate)
          lLastPerfDate = lBiggestLastDate;

      } /* All the keys in the account are deleted */
      else if (lLeastLastMEnd == lCurrentPerfDate)
        return (lpfnPrintError(
            "Performance Has Already Run For This Date And There Is No Valid "
            "Previous Month End To Start It Again",
            iID, 0, "", 999, 0, 0, "CALCPERF6", FALSE));
      else
        lLastPerfDate = lLeastLastMEnd;
    } /* leastlastdate >= currentperfdate */
    else
      lLastPerfDate = lLeastLastDate;
  } /* If user has not given a start date */

  /*
  ** Read perfctrl file and copy TaxCalc field to perfkey table. If this field
  ** is set to E(tax equivalent), A(after tax) or B(both) then for each key,
  ** pzTInfo needs to be allocated memory(array element same as NumDailyInfo)
  ** along with DailyInfo.
  */
  sprintf_s(sMsg, "Error Selecting Perfctrl For Account %s",
            zPmain.sUniqueName);
  lpprSelectPerfctrl(&zPCtrl, iID, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return (lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError,
                           zErr.iSqlError, zErr.iIsamCode, "CALCPERF7", FALSE));

  // for non-daily perf always run from month end
  if (!(zSysSet.bDailyPerf && strcmp(zPCtrl.sPerfInterval, "D") == 0) &&
      !lpfnIsItAMonthEnd(lLastPerfDate))
    lLastPerfDate = lpfnLastMonthEnd(lLastPerfDate);

  if (zPCtrl.lLastPerfDate != 0 && zPCtrl.lLastPerfDate < lLastPerfDate &&
      lLastPerfDate != zPmain.lPurgeDate) {
    // for daily perf always use last perf date from Perfctrl
    // for monthly - run from month end
    if (zSysSet.bDailyPerf && strcmp(zPCtrl.sPerfInterval, "D") == 0)
      lLastPerfDate = zPCtrl.lLastPerfDate;
    else if (lpfnIsItAMonthEnd(zPCtrl.lLastPerfDate))
      lLastPerfDate = zPCtrl.lLastPerfDate;
    else
      lLastPerfDate = lpfnLastMonthEnd(zPCtrl.lLastPerfDate);
  }

  if (lLastPerfDate < zPmain.lInceptionDate - 1)
    lLastPerfDate = zPmain.lInceptionDate - 1;

  //  strcpy_s(zPKTable.sTaxCalc, zPCtrl.sTaxCalc);
  strcpy_s(zPKTable.sCalcFlow, zPCtrl.sCalcFlow);
  strcpy_s(zPKTable.sPerfInterval, zPCtrl.sPerfInterval);
  strcpy_s(zPKTable.sDRDElig, zPCtrl.sDRDElig);
  zPKTable.lFlowStartDate = zPCtrl.lFlowStartDate;

  // get all the return types this account needs to calculate/save
  // zErr = GetAllReturnTypes(iID, &zPKTable);
  // if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
  // return zErr;

  bUseTrans = (lpfnGetTransCount() == 0);
  if (bUseTrans)
    lpfnStartTransaction(); // use database transactions on account level

  __try {
    CalcSelected = LoadPerfRule(zPmain.iID, lCurrentPerfDate, &zRuleTable);
    if (CalcSelected) {
      DeletePerfkeyByPerfRule(&zRuleTable, &zPKTable);
    }

    /*
    ** Delete any key which has an inception date between LastPerfDate(excluded)
    ** and CurrentPerfDate(included) and Undelete any deleted key in that range.
    ** If required, this run will reinsert/redelete them.
    */
    /*	fprintf(fp, "New Lastperfdate: %d, CurrentPerfDate: %d\n",
    lLastPerfDate, lCurrentDate); for (i = 0; i < zPKTable.iCount; i++)
    fprintf(fp, "ID: %d, ScrhdrNo: %d, InitDate: %d, DeleteDate: %d\n",
    zPKTable.pzPKey[i].zPK.iID, zPKTable.pzPKey[i].zPK.lScrhdrNo,
    zPKTable.pzPKey[i].zPK.lInitPerfDate, zPKTable.pzPKey[i].zPK.lDeleteDate);*/
    zErr = DeleteKeysMemoryDatabase(&zPKTable, lLastPerfDate, lCurrentPerfDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
      if (bUseTrans)
        lpfnRollbackTransaction();

      InitializePKeyTable(&zPKTable);
      return zErr;
    }
    /*	fprintf(fp, "After Deleting MemoryDatabase\n");
    for (i = 0; i < zPKTable.iCount; i++)
    fprintf(fp, "ID: %d, ScrhdrNo: %d, InitDate: %d, DeleteDate: %d\n",
    zPKTable.pzPKey[i].zPK.iID, zPKTable.pzPKey[i].zPK.lScrhdrNo,
    zPKTable.pzPKey[i].zPK.lInitPerfDate, zPKTable.pzPKey[i].zPK.lDeleteDate);*/

    /*
    ** When program finds any asofs(trans.entry date > account.last perf date
    * and
    ** trans.perf date < account.last perf date), it does not process any
    ** transaction whose perf Date < Earliest Perf Date. If the user has not
    ** given an earliest perf date, the program will look as far back as the
    ** 1st of the current perf date's month.
    */
    if (lEarliestPerfDate == 0) {
      if (lpfnIsItAMonthEnd(lLastPerfDate))
        lEarliestPerfDate = lLastPerfDate + 1;
      else {
        lpfnrjulmdy(lCurrentPerfDate, iMDY);
        iMDY[1] = 1;
        lpfnrmdyjul(iMDY, &lEarliestPerfDate);
      }
    } else if (lpfnIsItAMonthEnd(lLastPerfDate))
      lEarliestPerfDate = lLastPerfDate + 1;

    /*
    ** If the performance is running since account inception and we are not
    ** allowed to change anything before inception, force the anchordate to
    ** be inception date.
    */
    if (lAnchorDate > zPmain.lInceptionDate && zPKTable.iCount == 0)
      lAnchorDate = zPmain.lInceptionDate;

    /* Make sure EarliestPerfDate is never before inception */
    if (lEarliestPerfDate < zPmain.lInceptionDate)
      lEarliestPerfDate = zPmain.lInceptionDate;

    // lpfnTimer(1);
    zErr = CalculatePerformance(&zPmain, lLastPerfDate, lCurrentPerfDate,
                                lEarliestPerfDate, &zPKTable,
                                zPmain.sBaseCurrId, zPCtrl, FALSE, &zRuleTable);
  } // try
  __except (lpfnAbortTransaction(bUseTrans)) {
  }

  if (zErr.iSqlError != 0 ||
      (zErr.iBusinessError != 0 && zErr.iBusinessError != 500)) {
    if (bUseTrans)
      lpfnRollbackTransaction();

    sprintf_s(sMsg, "Performance For Account %s Failed", zPmain.sUniqueName);
    lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                   zErr.iIsamCode, "CALCPERF8A", FALSE);
  } else {
    if (bUseTrans)
      lpfnCommitTransaction();

    sprintf_s(sMsg, "Performance For Account %s Successful",
              zPmain.sUniqueName);
    lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                   zErr.iIsamCode, "CALCPERF8B", TRUE);
  }

  // lpfnTimer(2);
  // lpprTimerResult("PerformanceTimer.txt");

  //  fclose(fp);

  /* Free memory taken by pkey table */
  InitializePKeyTable(&zPKTable);

  // heapdump();

  return zErr;
} /* CalcPerf */

/**
** This is the function which is called to create missing performane scripts
* (pscrhdr & pscrdet).
** initializes the library, figures out current perf date and last date, create
** table of all the perfkeys for the account and then calls CalculatePerformance
** function which does the actual work.
**/
extern "C" DllExport ERRSTRUCT CreatePerformanceScripts(int iID,
                                                        long lCurrentDate,
                                                        long lStartDate) {
  ERRSTRUCT zErr;
  PKEYTABLE zPKTable;
  PERFCTRL zPCtrl;
  PARTPMAIN zPmain;
  PERFRULETABLE zRuleTable;
  static long lCurrentPerfDate = 0;
  static long lPricingDate = 0;
  long lLastPerfDate;
  char sMsg[80];
  BOOL bUseTrans;

  lpprInitializeErrStruct(&zErr);

  /*
  ** If user has not given any date, get the date(PricingDate from starsdate
  * table)
  ** or which we are doing performance.
  */
  if (lCurrentDate != 0)
    lCurrentPerfDate = lCurrentDate;
  else if (lCurrentPerfDate == 0) {
    lpprSelectStarsDate(&lCurrentPerfDate, &lPricingDate, &zErr);
    if (lCurrentPerfDate < 0)
      return (
          lpfnPrintError("Invalid Performance Date Returned By SelectStarsDate",
                         iID, 0, "", 509, 0, 0, "CREATESCRIPT1", FALSE));
  }

  zErr = InitializeCalcPerfLibrary();
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zErr = InitializeAppropriateLibrary("", "", 'S', -1, FALSE);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  lpprSelectOnePartPortmain(&zPmain, iID, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    if (zErr.iSqlError == SQLNOTFOUND)
      lpfnPrintError("Portfolio Not Found", iID, 0, "", 1, zErr.iSqlError, 0,
                     "CREATESCRIPT2", FALSE);

    return zErr;
  }

  /* Can't run performance on portfolio before its purge date */
  if (lStartDate < zPmain.lPurgeDate)
    lStartDate = zPmain.lPurgeDate;

  /* get all the perfkeys defined for the account */
  zPKTable.iCapacity = 0;
  zErr = GetAllPerfkeys(iID, &zPKTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /*
  ** Figure out Last Performance Date if user has not specified it.
  */
  if (lStartDate != 0)
    lLastPerfDate = lStartDate;
  else
    lLastPerfDate = zPmain.lInceptionDate - 1;

  /*
  ** Read perfctrl file and copy TaxCalc field to perfkey table. If this field
  ** is set to E(tax equivalent), A(after tax) or B(both) then for each key,
  ** pzTInfo needs to be allocated memory(array element same as NumDailyInfo)
  ** along with DailyInfo.
  */
  sprintf_s(sMsg, "Error Selecting Perfctrl For Account %s",
            zPmain.sUniqueName);
  lpprSelectPerfctrl(&zPCtrl, iID, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return (lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError,
                           zErr.iSqlError, zErr.iIsamCode, "CALCPERF7", FALSE));

  if (zPCtrl.lLastPerfDate != 0 && zPCtrl.lLastPerfDate < lLastPerfDate &&
      lLastPerfDate != zPmain.lPurgeDate) {
    if (lpfnIsItAMonthEnd(zPCtrl.lLastPerfDate))
      lLastPerfDate = zPCtrl.lLastPerfDate;
    else
      lLastPerfDate = lpfnLastMonthEnd(zPCtrl.lLastPerfDate);
  }

  if (lLastPerfDate < zPmain.lInceptionDate - 1)
    lLastPerfDate = zPmain.lInceptionDate - 1;

  strcpy_s(zPKTable.sCalcFlow, zPCtrl.sCalcFlow);
  strcpy_s(zPKTable.sPerfInterval, zPCtrl.sPerfInterval);
  strcpy_s(zPKTable.sDRDElig, zPCtrl.sDRDElig);
  zPKTable.lFlowStartDate = zPCtrl.lFlowStartDate;

  bUseTrans = (lpfnGetTransCount() == 0);
  if (bUseTrans)
    lpfnStartTransaction(); // use database transactions on account level

  __try {
    zErr = CalculatePerformance(&zPmain, lLastPerfDate, lCurrentPerfDate,
                                lLastPerfDate + 1, &zPKTable,
                                zPmain.sBaseCurrId, zPCtrl, TRUE, &zRuleTable);
  } __except (lpfnAbortTransaction(bUseTrans)) {
  }

  if (zErr.iSqlError != 0 ||
      (zErr.iBusinessError != 0 && zErr.iBusinessError != 500)) {
    if (bUseTrans)
      lpfnRollbackTransaction();

    sprintf_s(sMsg, "Performance For Account %s Failed", zPmain.sUniqueName);
    lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                   zErr.iIsamCode, "CALCPERF8A", FALSE);
  } else {
    if (bUseTrans)
      lpfnCommitTransaction();

    sprintf_s(sMsg, "Performance For Account %s Successful",
              zPmain.sUniqueName);
    lpfnPrintError(sMsg, iID, 0, "", zErr.iBusinessError, zErr.iSqlError,
                   zErr.iIsamCode, "CALCPERF8B", TRUE);
  }

  /* Free memory taken by pkey table */
  InitializePKeyTable(&zPKTable);

  return zErr;
} /* CreatePerformanceScripts */

/*
** This function is used to add all the perfkeys into the memory array
*/
ERRSTRUCT GetAllPerfkeys(int iID, PKEYTABLE *pzPKTable) {
  ERRSTRUCT zErr;
  PKEYSTRUCT zPKeyStruct;

  lpprInitializeErrStruct(&zErr);
  InitializePKeyTable(pzPKTable);

  /*
  ** InitializePKeyStruct function frees memory used by pzDInfo pointer if it's
  ** not NULL and iDInfoCount > 0. Since these variables are uninitialized, it
  * is
  ** very much likely that iDInfoCount will be > 0 and pzDInfo will be not NULL
  ** and in that situation, function will cause a memory fault when it tries
  ** to free memory which was never malloced. To avoid that, set iDInfoCount to
  * 0.
  */
  zPKeyStruct.iDInfoCapacity = zPKeyStruct.iWDInfoCapacity = 0;
  while (zErr.iSqlError == 0) {
    InitializePKeyStruct(&zPKeyStruct);

    lpprSelectPerfkeys(&zPKeyStruct.zPK, iID, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    if (zPKeyStruct.zPK.lDeleteDate < 0)
      zPKeyStruct.zPK.lDeleteDate = 0;

    // Find the script header that created the key, if not found return with an
    // error
    zPKeyStruct.iScrHDIndex =
        FindScrHdrByHdrNo(g_zSHdrDetTable, zPKeyStruct.zPK.lScrhdrNo);
    if (zPKeyStruct.iScrHDIndex < 0) {
      // we have to scroll through the rest of open cursor
      // in order to close it properly
      while (zErr.iSqlError == 0)
        lpprSelectPerfkeys(&zPKeyStruct.zPK, iID, &zErr);

      InitializePKeyStruct(&zPKeyStruct);
      return (lpfnPrintError("Invalid Script Header Found In The Key", iID,
                             zPKeyStruct.zPK.lPerfkeyNo, "P", 513, 0, 0,
                             "CALCPERF ADDPKEY2", FALSE));
    }

    zErr = AddPerfkeyToTable(pzPKTable, zPKeyStruct);
    if (zErr.iBusinessError != 0) {
      InitializePKeyStruct(&zPKeyStruct);
      return zErr;
    }
  } /* while no error */

  InitializePKeyStruct(&zPKeyStruct);
  return zErr;
} /* GetAllPerfkeys */

/**
** This function does all the work to calculate performance for the list of keys
** defined in the perfkey table, for the given date range. Calling program never
** calls this function directly, it is called by CalcPerf.
**/
ERRSTRUCT CalculatePerformance(PARTPMAIN *pzPmain, long lLastPerfDate,
                               long lCurrentPerfDate, long lEarliestPerfDate,
                               PKEYTABLE *pzPTable, char *sBaseCurrency,
                               PERFCTRL zPCtrl, BOOL bOnlyCreateScripts,
                               PERFRULETABLE *pzRuleTable) {
  ERRSTRUCT zErr;
  ACCDIVTABLE zADTable;
  ASSETTABLE2 zATable;
  HOLDINGTABLE zHTable;
  TRANSTABLE zTTable;
  PKEYASSETTABLE2 zPKeyAssetTable;
  PERFASSETMERGETABLE zPAMTable;
  int i, j, iDateIndex;
  long lLastMonthEnd;
  BOOL bAdjustPerfDate = false;

  lpprInitializeErrStruct(&zErr);

  /* This should not happen */
  if (lLastPerfDate >= lCurrentPerfDate)
    return (lpfnPrintError("Programming Error", pzPmain->iID, 0, "", 999, 0, 0,
                           "CALCPERF CALCPERF1", FALSE));

  // Initialize all the memory tables
  zADTable.iAccdivCreated = 0;
  InitializeAccdivTable(&zADTable);
  zATable.iAssetCreated = 0;
  InitializeAssetTable(&zATable);
  zHTable.iHoldingCreated = 0;
  InitializeHoldingTable(&zHTable);
  zTTable.iTransCreated = 0;
  InitializeTransTable(&zTTable);
  zPKeyAssetTable.iKeyCount = 0;
  InitializePKeyAssetTable(&zPKeyAssetTable);
  zPAMTable.iCapacity = 0;
  InitializePerfAssetMergeTable(&zPAMTable);

  // First, get all pending Accruals from accdiv records
  zErr = FillAccdivTable(pzPmain->iID, lLastPerfDate, lCurrentPerfDate,
                         &zATable, &zADTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /*
  ** Fetch all the trans and related assets records for the account and put
  ** them in their appropriate tables.
  */
  bAdjustPerfDate =
      (zSysSet.bDailyPerf && strcmp(zPCtrl.sPerfInterval, "D") == 0);

  zErr = FillTransAndAssetTables(pzPmain->iID, pzPmain->iVendorID,
                                 lLastPerfDate, lCurrentPerfDate, &zATable,
                                 &zTTable, bAdjustPerfDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }
  // lpfnTimer(3);

  /*
  ** Fetch all the holdings and related assets records for the account and put
  ** them in their appropriate tables.
  */
  zErr =
      FillHoldingAndAssetTables(pzPmain, lCurrentPerfDate, &zATable, &zHTable,
                                lLastPerfDate, lCurrentPerfDate, cCalcAccdiv);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /*
  ** Now that we have the Holdings and Accdiv records
  ** We then call GetAccruals to fill in the missing accrual records
  */
  if (cCalcAccdiv == 'Y') {
    zErr =
        GetAccruals(zADTable, &zATable, &zTTable, &zHTable, pzPmain->iVendorID,
                    lCurrentPerfDate, lLastPerfDate, lCurrentPerfDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  }
  // lpfnTimer(4);

  /* This now happens earlier, befor loading holdings - vay
  // Get all pending Accruals from accdiv records
  zErr = FillAccdivTable(pzPmain->iID, lLastPerfDate, lCurrentPerfDate,
  &zATable, &zADTable); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
  {
  CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
  pzRuleTable, &zADTable); return zErr;
  }
  */

  /* Get the assets associated with CurrId(of all the assets in the table) */
  zErr = GetCurrencyAsset(&zATable, zTTable, pzPmain->iVendorID, lLastPerfDate,
                          lCurrentPerfDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /*
  ** VI # 57941
  ** Get asset aggregation merge table filled. Even though DB table is more
  * generic, from code point of view creating specific memory table
  ** is easier. For now, only asset merge rule is implemented, so fill-in only
  * the asset merge table. As more performance aggregation
  ** rule (e.g. merge Equity and Global Equity segment) are implemented, more
  * "specific" rules will have to be implemented and tables need to be filled
  */
  zErr = FillPerfAssetMergeTable(&zPAMTable, &zATable, pzPmain->iVendorID,
                                 lLastPerfDate, lCurrentPerfDate);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  // the industry levels in the database are stored on the first day they are
  // valid and that's how they are retrieved and set in asset table. Fill the
  // industry levels in the table for the entire period
  FillLevelsForThePeriod(&zATable);

  // Create, if necessary, all the perfkeys for the account
  zErr = CreateDynamicPerfkeys(
      pzPTable, *pzPmain, lLastPerfDate, lCurrentPerfDate, zATable, pzRuleTable,
      zPAMTable, lEarliestPerfDate + 1, bOnlyCreateScripts);
  if (bOnlyCreateScripts || zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }
  // lpfnTimer(5);

  /* Count the number of active perfkeys for the account */
  j = 0;
  for (i = 0; i < pzPTable->iCount; i++) {
    if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate) == FALSE)
      j++;
  }
  /* If there is no active perfkey in the account, return with an error */
  if (j == 0) {
    strcpy_s(zPCtrl.sReprocessed, "Y");
    zPCtrl.lLastPerfDate = lCurrentPerfDate;
    lpprUpdatePerfctrl(zPCtrl, &zErr);
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return (lpfnPrintError("No Perfkey To Process", pzPmain->iID, 0, "", 500, 0,
                           0, "CALCPERF CALCPERF3", TRUE));
  }

  zErr = CreateDailyInfo(pzPTable, -1, lLastPerfDate, lCurrentPerfDate,
                         pzPmain->iReturnsToCalculate);
  if (zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /* create pkeyasset table to check which asset affects which perfkey */
  zErr = CreatePKeyAssetTable(&zPKeyAssetTable, *pzPTable, zATable, zPAMTable,
                              *pzRuleTable, lCurrentPerfDate, *pzPmain,
                              lLastPerfDate, lCurrentPerfDate);
  if (zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }
  // lpfnTimer(6);

  // For all the existing keys, get summdata records on LastPerfDate and
  // LastNondDate.
  zErr = GetLastAndBeginValues(pzPmain->iID, lLastPerfDate, pzPTable,
                               pzPmain->iReturnsToCalculate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }
  // lpfnTimer(7);

  /*
  ** Get the ending values and values on all the required month ends for all the
  * perfkeys in the table.
  ** If the account is doing tax related calculations, we need the begining
  * accraul broken into taxable and
  ** non taxable fields, so we have to read the holdings on begining date, if
  * account is not calculating tax
  ** related performance, the begining number will come from summdata.
  */
  for (i = lLastPerfDate; i <= lCurrentPerfDate; i++) {
    if (i == lCurrentPerfDate ||
        (lpfnIsItAMonthEnd(i) &&
         i != lLastPerfDate)) // ||
                              //(i == lLastPerfDate && lLastPerfDate >=
                              //pzPmain->lInceptionDate &&
                              //(strcmp(pzPTable->sTaxCalc, "E") == 0 ||
                              //strcmp(pzPTable->sTaxCalc, "A") == 0 ||
                              //strcmp(pzPTable->sTaxCalc, "B") == 0)))
    {
      zErr = GetHoldingValues(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                              *pzPTable, *pzRuleTable, pzPmain, zADTable,
                              zPAMTable, i, lLastPerfDate, lCurrentPerfDate,
                              cCalcAccdiv);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                           pzRuleTable, &zADTable, &zPAMTable);
        return zErr;
      }
    } // if CurrentPerfDate or a month end date
  } // loop through all the dates between CurrentPerfDate and LastPerfDate

  // lpfnTimer(8);

  /* If last date was a month end make it a period end and reset dayssincenond*/
  if (lpfnIsItAMonthEnd(lLastPerfDate)) {
    for (i = 0; i < pzPTable->iCount; i++) {
      pzPTable->pzPKey[i].pzDInfo[0].bPeriodEnd = TRUE;
      pzPTable->pzPKey[i].pzDInfo[0].lDaysSinceNond = 0;
    }
  }

  /*
  ** calculate flows, first calculate security and accrual flows, then calculate
  ** cash principal flow and then calculate cash income flow. This is done in
  ** three seperate function calls because transaction might be affecting three
  ** different assets. One security for which transaction occured, a second
  ** asset in which principal cash was paid and yet third asset for paying
  ** principal income.
  */
  for (i = 1; i < 4; i++) {
    zErr = CalculateFlow(*pzPTable, zTTable, zPKeyAssetTable, zATable,
                         *pzRuleTable, zPAMTable, i, lLastPerfDate,
                         lCurrentPerfDate, lEarliestPerfDate, *pzPmain);
    if (zErr.iBusinessError != 0) {
      CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                         pzRuleTable, &zADTable, &zPAMTable);
      return zErr;
    }
  } // i (flow type) = 1, 2, 3
  // lpfnTimer(9);

  // Generate Notional Flow, if needed
  zErr = GenerateNotionalFlow(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                              pzPTable, *pzRuleTable, pzPmain, zADTable,
                              zPAMTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /*
  ** Figure out if there is any month end or a 10% flow in any key, if either of
  ** the condition is true, get new market value, etc. on that date.
  */
  zErr = GetValuesOnFixedPoints(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                                pzPTable, *pzRuleTable, pzPmain, zADTable,
                                zPAMTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }
  // lpfnTimer(10);

  /* calculate weighted values for all the days in all the keys */
  zErr = CalculateWeightedValues(*pzPTable, TRUE, pzPmain->iReturnsToCalculate);
  if (zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /* calculate return and insert them in appropraite tables */
  zErr = CalculateReturnForAllKeys(*pzPTable, lLastPerfDate, lCurrentPerfDate,
                                   *pzPmain, zPCtrl, CalcSelected);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }
  // lpfnTimer(11);

  /*
  ** All the processing done so far does not apply to keys which are created
  ** using a perfrule for which WtdRecInd = 'W', calculate performance on
  ** these keys.
  */
  /*  zErr = WeightedAveragePerformance(pzPTable, zRuleTable, zPKeyAssetTable,
  zATable, lLastPerfDate, lCurrentPerfDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
  {
  CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
  &zRuleTable, &zADTable, &zPAMTable); return zErr;
  }*/

  zErr = FindParentPerfkeys(pzPTable, lLastPerfDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  /*
  ** Insert new perfkeys in the database and update last perf date in all
  ** perfkeys of the account.
  */
  for (i = 0; i < pzPTable->iCount; i++) {
    /*
    ** If the key is a parent key and it is for weighted average, don't change
    ** any of its field, accept it as it is.
    */
    if (strcmp(pzPTable->pzPKey[i].zPK.sParentChildInd, "P") == 0 &&
        strcmp(pzPTable->pzPKey[i].sRecordType, "W") == 0)
      // pzPTable->pzPKey[i].bWtdKey == TRUE)
      continue;

    pzPTable->pzPKey[i].bDeleteKey = FALSE;
    /*
    ** If a new key is incepted and deleted on the same day and it has no flow,
    ** it is useless, it is not inserted anywhere. Bur perfkey1 table already
    ** has it, so delete it from there.
    */
    if (pzPTable->pzPKey[i].zPK.lInitPerfDate ==
            pzPTable->pzPKey[i].zPK.lDeleteDate &&
        pzPTable->pzPKey[i].bNewKey == TRUE) {
      iDateIndex = GetDateIndex(pzPTable->pzPKey[i],
                                pzPTable->pzPKey[i].zPK.lInitPerfDate);
      if (iDateIndex < 0 || iDateIndex >= pzPTable->pzPKey[i].iDInfoCount)
        pzPTable->pzPKey[i].bDeleteKey = TRUE;
      else if (IsValueZero(
                   pzPTable->pzPKey[i].pzDInfo[iDateIndex].fNetFlow +
                       pzPTable->pzPKey[i].pzDInfo[iDateIndex].fNotionalFlow,
                   3))
        pzPTable->pzPKey[i].bDeleteKey = TRUE;
    } /* New key, incepted and deleted on the same day */

    /* If the key was deleted before today, don't update its last perf date */
    if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate) == TRUE)
      pzPTable->pzPKey[i].zPK.lLastPerfDate =
          pzPTable->pzPKey[i].zPK.lDeleteDate;
    else
      pzPTable->pzPKey[i].zPK.lLastPerfDate = lCurrentPerfDate;

    /*    lpprUpdateOldPerfkey(pzPTable->pzPKey[i].zPK, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
    &zRuleTable, &zADTable, &zPAMTable); return zErr;
    }*/
  }

  // now pending changes have to be sent from local datasets
  // back to SQL Server. If any of them fails, entire processing fails too and
  // account will be rolled back to previous state
  lpprInsertUnitValueBatch(NULL, 0, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  // if any MonthToDate UVs need to be recalculated due to
  // performance re-run for previous month - do it now
  if (!(zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval, "D") ==
                                  0)) // make sure it is not daily account
  {
    lLastMonthEnd = lpfnLastMonthEnd(lCurrentPerfDate);

    // make sure previous month was indeed re-run
    // and this is not 10% flow account
    if (lLastPerfDate < lLastMonthEnd &&
        !(zSysSet.fFlowThreshold != NAVALUE &&
          strcmp(pzPTable->sCalcFlow, "Y") == 0)) {
      lpprRecalcDailyUV(pzPmain->iID, lLastMonthEnd, lCurrentPerfDate, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                           pzRuleTable, &zADTable, &zPAMTable);
        return zErr;
      }
    }
  }

  // and then delete old unit values for previous month
  lpprDeleteMarkedUnitValue(pzPmain->iID, lLastPerfDate, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  zErr = UpdateAllPerfkeys(*pzPTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                       pzRuleTable, &zADTable, &zPAMTable);
    return zErr;
  }

  strcpy_s(zPCtrl.sReprocessed, "Y");
  zPCtrl.lLastPerfDate = lCurrentPerfDate;
  lpprUpdatePerfctrl(zPCtrl, &zErr);
  // lpfnTimer(12);
  /*
  lpprUpdateNewPerfkey(*pzPTable, &zErr);
  */

  CalcPerfInitialize(&zATable, &zHTable, &zTTable, &zPKeyAssetTable,
                     pzRuleTable, &zADTable, &zPAMTable);

  return zErr;
} /* calculateperformance */

/**
** This function is used to calculate netflow of all the transactions. Net flow
** can be Security Net Flow, Accrual Net Flow, Cash Net Flow and Income Net Flow
** WhichFlow arguments tells us which net flow to calculate, if it is 1 means
** calculate security and accrual net flow, it is 2 means calculate cash net
** flow, else if it is 3 means calculate income net flow.
**/
ERRSTRUCT CalculateFlow(PKEYTABLE zPKTable, TRANSTABLE zTTable,
                        PKEYASSETTABLE2 zPATable, ASSETTABLE2 zATable,
                        PERFRULETABLE zRuleTable, PERFASSETMERGETABLE zPAMTable,
                        int iWhichFlow, long lLastPerfDate,
                        long lCurrentPerfDate, long lEarliestPerfDate,
                        PARTPMAIN zPmain) {
  FLOWCALCSTRUCT zNFCS;
  ERRSTRUCT zErr;
  TRANS zTempTrans;
  DAILYINFO *pzDI;
  DAILYTAXINFO *pzTI;
  int i, j, k, l, iAssetIndex, iSTypeIndex, iDateIndex, iDInfoIndex;
  int iDateDelta, iEqPlusCKey, iFiPlusCKey, iSegment, iTCFlowCount;
  short iLongShort;
  char sPerfImpact[2], sPrimType[10], sSecType[10];
  double fBaseFlow1, fLocalFlow1, fBaseFlow2, fLocalFlow2, fBaseInc, fLocalInc,
      fBaseFees, fBaseCNFees;
  double fLocalFees, fLocalCNFees, fBaseSales, fLocalSales, fBasePurchases,
      fLocalPurchases, fTheFlow1, fTheFlow2;
  double fTheFees, fTheCNFees, fTheInc, fTheSales, fThePurchases,
      fBaseContributions, fLocalContributions;
  double fBaseWithdrawals, fLocalWithdrawals, fTheContributions,
      fTheWithdrawals;
  double fSecurityGL, fTotalGL, fMediumTermGL, fCurrencyGL, fShortTermGL,
      fLongTermGL, fEqPct, fFiPct;
  double fBaseAmort, fLocalAmort, fTheAmort, fLocalFeesOut, fBaseFeesOut,
      fTheFeesOut;
  BOOL bFixedInc, bAsofTrade, bSResult, bAResult, bTaxInfoRequired;
  BOOL bSpecialRule;

  lpprInitializeErrStruct(&zErr);
  iDateDelta = 0;
  fTheFlow1 = fTheFlow2 = fTheAmort = fTheFeesOut = fTheCNFees = 0.0;
  fTheFees = fTheInc = fTheSales = fThePurchases = fBaseContributions =
      fLocalContributions = 0.0;
  /* If there are no transaction in the table, nothing to do */
  if (zTTable.iNumTrans <= 0)
    return zErr;

  if (iWhichFlow != 1 && iWhichFlow != 2 && iWhichFlow != 3)
    return (lpfnPrintError("Invalid Flow To Calculate", zTTable.pzTrans[0].iID,
                           0, "", 999, 0, 0, "CALCPERF CALFLOW1", FALSE));

  bTaxInfoRequired = TaxInfoRequired(zPmain.iReturnsToCalculate);
  // Find out if special rule exists fro eq + eq cash and fi + fi cash
  iEqPlusCKey = iFiPlusCKey = -1;
  bSpecialRule = SpecialRuleForEquityAndFixedExists(zPKTable, zRuleTable);
  if (bSpecialRule) {
    iEqPlusCKey = FindKeyForASegmentType(zPKTable, EQUITYPLUSCASHSEGMENT,
                                         lLastPerfDate + 1);
    iFiPlusCKey = FindKeyForASegmentType(zPKTable, FIXEDPLUSCASHSEGMENT,
                                         lLastPerfDate + 1);
  } // if special eq + eq cash and/or fi + fi cash rule exists for this account

  iTCFlowCount = 0;
  for (i = 0; i < zTTable.iNumTrans; i++) {
    /* If transaction has no effect on performance, ignore it */
    k = zTTable.pzTrans[i].iTranTypeIndex;
    /*
    ** Out of all the trantypes with perfimpact = "X", only TS(transfer of
    ** security) and TC(transfer of currency) may have a performance impact.
    ** TS and TC will have a impact if SecXtend is "UP" or XSecXtend is "UP".
    ** TC will also have an impact if CurrAcctType is "4"(non-purpose loan) or
    ** XCurrAcctType is "4". These trades have only Security Flow.
    ** If PerfImpact is NOT "X", AcctType "4" or XAcctType "4" will have no
    ** effect on performance
    */
    if (strcmp(zTTypeTable.zTType[k].sPerfImpact, "X") == 0) {
      if (iWhichFlow != 1)
        continue;

      if (strcmp(zTTable.pzTrans[i].sTranType, "TS") != 0 &&
          strcmp(zTTable.pzTrans[i].sTranType, "TC") != 0)
        continue;
      else if (strcmp(zTTable.pzTrans[i].sSecXtend, "UP") != 0 &&
               strcmp(zTTable.pzTrans[i].sXSecXtend, "UP") != 0) {
        if (strcmp(zTTable.pzTrans[i].sTranType, "TS") == 0)
          continue;
        /* SB 5/22/2014 - Up until now Performance engine was not dealing with
        TC correctly. Since in most of the cases all currencies are classified
        the same way at Level1, 2 & 3, it never showed up as an issue, however,
        now that UBS GFO classifies USD and foreign currencies differently at
        level2 & 3, it is causing wrong performance numbers. CurracctType and
        XCurrAcctType are Neuberger ways of identifying transfer of currencies,
        we don't use this, so get rid of this condition and let TC transactions
        continue on in this function like other transaction types.
        else if (strcmp(zTTable.pzTrans[i].sCurrAcctType, "4") != 0 &&
        strcmp(zTTable.pzTrans[i].sXCurrAcctType, "4") != 0)
        continue;*/
      } /* if neither SecXtend nor XSecXtend is "UP" */
    } /* PerfImpact is X */
    else if ((strcmp(zTTable.pzTrans[i].sAcctType, "4") == 0 ||
              strcmp(zTTable.pzTrans[i].sXAcctType, "4") == 0) &&
             (strcmp(zTTable.pzTrans[i].sTranType, "FI") == 0 ||
              strcmp(zTTable.pzTrans[i].sTranType, "FO") == 0))
      continue;

    /*
    SB 5/22/2014 - since we need to calculate both contribution side and
    withdrawal side from the same TC transaction, we need to process TC twice in
    this for loop (for transactions). So, when FlowCount for TC is 1, use the
    transaction to calculate contribution side of the flow, when it's 2, use the
    same transaction to calculate withdrawal side of the flow
    */
    if (strcmp(zTTable.pzTrans[i].sTranType, "TC") == 0)
      iTCFlowCount++;

    bAsofTrade = FALSE;
    /* Skip the transaction, if it is of no use to us, else get its index */
    if (zTTable.pzTrans[i].lPerfDate <= lLastPerfDate ||
        zTTable.pzTrans[i].lPerfDate > lCurrentPerfDate) {
      /*
      ** If the transaction's perf date is earlier than the earliest date
      ** program is allowed to go to, give a warning and skip the transaction,
      ** else identify it as an AsOf trade and add its flow and cumulative
      ** flow(accounted for [CurrentPerfDate - Trans.PerfDate] days) in
      ** today's performance.
      */
      if (zTTable.pzTrans[i].lPerfDate < lEarliestPerfDate) {
        /* Print warning message only once. Ignore transaction every time */
        /*  if (iWhichFlow == 1)
        {
        sprintf_s(sMsg, "Transaction's PerfDate Is Earlier Than The
        Date(%d/%d/%d) " "Program Is Allowed To Go Back To", iMDY[0], iMDY[1],
        iMDY[2]);
        //            lpfnPrintError(sMsg, zTTable.pzTrans[i].iID,
        zTTable.pzTrans[i].lTransNo, "T",
        //
        999, 0, 0, "CALCPERF CALCNFLOW2", TRUE);
        }*/
        continue;
      } else {
        bAsofTrade = TRUE;

        iDateDelta = lCurrentPerfDate - zTTable.pzTrans[i].lPerfDate;
        iDateIndex = zPKTable.pzPKey[0].iDInfoCount - 1;
      }
    } /* Asoftransaction */
    else
      iDateIndex =
          GetDateIndex(zPKTable.pzPKey[0], zTTable.pzTrans[i].lPerfDate);

    /*
    ** Calculate all the flows. CalculateNetFlow function needs a trans struct
    ** so copy all the available fields from PartialTrans structure to a Trans
    ** structure (function does not use any field which we don't have)
    */
    CopyPartTransToTrans(zTTable.pzTrans[i], &zTempTrans);
    k = zTTable.pzTrans[i].iTranTypeIndex;
    strcpy_s(sPerfImpact, zTTypeTable.zTType[k].sPerfImpact);

    // If keys for either equity + cash or fixed + cash exist, get the
    // percentage
    if (iEqPlusCKey >= 0 || iFiPlusCKey >= 0) {
      zErr = GetEquityAndFixedPercent(zPKTable, zRuleTable, zPmain,
                                      zTempTrans.lPerfDate, &fEqPct, &fFiPct);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    } else
      fEqPct = fFiPct = 0;

    /* Get the right asset where this flow should go to */
    if (iWhichFlow == 1) {
      // iAssetIndex = zTTable.pzTrans[i].iSecAssetIndex;
      //  VI:57941 - transaction security or merged security's asset index
      iAssetIndex = CurrentOrMergedAssetIndexForToday(
          zPAMTable, zTTable.pzTrans[i].iSecAssetIndex,
          zTTable.pzTrans[i].lPerfDate);
      // For flow1, if the asset is not cash, make equity and fixed percent zero
      if (bSpecialRule &&
          (!IsThisASpecialAsset(
               zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
               CASHSEGMENT) &&
           !IsThisASpecialAsset(
               zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
               EQUITYCASHSEGMENT) &&
           !IsThisASpecialAsset(
               zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
               FIXEDCASHSEGMENT)))
        fEqPct = fFiPct = 0.0;
      else if (strcmp(zTTable.pzTrans[i].sXSecNo, "E.") ==
                   0 || // directed towards equity
               (bSpecialRule &&
                IsThisASpecialAsset(
                    zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
                    EQUITYCASHSEGMENT))) {
        fEqPct = 1.0;
        fFiPct = 0.0;
      } else if (strcmp(zTTable.pzTrans[i].sXSecNo, "B.") ==
                     0 || // directed towards fixed
                 (bSpecialRule &&
                  IsThisASpecialAsset(
                      zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
                      FIXEDCASHSEGMENT))) {
        fEqPct = 0.0;
        fFiPct = 1.0;
      }
      // fEqPct = fFiPct = 0;
    } else {
      /*if (iWhichFlow == 2)
              iAssetIndex = zTTable.pzTrans[i].iCashAssetIndex;
      else
              iAssetIndex = zTTable.pzTrans[i].iIncAssetIndex;*/
      // VI:57941 - cash security or merged security's asset index
      if (iWhichFlow == 2)
        iAssetIndex = CurrentOrMergedAssetIndexForToday(
            zPAMTable, zTTable.pzTrans[i].iCashAssetIndex,
            zTTable.pzTrans[i].lPerfDate);
      else
        iAssetIndex = CurrentOrMergedAssetIndexForToday(
            zPAMTable, zTTable.pzTrans[i].iIncAssetIndex,
            zTTable.pzTrans[i].lPerfDate);
      /*
      ** if the asset(not the currency's asset but the trade's security) is
      * equity, make
      ** fixed percent zero, if it is fixed, make equity percent zero, if it is
      * cash, leave
      ** the percentage unchanged, else(not equity, fixed or cash) make both of
      * them zero.
      */
      if (bSpecialRule &&
          (IsThisASpecialAsset(
               zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
               EQUITYSEGMENT) ||
           IsThisASpecialAsset(
               zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
               EQUITYCASHSEGMENT))) {
        fEqPct = 1.0; // 1.0
        fFiPct = 0.0;
      } // equity
      else if (bSpecialRule &&
               (IsThisASpecialAsset(
                    zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
                    FIXEDSEGMENT) ||
                IsThisASpecialAsset(
                    zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
                    FIXEDCASHSEGMENT))) {
        fEqPct = 0.0;
        fFiPct = 1.0; // 1.0
      } // fixed
      else if (bSpecialRule &&
               IsThisASpecialAsset(
                   zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex],
                   CASHSEGMENT)) {
        if (strcmp(zTTable.pzTrans[i].sXSecNo, "E.") ==
            0) // directed towards equity
        {
          fEqPct = 1.0; // 1.0
          fFiPct = 0.0;
        } else if (strcmp(zTTable.pzTrans[i].sXSecNo, "B.") ==
                   0) // directed towards fixed
        {
          fEqPct = 0.0;
          fFiPct = 1.0; // 1.0
        }
      } // cash
      else
        fEqPct = fFiPct = 0;
    } // whichflow = 2 or 3

    /* If it is a "TS" or "TC", it needs special processing */
    if (strcmp(zTempTrans.sTranType, "TC") == 0 ||
        strcmp(zTempTrans.sTranType, "TS") == 0) {
      // this is the case of TS/CR - moving a taxlot from UP to managed
      if (strcmp(zTempTrans.sSecXtend, "UP") == 0 &&
          strcmp(zTempTrans.sXSecXtend, "UP") != 0) {
        strcpy_s(zTempTrans.sSecNo, zTempTrans.sXSecNo);
        strcpy_s(zTempTrans.sWi, zTempTrans.sXWi);
        strcpy_s(zTempTrans.sCurrId, zTempTrans.sXCurrId);
        strcpy_s(zTempTrans.sAcctType, zTempTrans.sXAcctType);
        strcpy_s(zTempTrans.sCurrAcctType, zTempTrans.sXCurrAcctType);
        strcpy_s(zTempTrans.sSecXtend, "  ");
        strcpy_s(sPerfImpact, "C"); /* contribution */
        // iAssetIndex = zTTable.pzTrans[i].iXSecAssetIndex;
        iAssetIndex = CurrentOrMergedAssetIndexForToday(
            zPAMTable, zTTable.pzTrans[i].iXSecAssetIndex,
            zTTable.pzTrans[i].lPerfDate);
      }
      // this is the case of TS/DR - moving a taxlot from managed to UP
      else if (strcmp(zTempTrans.sSecXtend, "UP") != 0 &&
               strcmp(zTempTrans.sXSecXtend, "UP") == 0) {
        strcpy_s(zTempTrans.sXSecXtend, "  ");
        strcpy_s(sPerfImpact, "W"); /* withdrawal */
      }
      // SB 5/22/2014 - contribution side of the flow for TC transction
      else if (strcmp(zTempTrans.sTranType, "TC") == 0 && iTCFlowCount == 1)
      /* strcmp(zTempTrans.sCurrAcctType, "4") == 0 &&
         strcmp(zTempTrans.sXCurrAcctType, "4") != 0)*/
      {
        // SB 5/22/2014 -  both C & W flow for TC will be calculated once (as
        // part of security flow). TC will not have
        //									accrual
        //or cash flow.
        if (iWhichFlow != 1)
          continue;

        if (strcmp(zTempTrans.sDrCr, "DR") == 0)
          zTempTrans.fPcplAmt *= zTempTrans.fBaseXrate;

        strcpy_s(zTempTrans.sCurrId, zTempTrans.sXCurrId);
        strcpy_s(zTempTrans.sAcctType, zTempTrans.sXAcctType);
        strcpy_s(zTempTrans.sCurrAcctType, zTempTrans.sXCurrAcctType);
        // strcpy_s(zTempTrans.sSecXtend, "  ");

        if (strcmp(zTempTrans.sCurrId, zPmain.sBaseCurrId) ==
            0) // && strcmp(zTempTrans.sDrCr, "CR") == 0)
          zTempTrans.fBaseXrate = 1.0;

        strcpy_s(sPerfImpact, "C"); /* contribution */
        // iAssetIndex = zTTable.pzTrans[i].iXCashAssetIndex;
        //  VI:57941 - cross reference cash security or merged security's asset
        //  index
        iAssetIndex = CurrentOrMergedAssetIndexForToday(
            zPAMTable, zTTable.pzTrans[i].iXCashAssetIndex,
            zTTable.pzTrans[i].lPerfDate);
      }
      // SB 5/22/2014 - contribution side of the flow for TC transction
      else if (strcmp(zTempTrans.sTranType, "TC") == 0 && iTCFlowCount == 2)
      /*strcmp(zTempTrans.sCurrAcctType, "4") != 0 &&
         strcmp(zTempTrans.sXCurrAcctType, "4") == 0)*/
      {
        if (iWhichFlow != 1)
          continue;

        // strcpy_s(zTempTrans.sSecXtend, "  ");
        if (strcmp(zTempTrans.sDrCr, "DR") == 0)
          zTempTrans.fPcplAmt *= zTempTrans.fSecBaseXrate;
        else
          zTempTrans.fPcplAmt *= zTempTrans.fBaseXrate;

        if (strcmp(zTempTrans.sCurrId, zPmain.sBaseCurrId) == 0 &&
            strcmp(zTempTrans.sDrCr, "DR") == 0)
          zTempTrans.fBaseXrate = 1.0;

        if (strcmp(zTempTrans.sDrCr, "DR") == 0)
          zTempTrans.fBaseXrate = zTempTrans.fSecBaseXrate;

        strcpy_s(sPerfImpact, "W"); /* withdrawal */
        // iAssetIndex = zTTable.pzTrans[i].iCashAssetIndex;
        //  VI:57941 - cash security or merged security's asset index
        iAssetIndex = CurrentOrMergedAssetIndexForToday(
            zPAMTable, zTTable.pzTrans[i].iCashAssetIndex,
            zTTable.pzTrans[i].lPerfDate);
      } else
        continue;
    } /* Transfer of Currency or Transfer of security */

    /*
    ** Find the daily info index for the asset under consideration. Last
    * argument to the function to find PerfDate
    ** in daily asset info is FALSE which means perf date doesn't have to exist
    * in the array, as long as one date
    ** (highest date on or prior to perfdate) is found that's enough, if no date
    * is found then it means this asset
    ** has no valid daily infor (which at this time is limited to industry
    * level) and hence no perfkey for this
    ** asset will pass. So if no date is found in the array, no need to process
    * further, skip this transaction
    */
    iDInfoIndex = FindDailyInfoOffset(zTTable.pzTrans[i].lPerfDate,
                                      zATable.pzAsset[iAssetIndex], FALSE);
    if (iDInfoIndex < 0 ||
        iDInfoIndex >= zATable.pzAsset[iAssetIndex].iDailyCount)
      continue;

    /* Find out if this asset is a fixed inc or not, required for tax calc */
    iSTypeIndex = zATable.pzAsset[iAssetIndex].iSTypeIndex;
    if (strcmp(zPSTypeTable.zSType[iSTypeIndex].sPrimaryType, "B") == 0)
      bFixedInc = TRUE;
    else
      bFixedInc = FALSE;

    zErr = lpfnCalcNetFlow(zTempTrans, sPerfImpact, &zNFCS, true);
    if (zErr.iBusinessError != 0)
      return zErr;

    // Get the appropriate flow depending on whichflow we are currently
    // interested in(iWhichFlow).
    fBasePurchases = fBaseSales = fLocalPurchases = fLocalSales = 0.0;
    fBaseWithdrawals = fBaseContributions = fLocalWithdrawals =
        fLocalWithdrawals = 0.0;
    fBaseFlow1 = fBaseFlow2 = fLocalFlow1 = fLocalFlow2 = 0.0;
    fBaseInc = fBaseFees = fBaseCNFees = fLocalInc = fLocalFees = fLocalCNFees =
        0.0;
    fBaseAmort = fLocalAmort = 0.0;
    fLocalFeesOut = fBaseFeesOut = 0.0;

    if (iWhichFlow == 1) {
      fBaseFlow1 = zNFCS.fBPcplSecFlow + zNFCS.fBIncomeSecFlow;
      fLocalFlow1 = zNFCS.fLPcplSecFlow + zNFCS.fLIncomeSecFlow;
      fBaseFlow2 = zNFCS.fBAccrualFlow;
      fLocalFlow2 = zNFCS.fLAccrualFlow;

      fBaseFees = zNFCS.fBSecFees;
      fLocalFees = zNFCS.fLSecFees;
      fBaseInc = zNFCS.fBSecIncome;
      fLocalInc = zNFCS.fLSecIncome;
      if (strcmp(zTempTrans.sTranType, "FA") == 0) {
        fLocalFeesOut = zNFCS.fLSecFees;
        fBaseFeesOut = zNFCS.fBSecFees;
      } else if (strcmp(zTempTrans.sTranType, "CN") == 0) {
        fBaseCNFees = zNFCS.fBSecFees;
        fLocalCNFees = zNFCS.fLSecFees;
      }
    } // security and accrual flow
    else if (iWhichFlow == 2) {
      fBaseFlow1 = zNFCS.fBPcplCashFlow;
      fLocalFlow1 = zNFCS.fLPcplCashFlow;
    } // principal cash flow
    else {
      fBaseFlow1 = zNFCS.fBIncomeCashFlow;
      fLocalFlow1 = zNFCS.fLIncomeCashFlow;
    } // income cash flow

    if (strcmp(sPerfImpact, "C") == 0) {
      fLocalContributions = fLocalFlow1 + fLocalFlow2;
      fBaseContributions = fBaseFlow1 + fBaseFlow2;
    } else if (strcmp(sPerfImpact, "W") == 0) {
      fLocalWithdrawals = (fLocalFlow1 + fLocalFlow2) * -1;
      fBaseWithdrawals = (fBaseFlow1 + fBaseFlow2) * -1;
      // fLocalContributions = (fLocalFlow1 + fLocalFlow2);
      // fBaseContributions	=	(fBaseFlow1 + fBaseFlow2);
    } else if (strcmp(sPerfImpact, "O") == 0) {
      fLocalPurchases = fLocalFlow1;
      fBasePurchases = fBaseFlow1;
    } else if (strcmp(sPerfImpact, "S") == 0) {
      fLocalSales = fLocalFlow1 * -1;
      fBaseSales = fBaseFlow1 * -1;
    } else if (strcmp(sPerfImpact, "A") == 0) {
      fLocalAmort = fLocalFlow1;
      fBaseAmort = fBaseFlow1;
      // amortization does not have real impact on flow, so, reset all fields
      // back to 0
      fBaseFlow1 = fLocalFlow1 = fBaseFlow2 = fLocalFlow2 = 0;
      fBaseCNFees = fLocalCNFees = fBaseFees = fLocalFees = fBaseFeesOut =
          fLocalFeesOut = fBaseInc = fLocalInc;
    } else if (strcmp(sPerfImpact, "T") == 0) {
      if (strcmp(zTTypeTable.zTType[k].sDrCr, "CR") == 0) {
        fLocalContributions = fLocalFlow1 + fLocalFlow2;
        fBaseContributions = fBaseFlow1 + fBaseFlow2;
      } else {
        fLocalWithdrawals = (fLocalFlow1 + fLocalFlow2) * -1;
        fBaseWithdrawals = (fBaseFlow1 + fBaseFlow2) * -1;
      }
    }

    // If it is a closing trade and tax calculations are required and this
    // client is setup to have g/l adjustment as part of the calculation,
    // calculate gain/loss
    if (bTaxInfoRequired && zSysSet.bGLTaxAdj &&
        strcmp(zTTypeTable.zTType[k].sTranCode, "C") == 0 && iWhichFlow == 1) {
      zErr.iBusinessError = lpfnCalcGainLoss(
          zTempTrans, zTTypeTable.zTType[k].sTranCode,
          zTTypeTable.zTType[k].lSecImpact, sPrimType, sSecType,
          zPmain.sBaseCurrId, &fCurrencyGL, &fSecurityGL, &fShortTermGL,
          &fMediumTermGL, &fLongTermGL, &fTotalGL);
      if (zErr.iBusinessError != 0)
        return zErr;
    } else
      fCurrencyGL = fSecurityGL = fShortTermGL = fMediumTermGL = fLongTermGL =
          fTotalGL = 0;

    // If it is a reversal trade, reverse the effect of all the flows (and other
    // fields) beware about gain/loss handling: gains should be recorded as
    // negative (and RV of gains is positive) and opposite for losses. This is
    // due the fact that gains are expense for tax purpose and should bring
    // returns down.
    if (zTTable.pzTrans[i].bReversal == TRUE) {
      fBaseFlow1 *= -1;
      fBaseFlow2 *= -1;
      fBaseFees *= -1;
      fBaseCNFees *= -1;
      fBaseInc *= -1;
      fBaseSales *= -1;
      fBasePurchases *= -1;
      fBaseContributions *= -1;
      fBaseWithdrawals *= -1;
      fBaseAmort *= -1;
      fBaseFeesOut *= -1;

      fLocalFlow1 *= -1;
      fLocalFlow2 *= -1;
      fLocalFees *= -1;
      fLocalCNFees *= -1;
      fLocalInc *= -1;
      fLocalSales *= -1;
      fLocalPurchases *= -1;
      fLocalContributions *= -1;
      fLocalWithdrawals *= -1;
      fLocalAmort *= -1;
      fLocalFeesOut *= -1;
    } else {
      fCurrencyGL *= -1;
      fSecurityGL *= -1;
      fShortTermGL *= -1;
      fMediumTermGL *= -1;
      fLongTermGL *= -1;
      fTotalGL *= -1;
    } /* If transaction is a reversal */

    iLongShort = LongShortBitForTrans(zTTable.pzTrans[i]);

    /*
    ** check all the keys which are effected by this transaction's asset and
    ** add calculated flow to all the appropriate keys.
    */
    for (j = 0; j < zPATable.iKeyCount; j++) {
      if (IsKeyDeleted(zPKTable.pzPKey[j].zPK.lDeleteDate))
        continue;

      pzDI = &(zPKTable.pzPKey[j].pzDInfo[iDateIndex]);
      l = (j * zPATable.iNumAsset) + iAssetIndex;
      bSResult = zPATable.pzStatusFlag[l].piResult[iDInfoIndex] & iLongShort;
      bAResult = zPATable.pzStatusFlag[l].piResult[iDInfoIndex] & iLongShort;

      // if calculating flow for Total Portfolio on PS/SL of unsupervised
      // position, then instead of affecting Purchases/Sales fields, update
      // Withdrawals/Contributions SB 9/11/2014 - VI 55457 - Income should also
      // be considered a contribution
      if ((zPKTable.pzPKey[j].zPK.sTotalRecInd[0] == 'T') &&
          (strcmp(zTempTrans.sSecXtend, "UP") == 0)) {
        if (strcmp(sPerfImpact, "O") == 0) {
          fBaseWithdrawals = fBasePurchases * -1;
          fLocalWithdrawals = fLocalPurchases * -1;
          fBasePurchases = 0;
          fLocalPurchases = 0;
        } else if (strcmp(sPerfImpact, "S") == 0) {
          fBaseContributions = fBaseSales * -1;
          fLocalContributions = fLocalSales * -1;
          fBaseSales = 0;
          fLocalSales = 0;
        } else if (strcmp(sPerfImpact, "I") == 0 && iWhichFlow == 3) {
          fBaseContributions = fBaseFlow1;
          fLocalContributions = fLocalFlow1;
        }
      } // if unsupervised

      /*
      ** If the asset pointed by SecAssetIndex(whichflow = 1), CashAssetIndex(
      ** whichflow = 2) or IncAssetIndex(whichflow = 3) has first flag set for
      ** current key(in PkeyAssetTable) then add the base or local flow
      ** (depending on current key) to transaction's effective date's flow.
      ** Also add fees and income to effective date's fees and income, resp.
      */
      if (bSResult) {
        if (strcmp(zPKTable.pzPKey[j].zPK.sCurrProc, "L") == 0) {
          fTheFlow1 = fLocalFlow1;
          fTheFlow2 = fLocalFlow2;
          fTheFees = fLocalFees;
          fTheCNFees = fLocalCNFees;
          fTheInc = fLocalInc;
          fTheSales = fLocalSales;
          fThePurchases = fLocalPurchases;
          fTheContributions = fLocalContributions;
          fTheWithdrawals = fLocalWithdrawals;
          fTheAmort = fLocalAmort;
          fTheFeesOut = fLocalFeesOut;
        } else {
          fTheFlow1 = fBaseFlow1;
          fTheFlow2 = fBaseFlow2;
          fTheFees = fBaseFees;
          fTheCNFees = fBaseCNFees;
          fTheInc = fBaseInc;
          fTheSales = fBaseSales;
          fThePurchases = fBasePurchases;
          fTheContributions = fBaseContributions;
          fTheWithdrawals = fBaseWithdrawals;
          fTheAmort = fBaseAmort;
          fTheFeesOut = fBaseFeesOut;
        }

        /*
        ** Fees are added only to Total, if the key is not for Total sector,
        ** Fees should be zero.
        */
        if (zPKTable.pzPKey[j].zPK.sTotalRecInd[0] != 'T') {
          fTheFees = 0.0;
          fTheFeesOut = 0.0;
          fTheCNFees = 0.0;

          // SB 3/24/08 - Since fees outside the account (FA) have no cash
          // impact, it should not have any flow either. Here flow only for non
          // total segments is being zeroed-out, though even for total the
          // effect is zero but that already gets achieved by security and cash
          // flow canceling each other out
          if (strcmp(zTempTrans.sTranType, "FA") == 0)
            fTheFlow1 = 0;
        }

        /*
        ** Withholdings(PerfImpact - 'T') are added only to Total, if the key is
        ** not for Total sector, SecImpact should be zero.
        */
        if (strcmp(sPerfImpact, "T") == 0 &&
            zPKTable.pzPKey[j].zPK.sTotalRecInd[0] != 'T') {
          fTheFlow1 = 0.0;
          fTheFlow2 = 0.0;
          fTheContributions = 0.0;
          fTheWithdrawals = 0.0;
        }

        pzDI->fTodayFlow += fTheFlow1;
        pzDI->fTodayFees += fTheFees;
        pzDI->fTodayCNFees += fTheCNFees;
        pzDI->fPurchases += fThePurchases;
        pzDI->fSales += fTheSales;
        pzDI->fContributions += fTheContributions;
        pzDI->fWithdrawals += fTheWithdrawals;
        pzDI->fTodayFeesOut += fTheFeesOut;

        /*
        ** If the effect of an asof trade is to be reflected only in today's
        ** performance, add the effect(flow * datedelta) in today's cum values.
        */
        if (bAsofTrade) {
          pzDI->fCumFlow += fTheFlow1 * iDateDelta;
          pzDI->fCumFees += fTheFees * iDateDelta;
          pzDI->fCumFeesOut += fTheFeesOut * iDateDelta;
          pzDI->fCumCNFees += fTheCNFees * iDateDelta;
        } /* If it is an asof trade and it's effect have to applied today */
      } /* if first flag is TRUE */

      /*
      ** If whichflow is 1 and the asset's second flag is set for the current
      ** key(in PkeyAssetTable) then add the base or local flow (depending on
      ** current key) to transaction's effective date's flow.
      */
      if (iWhichFlow == 1 && bAResult) {
        pzDI->fTodayFlow += fTheFlow2;
        pzDI->fTodayIncome += fTheInc;

        /*
        ** If the effect of an asof trade is to be reflected only in today's
        ** performance, add the effect(flow/Inc * datedelta) in today's cum
        * values.
        */
        if (bAsofTrade) {
          pzDI->fCumFlow += fTheFlow2 * iDateDelta;
          pzDI->fCumIncome += fTheInc * iDateDelta;
        } // AsofTrade
      } /* if whichflow == 1 and second flag is TRUE */

      iSegment = SegmentTypeForTheKey(zPKTable.pzPKey[j].zPK);
      if (iSegment == CASHSEGMENT)
        pzDI->fContributions += fTheInc;

      zPKTable.pzPKey[j].pzDInfo[iDateIndex] = *pzDI;

      if (bTaxInfoRequired) {
        pzTI = &(zPKTable.pzPKey[j].pzTInfo[iDateIndex]);
        /*
        ** If tax work is required and trancode for the transaction is I(ncome)
        * or T(withholding),
        ** do taxable work
        */
        if (bAResult && iWhichFlow == 1 &&
            (strcmp(sPerfImpact, "I") == 0 || strcmp(sPerfImpact, "T") == 0)) {
          if (strcmp(zTTypeTable.zTType[k].sTranType, "WH") == 0) {
            /* If security is tax free(at federal level), send a warning and
             * continue */
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") == 0)
              lpfnPrintError("WARNING !! 'WH' Transaction Should Not Exist For "
                             "A Tax Free Security.",
                             0, 0, "", 1003, 0, 0, "CALCPERF CALFLOW2", TRUE);
            else
              pzTI->fTdyFedinctaxWthld += fTheFlow1;

            /* state tax calcualtions have been disabled - 5/12/06 - vay
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableState, "N") != 0)
            pzTI->fTdyStinctaxWthld   += fTheFlow1;
            */

          } /* if WH transaction */
          else if (strcmp(zTTypeTable.zTType[k].sTranType, "RR") == 0) {
            /* If security is tax free(at federal level), send a warning and
             * continue */
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") == 0)
              lpfnPrintError("WARNING !! 'RR' Transaction Should Not Exist For "
                             "A Tax Free Security.",
                             0, 0, "", 1003, 0, 3, "CALCPERF CALFLOW2 ", FALSE);
            else
              pzTI->fTdyFedtaxRclm += fTheFlow1;

            /* state tax calcualtions have been disabled - 5/12/06 - vay
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableState, "N") != 0)
            pzTI->fTdySttaxRclm  += fTheFlow1;
            */

          } /* if RR transaction */
          else if (strcmp(zTTypeTable.zTType[k].sTranType, "AR") == 0) {
            /* Do Work for Federal Level */
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") ==
                0) { /* tax free at federal level */
              if (bFixedInc)
                pzTI->fFedetaxIncRclm += fTheFlow1;
              else
                pzTI->fFedetaxDivRclm += fTheFlow1;
            } /* If taxfree security */
            else { /* taxable at federal level */
              if (bFixedInc)
                pzTI->fFedataxIncRclm += fTheFlow1;
              else
                pzTI->fFedataxDivRclm += fTheFlow1;
            } /* If taxable(at federal level) security */

            // Do Work For State Level
            /* state tax calcualtions have been disabled - 5/12/06 - vay
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableState, "N") == 0)
            { // tax free at state level
            if (bFixedInc)
            pzTI->fStetaxIncRclm += fTheFlow1;
            else
            pzTI->fStetaxDivRclm += fTheFlow1;
            } // If taxfree security
            else
            { // Taxable at state level
            if (bFixedInc)
            pzTI->fStataxIncRclm += fTheFlow1;
            else
            pzTI->fStataxDivRclm += fTheFlow1;
            } // If taxable(at federal level) security
            */

          } /* if AR transaction */
          else /* != 'WH' and != 'RR' and != 'AR' */
          {
            /* Do Work for Federal Level */
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") == 0)
              pzTI->fTdyFedetaxInc += fTheFlow1;
            else if ((strcmp(zPKTable.sDRDElig, "Y") == 0 &&
                      strcmp(zATable.pzAsset[iAssetIndex].sDRDElig, "Y") == 0))
              pzTI->fTdyFedataxInc += fTheFlow1 * DRDELIGPERCENTAGE;
            else
              pzTI->fTdyFedataxInc += fTheFlow1;

            /* Do Work For State Level */
            /* state tax calcualtions have been disabled - 5/12/06 - vay
            if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableState, "N") == 0)
            pzTI->fTdyStetaxInc += fTheFlow1;
            else
            pzTI->fTdyStataxInc += fTheFlow1;
            */
          } /* if !'WH' && !AR && !RR transaction */
        } /* If tax calculations are required && an Income transaction */
        else if (bAResult && iWhichFlow == 1 &&
                 (strcmp(sPerfImpact, "O") == 0 ||
                  strcmp(sPerfImpact, "S") == 0 ||
                  strcmp(sPerfImpact, "C") == 0 ||
                  strcmp(sPerfImpact, "W") == 0)) {
          // Add the purchased/sold accrual as the income flow
          /* Do Work for Federal Level */
          if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") == 0)
            pzTI->fTdyFedetaxInc += fTheFlow2;
          else
            pzTI->fTdyFedataxInc += fTheFlow2;

          /* Do Work For State Level */
          /* state tax calcualtions have been disabled - 5/12/06 - vay
          if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableState, "N") == 0)
          pzTI->fTdyStetaxInc += fTheFlow2;
          else
          pzTI->fTdyStataxInc += fTheFlow2;
          */
        } // purchased or sold accrual
        else if (bAResult && iWhichFlow == 1 &&
                 (strcmp(sPerfImpact, "A") == 0)) {
          // Add the amortized/accreted amount
          /* Do Work for Federal Level */
          if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") == 0)
            pzTI->fTdyFedetaxAmort += fTheAmort;
          else
            pzTI->fTdyFedataxAmort += fTheAmort;

          /* Do Work For State Level */
          /* state tax calcualtions have been disabled - 5/12/06 - vay
          if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableState, "N") == 0)
          pzTI->fTdyStetaxAmort += fTheAmort;
          else
          pzTI->fTdyStataxAmort += fTheAmort;
          */
        } // amortization/accretioon

        if (bAResult && iWhichFlow == 1 &&
            strcmp(zTTypeTable.zTType[k].sTranCode, "C") == 0) {
          // iAssetIndex = zTTable.pzTrans[i].iSecAssetIndex;
          // taxwork at federal level
          // if (strcmp(zATable.pzAsset[iAssetIndex].sTaxableCountry, "N") == 0)
          if (strcmp(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex]
                         .sTaxableCountry,
                     "N") == 0) {
            pzTI->fTdyFedetaxStrgl += fShortTermGL;
            pzTI->fTdyFedetaxLtrgl += fLongTermGL;
            pzTI->fTdyFedetaxCrrgl += fCurrencyGL;
          } // taxfree at federal level
          else {
            pzTI->fTdyFedataxStrgl += fShortTermGL;
            pzTI->fTdyFedataxLtrgl += fLongTermGL;
            pzTI->fTdyFedataxCrrgl += fCurrencyGL;
          } // taxable at federal level

          // taxwork at state level
          /* state tax calcualtions have been disabled - 5/12/06 - vay
          if
          (strcmp(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex].sTaxableState,
          "N") == 0)
          {
          pzTI->fTdyStetaxStrgl += fShortTermGL;
          pzTI->fTdyStetaxLtrgl += fLongTermGL;
          pzTI->fTdyStetaxCrrgl += fCurrencyGL;
          } // taxfree at state level
          else
          {
          pzTI->fTdyStataxStrgl += fShortTermGL;
          pzTI->fTdyStataxLtrgl += fLongTermGL;
          pzTI->fTdyStataxCrrgl += fCurrencyGL;
          } // taxable at state level
          */
        } // if closing transaction and taxable work is required

        zPKTable.pzPKey[j].pzTInfo[iDateIndex] = *pzTI;
      } // if taxable work is required
    } /* for j < zPATable.iCount */

    /*
    ** If this transaction is supposed to have flows in Equity + Equity Cash
    * sector then EqPct is
    ** non zero else it is set to zero. So if there is a key for Equity + Equity
    * Cash and
    ** percent of flow which should go to this key is not zero, add flow(and
    * other relevent
    ** fields) to this key.
    */
    if (iEqPlusCKey != -1 && fEqPct > 0.0 &&
        strcmp(sPerfImpact, "T") != 0) // perfimpact "T" affects only total
    {
      zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fTodayFlow +=
          (fBaseFlow1 + fBaseFlow2) * fEqPct;
      zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fTodayIncome +=
          fBaseInc * fEqPct;
      zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fPurchases +=
          fBasePurchases * fEqPct;
      zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fSales +=
          fBaseSales * fEqPct;
      if (strcmp(sPerfImpact, "I") == 0)
        zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fContributions +=
            (fBaseContributions + fBaseFlow1) * fEqPct;
      else
        zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fContributions +=
            fBaseContributions * fEqPct;
      zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fWithdrawals +=
          fBaseWithdrawals * fEqPct;

      if (bAsofTrade) {
        zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fCumFlow +=
            (fBaseFlow1 + fBaseFlow2) * iDateDelta * fEqPct;
        zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fCumIncome +=
            fBaseInc * iDateDelta * fEqPct;
      }
    } // Special additional processing for Equity + Equity Cash key

    /*
    ** If this transaction is supposed to have flows in Fixed + Fixed cash
    * sector then FiPct is
    ** non zero else it is set to zero. So if there is a key for Fixed + Fixed
    * Cash and
    ** percent of flow which should go to this key is not zero, add flow(and
    * other relevent
    ** fields) to this key.
    */
    if (iFiPlusCKey != -1 && fFiPct > 0.0 &&
        strcmp(sPerfImpact, "T") != 0) // perfimpact "T" affects only total
    {
      zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fTodayFlow +=
          (fBaseFlow1 + fBaseFlow2) * fFiPct;
      zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fTodayIncome +=
          fBaseInc * fFiPct;
      zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fPurchases +=
          fBasePurchases * fFiPct;
      zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fSales +=
          fBaseSales * fFiPct;
      if (strcmp(sPerfImpact, "I") == 0)
        zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fContributions +=
            (fBaseContributions + fBaseFlow1) * fFiPct;
      else
        zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fContributions +=
            fBaseContributions * fFiPct;
      zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fWithdrawals +=
          fBaseWithdrawals * fFiPct;

      if (bAsofTrade) {
        zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fCumFlow +=
            (fBaseFlow1 + fBaseFlow2) * iDateDelta * fFiPct;
        zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fCumIncome +=
            fBaseInc * iDateDelta * fFiPct;
      }

    } // Special additional processing for Fixed + Fixed Cash key

    /*
    SB 5/24/2014 - Unlike other transaction type, we need to calculate both
    contribution and withdrawal sides of the flow from the TC transactions.
    Moreover, both sides will be for different securities and hence affect
    different perfkeys. So, the easiest way to do this is process the TC
    transaction twice. First time calculate the contribution side and second
    time calculate the withdrawal side. So, if this is the first time TC
    transaction was processed, force the "for" loop over transactions to go back
    to process the same transaction again. If we are done with both sides to TC,
    then simply reset the varaible that keeps track of which side of TC has been
    just processed.
    */
    if (strcmp(zTTable.pzTrans[i].sTranType, "TC") == 0) {
      if (iTCFlowCount == 1)
        i--;
      else if (iTCFlowCount >= 2)
        iTCFlowCount = 0;
    }
  } /* for i < zTTable.iNumTrans */

  return zErr;
} /* calculateflow */

/**
** This function goes through PerfkeyTable and figures out if the net flow on
** any day for any key is 10% or more of the begining market. If it is then for
** that day, it reads the ending market value, accr int and accr div from the
** holdings file. These ending values become begining values for the subsequent
** dates and the process of finding 10% flow(compared to new values) continues
** until we reach end of the period. This way the total date period gets
** divided into Num 10% flow dates + 1 subperiods. E.g. if two days are found
** when flow is 10% or more of the market value, then we have three subperiods,
** which are, starting date to first 10% flow date, first 10% flow date + 1 to
** second 10% flow date and second 10% flow date + 1 to end date.
**/
ERRSTRUCT GetValuesOnFixedPoints(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable,
                                 TRANSTABLE *pzTTable,
                                 PKEYASSETTABLE2 *pzPATable,
                                 PKEYTABLE *pzPTable, PERFRULETABLE zRuleTable,
                                 PARTPMAIN *pzPmain, ACCDIVTABLE zADTable,
                                 PERFASSETMERGETABLE zPAMTable) {
  ERRSTRUCT zErr;
  double fEMV, fAbsTodayFlow;
  int i, j, iStartIndex, iKeyCount;
  BOOL bTotalAlreadyExisted;
  BOOL bIsItAMarketHoliday;
  long lLastPDate, lCurrentPDate, lTempDate;

  lpprInitializeErrStruct(&zErr);

  iStartIndex = 0;

  /* Calculate 10% of the begining market value for each key */
  for (i = 0; i < pzPTable->iCount; i++)
    Calc10PercentOfMV(&pzPTable->pzPKey[i], -1, zSysSet.fFlowThreshold);

  i = pzPTable->pzPKey[0].iDInfoCount;
  lLastPDate = pzPTable->pzPKey[0].pzDInfo[0].lDate;
  lCurrentPDate = pzPTable->pzPKey[0].pzDInfo[i - 1].lDate;

  /*
  ** The first date in all the keys is LastPerfDate, ignore that date and start
  ** with the next day and go upto the CurrentPerfDate - 1(values for the
  ** current perf date for all the keys, Ending Values, is already in the array)
  ** If any of the date is a month end, then get market value, acc int,
  ** etc for all the keys on that day. If it is not then check whether any key
  ** has 10% flow, if it does, get new values for that key on that day.
  ** If the account is incepted in LastPerfDate and CurrentPerfDate range then
  ** force a roll and valuation on the inception date. If any of the new key
  ** being incepted in this run have market value or accruals on this day,
  ** this should be the inception date for that key.
  */
  for (j = 1; j < pzPTable->pzPKey[0].iDInfoCount; j++) {
    lTempDate = pzPTable->pzPKey[0].pzDInfo[j].lDate;
    if (lTempDate == pzPmain->lInceptionDate) {
      zErr =
          GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable, *pzPTable,
                           zRuleTable, pzPmain, zADTable, zPAMTable, lTempDate,
                           lLastPDate, lCurrentPDate, cCalcAccdiv);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      /*
      ** If any of the key has a market value/accrued, then it should be
      ** incepted today.
      */
      for (i = 0; i < pzPTable->iCount; i++) {
        /* If key is deleted, no need to process it */
        if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate) == TRUE)
          continue;

        /* Nothing to do with Weighted Key */
        //        if (pzPTable->pzPKey[i].bWtdKey == TRUE)
        if (strcmp(pzPTable->pzPKey[i].sRecordType, "W") == 0)
          continue;

        fEMV = pzPTable->pzPKey[i].pzDInfo[j].fMktVal +
               pzPTable->pzPKey[i].pzDInfo[j].fAccrInc +
               pzPTable->pzPKey[i].pzDInfo[j].fAccrDiv;

        if (!IsValueZero(fEMV, 3)) // != 0.0)
        {
          /* If key is deleted, no need to process it */
          if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate) == TRUE)
            continue;

          pzPTable->pzPKey[i].pzDInfo[j].bPeriodEnd = TRUE;
          pzPTable->pzPKey[i].zPK.lInitPerfDate = lTempDate;
          pzPTable->pzPKey[i].zPK.lLndPerfDate = lTempDate;

          ChangeTotalInceptDateIfRequired(*pzPTable, lTempDate);

          /* there is new market value, so calculate new 10% of market value*/
          Calc10PercentOfMV(&pzPTable->pzPKey[i], j, zSysSet.fFlowThreshold);
        } /* If the key has market value or accrued */
      } /* loop on all the accounts in the table */
    } /* If it is the day, account is incepted */
    else if (lpfnIsItAMonthEnd(
                 lTempDate)) //|| lTempDate == pzPmain->iFiscalMonth)
    {
      // fprintf(fp, "%d is a monthend date. Gettig holding Values\n",
      // lTempDate);
      zErr =
          GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable, *pzPTable,
                           zRuleTable, pzPmain, zADTable, zPAMTable, lTempDate,
                           lLastPDate, lCurrentPDate, cCalcAccdiv);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      iKeyCount = pzPTable->iCount;
      for (i = 0; i < iKeyCount; i++) {
        /* If key is deleted, no need to process it */
        if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate) == TRUE)
          continue;

        /* Nothing to do with Weighted Key */
        if (strcmp(pzPTable->pzPKey[i].sRecordType, "W") == 0)
          continue;

        /* Today's ending value */
        fEMV = pzPTable->pzPKey[i].pzDInfo[j].fMktVal +
               pzPTable->pzPKey[i].pzDInfo[j].fAccrInc +
               pzPTable->pzPKey[i].pzDInfo[j].fAccrDiv;
        /*
        ** If the Endind MV(incluing accruals) is zero, the key will be marked
        * deleted in
        ** "CalculateReturnForAllKeys" function, in that case set the DeleteKey
        * variable to
        ** TRUE. Don't set the DeleteDate yet, because if we do that rest of the
        * functions
        ** will ignore this key which we don't want untill the end of the
        * program.
        */
        // fprintf(fp, "i : %d, ScrhdrNo: %d, EMV : %f, bDeletekey: %d, KCopied:
        // %d\n", i, pzPTable->pzPKey[i].zPK.lScrhdrNo, fEMV,
        // pzPTable->pzPKey[i].bDeleteKey, pzPTable->pzPKey[i].bKeyCopied);
        if (pzPTable->pzPKey[i].bDeleteKey == TRUE) {
          /*
          ** If the current key is to be deleted and a new matching key is
          ** required(new inception) but has not been incepted yet, create it
          */
          if (pzPTable->pzPKey[i].bKeyCopied == FALSE &&
              pzPTable->pzPKey[i].zPK.lInitPerfDate <= lTempDate &&
              !IsValueZero(fEMV, 3)) // != 0.0)
          // SB 4/28/99 - Atleast right now I don't care about one day return
          // (pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow != 0.0 || fEMV ))
          {
            // fprintf(fp, "Copying key %d, starting on %d = %d\n", i, j,
            // lTempDate);
            zErr =
                CopyAnExistingKey(pzPTable, i, j, pzPmain->iReturnsToCalculate);
            if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
              return zErr;

            pzPTable->pzPKey[i].bKeyCopied = TRUE;

            /* if the key just created is not total then check if total account
             * needs to be created */
            if (strcmp(pzPTable->pzPKey[i].zPK.sTotalRecInd, "T") != 0) {
              zErr = CopyTotalIfRequired(pzPTable, j,
                                         pzPmain->iReturnsToCalculate);
              if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
                return zErr;
            }
          } /* Another copy of key to be deleted has not been created yet */
          continue;
        } /* Key has been marked for deletion */
        else if (IsValueZero(fEMV, 3) &&
                 IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow, 2)) {
          if (pzPTable->pzPKey[i].zPK.lInitPerfDate > lTempDate)
            continue;

          pzPTable->pzPKey[i].bDeleteKey = TRUE;
          //					fprintf(fp, "Deleting key %d on
          //%d = %d\n", i, j, lTempDate);
        } /* Ending MV is zero, mark the key for deletion */
        else if (pzPTable->pzPKey[i].bNewKey == TRUE &&
                 pzPTable->pzPKey[i].bDeleteKey == FALSE &&
                 pzPTable->pzPKey[i].zPK.lInitPerfDate > lTempDate &&
                 IsValueZero(fEMV, 3) &&
                 !IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow, 2)) {
          pzPTable->pzPKey[i].zPK.lInitPerfDate = lTempDate;
          pzPTable->pzPKey[i].zPK.lLndPerfDate = lTempDate;

        } else if (pzPTable->pzPKey[i].bNewKey == TRUE &&
                   pzPTable->pzPKey[i].bDeleteKey == FALSE &&
                   pzPTable->pzPKey[i].zPK.lInitPerfDate > lTempDate &&
                   !IsValueZero(fEMV, 3)) // != 0.0)
        //								 pzPTable->pzPKey[i].zPK.lInitPerfDate
        //== lCurrentPDate && 	 SB 4/28/99 - Atleast right now I don't care about
        //one day return      (pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow != 0.0
        //|| fEMV != 0.0))
        {
          pzPTable->pzPKey[i].zPK.lInitPerfDate = lTempDate;
          pzPTable->pzPKey[i].zPK.lLndPerfDate = lTempDate;

          if (IsValueZero(fEMV, 3)) // == 0.0)
            pzPTable->pzPKey[i].bDeleteKey = TRUE;
          // fprintf(fp, "Changed Initperfdate for key %d on %d = %d\n", i, j,
          // lTempDate);
        }
        /*
        ** If the key's inception date is greater than the date we are working
        * on,
        ** it's not a period end for this key.
        */
        else if (pzPTable->pzPKey[i].zPK.lInitPerfDate > lTempDate)
          continue;

        pzPTable->pzPKey[i].pzDInfo[j].bPeriodEnd = TRUE;
        pzPTable->pzPKey[i].zPK.lMePerfDate = lTempDate;
        /*
        ** Don't want to update lastnondaily perfdate yet because IndexValues
        ** (ROR_MONTHLY/ROR_DAILY) have not been fetched yet, if it is updated
        ** write now, we will never find the right index values for all keys.
        */
        /*pzPTable->pzPKey[i].zPK.lLndPerfDate = lTempDate;*/

        /* there is new market value, so calculate new 10% of market value*/
        Calc10PercentOfMV(&pzPTable->pzPKey[i], j, zSysSet.fFlowThreshold);
      } /* for i < NumPKey */
    } /* if a month end  or a fiscal date */
    else {
      bIsItAMarketHoliday =
          (lpfnIsItAMarketHoliday(lTempDate, zSysSet.sSysCountry) == 1 ? TRUE
                                                                       : FALSE);

      /*
      ** If holdings values are required in the following loop, it is required
      ** only for the "ith" key, still get the values for all key, this will
      ** save roll and valuation time later if another key requires these values
      ** on the same day(new keys incepted on same day or keys having 10% flow
      ** on the same day).
      */
      iKeyCount = pzPTable->iCount;
      for (i = 0; i < iKeyCount; i++) {
        if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate))
          continue;

        /* Nothing to do with Weighted Key */
        //        if (pzPTable->pzPKey[i].bWtdKey == TRUE)
        if (strcmp(pzPTable->pzPKey[i].sRecordType, "W") == 0)
          continue;

        /*
        ** If it is a new key, its inception date is the first day it
        ** has a non zero flow.
        */
        if (pzPTable->pzPKey[i].bNewKey == TRUE &&
            pzPTable->pzPKey[i].bDeleteKey == FALSE &&
            pzPTable->pzPKey[i].zPK.lInitPerfDate > lTempDate &&
            (!IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fTodayCNFees, 3) ||
             !IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fNotionalFlow, 3) ||
             !IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow, 3) ||
             !IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fTodayFees,
                          3))) // != 0.0)
        {
          zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable,
                                  *pzPTable, zRuleTable, pzPmain, zADTable,
                                  zPAMTable, lTempDate, lLastPDate,
                                  lCurrentPDate, cCalcAccdiv);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;

          /* Today's ending value */
          fEMV = pzPTable->pzPKey[i].pzDInfo[j].fMktVal +
                 pzPTable->pzPKey[i].pzDInfo[j].fAccrInc +
                 pzPTable->pzPKey[i].pzDInfo[j].fAccrDiv;

          /*fprintf(fp, "Getting MV on %d, For Key %d which has today's flow of
          %f and MV: %f\n", lTempDate, i,
          pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow, fEMV);*/
          // 11/14/05 - new key will be incepted today even if it is only one
          // day return, i.e. if it has a non zero flow. If Market Value becomes
          // zero, key will be marked for deletion
          pzPTable->pzPKey[i].pzDInfo[j].bPeriodEnd = TRUE;
          pzPTable->pzPKey[i].zPK.lInitPerfDate = lTempDate;
          pzPTable->pzPKey[i].zPK.lLndPerfDate = lTempDate;

          ChangeTotalInceptDateIfRequired(*pzPTable, lTempDate);
          // there is new market value, so calculate new 10% of market value
          Calc10PercentOfMV(&pzPTable->pzPKey[i], j, zSysSet.fFlowThreshold);

          if (IsValueZero(fEMV, 3))
            pzPTable->pzPKey[i].bDeleteKey = TRUE;

          continue;
        } /* New key with a non zero today's flow */

        /*
        ** If the current key is to be deleted and a new matching key is
        ** required(new inception) but has not been incepted yet, create it
        ** but only if there is ending market value
        */
        if (pzPTable->pzPKey[i].bDeleteKey == TRUE &&
            pzPTable->pzPKey[i].bKeyCopied == FALSE &&
            pzPTable->pzPKey[i].zPK.lInitPerfDate <= lTempDate &&
            (!IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow, 3) ||
             !IsValueZero(pzPTable->pzPKey[i].pzDInfo[j].fNotionalFlow, 3))

                ) // != 0.0)
        {
          zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable,
                                  *pzPTable, zRuleTable, pzPmain, zADTable,
                                  zPAMTable, lTempDate, lLastPDate,
                                  lCurrentPDate, cCalcAccdiv);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;

          fEMV = pzPTable->pzPKey[i].pzDInfo[j].fMktVal +
                 pzPTable->pzPKey[i].pzDInfo[j].fAccrInc +
                 pzPTable->pzPKey[i].pzDInfo[j].fAccrDiv;

          zErr =
              CopyAnExistingKey(pzPTable, i, j, pzPmain->iReturnsToCalculate);
          Calc10PercentOfMV(&pzPTable->pzPKey[pzPTable->iCount - 1], j,
                            zSysSet.fFlowThreshold);
          if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
            return zErr;

          pzPTable->pzPKey[i].bKeyCopied = TRUE;

          /* SB 7/22/09
          ** Earlier the for key being marked deleted because market value is
          * zero was happening after
          ** CopyTotalIfRequired function, however that was wrong in the case
          * when a new total key was being
          ** copied becuase in that case the toptal key was the last key and
          * that's what was being marked
          ** deleted rather the key just got copied from CopyKeyIfRequired
          * function. This was later causing
          ** key violation issues. Now moved the statement before
          * CopyTotalIfRequired function to fix key violations.
          ** Also, if the key just created was already for total account, no
          * need to call CopyTotalIfRequired
          ** function to create total again.
          */
          // if copied key has not produced EMV it should be marked for
          // deleteion right away
          if (IsValueZero(fEMV, 3))
            pzPTable->pzPKey[pzPTable->iCount - 1].bDeleteKey = TRUE;

          if (strcmp(pzPTable->pzPKey[i].zPK.sTotalRecInd, "T") != 0) {
            zErr =
                CopyTotalIfRequired(pzPTable, j, pzPmain->iReturnsToCalculate);
            if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
              return zErr;
          }

          /*fprintf(fp, "Copying Key %d on %d = %d, this is NOT a monthend.
          TodayFlow - %f. Total Keys After Creating\n", i, j, lTempDate,
          pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow, pzPTable->iCount);*/

        } /* Another copy of key to be deleted has not been created yet */

        // if System-wide option for 10% flow is set (i.e. has valid value)
        // and portfolio is set to have 10% flow calcs on it and the date on
        // which the 10% flow calculation should start is on or earlier than
        // today OR system and portfolio both set to allow daily performance
        // then do the rest of the processing, otherwise, skip the rest of the
        // processing
        if (!((zSysSet.fFlowThreshold != NAVALUE &&
               strcmp(pzPTable->sCalcFlow, "Y") == 0 &&
               pzPTable->lFlowStartDate <= lTempDate) ||
              (zSysSet.bDailyPerf &&
               strcmp(pzPTable->sPerfInterval, "D") == 0)))
          continue;

        // if it is non-business day - can't really do much, skip
        // (we don't have MV or AI on weekends)
        // NOTE:
        // 1)	payment transactions occuring over weekend will have their
        // PerfDate adjusted so
        //	  they will be accounted for either on Monday; or, if this is
        //EOM, as part of EOM 	  calculations above
        // 2) ...

        if (bIsItAMarketHoliday)
          continue;

        /* If today's flow is negative, make it +ve for comparision */
        if (pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow +
                pzPTable->pzPKey[i].pzDInfo[j].fNotionalFlow >=
            0.0)
          fAbsTodayFlow = pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow +
                          pzPTable->pzPKey[i].pzDInfo[j].fNotionalFlow;
        else
          fAbsTodayFlow = (pzPTable->pzPKey[i].pzDInfo[j].fTodayFlow +
                           pzPTable->pzPKey[i].pzDInfo[j].fNotionalFlow) *
                          -1.0;

        /* if 10% flow date or Daily Performance, read mkt val, acc int, acc div
         */
        if ((fAbsTodayFlow >= pzPTable->pzPKey[i].fAbs10PrcntMV &&
             !IsValueZero(fAbsTodayFlow, 3)) ||
            (zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval, "D") == 0)) {
          zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable,
                                  *pzPTable, zRuleTable, pzPmain, zADTable,
                                  zPAMTable, lTempDate, lLastPDate,
                                  lCurrentPDate, cCalcAccdiv);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;

          pzPTable->pzPKey[i].pzDInfo[j].bPeriodEnd = TRUE;

          /* Today's ending value */
          fEMV = pzPTable->pzPKey[i].pzDInfo[j].fMktVal +
                 pzPTable->pzPKey[i].pzDInfo[j].fAccrInc +
                 pzPTable->pzPKey[i].pzDInfo[j].fAccrDiv;

          if (IsValueZero(fEMV, 3)) {
            if (pzPTable->pzPKey[i].zPK.lInitPerfDate > lTempDate)
              pzPTable->pzPKey[i].pzDInfo[j].bPeriodEnd = FALSE;
            else
              pzPTable->pzPKey[i].bDeleteKey = TRUE;
          }

          /* there is new market value, so calculate new 10% of market value*/
          Calc10PercentOfMV(&pzPTable->pzPKey[i], j, zSysSet.fFlowThreshold);
        } /* if 10% flow date */
      } /* for i < iNumKeys */
    } /* if not a month end date */
  } /* for j < iNumDays */

  // Find the earliest date  segment is incepting and if for some reason
  // inception date on (a newly created) total portfolio is later than that,
  // then push it back to match the earliest incepted segment
  lTempDate = lCurrentPDate;
  for (i = 0; i < pzPTable->iCount; i++) {
    if (pzPTable->pzPKey[i].bNewKey == TRUE &&
        lTempDate > pzPTable->pzPKey[i].zPK.lInitPerfDate) {
      // Found a new key with inception date earlier than other new keys, if
      // any. Now try to find out where at the time of segment inception, there
      // alread was (which may get deleted at a later date) a total portfolio or
      // not. If there already was a total portfolio, ignore the inception date
      // from this segment
      bTotalAlreadyExisted = FALSE;
      for (j = 0; j < pzPTable->iCount; j++) {
        if (strcmp(pzPTable->pzPKey[j].zPK.sTotalRecInd, "T") != 0)
          continue;

        if (IsKeyDeleted(pzPTable->pzPKey[j].zPK.lDeleteDate))
          continue;

        if (pzPTable->pzPKey[j].zPK.lInitPerfDate <=
            pzPTable->pzPKey[i].zPK.lInitPerfDate)
          bTotalAlreadyExisted = TRUE;
      }

      // If total portfolio already existed the the time the segment was
      // created, don't care about it's inception date
      if (!bTotalAlreadyExisted)
        lTempDate = pzPTable->pzPKey[i].zPK.lInitPerfDate;
    } // found a new key with inception earlier than other new keys, if any
  }
  if (lTempDate != lCurrentPDate)
    ChangeTotalInceptDateIfRequired(*pzPTable, lTempDate);

  return zErr;
} /* GetValuesOnFixedPoints */

/**
** Function that calculates absolute value of 10% of market value + accruals
** If the date index given to this function is -1 then it use begin MV, else
** uses MV at jth array item.
**/
int Calc10PercentOfMV(PKEYSTRUCT *pzPKey, int iDateIndex, double fRate) {
  if (iDateIndex < -1 || iDateIndex > pzPKey->iDInfoCount)
    return -1;

  if (fRate == NAVALUE)
    pzPKey->fAbs10PrcntMV = MKT_VAL_INVALID;
  else if (iDateIndex != -1)
    pzPKey->fAbs10PrcntMV = (pzPKey->pzDInfo[iDateIndex].fMktVal +
                             pzPKey->pzDInfo[iDateIndex].fAccrInc +
                             pzPKey->pzDInfo[iDateIndex].fAccrDiv) *
                            fRate;
  else
    pzPKey->fAbs10PrcntMV =
        (pzPKey->fBeginMV + pzPKey->fBeginAI + pzPKey->fBeginAD) * fRate;

  /* If the number is negative, turn it into positive */
  if (pzPKey->fAbs10PrcntMV < 0.0)
    pzPKey->fAbs10PrcntMV *= -1.0;

  return 0;
} /* Calc10PercentOfMV */

/**
** Function to make sure a segment is not created before Total record.
**/
int ChangeTotalInceptDateIfRequired(PKEYTABLE zPTable, long lSegmentInitDate) {
  int i, j;

  j = GetDateIndex(zPTable.pzPKey[0], lSegmentInitDate);
  if (j >= 0 && j < zPTable.pzPKey[0].iDInfoCount) {
    for (i = 0; i < zPTable.iCount; i++) {
      if (IsKeyDeleted(zPTable.pzPKey[i].zPK.lDeleteDate) == TRUE)
        continue;

      if (zPTable.pzPKey[i].bNewKey &&
          strcmp(zPTable.pzPKey[i].zPK.sTotalRecInd, "T") == 0 &&
          zPTable.pzPKey[i].zPK.lInitPerfDate > lSegmentInitDate) {
        zPTable.pzPKey[i].zPK.lInitPerfDate = lSegmentInitDate;
        zPTable.pzPKey[i].pzDInfo[j].bPeriodEnd = TRUE;
      }
    } /* search for total */
  } /* If a new init_perf_date was identified */

  return (0);
} /* ChangeTotalInceptDateIfRequired */

/**
** This function insert a new key in the memory table and database which is
** for the same scrhdr_no as an existing key. This is done only if the
** existing key is about to get deleted(MV, flows, etc goes to zero) and then
** another key(with the same scrhdr_no) has to get created within the time
** range the program is run for. E.g. if the performance is called with strat
** date of 12/31/97 and end date is 3/20/98 and portfolio is sold out of equity
** on 1/31/98 and then buys back some security in equity on 2/16, the existing
** key for equity should get deleted on 1/31/98 and a new key for equity should
** start on 2/16. So this function will be called to create a copy of equity
** on 2/16/98.
**/
ERRSTRUCT CopyAnExistingKey(PKEYTABLE *pzPKTable, int iKeyIndex, int iDateIndex,
                            int iReturnsToCalculate) {
  ERRSTRUCT zErr;
  PKEYSTRUCT zNewKey;
  int i, iLast;

  lpprInitializeErrStruct(&zErr);

  if (iKeyIndex < 0 || iKeyIndex >= pzPKTable->iCount || iDateIndex < 0 ||
      iDateIndex >= pzPKTable->pzPKey[iKeyIndex].iDInfoCount)
    return (lpfnPrintError("Programming Error", pzPKTable->pzPKey[0].zPK.iID, 0,
                           "", 999, 0, 0, "CALCPERF COPYKEY1", FALSE));
  zNewKey.iDInfoCapacity = zNewKey.iWDInfoCapacity = 0;
  InitializePKeyStruct(&zNewKey);
  zNewKey.zPK.iID = pzPKTable->pzPKey[iKeyIndex].zPK.iID;
  zNewKey.zPK.iPortfolioID = pzPKTable->pzPKey[iKeyIndex].zPK.iPortfolioID;
  zNewKey.zPK.lRuleNo = pzPKTable->pzPKey[iKeyIndex].zPK.lRuleNo;
  zNewKey.zPK.lScrhdrNo = pzPKTable->pzPKey[iKeyIndex].zPK.lScrhdrNo;
  strcpy_s(zNewKey.zPK.sCurrProc, pzPKTable->pzPKey[iKeyIndex].zPK.sCurrProc);
  strcpy_s(zNewKey.zPK.sTotalRecInd,
           pzPKTable->pzPKey[iKeyIndex].zPK.sTotalRecInd);
  strcpy_s(zNewKey.zPK.sParentChildInd,
           pzPKTable->pzPKey[iKeyIndex].zPK.sParentChildInd);
  strcpy_s(zNewKey.zPK.sDescription,
           pzPKTable->pzPKey[iKeyIndex].zPK.sDescription);
  //  iLast = pzPKTable->pzPKey[iKeyIndex].iDInfoCount - 1;
  //  zNewKey.zPK.lInitPerfDate =
  //  pzPKTable->pzPKey[iKeyIndex].pzDInfo[iLast].lDate;
  zNewKey.zPK.lInitPerfDate =
      pzPKTable->pzPKey[iKeyIndex].pzDInfo[iDateIndex].lDate;
  zNewKey.lParentRuleNo = pzPKTable->pzPKey[iKeyIndex].lParentRuleNo;
  //  zNewKey.bWtdKey = pzPKTable->pzPKey[iKeyIndex].bWtdKey;
  strcpy_s(zNewKey.sRecordType, pzPKTable->pzPKey[iKeyIndex].sRecordType);
  zNewKey.iScrHDIndex = pzPKTable->pzPKey[iKeyIndex].iScrHDIndex;
  zNewKey.bNewKey = TRUE;

  zErr = AddPerfkeyToTable(pzPKTable, zNewKey);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  iLast = pzPKTable->pzPKey[iKeyIndex].iDInfoCount - 1;
  zErr = CreateDailyInfo(pzPKTable, pzPKTable->iCount - 1,
                         pzPKTable->pzPKey[iKeyIndex].pzDInfo[0].lDate,
                         pzPKTable->pzPKey[iKeyIndex].pzDInfo[iLast].lDate,
                         iReturnsToCalculate);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    return zErr;

  iLast = pzPKTable->iCount - 1;
  for (i = iDateIndex; i < pzPKTable->pzPKey[iKeyIndex].iDInfoCount; i++) {
    pzPKTable->pzPKey[iLast].pzDInfo[i] =
        pzPKTable->pzPKey[iKeyIndex].pzDInfo[i];

    if (TaxInfoRequired(iReturnsToCalculate))
      pzPKTable->pzPKey[iLast].pzTInfo[i] =
          pzPKTable->pzPKey[iKeyIndex].pzTInfo[i];
  }

  pzPKTable->pzPKey[iLast].pzDInfo[iDateIndex].bPeriodEnd = TRUE;

  /*  zErr = InsertPerfkey(zNewKey, &pzPKTable->pzPKey[iLast].lPerfkeyNo);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
  return zErr;
  */

  //  CopyPkeyStructToPerfkey (&zPerfkey, &zNewKey );

  /*  lpprInsertPerfkey(zNewKey.zPK, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
  return zErr;*/

  return zErr;
} /* CopyAnExistingKey */

/**
** This function is used immediately after "CopyAnExistingKey" is called to
** make sure that there is never a case when a Segment Key starts before a
** Total key. "CopyAnExistingKey" function is called only when a key gets
** marked as deleted and then another key for the same ScrhdrNo is required(
** both the things should happen at some dates between LastPerfDate and
** CurrentPerfDate). If Total key also gets deleted and it has not reappeared
** before a Segment key is created, this function will make a copy of the
** deleted Total key to make sure that no segment key exist before a total.
**/
ERRSTRUCT CopyTotalIfRequired(PKEYTABLE *pzPKTable, int iDIndex,
                              int iReturnsToCalculate) {
  ERRSTRUCT zErr;
  int i, iTIndex;
  double fEMV = 0;

  lpprInitializeErrStruct(&zErr);

  iTIndex = -1;
  for (i = 0; i < pzPKTable->iCount; i++) {
    if (strcmp(pzPKTable->pzPKey[i].zPK.sTotalRecInd, "T") == 0) {
      /* If key is deleted, skip it */
      if (IsKeyDeleted(pzPKTable->pzPKey[i].zPK.lDeleteDate))
        continue;
      /*
      ** If the live key is not marked to be deleted at the end of this run
      ** then we have a live total key, no need to do anything
      */
      else if (pzPKTable->pzPKey[i].bDeleteKey == FALSE)
        return zErr;
      /* If the Key has already been copied as a new key, skip it */
      else if (pzPKTable->pzPKey[i].bKeyCopied == TRUE)
        continue;
      /*
      ** If there's already a non-deleted key with same inception date as the
      * new one being asked to copy,
      ** then we already have a key, no need to create another one.
      */
      else if (pzPKTable->pzPKey[i].zPK.lInitPerfDate ==
               pzPKTable->pzPKey[i].pzDInfo[iDIndex].lDate)
        return zErr;
      else if (pzPKTable->pzPKey[i].zPK.lInitPerfDate <
               pzPKTable->pzPKey[i].pzDInfo[iDIndex].lDate) {
        fEMV = pzPKTable->pzPKey[i].pzDInfo[iDIndex].fMktVal +
               pzPKTable->pzPKey[i].pzDInfo[iDIndex].fAccrInc +
               pzPKTable->pzPKey[i].pzDInfo[iDIndex].fAccrDiv;

        if (pzPKTable->pzPKey[i].pzDInfo[iDIndex].bPeriodEnd &&
            IsValueZero(fEMV, 2))
          return zErr;
        else
          iTIndex = i;
      } else
        iTIndex = i;
    } /* If total key */
  } /* for i < pzPKTable->iCount */

  if (iTIndex == -1) {
    if (!CalcSelected)
      return (lpfnPrintError("Programming Error", pzPKTable->pzPKey[0].zPK.iID,
                             0, "", 999, 0, 0, "CALCPERF COPYTOTAL", FALSE));
  } else {
    zErr = CopyAnExistingKey(pzPKTable, iTIndex, iDIndex, iReturnsToCalculate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    pzPKTable->pzPKey[iTIndex].bKeyCopied = TRUE;
  }

  return zErr;
} /* CopyTotalIfRequired */

/**
** This function is used to calculate weighted flow, income and fees for all
** the keys in the table.
**/
ERRSTRUCT CalculateWeightedValues(PKEYTABLE zPKTable, BOOL bSpecialCase,
                                  int iReturnsToCalculate) {
  ERRSTRUCT zErr;
  int i, j;
  DAILYINFO *pzPrevDI, *pzNewDI;
  DAILYTAXINFO *pzPrevTI, *pzNewTI;

  lpprInitializeErrStruct(&zErr);

  for (i = 0; i < zPKTable.iCount; i++) {
    /* If key is deleted, no need to process it */
    if (IsKeyDeleted(zPKTable.pzPKey[i].zPK.lDeleteDate) == TRUE)
      continue;

    for (j = 1; j < zPKTable.pzPKey[i].iDInfoCount; j++) {
      /*
      ** SB 5/27/98, added an extra check to take care of following situation:
      ** On the day of inception of a portfolio,  there is a flow in a key but
      ** the market value is zero(this can happen when inception date of the
      ** portfolio is changed, e.g. for portfolio "33170021" inception day is
      ** 4/15/98 but there are already some free recieves prior to that, on
      ** 4/15/98 there are couple of free delivers which make Equity Market
      ** Value on 4/15/98 equal to Zero, but there is flow from these free
      ** delivers), so the key will not be created on that day. Couple of days
      ** (4/17/98 in our example) later there is a flow and on that day Key is
      ** being created. If we don't have the following check(if the day of flow
      ** is less than inception day, ignore the flow), flow on the inception
      ** day will be wrong(it will include the flows of free delivers also).
      */
      if (zPKTable.pzPKey[i].pzDInfo[j].lDate <
          zPKTable.pzPKey[i].zPK.lInitPerfDate)
        continue;

      pzPrevDI = &(zPKTable.pzPKey[i].pzDInfo[j - 1]);
      pzNewDI = &(zPKTable.pzPKey[i].pzDInfo[j]);

      /* New net, cum and wtd Flow */
      NewNetCumWtd(pzPrevDI->fNetFlow, pzPrevDI->fCumFlow, pzPrevDI->bPeriodEnd,
                   pzPrevDI->lDaysSinceNond, pzNewDI->fTodayFlow,
                   &pzNewDI->fNetFlow, &pzNewDI->fCumFlow, &pzNewDI->fWtdFlow,
                   bSpecialCase);

      /* New net, cum and wtd Income */
      NewNetCumWtd(pzPrevDI->fIncome, pzPrevDI->fCumIncome,
                   pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                   pzNewDI->fTodayIncome, &pzNewDI->fIncome,
                   &pzNewDI->fCumIncome, &pzNewDI->fWtdInc, bSpecialCase);

      /* New net, cum and wtd Fees */
      NewNetCumWtd(pzPrevDI->fFees, pzPrevDI->fCumFees, pzPrevDI->bPeriodEnd,
                   pzPrevDI->lDaysSinceNond, pzNewDI->fTodayFees,
                   &pzNewDI->fFees, &pzNewDI->fCumFees, &pzNewDI->fWtdFees,
                   bSpecialCase);

      /* New net, cum and wtd Fees Out*/
      NewNetCumWtd(pzPrevDI->fFeesOut, pzPrevDI->fCumFeesOut,
                   pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                   pzNewDI->fTodayFeesOut, &pzNewDI->fFeesOut,
                   &pzNewDI->fCumFeesOut, &pzNewDI->fWtdFeesOut, bSpecialCase);

      /* New net, cum and wtd CN Fees */
      NewNetCumWtd(pzPrevDI->fCNFees, pzPrevDI->fCumCNFees,
                   pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                   pzNewDI->fTodayCNFees, &pzNewDI->fCNFees,
                   &pzNewDI->fCumCNFees, &pzNewDI->fWtdCNFees, bSpecialCase);

      if (!pzPrevDI->bPeriodEnd) {
        pzNewDI->fPurchases += pzPrevDI->fPurchases;
        pzNewDI->fSales += pzPrevDI->fSales;
        pzNewDI->fContributions += pzPrevDI->fContributions;
        pzNewDI->fWithdrawals += pzPrevDI->fWithdrawals;
        // pzNewDI->fFeesOut       += pzPrevDI->fFeesOut +
        // pzNewDI->fTodayFeesOut;
      }

      /* If doing any tax work, calculate necessary fields in TIfo  */
      if (TaxInfoRequired(iReturnsToCalculate)) {
        pzPrevTI = &(zPKTable.pzPKey[i].pzTInfo[j - 1]);
        pzNewTI = &(zPKTable.pzPKey[i].pzTInfo[j]);

        /* New net, cum and wtd federal income tax withheld */
        NewNetCumWtd(pzPrevTI->fFedinctaxWthld, pzPrevTI->fCumFedinctaxWthld,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedinctaxWthld, &pzNewTI->fFedinctaxWthld,
                     &pzNewTI->fCumFedinctaxWthld, &pzNewTI->fWtdFedinctaxWthld,
                     bSpecialCase);

        /* New net, cum and wtd state income tax withheld */
        /* state tax calcualtions have been disabled - 5/12/06 - vay
        NewNetCumWtd(pzPrevTI->fStinctaxWthld, pzPrevTI->fCumStinctaxWthld,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStinctaxWthld, &pzNewTI->fStinctaxWthld,
        &pzNewTI->fCumStinctaxWthld, &pzNewTI->fWtdStinctaxWthld, bSpecialCase);
        */

        /* New net, cum and wtd federal income tax reclaimed */
        NewNetCumWtd(pzPrevTI->fFedtaxRclm, pzPrevTI->fCumFedtaxRclm,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedtaxRclm, &pzNewTI->fFedtaxRclm,
                     &pzNewTI->fCumFedtaxRclm, &pzNewTI->fWtdFedtaxRclm,
                     bSpecialCase);

        /* New net, cum and wtd state income tax reclaimed */
        /* state tax calcualtions have been disabled - 5/12/06 - vay
        NewNetCumWtd(pzPrevTI->fSttaxRclm, pzPrevTI->fCumSttaxRclm,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond, pzNewTI->fTdySttaxRclm,
        &pzNewTI->fSttaxRclm, &pzNewTI->fCumSttaxRclm, &pzNewTI->fWtdSttaxRclm,
        bSpecialCase);
        */

        /* New net, cum and wtd federal tax equivalent income */
        NewNetCumWtd(pzPrevTI->fFedetaxInc, pzPrevTI->fCumFedetaxInc,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedetaxInc, &pzNewTI->fFedetaxInc,
                     &pzNewTI->fCumFedetaxInc, &pzNewTI->fWtdFedetaxInc,
                     bSpecialCase);

        /* New net, cum and wtd federal taxable income */
        NewNetCumWtd(pzPrevTI->fFedataxInc, pzPrevTI->fCumFedataxInc,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedataxInc, &pzNewTI->fFedataxInc,
                     &pzNewTI->fCumFedataxInc, &pzNewTI->fWtdFedataxInc,
                     bSpecialCase);

        /* New net, cum and wtd state tax equivalent income */
        /* state tax calcualtions have been disabled - 5/12/06 - vay
        NewNetCumWtd(pzPrevTI->fStetaxInc, pzPrevTI->fCumStetaxInc,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond, pzNewTI->fTdyStetaxInc,
        &pzNewTI->fStetaxInc, &pzNewTI->fCumStetaxInc, &pzNewTI->fWtdStetaxInc,
        bSpecialCase);
        */

        /* New net, cum and wtd state taxable income */
        /* state tax calcualtions have been disabled - 5/12/06 - vay
        NewNetCumWtd(pzPrevTI->fStataxInc, pzPrevTI->fCumStataxInc,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond, pzNewTI->fTdyStataxInc,
        &pzNewTI->fStataxInc, &pzNewTI->fCumStataxInc, &pzNewTI->fWtdStataxInc,
        bSpecialCase);
        */

        /* New net, cum and wtd federal tax equivalent short term Real G/L */
        NewNetCumWtd(pzPrevTI->fFedetaxStrgl, pzPrevTI->fCumFedetaxStrgl,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedetaxStrgl, &pzNewTI->fFedetaxStrgl,
                     &pzNewTI->fCumFedetaxStrgl, &pzNewTI->fWtdFedetaxStrgl,
                     bSpecialCase);

        /* New net, cum and wtd federal tax equivalent long(>12M)term Real G/L*/
        NewNetCumWtd(pzPrevTI->fFedetaxLtrgl, pzPrevTI->fCumFedetaxLtrgl,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedetaxLtrgl, &pzNewTI->fFedetaxLtrgl,
                     &pzNewTI->fCumFedetaxLtrgl, &pzNewTI->fWtdFedetaxLtrgl,
                     bSpecialCase);

        /*New net, cum and wtd federal tax equiv currency Real G/L */
        NewNetCumWtd(pzPrevTI->fFedetaxCrrgl, pzPrevTI->fCumFedetaxCrrgl,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedetaxCrrgl, &pzNewTI->fFedetaxCrrgl,
                     &pzNewTI->fCumFedetaxCrrgl, &pzNewTI->fWtdFedetaxCrrgl,
                     bSpecialCase);

        /* New net, cum and wtd federal taxable short term Real G/L */
        NewNetCumWtd(pzPrevTI->fFedataxStrgl, pzPrevTI->fCumFedataxStrgl,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedataxStrgl, &pzNewTI->fFedataxStrgl,
                     &pzNewTI->fCumFedataxStrgl, &pzNewTI->fWtdFedataxStrgl,
                     bSpecialCase);

        /* New net, cum and wtd federal taxable long(>18M)term Real G/L*/
        NewNetCumWtd(pzPrevTI->fFedataxLtrgl, pzPrevTI->fCumFedataxLtrgl,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedataxLtrgl, &pzNewTI->fFedataxLtrgl,
                     &pzNewTI->fCumFedataxLtrgl, &pzNewTI->fWtdFedataxLtrgl,
                     bSpecialCase);

        /*New net, cum and wtd federal taxable currency Real G/L */
        NewNetCumWtd(pzPrevTI->fFedataxCrrgl, pzPrevTI->fCumFedataxCrrgl,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedataxCrrgl, &pzNewTI->fFedataxCrrgl,
                     &pzNewTI->fCumFedataxCrrgl, &pzNewTI->fWtdFedataxCrrgl,
                     bSpecialCase);

        /* state tax calcualtions have been disabled - 5/12/06 - vay

        // New net, cum and wtd state tax equivalent short term Real G/L
        NewNetCumWtd(pzPrevTI->fStetaxStrgl, pzPrevTI->fCumStetaxStrgl,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStetaxStrgl, &pzNewTI->fStetaxStrgl,
        &pzNewTI->fCumStetaxStrgl, &pzNewTI->fWtdStetaxStrgl, bSpecialCase);

        // New net, cum and wtd state tax equivalent long(>18M)term Real G/L
        NewNetCumWtd(pzPrevTI->fStetaxLtrgl, pzPrevTI->fCumStetaxLtrgl,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStetaxLtrgl, &pzNewTI->fStetaxLtrgl,
        &pzNewTI->fCumStetaxLtrgl, &pzNewTI->fWtdStetaxLtrgl, bSpecialCase);

        //	New net, cum and wtd state tax equiv currency Real G/L
        NewNetCumWtd(pzPrevTI->fStetaxCrrgl, pzPrevTI->fCumStetaxCrrgl,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStetaxCrrgl, &pzNewTI->fStetaxCrrgl,
        &pzNewTI->fCumStetaxCrrgl, &pzNewTI->fWtdStetaxCrrgl, bSpecialCase);

        // New net, cum and wtd state taxable short term Real G/L
        NewNetCumWtd(pzPrevTI->fStataxStrgl, pzPrevTI->fCumStataxStrgl,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStataxStrgl, &pzNewTI->fStataxStrgl,
        &pzNewTI->fCumStataxStrgl, &pzNewTI->fWtdStataxStrgl, bSpecialCase);

        // New net, cum and wtd state taxable long(>18M)term Real G/L
        NewNetCumWtd(pzPrevTI->fStataxLtrgl, pzPrevTI->fCumStataxLtrgl,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStataxLtrgl, &pzNewTI->fStataxLtrgl,
        &pzNewTI->fCumStataxLtrgl, &pzNewTI->fWtdStataxLtrgl, bSpecialCase);

        // New net, cum and wtd state taxable currency Real G/L
        NewNetCumWtd(pzPrevTI->fStataxCrrgl, pzPrevTI->fCumStataxCrrgl,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStataxCrrgl, &pzNewTI->fStataxCrrgl,
        &pzNewTI->fCumStataxCrrgl, &pzNewTI->fWtdStataxCrrgl, bSpecialCase);

        state tax disabled code ends */

        /*New net, cum and wtd federal tax equiv Amortization/Accretion */
        NewNetCumWtd(pzPrevTI->fFedetaxAmort, pzPrevTI->fCumFedetaxAmort,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedetaxAmort, &pzNewTI->fFedetaxAmort,
                     &pzNewTI->fCumFedetaxAmort, &pzNewTI->fWtdFedetaxAmort,
                     bSpecialCase);

        /* New net, cum and wtd federal taxable Amortization/Accretion */
        NewNetCumWtd(pzPrevTI->fFedataxAmort, pzPrevTI->fCumFedataxAmort,
                     pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
                     pzNewTI->fTdyFedataxAmort, &pzNewTI->fFedataxAmort,
                     &pzNewTI->fCumFedataxAmort, &pzNewTI->fWtdFedataxAmort,
                     bSpecialCase);

        /* state tax calcualtions have been disabled - 5/12/06 - vay
        //	New net, cum and wtd state tax equiv Amortization/Accretion
        NewNetCumWtd(pzPrevTI->fStetaxAmort, pzPrevTI->fCumStetaxAmort,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStetaxAmort, &pzNewTI->fStetaxAmort,
        &pzNewTI->fCumStetaxAmort, &pzNewTI->fWtdStetaxAmort, bSpecialCase);

        // New net, cum and wtd state taxable Amortization/Accretion
        NewNetCumWtd(pzPrevTI->fStataxAmort, pzPrevTI->fCumStataxAmort,
        pzPrevDI->bPeriodEnd, pzPrevDI->lDaysSinceNond,
        pzNewTI->fTdyStataxAmort, &pzNewTI->fStataxAmort,
        &pzNewTI->fCumStataxAmort, &pzNewTI->fWtdStataxAmort, bSpecialCase);
        state tax disabled code ends */

        zPKTable.pzPKey[i].pzTInfo[j - 1] = *pzPrevTI;
        zPKTable.pzPKey[i].pzTInfo[j] = *pzNewTI;
      } /* If tax work is required */

      /* Calculate today's dayssince non daily */
      /* For daily performance, it is always 0 */
      if (zSysSet.bDailyPerf && strcmp(zPKTable.sPerfInterval, "D") == 0)
        pzNewDI->lDaysSinceNond = 0;
      else if (pzPrevDI->bPeriodEnd)
        pzNewDI->lDaysSinceNond = 1;
      else
        pzNewDI->lDaysSinceNond = pzPrevDI->lDaysSinceNond + 1;

      zPKTable.pzPKey[i].pzDInfo[j - 1] = *pzPrevDI;
      zPKTable.pzPKey[i].pzDInfo[j] = *pzNewDI;
    } /* for j < NumDInfo */
  } /* for i < NumPKey */

  return zErr;
} /* calculateweightedvalues */

/**
** This function is used to calculate new net, cumulative and weighted values
** (could be flow, income, fee, etc.) using previous day's net, cumulative,
** weighted values and today's value(todayflow, todayincome, etc.) It also
** calculate new dayssincenondaily number. If previous day was a period end,
** new net, cum and wtd are all equal to today's value and new days since nond
** is 1, else new days since nond is 1 more than previous and new values are
** calculated as follows:
**   new net = previous net + today value
**   new cum = prev cum + prev net + today value
**   new wtd = cum / days since last nond
**/
void NewNetCumWtd(double zPNet, double zPCum, BOOL bPPeriodEnd,
                  long lPDaysSinceNond, double zNToday, double *pzNNet,
                  double *pzNCum, double *pzNWtd, BOOL bAddToCurrentCum) {
  double fFlowCoeff;

  switch (zSysSet.zSyssetng.iFlowWeightMethod) {
  case 0:           // End of day
    fFlowCoeff = 0; // do not include today's flow in cumulative
    break;
  case 1: // beginning of day
    fFlowCoeff = 1;
    break;
  case 2: // mid-day
    fFlowCoeff = 0.5;
    break;
  case 3: // Value Dependent
    if (zNToday > 0)
      fFlowCoeff = 1;
    else
      fFlowCoeff = 0;
    break;
  default:
    fFlowCoeff = 0; // do not include today's flow in cumulative
    break;
  }

  /*
  ** If previous day was a period end, new net, cum and weigthed values will
  ** all be equal to today's value, else it is calculated using the formula
  ** given above.
  */
  if (bPPeriodEnd) {
    if (bAddToCurrentCum == FALSE) {
      *pzNNet = zNToday;
      *pzNCum = 0;
    } else {
      *pzNNet = zNToday;
      *pzNCum = zNToday * fFlowCoeff;

    } /* Don't discard values in Current Cum, Add today's value to it  */
  } /* If previous was a period end */
  else {
    /* Net = Previous Net + Today Value */
    *pzNNet = zPNet + zNToday;

    /*
    ** If Current Cum needs to be added then
    **   Current Cum = Current Cum + (Prev Cum + Prev Net + Today Value)
    ** else
    **   Current Cum = Prev Cum + Prev Net + Today Value
    */
    if (bAddToCurrentCum == FALSE)
      *pzNCum = zPCum + zPNet + zNToday * fFlowCoeff;
    else
      *pzNCum += zPCum + zPNet + zNToday * fFlowCoeff;

  } /* previous was not a period end */

  /* Wtd = Cum / Date Delta */
  *pzNWtd = *pzNCum / (lPDaysSinceNond + 1);
} /* NetCumWtdUsingPrevious */

/**
** By the time this function is called PerfkeyTable is filled with all the
** values required to calculate returns and also for each key, period end, if
** any, are already identified. This function calculates returns for all the
** subperiods and then links them to obtain return for the whole period. All the
** period end records are written in monthly database and the final overall
** record is written in Daily(if perf date happens to be a period end, obviously
** this record goes in monthly as well).
**/
ERRSTRUCT CalculateReturnForAllKeys(PKEYTABLE zPTable, long lLastPerfDate,
                                    long lCurrentPerfDate, PARTPMAIN zPmain,
                                    PERFCTRL zPCtrl, int bCalcSelected) {
  ERRSTRUCT zErr;
  RORSTRUCT zRorStruct, zMonthlyRor;
  SUMMDATA zSummdata, zMonthsum;
  PORTTAXTABLE zPTaxTable;
  MONTHTABLE zMTable;
  TAXPERF zTaxperf;
  DAILYFLOWS zDFlows;
  BOOL bCalcRequired, bStart, bFinish, bFound;
  int i, j, iDIndex, iSegIndex, x;
  double fEMV;
  short iCurrentMDY[3];
  // var to keep key's Last Non-Daily Date - required for InsertUpdateUV since
  // original date gets overwritten before this function can be called
  long lLndPerfDate = 0;
  long lPricingDate, lTradeDate;

  lpprInitializeErrStruct(&zErr);
  zPTaxTable.iCapacity = 0;
  InitializePorttaxTable(&zPTaxTable);
  zMTable.iCapacity = 0;
  InitializeMonthlyTable(&zMTable);

  lpprSelectStarsDate(&lTradeDate, &lPricingDate, &zErr);
  /*
  ** Get the ror indexes at the Last Perf Date and also identify if DAILY
  ** database has any scratch record (for all keys) which we can update with
  ** the new values. If for any key, we don't have a scratch record, we will
  ** have to insert a new record.
  */
  // lpfnTimer(13);
  zErr = GetScratchRecord(zPTable, zPmain.iID, lLastPerfDate, lCurrentPerfDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // lpfnTimer(14);
  /*
  ** Delete all the records(for the account) between lastperfdate(excluded) and
  ** current perf date(included) from the MONTHLY database and then all the
  ** fixed records are recalculated and inserted. Obviously, if we are doing
  ** regular processing, all the records for the account can be deleted but if
  ** we doing AsOf processing, only accounts in the perfkey table are deleted.
  */
  zErr = DeletePeriodPerformSet(zPTable, zPmain.iID, lLastPerfDate + 1,
                                lCurrentPerfDate, bCalcSelected,
                                zPmain.lInceptionDate);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  // lpfnTimer(15);

  // Delete any entry from bankstat table for this account for a date graeter
  // than last perf date
  lpprDeleteBankstat(zPmain.iID, lLastPerfDate + 1, -1, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  if (!CalcSelected) {
    // Delete any entry from dailyflows table for this account for a date
    // graeter than last perf date
    lpprDeleteDailyFlows(zPmain.iID, lLastPerfDate, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    /*
    ** If tax equivalent and/or taxable returns need to be calclulated, read
    ** appropriate tax rate for the portfolio.
    */
    if (TaxInfoRequired(zPmain.iReturnsToCalculate)) {
      zErr = FillPorttaxTable(zPmain.iID, lLastPerfDate, lCurrentPerfDate,
                              &zPTaxTable);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      // Delete any entry from taxperf table for this account for a date graeter
      // than last perf date
      lpprDeleteTaxperf(zPmain.iID, lLastPerfDate + 1, -1, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;
    } /* If tax related returns need to be calculated */
  } // if not calculating performance only for specific rules

  /*
  ** For each key, fixed records(10% flow and month end) have already been
  ** identified and market values, net flow, weighted flow, etc. for these dates
  ** have already been calculated. Calculate return at every fixed date and the
  ** current date. Insert perform set at every fixed point in the monthly
  ** database and that on current perform date in the daily database.
  */
  for (i = 0; i < zPTable.iCount; i++) {
    /* If key is deleted, no need to process it */
    if (IsKeyDeleted(zPTable.pzPKey[i].zPK.lDeleteDate) == TRUE)
      continue;

    iDIndex = -1; // first time begining value are not coming from DInfo array

    // lookup segment index - it is needed to keep track of cumulative monthly
    // data
    iSegIndex = FindMonthlyDataByID(zMTable, zPTable.pzPKey[i].zPK.iID);

    bStart = TRUE;
    bFinish = FALSE;
    if (CalcSelected) {
      // Delete any entry from dailyflows table for current segment (not entire
      // account) for a date graeter than last perf date
      /*bFound = FALSE;
      for (x = i; x >= 0; x--)
      {
              if (zPTable.pzPKey[i].zPK.iID == zPTable.pzPKey[x].zPK.iID &&
      IsKeyDeleted(zPTable.pzPKey[x].zPK.lDeleteDate) == TRUE
                      && zPTable.pzPKey[x].zPK.lDeleteDate > lLastPerfDate &&
      zPTable.pzPKey[x].bDeletedFromDB) {
                      lpprDeleteDailyFlowsByID(zPTable.pzPKey[i].zPK.iID,
      zPTable.pzPKey[x].zPK.lDeleteDate, &zErr); bFound = TRUE; break;
              }
      }*/
      // if (!bFound)
      lpprDeleteDailyFlowsByID(zPTable.pzPKey[i].zPK.iID, lLastPerfDate, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      if (TaxInfoRequired(zPmain.iReturnsToCalculate)) {
        zErr = FillPorttaxTable(zPTable.pzPKey[i].zPK.iID, lLastPerfDate,
                                lCurrentPerfDate, &zPTaxTable);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

        // Delete any entry from taxperf table for current Segment (not entire
        // account) for a date graeter than last perf date
        lpprDeleteTaxperfForSegment(zPTable.pzPKey[i].zPK.iID,
                                    lLastPerfDate + 1, -1, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
      } /* If tax related returns need to be calculated */
    } // if calculating performance for only selected rules

    for (j = 1; j < zPTable.pzPKey[i].iDInfoCount; j++) {
      /* Sometimes a key get deleted inside this loop as well, so check again if
       * key gets deleted, no need to process it */
      if (IsKeyDeleted(zPTable.pzPKey[i].zPK.lDeleteDate) == TRUE)
        continue;

      // if today's flow, fees or feesout is non-zero, insert the record n
      // dailyflows table
      if (!IsValueZero(zPTable.pzPKey[i].pzDInfo[j].fTodayFlow, 2) ||
          !IsValueZero(zPTable.pzPKey[i].pzDInfo[j].fTodayFees, 2) ||
          !IsValueZero(zPTable.pzPKey[i].pzDInfo[j].fTodayCNFees, 2) ||
          !IsValueZero(zPTable.pzPKey[i].pzDInfo[j].fTodayFeesOut, 2)) {
        zDFlows.iSegmainID = zPTable.pzPKey[i].zPK.iID;
        zDFlows.lPerformDate = zPTable.pzPKey[i].pzDInfo[j].lDate;
        zDFlows.fNetFlow =
            RoundDouble(zPTable.pzPKey[i].pzDInfo[j].fTodayFlow +
                            zPTable.pzPKey[i].pzDInfo[j].fTodayFees,
                        2);
        zDFlows.fFees = zPTable.pzPKey[i].pzDInfo[j].fTodayFees -
                        zPTable.pzPKey[i].pzDInfo[j].fTodayFeesOut;
        zDFlows.fCNFees = zPTable.pzPKey[i].pzDInfo[j].fTodayCNFees;
        zDFlows.fFeesOut = zPTable.pzPKey[i].pzDInfo[j].fTodayFeesOut;
        zDFlows.fCreateDate = lpfnCurrentDateAndTime();

        lpprInsertDailyFlows(zDFlows, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
          if (zErr.iSqlError != DB_E_PKVIOLATION) {
            InitializePorttaxTable(&zPTaxTable);
            InitializeMonthlyTable(&zMTable);
            return zErr;
          }
        }
      } // if today's flow, fees or feesout is non-zero

      if (zPTable.pzPKey[i].pzDInfo[j].bPeriodEnd == TRUE ||
          zPTable.pzPKey[i].pzDInfo[j].lDate == lCurrentPerfDate) {
        // convert the current date into month/date/year
        lpfnrjulmdy(zPTable.pzPKey[i].pzDInfo[j].lDate, iCurrentMDY);

        /*
        ** Beginning values on the first date we are calculating RORs, are the
        ** beginning values for the period and the begining values at the
        ** subsequent days on which RORs are calculated, are the values on the
        ** previous fixed point
        */
        if (iDIndex == -1) {
          InitializeRorStruct(&zRorStruct, zPTable.pzPKey[i].pzDInfo[j].lDate);
          zRorStruct.fBeginMV = zPTable.pzPKey[i].fBeginMV;
          zRorStruct.fBeginAI = zPTable.pzPKey[i].fBeginAI;
          zRorStruct.fBeginAD = zPTable.pzPKey[i].fBeginAD;
          zRorStruct.fBeginInc = zPTable.pzPKey[i].fBeginInc;
        } else {
          zRorStruct.fBeginMV = zPTable.pzPKey[i].pzDInfo[iDIndex].fMktVal;
          zRorStruct.fBeginAI = zPTable.pzPKey[i].pzDInfo[iDIndex].fAccrInc;
          zRorStruct.fBeginAD = zPTable.pzPKey[i].pzDInfo[iDIndex].fAccrDiv;
          zRorStruct.fBeginInc = zPTable.pzPKey[i].pzDInfo[iDIndex].fIncome;
        }

        /* get the rest of the values required to calculate different RORs */
        zRorStruct.fEndMV = zPTable.pzPKey[i].pzDInfo[j].fMktVal;
        zRorStruct.fEndAI = zPTable.pzPKey[i].pzDInfo[j].fAccrInc;
        zRorStruct.fEndAD = zPTable.pzPKey[i].pzDInfo[j].fAccrDiv;
        zRorStruct.fNetFlow = zPTable.pzPKey[i].pzDInfo[j].fNetFlow +
                              zPTable.pzPKey[i].pzDInfo[j].fNotionalFlow;
        zRorStruct.fWtFlow = zPTable.pzPKey[i].pzDInfo[j].fWtdFlow;
        zRorStruct.fFees = zPTable.pzPKey[i].pzDInfo[j].fFees;
        zRorStruct.fWtFees = zPTable.pzPKey[i].pzDInfo[j].fWtdFees;
        zRorStruct.fCNFees = zPTable.pzPKey[i].pzDInfo[j].fCNFees;
        zRorStruct.fWtCNFees = zPTable.pzPKey[i].pzDInfo[j].fWtdCNFees;
        zRorStruct.fIncome = zPTable.pzPKey[i].pzDInfo[j].fIncome;
        zRorStruct.fWtIncome = zPTable.pzPKey[i].pzDInfo[j].fWtdInc;
        zRorStruct.fFeesOut = zPTable.pzPKey[i].pzDInfo[j].fFeesOut;
        zRorStruct.fWtFeesOut = zPTable.pzPKey[i].pzDInfo[j].fWtdFeesOut;

        if (strcmp(zPTable.pzPKey[i].zPK.sTotalRecInd, "T") == 0)
          zRorStruct.bTotalPortfolio = TRUE;
        else
          zRorStruct.bTotalPortfolio = FALSE;

        if (zPTable.pzPKey[i].bNewKey == TRUE &&
            zPTable.pzPKey[i].zPK.lInitPerfDate ==
                zPTable.pzPKey[i].pzDInfo[j].lDate)
          zRorStruct.bInceptionRor = TRUE;
        else
          zRorStruct.bInceptionRor = FALSE;

        /*
        ** Calculate the ending MV and Begin MV. If EndMV is zero and BeginMV
        ** is not zero then this is the Termination Record.
        */
        fEMV = zRorStruct.fEndMV + zRorStruct.fEndAI + zRorStruct.fEndAD;
        if (IsValueZero(fEMV, 3)) {
          zPTable.pzPKey[i].zPK.lDeleteDate =
              zPTable.pzPKey[i].pzDInfo[j].lDate;
          zRorStruct.bTerminationRor = TRUE;
        } else
          zRorStruct.bTerminationRor = FALSE;

        /*
        ** If the key is started today, deleted today and it has no flow,
        ** there is nothing to calculate and nothing to write.
        */
        if (zRorStruct.bInceptionRor == TRUE &&
            zRorStruct.bTerminationRor == TRUE &&
            zPTable.pzPKey[i].pzDInfo[j].lDate ==
                zPTable.pzPKey[i].zPK.lInitPerfDate &&
            IsValueZero(zRorStruct.fNetFlow, 3) &&
            IsValueZero(zRorStruct.fFees, 3))
          continue;

        /*
        ** If the key is incepted on the same day when the account is incepted,
        ** its first day's performance is not calculated, index starts at 100.
        ** If it is a new key and is getting terminated today and its flow equal
        ** to -income, index starts at 100.
        */
        if (zPTable.pzPKey[i].bNewKey == TRUE &&
            zRorStruct.bInceptionRor == TRUE)
          bCalcRequired = TRUE; /* index on inception day starts at 100*/
        else if (zRorStruct.bInceptionRor == TRUE &&
                 zRorStruct.bTerminationRor == TRUE) {
          if (zRorStruct.fIncome * -1.0 == zRorStruct.fNetFlow)
            bCalcRequired = FALSE;
          else
            bCalcRequired = TRUE;
        } else
          bCalcRequired = TRUE;

        zRorStruct.iReturnstoCalculate = zPmain.iReturnsToCalculate;
        if (TaxInfoRequired(zPmain.iReturnsToCalculate)) {
          // strcpy_s(zRorStruct.sTaxCalc, zPTable.sTaxCalc);
          zRorStruct.zPTax = FindCorrectPorttax(
              zPTaxTable, zPTable.pzPKey[i].pzDInfo[j].lDate);
          CreateTaxinfoForAKey(zPTable, i, j, &zRorStruct.zTInfo);
        } // If tax calculations are required

        if (bCalcRequired)
          CalculateGFAndNFRor(&zRorStruct);

        /* If taxable, tax equivalent or both ror needs to be calculated */
        if (TaxInfoRequired(zPmain.iReturnsToCalculate)) {
          // Create a taxperf record and insert it if it is not a daily record
          if (zPTable.pzPKey[i].pzDInfo[j].bPeriodEnd ||
              strcmp(zSummdata.sPerformType, "I") == 0 ||
              strcmp(zSummdata.sPerformType, "T") == 0) {
            CreateTaxperfFromDailyTaxInfo(
                zPTable.pzPKey[i].pzTInfo[j],
                zPTable.pzPKey[i].zPK.iPortfolioID, zPTable.pzPKey[i].zPK.iID,
                zPTable.pzPKey[i].pzDInfo[j].lDate, &zTaxperf);

            // 1-day return may cause PK violation even if we deleted data for
            // entire period if this is the case, ignore such PK
            lpprInsertTaxperf(zTaxperf, &zErr);
            if (zErr.iSqlError == DB_E_PKVIOLATION)
              zErr.iSqlError = 0;
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
              InitializePorttaxTable(&zPTaxTable);
              InitializeMonthlyTable(&zMTable);
              return zErr;
            }
          } // if a period end
        } /* If tax calculations are required */

        /* Calculate the ending Principal, base & Income index values */
        zPTable.pzPKey[i].zNewIndex = zRorStruct.zAllRor;

        // SB 12/11/2012 - VI 50551 - since for terminating segments weighted
        // flow is not used in calculation, it should not be
        //														saved
        //in DB either. If not zero for terminating segments, it causes problem
        //in composite merge if (zRorStruct.bTerminationRor)
        //	zPTable.pzPKey[i].pzDInfo[j].fWtdFlow = 0;

        // create a summdata record for the period end or current date
        CreatePerformFromDInfo(zPTable, i, j, &zSummdata);

        /*
        ** if this is the first period end for this key, or if this period end
        * is for a different
        ** month than the last period end, monthsum is same as summdata. If on
        * the other hand,
        ** this period end is for the same month as the last one (typically will
        * hapen in case
        ** of inception, termination and 10% flow) then add the values together
        */
        if (iSegIndex != -1) {
          // if data found...
          short iSegmentMDY[3];

          lpfnrjulmdy(zMTable.pzMonthlyData[iSegIndex].zMonthsum.lPerformDate,
                      iSegmentMDY);
          if (iCurrentMDY[0] != iSegmentMDY[0]) {
            // but for previous month - reset it...
            InitializePerform(&zMTable.pzMonthlyData[iSegIndex].zMonthsum, 0);
            InitializeRorStruct(&zMTable.pzMonthlyData[iSegIndex].zMonthlyRor,
                                0);
            zMTable.pzMonthlyData[iSegIndex].iNumSubPeriods = 0;

            // and now set this month data
            InitializeAllRors(&zMonthlyRor.zAllRor, TRUE);
            zMonthlyRor = zRorStruct;
            if (zMonthlyRor.bInceptionRor)
              zMonthlyRor.fBeginMV = zMonthlyRor.fBeginAI =
                  zMonthlyRor.fBeginAD = 0;

            zMTable.pzMonthlyData[iSegIndex].zMonthlyRor = zMonthlyRor;
            CreateMonthsumFromTwoSummdata(
                zSummdata, zMTable.pzMonthlyData[iSegIndex].zMonthsum,
                &zMTable.pzMonthlyData[iSegIndex].zMonthsum);
          } else {
            // if for current month - add/link accordingly
            zMTable.pzMonthlyData[iSegIndex].iNumSubPeriods++;
            CreateMonthsumFromTwoSummdata(
                zSummdata, zMTable.pzMonthlyData[iSegIndex].zMonthsum,
                &zMTable.pzMonthlyData[iSegIndex].zMonthsum);
            LinkReturnsForTheMonth(
                zMTable.pzMonthlyData[iSegIndex].zMonthlyRor.zAllRor,
                zRorStruct.zAllRor,
                &zMTable.pzMonthlyData[iSegIndex].zMonthlyRor.zAllRor);
          }
        } // segindex != -1
        else {
          // segment found for the 1st time - just add data to the table
          InitializeAllRors(&zMonthlyRor.zAllRor, TRUE);
          zMonthlyRor = zRorStruct;
          if (zMonthlyRor.bInceptionRor)
            zMonthlyRor.fBeginMV = zMonthlyRor.fBeginAI = zMonthlyRor.fBeginAD =
                0;

          AddMonthlyDataToTable(&zMTable, zSummdata, zMonthlyRor);
          iSegIndex = FindMonthlyDataByID(zMTable, zPTable.pzPKey[i].zPK.iID);
          if (iSegIndex == -1) {
            InitializePorttaxTable(&zPTaxTable);
            InitializeMonthlyTable(&zMTable);

            zErr = lpfnPrintError("Unable to add segment to MTable", 0, 0, "",
                                  997, 0, 0, "CALCPERF ADDMONTHLY1", FALSE);
          }
        } // segindex == -1

        /*
        ** If this is the first record for a new key, identify it as an
        ** initialization record and write it into period table.
        */
        if (zRorStruct.bInceptionRor == TRUE)
          strcpy_s(zSummdata.sPerformType, "I"); /* Initiallization record */

        /*
        ** If the ending market value for the key has gone down to zero, and
        ** there is no flow to account for it, mark this as being deleted and
        ** write it into period as well as monthly tables.
        */
        if (IsValueZero(fEMV, 3)) {
          zPTable.pzPKey[i].zPK.lDeleteDate = zSummdata.lPerformDate;
          strcpy_s(zSummdata.sPerformType, "T"); /* Termination record */
        }

        // if we're	about to save month end (or termination) data
        if (lpfnIsItAMonthEnd(zSummdata.lPerformDate) ||
            strcmp(zSummdata.sPerformType, "T") == 0) {
          // check to see if FWF needs to be calculated for the month
          if (zMTable.pzMonthlyData[iSegIndex].iNumSubPeriods > 0) {
            // replace Monthsum with cumulative values
            // and calculate FWF here
            zMonthsum = zMTable.pzMonthlyData[iSegIndex].zMonthsum;
            zMonthlyRor = zMTable.pzMonthlyData[iSegIndex].zMonthlyRor;
            strcpy_s(zMonthsum.sPerformType, zSummdata.sPerformType);
            strcpy_s(zMTable.pzMonthlyData[iSegIndex].zMonthsum.sPerformType,
                     zSummdata.sPerformType);

            zMonthlyRor.fEndMV = zMonthsum.fMktVal;
            zMonthlyRor.fEndAI = zMonthsum.fAccrInc;
            zMonthlyRor.fEndAD = zMonthsum.fAccrDiv;
            zMonthlyRor.fEndInc = zMonthsum.fIncome;
            zMonthlyRor.fNetFlow = zMonthsum.fNetFlow + zMonthsum.fNotionalFlow;
            zMonthlyRor.fFees = zMonthsum.fFees;
            zMonthlyRor.fWtFees = zMonthsum.fWtdFees;
            zMonthlyRor.fCNFees = zMonthsum.fCNFees;
            zMonthlyRor.fWtCNFees = zMonthsum.fWtdCNFees;
            zMonthlyRor.fIncome = zMonthsum.fIncome;
            zMonthlyRor.fFeesOut = zMonthsum.fFeesOut;
            zMonthlyRor.fWtFeesOut = zMonthsum.fWtdFeesOut;

            CalculateWeightedFlow(&zMonthlyRor);
            zMonthsum.fWtdFlow = RoundDouble(zMonthlyRor.fGFTWWtdFlow, 3);
          } else
            zMonthsum = zSummdata; // if no interim periods, Monthsum is same as
                                   // Summdata
        } // if EOM

        // lpfnTimer(6);

        /*
        ** If it is a period end, an initialization record or a termination
        ** record, write it in the monthly database.
        **
        ** If the record has same init date and delete date, no need to write
        * it.
        ** 11/14/05 The above stated is not the case anymore, i.e. we need to
        * save such record
        ** for further usage in composite merge
        **
        */
        if ((zPTable.pzPKey[i].pzDInfo[j].bPeriodEnd ||
             strcmp(zSummdata.sPerformType, "I") == 0 ||
             strcmp(zSummdata.sPerformType, "T") == 0))
        // 11/14/05 && (zPTable.pzPKey[i].zPK.lInitPerfDate !=
        // zPTable.pzPKey[i].zPK.lDeleteDate))
        {
          // lpfnTimer(16);
          // lpfnTimer(7);

          zSummdata.fCreateDate = lpfnCurrentDateAndTime();
          zSummdata.fChangeDate = zSummdata.fCreateDate;
          // 1-day return may cause PK violation, in such case
          // try to delete and re-insert
          lpprInsertPeriodSummdata(zSummdata, &zErr);
          if (zErr.iSqlError == DB_E_PKVIOLATION) {
            lpprDeleteSummdata(zSummdata.iID, zSummdata.lPerformDate,
                               zSummdata.lPerformDate, &zErr);
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
              InitializePorttaxTable(&zPTaxTable);
              InitializeMonthlyTable(&zMTable);
              return zErr;
            }
            lpprInsertPeriodSummdata(zSummdata, &zErr);
          }
          // lpfnTimer(17);
          // lpfnTimer(8);

          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
            InitializePorttaxTable(&zPTaxTable);
            InitializeMonthlyTable(&zMTable);
            return zErr;
          }

          /*
          ** SB 8/3/09 - Now termination record is also saved in monthsum. This
          * is necessary for many reports
          ** (e.g. composite spreadsheet) that work off of monthsum which till
          * now were missing data (e.g. flow)
          ** from non-month end terminating record.
          */
          if (lpfnIsItAMonthEnd(zSummdata.lPerformDate) ||
              strcmp(zSummdata.sPerformType, "T") == 0) {
            zMonthsum.fCreateDate = lpfnCurrentDateAndTime();
            zMonthsum.fChangeDate = zMonthsum.fCreateDate;
            zMonthsum.lPerformDate = zSummdata.lPerformDate;
            // lpfnTimer(9);
            // lpfnTimer(18);
            lpprInsertMonthlySummdata(zMonthsum, &zErr);
            // lpfnTimer(19);
            // lpfnTimer(10);
            //  1-day return may cause PK violation, in such case
            //  try to delete and re-insert
            if (zErr.iSqlError == DB_E_PKVIOLATION) {
              lpprDeleteMonthSum(zMonthsum.iID, zMonthsum.lPerformDate,
                                 zMonthsum.lPerformDate, &zErr);
              if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
                InitializePorttaxTable(&zPTaxTable);
                InitializeMonthlyTable(&zMTable);
                return zErr;
              }
              lpprInsertMonthlySummdata(zMonthsum, &zErr);
            }

            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
              InitializePorttaxTable(&zPTaxTable);
              InitializeMonthlyTable(&zMTable);
              return zErr;
            }
          }

          /* Update last non daily date */
          lLndPerfDate = zPTable.pzPKey[i].zPK.lLndPerfDate;
          zPTable.pzPKey[i].zPK.lLndPerfDate = zSummdata.lPerformDate;

          // If it is a month end date, update month end date
          if (lpfnIsItAMonthEnd(zPTable.pzPKey[i].pzDInfo[j].lDate)) {
            zPTable.pzPKey[i].zPK.lMePerfDate = zSummdata.lPerformDate;
            zPTable.pzPKey[i].zPK.lLndPerfDate = zSummdata.lPerformDate;
          }

          // If it is an initialization record, update init and last nondaily
          // dates
          if (strcmp(zSummdata.sPerformType, "I") == 0) {
            zPTable.pzPKey[i].zPK.lInitPerfDate = zSummdata.lPerformDate;
            zPTable.pzPKey[i].zPK.lLndPerfDate = zSummdata.lPerformDate;
          }

          zErr =
              InsertOrUpdateUV(&zPTable, i, "M", zRorStruct, lLastPerfDate,
                               lLndPerfDate, lCurrentPerfDate,
                               zPTable.pzPKey[i].pzDInfo[j].bPeriodEnd, zPmain);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
            InitializePorttaxTable(&zPTaxTable);
            InitializeMonthlyTable(&zMTable);
            return zErr;
          }

          iDIndex = j;
        } /* if period end */

        /* If it is the current perf date, write records in Daily database
        ** If the record has same init date and delete date, no need to write
        * it.
        ** 11/14/05 The above stated is not the case anymore, i.e. we need to
        * save such record
        ** for further usage in composite merge
        */
        if (zPTable.pzPKey[i].pzDInfo[j].lDate == lCurrentPerfDate)
        // 11/14/05 zPTable.pzPKey[i].zPK.lInitPerfDate !=
        // zPTable.pzPKey[i].zPK.lDeleteDate)
        {
          // update last perf date
          zPTable.pzPKey[i].zPK.lLastPerfDate = zSummdata.lPerformDate;

          zSummdata.fChangeDate = lpfnCurrentDateAndTime();

          if (zPTable.pzPKey[i].lScratchDate != zSummdata.lPerformDate)
            zSummdata.fCreateDate = zSummdata.fChangeDate;

          // new, 9/4/2017: only enter into dsumdata if pricing or daily
          if (lPricingDate == lCurrentPerfDate ||
              !lpfnIsItAMonthEnd(lCurrentPerfDate)) {

            if (zPTable.pzPKey[i].lScratchDate != 0)
              lpprUpdateDailySummdata(zSummdata, zPTable.pzPKey[i].lScratchDate,
                                      &zErr);
            else {
              lpprInsertDailySummdata(zSummdata, &zErr);
              if (zErr.iSqlError != 0 ||
                  zErr.iBusinessError !=
                      0) { // if PK violation due to possible 1-day return -
                           // just force deletion and re-insert
                // otherwise, ignore this error
                if (strcmp(zSummdata.sPerformType, "D") == 0) {
                  lpprDeleteDSumdata(zSummdata.iID, zSummdata.lPerformDate,
                                     zSummdata.lPerformDate, &zErr);
                  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
                    lpprInsertDailySummdata(zSummdata, &zErr);
                } else
                  lpprInitializeErrStruct(&zErr);
              } // error inserting
            } // if no scratch date
          }
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
            InitializePorttaxTable(&zPTaxTable);
            InitializeMonthlyTable(&zMTable);
            return zErr;
          }

          zErr =
              InsertOrUpdateUV(&zPTable, i, "D", zRorStruct, lLastPerfDate,
                               lLndPerfDate, lCurrentPerfDate,
                               zPTable.pzPKey[i].pzDInfo[j].bPeriodEnd, zPmain);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
            InitializePorttaxTable(&zPTaxTable);
            InitializeMonthlyTable(&zMTable);
            return zErr;
          }
        } /* if it is the current perf date */

        /*
        ** For the next subperiod, if any, new calculated ror index becomes the
        ** begining index.
        */
        zPTable.pzPKey[i].zBeginIndex = zPTable.pzPKey[i].zNewIndex;

        /*InitializeDWRorStruct(&zDWRor);
        zDWRor.iNumFlows = 1; *//* For begining values */
      } /* if periodend or current perf date */
    } /* for j < iDInfoCount */
  } /* for i < NumKeys */

  // Free up the memory
  InitializePorttaxTable(&zPTaxTable);
  InitializeMonthlyTable(&zMTable);

  return zErr;
} /* calculatereturnforallkeys */

/**
** This function is used to update few fields (rortype, performno) in the perf
** key table. This function is called after the perform record is inserted and
** before ror records are inserted.
**/
ERRSTRUCT ModifyRorTables(ALLRORS zRorIndex, int iRorType, long lPerformNo) {
  ERRSTRUCT zErr;
  //  int       iRTypeIdx;

  lpprInitializeErrStruct(&zErr);
  // SB - 2/23/99
  // Modify Dollar weighted
  //  zRorIndex.zIndex.iRorType = iRorType;
  // zRorIndex.zIndex.lPerformNo = lPerformNo;

  /*zErr = GetRorTypeIndex(sRorType, &iRTypeIdx);
  if (zErr.iBusinessError != 0)
  return zErr;

  // Modify Dollar weighted
  strcpy_s(zRorIndex.zDWIdx[iRTypeIdx].sRorType, sRorType);
  zRorIndex.zDWIdx[iRTypeIdx].lPerformNo = lPerformNo;

  // Modify Gross Of Fees
  strcpy_s(zRorIndex.zGFIdx[iRTypeIdx].sRorType, sRorType);
  zRorIndex.zGFIdx[iRTypeIdx].lPerformNo = lPerformNo;

  // Modify Net Of Fees
  strcpy_s(zRorIndex.zIndex[iRTypeIdx].sRorType, sRorType);
  zRorIndex.zIndex[iRTypeIdx].lPerformNo = lPerformNo;*/

  return zErr;
} /* ModifyRorTables */

/**
** Function that calculates tax related returns for a key. This function is
called with
** a RorStructure that has market value, net flow, weighted flow, etc., which
are the values
** for the key without any tax calculations. This function adds some tax
adjustments to these
** values and then calculates the required returns.
** /
ERRSTRUCT TaxRelatedReturns(PKEYTABLE zPTable, int iKey, int iDate, PORTTAX
zPorttax, RORSTRUCT *pzRor)
{
ERRSTRUCT			zErr;
RORSTRUCT			zTaxRor;
double				fTaxRate, fFIncAdj, fWtFIncAdj, fFWthldAdj,
fWtFWthldAdj, fFRclmAdj, fWtFRclmAdj; double
fFEndAIAdj, fFEndADAdj, fFBeginAIAdj, fFBeginADAdj, fFAIRclmAdj, fFADRclmAdj;
double				fBegFedataxAI, fBegFedataxAD, fBegFedetaxAI,
fBegFedetaxAD; int						i, iBeginDate;
DAILYTAXINFO	*pzTI;

lpprInitializeErrStruct(&zErr);

// If not a valid tax rate, default it to 39.6 %
if (zPorttax.fFedIncomeRate <= 0.0)
zPorttax.fFedIncomeRate = 39.6;

// if short-term or long-term gain/loss rate(s) is(are) zero, default it to
income rate if (zPorttax.fFedStGLRate <= 0) zPorttax.fFedStGLRate =
zPorttax.fFedIncomeRate;

if (zPorttax.fFedLtGLRate <= 0)
zPorttax.fFedLtGLRate = zPorttax.fFedIncomeRate;

// If any of the tax rate >= 100 % or <= 0, return an error
if (zPorttax.fFedIncomeRate <= 0.0 || zPorttax.fFedIncomeRate >= 100.0 ||
zPorttax.fFedStGLRate   <= 0.0 || zPorttax.fFedStGLRate   >= 100.0 ||
zPorttax.fFedLtGLRate   <= 0.0 || zPorttax.fFedLtGLRate   >= 100.0)
return(lpfnPrintError("Invalid Tax Rate", zPorttax.iID, 0, "", 517, 0, 0,
"CALCPERF TAXRETURN1", FALSE));

pzTI = &zPTable.pzPKey[iKey].pzTInfo[iDate];

// Find previous period end (to get begining accrual)
iBeginDate = -1;
i = iDate - 1;
while (iBeginDate == -1 && i >= 0)
{
if (zPTable.pzPKey[iKey].pzDInfo[i].bPeriodEnd)
iBeginDate = i;

i--;
}
if (iBeginDate < 0)
iBeginDate = 0;

if (iBeginDate == 0)
{
fBegFedataxAI = zPTable.pzPKey[iKey].fBegFedataxAI;
fBegFedataxAD = zPTable.pzPKey[iKey].fBegFedataxAD;
fBegFedetaxAI = zPTable.pzPKey[iKey].fBegFedetaxAI;
fBegFedetaxAD = zPTable.pzPKey[iKey].fBegFedetaxAD;
}
else
{
fBegFedataxAI = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedataxAccrInc;
fBegFedataxAD = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedataxAccrDiv;
fBegFedetaxAI = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedetaxAccrInc;
fBegFedetaxAD = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedetaxAccrDiv;
}

// Taxable returns
if (AfterTaxRequired(zPTable.sTaxCalc))
{
// get the effective tax rate for federal tax calculations
fTaxRate = EffectiveTaxRate(zPorttax.fFedIncomeRate);

// Federal withholding income adjustment
fFWthldAdj		= AfterTaxAdjustment(pzTI->fFedinctaxWthld, fTaxRate);
fWtFWthldAdj	= AfterTaxAdjustment(pzTI->fWtdFedinctaxWthld, fTaxRate);

// Federal reclaim adjustment
fFRclmAdj		= AfterTaxAdjustment(pzTI->fFedtaxRclm, fTaxRate);
fWtFRclmAdj = AfterTaxAdjustment(pzTI->fWtdFedtaxRclm, fTaxRate);

// Federal taxable income adjustment
fFIncAdj		= AfterTaxAdjustment(pzTI->fFedataxInc, fTaxRate);
fWtFIncAdj	= AfterTaxAdjustment(pzTI->fWtdFedataxInc, fTaxRate);

// Federal Ending Accr Inc adjustment
fFEndAIAdj = AfterTaxAdjustment(pzTI->fFedataxAccrInc, fTaxRate);

// Federal Ending Accr Div adjustment
fFEndADAdj = AfterTaxAdjustment(pzTI->fFedataxAccrDiv, fTaxRate);

// Federal Begining Accr Inc adjustment
fFBeginAIAdj = AfterTaxAdjustment(fBegFedataxAI, fTaxRate);

// Federal Begining Accr Div adjustment
fFBeginADAdj = AfterTaxAdjustment(fBegFedataxAD, fTaxRate);

// Federal Accr Inc Reclaim adjustment
fFAIRclmAdj = AfterTaxAdjustment(pzTI->fFedataxIncRclm, fTaxRate);

// Federal Accr Div Reclaim adjustment
fFADRclmAdj = AfterTaxAdjustment(pzTI->fFedataxDivRclm, fTaxRate);

/ *
** Add the federal taxable adjustments(to the values without any tax
** effects) and calculate Federal Taxable returns
* /
zTaxRor = *pzRor;

// Net flow = net flow + inc adj + withld adj + recl adj
zTaxRor.fNetFlow += (fFIncAdj + fFWthldAdj + fFRclmAdj);

// Wt flow = Wt flow + Wtinc adj + Wtwithld adj + Wtrecl adj
zTaxRor.fWtFlow += (fWtFIncAdj + fWtFWthldAdj + fWtFRclmAdj);

// AI = AI + AI adj + AI Rclm Adj
zTaxRor.fEndAI += (fFEndAIAdj + fFAIRclmAdj);
zTaxRor.fBeginAI += fFBeginAIAdj;

// AD = AD + AD adj + AD Rclm Adj
zTaxRor.fEndAD += (fFEndADAdj + fFADRclmAdj);
zTaxRor.fBeginAD += fFBeginADAdj;

// Call function to calculate Federal Taxable rors
zErr = CalculateGFAndNFRor(&zTaxRor, TRUE);
if (zErr.iBusinessError != 0)
return zErr;

// CalculateGFAndNFRor calculates the numbers for GTW and NTW returns, take the
values
// from there and update GTWAftertax and NTWAftertax returns.
pzRor->zAllRor.zIndex[GTWAfterTaxRor - 1].fBaseRorIdx =
zTaxRor.zAllRor.zIndex[GTWRor - 1].fBaseRorIdx;
pzRor->zAllRor.zIndex[NTWAfterTaxRor - 1].fBaseRorIdx =
zTaxRor.zAllRor.zIndex[NTWRor - 1].fBaseRorIdx; } // Taxable

// Tax equivalent returns
if (TaxEquivRequired(zPTable.sTaxCalc))
{
/// Federal effective tax income rate
fTaxRate = EffectiveTaxRate(zPorttax.fFedIncomeRate);

// Federal income adjustments
fFIncAdj  = TaxEquivalentAdjustment(pzTI->fFedetaxInc, fTaxRate);
fWtFIncAdj= TaxEquivalentAdjustment(pzTI->fWtdFedetaxInc, fTaxRate);

// Federal Ending Accr Inc adjustment
fFEndAIAdj = TaxEquivalentAdjustment(pzTI->fFedetaxAccrInc, fTaxRate);

// Federal Ending Accr Div adjustment
fFEndADAdj = TaxEquivalentAdjustment(pzTI->fFedetaxAccrDiv, fTaxRate);

// Federal Begining Accr Inc adjustment
fFBeginAIAdj = TaxEquivalentAdjustment(fBegFedetaxAI, fTaxRate);

// Federal Begining Accr Div adjustment
fFBeginADAdj = TaxEquivalentAdjustment(fBegFedetaxAD, fTaxRate);

// Federal Accr Inc Reclaim adjustment
fFAIRclmAdj = TaxEquivalentAdjustment(pzTI->fFedetaxIncRclm, fTaxRate);

// Federal Accr Div Reclaim adjustment
fFADRclmAdj = TaxEquivalentAdjustment(pzTI->fFedetaxDivRclm, fTaxRate);

/ *
** Add the federal tax equivalent adjustments(from the values without any tax
** effects) and calculate Federal Tax equivalent returns
* /
zTaxRor = *pzRor;

zTaxRor.fNetFlow += fFIncAdj;
zTaxRor.fWtFlow += fWtFIncAdj;


// End AI = End AI + End AI adj + AI Rclm Adj  and Begin AI = Begin AI + Begin
AI Adj zTaxRor.fEndAI += (fFEndAIAdj + fFAIRclmAdj); zTaxRor.fBeginAI +=
fFBeginAIAdj;

// End AD = End AD + End AD adj + AD Rclm Adj  and Begin AD = Begin AD + Begin
AD Adj zTaxRor.fEndAD += (fFEndADAdj + fFADRclmAdj); zTaxRor.fBeginAD +=
fFBeginADAdj;

// Call function to calculate Federal Taxable rors
zErr = CalculateGFAndNFRor(&zTaxRor, TRUE);
if (zErr.iBusinessError != 0)
return zErr;

// CalculateGFAndNFRor calculates the numbers for GTW and NTW returns, take the
values
// from there and update GTWAftertax and NTWAftertax returns.
pzRor->zAllRor.zIndex[GTWTaxEquivRor - 1].fBaseRorIdx =
zTaxRor.zAllRor.zIndex[GTWRor - 1].fBaseRorIdx;
pzRor->zAllRor.zIndex[NTWTaxEquivRor - 1].fBaseRorIdx =
zTaxRor.zAllRor.zIndex[NTWRor - 1].fBaseRorIdx; } // Tax equivalent

return zErr;
} / * Taxrelatedreturns */

/**
** Function to create a Perform record out of a dailyinfo record (for a perfkey
** in pkey table).
**/
void CreatePerformFromDInfo(PKEYTABLE zPTable, int iKey, int iDate,
                            SUMMDATA *pzPerform) {
  InitializePerform(pzPerform, zPTable.pzPKey[iKey].pzDInfo[iDate].lDate);

  pzPerform->iPortfolioID = zPTable.pzPKey[iKey].zPK.iPortfolioID;
  pzPerform->iID = zPTable.pzPKey[iKey].zPK.iID;

  pzPerform->fMktVal =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fMktVal, 2);
  pzPerform->fBookValue =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fBookValue, 2);

  pzPerform->fAccrInc =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fAccrInc, 2);
  pzPerform->fAccrDiv =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fAccrDiv, 2);

  pzPerform->fNetFlow =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fNetFlow +
                      zPTable.pzPKey[iKey].pzDInfo[iDate].fFees,
                  2);
  pzPerform->fNotionalFlow =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fNotionalFlow, 2);
  pzPerform->fCumFlow =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fCumFlow +
                      zPTable.pzPKey[iKey].pzDInfo[iDate].fCumFees,
                  2);
  pzPerform->fWtdFlow =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdFlow +
                      zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdFees,
                  2);

  /* pzPerform->fFees = RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fFees,
   * 2);*/
  pzPerform->fFees =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fFees -
                      zPTable.pzPKey[iKey].pzDInfo[iDate].fFeesOut,
                  2);
  pzPerform->fFeesOut =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fFeesOut, 2);
  pzPerform->fCumFees =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fCumFees, 2);
  pzPerform->fWtdFees =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdFees -
                      zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdFeesOut,
                  2);
  pzPerform->fCumFeesOut =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fCumFeesOut, 2);
  pzPerform->fWtdFeesOut =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdFeesOut, 2);

  pzPerform->fCNFees =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fCNFees, 2);
  pzPerform->fCumCNFees =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fCumCNFees, 2);
  pzPerform->fWtdCNFees =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdCNFees, 2);

  pzPerform->fIncome =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fIncome, 2);
  pzPerform->fCumIncome =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fCumIncome, 2);
  pzPerform->fWtdInc =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWtdInc, 2);

  pzPerform->fEstAnnIncome =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fEstAnnIncome, 2);

  pzPerform->fExchRateBase = zPTable.pzPKey[iKey].pzDInfo[iDate].fExchRateBase;
  pzPerform->lDaysSinceNond =
      zPTable.pzPKey[iKey].pzDInfo[iDate].lDaysSinceNond;
  pzPerform->lDaysSinceLast = zPTable.pzPKey[iKey].pzDInfo[iDate].lDate -
                              zPTable.pzPKey[iKey].pzDInfo[0].lDate;

  pzPerform->fPurchases =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fPurchases, 2);
  pzPerform->fSales =
      RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fSales, 2);
  if (strcmp(zPTable.pzPKey[iKey].zPK.sTotalRecInd, "T") == 0) {
    pzPerform->fContribution =
        RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fContributions, 2);
    pzPerform->fWithdrawals =
        RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWithdrawals, 2);
  } else {
    pzPerform->fTransfersIn =
        RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fContributions, 2);
    pzPerform->fTransfersOut =
        RoundDouble(zPTable.pzPKey[iKey].pzDInfo[iDate].fWithdrawals, 2);
  }

  if (zPTable.pzPKey[iKey].pzDInfo[iDate].bPeriodEnd) {
    if (lpfnIsItAMonthEnd(zPTable.pzPKey[iKey].pzDInfo[iDate].lDate))
      strcpy_s(pzPerform->sPerformType, "M");
    else
      strcpy_s(pzPerform->sPerformType, "F");
  } else
    strcpy_s(pzPerform->sPerformType, "D");
} /* createperformfromdinfo */

void CreateMonthsumFromTwoSummdata(SUMMDATA zCurrentSum, SUMMDATA zPreviousSum,
                                   SUMMDATA *pzMSum, BOOL bCumOnly) {
  if (!bCumOnly) {
    pzMSum->iPortfolioID = zCurrentSum.iPortfolioID;
    pzMSum->iID = zCurrentSum.iID;

    pzMSum->lPerformDate = lpfnCurrentMonthEnd(zCurrentSum.lPerformDate);
    pzMSum->fMktVal = zCurrentSum.fMktVal;
    pzMSum->fBookValue = zCurrentSum.fBookValue;
    pzMSum->fAccrInc = zCurrentSum.fAccrInc;
    pzMSum->fAccrDiv = zCurrentSum.fAccrDiv;
    strcpy_s(pzMSum->sPerformType, zCurrentSum.sPerformType);
  }

  if (strcmp(zPreviousSum.sPerformType, "T") != 0) {
    pzMSum->fNetFlow =
        RoundDouble(zCurrentSum.fNetFlow + zPreviousSum.fNetFlow, 2);
    pzMSum->fNotionalFlow =
        RoundDouble(zCurrentSum.fNotionalFlow + zPreviousSum.fNotionalFlow, 2);
    pzMSum->fCumFlow = 0;
    pzMSum->fWtdFlow = 0; // will get calculated later
    pzMSum->fIncome =
        RoundDouble(zCurrentSum.fIncome + zPreviousSum.fIncome, 2);
    pzMSum->fCumIncome = 0;
    pzMSum->fWtdInc = 0; // will get calculated later
    pzMSum->fFees = RoundDouble(zCurrentSum.fFees + zPreviousSum.fFees, 2);
    pzMSum->fFeesOut =
        RoundDouble(zCurrentSum.fFeesOut + zPreviousSum.fFeesOut, 2);
    pzMSum->fCumFees = 0;
    pzMSum->fWtdFees = 0;
    pzMSum->fCumFeesOut = 0;
    pzMSum->fWtdFeesOut = 0;
    pzMSum->fCNFees =
        RoundDouble(zCurrentSum.fCNFees + zPreviousSum.fCNFees, 2);
    pzMSum->fCumCNFees = 0;
    pzMSum->fWtdCNFees = 0;
    pzMSum->fExchRateBase = 1.0;
    pzMSum->lDaysSinceNond = 0;
    pzMSum->lDaysSinceLast = 0;
    pzMSum->fPurchases =
        RoundDouble(zCurrentSum.fPurchases + zPreviousSum.fPurchases, 2);
    pzMSum->fSales = RoundDouble(zCurrentSum.fSales + zPreviousSum.fSales, 2);
    pzMSum->fContribution =
        RoundDouble(zCurrentSum.fContribution + zPreviousSum.fContribution, 2);
    pzMSum->fWithdrawals =
        RoundDouble(zCurrentSum.fWithdrawals + zPreviousSum.fWithdrawals, 2);
    pzMSum->fTransfersIn =
        RoundDouble(zCurrentSum.fTransfersIn + zPreviousSum.fTransfersIn, 2);
    pzMSum->fTransfersOut =
        RoundDouble(zCurrentSum.fTransfersOut + zPreviousSum.fTransfersOut, 2);
  } else {
    pzMSum->fNetFlow = RoundDouble(zCurrentSum.fNetFlow, 2);
    pzMSum->fNotionalFlow = RoundDouble(zCurrentSum.fNotionalFlow, 2);
    pzMSum->fCumFlow = 0;
    pzMSum->fWtdFlow = 0; // will get calculated later
    pzMSum->fIncome = RoundDouble(zCurrentSum.fIncome, 2);
    pzMSum->fCumIncome = 0;
    pzMSum->fWtdInc = 0; // will get calculated later
    pzMSum->fFees = RoundDouble(zCurrentSum.fFees, 2);
    pzMSum->fFeesOut = RoundDouble(zCurrentSum.fFeesOut, 2);
    pzMSum->fCumFees = 0;
    pzMSum->fWtdFees = 0;
    pzMSum->fCumFeesOut = 0;
    pzMSum->fWtdFeesOut = 0;
    pzMSum->fFees = RoundDouble(zCurrentSum.fCNFees, 2);
    pzMSum->fCumCNFees = 0;
    pzMSum->fWtdCNFees = 0;
    pzMSum->fExchRateBase = 1.0;
    pzMSum->lDaysSinceNond = 0;
    pzMSum->lDaysSinceLast = 0;
    pzMSum->fPurchases = RoundDouble(zCurrentSum.fPurchases, 2);
    pzMSum->fSales = RoundDouble(zCurrentSum.fSales, 2);
    pzMSum->fContribution = RoundDouble(zCurrentSum.fContribution, 2);
    pzMSum->fWithdrawals = RoundDouble(zCurrentSum.fWithdrawals, 2);
    pzMSum->fTransfersIn = RoundDouble(zCurrentSum.fTransfersIn, 2);
    pzMSum->fTransfersOut = RoundDouble(zCurrentSum.fTransfersOut, 2);
  }
} /* createperformfromdinfo */

/**
** Function to calculate Weighted Average Performance for keys which are
** created using a perfrule that has WtdRecInd = 'W'.
** /
ERRSTRUCT WeightedAveragePerformance(PKEYTABLE *pzPTable, PERFRULETABLE zRTable,
PKEYASSETTABLE zPATable,ASSETTABLE zATable,
long lLastPerfDate, long lCurrentPerfDate)
{
PARENTRULETABLE zPRTable;
VALIDDPTABLE    zVDPTable;
ERRSTRUCT       zErr;
long            lStartDate;
int             i;

lpprInitializeErrStruct(&zErr);

if (lpfnCurrentMonthEnd(lLastPerfDate))
lStartDate = lLastPerfDate;
else
lStartDate = lpfnLastMonthEnd(lLastPerfDate);

zPRTable.iCapacity = 0;
InitializeParentRuleTable(&zPRTable);

// Get a list of unique parent perfrule_no for the account.
for (i = 0; i < pzPTable->iCount; i++)
{
// If the key has been deleted from db but still in memory table, ignore
if (pzPTable->pzPKey[i].bDeletedFromDB == TRUE)
continue;

// If the key was deleted on or before the start date, ignore it
if (pzPTable->pzPKey[i].zPK.lDeleteDate != 0 &&
pzPTable->pzPKey[i].zPK.lDeleteDate <= lStartDate)
continue;

// If not a child, ignore it
if (pzPTable->pzPKey[i].zPK.sParentChildInd[0] != 'C')
continue;

zErr = AddParentRuleIfNew(&zPRTable, pzPTable->pzPKey[i].lParentRuleNo);
if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
{
InitializeParentRuleTable(&zPRTable);
return zErr;
}
} // for on i

// If no rule for weighted performance, nothing to do
if (zPRTable.iCount == 0)
return zErr;

if (lCurrentPerfDate <= lStartDate)
return(lpfnPrintError("Invalid Dates", pzPTable->pzPKey[0].zPK.iID, 0, "",
999, 0, 0, "CALCPERF WTDAVGPERF1", FALSE));

/ *
** Find out what weighted parent perfkeys will be required based on the list
** of rules obtained in last step and dates when their children are valid. If
** a key matching those requirements already exist, find it.
* /
zVDPTable.iVDPRCreated = 0;
InitializeValidDPTable(&zVDPTable);
for (i = 0; i < zPRTable.iCount; i++)
{
zErr = RequiredKeysForParentRule(&zVDPTable, zPRTable.plPRule[i],
lStartDate, lCurrentPerfDate, *pzPTable);
if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
return zErr;
} // For on i

/ *
** Create any weighted parent key which is required but does not already
** exist, delete any extra weighted parent key which might have existed
** before but is not required now(in no case, go prior to startdate) and
** also get perform, gfror and nfror records for rest of the keys.
* /
zErr = GetWtdInfoAndDelKeys(pzPTable, zRTable, &zVDPTable, zPATable,
zATable, lLastPerfDate, lStartDate, lCurrentPerfDate);
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return zErr;

// Now calculate performance on all the parent weighted keys
zErr = CalcWtdPerformance(zVDPTable, pzPTable, lLastPerfDate, lStartDate,
lCurrentPerfDate); if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) return
zErr;

return zErr;
} / * WeightedAveragePerformance */

/**
** Function to create ValidDatePerfrule for the given perfrule and dates. The
** objective of the function is to find the largest continuous parent key(s)
** required (based on the date ranges the children are active) between the start
** date and end date. (E.g. let's say the given perfrule is based on children
** rules 1 and 2 and the start date is 12/31/97 and end date is 6/30/98. Let's
** also say there are 3 keys based on rule 1, first starting on 1/1/98 and
** ending on 2/5/98, second starting on 2/26/98 and ending on 5/3/98 and third
** starting on 6/20/98 and is live on 6/30/98. Also assume there are two keys
** based on perfrule 2, first starting on 1/31/98 and ending on 4/3/98 and
** second starting on 6/10/98 and endingin on 6/26/98. Then this function
** should create two records for the parent key, first starting on 1/1/98 and
** ending on 5/3/98 and second starting on 6/10/98 and active on 6/30/98.)
** /
ERRSTRUCT RequiredKeysForParentRule(VALIDDPTABLE *pzVDPTable, long lParentRule,
long lStartDate, long lEndDate, PKEYTABLE zPTable)
{
VALIDDATEPRULE zVDPR;
ERRSTRUCT      zErr;
char           *sResultString, cLastChar;
int            i, j;

long           lKeyStartDate, lKeyEndDate;

memset ( &zVDPR, 0, sizeof (VALIDDATEPRULE));
lpprInitializeErrStruct(&zErr);

// Allocate enough space for the result string
sResultString = malloc(lEndDate - lStartDate);

// entire result array is invalid
memset(sResultString, 'N', lEndDate - lStartDate);

/ *
** Go through all the keys in the system, for every child key whose parent
** is current perfrule, check the dates. Set the resultstring to 'Y'
** for the date range for which the key is live(and which falls between
** Start Date and End Date).
* /
for (j = 0; j < zPTable.iCount; j++)
{
// If the key has been deleted from db but still in memory table, ignore
if (zPTable.pzPKey[j].bDeletedFromDB == TRUE)
continue;

// If not the child key or current rule is not its parent, ignore it
if (zPTable.pzPKey[j].zPK.sParentChildInd[0] != 'C' ||
zPTable.pzPKey[j].lParentRuleNo != lParentRule)
continue;

// If the key was deleted on or before StartDate, ignore it
if (zPTable.pzPKey[j].zPK.lDeleteDate != 0 &&
zPTable.pzPKey[j].zPK.lDeleteDate <= lStartDate)
continue;

// Get the start date which we are interested in
if (zPTable.pzPKey[j].zPK.lInitPerfDate >= lStartDate + 1)
lKeyStartDate = zPTable.pzPKey[j].zPK.lInitPerfDate;
else
lKeyStartDate = lStartDate + 1;

// Get the end date which we are interested in
if (zPTable.pzPKey[j].zPK.lDeleteDate > lEndDate ||
zPTable.pzPKey[j].zPK.lDeleteDate == 0)
lKeyEndDate = lEndDate;
else
lKeyEndDate = zPTable.pzPKey[j].zPK.lDeleteDate;

if (lKeyEndDate > lEndDate || lKeyStartDate < lStartDate + 1)
{
free(sResultString);
return(lpfnPrintError("Invalid Dates", zPTable.pzPKey[0].zPK.iID, 0, "",
999, 0, 0, "CALCPERF REQUIREDKEYS1", FALSE));
}

memset(sResultString + (lKeyStartDate - lStartDate - 1), 'Y',
(lKeyEndDate - lKeyStartDate + 1));
} // for on j

// Create one record for each continuous stream of 'Y' in the result string
cLastChar = 'N';
for (j = 0; j < lEndDate - lStartDate; j++)
{
if (*(sResultString + j) != cLastChar)
{
if (cLastChar == 'N')
{
InitializeValidDatePrule(&zVDPR);
zVDPR.lStartDate = lStartDate + 1 + j;
zVDPR.lPerfruleNo = lParentRule;
AddValidDatePRuleToTable(pzVDPTable, zVDPR);
}
else
pzVDPTable->pzVDPR[pzVDPTable->iNumVDPR - 1].lEndDate = lStartDate + j;
} / If current charachter is not same as last charachter

cLastChar = *(sResultString + j);
} // For on j

// Free memory allocated by malloc
free(sResultString);

// For each record in the table, try to find an existing key
for (i = 0, j = 0; i < pzVDPTable->iNumVDPR; i++)
{
/ *
** Since both pzVDPTable and zPTable are sorted on dates(first on start
** date and second on init_perf_date), for the current value of i, j does
** not have to start at 0, it can start where it was left off for the
** previous value of i
* /
while (pzVDPTable->pzVDPR[i].lPerfkeyNo == 0 && j < zPTable.iCount)
{
if (zPTable.pzPKey[j].zPK.lRuleNo == lParentRule &&
zPTable.pzPKey[j].bDeletedFromDB == FALSE &&
(zPTable.pzPKey[j].zPK.lDeleteDate == 0 ||
zPTable.pzPKey[j].zPK.lDeleteDate >= pzVDPTable->pzVDPR[i].lStartDate))
{
/ *
** If the key found which has inception same as the required start
** date, use it. Else If the key inception was before the required
** start date and required start date is exactly equal to the system
** start date + 1 then also this key can be used. If a usable key is
** found, update its delete date to be equal to the required end date.
* /
if ( (zPTable.pzPKey[j].zPK.lInitPerfDate == pzVDPTable->pzVDPR[i].lStartDate)
|| (zPTable.pzPKey[j].zPK.lInitPerfDate < pzVDPTable->pzVDPR[i].lStartDate &&
pzVDPTable->pzVDPR[i].lStartDate == lStartDate + 1))
{
pzVDPTable->pzVDPR[i].lPerfkeyNo = zPTable.pzPKey[j].zPK.lPerfkeyNo;
zPTable.pzPKey[j].zPK.lDeleteDate = pzVDPTable->pzVDPR[i].lEndDate;
}
}

j++;
} //  While j < numpkey and an existing perfkey not found
} // for on i

return zErr;
} /* RequiredKeysForParentRule */

/**
** Function to create new parent perfkeys which are required but does not exist
** and also to get rid of any existing keys which are not required any more.
** /
ERRSTRUCT GetWtdInfoAndDelKeys(PKEYTABLE *pzPTable, PERFRULETABLE zPRTable,
VALIDDPTABLE *pzVDPTable, PKEYASSETTABLE zPATable,
ASSETTABLE zATable, long lLastPerfDate, long lStartDate, long lEndDate)
{
ERRSTRUCT     zErr;
WTDDAILYINFO  zWDInfo;
SUMMDATA      zSummdata;
RTRNSET       zRtrnset;
int						i, j, iKIndex;
BOOL					bDeleteRecord;

lpprInitializeErrStruct(&zErr);

/ *
** First step is to get all the permanent records for the account and store
** the one which are required to be recalculated(weighted parent key's
** record between ket start and end dates) or are required in the calculation
** (children key's records). Get rid of records(if any) of the weighted
** parent key which we don't need any more.
* /
while (zErr.iSqlError == 0)
{
InitializeWtdDailyInfo(&zWDInfo);

lpprMonthlyWeightedInfo(&zSummdata, &zRtrnset,
pzPTable->pzPKey[0].zPK.iPortfolioID, lLastPerfDate, &zErr); if (zErr.iSqlError
== SQLNOTFOUND) break; else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return(lpfnPrintError("Error Reading Perform Monthly Cursor",
pzPTable->pzPKey[0].zPK.iID, 0, "", 0, zErr.iBusinessError,
zErr.iIsamCode, "CALCPERF GETWTDINFO1", FALSE));

//Try to find the key in the table, if not found delete perform, gfror,etc
// SB - TAKE ANOTHER LOOK AT THIS LOGIC 2/11/99
//iKIndex = FindPerfkeyByPerfkeyNo(*pzPTable, zSummdata.lPerfkeyNo);
if (iKIndex < 0)
{
// Not allowed to touch anything on or prior to start date
if (zWDInfo.lDate <= lStartDate)
continue;

// SB - TAKE ANOTHER LOOK AT THIS LOGIC 2/11/99
//      lpprDeletePerfkey ( zSummdata.lPerformNo, &zErr );
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return zErr;
}

/ *
** If the key is for a weighted parent, see if it is in WtdDailyInfo table,
** if not then delete it, if it is there but it is for a date greater than
** the end date in WtdDailyInfo then also delete the reocrd.
* /
if (strcmp(pzPTable->pzPKey[iKIndex].zPK.sParentChildInd, "P") == 0 &&
strcmp(pzPTable->pzPKey[iKIndex].sRecordType, "W") == 0)
{
bDeleteRecord = FALSE;
// SB - TAKE ANOTHER LOOK AT THIS LOGIC 2/11/99
//      j = FindPerfkeyInValidDPRTable(*pzVDPTable, zSummdata.lPerfkeyNo);
if (j == -1)
bDeleteRecord = TRUE;
else
{
if (lpfnIsItAMonthEnd(zWDInfo.lDate) == FALSE)
{
if (zWDInfo.lDate != pzVDPTable->pzVDPR[j].lStartDate &&
zWDInfo.lDate != pzVDPTable->pzVDPR[j].lEndDate)
bDeleteRecord = TRUE;
}
else if (zWDInfo.lDate > pzVDPTable->pzVDPR[j].lEndDate &&
pzVDPTable->pzVDPR[j].lEndDate != 0)
bDeleteRecord = TRUE;

} // if j != -1

if (bDeleteRecord == TRUE)
{
/ *      zErr = DeletePerformSetByPerfNo(lPerformNo, FALSE);
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return zErr;
* /
lpprDeletePeriodSummdataForADateRange (zRtrnset.iID, zWDInfo.lDate, &zErr );
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return zErr;

lpprDeletePeriodRtrnsetForADateRange  (zRtrnset.iID,  zWDInfo.lDate, &zErr );
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return zErr;

continue;
}
} // If parent and a weighted key

/ *
** When control comes here, the current record is either for a child key
** (or parent key which is not Weighted Average) or a parent key which is
** usable, so add the WtdDailyInfo to the key.
* /
zErr = AddWtdDailyInfoToPerfkey(&pzPTable->pzPKey[iKIndex], zWDInfo);
if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
return zErr;
} // while no error

/ *
** Function "DeletePerformSetByPerformNo" deletes records from perform, gfror,
** nfror, etc., but it does not delete record grom perfkey1 table. Weighted
** parent Keys which exist in the perfkey1 table(and in memory table) which
** are not required anymore(and whose perform, gfror, etc. records have
** been deleted in the previous loop) should be deleted too.
* /
for (i = 0; i < pzPTable->iCount; i++)
{
if (strcmp(pzPTable->pzPKey[i].sRecordType, "W") != 0 ||
strcmp(pzPTable->pzPKey[i].zPK.sParentChildInd, "C") == 0 ||
(pzPTable->pzPKey[i].zPK.lDeleteDate != 0 &&
pzPTable->pzPKey[i].zPK.lDeleteDate <= lStartDate))
continue;

pzPTable->pzPKey[i].bDeleteKey = FALSE;
/ *
** This key is a parent weighted key which is either live or was deleted
** after the start date. Search for it in the VDPRTable, if not found
** then we don't need this key anymore. If the key was incepted after the
** start date then physically delete it from the database else mark it
** deleted as on the start date.
* /
j = FindPerfkeyInValidDPRTable(*pzVDPTable,pzPTable->pzPKey[i].zPK.lPerfkeyNo);
if (j == -1)
{
if (pzPTable->pzPKey[i].zPK.lInitPerfDate <= lStartDate)
pzPTable->pzPKey[i].zPK.lDeleteDate = lStartDate;
else
{
pzPTable->pzPKey[i].bDeleteKey = TRUE;
pzPTable->pzPKey[i].bDeletedFromDB = TRUE;
}
}
} // for on i

for (i = 0; i < pzPTable->iCount; i++)
pzPTable->pzPKey[i].lScratchDate = 0;

while (zErr.iSqlError == 0)
{
lpprSelectDailyRor(&zRtrnset, pzPTable->pzPKey[0].zPK.iPortfolioID, &zErr);
if (zErr.iSqlError == SQLNOTFOUND)
break;
else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return(lpfnPrintError("Error Fetching Ror Daily Cursor",
pzPTable->pzPKey[0].zPK.iID, 0, "", 0, zErr.iBusinessError, zErr.iIsamCode,
"CALCPERF GETWTDINFO2", FALSE));

i = FindPerfkeyInValidDPRTable(*pzVDPTable, pzPTable->pzPKey[0].zPK.lPerfkeyNo);
if (i >= 0)
{
// SB - TAKE ANOTHER LOOK AT THIS LOGIC 2/11/99
//      iKIndex = FindPerfkeyByPerfkeyNo(*pzPTable, zSummdata.lPerfkeyNo);
//      if (iKIndex >= 0 && (lTempLong == lEndDate || lTempLong <
lLastPerfDate))
//pzPTable->pzPKey[iKIndex].lScratchPerfNo = zRtrnset.lPerformNo;
}
} // while no error in daily loop

/ *
** Till now we have deleted keys(or have marked them for deletion), now
** create new keys. When creating new key, find the first asset which matches
** any of the children and then using Parent script and that asset, create
** a perfscript. Use that perfscript for the parent(irrespective of whether
** the parent rrule passes that script or not).
* /
for (i = 0; i < pzVDPTable->iNumVDPR; i++)
{
if (pzVDPTable->pzVDPR[i].lPerfkeyNo == 0)
{
zErr = PerfkeyForWeightedParent(pzPTable, zPATable, zATable, zPRTable,
pzVDPTable->pzVDPR[i].lPerfruleNo,
pzVDPTable->pzVDPR[i].lStartDate,
pzVDPTable->pzVDPR[i].lEndDate);
if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
return zErr;

/ *
** Update current record's perfkey no to be equal to that of the newly
** added key.
* /
j = pzPTable->iCount - 1;
pzVDPTable->pzVDPR[i].lPerfkeyNo = pzPTable->pzPKey[j].zPK.lPerfkeyNo;
} // If key does not exist
} // for on i


return zErr;
} // GetWtdInfoAndDelKeys


/**
** Function that calculates weighted performance for a parent perfkey using
** the following formula :
**   Ror = Sum(Wi * Ri) / Sum(Wi)
** Where,
**    Wi = abs(BegMV + BegAI + BegAD + Wtd Flow) * date  delta for the ith child
**    Ri = Ror for the ith child
** Three dates(and some other variables) are passed to this function,
**  lastPerfDate : Date on which performance for this account was run last time
**                 (used to identify scratch perform for daily tables)
**  lStartDate and End Date : For any key, performance is calculated for
**                            inception(if any), deletion(if any) and all month
**                            end dates between Start Date(exclusive) and
**                            End Date(inclusive).
**/
ERRSTRUCT CalcWtdPerformance(VALIDDPTABLE zVDPTable, PKEYTABLE *pzPTable,
                             long lLastPerfDate, long lStartDate,
                             long lEndDate) {
  ERRSTRUCT zErr;
  int iKIdx, i, j, k;
  double zWi, zGRi, zNRi, zGNumerator, zGDenominator, zNNumerator;
  double zNDenominator, fNAValue;
  double zTempDec1, zTempDec2;
  long lKeyInitDate, lKeyStartDate, lKeyEndDate, lLastMEndDate, lCurrentDate;
  WTDDAILYINFO zCurrentVal, zLastNondVal;
  ALLRORS zRor;
  SUMMDATA zPerform;

  lpprInitializeErrStruct(&zErr);
  fNAValue = NAVALUE;

  /*
  ** Go through Valid date perfrule table and for each record calculate
  ** performance. Update fields for the corresponding key in perfkey table.
  */
  for (i = 0; i < zVDPTable.iNumVDPR; i++) {
    iKIdx = FindPerfkeyByPerfkeyNo(*pzPTable, zVDPTable.pzVDPR[i].lPerfkeyNo);
    if (iKIdx < 0)
      return (lpfnPrintError("Programming Error", pzPTable->pzPKey[0].zPK.iID,
                             0, "", 999, 0, 0, "CALCPERF CALCWTD1", FALSE));

    /* Initialize begin and end ror index */
    InitializeAllRors(&pzPTable->pzPKey[iKIdx].zBeginIndex, TRUE);
    InitializeAllRors(&pzPTable->pzPKey[iKIdx].zNewIndex, TRUE);
    /*
    ** If the first data in WDInfo is for the (system) start date, that should
    ** be the starting value of the index, else index will start at 100.
    ** Also note that right now only base ror for rortype = "A"(MV + AI + AD)
    ** is being calculated and it is being stored at array index 0(as opposed
    ** to use GetRorTypeIndex function to get the array index).
    */
    if (pzPTable->pzPKey[iKIdx].iWDInfoCount > 0) {
      if (pzPTable->pzPKey[iKIdx].pzWDInfo[0].lDate == lStartDate) {
        // SB 5/27/15, simplified the structure
        pzPTable->pzPKey[iKIdx].zBeginIndex.fBaseRorIdx[GTWRor - 1] =
            pzPTable->pzPKey[iKIdx].pzWDInfo[GTWRor - 1].fNBaseRor;
        /* SB - 2/23/99
        pzPTable->pzPKey[iKIdx].zBeginIndex.zGFIdx[0].fBaseRorIdx =
        pzPTable->pzPKey[iKIdx].pzWDInfo[0].fGBaseRor;
        pzPTable->pzPKey[iKIdx].zBeginIndex.zIndex[0].fBaseRorIdx =
        pzPTable->pzPKey[iKIdx].pzWDInfo[0].fNBaseRor;*/
      }
    }

    lKeyInitDate = pzPTable->pzPKey[iKIdx].zPK.lInitPerfDate;
    if (lKeyInitDate == zVDPTable.pzVDPR[i].lStartDate) /* new key */
    {
      lLastMEndDate = lKeyInitDate;
      lKeyStartDate = lKeyInitDate;
    } else /* old key */
    {
      lLastMEndDate = lStartDate;
      lKeyStartDate = lStartDate + 1;
    }

    if (zVDPTable.pzVDPR[i].lEndDate == 0) /* live key */
      lKeyEndDate = lEndDate;
    else
      lKeyEndDate = zVDPTable.pzVDPR[i].lEndDate;

    for (lCurrentDate = lKeyStartDate; lCurrentDate <= lKeyEndDate;
         lCurrentDate++) {
      if (lCurrentDate == lKeyInitDate || lCurrentDate == lKeyEndDate ||
          lpfnCurrentMonthEnd(lCurrentDate)) {
        InitializeAllRors(&zRor, FALSE);
        InitializePerform(&zPerform, lCurrentDate);
        zGNumerator = zGDenominator = zNNumerator = zNDenominator = 0.0;

        for (j = 0; j < pzPTable->iCount; j++) {
          /*
          ** If the key has been deleted from db but still in memory table,
          ** ignore it
          */
          if (pzPTable->pzPKey[j].bDeletedFromDB == TRUE)
            continue;

          if (strcmp(pzPTable->pzPKey[j].zPK.sParentChildInd, "C") == 0 &&
              pzPTable->pzPKey[j].lParentRuleNo ==
                  zVDPTable.pzVDPR[i].lPerfruleNo &&
              pzPTable->pzPKey[j].iWDInfoCount > 0) {
            zWi = zGRi = zNRi = 0.0;
            for (k = pzPTable->pzPKey[j].iWDInfoCount - 1; k >= 0; k--) {
              zCurrentVal = pzPTable->pzPKey[j].pzWDInfo[k];
              if (zCurrentVal.lDate < lLastMEndDate)
                break;
              else if (zCurrentVal.lDate > lCurrentDate)
                continue;
              else {
                /*
                ** For a new key, LastMEndDate = KeyInitDate for others it
                ** it is Last month end. We are interested in LastMEndDate
                ** only if it is key's inception date.
                */
                if (zCurrentVal.lDate == lLastMEndDate &&
                    lLastMEndDate != lKeyInitDate)
                  break;

                /* Ending Values */
                if (zCurrentVal.lDate == lCurrentDate) {
                  zPerform.fMktVal += zCurrentVal.fMktVal;
                  zPerform.fAccrInc += zCurrentVal.fAccrInc;
                  zPerform.fAccrDiv += zCurrentVal.fAccrDiv;
                  zPerform.fNetFlow += zCurrentVal.fNetFlow;
                  zPerform.fIncome += zCurrentVal.fIncome;
                  zPerform.fFees += zCurrentVal.fFees;
                  zPerform.fFeesOut += zCurrentVal.fFeesOut;
                  zPerform.fCNFees += zCurrentVal.fCNFees;

                  /*
                  ** If it is parent key's inception, no need to calculate
                  ** performance, start index at 100.
                  */
                  if (zCurrentVal.lDate == lKeyInitDate)
                    break;
                }

                /* If this is child key's last record, ignore it */
                if (k == 0)
                  break;

                zLastNondVal = pzPTable->pzPKey[j].pzWDInfo[k - 1];
                /*
                ** Numerator = Sum(Wi * Ri)
                ** Denominator = Sum(Wi)
                **  Where
                **  Wi = abs(BegMV + BegAI + BeginAD + WtdFlow) * DateDelta
                **  Ri = Ror
                */
                zGRi = GetRorFromTwoIndexes(zLastNondVal.fGBaseRor,
                                            zCurrentVal.fGBaseRor);
                zNRi = GetRorFromTwoIndexes(zLastNondVal.fNBaseRor,
                                            zCurrentVal.fNBaseRor);
                /* If any of the Ror is invalid, ignore the record */
                if ((zGRi == fNAValue) || (zNRi == fNAValue))
                  continue;

                /* Beg Val = Beg MV + AI + AD */
                zTempDec1 = zLastNondVal.fMktVal + zLastNondVal.fAccrInc;
                zTempDec1 += zLastNondVal.fAccrDiv;
                zTempDec1 += zCurrentVal.fWtdFlow;

                /* Wi = abs ((beg val + wtdflow) * date delta ) */
                zTempDec2 = zCurrentVal.lDate - zLastNondVal.lDate;
                zWi = zTempDec1 * zTempDec2;
                if (zWi < 0.0)
                  zWi *= -1.0;

                zTempDec1 = zWi * zGRi;
                zTempDec2 = zWi * zNRi;

                zGNumerator += zTempDec1;
                zGDenominator += zWi;

                zNNumerator += zTempDec2;
                zNDenominator += zWi;

              } /* if k > 0 and [k].lDate between begindate and CurrentDate*/
            } /* for k */
          } /* If a child found */
        } /* For on j */

        /* Ror = Sum(Wi * Ri) / Sum (Wi) */
        if (!IsValueZero(zGDenominator, 3)) // != 0.0)
        {
          // SB 5/27/15 simplified the structure
          zTempDec1 = zGNumerator / zGDenominator;
          zRor.fBaseRorIdx[GTWRor - 1] = zTempDec1 / 100.0;
        }

        if (!IsValueZero(zNDenominator, 3)) // !=  0.0)
        {
          // SB 5/27/15 simplified the structure
          zTempDec2 = zNNumerator / zNDenominator;
          zRor.fBaseRorIdx[NTWRor - 1] = zTempDec2 / 100.0;
        }

        /* Calculate new index */
        // CalculateNewRorIndex(pzPTable->pzPKey[iKIdx].zBeginIndex, zRor,
        //                    &pzPTable->pzPKey[iKIdx].zNewIndex, 0);

        /* What type of perform  record is it */
        if (lpfnCurrentMonthEnd(lCurrentDate))
          strcpy_s(zPerform.sPerformType, "M"); /* Month end */
        else
          strcpy_s(zPerform.sPerformType, "D"); /* Daily */

        if (lCurrentDate == zVDPTable.pzVDPR[i].lEndDate) /* Termination*/
        {
          strcpy_s(zPerform.sPerformType, "T");
          pzPTable->pzPKey[iKIdx].zPK.lDeleteDate = lCurrentDate;
        }

        if (lCurrentDate == lKeyInitDate)
          strcpy_s(zPerform.sPerformType, "I"); /* Inception */

        pzPTable->pzPKey[iKIdx].zPK.lLastPerfDate = lCurrentDate;
        zPerform.lPerformDate = lCurrentDate;

        /* Update LastMEndDate and begin index */
        lLastMEndDate = lCurrentDate;
        pzPTable->pzPKey[iKIdx].zBeginIndex = pzPTable->pzPKey[iKIdx].zNewIndex;

        if (strcmp(zPerform.sPerformType, "D") != 0) /* Monthly */
        {
          j = 0;
          k = -1;
          /* If record already exist, update it, else insert a new one */
          for (j = 0; j < pzPTable->pzPKey[iKIdx].iWDInfoCount; j++) {
            if (pzPTable->pzPKey[iKIdx].pzWDInfo[j].lDate == lCurrentDate) {
              k = j;
              pzPTable->pzPKey[iKIdx].pzWDInfo[j].bPerformUsed = TRUE;
              lpprUpdatePeriodSummdata(zPerform, &zErr);
              break;
            }
          }
          if (k == -1)
            lpprInsertPeriodSummdata(zPerform, &zErr);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;

          /* obsolete - RTRNSET is not supported anymore - vay
          // if this function will be re-activated, then this code should be
          replaced
          // by UNITVALUE functions instead
          if (k == -1)
          {
          lpprInsertPeriodRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[GTWRor -
          1], &zErr); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

          lpprInsertPeriodRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[NTWRor -
          1], &zErr); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;
          } // Record does not exist
          else
          {
          //
          lpprUpdateMonthlyRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[GTWRor -
          1], &zErr); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

          //
          lpprUpdateMonthlyRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[0],
          &zErr); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) return
          zErr; } // Record already exists */

          /* Update relevant dates in the perfkey */
          if (strcmp(zPerform.sPerformType, "M") == 0)
            pzPTable->pzPKey[iKIdx].zPK.lMePerfDate = lCurrentDate;

          pzPTable->pzPKey[iKIdx].zPK.lLndPerfDate = lCurrentDate;

        } /* If not daily */
        else {
          if (pzPTable->pzPKey[iKIdx].lScratchDate != 0) {
            lpprUpdateDailySummdata(
                zPerform, pzPTable->pzPKey[iKIdx].lScratchDate, &zErr);
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
              return zErr;

            /* obsolete - RTRNSET is not supported anymore - vay
            // if this function will be re-activated, then this code should be
            replaced
            // by UNITVALUE functions instead
            lpprUpdateDailyRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[GTWRor -
            1], pzPTable->pzPKey[iKIdx].lScratchDate, &zErr); if (zErr.iSqlError
            != 0 || zErr.iBusinessError != 0) return zErr;
            lpprUpdateDailyRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[NTWRor -
            1], pzPTable->pzPKey[iKIdx].lScratchDate, &zErr); if (zErr.iSqlError
            != 0 || zErr.iBusinessError != 0) return zErr;
            */

          } /* Record already exists */
          else {
            lpprInsertDailySummdata(zPerform, &zErr);
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
              return zErr;

            /* obsolete - RTRNSET is not supported anymore - vay
            // if this function will be re-activated, then this code should be
            replaced
            // by UNITVALUE functions instead
            lpprInsertDailyRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[GTWRor -
            1], &zErr ); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;

            lpprInsertDailyRor(pzPTable->pzPKey[iKIdx].zNewIndex.zIndex[NTWRor -
            1], &zErr ); if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
            return zErr;
            */
          } /* Record does not exist */

        } /* Daily */
      } /* if Key End Date or a month end date */
    } /* for on current date */
  }
  return zErr;
} // CalcWtdPerformance

/* This function "walks" the heap, starting
 * at the beginning (_pentry = NULL). It prints out each
 * heap entry's use, location, and size. It also prints
 * out information about the overall state of the heap as
 * soon as _heapwalk returns a value other than _HEAPOK.
 */
DllExport void heapdump(void) {
  _HEAPINFO hinfo;
  int heapstatus;
  char sMsg[300];
  long lTotSize;

  lTotSize = 0;
  hinfo._pentry = NULL;
  while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK) {
    sprintf_s(sMsg, "%6s block at %dp of size %d\n",
              (hinfo._useflag == _USEDENTRY ? "USED" : "FREE"), *hinfo._pentry,
              hinfo._size);
    lpfnPrintError(sMsg, 0, 0, "", 0, 0, 0, "MEMDEBUG", TRUE);
    if (hinfo._useflag == _USEDENTRY)
      lTotSize += hinfo._size;
  }

  sprintf_s(sMsg, "Total Memory used Is: %d\n", lTotSize);
  lpfnPrintError(sMsg, 0, 0, "", 0, 0, 0, "MEMDEBUG", TRUE);

  switch (heapstatus) {
  case _HEAPEMPTY:
    lpfnPrintError("OK - empty heap", 0, 0, "", 0, 0, 0, "MEMDEBUG", TRUE);
    break;
  case _HEAPEND:
    lpfnPrintError("OK - end of heap", 0, 0, "", 0, 0, 0, "MEMDEBUG", TRUE);
    break;
  case _HEAPBADPTR:
    lpfnPrintError("ERROR - bad pointer to heap", 0, 0, "", 0, 0, 0, "MEMDEBUG",
                   TRUE);
    break;
  case _HEAPBADBEGIN:
    lpfnPrintError("ERROR - bad start of heap", 0, 0, "", 0, 0, 0, "MEMDEBUG",
                   TRUE);
    break;
  case _HEAPBADNODE:
    lpfnPrintError("ERROR - bad node in heap", 0, 0, "", 0, 0, 0, "MEMDEBUG",
                   TRUE);
    break;
  }
} // heapdump