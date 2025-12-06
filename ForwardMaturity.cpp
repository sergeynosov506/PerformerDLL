/*
	This unit is used to create Forward Maturity transactions.
// 2018-11-16 J# PER-9268 Wells Audit - Initialize variables, free memory,deprecated string function  - sergeyn
*/


#include "Payments.h"


DLLAPI ERRSTRUCT STDCALL WINAPI GenerateForwardMaturity(long lValDate, char *sMode, int iID, 
																									 char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType)
{
  long lForwardDate;
	ERRSTRUCT  zErr;
  PINFOTABLE zPInfoTable;
	PORTTABLE  zPmainTable;
	

  lpprInitializeErrStruct(&zErr);
  lForwardDate = lValDate + FORWARDDAYS;
  zPmainTable.iPmainCreated = 0;
  
	/* Build the table with all the branch account */
  zErr = BuildPortmainTable(&zPmainTable, sMode, iID, zCTable);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
	{
     InitializePortTable(&zPmainTable);
		 return zErr;
	}

  zPInfoTable.iPICreated = 0;
  InitializePInfoTable(&zPInfoTable);
  /* 
  ** If batch mode, Create the file with the data from holdings, assets and 
  ** bondschd tables.
  */
  if (sMode[0] == 'B') 
  {
    zErr = ForwardMaturityUnloadAndSort(lForwardDate, lValDate - zSysSet.zSyssetng.iPaymentsStartDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
     InitializePortTable(&zPmainTable);
		 return zErr;
		}

    /* Build PInfo table for dividends */
    zErr = BuildPInfoTable(&zPInfoTable, "F", lForwardDate, "");
    if (zErr.iBusinessError != 0)
		{
			InitializePInfoTable(&zPInfoTable);
		  InitializePortTable(&zPmainTable);
			return zErr;
		}
  }
  else
  {
    zPInfoTable.iPICreated = 1;
    zPInfoTable.pzPRec = (PINFO *)realloc(zPInfoTable.pzPRec, sizeof(PINFO));
    if (zPInfoTable.pzPRec == NULL)
		{
			InitializePInfoTable(&zPInfoTable);
		  InitializePortTable(&zPmainTable);
			return(lpfnPrintError("Insufficient Memory", 0, 0, "",  997, 0, 0, "FORWARDMATURITY1", FALSE));
		}
    zPInfoTable.pzPRec[0].iID = iID;
    zPInfoTable.pzPRec[0].lStartPosition = 0;
    zPInfoTable.iNumPI = 1;
  }

  zErr = ForwardMaturityGeneration(zPmainTable, zPInfoTable, zSATable, zCTable, sMode, sSecNo, sWi, sSecXtend, sAcctType, 
																	 lValDate - zSysSet.zSyssetng.iPaymentsStartDate, lValDate, lForwardDate);

  /* Free up the memory */
  InitializePortTable(&zPmainTable);
	InitializePInfoTable(&zPInfoTable);
	
  return zErr;

}/*GenerateForwardMaturity*/


ERRSTRUCT ForwardMaturityGeneration(PORTTABLE zPmainTable, PINFOTABLE zPInfoTable, SUBACCTTABLE zSTable, 
																	  CURRTABLE zCTable, char *sMode, char *sSecNo, char *sWi, 
																	  char *sSecXtend, char *sAcctType, long lStartDate, long lValDate, long lForwardDate)
{
  ERRSTRUCT					zErr;
  FORWARDMATSTRUCT  zFMTable;
  
  int     i, j, k, l, m, iLastIndex;
  char    sLastSecNo[13], sLastWi[2], sLastSecXtend[3], sLastAcctType[2];
  char    sFileName[90], sErrMsg[100];
  double	fUnits, fOrigFace;
  BOOL    bIncTypeFound;
	double	OpenLiability,CurrLiability = 0;
  
	lpprInitializeErrStruct(&zErr);
	zFMTable.iMatCreated = 0;
	OpenLiability = 0;
	InitializeForwardMatTable(&zFMTable);
	/* Filename for sorted file on valdate */
  if (*sMode == 'B' )
    strcpy_s(sFileName, MakePricingFileName(lForwardDate, "", "F"));
  else
    strcpy_s(sFileName, "");

  zFMTable.iMatCreated = 0;
  iLastIndex = 0;

  /* Do processing for all the branch accounts in the pinfo table */
  for (i = 0; i < zPInfoTable.iNumPI; i++)
  {
    m = -1;
    for (j = iLastIndex; j < zPmainTable.iNumPmain; j++)
    {
      if (zPmainTable.pzPmain[j].iID == zPInfoTable.pzPRec[i].iID)
      {
        m = j;
        iLastIndex = m + 1;
        break;
      }
    }
    if (m == -1)
      continue;
 
    // If the account is not set to  generate maturities automatically, nothing to do for this account.
    if (!zPmainTable.pzPmain[m].bMature)
      continue;

    // Build a table of all the records for current portfolio
    zErr = BuildForwardMatTable(sMode, sFileName, zPmainTable.pzPmain[m].iID, sSecNo,
																sWi, sSecXtend, sAcctType, lStartDate, lForwardDate,
																zPInfoTable.pzPRec[i].lStartPosition, &zFMTable);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    {
      lpfnPrintError("Forward Maturity Could Not Be Processed",zPmainTable.pzPmain[m].iID, 0, "",
										 zErr.iBusinessError, zErr.iSqlError, zErr.iIsamCode, " FORWARD MATURITY PROCESS1", FALSE);

      lpprInitializeErrStruct(&zErr);
      continue;
    }
    
    if (zFMTable.iNumMat == 0)
      continue;
		
		/* Process All the records for current branch account */
    sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = sLastAcctType[0] = '\0';
    fUnits = fOrigFace = 0;
    for (j = 0; j < zFMTable.iNumMat; j++)
    {
			// This module should mature only forwards, it can be restricted in query that's getting
			// all eligible records but this routine should check i too instead of relying on the query
		  if (strcmp(zFMTable.pzForwardMat[j].sSecXtend, "FS") != 0 &&
					strcmp(zFMTable.pzForwardMat[j].sSecXtend, "FP") != 0)
					continue;

			/*
      ** Aggregate values from all the lots for same security and create a TRANS
      ** record out of it. Pass this TRANS record to TranAlloc for processing.
      */
      if (sLastSecNo[0] == '\0' ||    /* First Time or same security as last */
          strcmp(sLastSecNo, zFMTable.pzForwardMat[j].sSecNo) == 0 &&
          strcmp(sLastWi, zFMTable.pzForwardMat[j].sWi) == 0 &&
          strcmp(sLastSecXtend, zFMTable.pzForwardMat[j].sSecXtend) == 0 &&
          strcmp(sLastAcctType, zFMTable.pzForwardMat[j].sAcctType) == 0)
      {
        fUnits += zFMTable.pzForwardMat[j].fUnits;
        fOrigFace += zFMTable.pzForwardMat[j].fOrigFace;
				OpenLiability +=zFMTable.pzForwardMat[j].fOpenliability;
				CurrLiability +=zFMTable.pzForwardMat[j].fCurliability;
        /* If first record, copy to last secno, wi, etc. */
        if (sLastSecNo[0] == '\0')
        {
          strcpy_s(sLastSecNo, zFMTable.pzForwardMat[j].sSecNo);
          strcpy_s(sLastWi, zFMTable.pzForwardMat[j].sWi);
          strcpy_s(sLastSecXtend, zFMTable.pzForwardMat[j].sSecXtend);
          strcpy_s(sLastAcctType, zFMTable.pzForwardMat[j].sAcctType);
        }
      }
      else
      {
        k = zPmainTable.pzPmain[m].iCurrIndex;

        /* Find the matching income type */
        bIncTypeFound = FALSE;
        for (l = 0; l < zSTable.iNumSAcct; l++)
        {
           if (strcmp(zSTable.zSAcct[l].sAcctType, zFMTable.pzForwardMat[j-1].sAcctType) == 0)
					 {	 
             bIncTypeFound = TRUE;
             break;
					 }
        } //for l < NumSAcct
                    
        if (!bIncTypeFound)
          lpfnPrintError("Invalid SubAccount", zPmainTable.pzPmain[m].iID, 0, "", 110, 0, 0, "MATURITY PROCESS2", FALSE);

				zErr = CreateTrans(zFMTable.pzForwardMat[j-1], fUnits, 
													 fOrigFace, zCTable.zCurrency[k].fCurrExrate,
													 zPmainTable.pzPmain[m].sAcctMethod,
													 zSTable.zSAcct[l].sXrefAcctType, lValDate,OpenLiability,CurrLiability);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        {
					
          lpfnPrintError(sErrMsg, zPmainTable.pzPmain[m].iID, 0, "", zErr.iBusinessError, 
												 zErr.iSqlError, zErr.iIsamCode, "MATURITY PROCESS3", TRUE);
          lpprInitializeErrStruct(&zErr);
        }

        /* Do a begin work for the next security */
				
        strcpy_s(sLastSecNo, zFMTable.pzForwardMat[j].sSecNo);
        strcpy_s(sLastWi, zFMTable.pzForwardMat[j].sWi);
        strcpy_s(sLastSecXtend, zFMTable.pzForwardMat[j].sSecXtend);
        strcpy_s(sLastAcctType, zFMTable.pzForwardMat[j].sAcctType);

        fUnits = zFMTable.pzForwardMat[j].fUnits;
        fOrigFace = zFMTable.pzForwardMat[j].fOrigFace;
				OpenLiability =zFMTable.pzForwardMat[j].fOpenliability;
				CurrLiability =zFMTable.pzForwardMat[j].fCurliability;
      } /* Different security */
    } /* for j < iNumMat */ 

    /* 
    ** If there was atleast one record in the table, and if the last record in 
    ** the table is for same security as its previous record, then for this 
    ** security TranAlloc must not have been called yet. If that's the case call
    ** tranalloc. When the control comes here, j is equal to zMTable.iNumMat, 
    ** so reduce it by 1 to check last record.
    */
    if (j > 0)
    {
      j--;
      if (strcmp(sLastSecNo, zFMTable.pzForwardMat[j].sSecNo) == 0 &&
          strcmp(sLastWi, zFMTable.pzForwardMat[j].sWi) == 0 &&
          strcmp(sLastSecXtend, zFMTable.pzForwardMat[j].sSecXtend) == 0 &&
          strcmp(sLastAcctType, zFMTable.pzForwardMat[j].sAcctType) == 0)
      {
        if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
        {
           k = zPmainTable.pzPmain[m].iCurrIndex;

					 /* Find the matching income type */
					 bIncTypeFound = FALSE;
					 for (l = 0; l < zSTable.iNumSAcct; l++)
					 {
							if (strcmp(zSTable.zSAcct[l].sAcctType, zFMTable.pzForwardMat[j].sAcctType) == 0)
							{
								 bIncTypeFound = TRUE;
								 break;
							}
					 } // for l < zSTable.iNumSAcct
                    
		       if (!bIncTypeFound)
					 {
						 
             lpfnPrintError("Invalid SubAccount", zPmainTable.pzPmain[m].iID,
						 							  0, "", 110, 0, 0, "MATURITY PROCESS4", TRUE);
					 }

					 zErr = CreateTrans(zFMTable.pzForwardMat[j], fUnits, 
						 																  fOrigFace, zCTable.zCurrency[k].fCurrExrate,
																						  zPmainTable.pzPmain[m].sAcctMethod,
																							zSTable.zSAcct[l].sXrefAcctType, lValDate,OpenLiability,CurrLiability);
				} // if neither sql error or business error
      } /* If Last Record(in the table) is same as previous record */
    } // if j > 0
  } /* i < numPmain */
	InitializeForwardMatTable(&zFMTable);
  return zErr;
 		
}/*ForwardMaturityGeneration*/


/**
** Function to build a memory table of records belonging to an account or a 
** specific security for an account. If the functin is running in Batch mode it
** is assumed there is a sorted file with all the records, if it is running in
** a single account mode or single security(in a single account) mode then it
** reads the MATURITY_UNLOAD cursor to get the data. 
**/
ERRSTRUCT BuildForwardMatTable(char *sMode, char *sFileName, int iID,
	char *sSecNo, char *sWi, char *sSecXtend,
	char *sAcctType, long lStartDate, long lValDate,
	long lStartPosition, FORWARDMATSTRUCT *pzFMTable)
{
	int				iLastID;
	char      sStr1[201], *sStr2;
	char *next_token1 = NULL;
	long			lEndDate;
	ERRSTRUCT zErr;
	FMATSTRUCT zTempMat;
	FILE      *fp;

	lpprInitializeErrStruct(&zErr);

	if (sMode[0] == 'B')
	{
		iLastID = 0;
		fp = fopen(sFileName, "r");
		if (fp == NULL)
			zErr = lpfnPrintError("Error Opening File", iID, 0, "", 999, 0, 0, "FUTURE MATURITY BUILDTABLE1", FALSE);

		/* Go directly where the account is starting */
		if (fseek(fp, 0L, 0) < 0)//lStartPosition
			zErr = lpfnPrintError("Error Seeking File", iID, 0, "", 999, 0, 0, "FUTURE MATURITY BUILDTABLE2", FALSE);

		/*
		** Read records from the file. For the record size, we need to give the size
		** of the biggest record, because fgets stops as soon as it gets a new line
		** character(or has read the given number of character, whichever is first),
		** so use 200 as the size, even though the length of records will be much
		** lower than that.
		*/
		while ((zErr.iSqlError == 0 && zErr.iBusinessError == 0) && (fgets(sStr1, 200, fp) != NULL))
		{
			InitializeForwardMatStruct(&zTempMat);

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
			if (atoi(sStr2) != iID)
			{
				if (iLastID != 0)
					break;
				else
					continue;
			}
			zTempMat.iID = atoi(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempMat.sAcctType, sStr2);


			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fClosePrice = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fCurExrate = atof(sStr2);


			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fCurIncExrate = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fCurliability = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempMat.sCurrId, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.lExpDate = atol(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempMat.sIncCurrId, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fOpenliability = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fOrigFace = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.iSecID = atoi(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempMat.sSecNo, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.iSecType = atoi(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempMat.sSecXtend, sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fTrdUnit = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.lTransNo = atol(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			zTempMat.fUnits = atof(sStr2);

			sStr2 = strtok_s(NULL, "|", &next_token1);
			strcpy_s(zTempMat.sWi, sStr2);

			/* Add the record to the given table */
			zErr = AddRecordToForwardMatTable(pzFMTable, zTempMat);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				break;

			iLastID = zTempMat.iID;
		} /* while not(eof) */

		fclose(fp);

	} /* if batch mode */
	else
	{
		lEndDate = GetPaymentsEndingDate(lValDate);
		while (zErr.iSqlError == 0)
		{
			InitializeForwardMatStruct(&zTempMat);
			lpprForwardMaturityUnload(&zTempMat, lStartDate, lEndDate, sMode, iID, sSecNo,
				sWi, sSecXtend, sAcctType, &zErr);
			if (zErr.iSqlError == SQLNOTFOUND)
			{
				zErr.iSqlError = 0;
				break;
			}
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			{
				zErr = lpfnPrintError("Error Fetching Unload Cursor", 0, 0, "", 0,
					zErr.iSqlError, zErr.iIsamCode, "MATURITY BUILDTABLE3", FALSE);
				break;
			}
			/* Add the record to the given table */
			zErr = AddRecordToForwardMatTable(pzFMTable, zTempMat);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				break;
		} /* while no error */
	} // not batch mode

	return zErr;
} /* BuildMatTable */


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
ERRSTRUCT ForwardMaturityUnloadAndSort(long lValDate, long lStartDate)
{
	ERRSTRUCT zErr;
	char      sFName[90] /*sFName2[90]*/, sUnlStr[300];
	FMATSTRUCT zTempMat;
	FILE      *fp;
	long			lEndDate;

	lpprInitializeErrStruct(&zErr);

	strcpy_s(sFName, MakePricingFileName(lValDate, "", "F"));
	/* Create and open file for writing */
	fp = fopen(sFName, "w");
	if (fp == NULL)
		zErr = lpfnPrintError("Error Opening File", 0, 0, "", 999, 0, 0, "FORWARD MATURITY UNLOAD1", FALSE);

	lEndDate = GetPaymentsEndingDate(lValDate);
	/* Fetch all the records and write them to the file */
	while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
	{
		memset(&zTempMat, 0, sizeof(FMATSTRUCT));
		lpprForwardMaturityUnload(&zTempMat, lStartDate, lEndDate, "", 0, "", "", "", "", &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		{
			zErr = lpfnPrintError("Error Fetching Unload Cursor", 0, 0, "", 0,
				zErr.iSqlError, zErr.iIsamCode, "FORWARD MATURITY FETCHUNLOAD", FALSE);
			break;
		}
		/* Get all the fields */
		sprintf_s(sUnlStr, "%d|%s|%f|%f|%f|%f|%s|%ld|%s|%f|%f|%d|%s|%d|%s|%f|%ld|%f|%s|\n",
			zTempMat.iID, zTempMat.sAcctType, zTempMat.fClosePrice, zTempMat.fCurExrate, zTempMat.fCurIncExrate,
			zTempMat.fCurliability, zTempMat.sCurrId, zTempMat.lExpDate, zTempMat.sIncCurrId,
			zTempMat.fOpenliability, zTempMat.fOrigFace, zTempMat.iSecID, zTempMat.sSecNo,
			zTempMat.iSecType, zTempMat.sSecXtend, zTempMat.fTrdUnit, zTempMat.lTransNo, zTempMat.fUnits,
			zTempMat.sWi);
		/* Write the string to the file) */
		if (fputs(sUnlStr, fp) == EOF)
			zErr = lpfnPrintError("Error Writing To The File", 0, 0, "", 999, 0, 0, "FORWARD MATURITY UNLOAD2", FALSE);
	} /* while no error */
	fclose(fp);

	return zErr;
} /* MaturityUnloadAndSort */


/**
** Function to create a trans record from a MatStruct record
**/
ERRSTRUCT CreateTrans(FMATSTRUCT zFMat, double fUnits, double fOrigFace, double fBaseCurrExrate, char *sAcctMethod, 
                      char *sIncAcctType, long lValDate,double OpenLiability,double CurrLiability)
{
	char			 sTemp[30];
	long			 lBusinessValDate;
	long			 lTrdDate,lPriceDate; 
  int        j;
  ERRSTRUCT  zErr;
  TRANS      zTrans;
  ASSETS     zAssets;
  DTRANSDESC zDTDesc[1];
  BOOL       bShort = FALSE;
  TRANTYPE   zTType;
  

  lpprInitializeErrStruct(&zErr);
	lBusinessValDate = GetPaymentsEndingDate(lValDate);

  if (fBaseCurrExrate <= 0.0)
    return(lpfnPrintError("Invalid Exrate", 0, 0, "", 67, 0, 0, "MATURITY CREATEATRANS1", FALSE));
	
  /* First Create Trans Record */
	lpprInitializeTransStruct(&zTrans);
  zTrans.iID = zFMat.iID;
  zTrans.lTransNo = zFMat.lTransNo;
  strcpy_s(zTrans.sSecNo, zFMat.sSecNo);
  strcpy_s(zTrans.sWi, zFMat.sWi);
  strcpy_s(zTrans.sSecXtend, zFMat.sSecXtend);
  strcpy_s(zTrans.sAcctType, zFMat.sAcctType);
  
	zTrans.iSecID = zFMat.iSecID;
  
  if (strcmp(zFMat.sSecXtend,"FS") == 0)
    bShort = TRUE;
  else if (strcmp(zFMat.sSecXtend,"FP") == 0)
    bShort = FALSE;

	zTrans.fUnits = fUnits;
	if (bShort)
	{
	  if (fUnits < 0)
		{
			lpprSelectTranType(&zTType, "CF", "DR", &zErr);
			strcpy_s(zTrans.sTranType, "CF");
			strcpy_s(zTrans.sDrCr, "DR");
		}
		else 
		{
			lpprSelectTranType(&zTType, "CF", "CR", &zErr);
			strcpy_s(zTrans.sTranType, "CF");
			strcpy_s(zTrans.sDrCr, "CR");
		}
	}
  else
	{
	  if (fUnits < 0)
		{
			lpprSelectTranType(&zTType, "FC", "CR", &zErr);
			strcpy_s(zTrans.sTranType, "FC");
			strcpy_s(zTrans.sDrCr, "CR");
		}
		else
		{
			lpprSelectTranType(&zTType, "FC", "DR", &zErr);
			strcpy_s(zTrans.sTranType, "FC");
			strcpy_s(zTrans.sDrCr, "DR");
		}
	}
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		 return zErr;
  zTrans.fOrigFace = fOrigFace;

  if (fUnits < 0) 
	{
		zTrans.fUnits = fUnits * -1;
		zTrans.fTotCost	=	CurrLiability * -1;
	}
	else
	  zTrans.fTotCost	=	CurrLiability;
  
  if (zFMat.fTrdUnit!= 0 && fUnits !=0)
		zTrans.fOpenUnitCost  = OpenLiability/(fUnits * zFMat.fTrdUnit);   

  /*zTrans.fPcplAmt		=		zFMat.fCurliability - zFMat.fOpenliability;
	zTrans.fOrigCost	=		zFMat.fOpenliability;
	zTrans.fTotCost		=		zFMat.fCurliability;*/


	lpprSelectStarsDate(&lTrdDate,&lPriceDate,&zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		 return zErr;
  
	zTrans.lTrdDate = zFMat.lExpDate;
  zTrans.lStlDate = zFMat.lExpDate;
  zTrans.lEffDate = zFMat.lExpDate;
  zTrans.lEntryDate = lTrdDate;

  strcpy_s(zTrans.sCurrId, zFMat.sCurrId);
  strcpy_s(zTrans.sCurrAcctType, zFMat.sAcctType);

  strcpy_s(zTrans.sIncCurrId, zFMat.sIncCurrId);
  strcpy_s(zTrans.sIncAcctType, zFMat.sAcctType);

  strcpy_s(zTrans.sSecCurrId, zFMat.sCurrId);
  strcpy_s(zTrans.sAccrCurrId, zFMat.sIncCurrId);
  if (!IsValueZero(fBaseCurrExrate,XRATEPERCISION))
	{
		zTrans.fBaseXrate = zFMat.fCurExrate / fBaseCurrExrate;
		zTrans.fIncBaseXrate = zFMat.fCurIncExrate / fBaseCurrExrate;
	}

  zTrans.fSecBaseXrate = zTrans.fBaseXrate;
  zTrans.fAccrBaseXrate = zTrans.fIncBaseXrate;
  zTrans.fSysXrate = zFMat.fCurExrate;
  zTrans.fIncSysXrate = zFMat.fCurIncExrate;

  strcpy_s(zTrans.sAcctMthd, sAcctMethod);
  strcpy_s(zTrans.sTransSrce, "P");
  strcpy_s(zTrans.sCreatedBy, "PRICING");
  zTrans.lPostDate = lTrdDate;
	
	_strdate(sTemp);
	lpfnrstrdate(sTemp, &(zTrans.lCreateDate));
	_strtime(zTrans.sCreateTime);

  /* 
  ** Now Create Asset Record, don't have to worry about fields not required
  ** by TranAlloc.
  */
  strcpy_s(zAssets.sSecNo, zFMat.sSecNo);
  strcpy_s(zAssets.sWhenIssue, zFMat.sWi);
  zAssets.iSecType = zFMat.iSecType;
  zAssets.fTradUnit = zFMat.fTrdUnit;
  strcpy_s(zAssets.sCurrId, zFMat.sCurrId);
  strcpy_s(zAssets.sIncCurrId, zFMat.sIncCurrId);
  zAssets.fCurExrate = zFMat.fCurExrate;
  zAssets.fCurIncExrate = zFMat.fCurIncExrate;

  j = FindSecType(zSTTable, zAssets.iSecType);
  if (j < 0)
    return(lpfnPrintError("Invalid Sec Type", zFMat.iID, 0, "", 18, 0, 0, "MATURITY CREATETRANS3", FALSE));

	// for forward transactions
	if (zFMat.lExpDate > lBusinessValDate)
	{
		zErr = CreateAndInsertFWTrans(zTrans,"");
	
	}
	else // for active transactions
        // TranAlloc (...TRUE) allows usage of DB Transactions
		zErr = lpfnTranAlloc(&zTrans, zTType, zSTTable.zSType[j], zAssets, 
												  zDTDesc, 0, NULL, "C", TRUE); //FALSE );

  return zErr;
} /* CreateTrans */


void InitializeForwardMatStruct(FMATSTRUCT* pzTempMat)
{
	strcpy_s(pzTempMat->sAcctType," ");
	pzTempMat->fClosePrice		= 0;
	pzTempMat->fCurExrate			= 0;
	pzTempMat->fCurIncExrate	= 0;
	pzTempMat->fCurliability	= 0;
	strcpy_s(pzTempMat->sCurrId," ");
	pzTempMat->lExpDate				= 0;
	strcpy_s(pzTempMat->sIncCurrId," ");
	pzTempMat->iID						= 0;
	pzTempMat->fOpenliability = 0;
	pzTempMat->fOrigFace			= 0;
	pzTempMat->iSecID					= 0;
	strcpy_s(pzTempMat->sSecNo," ");
	pzTempMat->iSecType				= 0;
	strcpy_s(pzTempMat->sSecXtend," ");
	pzTempMat->fTrdUnit				= 0;
	pzTempMat->lTransNo				= 0;
	pzTempMat->fUnits					= 0;
	strcpy_s(pzTempMat->sWi," ");
}


ERRSTRUCT AddRecordToForwardMatTable(FORWARDMATSTRUCT *pzMTab, FMATSTRUCT zMatStruct)
{
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  if (pzMTab->iMatCreated == pzMTab->iNumMat)
  {
    pzMTab->iMatCreated += NUMMATRECORD;
    pzMTab->pzForwardMat = (FMATSTRUCT *)realloc(pzMTab->pzForwardMat, 
			                                        pzMTab->iMatCreated * sizeof(FMATSTRUCT));
    if (pzMTab->pzForwardMat == NULL)
      return(lpfnPrintError("Insufficient Memory", 0, 0, "", 997, 0, 0, "FARWARD MATURITY ADDMAT", FALSE));
  }

  pzMTab->pzForwardMat[pzMTab->iNumMat++] = zMatStruct;

  return zErr;
} /* AddRecordToForwardMatTable */



/*This routine frees up the allocated memory*/
void InitializeForwardMatTable(FORWARDMATSTRUCT *pzMTable)
{
  if (pzMTable->iMatCreated > 0 && pzMTable->pzForwardMat != NULL)
    free(pzMTable->pzForwardMat);
  
  pzMTable->pzForwardMat = NULL;
  pzMTable->iNumMat = pzMTable->iMatCreated = 0;
}

