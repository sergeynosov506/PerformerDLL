/**
*
* SUB-SYSTEM: payments
*
* FILENAME: maturity.c
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
*
// 2018-11-16 J# PER-9268 Wells Audit - Initialize variables, free
memory,deprecated string function  - sergeyn
**/

#include "payments.h"

extern "C" {

/**
** Function that does the main processing of generating maturity
**/
ERRSTRUCT STDCALL WINAPI GenerateMaturity(long lValDate, const char *sMode,
                                          int iID, const char *sSecNo,
                                          const char *sWi,
                                          const char *sSecXtend,
                                          const char *sAcctType) {
  ERRSTRUCT zErr;
  PINFOTABLE zPInfoTable;
  PORTTABLE zPmainTable;

  lpprInitializeErrStruct(&zErr);
  zPmainTable.iPmainCreated = 0;

  /* Build the table with all the branch account */
  zErr = BuildPortmainTable(&zPmainTable, sMode, iID, zCTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
    InitializePortTable(&zPmainTable);
    return zErr;
  }

  zPInfoTable.iPICreated = 0;
  InitializePInfoTable(&zPInfoTable);
  /*
  ** If batch mode, Create the file with the data from holdings, assets and
  ** bondschd tables.
  */
  if (sMode[0] == 'B') {
    zErr =
        MaturityUnloadAndSort(lValDate + FORWARDDAYS,
                              lValDate - zSysSet.zSyssetng.iPaymentsStartDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) {
      InitializePortTable(&zPmainTable);
      return zErr;
    }

    // Build PInfo table for maturity
    zErr = BuildPInfoTable(&zPInfoTable, "M", lValDate + FORWARDDAYS, "");
    if (zErr.iBusinessError != 0) {
      InitializePInfoTable(&zPInfoTable);
      InitializePortTable(&zPmainTable);
      return zErr;
    }
  } else {
    zPInfoTable.iPICreated = 1;
    zPInfoTable.pzPRec = (PINFO *)realloc(zPInfoTable.pzPRec, sizeof(PINFO));
    if (zPInfoTable.pzPRec == NULL) {
      InitializePInfoTable(&zPInfoTable);
      InitializePortTable(&zPmainTable);
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                             "MATURITY1", FALSE));
    }
    zPInfoTable.pzPRec[0].iID = iID;
    zPInfoTable.pzPRec[0].lStartPosition = 0;
    zPInfoTable.iNumPI = 1;
  }

  // Added lForwardDate for forward transactions
  zErr = MaturityGeneration(zPmainTable, zPInfoTable, zSATable, zCTable,
                            (char *)sMode, (char *)sSecNo, (char *)sWi,
                            (char *)sSecXtend, (char *)sAcctType,
                            lValDate - zSysSet.zSyssetng.iPaymentsStartDate,
                            lValDate, lValDate + FORWARDDAYS);

  /* Free up the memory */
  InitializePortTable(&zPmainTable);
  InitializePInfoTable(&zPInfoTable);

  return zErr;
} /* GenerateMaturity */

/**
** Function that does main processing of generating maturity for all the
** accounts in the supplied porttable.
**/
ERRSTRUCT MaturityGeneration(PORTTABLE zPmainTable, PINFOTABLE zPInfoTable,
                             SUBACCTTABLE zSTable, CURRTABLE zaCTable,
                             char *sMode, char *sSecNo, char *sWi,
                             char *sSecXtend, char *sAcctType, long lStartDate,
                             long lValDate, long lForwardDate) {
  ERRSTRUCT zErr;
  MATTABLE zMTable;
  int i, j, k, l, m, iLastIndex;
  char sLastSecNo[13], sLastWi[2], sLastSecXtend[3], sLastAcctType[2];
  char sFileName[90], sErrMsg[100];
  double fUnits, fTotCost, fOrigFace;
  BOOL bIncTypeFound;

  lpprInitializeErrStruct(&zErr);
  zMTable.iMatCreated = 0;
  InitializeMaturityTable(&zMTable);
  zaCTable.iNumCurrency = 0;
  // Filename for sorted file on valdate
  if (*sMode == 'B')
    strcpy_s(sFileName, MakePricingFileName(lForwardDate, "", "M"));
  else
    strcpy_s(sFileName, "");

  zMTable.iMatCreated = 0;
  iLastIndex = 0;

  // Do processing for all the branch accounts in the pinfo table
  for (i = 0; i < zPInfoTable.iNumPI; i++) {
    m = -1;
    for (j = iLastIndex; j < zPmainTable.iNumPmain; j++) {
      if (zPmainTable.pzPmain[j].iID == zPInfoTable.pzPRec[i].iID) {
        m = j;
        iLastIndex = m + 1;
        break;
      }
    }
    if (m == -1)
      continue;

    // If the account is not set to  generate maturities automatically, nothing
    // to do for this account.
    if (!zPmainTable.pzPmain[m].bMature)
      continue;

    // Build a table of all the records for current portfolio
    zErr = BuildMatTable(sMode, sFileName, zPmainTable.pzPmain[m].iID, sSecNo,
                         sWi, sSecXtend, sAcctType, lStartDate, lForwardDate,
                         zPInfoTable.pzPRec[i].lStartPosition, &zMTable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      lpfnPrintError("Maturity Could Not Be Processed",
                     zPmainTable.pzPmain[m].iID, 0, "", zErr.iBusinessError,
                     zErr.iSqlError, zErr.iIsamCode, "MATURITY PROCESS1",
                     FALSE);

      lpprInitializeErrStruct(&zErr);
      continue;
    }

    if (zMTable.iNumMat == 0)
      continue;
    //    else
    //		lpfnStartDBTransaction();

    // Process All the records for current branch account
    sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = sLastAcctType[0] = '\0';
    fUnits = fTotCost = fOrigFace = 0;
    for (j = 0; j < zMTable.iNumMat; j++) {
      /*
       ** Aggregate values from all the lots for same security and create a
       * TRANS
       ** record out of it. Pass this TRANS record to TranAlloc for processing.
       */
      if (sLastSecNo[0] == '\0' || /* First Time or same security as last */
          strcmp(sLastSecNo, zMTable.pzMaturity[j].sSecNo) == 0 &&
              strcmp(sLastWi, zMTable.pzMaturity[j].sWi) == 0 &&
              strcmp(sLastSecXtend, zMTable.pzMaturity[j].sSecXtend) == 0 &&
              strcmp(sLastAcctType, zMTable.pzMaturity[j].sAcctType) == 0) {
        fUnits += zMTable.pzMaturity[j].fUnits;
        fTotCost += zMTable.pzMaturity[j].fTotCost;
        fOrigFace += zMTable.pzMaturity[j].fOrigFace;

        /* If first record, copy to last secno, wi, etc. */
        if (sLastSecNo[0] == '\0') {
          strcpy_s(sLastSecNo, zMTable.pzMaturity[j].sSecNo);
          strcpy_s(sLastWi, zMTable.pzMaturity[j].sWi);
          strcpy_s(sLastSecXtend, zMTable.pzMaturity[j].sSecXtend);
          strcpy_s(sLastAcctType, zMTable.pzMaturity[j].sAcctType);
        }
      } else {
        k = zPmainTable.pzPmain[m].iCurrIndex;

        /* Find the matching income type */
        bIncTypeFound = FALSE;
        for (l = 0; l < zSTable.iNumSAcct; l++) {
          if (strcmp(zSTable.zSAcct[l].sAcctType,
                     zMTable.pzMaturity[j - 1].sAcctType) == 0) {
            bIncTypeFound = TRUE;
            break;
          }
        } // for l < NumSAcct

        if (!bIncTypeFound) {
          //					lpfnRollbackDBTransaction();
          lpfnPrintError("Invalid SubAccount", zPmainTable.pzPmain[m].iID, 0,
                         "", 110, 0, 0, "MATURITY PROCESS2", FALSE);
        }

        zErr = CreateTransAndCallTranAlloc(
            zMTable.pzMaturity[j - 1], fUnits, fTotCost, fOrigFace,
            zPmainTable.pzPmain[m].sBaseCurrId,
            zPmainTable.pzPmain[m].sAcctMethod, zSTable.zSAcct[l].sXrefAcctType,
            lValDate);
        if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
          ;
        //					lpfnCommitDBTransaction();
        else {
          // lpfnRollbackDBTransaction();
          lpfnPrintError(sErrMsg, zPmainTable.pzPmain[m].iID, 0, "",
                         zErr.iBusinessError, zErr.iSqlError, zErr.iIsamCode,
                         "MATURITY PROCESS3", TRUE);
          lpprInitializeErrStruct(&zErr);
        }

        /* Do a begin work for the next security */
        // lpfnStartDBTransaction();
        strcpy_s(sLastSecNo, zMTable.pzMaturity[j].sSecNo);
        strcpy_s(sLastWi, zMTable.pzMaturity[j].sWi);
        strcpy_s(sLastSecXtend, zMTable.pzMaturity[j].sSecXtend);
        strcpy_s(sLastAcctType, zMTable.pzMaturity[j].sAcctType);

        fUnits = zMTable.pzMaturity[j].fUnits;
        fTotCost = zMTable.pzMaturity[j].fTotCost;
        fOrigFace = zMTable.pzMaturity[j].fOrigFace;
      } /* Different security */
    } /* for j < iNumMat */

    /*
    ** If there was atleast one record in the table, and if the last record in
    ** the table is for same security as its previous record, then for this
    ** security TranAlloc must not have been called yet. If that's the case call
    ** tranalloc. When the control comes here, j is equal to zMTable.iNumMat,
    ** so reduce it by 1 to check last record.
    */
    if (j > 0) {
      j--;
      if (strcmp(sLastSecNo, zMTable.pzMaturity[j].sSecNo) == 0 &&
          strcmp(sLastWi, zMTable.pzMaturity[j].sWi) == 0 &&
          strcmp(sLastSecXtend, zMTable.pzMaturity[j].sSecXtend) == 0 &&
          strcmp(sLastAcctType, zMTable.pzMaturity[j].sAcctType) == 0) {
        if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
          k = zPmainTable.pzPmain[m].iCurrIndex;

          /* Find the matching income type */
          bIncTypeFound = FALSE;
          for (l = 0; l < zSTable.iNumSAcct; l++) {
            if (strcmp(zSTable.zSAcct[l].sAcctType,
                       zMTable.pzMaturity[j].sAcctType) == 0) {
              bIncTypeFound = TRUE;
              break;
            }
          } // for l < zSTable.iNumSAcct

          if (!bIncTypeFound) {
            // lpfnRollbackDBTransaction();
            lpfnPrintError("Invalid SubAccount", zPmainTable.pzPmain[m].iID, 0,
                           "", 110, 0, 0, "MATURITY PROCESS4", TRUE);
          }

          zErr = CreateTransAndCallTranAlloc(
              zMTable.pzMaturity[j], fUnits, fTotCost, fOrigFace,
              zPmainTable.pzPmain[m].sBaseCurrId,
              zPmainTable.pzPmain[m].sAcctMethod,
              zSTable.zSAcct[l].sXrefAcctType, lValDate);
        } // if neither sql error or business error
      } /* If Last Record(in the table) is same as previous record */
      /*if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
                                lpfnCommitDBTransaction();
      else
                                lpfnRollbackDBTransaction();*/
    } // if j > 0
  } /* i < numPmain */
  InitializeMaturityTable(&zMTable);
  return zErr;
} /* MaturityGeneration */

void InitializeMatStruct(MATSTRUCT *pzMStruct) {
  pzMStruct->iID = 0;
  pzMStruct->lTransNo = 0;
  strcpy_s(pzMStruct->sSecNo, " ");
  strcpy_s(pzMStruct->sWi, " ");
  strcpy_s(pzMStruct->sSecXtend, " ");
  strcpy_s(pzMStruct->sAcctType, " ");
  pzMStruct->fUnits = pzMStruct->fOrigFace = 0;
  strcpy_s(pzMStruct->sSecSymbol, " ");
  pzMStruct->iSecID = 0;
  pzMStruct->iSecType = 0;
  pzMStruct->fTrdUnit = 1;
  strcpy_s(pzMStruct->sCurrId, " ");
  strcpy_s(pzMStruct->sIncCurrId, " ");
  pzMStruct->fCurExrate = 1;
  pzMStruct->fCurIncExrate = 1;
  pzMStruct->lMaturityDate = 0;
  pzMStruct->fRedemptionPrice = 100;
  pzMStruct->fVariableRate = 1;
} /* InitializeMatStruct */

void InitializeMatTable(MATTABLE *pzMTable) {
  if (pzMTable->iMatCreated > 0 && pzMTable->pzMaturity != NULL)
    free(pzMTable->pzMaturity);

  pzMTable->pzMaturity = NULL;
  pzMTable->iNumMat = pzMTable->iMatCreated = 0;
}

/**
** Function to build sectypes table
**/
ERRSTRUCT BuildSecTypeTable(SECTYPETABLE *pzSTable) {
  ERRSTRUCT zErr;
  SECTYPE zTempST;

  lpprInitializeErrStruct(&zErr);

  pzSTable->iNumSType = 0;
  while (zErr.iSqlError == 0) {
    lpprSelectAllSectype(&zTempST, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError)
      return (lpfnPrintError("Error Fetching Sec Type Cursor", 0, 0, "", 0,
                             zErr.iSqlError, zErr.iIsamCode,
                             "MATURITY BUILDSEC3", FALSE));

    if (zSTTable.iNumSType == NUMSECTYPES)
      return (lpfnPrintError("Sec Type Table Is Full", 0, 0, "", 997, 0, 0,
                             "MATURITY BUILDSEC4", FALSE));

    pzSTable->zSType[pzSTable->iNumSType++] = zTempST;
  } /* No error in fetching sec type cursor */

  return zErr;
} /* BuildSecTypeTable */

/**
** Function to create the unload file for the entire firm's data required for
** generating Maturity payments for the given date. This function also creates a
** sorted file out of the unloaded file. It first checks if the file
** maturity.unl.dddd (dddd = given date) already exists, if it does, unload
** is skipped. Then it checks if maturity.srt.dddd exists, if it does sorting is
** also skipped(obviously, if a new unload file is created, no need to check if
** sort file exists or not, sorting is always done).
** NOTE : Whenever this function is called it is assumed that the program is
**        running in B(atch) mode.
**/
ERRSTRUCT MaturityUnloadAndSort(long lValDate, long lStartDate) {
  ERRSTRUCT zErr;
  char sFName[90], sFName2[90], sUnlStr[300];
  MATSTRUCT zTempMat;
  FILE *fp;
  long lEndDate;

  lpprInitializeErrStruct(&zErr);

  strcpy_s(sFName, MakePricingFileName(lValDate, "", "M"));
  strcpy_s(sFName2, MakePricingFileName(lValDate, "", "M"));
  /* Create and open file for writing */
  fp = fopen(sFName, "w");
  if (fp == NULL)
    zErr = lpfnPrintError("Error Opening File", 0, 0, "", 999, 0, 0,
                          "MATURITY UNLOAD1", FALSE);

  lEndDate = GetPaymentsEndingDate(lValDate);

  // Fetch all the records and write them to the file
  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) {
    InitializeMatStruct(&zTempMat);
    lpprMaturityUnload(&zTempMat, lStartDate, lEndDate, "", 0, "", "", "", "",
                       &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
      zErr = lpfnPrintError("Error Fetching Unload Cursor", 0, 0, "", 0,
                            zErr.iSqlError, zErr.iIsamCode,
                            "MATURITY FETCHUNLOAD", FALSE);
      break;
    }
    // Get all the fields
    sprintf_s(
        sUnlStr, "%d|%s|%s|%s|%s|%f|%f|%s|%d|%d|%f|%s|%s|%f|%f|%ld|%f|%f|%ld\n",
        zTempMat.iID, zTempMat.sSecNo, zTempMat.sWi, zTempMat.sSecXtend,
        zTempMat.sAcctType, zTempMat.fUnits, zTempMat.fOrigFace,
        zTempMat.sSecSymbol, zTempMat.iSecID, zTempMat.iSecType,
        zTempMat.fTrdUnit, zTempMat.sCurrId, zTempMat.sIncCurrId,
        zTempMat.fCurExrate, zTempMat.fCurIncExrate, zTempMat.lMaturityDate,
        zTempMat.fRedemptionPrice, zTempMat.fVariableRate, zTempMat.lTransNo);
    // Write the string to the file)
    if (fputs(sUnlStr, fp) == EOF)
      zErr = lpfnPrintError("Error Writing To The File", 0, 0, "", 999, 0, 0,
                            "MATURITY UNLOAD2", FALSE);
  } /* while no error */

  fclose(fp);

  /*
  ** Sort the file in the following order :
  ** br_acct(0), sec_no(1), wi(2), sec_xtend(3), acct_type(4)
  */
  /*  sprintf_s(sUnlStr, "sort -o%s -t'|' +0 -1 +1 -2 +2 -3 +3 -4 +4 %s",
    sFName2, sFName); if (system(sUnlStr) != 0) zErr = lpfnPrintError("Error
    Sorting File", 0, 0, "", 999, 0, 0, "MATURITY UNLOAD3", FALSE);*/

  return zErr;
} /* MaturityUnloadAndSort */

/**
** Function to build a memory table of records belonging to an account or a
** specific security for an account. If the functin is running in Batch mode it
** is assumed there is a sorted file with all the records, if it is running in
** a single account mode or single security(in a single account) mode then it
** reads the MATURITY_UNLOAD cursor to get the data.
**/
ERRSTRUCT BuildMatTable(char *sMode, char *sFileName, int iID, char *sSecNo,
                        char *sWi, char *sSecXtend, char *sAcctType,
                        long lStartDate, long lValDate, long lStartPosition,
                        MATTABLE *pzMTable) {
  ERRSTRUCT zErr;
  MATSTRUCT zTempMat;
  FILE *fp;
  int iLastID;
  char sStr1[201], *sStr2;
  char *next_token1 = NULL;
  long lEndDate;
  int posat;

  lpprInitializeErrStruct(&zErr);

  InitializeMatTable(pzMTable);

  if (sMode[0] == 'B') {
    iLastID = 0;
    fp = fopen(sFileName, "r");
    if (fp == NULL)
      zErr = lpfnPrintError("Error Opening File", iID, 0, "", 999, 0, 0,
                            "MATURITY BUILDTABLE1", FALSE);

    // Go directly where the account is starting
    if (fseek(fp, lStartPosition, 0) < 0)
      zErr = lpfnPrintError("Error Seeking File", iID, 0, "", 999, 0, 0,
                            "MATURITY BUILDTABLE2", FALSE);
    posat = ftell(fp);
    /*
    ** Read records from the file. For the record size, we need to give the size
    ** of the biggest record, because fgets stops as soon as it gets a new line
    ** character(or has read the given number of character, whichever is first),
    ** so use 200 as the size, even though the length of records will be much
    ** lower than that.
    */
    while ((zErr.iSqlError == 0 && zErr.iBusinessError == 0) &&
           (fgets(sStr1, 200, fp) != NULL)) {
      InitializeMatStruct(&zTempMat);

      /*
      ** If fetched account is not same as the passed account, there are two
      ** possibility, first is that the last account processed is not null,
      ** which means that we are done(since file is sorted by account), and
      ** second is that last account processed is NULL which means we have not
      ** found records of the the passed account yet, in that case read next
      ** record. If we have found a record of the passed account, then use
      ** strtok to get all the fields out of it into a DILIBSTRUCt variable and
      ** add that to the table.
      */
      sStr2 = strtok_s(sStr1, "|", &next_token1);
      if (atoi(sStr2) != iID) {
        if (iLastID != 0)
          break;
        else
          continue;
      }
      zTempMat.iID = atoi(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sSecNo, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sWi, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sSecXtend, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sAcctType, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fUnits = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fOrigFace = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sSecSymbol, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.iSecID = atoi(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.iSecType = atoi(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fTrdUnit = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sCurrId, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      strcpy_s(zTempMat.sIncCurrId, sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fCurExrate = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fCurIncExrate = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.lMaturityDate = atol(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fRedemptionPrice = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.fVariableRate = atof(sStr2);

      sStr2 = strtok_s(NULL, "|", &next_token1);
      zTempMat.lTransNo = atol(sStr2);

      // Add the record to the given table
      zErr = AddRecordToMatTable(pzMTable, zTempMat);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        break;

      iLastID = zTempMat.iID;
    } /* while not(eof) */

    fclose(fp);
  } /* if batch mode */
  else {
    lEndDate = GetPaymentsEndingDate(lValDate);
    while (zErr.iSqlError == 0) {
      InitializeMatStruct(&zTempMat);
      lpprMaturityUnload(&zTempMat, lStartDate, lEndDate, sMode, iID, sSecNo,
                         sWi, sSecXtend, sAcctType, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND) {
        zErr.iSqlError = 0;
        break;
      } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) {
        zErr = lpfnPrintError("Error Fetching Unload Cursor", 0, 0, "", 0,
                              zErr.iSqlError, zErr.iIsamCode,
                              "MATURITY BUILDTABLE3", FALSE);
        break;
      }
      /* Add the record to the given table */
      zErr = AddRecordToMatTable(pzMTable, zTempMat);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        break;
    } /* while no error */
  } // not batch mode

  return zErr;
} /* BuildMatTable */

/**
** Function to add a record(of MATSTRUCT type) to MATTABLE.
**/
ERRSTRUCT AddRecordToMatTable(MATTABLE *pzMTab, MATSTRUCT zMatStruct) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzMTab->iMatCreated == pzMTab->iNumMat) {
    pzMTab->iMatCreated += NUMMATRECORD;
    pzMTab->pzMaturity = (MATSTRUCT *)realloc(
        pzMTab->pzMaturity, pzMTab->iMatCreated * sizeof(MATSTRUCT));
    if (pzMTab->pzMaturity == NULL)
      return (lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0,
                             "MATURITY ADDMAT", FALSE));
  }

  pzMTab->pzMaturity[pzMTab->iNumMat++] = zMatStruct;

  return zErr;
} /* AddRecordToMatTable */

/**
** Function to create a trans record from a MatStruct record
**/
ERRSTRUCT CreateTransAndCallTranAlloc(MATSTRUCT zMat, double fUnits,
                                      double fTotCost, double fOrigFace,
                                      char *sBaseCurrId, char *sAcctMethod,
                                      char *sIncAcctType, long lValDate) {
  ERRSTRUCT zErr;
  int j;
  TRANS zTrans;
  ASSETS zAssets;
  DTRANSDESC zDTDesc[1];
  TRANTYPE zTType;
  char sTemp[30];
  long lBusinessValDate, lTrdDate, lPriceDate;
  PRICEINFO zPrice;

  lpprInitializeErrStruct(&zErr);
  lBusinessValDate = GetPaymentsEndingDate(lValDate);

  // First Create Trans Record
  lpprInitializeTransStruct(&zTrans);
  zTrans.iID = zMat.iID;
  zTrans.lTransNo = zMat.lTransNo;
  strcpy_s(zTrans.sSecNo, zMat.sSecNo);
  strcpy_s(zTrans.sWi, zMat.sWi);
  strcpy_s(zTrans.sSecXtend, zMat.sSecXtend);
  strcpy_s(zTrans.sAcctType, zMat.sAcctType);
  strcpy_s(zTrans.sSecSymbol, zMat.sSecSymbol);
  zTrans.iSecID = zMat.iSecID;

  if (fUnits < 0.0) {
    zTrans.fUnits = fUnits *= -1.0;
    if (fTotCost < 0)
      fTotCost *= -1.0;
    lpprSelectTranType(&zTType, "MS", "DR", &zErr);
    strcpy_s(zTrans.sTranType, "MS");
    strcpy_s(zTrans.sDrCr, "DR");
  } else {
    zTrans.fUnits = fUnits;
    lpprSelectTranType(&zTType, "ML", "CR", &zErr);
    strcpy_s(zTrans.sTranType, "ML");
    strcpy_s(zTrans.sDrCr, "CR");
  }
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  /*  if (strcmp(zMat.sBschdType, "C") == 0)
    {
      if (bShort)
                    lpprSelectTranType(&zTType, "CD", "DR", &zErr);
      else
                    lpprSelectTranType(&zTType, "CB", "CR", &zErr);
    }
    else if (strcmp(zMat.sBschdType, "M") == 0)
    {
      if (bShort)
                    lpprSelectTranType(&zTType, "MS", "DR", &zErr);
      else
                    lpprSelectTranType(&zTType, "ML", "CR", &zErr);
    }
    else
      return(lpfnPrintError("Invalid Bond Schedule Type", zMat.iID, 0, "", 999,
    0, 0, "MATURITY CREATETRANS2", FALSE));*/

  lpprSelectStarsDate(&lTrdDate, &lPriceDate, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  zTrans.fOrigFace = fOrigFace;
  zTrans.fPcplAmt = zTrans.fUnits * zMat.fRedemptionPrice * zMat.fTrdUnit *
                    zMat.fVariableRate;
  zTrans.lTrdDate = zMat.lMaturityDate;
  zTrans.lStlDate = zMat.lMaturityDate;
  zTrans.lEffDate = zMat.lMaturityDate;
  zTrans.lEntryDate = lTrdDate;

  strcpy_s(zTrans.sCurrId, zMat.sCurrId);
  strcpy_s(zTrans.sCurrAcctType, zMat.sAcctType);

  strcpy_s(zTrans.sIncCurrId, zMat.sIncCurrId);
  strcpy_s(zTrans.sIncAcctType, zMat.sAcctType);

  strcpy_s(zTrans.sSecCurrId, zMat.sCurrId);
  strcpy_s(zTrans.sAccrCurrId, zMat.sIncCurrId);

  if (strcmp(zMat.sCurrId, sBaseCurrId) == 0)
    zTrans.fBaseXrate = 1.0;
  else if (strcmp(zSysSet.zSyssetng.sSystemcurrency, sBaseCurrId) == 0)
    zTrans.fBaseXrate = zMat.fCurExrate;
  else {
    lpprGetSecurityPrice(sBaseCurrId, "N", zMat.lMaturityDate, &zPrice, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zTrans.fBaseXrate = zMat.fCurExrate / zPrice.fExrate;
  }

  if (strcmp(zMat.sCurrId, zMat.sIncCurrId) == 0)
    zTrans.fIncBaseXrate = zTrans.fBaseXrate;
  else if (strcmp(zMat.sIncCurrId, sBaseCurrId) == 0)
    zTrans.fIncBaseXrate = 1.0;
  else if (strcmp(zSysSet.zSyssetng.sSystemcurrency, sBaseCurrId) == 0)
    zTrans.fIncBaseXrate = zMat.fCurExrate;
  else {
    lpprGetSecurityPrice(sBaseCurrId, "N", zMat.lMaturityDate, &zPrice, &zErr);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    zTrans.fIncBaseXrate = zMat.fCurExrate / zPrice.fExrate;
  }

  zTrans.fSecBaseXrate = zTrans.fBaseXrate;
  zTrans.fAccrBaseXrate = zTrans.fIncBaseXrate;
  zTrans.fSysXrate = zMat.fCurExrate;
  zTrans.fIncSysXrate = zMat.fCurIncExrate;
  zTrans.fTotCost = fTotCost; // zMat.fTotCost;
  strcpy_s(zTrans.sAcctMthd, sAcctMethod);
  strcpy_s(zTrans.sMiscDescInd, "N");
  strcpy_s(zTrans.sTransSrce, "P");
  strcpy_s(zTrans.sCreatedBy, "PRICING");
  zTrans.lPostDate = lTrdDate;

  _strdate(sTemp);
  lpfnrstrdate(sTemp, &(zTrans.lCreateDate));
  _strtime(zTrans.sCreateTime);

  // Now Create Asset Record, don't have to worry about fields not required by
  // TranAlloc.
  strcpy_s(zAssets.sSecNo, zMat.sSecNo);
  strcpy_s(zAssets.sWhenIssue, zMat.sWi);
  zAssets.iSecType = zMat.iSecType;
  zAssets.fTradUnit = zMat.fTrdUnit;
  strcpy_s(zAssets.sCurrId, zMat.sCurrId);
  strcpy_s(zAssets.sIncCurrId, zMat.sIncCurrId);
  zAssets.fCurExrate = zMat.fCurExrate;
  zAssets.fCurIncExrate = zMat.fCurIncExrate;

  j = FindSecType(zSTTable, zAssets.iSecType);
  if (j < 0)
    return (lpfnPrintError("Invalid Sec Type", zMat.iID, 0, "", 18, 0, 0,
                           "MATURITY CREATETRANS3", FALSE));

  // for forward transactions
  if (zMat.lMaturityDate > lBusinessValDate)
    zErr = CreateAndInsertFWTrans(zTrans, zSTTable.zSType[j].sPayType);
  else // for active transactions
       // TranAlloc (...TRUE) allows usage of DB Transactions
    zErr = lpfnTranAlloc(&zTrans, zTType, zSTTable.zSType[j], zAssets, zDTDesc,
                         0, NULL, "C", TRUE); // FALSE );

  return zErr;
} // CreateTransAndCallTranAlloc

/**
** Function to create and insert a record in FWTrans table
**/
ERRSTRUCT CreateAndInsertFWTrans(TRANS zTrans, const char PayType[]) {
  ERRSTRUCT zErr;
  FWTRANS zFWTrans;

  lpprInitializeErrStruct(&zErr);
  if (PayType[0] == 'D') {
    if (zTrans.fPcplAmt - zTrans.fTotCost >= 0) {
      zTrans.fAccrInt = zTrans.fPcplAmt - zTrans.fTotCost;
      zTrans.fIncomeAmt = zTrans.fAccrInt;
      zTrans.fPcplAmt = zTrans.fTotCost;
    } else
      zTrans.fAccrInt = zTrans.fIncomeAmt = 0;
    strcpy_s(zTrans.sIncomeFlag, "D");
  }

  zFWTrans.iID = zTrans.iID;
  zFWTrans.lTransNo = zTrans.lTransNo;
  zFWTrans.lDivintNo = 0;
  strcpy_s(zFWTrans.sTranType, zTrans.sTranType);
  strcpy_s(zFWTrans.sSecNo, zTrans.sSecNo);
  strcpy_s(zFWTrans.sWi, zTrans.sWi);
  zFWTrans.iSecID = zTrans.iSecID;
  strcpy_s(zFWTrans.sSecXtend, zTrans.sSecXtend);
  strcpy_s(zFWTrans.sAcctType, zTrans.sAcctType);
  strcpy_s(zFWTrans.sDivType, "");
  zFWTrans.fDivFactor = 0.0;
  zFWTrans.fUnits = zTrans.fUnits;
  zFWTrans.fPcplAmt = zTrans.fPcplAmt;
  zFWTrans.fIncomeAmt = zTrans.fIncomeAmt;
  zFWTrans.lTrdDate = zTrans.lTrdDate;
  zFWTrans.lStlDate = zTrans.lStlDate;
  zFWTrans.lEffDate = zTrans.lEffDate;
  strcpy_s(zFWTrans.sCurrId, zTrans.sCurrId);
  strcpy_s(zFWTrans.sIncCurrId, zTrans.sIncCurrId);
  strcpy_s(zFWTrans.sIncAcctType, zTrans.sIncAcctType);
  strcpy_s(zFWTrans.sSecCurrId, zTrans.sSecCurrId);
  strcpy_s(zFWTrans.sAccrCurrId, zTrans.sAccrCurrId);
  zFWTrans.fBaseXrate = zTrans.fBaseXrate;
  zFWTrans.fIncBaseXrate = zTrans.fIncBaseXrate;
  zFWTrans.fSecBaseXrate = zTrans.fSecBaseXrate;
  zFWTrans.fAccrBaseXrate = zTrans.fAccrBaseXrate;
  zFWTrans.fSysXrate = zTrans.fSysXrate;
  zFWTrans.fIncSysXrate = zTrans.fIncSysXrate;
  strcpy_s(zFWTrans.sDrCr, zTrans.sDrCr);
  zFWTrans.lCreateDate = zTrans.lCreateDate;
  strcpy_s(zFWTrans.sCreateTime, zTrans.sCreateTime);

  strcpy_s(zFWTrans.sDescription, "Will Mature");

  lpprInsertFWTrans(&zFWTrans, &zErr);
  return zErr;
}

/**
** Function to find given sectype in the global sectype table
**/
int FindSecType(SECTYPETABLE zSTypeTable, int iSType) {
  int i, iIndex;

  i = 0;
  iIndex = -1;

  while (iIndex == -1 && i < zSTypeTable.iNumSType) {
    if (iSType == zSTypeTable.zSType[i].iSecType)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* FindSecType */
} // extern "C"
