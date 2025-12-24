/**
*
* SUB-SYSTEM: payments
*
* FILENAME: digenerate1.c
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
// 2018-11-16 J# PER-9268 Wells Audit - Initialize variables, free
memory,deprecated string function  - sergeyn
**/

#include "payments.h"

BOOL APIENTRY DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved) {
  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    break;

  case DLL_PROCESS_DETACH:
    FreePayments();
    break;

  default:
    break;
  }

  return TRUE;

} // DllMain

extern "C" {

ERRSTRUCT STDCALL WINAPI GenerateDivInt(long lValDate, const char *sMode,
                                        const char *sType, int iID,
                                        const char *sSecNo, const char *sWi,
                                        const char *sSecXtend,
                                        const char *sAcctType, long lTransNo) {
  ERRSTRUCT zErr;
  PINFOTABLE zPInfoTable;
  PORTTABLE zPmainTable;
  long lStartDate, lForwardDate;

  lpprInitializeErrStruct(&zErr);
  zPmainTable.iPmainCreated = 0;
  zPInfoTable.iPICreated = 0;
  InitializePInfoTable(&zPInfoTable);
  lStartDate = lValDate - zSysSet.zSyssetng.iPaymentsStartDate;
  lForwardDate = lValDate + FORWARDDAYS;

  // If in batch mode, create the file with data from holdings, divint, etc.
  if (sMode[0] == 'B') {
    zErr = DivintUnloadAndSort(lForwardDate, lStartDate, sType);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    // Build PInfo table for dividends
    zErr = BuildPInfoTable(&zPInfoTable, "D", lForwardDate, sType);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
      InitializePInfoTable(&zPInfoTable); // Free up the memory
      return zErr;
    }
  } // batch mode
  else {
    zPInfoTable.iPICreated = 1;
    zPInfoTable.pzPRec = (PINFO *)realloc(zPInfoTable.pzPRec, sizeof(PINFO));
    if (zPInfoTable.pzPRec == NULL) {
      InitializePInfoTable(&zPInfoTable); // Free up the memory
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                             "DIVINT1", FALSE));
    }

    zPInfoTable.pzPRec[0].iID = iID;
    zPInfoTable.pzPRec[0].lStartPosition = 0;
    zPInfoTable.iNumPI = 1;
  } // single account mode

  // Build the table with all the branch account
  zErr = BuildPortmainTable(&zPmainTable, sMode, iID, zCTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    InitializePortTable(&zPmainTable);
    InitializePInfoTable(&zPInfoTable); // Free up the memory
    return zErr;
  }

  // Call the function which does actual work, Added lForwardDate to do Forward
  // Transactions
  zErr = DivintGeneration(zPmainTable, zPInfoTable, (char *)sSecNo, (char *)sWi,
                          (char *)sSecXtend, (char *)sAcctType, lTransNo,
                          lStartDate, lValDate, lForwardDate, (char *)sMode,
                          (char *)sType, zSATable, zCTable);

  // Free up the memory
  InitializePInfoTable(&zPInfoTable);
  InitializePortTable(&zPmainTable);

  return zErr;
} // GenerateDivInt

/**
** Function to build a table of all the subaccts in the system
**/
ERRSTRUCT BuildSubacctTable(SUBACCTTABLE *pzSATable) {
  ERRSTRUCT zErr;
  char sAcctType[2], sXrefAcctType[2];

  lpprInitializeErrStruct(&zErr);

  pzSATable->iNumSAcct = 0;
  while (!zErr.iSqlError) {
    lpprSelectSubacct(sAcctType, sXrefAcctType, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    if (pzSATable->iNumSAcct == NUMSUBACCT)
      return (lpfnPrintError("SubAcct Table Is Full", 0, 0, "", 997, 0, 0,
                             "DIVINT BUILDSACCT1", FALSE));

    strcpy_s(pzSATable->zSAcct[pzSATable->iNumSAcct].sAcctType, sAcctType);
    strcpy_s(pzSATable->zSAcct[pzSATable->iNumSAcct].sXrefAcctType,
             sXrefAcctType);
    pzSATable->iNumSAcct++;
  } /* while no error */

  return zErr;
} /* Buildsubaccttable */

/**
** Function to build a table of all the currencies and their exrate
**/
ERRSTRUCT BuildCurrencyTable(CURRTABLE *pzCTable) {
  ERRSTRUCT zErr;
  PARTCURR zPartCurr;

  lpprInitializeErrStruct(&zErr);

  pzCTable->iNumCurrency = 0;
  while (!zErr.iSqlError) {
    lpprSelectCurrency(&zPartCurr, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    if (pzCTable->iNumCurrency == NUMCURRENCY)
      return (lpfnPrintError("Currency Table Is Full", 0, 0, "", 997, 0, 0,
                             "DIVINT BUILDCURR1", FALSE));

    strcpy_s(pzCTable->zCurrency[pzCTable->iNumCurrency].sCurrId,
             zPartCurr.sCurrId);
    strcpy_s(pzCTable->zCurrency[pzCTable->iNumCurrency].sSecNo,
             zPartCurr.sSecNo);
    strcpy_s(pzCTable->zCurrency[pzCTable->iNumCurrency].sWi, zPartCurr.sWi);
    pzCTable->zCurrency[pzCTable->iNumCurrency].fCurrExrate =
        zPartCurr.fCurrExrate;
    pzCTable->iNumCurrency++;
  } /* while no error */

  return zErr;
} /* Buildcurrencytable */

/**
** Function to build portmain table
**/
ERRSTRUCT BuildPortmainTable(PORTTABLE *pzPTable, const char *sMode, int iID,
                             CURRTABLE zCTable) {
  ERRSTRUCT zErr;
  PARTPMAIN zTempPmain;

  lpprInitializeErrStruct(&zErr);
  InitializePortTable(pzPTable);

  while (!zErr.iSqlError) {
    if (strcmp(sMode, "B") == 0)
      lpprSelectAllPartPortmain(&zTempPmain, &zErr);
    else
      lpprSelectOnePartPortmain(&zTempPmain, iID, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      if (strcmp(sMode, "B") != 0)
        return (lpfnPrintError("Invalid Account", iID, 0, "", 1, SQLNOTFOUND, 0,
                               "DIVINT BUILDPTABLE", FALSE));

      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return zErr;

    if (pzPTable->iPmainCreated == pzPTable->iNumPmain) {
      pzPTable->iPmainCreated += NUMPDIRRECORD;

      pzPTable->pzPmain = (PARTPMAIN *)realloc(
          pzPTable->pzPmain, pzPTable->iPmainCreated * sizeof(PARTPMAIN));
      if (pzPTable->pzPmain == NULL)
        return (lpfnPrintError("Insufficient Memory", iID, 0, "", 997, 0, 0,
                               "DIVINT BUILDPTABLE2", FALSE));
    } // If table is full

    zTempPmain.iCurrIndex =
        FindCurrIdInCurrencyTable(zCTable, zTempPmain.sBaseCurrId);
    if (zTempPmain.iCurrIndex == -1)
      return (lpfnPrintError("Invalid Base Currency", iID, 0, "", 14, 0, 0,
                             "DIVINT BUILDPTABLE3", FALSE));

    pzPTable->pzPmain[pzPTable->iNumPmain++] = zTempPmain;
    if (strcmp(sMode, "B") !=
        0)   // if not a batch mode, we have already selected the
      break; // ID we want, break out of loop
  } // While no error

  return zErr;
} // BuildPortmainTable

/**
** Function to build pinfo table
**/
ERRSTRUCT BuildPInfoTable(PINFOTABLE *pzPTable, const char *sLibName,
                          long lValDate, const char *sType) {
  ERRSTRUCT zErr;
  char sFileName[90], sMsg[80];
  int iLastID;
  char sStr1[501], *sStr2;
  FILE *fp;
  long lLength, lPosition;

  lpprInitializeErrStruct(&zErr);

  if (strcmp(sLibName, "D") != 0 && strcmp(sLibName, "M") != 0 &&
      strcmp(sLibName, "A") != 0 && strcmp(sLibName, "F") != 0)
    return (lpfnPrintError(
        "Library Name Can Only Be D(dividend), M(maturity), A(amortization), "
        "OR F(orward) ",
        0, 0, "", 999, 0, 0, "DIVINT BUILDPITABLE1", FALSE));

  // Filename for sorted file on valdate
  strcpy_s(sFileName, MakePricingFileName(lValDate, sType, sLibName));

  fp = fopen(sFileName, "r");
  if (fp == NULL) {
    sprintf_s(sMsg, "Error Opening File %s", sFileName);
    return (lpfnPrintError(sMsg, 0, 0, "", 999, 0, 0, "DIVINT BUILDPTIABLE2",
                           FALSE));
  }

  iLastID = lPosition = 0;
  while (fgets(sStr1, 500, fp) != NULL) {
    lLength = strlen(sStr1) + 1;

    if (strcmp(sLibName, "D") == 0) {
      sStr2 = strtok(sStr1, "|"); // 'H' or 'D', ignore it
      sStr2 = strtok(NULL, "|");  // Account ID
    } else
      sStr2 = strtok(sStr1, "|"); // Account ID

    if (atoi(sStr2) == iLastID) // same as last account
      lPosition += lLength;
    else {
      if (pzPTable->iPICreated == pzPTable->iNumPI) {
        pzPTable->iPICreated += NUMPINFORECORD;
        pzPTable->pzPRec = (PINFO *)realloc(
            pzPTable->pzPRec, pzPTable->iPICreated * sizeof(PINFO));
        if (pzPTable->pzPRec == NULL) {
          zErr = lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                                "DIVINT BUILDPTABLE3", FALSE);
          break;
        }
      } // If table is full

      pzPTable->pzPRec[pzPTable->iNumPI].iID = atoi(sStr2);
      pzPTable->pzPRec[pzPTable->iNumPI].lStartPosition = lPosition;
      pzPTable->iNumPI++;
      iLastID = atoi(sStr2);
      lPosition += lLength;
    } // not same account as the last one
  } // Not EOF

  fclose(fp);

  return zErr;
} // Buildpinfotable

/**
** Function to build a memory table of all the records belonging to an account.
** The function assumes that the file is sorted (by branch_account, sec_no, wi,
** trans_no, etc) and it has records in DILIBSTRUCT format, each field seperated
** by '|' character. Write now, this function works only for a single account,
** it later may be extended to accept a list of accounts.
**/
ERRSTRUCT BuildDITable(const char *sMode, const char *sType,
                       const char *sFileName, int iID, const char *sSecNo,
                       const char *sWi, const char *sSecXtend,
                       const char *sAcctType, long lTransNo, long lStartDate,
                       long lValDate, long lStartPosition,
                       DILIBTABLE *pzDITable) {
  ERRSTRUCT zErr;
  DILIBSTRUCT zTempDILib;
  FILE *fp;
  char sStr1[501], *sStr2;
  long lEndDate;
  int iEndId = 0;

  lpprInitializeErrStruct(&zErr);
  InitializeDILibTable(pzDITable);

  if (strcmp(sMode, "B") == 0) {
    fp = fopen(sFileName, "r");
    if (fp == NULL)
      return (lpfnPrintError("Error Opening File", iID, 0, "", 999, 0, 0,
                             "DIVINT BUILDTABLE1", FALSE));

    if (fseek(fp, lStartPosition, SEEK_SET) < 0)
      zErr = lpfnPrintError("Error Seeking File", iID, 0, "", 999, 0, 0,
                            "DIVINT BUILDTABLE2", FALSE);
    else {
      /*
      ** Read records from the file. For the record size, we need to give the
      * size
      ** of the biggest record, because fgets stops as soon as it gets a new
      * line
      ** character(or has read the given number of character, whichever is
      * first),
      ** so use 500 as the size, even though the length of records will be much
      ** lower than that.
      */
      while (fgets(sStr1, 500, fp) != NULL) {
        InitializeDILibStruct(&zTempDILib);

        sStr2 = strtok(sStr1, "|");

        if (sStr2[0] != 'H' && sStr2[0] != 'D') {
          zErr = lpfnPrintError(
              "Record Can Only Come From H(oldings) or (hold)D(el)", iID, 0, "",
              995, 0, 0, "DIVINT BUILDTABLE4", FALSE);
          break;
        }
        strcpy_s(zTempDILib.sTableName, sStr2);

        /*
        ** If fetched account is not same as the passed account, there are two
        ** possibility, first is that the last account processed is not null,
        ** which means that we are done(since file is sorted by account), and
        ** second is that last account processed is NULL which means we have not
        ** found records of the the passed account yet, in that case read next
        ** record. If we have found a record of the passed account, then use
        ** strtok to get all the fields out of it into a DILIBSTRUCt variable
        * and
        ** add that to the table.
        */
        sStr2 = strtok(NULL, "|");
        if (atoi(sStr2) > iID)
          break;
        else if (atoi(sStr2) < iID)
          continue;

        zTempDILib.iID = atoi(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sSecNo, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sWi, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sSecXtend, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sSecSymbol, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sAcctType, sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lTransNo = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fUnits = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lEffDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lEligDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lStlDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fOrigFace = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fOrigYield = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lEffMatDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fEffMatPrice = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sOrigTransType, sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lCreateDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sSafekInd, sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.iSecID = atoi(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fTrdUnit = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.iSecType = atoi(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sAutoAction, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sAutoDivint, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sCurrId, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sIncCurrId, sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fCurExrate = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fCurIncExrate = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lDivintNo = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sDivType, sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lExDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lRecDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lPayDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.fDivRate = atof(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sPostStatus, sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lDivCreateDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lModifyDate = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lFwdDivintNo = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        zTempDILib.lPrevDivintNo = atol(sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sDeleteFlag, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sProcessFlag, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sProcessType, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sShortSettle, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sInclUnits, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sSplitInd, sStr2);

        sStr2 = strtok(NULL, "|");
        strcpy_s(zTempDILib.sDTTranType, sStr2);
        /*
        ** Rest of the fields come from divhist table which is in an OUTER join
        ** with other tables(in the SQL which creates this file), so in many(in
        ** fact most) cases, we will have no more fields. If strtok returns
        * NULL,
        ** we have no fields coming from divhist, else we do.
        */
        sStr2 = strtok(NULL, "|");
        if (sStr2 == NULL)
          zTempDILib.bNullDivhist = TRUE;
        else {
          zTempDILib.bNullDivhist = FALSE;
          strcpy_s(zTempDILib.sTranType, sStr2);

          sStr2 = strtok(NULL, "|");
          zTempDILib.lDivTransNo = atol(sStr2);

          sStr2 = strtok(NULL, "|");
          strcpy_s(zTempDILib.sTranLocation, sStr2);
        }

        /*
         ** Add the record to the given table
         ** If the option is 'B' - no checking
         ** If the option is 'I' - allow only capital gain and income
         ** If the option is 'S' - allow only stock dividends and splits
         */
        if (strcmp(sType, "B") == 0 ||
            (strcmp(sType, "I") == 0 &&
             (strcmp(zTempDILib.sProcessType, "I") == 0 ||
              strcmp(zTempDILib.sProcessType, "C") == 0 ||
              strcmp(zTempDILib.sProcessType, "L") == 0)) ||
            (strcmp(sType, "S") == 0 &&
             (strcmp(zTempDILib.sProcessType, "S") == 0 ||
              strcmp(zTempDILib.sProcessType, "D") == 0))) {
          zErr = AddRecordToDILibTable(pzDITable, zTempDILib);
          if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
            break;
        }
      } /* while not(eof) */
    }
    fclose(fp);
  } /* if batch mode */
  else {
    lEndDate = GetPaymentsEndingDate(lValDate);
    while (zErr.iSqlError == 0) {
      lpprDivintUnload(&zTempDILib, lStartDate, lEndDate, sMode, sType, iID,
                       iEndId, sSecNo, sWi, sSecXtend, sAcctType, lTransNo,
                       &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0)
        return zErr;

      /*
       ** Add the record to the given table
       ** If the option is 'B' - no checking
       ** If the option is 'I' - allow only capital gain and income
       ** If the option is 'S' - allow only stock dividends and splits
       */
      if (strcmp(sType, "B") == 0 ||
          (strcmp(sType, "I") == 0 &&
           (strcmp(zTempDILib.sProcessType, "I") == 0 ||
            strcmp(zTempDILib.sProcessType, "C") == 0 ||
            strcmp(zTempDILib.sProcessType, "L") == 0)) ||
          (strcmp(sType, "S") == 0 &&
           (strcmp(zTempDILib.sProcessType, "S") == 0 ||
            strcmp(zTempDILib.sProcessType, "D") == 0))) {
        zErr = AddRecordToDILibTable(pzDITable, zTempDILib);
        if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
          return zErr;
      }
    } // while no error
  } // Not batch mode

  return zErr;
} // BuildDITable

/**
** Function to add a record(of DILIBSTRUCT type) to DILIBTABLE.
**/
ERRSTRUCT AddRecordToDILibTable(DILIBTABLE *pzDITab, DILIBSTRUCT zDILStruct) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzDITab->iDIRecCreated == pzDITab->iNumDIRec) {
    pzDITab->iDIRecCreated += NUMDIRECORD;
    pzDITab->pzDIRec = (DILIBSTRUCT *)realloc(
        pzDITab->pzDIRec, pzDITab->iDIRecCreated * sizeof(DILIBSTRUCT));
    if (pzDITab->pzDIRec == NULL)
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                             "DIVINT ADDDIL1", FALSE));
  }

  pzDITab->pzDIRec[pzDITab->iNumDIRec++] = zDILStruct;

  return zErr;
} /* AddRecordToDILibTable */

/**
** Function that does main processing of paying dividend/interest for all the
** accounts in the supplied porttable.
**/
ERRSTRUCT DivintGeneration(PORTTABLE zPmainTable, PINFOTABLE zPInfoTable,
                           char *sSecNo, char *sWi, char *sSecXtend,
                           char *sAcctType, long lTransNo, long lStartDate,
                           long lValDate, long lForwardDate, char *sMode,
                           char *sType, SUBACCTTABLE zSTable,
                           CURRTABLE zCTable) {
  ERRSTRUCT zErr;
  DILIBTABLE zDTable;
  ACCDIV zAccdiv;
  int i, j, m, s, iLastIndex;
  char sLastSecNo[13], sLastWi[2], sLastSecXtend[3], sLastAcctType[3];
  char sLastTableName[2], sFileName[90], sErrMsg[100];
  long lLastTransNo, lLastDivintNo, lBusinessValDate;
  double fTotalUnits, fTotalNUnits, fTotalIncome, fSumTrUnits, fSumRdUnits,
      fSumIncome;
  BOOL bLotDeleted, bFirstRecord, bUseTrans;

  lpprInitializeErrStruct(&zErr);
  fTotalUnits = fTotalNUnits = fTotalIncome = fSumTrUnits = fSumRdUnits =
      fSumIncome = 0;
  lBusinessValDate = GetPaymentsEndingDate(lValDate);

  // Filename for sorted file on valdate or NULL if not in batch mode
  // the date here has to be the same as the one passed in DivintUnloadAndSort
  if (strcmp(sMode, "B") == 0)
    strcpy_s(sFileName, MakePricingFileName(lForwardDate, sType, "D"));
  else
    strcpy_s(sFileName, "");

  // Do processing for all the branch accounts in the port table
  zDTable.iDIRecCreated = iLastIndex = 0;
  for (i = 0; i < zPmainTable.iNumPmain; i++) {
    m = -1;
    for (j = iLastIndex; j < zPInfoTable.iNumPI; j++) {
      if (zPmainTable.pzPmain[i].iID == zPInfoTable.pzPRec[j].iID) {
        m = j;
        iLastIndex = m + 1;
        break;
      }
    }
    if (m == -1)
      continue;

    // Start a new database transaction
    bUseTrans = (lpfnGetTransCount() == 0);
    if (bUseTrans)
      lpfnStartTransaction();

    sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = sLastAcctType[0] =
        sLastTableName[0] = '\0';
    lLastTransNo = lLastDivintNo = 0;

    __try {
      // Initialize all sum variables
      fTotalUnits = fTotalNUnits = fTotalIncome = fSumTrUnits = fSumRdUnits =
          fSumIncome = 0;

      // Build a table of all the records for current branch account
      zErr = BuildDITable(sMode, sType, sFileName, zPmainTable.pzPmain[i].iID,
                          sSecNo, sWi, sSecXtend, sAcctType, lTransNo,
                          lStartDate, lForwardDate,
                          zPInfoTable.pzPRec[m].lStartPosition, &zDTable);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        if (bUseTrans)
          lpfnRollbackTransaction();

        lpfnPrintError("Dividend/Interest Could Not Be Processed",
                       zPmainTable.pzPmain[i].iID, 0, "", zErr.iBusinessError,
                       zErr.iSqlError, zErr.iIsamCode, "DIVINT PROCESS2", TRUE);

        lpprInitializeErrStruct(&zErr);
        continue;
      }

      // Empty the fwtrans table for the account
      lpprDeleteFWTrans(zPmainTable.pzPmain[i].iID, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        lpfnPrintError("Clear FWTrans Failed", zPmainTable.pzPmain[i].iID, 0,
                       "", 0, zErr.iSqlError, zErr.iIsamCode,
                       "DIVINT PROCESS2.5", FALSE);
        lpprInitializeErrStruct(&zErr);
        InitializeDILibTable(&zDTable);
        continue;
      }

      bFirstRecord = TRUE;

      // Process All the records for current account
      for (j = 0; j < zDTable.iNumDIRec; j++) {
        // Don't pay anything which is less that Pricing Effective Date
        if (zDTable.pzDIRec[j].lPayDate <
            zPmainTable.pzPmain[i].lPricingEffectiveDate)
          continue;

        // Don't pay anything until after the settlement date of the trade,
        // unless it is a stock split/dividend (which is based on trade date and
        // not the settlement date)
        if (zDTable.pzDIRec[j].lPayDate <= zDTable.pzDIRec[j].lStlDate &&
            strcmp(zDTable.pzDIRec[j].sProcessType, "S") != 0 &&
            strcmp(zDTable.pzDIRec[j].sProcessType, "D") != 0)
          continue;

        // Don't pay anything before the effective date of the trade (In case
        // the lot was created by a FR  transaction, it is possible that the
        // previous check(of settlement date) passes but the trade might not be
        // eligible because effective date is greater than pay date
        if (zDTable.pzDIRec[j].lPayDate < zDTable.pzDIRec[j].lEffDate)
          continue;

        s = FindSecType(zSTTable, zDTable.pzDIRec[j].iSecType);
        if (s < 0) // should not happen, all secuirities should have a valid
                   // security type
          continue;

        // SB 7/3/03 - If a stock is sold to settle on or prior to the record
        // date of a dividend, it's not eligible to receive the dividend. Any
        // security sold to settle prior to the record date is already
        // eliminated by the query, so here eliminate the case when record is
        // coming from holddel (sold) and it's a stock and record date of the
        // dividend is same as the settlement date of the sale (stored in
        // create\ date variable)
        if (strcmp(zSTTable.zSType[s].sPrimaryType, "E") == 0 &&
            strcmp(zDTable.pzDIRec[j].sTableName, "D") == 0 &&
            zDTable.pzDIRec[j].lCreateDate == zDTable.pzDIRec[j].lRecDate)
          continue;

        // SB 6/21/2002 - Until now capital gain was always paid even if account
        // is not set to pay income - changed it so for processing purposes
        // capital gain is treated like income if process type is I(income),
        // L(liquidation) or C(capital gain) and account is not set to pay
        // income automatically, ignore the record

        // SB 4/30/03 - Changed it so that divideds are always generated, for
        // dividends this check moved to dipay, at that place if account is not
        // set up to pay, no transaction is generated. Reason for this change is
        // to generate pending accruals for equities. Don't generate RIs if
        // account is not set up to generate income SB 2/10/09 - (VI# 42033) -
        // Now fixed income mutual funds are treated like equity, i.e. RI is
        // generated for fixed income mutual fund even if account is not set to
        // automatically pay income
        if (!zPmainTable.pzPmain[i].bIncome &&
            (strcmp(zDTable.pzDIRec[j].sProcessType, "I") == 0 ||
             strcmp(zDTable.pzDIRec[j].sProcessType, "L") == 0 ||
             strcmp(zDTable.pzDIRec[j].sProcessType, "C") == 0) &&
            (strcmp(zSTTable.zSType[s].sPrimaryType, "B") == 0 &&
             strcmp(zSTTable.zSType[s].sSecondaryType, "U") != 0))
          continue;

        // if process type is S(stock split) or D(stock dividends) and account
        // is not set to generate corporate actions automatically, ignore the
        // record
        if (!zPmainTable.pzPmain[i].bActions &&
            (strcmp(zDTable.pzDIRec[j].sProcessType, "S") == 0 ||
             strcmp(zDTable.pzDIRec[j].sProcessType, "D") == 0))
          continue;

        /*
        ** For each lot, holddel might have zero to n(theoritically n could be
        * any
        ** +ve number) records and holdings will have zero or one record. If
        ** process type is S(stock split) or D(stock dividend) then the record
        ** from Holding(if any) is used else first record (either from holddel
        * or
        ** holdings[if no holddel record for the lot]) is used for processing.
        */
        if (strcmp(sLastSecNo, zDTable.pzDIRec[j].sSecNo) == 0 &&
            strcmp(sLastWi, zDTable.pzDIRec[j].sWi) == 0 &&
            strcmp(sLastSecXtend, zDTable.pzDIRec[j].sSecXtend) == 0 &&
            strcmp(sLastAcctType, zDTable.pzDIRec[j].sAcctType) == 0 &&
            lLastTransNo == zDTable.pzDIRec[j].lTransNo &&
            lLastDivintNo == zDTable.pzDIRec[j].lDivintNo)
          continue;

        // If the divint no changes, verify and adjust totals(for the last
        // divint) and create accdiv/divhist, etc. After that continue
        // processing the current record
        if (lLastDivintNo != zDTable.pzDIRec[j].lDivintNo && !bFirstRecord) {
          zErr = VerifyAndCommit(zDTable, lValDate, lForwardDate,
                                 lBusinessValDate, zSTable, zCTable,
                                 zPmainTable.pzPmain[i].sAcctMethod,
                                 zPmainTable.pzPmain[i].iCurrIndex, fTotalUnits,
                                 fTotalNUnits, fTotalIncome, fSumTrUnits,
                                 fSumRdUnits, fSumIncome, lLastDivintNo);
          if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
            continue;

          lLastDivintNo = zDTable.pzDIRec[j].lDivintNo;
          fTotalUnits = fTotalNUnits = fTotalIncome = fSumTrUnits =
              fSumRdUnits = fSumIncome = 0;
        } // Divintno changed

        /*
        ** If secno or wi is different than last secno or wi(and it is not the
        ** first record) and no error has occured so far, do a COMMITWORK to
        ** make sure the dividends/interest for the current security are
        ** commited in the database then do another BEGIN WORK for next security
        */
        if (!bFirstRecord &&
            (strcmp(sLastSecNo, zDTable.pzDIRec[j].sSecNo) != 0 ||
             strcmp(sLastWi, zDTable.pzDIRec[j].sWi) != 0)) {
          if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
            // Add verify work for last divint
            if (bUseTrans)
              lpfnCommitTransaction();

            sprintf_s(sErrMsg,
                      "Dividend/Interest Commited For Acct - %d, SecNo %s",
                      zPmainTable.pzPmain[m].iID, sLastSecNo);
            // lpfnPrintError(sErrMsg, zPmainTable.pzPmain[i].iID, 0, "", 0, 0,
            // 0, "DIVINT PROCESS3", TRUE);
          } else {
            if (bUseTrans)
              lpfnRollbackTransaction();

            sprintf_s(sErrMsg,
                      "Dividend/Interest Could Not Be Processed For Acct - %d, "
                      "SecNo - %s",
                      zPmainTable.pzPmain[i].iID, sLastSecNo);
            lpfnPrintError(sErrMsg, zPmainTable.pzPmain[i].iID, 0, "",
                           zErr.iBusinessError, zErr.iSqlError, zErr.iIsamCode,
                           "DIVINT PROCESS3", TRUE);
            lpprInitializeErrStruct(&zErr);
          }

          // Do a begin work for the next security
          if (bUseTrans)
            lpfnStartTransaction();
        } // If different security than the last one

        /*
         ** If it is a split dividend payment and current record is coming from
         ** holddel, we are not interested in it.
         */
        if ((strcmp(zDTable.pzDIRec[j].sProcessType, "S") == 0 ||
             strcmp(zDTable.pzDIRec[j].sProcessType, "D") == 0) &&
            strcmp(zDTable.pzDIRec[j].sTableName, "D") == 0)
          continue;

        // This is the last lot processed(either successfully or unsuccessfully)
        strcpy_s(sLastSecNo, zDTable.pzDIRec[j].sSecNo);
        strcpy_s(sLastWi, zDTable.pzDIRec[j].sWi);
        strcpy_s(sLastSecXtend, zDTable.pzDIRec[j].sSecXtend);
        strcpy_s(sLastAcctType, zDTable.pzDIRec[j].sAcctType);
        lLastTransNo = zDTable.pzDIRec[j].lTransNo;
        lLastDivintNo = zDTable.pzDIRec[j].lDivintNo;
        bFirstRecord = FALSE;

        // If the boolean null divhist is false, modify already existing record
        // in accdiv
        if (!zDTable.pzDIRec[j].bNullDivhist) {
          // If modify date is not set, nothing to do
          if (zDTable.pzDIRec[j].lModifyDate == 0 ||
              zDTable.pzDIRec[j].lModifyDate ==
                  zDTable.pzDIRec[j].lDivCreateDate)
            continue;

          // If it is a stock split/dividend, nothing to do
          if (strcmp(zDTable.pzDIRec[j].sProcessType, "S") == 0 ||
              strcmp(zDTable.pzDIRec[j].sProcessType, "D") == 0)
            continue;

          // If it has already paid, generate a warning and continue
          if ((strcmp(zDTable.pzDIRec[j].sTranLocation, "T") == 0 &&
               zDTable.pzDIRec[j].lDivTransNo != 0) ||
              (strcmp(zDTable.pzDIRec[j].sTranLocation, "R") == 0)) {
            lpfnPrintError("Cannot Modify Dividend/Income, It Has Already Paid",
                           zPmainTable.pzPmain[i].iID, 0, "", 999, 0, 0,
                           "DIVINT PROCESS4", TRUE);
            continue;
          } else
            bLotDeleted = FALSE;

          if (strcmp(zDTable.pzDIRec[j].sDeleteFlag, "Y") == 0) {
            bLotDeleted = TRUE;
            lpprDeleteAccdivOneLot(zDTable.pzDIRec[j].iID,
                                   zDTable.pzDIRec[j].lTransNo,
                                   zDTable.pzDIRec[j].lDivintNo, &zErr);
            if (zErr.iSqlError != 0)
              continue;

            lpprDeleteDivhist(zDTable.pzDIRec[j].iID,
                              zDTable.pzDIRec[j].lDivintNo,
                              zDTable.pzDIRec[j].lTransNo, &zErr);
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
              continue;
          } /* If DeleteFlag is TRUE */

          /*
           ** Select the accdiv record and see what has changed. Make the
           ** appropriate changes in the accdiv record and update it(or
           ** delete it, if it is not valid anymore)
           */
          if (!bLotDeleted) {
            lpprSelectOneAccdiv(&zAccdiv, zDTable.pzDIRec[j].iID,
                                zDTable.pzDIRec[j].lDivintNo,
                                zDTable.pzDIRec[j].lTransNo, &zErr);
            if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
              continue;

            // Ex Date Changed
            if (zDTable.pzDIRec[j].lExDate != zAccdiv.lTrdDate) {
              /*
              ** If the new exdate is greater than the original exdate or the
              ** new exdate is less than the eligibility date on the lot,
              ** delete it from accdiv and divhist
              */
              if (zDTable.pzDIRec[j].lExDate > zAccdiv.lTrdDate ||
                  zDTable.pzDIRec[j].lExDate < zAccdiv.lEligDate) {
                bLotDeleted = TRUE;
                lpprDeleteAccdivOneLot(zAccdiv.iID, zAccdiv.lTransNo,
                                       zAccdiv.lDivintNo, &zErr);
                if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                  continue;

                lpprDeleteDivhist(zAccdiv.iID, zAccdiv.lDivintNo,
                                  zAccdiv.lTransNo, &zErr);
                if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                  continue;
              } // Lot Not eligible any more
              else
                bLotDeleted = FALSE;
            } // Exdate has changed
          } // if lot is not deleted yet

          if (!bLotDeleted) {
            // add logic to check for changes in divtype (from 06 to 00, or
            // LT/ST to 00, or vica versa )
            if (strcmp(zDTable.pzDIRec[j].sDivType, zAccdiv.sDivType) != 0) {
              // if new divtype is LT or ST - then change trantype on pending
              // accdiv properly (equals to divtype) Use Divtypes trantype to
              // determine LT or ST type - JTG  07/18/2006
              if (strcmp(zDTable.pzDIRec[j].sDTTranType, "LT") == 0 ||
                  strcmp(zDTable.pzDIRec[j].sDTTranType, "ST") == 0)
                strcpy_s(zAccdiv.sTranType, zDTable.pzDIRec[j].sDTTranType);
              // but if divtype on pending accdiv changed from LT/ST to any
              // other - make trantype "AD" back
              else if (strcmp(zAccdiv.sTranType, "LT") == 0 ||
                       strcmp(zAccdiv.sTranType, "ST") == 0)
                strcpy_s(zAccdiv.sTranType, "AD");

              // now change DivType on pending accdiv also
              strcpy_s(zAccdiv.sDivType, zDTable.pzDIRec[j].sDivType);
            }

            if (zDTable.pzDIRec[j].lPayDate != zAccdiv.lStlDate) {
              zAccdiv.lStlDate = zDTable.pzDIRec[j].lPayDate;
              zAccdiv.lEffDate = zDTable.pzDIRec[j].lPayDate;
            }

            if ((zDTable.pzDIRec[j].lExDate != zAccdiv.lTrdDate) ||
                (zDTable.pzDIRec[j].fDivRate != zAccdiv.fDivFactor)) {
              strcpy_s(zDTable.pzDIRec[j].sAddUpdFlag, "U");
              zDTable.pzDIRec[j].bProcessLot = TRUE;
              zDTable.pzDIRec[j].fUnits = zAccdiv.fUnits;

              zErr =
                  CalculateTotals(&zDTable.pzDIRec[j], &fTotalUnits,
                                  &fTotalNUnits, &fTotalIncome, &fSumTrUnits,
                                  &fSumRdUnits, &fSumIncome, lBusinessValDate);
              if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
                continue;
            } // modified exdate or dividend rate

            // Update the accdiv and divhist for changes to paydate or dividend
            // type
            if (!zDTable.pzDIRec[j].bProcessLot) {
              lpprUpdateAccdiv(&zAccdiv, &zErr);
              if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                continue;

              lpprUpdateDivhist(zAccdiv.iID, zAccdiv.lDivintNo,
                                zAccdiv.lTransNo, zAccdiv.fUnits,
                                zAccdiv.lTrdDate, zAccdiv.lStlDate, &zErr);
              if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
                continue;
            } // process flag not = 'y'
          } // If lot still exist
        } // Record already in divhist table
        else {
          // If new dividend and delete flag set - do not accrue
          if (strcmp(zDTable.pzDIRec[j].sDeleteFlag, "Y") == 0)
            continue;

          // If the lot is eligible for payment on exdate OR it is an income
          // payment(not splits) and the lot was not created by FR or
          // TS(transfer of security) and the trade has settled on record date
          // then it should get this payment
          if (zDTable.pzDIRec[j].lEligDate < zDTable.pzDIRec[j].lExDate ||
              (zDTable.pzDIRec[j].lStlDate <= zDTable.pzDIRec[j].lRecDate &&
               strcmp(zDTable.pzDIRec[j].sOrigTransType, "FR") != 0 &&
               strcmp(zDTable.pzDIRec[j].sOrigTransType, "TS") != 0 &&
               (strcmp(zDTable.pzDIRec[j].sProcessType, "C") == 0 ||
                strcmp(zDTable.pzDIRec[j].sProcessType, "I") == 0 ||
                strcmp(zDTable.pzDIRec[j].sProcessType, "L") == 0))) {
            zDTable.pzDIRec[j].bProcessLot = TRUE;
            strcpy_s(zDTable.pzDIRec[j].sAddUpdFlag, "A");
            zErr = CalculateTotals(&zDTable.pzDIRec[j], &fTotalUnits,
                                   &fTotalNUnits, &fTotalIncome, &fSumTrUnits,
                                   &fSumRdUnits, &fSumIncome, lBusinessValDate);
            if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
              continue;
          } /* If record eligible for payment */
        } /* no record in history table, dividend-income has not yet accrued */
      } /* for j < iNumDIRec */
    } // try
    __except (lpfnAbortTransaction(bUseTrans)) {
    }

    /* For the last lot of each branch account */
    if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
      if (lLastDivintNo != 0) {
        zErr = VerifyAndCommit(
            zDTable, lValDate, lForwardDate, lBusinessValDate, zSTable, zCTable,
            zPmainTable.pzPmain[i].sAcctMethod,
            zPmainTable.pzPmain[i].iCurrIndex, fTotalUnits, fTotalNUnits,
            fTotalIncome, fSumTrUnits, fSumRdUnits, fSumIncome, lLastDivintNo);

        fTotalUnits = fTotalNUnits = fTotalIncome = fSumTrUnits = fSumRdUnits =
            fSumIncome = 0;
      }

      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
        if (bUseTrans)
          lpfnRollbackTransaction();
      } else {
        if (bUseTrans)
          lpfnCommitTransaction();
      }
    } // if no error for the last lot
    else {
      if (bUseTrans)
        lpfnRollbackTransaction();

      zErr.iSqlError = zErr.iBusinessError = 0;
    }
  } /* i < numpdir */

  InitializeDILibTable(&zDTable);

  return zErr;
} /* DivintGeneration */

/**
** Function to calculate new money or units and to tally the total units
**/
ERRSTRUCT CalculateTotals(DILIBSTRUCT *pzDILS, double *pfTotalUnits,
                          double *pzNewTotUnits, double *pfTotalIncome,
                          double *pfSumTrUnits, double *pfSumRdUnits,
                          double *pfSumIncome, long lBusinessValDate) {
  char sSplitInd[2], sMsg[80];
  double fSplitRate, fProduct, fQuotient, fNewUnits, fTruncUnits, fRndUnits,
      fDivisor, fInfRatio;
  ERRSTRUCT zErr;
  PARTFINC zFinc;
  int i;
  int iCount;
  long lLastIncDate;
  double fInflRate, fAdjustedDivRate;
  BOOL bFirstIncome, bVariableRate;

  fProduct = fQuotient = fNewUnits = 0;

  lpprInitializeErrStruct(&zErr);

  // For stock splits and stock dividends, calculate the amount of new shares to
  // be credited/debited
  if (strcmp(pzDILS->sProcessType, "S") == 0 ||
      strcmp(pzDILS->sProcessType, "D") == 0) {
    fSplitRate = pzDILS->fDivRate;
    strcpy_s(sSplitInd, pzDILS->sSplitInd);

    zErr = CalcSplitFactor(fSplitRate, &fQuotient, &fProduct);
    if (fQuotient == 0 || fProduct == 0) {
      pzDILS->bProcessLot = FALSE;
      sprintf_s(sMsg, "Invalid P/Q - %F/%F", fProduct, fQuotient);
      return (lpfnPrintError(sMsg, pzDILS->iID, pzDILS->lDivintNo, "I", 999, 0,
                             0, "DIVINT CALCTOTALS1", FALSE));
    }

    *pfTotalUnits += pzDILS->fUnits;

    // calculate new units for this lot
    CalcNewUnits(sSplitInd, fQuotient, fProduct, pzDILS->fUnits, &fTruncUnits,
                 &fRndUnits);

    // want to carry units upto 5 decimal places
    pzDILS->fTruncUnits = TruncateDouble(fTruncUnits, 5);
    pzDILS->fRndUnits = RoundDouble(fRndUnits, 5);

    *pfSumTrUnits += fTruncUnits;
    *pfSumRdUnits += fRndUnits;

    // calculate new units overall
    CalcNewUnits(sSplitInd, fQuotient, fProduct, *pfTotalUnits, &fTruncUnits,
                 &fRndUnits);

    *pzNewTotUnits = fRndUnits;

    // SB 5/23/01 - If it is a reverse split (processtype - "D"), reduce the
    // Rounded Units (more than truncated) else increase the Truncated units
    // (less).
    //		if (strcmp(pzDILS->sSplitInd, "D") == 0)
    //		*pzNewTotUnits = fRndUnits;
    // else
    //*pzNewTotUnits = fTruncUnits;

    // SB 10/30/2000 - if new units are zero, send a warning, untill now it was
    // an error
    if (pzDILS->fTruncUnits == 0 || pzDILS->fRndUnits == 0) {
      pzDILS->bProcessLot = FALSE;
      sprintf_s(sMsg, "Invalid Units For Taxlot %d", pzDILS->lTransNo);
      lpfnPrintError(sMsg, pzDILS->iID, pzDILS->lDivintNo, "I", 999, 0, 0,
                     "DIVINT CALCTOTALS2", TRUE);
    }

    // SB 5/24/01 - According to Leno, can not reduce the units to less than 1,
    // if the units become less than one because of the reverse split, don't
    // post RS trade for this taxlot JTG 5/17/07 Fixed the units check - it was
    // using origtaxlotunits - newcombinedunits
    //                                  now it is using origcombinedunits -
    //                                  newcombinedunits
    if (strcmp(pzDILS->sSplitInd, "D") == 0 &&
        *pfTotalUnits - *pzNewTotUnits < 1) {
      pzDILS->bProcessLot = FALSE;
      *pfTotalUnits -= pzDILS->fUnits;
      *pzNewTotUnits -= fRndUnits;
      *pfSumTrUnits -= fTruncUnits;
      *pfSumRdUnits -= fRndUnits;
      lpfnPrintError(
          "Cannot Process Reverse Split which causes taxlot to be less than 1",
          pzDILS->iID, pzDILS->lDivintNo, "I", 999, 0, 0, "DIVINT CALCTOTALS3",
          TRUE);
    }
  } // Calculate new split units
  // Set the income amount of transaction, if not stock split or stock dividend
  else {
    /*     if (pzDILS->sPostStatus[0] != 'Y' && pzDILS->sInclUnits[0] == 'Y')
           return(lpfnPrintError("Incorrect Rate for Div On Inc Shares",
       pzDILS->iID, pzDILS->lDivintNo, "I", 999, 0, 0, "DIVINT CALCTOTALS3",
       FALSE));*/

    // SB 3/25/1999 to match these numbers to STARS(to take care of how
    // div_factor is stored) divide the income number by 10 in case of a bond
    i = FindSecType(zSTTable, pzDILS->iSecType);
    if (i < 0) {
      sprintf_s(sMsg, "Invalid Sectype %d for security %s", pzDILS->iSecType,
                pzDILS->sSecNo);
      return (lpfnPrintError(sMsg, pzDILS->iID, pzDILS->lDivintNo, "I", 999, 0,
                             0, "DIVINT CALCTOTALS4", FALSE));
    }

    if (strcmp(zSTTable.zSType[i].sPrimaryType, "B") == 0)
      fDivisor = 10.0;
    else
      fDivisor = 1.0;

    // SB 1/20/04 - For TIPS, multiply the income amount by inflation index
    // ratio to get nomnal cash
    if (zSTTable.zSType[i].sPrimaryType[0] == PTYPE_BOND &&
        zSTTable.zSType[i].sSecondaryType[0] == STYPE_TBILLS &&
        zSTTable.zSType[i].sPayType[0] == PAYTYPE_INFPROTCTD) {
      // Get the issue date of the security
      lpprSelectFixedInc(pzDILS->sSecNo, pzDILS->sWi, &zFinc, &zErr);
      if (zErr.iSqlError)
        return (lpfnPrintError("Error Fetching Issue Date", pzDILS->iID,
                               pzDILS->lDivintNo, "I", 999, zErr.iSqlError,
                               zErr.iIsamCode, "DIVINT CALCTOTALS5", FALSE));

      // Get the inflation index ratio on the payment date of the bond
      fInfRatio =
          lpfnInflationIndexRatio(pzDILS->sSecNo, zFinc.lIssueDate,
                                  pzDILS->lPayDate, zFinc.lInflationIndexID);
      if (fInfRatio <= 0) {
        // It's possible not to have inflation index for a future date payment
        // (which is going to go in FWTrans), for those default the ratio to
        // be 1.0 but if this record is for a current/past date then it'll
        // become a real transaction and for that it should have a valid ratio,
        // if it doesn't then return with an error.
        if (pzDILS->lPayDate <= lBusinessValDate)
          return (lpfnPrintError("Invalid Inflation Ratio", pzDILS->iID,
                                 pzDILS->lDivintNo, "I", 999, 0, 0,
                                 "DIVINT CALCTOTALS6", FALSE));
        else
          fInfRatio = 1.0;
      } // if not a valid inflation ratio
    } // if TIPS
    else
      fInfRatio = 1.0;

    // vay 10/31/05 - For MIPS, calculate Phantom Income as we would for TIPS,
    // but then apply it to  payment amount as actual cash
    fAdjustedDivRate = pzDILS->fDivRate;
    bVariableRate = FALSE;

    if (zSTTable.zSType[i].sPrimaryType[0] == PTYPE_BOND &&
        zSTTable.zSType[i].sSecondaryType[0] == STYPE_MUNICIPALS &&
        zSTTable.zSType[i].sPayType[0] == PAYTYPE_INFPROTCTD) {
      lLastIncDate = 0;
      lpprGetLastIncomeDateMIPS(pzDILS->iID, pzDILS->lTransNo, pzDILS->lPayDate,
                                &lLastIncDate, &iCount, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      if (iCount == 1)
        bFirstIncome = TRUE;
      else
        bFirstIncome = FALSE;

      // calculate the inflation rate (which will be added to coupon rate)
      // between last income date and this income date
      zErr = lpfnCalculateInflationRate(
          pzDILS->iID, pzDILS->sSecNo, pzDILS->sWi, pzDILS->lTransNo,
          lLastIncDate, pzDILS->lPayDate, pzDILS->lEffDate, FALSE, bFirstIncome,
          &fInflRate);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        // It's possible not to have inflation index for a future date payment
        // (which is going to go in FWTrans), for those default the rate to 0,
        // but if it is for current/past date then it'll become a real
        // transaction and for that it should have a valid rate, if it doesn't
        // then return with an error.
        if (pzDILS->lPayDate <= lBusinessValDate) {
          pzDILS->bProcessLot = FALSE;
          return (lpfnPrintError("Invalid Inflation Rate", pzDILS->iID,
                                 pzDILS->lDivintNo, "I", 999, 0, 0,
                                 "DIVINT CALCTOTALS7a", FALSE));
        } else {
          fInflRate = 0;
          // reset error codes
          lpprInitializeErrStruct(&zErr);
        }
      }

      // rate calculated above is in %% already, need to convert to our storage,
      // i.e. if inflation runs at 2%, we would get 0.02, divrate of 1.5% is
      // stored as 15, i.e. we need 0.02 to become equal to 20, so adjusted
      // coupon is 35 or 3.5%
      fInflRate = fInflRate * fDivisor / pzDILS->fTrdUnit;
      // add inflation rate to regular coupon to get "adjusted" rate
      fAdjustedDivRate = pzDILS->fDivRate + fInflRate;
      // set flag which will be used how to determine Total Income
      // i.e. if it can be calculated as (total units * constant rate)
      // or rather sum(current units * variable rate)
      bVariableRate = TRUE;

      // if after applying inflation rate to coupon rate we result with no
      // payment at all (i.e. deflationary period) then do not process this lot
      if ((fAdjustedDivRate < 0) || IsValueZero(fAdjustedDivRate, 7)) {
        pzDILS->bProcessLot = FALSE;
        sprintf_s(sMsg, "Invalid Adjusted Rate For Taxlot %d",
                  pzDILS->lTransNo);
        return lpfnPrintError(sMsg, pzDILS->iID, pzDILS->lDivintNo, "I", 999, 0,
                              0, "DIVINT CALCTOTALS7b", TRUE);
      }
    } // if MIPS

    // Set the units
    *pfTotalUnits += pzDILS->fUnits;
    *pfSumTrUnits += pzDILS->fUnits;
    *pfSumRdUnits += pzDILS->fUnits;
    *pzNewTotUnits += pzDILS->fUnits;
    pzDILS->fTruncUnits = TruncateDouble(pzDILS->fUnits, 5);
    pzDILS->fRndUnits = RoundDouble(pzDILS->fUnits, 5);

    // SB 1/20/04 - multiply by inflation index ratio, for TIPS its real ratio
    // for other securities it's 1
    pzDILS->fIncomeAmt =
        RoundDouble(pzDILS->fUnits * fAdjustedDivRate * pzDILS->fTrdUnit *
                        fInfRatio / fDivisor,
                    2);
    if (bVariableRate)
      *pfTotalIncome += RoundDouble(pzDILS->fIncomeAmt, 2);
    else
      *pfTotalIncome = RoundDouble(*pfTotalUnits * fAdjustedDivRate *
                                       pzDILS->fTrdUnit * fInfRatio / fDivisor,
                                   2);

    *pfSumIncome += RoundDouble(pzDILS->fIncomeAmt, 2);
  } /* Not a Stock Split or Stock Dividend */

  return zErr;
} // CalculateTotals

/**
** This function adjusts the lots for appropriate units and then it
* inserts/updates
** accdiv & divhist records.
**/
ERRSTRUCT VerifyAndCommit(DILIBTABLE zDTable, long lValDate, long lForwardDate,
                          long lBusinessValDate, SUBACCTTABLE zSTable,
                          CURRTABLE zCTable, char *sAcctMethod,
                          int iPortCurrIndx, double fTotalUnits,
                          double fTotalNUnits, double fTotalPcpl,
                          double fSumTrUnits, double fSumRdUnits,
                          double fSumPcpl, long lDivintNo) {
  double fMnyDiff, fUnitTrDiff, fUnitRdDiff, fUnitDiff;
  int i, k;
  char sTruncRnd[2];
  BOOL bNegativeUnits, bNegativeMny, bOkFlag;
  ACCDIV zAccdiv;
  DIVHIST zDivhist;
  FWTRANS zFWTrans;
  ERRSTRUCT zErr;

  i = k = 0;
  bNegativeUnits = bNegativeMny = FALSE;

  lpprInitializeErrStruct(&zErr);

  // Check if the total units or total money are negative.  If so, multiply by
  // minus one for comparison work
  if (fTotalPcpl < 0) {
    bNegativeMny = TRUE;
    fTotalPcpl *= -1;
    fSumPcpl *= -1;
  }

  if (fTotalUnits < 0) {
    bNegativeUnits = TRUE;
    fTotalUnits *= -1;
    fSumTrUnits *= -1;
    fSumRdUnits *= -1;
    fTotalNUnits *= -1;
  }

  // Calculate the money diff
  fMnyDiff = TruncateDouble(fSumPcpl - fTotalPcpl, 2);

  // Calculate the share difference
  fUnitTrDiff = TruncateDouble(fSumTrUnits - fTotalNUnits, 6);
  fUnitRdDiff = TruncateDouble(fSumRdUnits - fTotalNUnits, 6);

  if (IsValueZero(fUnitTrDiff, 5)) {
    strcpy_s(sTruncRnd, "T");
    fUnitDiff = fUnitTrDiff;
  } else if (IsValueZero(fUnitRdDiff, 5)) {
    strcpy_s(sTruncRnd, "R");
    fUnitDiff = fUnitRdDiff;
  } else if (fUnitTrDiff <= fUnitRdDiff) {
    strcpy_s(sTruncRnd, "T");
    fUnitDiff = fUnitTrDiff;
  } else {
    strcpy_s(sTruncRnd, "R");
    fUnitDiff = fUnitRdDiff;
  }

  /* Check to make sure that at least one lot is eligible for processing */
  bOkFlag = FALSE;
  for (i = 0; i < zDTable.iNumDIRec; i++) {
    if (zDTable.pzDIRec[i].bProcessLot)
      bOkFlag = TRUE;
  }

  /*
   ** If there are no lots to process, force the unit diff and
   ** money diff to zero, otherwise the while loop will loop forever
   */
  if (!bOkFlag) {
    fUnitDiff = 0;
    fMnyDiff = 0;
  }

  /*
   ** Set the truncate/round flag on the lot to indicate which units to place on
   * the accdiv.
   ** Also if necessary adjust the units up/down to match to the total amount
   */
  while (fUnitDiff != 0.0 || fMnyDiff != 0.0) {
    for (i = 0; i < zDTable.iNumDIRec; i++) {
      /* If lot is not eligible - skip */
      if (!zDTable.pzDIRec[i].bProcessLot)
        continue;

      if (zDTable.pzDIRec[i].lDivintNo != lDivintNo)
        continue;

      /* If the difference is zero - skip to next lot */
      if (fUnitDiff == 0 && fMnyDiff == 0)
        break;

      // SB 5/24/01 For a reverse split don't apply the logic of
      // roundin/truncating
      if (strcmp(zDTable.pzDIRec[i].sSplitInd, "D") == 0)
        fUnitDiff = 0;
      /*
       ** If the setting is truncate, adjust the truncate units
       ** Else adjust the rounded units
       */
      if (sTruncRnd[0] == 'T' && fUnitDiff != 0) {
        if ((fUnitDiff < 0 && bNegativeUnits) ||
            (fUnitDiff > 0 && !bNegativeUnits))
          zDTable.pzDIRec[i].fTruncUnits -= 0.00001;
        else
          zDTable.pzDIRec[i].fTruncUnits += 0.00001;

        /* Adjust the total unit diff by 1 */
        if (fUnitDiff < 0)
          fUnitDiff += 0.00001;
        else
          fUnitDiff -= 0.00001;
        if (fabs(fUnitDiff) < 0.00001)
          fUnitDiff = 0.0;
      }

      if (sTruncRnd[0] == 'R' && fUnitDiff != 0) {
        if ((fUnitDiff < 0 && bNegativeUnits) ||
            (fUnitDiff > 0 && !bNegativeUnits))
          zDTable.pzDIRec[i].fRndUnits -= 0.00001;
        else
          zDTable.pzDIRec[i].fRndUnits += 0.00001;

        /* Adjust the total unit diff by 1 */
        if (fUnitDiff < 0)
          fUnitDiff += 0.00001;
        else
          fUnitDiff -= 0.00001;
        if (fabs(fUnitDiff) < 0.00001)
          fUnitDiff = 0.0;
      } /* end of units check */

      /* Adjust the principal amount, if the difference is not zero  */
      if (fMnyDiff != 0.0) {
        if ((fMnyDiff < 0 && bNegativeMny) || (fMnyDiff > 0 && !bNegativeMny))
          zDTable.pzDIRec[i].fIncomeAmt -= 0.01;
        else
          zDTable.pzDIRec[i].fIncomeAmt += 0.01;

        /* Reduce the money diff by one cent */
        if (fMnyDiff < 0.0)
          fMnyDiff += 0.01;
        else
          fMnyDiff -= 0.01;
        //         if (fabs(fMnyDiff) < 0.01)
        if (RoundDouble(fabs(fMnyDiff), 2) < 0.01)
          fMnyDiff = 0.0;
      } /* end of mny check */
    } /* end of 1 loop */
  } /* end of while loop */

  /*
   ** Add or Update Accdiv and Divhist Rows.  The process lot variable
   ** will be set to false to insure that it is not processed again
   */
  for (i = 0; i < zDTable.iNumDIRec; i++) {
    if (!zDTable.pzDIRec[i].bProcessLot)
      continue;

    if (zDTable.pzDIRec[i].lDivintNo != lDivintNo)
      continue;

    // Set the truncate/round flag
    zDTable.pzDIRec[i].sTruncRnd[0] = sTruncRnd[0];

    if (zDTable.pzDIRec[i].sAddUpdFlag[0] == 'A') {
      zDTable.pzDIRec[i].bProcessLot = FALSE;

      // in the function, lValDate is only used for EntryDate, so use current
      // date
      zErr = CreateAccdivRecord(zDTable.pzDIRec[i], lValDate, zSTable,
                                zCTable.zCurrency[iPortCurrIndx].fCurrExrate,
                                sAcctMethod, &zAccdiv);

      // if ProcessType not in (S, D) and IncomeAmt = 0 - ignore the record,
      // which is the same statement as (if CashImpact<>0 but both Pcpl and
      // Income Amounts = 0) 3/22/04 - allow AD with 0 Income to be stored in
      // Accdiv/Divhist - vay
      /*
                      if (strcmp(zDTable.pzDIRec[i].sProcessType, "S") != 0 &&
                              strcmp(zDTable.pzDIRec[i].sProcessType, "D") != 0
         && IsValueZero(zDTable.pzDIRec[i].fIncomeAmt, 2)) continue;
      */

      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        break;

      if (zDTable.pzDIRec[i].lExDate > lBusinessValDate) {
        CreateFWTransRecord(zAccdiv, zDTable.pzDIRec[i].iSecType, &zFWTrans);

        lpprInsertFWTrans(&zFWTrans, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          break;
      } else {
        lpprInsertAccdiv(&zAccdiv, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          break;

        // Create Divhist row
        CreateDivhistRecord(zDTable.pzDIRec[i], zAccdiv.sTranType, &zDivhist);

        // Insert divhist row
        lpprInsertDivhist(&zDivhist, &zErr);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          break;
      }
    } /* end of add mode */
    else {
      // make sure, do not process forward record
      if (zDTable.pzDIRec[i].lExDate > lBusinessValDate)
        break;

      zDTable.pzDIRec[i].bProcessLot = FALSE;

      zErr = CreateAccdivRecord(zDTable.pzDIRec[i], lValDate, zSTable,
                                zCTable.zCurrency[iPortCurrIndx].fCurrExrate,
                                sAcctMethod, &zAccdiv);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        break;

      lpprUpdateAccdiv(&zAccdiv, &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        break;

      lpprUpdateDivhist(zAccdiv.iID, zAccdiv.lDivintNo, zAccdiv.lTransNo,
                        zAccdiv.fUnits, zAccdiv.lTrdDate, zAccdiv.lStlDate,
                        &zErr);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        break;
    } /* end of update mode */
  } /* end of 2 loop */

  return zErr;
}

/**
** Function to create a divhist record from a DILIBSTRUCT record
**/
void CreateDivhistRecord(DILIBSTRUCT zDILS, char *sTranType,
                         DIVHIST *pzDivhist) {
  InitializeDivhist(pzDivhist);

  pzDivhist->iID = zDILS.iID;
  strcpy_s(pzDivhist->sSecNo, zDILS.sSecNo);
  strcpy_s(pzDivhist->sWi, zDILS.sWi);
  pzDivhist->iSecID = zDILS.iSecID;
  strcpy_s(pzDivhist->sSecXtend, zDILS.sSecXtend);
  strcpy_s(pzDivhist->sAcctType, zDILS.sAcctType);
  pzDivhist->lTransNo = zDILS.lTransNo;
  pzDivhist->lDivintNo = zDILS.lDivintNo;
  pzDivhist->fUnits = zDILS.fUnits;
  strcpy_s(pzDivhist->sTranType, sTranType);
  strcpy_s(pzDivhist->sTranLocation, "A");
  pzDivhist->lExDate = zDILS.lExDate;
  pzDivhist->lPayDate = zDILS.lPayDate;

} /* CreateDivhistRecord */

/**
** Function to create an accdiv record from a DILIBSTRUCT record
**/
ERRSTRUCT CreateAccdivRecord(DILIBSTRUCT zDILS, long lValDate,
                             SUBACCTTABLE zSTable, double fBaseCurrExrate,
                             char *sAcctMethod, ACCDIV *pzAccdiv) {
  ERRSTRUCT zErr;
  int i;
  long lDate;
  char sTemp[30];

  lpprInitializeErrStruct(&zErr);
  InitializeAccdiv(pzAccdiv);

  i = 0;
  if (fBaseCurrExrate <= 0.0)
    return (lpfnPrintError("Invalid Exrate", zDILS.iID, zDILS.lDivintNo, "I",
                           67, 0, 0, "DIVINT CREATEADIV1", FALSE));

  pzAccdiv->iID = zDILS.iID;

  strcpy_s(pzAccdiv->sSecNo, zDILS.sSecNo);
  strcpy_s(pzAccdiv->sWi, zDILS.sWi);
  pzAccdiv->iSecID = zDILS.iSecID;
  strcpy_s(pzAccdiv->sSecXtend, zDILS.sSecXtend);
  strcpy_s(pzAccdiv->sAcctType, zDILS.sAcctType);
  pzAccdiv->lTransNo = zDILS.lTransNo;

  pzAccdiv->lDivintNo = zDILS.lDivintNo;
  pzAccdiv->lEligDate = zDILS.lEligDate;

  strcpy_s(pzAccdiv->sSecSymbol, zDILS.sSecSymbol);

  // preset TranType to AD ...
  strcpy_s(pzAccdiv->sTranType, "AD");

  // and change it accordingly if required for specified ProcessType and/or
  // DivType
  if (zDILS.sProcessType[0] == 'S')
    strcpy_s(pzAccdiv->sTranType, "SP");
  else if (zDILS.sProcessType[0] == 'D')
    strcpy_s(pzAccdiv->sTranType, "SD");
  else if (zDILS.sProcessType[0] == 'L')
    strcpy_s(pzAccdiv->sTranType, "LD");
  else if (zDILS.sProcessType[0] == 'C') {
    // added 11/12/01 - vay
    // Hereafter for capital gain/loss new divtypes - LT/ST - will be used
    // The corresponding actual payment transaction type = divtype
    // Use divtypes trantype - JTG 07/18/2006
    if (strcmp(zDILS.sDTTranType, "LT") == 0 ||
        strcmp(zDILS.sDTTranType, "ST") == 0)
      strcpy_s(pzAccdiv->sTranType, zDILS.sDTTranType);
  }

  /*
  ** The include new units flag should only be set for non stock dividend
  ** and splits
  */
  if (zDILS.sInclUnits[0] != 'Y' && zDILS.sSplitInd[0] == 'D')
    strcpy_s(pzAccdiv->sTranType, "RS");

  strcpy_s(pzAccdiv->sDivType, zDILS.sDivType);
  pzAccdiv->fDivFactor = zDILS.fDivRate;

  // If RS then use Rounded units
  if (zDILS.sTruncRnd[0] == 'T' && strcmp(pzAccdiv->sTranType, "RS") != 0)
    pzAccdiv->fUnits = zDILS.fTruncUnits;
  else
    pzAccdiv->fUnits = zDILS.fRndUnits;

  pzAccdiv->fOrigFace = zDILS.fOrigFace;

  pzAccdiv->fIncomeAmt = zDILS.fIncomeAmt;

  pzAccdiv->lTrdDate = zDILS.lExDate;
  pzAccdiv->lStlDate = zDILS.lPayDate;
  pzAccdiv->lEffDate = zDILS.lPayDate;
  pzAccdiv->lEntryDate = lValDate;

  /*
   ** Set the curr id of the trade, if the trade is a split or stock
   ** dividend, the currency id is the primary currency of the security
   ** Otherwise the currency id is the secondary or income currency of
   ** the security.
   ** Also for splits and some stock dividends, the ex date of the dividend
   ** is the settlement date and the paydate the trade date.
   */
  if (zDILS.sProcessType[0] == 'S' || zDILS.sProcessType[0] == 'D') {
    strcpy_s(pzAccdiv->sCurrId, zDILS.sCurrId);

    /* Check stock dividend ex and paydate and set accordingly */
    if (zDILS.sProcessType[0] == 'D' && zDILS.lPayDate > zDILS.lExDate) {
      pzAccdiv->lTrdDate = zDILS.lExDate;
      pzAccdiv->lStlDate = zDILS.lPayDate;
    } else {
      pzAccdiv->lTrdDate = zDILS.lPayDate;
      pzAccdiv->lStlDate = zDILS.lExDate;
    }

    // SB 4/8/2010 (VI #43985) - Effective date should always be settlement date
    // (which typically is pay date) and not exdate
    pzAccdiv->lEffDate = pzAccdiv->lStlDate;
  } else
    strcpy_s(pzAccdiv->sCurrId, zDILS.sIncCurrId);

  i = FindAcctTypeInSubacctTable(zSTable, zDILS.sAcctType);
  if (i < 0)
    return (lpfnPrintError("Invalid AccountType", zDILS.iID, zDILS.lDivintNo,
                           "I", 22, 0, 0, "DIVINT CREATEADIV6", FALSE));

  /*
   ** If type is a stock split, stock dividend or cap gain, the currency and
   ** income currency account type match to the security's account type
   ** Otherwise the match is the income account type of the security
   */
  if (zDILS.sProcessType[0] == 'S' || zDILS.sProcessType[0] == 'D' ||
      zDILS.sProcessType[0] == 'C' || zDILS.sProcessType[0] == 'L') {
    strcpy_s(pzAccdiv->sCurrAcctType, zDILS.sAcctType);
    strcpy_s(pzAccdiv->sIncCurrId, zDILS.sIncCurrId);
    strcpy_s(pzAccdiv->sIncAcctType, zDILS.sAcctType);
  } else {
    strcpy_s(pzAccdiv->sCurrAcctType, zSTable.zSAcct[i].sXrefAcctType);
    strcpy_s(pzAccdiv->sIncCurrId, zDILS.sIncCurrId);
    strcpy_s(pzAccdiv->sIncAcctType, zSTable.zSAcct[i].sXrefAcctType);
  }

  strcpy_s(pzAccdiv->sSecCurrId, zDILS.sCurrId);
  strcpy_s(pzAccdiv->sAccrCurrId, zDILS.sIncCurrId);

  pzAccdiv->fBaseXrate = zDILS.fCurExrate / fBaseCurrExrate;
  pzAccdiv->fIncBaseXrate = zDILS.fCurIncExrate / fBaseCurrExrate;

  pzAccdiv->fSecBaseXrate = pzAccdiv->fBaseXrate;
  pzAccdiv->fAccrBaseXrate = pzAccdiv->fIncBaseXrate;

  pzAccdiv->fSysXrate = zDILS.fCurIncExrate;
  pzAccdiv->fIncSysXrate = zDILS.fCurIncExrate;

  pzAccdiv->fOrigYld = zDILS.fOrigYield;
  pzAccdiv->lEffMatDate = zDILS.lEffMatDate;
  pzAccdiv->fEffMatPrice = zDILS.fEffMatPrice;

  strcpy_s(pzAccdiv->sAcctMthd, sAcctMethod);
  strcpy_s(pzAccdiv->sTransSrce, "P");

  /* Set the debit_credit column and the transaction type accordingly */
  if (pzAccdiv->fUnits >= 0) {
    /*
     ** If the type is a stock split or dividend and not a reverse split
     ** set the dr_cr column to dr
     */
    if ((zDILS.sProcessType[0] == 'S' || zDILS.sProcessType[0] == 'D') &&
        strcmp(pzAccdiv->sTranType, "RS") != 0)
      strcpy_s(pzAccdiv->sDrCr, "DR");
    else
      strcpy_s(pzAccdiv->sDrCr, "CR");
  } else {
    /*
     ** Negative units are not allowed on transactions, the debit/credit
     ** column is used for such an indication
     */
    pzAccdiv->fUnits *= -1.0;
    pzAccdiv->fIncomeAmt *= -1.0;
    strcpy_s(pzAccdiv->sDrCr, "DR");

    /* If the trantype is a reverse split (RS), flip to 'RX'  */
    if (strcmp(pzAccdiv->sTranType, "RS") == 0)
      strcpy_s(pzAccdiv->sTranType, "RX");

    /* If the trantype is a split split (SP), flip to 'SX'  */
    if (strcmp(pzAccdiv->sTranType, "SP") == 0) {
      strcpy_s(pzAccdiv->sTranType, "SX");
      strcpy_s(pzAccdiv->sDrCr, "CR");
    }
    /* If the trantype is a split dividend (SD), flip to 'SB'  */
    else if (strcmp(pzAccdiv->sTranType, "SD") == 0) {
      strcpy_s(pzAccdiv->sTranType, "SB");
      strcpy_s(pzAccdiv->sDrCr, "CR");
    }
  }

  strcpy_s(pzAccdiv->sDtcInclusion, " ");
  strcpy_s(pzAccdiv->sDtcResolve, " ");

  strcpy_s(pzAccdiv->sIncomeFlag, zDILS.sSafekInd);
  strcpy_s(pzAccdiv->sLetterFlag, " ");
  strcpy_s(pzAccdiv->sLedgerFlag, " ");

  strcpy_s(pzAccdiv->sCreatedBy, "PRICING");
  _strdate(sTemp);
  lpfnrstrdate(sTemp, &lDate);
  pzAccdiv->lCreateDate = lDate;

  _strtime(pzAccdiv->sCreateTime);

  strcpy_s(pzAccdiv->sSuspendFlag, "N");
  strcpy_s(pzAccdiv->sDeleteFlag, "N");

  return zErr;
} /* CreateAccdivRecord */

ERRSTRUCT CreateFWTransRecord(ACCDIV zAccdiv, int iSecType,
                              FWTRANS *pzFWTrans) {
  ERRSTRUCT zErr;
  int i;

  lpprInitializeErrStruct(&zErr);
  InitializeFWTrans(pzFWTrans);

  pzFWTrans->iID = zAccdiv.iID;
  pzFWTrans->lDivintNo = zAccdiv.lDivintNo;
  pzFWTrans->lTransNo = zAccdiv.lTransNo;
  strcpy_s(pzFWTrans->sTranType, zAccdiv.sTranType);
  strcpy_s(pzFWTrans->sSecNo, zAccdiv.sSecNo);
  strcpy_s(pzFWTrans->sWi, zAccdiv.sWi);
  pzFWTrans->iSecID = zAccdiv.iSecID;
  strcpy_s(pzFWTrans->sSecXtend, zAccdiv.sSecXtend);
  strcpy_s(pzFWTrans->sAcctType, zAccdiv.sAcctType);
  strcpy_s(pzFWTrans->sDivType, zAccdiv.sDivType);
  pzFWTrans->fDivFactor = zAccdiv.fDivFactor;
  pzFWTrans->fUnits = zAccdiv.fUnits;
  pzFWTrans->fIncomeAmt = zAccdiv.fIncomeAmt;
  pzFWTrans->lTrdDate = zAccdiv.lTrdDate;
  pzFWTrans->lStlDate = zAccdiv.lStlDate;
  pzFWTrans->lEffDate = zAccdiv.lEffDate;
  strcpy_s(pzFWTrans->sCurrId, zAccdiv.sCurrId);
  strcpy_s(pzFWTrans->sIncCurrId, zAccdiv.sIncCurrId);
  strcpy_s(pzFWTrans->sIncAcctType, zAccdiv.sIncAcctType);
  strcpy_s(pzFWTrans->sSecCurrId, zAccdiv.sSecCurrId);
  strcpy_s(pzFWTrans->sAccrCurrId, zAccdiv.sAccrCurrId);
  pzFWTrans->fBaseXrate = zAccdiv.fBaseXrate;
  pzFWTrans->fIncBaseXrate = zAccdiv.fIncBaseXrate;
  pzFWTrans->fSecBaseXrate = zAccdiv.fSecBaseXrate;
  pzFWTrans->fAccrBaseXrate = zAccdiv.fAccrBaseXrate;
  pzFWTrans->fSysXrate = zAccdiv.fSysXrate;
  pzFWTrans->fIncSysXrate = zAccdiv.fIncSysXrate;
  strcpy_s(pzFWTrans->sDrCr, zAccdiv.sDrCr);
  pzFWTrans->lCreateDate = zAccdiv.lCreateDate;
  strcpy_s(pzFWTrans->sCreateTime, zAccdiv.sCreateTime);

  if (strcmp(pzFWTrans->sTranType, "SD") == 0)
    strcpy_s(pzFWTrans->sDescription, "Will Pay Stock Dividend");
  else if (strcmp(pzFWTrans->sTranType, "SP") == 0)
    strcpy_s(pzFWTrans->sDescription, "Will Split");
  else if (strcmp(pzFWTrans->sTranType, "LD") == 0)
    strcpy_s(pzFWTrans->sDescription, "Will Pay Liquidating Dividend");
  else if (strcmp(pzFWTrans->sTranType, "AD") == 0) {
    i = FindSecType(zSTTable, iSecType);
    if (i >= 0) {
      if (strcmp(zSTTable.zSType[i].sPrimaryType, "B") == 0) {
        strcpy_s(pzFWTrans->sDescription, "Will Pay Interest");
        strcpy_s(pzFWTrans->sTranType, "RI");
      } else {
        strcpy_s(pzFWTrans->sDescription, "Will Pay Dividend");
        strcpy_s(pzFWTrans->sTranType, "RD");
      }
    } // sectype found
  } // AD

  return zErr;
} /* CreateAccdivRecord */

/**
** Calculate the new units and the transaction type
**/
void CalcNewUnits(char *sSplitInd, double fQuotient, double fProduct,
                  double fOldUnits, double *pfTruncUnits, double *pfRndUnits) {
  // double  /*fTempDec,*/ fTempDec1; //, 0;
  double fNewUnits;

  fNewUnits = *pfTruncUnits = *pfRndUnits = 0;
  /*
  ** If the split indicator is 'D' (decrease units) , the transaction type
  ** is 'RS' for reverse split.  The calculation for reverse splits is :-
  **      ((units * quotient) / product) - units = new units
  **
  ** The calculation for regular splits and stock dividends is :-
  **      (units * product) / quotient = new units
  */
  if (sSplitInd[0] == 'D') {
    // SB 5/24/01 Old formula was incorrect in case of reverse split, changed it
    // to New Units = Units - (old units * product / quotient)
    //		if (!IsValueZero(fProduct, 6))
    //			fNewUnits = ((fOldUnits * fQuotient) / fProduct) -
    // fOldUnits;
    if (!IsValueZero(fQuotient, 6))
      fNewUnits = fOldUnits - ((fOldUnits * fProduct) / fQuotient);

    *pfTruncUnits = TruncateDouble(fNewUnits, 5);
    *pfRndUnits = RoundDouble(fNewUnits, 5);
    /*
    ** If the resulting units is a fractional unit then the rounded units have
    * to be
    ** rounded up to the next unit (e.g, 2.6 becomes 3 (which round function
    * already does)
    ** and even 2.2 becomes 3 (which round will not do, that's why take the
    * truncated number
    ** and add 1 to it, in case there is any fractional unt)
    ** This is done to take care of situation like this, if a account has 525
    * units and there
    ** is a reverse split of 1 for 200 then account should end up with 2 units.
    * New units will
    ** be calculated to 522.375. truncated and rounded unit will be 522 and
    * after the RS, there
    ** will be 525 - 522 = 3 units left which is incorrect. If rounding up is
    * done then 522.375
    ** will be rounded to 523 and 525 - 523 = 2 will be correct.
    */
    // if (*pfTruncUnits != fNewUnits)
    //*pfRndUnits = *pfTruncUnits + 1;
  } else {
    if (!IsValueZero(fQuotient, 6))
      fNewUnits = fOldUnits * fProduct / fQuotient;

    *pfTruncUnits = TruncateDouble(fNewUnits, 5);
    *pfRndUnits = RoundDouble(fNewUnits, 5);
  } /* SplitIndicator is not 'D' */

} /* end of set units function */

/**
** Function to calculate the numerator and the denomiator of the
** split ratio
**/
ERRSTRUCT CalcSplitFactor(double fSplitRate, double *pfQuotient,
                          double *pfProduct) {
  double fRemainder, fDiff;
  int iQuote;
  BOOL bComplete;
  ERRSTRUCT zErr;

  /* Initialize variables */
  iQuote = 0;
  bComplete = FALSE;

  lpprInitializeErrStruct(&zErr);
  *pfQuotient = *pfProduct = fRemainder = fDiff = 0;

  for (iQuote = 1; iQuote <= 1000; iQuote++) {
    *pfQuotient = iQuote;

    /* Multiply the split rate by the quotient to calculate the product */
    *pfProduct = fSplitRate * *pfQuotient;

    bComplete = FALSE;
    while (!bComplete) {
      /* Divide Product by the Quotient to calculate the remainder */
      fRemainder = *pfProduct / *pfQuotient;

      // Subtract the split rate from the remainder to calculate the differnce
      fDiff = fRemainder - fSplitRate;

      // If the difference is > -0.00005 but less than 0.00005, the difference
      // is zero
      if (fDiff > -0.00005 && fDiff < 0.00005)
        fDiff = 0;

      /*
       ** If the difference is equal to zero, both the P and Quotient
       ** are calculated, no more work
       */
      if (fDiff == 0)
        return zErr;
      else if (fDiff < 0) /* If the difference < 0 */
      {
        *pfProduct += 1;
        continue;
      } else /* if the difference > 0 */
        bComplete = TRUE;
    }
  }

  return zErr;
} /* CalcSplitFactor */

/**
** Function to find an account_type(and its corresponding xref_acct_type) in the
** subaccount table.
**/
int FindAcctTypeInSubacctTable(SUBACCTTABLE zSTable, char *sAcctType) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (i < zSTable.iNumSAcct && iIndex == -1) {
    if (strcmp(zSTable.zSAcct[i].sAcctType, sAcctType) == 0)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findAcctTypeinSubacctTable */

/**
** Function to find a currency id in the currency table
**/
int FindCurrIdInCurrencyTable(CURRTABLE zCTable, char *sCurrId) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (i < zCTable.iNumCurrency && iIndex == -1) {
    if (strcmp(zCTable.zCurrency[i].sCurrId, sCurrId) == 0)
      iIndex = i;

    i++;
  }

  return iIndex;

} /* findCurrIdInCurrencyTable */
} // extern "C"
