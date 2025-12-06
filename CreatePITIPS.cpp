/*
* 
* FILENAME: CreatePITIPS.c
* 
* DESCRIPTION: To create PI/BA trans for TIPS holdings of a portfolio for the given date
* 
* PUBLIC FUNCTION(S): 
*                      1. DLLAPI ERRSTRUCT STDCALL WINAPI CreatePITIPS(int iPortfolioID, long lDate);
* 
* 
* USAGE: To be called from Portfolio Actions app
* 
* AUTHOR: Valeriy Yegorov
*
**/
// History
// 2018-11-16 J# PER-9268 Wells Audit - Initialize variables, free memory,deprecated string function  - sergeyn
// 2/11/2010 - VI#43627 More changes  - mk
// 2/11/2010 - VI#43627 Modified logic to create PIs for those accounts, securities that apply  - mk
// 3/18/2004 - Fixed call to tranalloc - number of desc items is 0  - vay
// 3/18/2004 - Added DB transactions  - vay
// 2/20/2004 - Don't generate descriptions on PI/BA - vay

#include "CreatePITIPS.H"
	

/*F*
** Function called by the user to do Phantom Income processing for TIPS. 
** Valid Modes are B(batch), A(single account) and S (single security in an account). 
*F*/
DLLAPI ERRSTRUCT STDCALL WINAPI GeneratePhantomIncome(long lValDate, char *sMode, 
																								 int iID, char *sSecNo, char *sWi, 
																								 char *sSecXtend, 
																								 char *sAcctType, long lTransNo)
{ 
	ERRSTRUCT    zErr;
  PINFOTABLE   zPInfoTable;
  PORTTABLE    zPmainTable;
  ASSETS zAssets;
char						sMsg[80];

  lpprInitializeErrStruct(&zErr);
  zPmainTable.iPmainCreated = 0;
  zPInfoTable.iPICreated = 0;
  InitializePInfoTable(&zPInfoTable);

  if (strcmp(sSecNo, "") != 0 && strcmp(sWi, "") != 0) 
  {

	lpprSelectAsset(&zAssets, sSecNo, sWi, -1, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
	{
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			sprintf_s(sMsg, "Security: %s, Wi: %s Not Found In Asset Table", sSecNo, sWi);
			return(lpfnPrintError(sMsg, 0, 0, "T",  8, 0, 0, "PITIPSANF", FALSE));
		}

		return zErr;
	}
	if (strcmp(zAssets.sAutoDivint, "Y") != 0) return zErr;
  }


	/* If in batch mode, create the file with data from holdings, fixedinc, etc. */
  if (sMode[0] == 'B')
  {
    zErr = PITIPSUnloadAndSort(lValDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    /* Build PInfo table */
    zErr = BuildPInfoTable(&zPInfoTable, "D", lValDate, "T");
    if (zErr.iBusinessError != 0)
      return zErr;
  } 
  else
  {
     zPInfoTable.iPICreated = 1;
     zPInfoTable.pzPRec = (PINFO *)realloc(zPInfoTable.pzPRec, sizeof(PINFO));
     if (zPInfoTable.pzPRec == NULL)
       return(lpfnPrintError("Insufficient Memory", 0, 0, "",  997, 0, 0, "PITIPS1", FALSE));

     zPInfoTable.pzPRec[0].iID = iID;
     zPInfoTable.pzPRec[0].lStartPosition = 0;
     zPInfoTable.iNumPI = 1;
  }

  /* Build the table with all the branch account */
  zErr = BuildPortmainTable(&zPmainTable, sMode, iID, zCTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
  {
    InitializePInfoTable(&zPInfoTable); /* Free up the memory */
    return zErr;
  }

  /* Call the function which does actual work */
  zErr = PITIPSGeneration(zPmainTable, zPInfoTable, zSTTable, zCTable, 
		                      lValDate, sMode, sSecNo, sWi, sSecXtend, sAcctType, lTransNo);

  /* Free up the memory */
  InitializePInfoTable(&zPInfoTable);
  InitializePortTable(&zPmainTable); 
  return zErr;
} /* CreatePITIPS */

/*F*
** Function that does main processing of Phantom Income 
** for all the accounts in the supplied porttable.
*F*/
ERRSTRUCT PITIPSGeneration(PORTTABLE zPMainTable, PINFOTABLE zPInfoTable, 
													 SECTYPETABLE zSTable, CURRTABLE zCTable, 
													 long lValDate, char *sMode,
													 char *sSecNo, char *sWi,
													 char *sSecXtend, char *sAcctType, long lTransNo)
{
  ERRSTRUCT		zErr;
  PITIPSTABLE zPITable;
  int					i, j, k, m, iLastID;
  char				sLastSecNo[13], sLastWi[2], sLastSecXtend[3], sLastAcctType[3];
  char				sFileName[30], sErrMsg[100];
  long				lLastTransNo; 
  BOOL				bYearEnd;
	BOOL				bDoTrans;
  ASSETS zAssets;
	

  lpprInitializeErrStruct(&zErr);
  zPITable.iPICreated = 0;
	InitializePITIPSTable(&zPITable);

  /* Filename for sorted file on valdate, the "S" parameter does not do anything here */
	strcpy_s(sFileName, MakePricingFileName(lValDate, "T", "D"));

  /* Is the val date a year end date */
  bYearEnd = IsThisAnYearEnd(lValDate);
 
  zPITable.iPICreated = 0;
  /* Do processing for all the branch accounts in the port table */
  for (i = 0; i < zPMainTable.iNumPmain; i++)
  {
	  if (!zPMainTable.pzPmain[i].bIncome) continue;

    /* Build a table of all the records for current branch account */
    zErr = BuildPITable(sMode, sFileName, lValDate,
												zPMainTable.pzPmain[i].iID, sSecNo,
                        sWi, sSecXtend, sAcctType, lTransNo, &zPITable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    {
      lpfnPrintError("Phantom Income Could Not Be Processed", zPMainTable.pzPmain[i].iID, 0, "", 
										 999, 0, 0, "PITIPS2", TRUE);
      lpprInitializeErrStruct(&zErr);
      continue;
    }

    /* Process All the records for current iID */
    sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = '\0';
    sLastAcctType[0] = '\0';
		iLastID = 0;
    lLastTransNo = 0;
    for (j = 0; j < zPITable.iNumPI; j++)
    {
      /* 
      ** On a year end, processing is done for all the lots, 
			** otherwise only those lots are processed for which 
			** DayOfMonth(MaturityDate) = DayOfMonth(ValDate)
			** i.e. if running for 1/15/04, then only securities
			** maturing on 15th (any month) will be processed
      */
      if (!(bYearEnd || 
						DayOfMonth(zPITable.pzPI[j].lMaturityDate) == DayOfMonth(lValDate)))
        continue;

      k = FindSecType(zSTable, zPITable.pzPI[j].iSecType);
      if (k < 0)
			{
        InitializePITIPSTable(&zPITable);
				return(lpfnPrintError("Invalid Sectype", zPMainTable.pzPmain[i].iID, 0, "",  
													     14, 0, 0, "PITIPS3", FALSE));
      }  
			
			// If not a TIPS security, no phantom income
			if (!((zSTable.zSType[k].sPrimaryType[0] == PTYPE_BOND)
					&& (zSTable.zSType[k].sSecondaryType[0] == STYPE_TBILLS) 
					&& (zSTable.zSType[k].sPayType[0] == PAYTYPE_INFPROTCTD)))
				continue;

			// if effective date on the lot is greater or equal to the current pricing date, skip it
			if (zPITable.pzPI[j].lEffDate >= lValDate)
				continue;

			// if current pricing date is greater or equal to maturity date, skip it
			// (should have been already matured and PI generated at ML)
			if (lValDate >= zPITable.pzPI[j].lMaturityDate)
				continue;

			if (strcmp(sSecNo, "") != 0 && strcmp(sWi, "") != 0) 
			{

				lpprSelectAsset(&zAssets, zPITable.pzPI[j].sSecNo, zPITable.pzPI[j].sWi, -1, &zErr);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				{
					if (zErr.iSqlError == SQLNOTFOUND)
					{
						sprintf_s(sErrMsg, "Security: %s, Wi: %s Not Found In Asset Table", zPITable.pzPI[j].sSecNo, zPITable.pzPI[j].sWi);
						return(lpfnPrintError(sErrMsg, 0, 0, "T",  8, 0, 0, "PITIPSANF", FALSE));
					}
					return zErr;
				}
				if (strcmp(zAssets.sAutoDivint, "Y") != 0) return zErr;
			}
      /*
      ** If the security is same as last one processed and if there was an
      ** error in last processing, don't do any processing(all the lots are 
      ** going to be rolled back). If on the other hand security is not same
      ** as last one, finish the current database transaction with a commit(no
      ** errors for any lot) or a rollback and then start a new transaction.
      */
      if (zPITable.pzPI[j].iID == iLastID && 
				  strcmp(zPITable.pzPI[j].sSecNo, sLastSecNo) == 0 &&
          strcmp(zPITable.pzPI[j].sWi, sLastWi) == 0 && 
          strcmp(zPITable.pzPI[j].sSecXtend, sLastSecXtend) == 0 &&
          strcmp(zPITable.pzPI[j].sAcctType, sLastAcctType) == 0)
      {
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				{
         //print error message
					continue;
				}
      }
      else 
      {
        if (iLastID != 0) /* Not the First Record */
        {
          if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
          {
            lpfnPrintError(sErrMsg, iLastID, 0, "",  999, 0, 0, "PITIPS4", TRUE);
            lpprInitializeErrStruct(&zErr);
          }
				} /* Not the first record */

        iLastID = zPITable.pzPI[j].iID;
        strcpy_s(sLastSecNo, zPITable.pzPI[j].sSecNo);
        strcpy_s(sLastWi, zPITable.pzPI[j].sWi);
        strcpy_s(sLastSecXtend, zPITable.pzPI[j].sSecXtend);
        strcpy_s(sLastAcctType, zPITable.pzPI[j].sAcctType);
      } /* Different security than last time */

			m = zPMainTable.pzPmain[i].iCurrIndex;

			//now generate and post PI and BA transactions (using DB transactions)
			bDoTrans = (lpfnGetTransCount()==0);
			if (bDoTrans) 
			{
				zErr.iBusinessError = lpfnStartTransaction();
 				if (zErr.iBusinessError != 0)
					return (lpfnPrintError("Unable to start DB trans", zPMainTable.pzPmain[i].iID, 0, "", 
									999, 0, 0, "PITIPS5", TRUE));
			}

			__try
			{

				zErr = PITIPSCallTranAlloc(zPITable.pzPI[j], zPMainTable.pzPmain[i].sAcctMethod, 
																zSTable.zSType[k], lValDate);
			}
			__except(lpfnAbortTransaction(bDoTrans)){}

			if (bDoTrans) 
			{
				if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
					lpfnCommitTransaction();
				else
				{
					lpfnRollbackTransaction();
					lpfnPrintError("Unable to post phantom income", zPMainTable.pzPmain[i].iID, 0, "", 
									zErr.iBusinessError, zErr.iSqlError, 0, "PITIPS6", TRUE);
				}
			} 

    } /* for j < iNumPI */ 

    /* For the last lot of each branch account */
      zErr.iSqlError = zErr.iBusinessError = 0;
  } /* i < numpdir */

	InitializePITIPSTable(&zPITable);
  return zErr;
} /* CreatePITIPS */

/*F*
** Function to create the unload file for the entire firm's data required for
** creating phantom income transactions for the given date. This function
** also creates a sorted file out of the unloaded file. It first checks if the 
** file PITIPS.unl.dddd (dddd = given date) already exists, if it does, unload
** is skipped. Then it checks if PITIPS.srt.dddd exists, if it does sorting is
** also skipped(obviously, if a new unload file is created, no need to check if
** sort file exists or not, sorting is always done). 
*F*/
ERRSTRUCT PITIPSUnloadAndSort(long lValDate)
{
	ERRSTRUCT			zErr;
	char					sFName[30], sUnlStr[500];
	PITIPSSTRUCT	zTempPI;
	FILE					*fp;

	lpprInitializeErrStruct(&zErr);

	strcpy_s(sFName, MakePricingFileName(lValDate, "T", "D"));

	/* Create, if required, and open the file for writing */
	fp = fopen(sFName, "w");
	if (fp == NULL)
		zErr = lpfnPrintError("Error Opening File", 0, 0, "", 999, 0, 0,
			"PITIPS UNLOAD1", FALSE);

	/* Fetch all the records and write them to the file */
	while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
	{
		InitializePITIPSStruct(&zTempPI);
		lpprPITIPSUnload("B", lValDate, 0, "", "", "", "", 0, &zTempPI, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			break;

		if (strcmp(zTempPI.sSecNo, "") == 0)
			strcpy_s(zTempPI.sSecNo, " ");
		if (strcmp(zTempPI.sWi, "") == 0)
			strcpy_s(zTempPI.sWi, " ");
		if (strcmp(zTempPI.sSecXtend, "") == 0)
			strcpy_s(zTempPI.sSecXtend, " ");
		if (strcmp(zTempPI.sSecSymbol, "") == 0)
			strcpy_s(zTempPI.sSecSymbol, " ");
		if (strcmp(zTempPI.sAcctType, "") == 0)
			strcpy_s(zTempPI.sAcctType, " ");
		if (strcmp(zTempPI.sCurrId, "") == 0)
			strcpy_s(zTempPI.sCurrId, " ");
		if (strcmp(zTempPI.sIncCurrId, "") == 0)
			strcpy_s(zTempPI.sIncCurrId, " ");

		/* Get all the fields in a string */
		sprintf_s(sUnlStr, "%s|%d|%s|%s|%s|%s|%s|%ld|%d|%f|%ld|%ld|%ld|%f|%f|"
			"%d|%f|%s|%s|%ld\n",
			"H\0", zTempPI.iID, zTempPI.sSecNo, zTempPI.sWi,
			zTempPI.sSecXtend, zTempPI.sSecSymbol,
			zTempPI.sAcctType, zTempPI.lTransNo, zTempPI.iSecID, zTempPI.fUnits,
			zTempPI.lTrdDate, zTempPI.lStlDate, zTempPI.lEffDate,
			zTempPI.fTotCost, zTempPI.fOrigCost,
			zTempPI.iSecType, zTempPI.fTradUnit,
			zTempPI.sCurrId, zTempPI.sIncCurrId,
			zTempPI.lMaturityDate);

		/* Write the string to the file */
		if (fputs(sUnlStr, fp) == EOF)
			zErr = lpfnPrintError("Error Writing To The File", 0, 0, "", 999, 0, 0,
				"PITIPS UNLOAD2", FALSE);

	} /* while no error */
	fclose(fp);
	return zErr;
} /* PITIPSUnloadAndSort */

/*F*
** Function to build a memory table of all the records belonging to an account.
** The function assumes that the file is sorted 
** (by branch_account, sec_no, wi, trans_no, etc) and 
** it has records in PITIPSSTRUCT format, each field separated by '|' character. 
** At this moment, this function works only for a single account,
** it later may be extended to accept a list of accounts.
*F*/
ERRSTRUCT BuildPITable(char *sMode, char *sFileName, long lValDate,
	int iID, char *sSecNo, char *sWi, char *sSecXtend,
	char *sAcctType, long lTransNo, PITIPSTABLE *pzPITable)
{
	ERRSTRUCT			zErr;
	PITIPSSTRUCT	zTempPI;
	FILE					*fp;
	char					sStr1[301], *sStr2;
	char *next_token1 = NULL;
	int						i;

	lpprInitializeErrStruct(&zErr);
	InitializePITIPSTable(pzPITable);

	if (sMode[0] == 'B')
	{
		fp = fopen(sFileName, "r");
		if (fp == NULL)
			zErr = lpfnPrintError("Error Opening File", 0, 0, "", 999, 0, 0,
				"PITIPS BUILDTABLE1", FALSE);

		/*
		** Read records from the file. For the record size, we need to give the size
		** of the biggest record, because fgets stops as soon as it gets a new line
		** character(or has read the given number of character, whichever is first),
		** so use 300 as the size, even though the length of records will be much
		** lower than that.
		*/
		while ((zErr.iSqlError == 0 && zErr.iBusinessError == 0)&&(fgets(sStr1, 300, fp) != NULL))
		{
			InitializePITIPSStruct(&zTempPI);

			sStr2 = strtok_s(sStr1, "|", &next_token1);

			if (sStr2[0] != 'H')
			{
				zErr = lpfnPrintError("Record Can Only Come From H(oldings)",
					iID, 0, "", 995, 0, 0, "PITIPS BUILDTABLE", FALSE);
				break;
			}
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
			sStr2 = strtok_s(NULL, "|", &next_token1);
			i = atoi(sStr2);
			if (i > iID)
				break;
			else if (i < iID)
				continue;

			zTempPI.iID = i;

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sSecNo, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sWi, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sSecXtend, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sSecSymbol, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sAcctType, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.lTransNo = atol(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.iSecID = atoi(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.fUnits = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.lTrdDate = atol(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.lStlDate = atol(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.lEffDate = atol(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.fTotCost = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.fOrigCost = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.iSecType = atoi(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.fTradUnit = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sCurrId, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempPI.sIncCurrId, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempPI.lMaturityDate = atol(sStr2);

			/* Add the record to the given table */
			zErr = AddRecordToPITable(pzPITable, zTempPI);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				break;
		} /* while not(eof) */
		fclose(fp);
	} /* if batch mode */
	else
	{
		while (zErr.iSqlError == 0)
		{
			lpprPITIPSUnload(sMode, lValDate,
				iID, sSecNo, sWi, sSecXtend, sAcctType, lTransNo,
				&zTempPI, &zErr);

			if (zErr.iSqlError == SQLNOTFOUND)
			{
				zErr.iSqlError = 0;
				break;
			}
			else if (zErr.iSqlError != 0)
				break;

			zErr = AddRecordToPITable(pzPITable, zTempPI);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				break;
		} /* No error */

	} /* Not in batch mode */

	return zErr;
} /* BuildPITable */

/*F*
** Function to add a record(of PITIPSSTRUCT type) to PITIPSTABLE.
*F*/
ERRSTRUCT AddRecordToPITable(PITIPSTABLE *pzPITab, PITIPSSTRUCT zPI)
{
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzPITab->iPICreated == pzPITab->iNumPI)
  {
    pzPITab->iPICreated += NUMPITIPS;
    pzPITab->pzPI = (PITIPSSTRUCT *)realloc(pzPITab->pzPI, 
                                  pzPITab->iPICreated * sizeof(PITIPSSTRUCT));
    if (pzPITab->pzPI == NULL)
      return(lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0, 
                        "PITIPS ADDREC", FALSE));
  }

  pzPITab->pzPI[pzPITab->iNumPI++] = zPI;

  return zErr;
} /* AddRecordToPITable */

ERRSTRUCT PITIPSCallTranAlloc(PITIPSSTRUCT zPI, char *sAcctMthd, 
															SECTYPE zSType, long lValDate)
{
  ERRSTRUCT		zErr;
  TRANS				zTR;
	ASSETS			zAssets;
  DTRANSDESC	zDTr[1];
	TRANTYPE		zTranType;

	char				sTemp[30];

	int					iCount;
	long				lLastIncDate;
	double			fPhantomIncome;
	BOOL				bFirstIncome;

  lpprInitializeErrStruct(&zErr);
	lpprInitializeTransStruct(&zTR);
	lpprInitializeDtransDesc(&zDTr[0]);

  // copy basic fields to Trans record
	zTR.iID = zPI.iID;
  strcpy_s(zTR.sSecNo, zPI.sSecNo);
  strcpy_s(zTR.sWi, zPI.sWi);
  strcpy_s(zTR.sSecXtend, zPI.sSecXtend);
  strcpy_s(zTR.sAcctType, zPI.sAcctType);
  strcpy_s(zTR.sSecSymbol, zPI.sSecSymbol);
	zTR.iSecID = zPI.iSecID;

	// get the latest date on which PI/RI/PS was posted
	zTR.lTaxlotNo = zPI.lTransNo;
	zTR.lStlDate = zTR.lTrdDate = zTR.lEffDate = zTR.lEntryDate = lValDate;
	zTR.fUnits = zPI.fUnits;

	lLastIncDate = 0;
	lpprGetLastIncomeDate(zTR.iID, zTR.lTaxlotNo, zTR.lStlDate, &lLastIncDate, &iCount, &zErr); 
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	// if income has been already posted for this date - don't do it again
/*	
	if (lLastIncDate == lValDate)
		return zErr;
*/
	if (iCount == 1)
		bFirstIncome = TRUE;
	else
		bFirstIncome = FALSE;


	// calculate the phantom income (equal to increase/decrease in principal value
	// of the position) between last income date and this income date
	zErr = lpfnCalculatePhantomIncome(zTR.iID, zTR.sSecNo, zTR.sWi, zTR.lTaxlotNo, lLastIncDate, 
																		zTR.lStlDate, zPI.lTrdDate, zPI.fOrigCost, zPI.fUnits,
																		bFirstIncome, &fPhantomIncome);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	// Post Phantom Income Transaction
	strcpy_s(zTR.sTranType, "PI");
  strcpy_s(zTR.sCurrId, zPI.sCurrId);
  strcpy_s(zTR.sCurrAcctType, zPI.sAcctType);
  strcpy_s(zTR.sIncCurrId, zPI.sCurrId);
  strcpy_s(zTR.sIncAcctType, zPI.sAcctType);
  strcpy_s(zTR.sSecCurrId, zPI.sCurrId);
  strcpy_s(zTR.sAccrCurrId, zPI.sIncCurrId);

  strcpy_s(zTR.sAcctMthd, sAcctMthd);
  strcpy_s(zTR.sTransSrce, "S");
  strcpy_s(zTR.sCreatedBy, "PRICING");
	strcpy_s(zTR.sMiscDescInd, "N");
	_strdate(sTemp);
	lpfnrstrdate(sTemp, &zTR.lCreateDate);
	_strtime(zTR.sCreateTime);

	if (IsValueZero(fPhantomIncome, 2))
		return zErr; // if no increase in value, no need to generatePI & BA transaction
	else if (fPhantomIncome > 0)
		strcpy_s(zTR.sDrCr, "CR");
	else
	  strcpy_s(zTR.sDrCr, "DR");

	zTR.fIncomeAmt = fabs(fPhantomIncome);

	// init assets record for TranProc
	CopyFieldsFromTransToAssets(zTR, zPI.fTradUnit, &zAssets);

	// Get the trantype row for the transaction 
	lpprSelectTranType(&zTranType, zTR.sTranType, zTR.sDrCr, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;
			
	// Pass the transaction to tranalloc
	zErr = lpfnTranAlloc(&zTR, zTranType, zSType, zAssets, 
												zDTr, 0, NULL, "C", FALSE);
	if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		return zErr;

	// Now do the basis adjustmet transaction for the same amount
	strcpy_s(zTR.sTranType, "BA");
	strcpy_s(zTR.sBalToAdjust, "CURR");
	zTR.fPcplAmt = zTR.fIncomeAmt ;
	zTR.fIncomeAmt = 0;

	lpprSelectTranType(&zTranType, zTR.sTranType, zTR.sDrCr, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	// Pass the transaction to tranalloc
	zErr = lpfnTranAlloc(&zTR, zTranType, zSType, zAssets, 
											  zDTr, 0, NULL, "C", FALSE);
  return zErr;

} /* AmortCallTranAlloc */

void InitializePITIPSTable(PITIPSTABLE *pzPITable)
{
  if (pzPITable->iPICreated > 0 && pzPITable->pzPI != NULL)
    free(pzPITable->pzPI);

  pzPITable->pzPI = NULL;
  pzPITable->iPICreated = pzPITable->iNumPI = 0;
}

void InitializePITIPSStruct(PITIPSSTRUCT *pzPI)
{
	memset(pzPI, 0, sizeof(*pzPI));
} /* InitializePITIPSStruct */

BOOL IsThisAnYearEnd(long lDate)
{
  short iMDY[3];

  if (lpfnrjulmdy(lDate, iMDY) < 0)
    return FALSE;

  return ((iMDY[0] == 12) && (iMDY[1] == 31));
}

int DayOfMonth(long lDate)
{
  short iMDY[3];

  if (lpfnrjulmdy(lDate, iMDY) < 0)
    return 0;
	else
		return iMDY[1];
}

