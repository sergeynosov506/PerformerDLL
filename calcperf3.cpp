/**
*
* SUB-SYSTEM: calcperf
*
* FILENAME: calcperf3.ec
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
**/
// History
// 2024-03-04 J#PER13058 Fix issue w / duplicate asset on prior change - mk.
// 2024-02-12 J#PER12987 Undid change - mk.
// 2024-01-02 J#PER12987 Added logic to load historical industry levels for continuing history assets - mk.
// 2023-04-08 J#PER11602 Delete perf date when inception date moves within a month - mk.
// 2021-06-01 J#PER11638 Broke out weighted fees out from weighted fees - mk.
// 2021-03-11 J# PER 11415 Restored changes based on new FeesOut logic - mk.
// 2021-03-03 J# PER-11415 Rolled back changes - mk
// 2021-02-26 J# PER-11415 Adjustments for feesout based on perf.dlland reporting - mk.
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2020-04-17 J# PER-10655 Added CN transactions -mk.
// 2018-09-05 VI# 61942 Updated taxrate to 37 for 2018 forward -mk
// 2013-04-12 VI# 52068 Fixed sideeffect w/Calcselected -mk
// 2013-01-14 VI# 51154 Added cleanup for orphaned unitvalue entries to be marked as deleted -mk
// 2010-06-30 VI# 44433 Added calculation of estimated annual income -mk
// 2010-06-16 VI# 42903 Added TodayFeesOut into DAILYINFO - sergeyn
// 2009-04-22 VI#42310: Changed call to GetSyssettings - mk
// 2009-04-21 VI#42310: Exposed GetSysSettings - mk.
// 2007-12-20 Init pzPKey->fBegFedetaxAD to 0 in InitializePKeyStruct() - used (indirectly) taxequiv ror calc - yb. 
// 2007-11-16 Added Vendor id    - yb



#include "calcperf.h" 
static BOOL	bCPLibInitialized = FALSE;

/**
** Function to initialize calcperf library module. It declares all the cursors
** and also builds global tables for all sectypes, trantypes and currencies
** defined in the system.
**/
DllExport ERRSTRUCT GetSysSettings(void)
{
	SYSVALUES	zSysvalues;
	ERRSTRUCT	zErr;
	long		lCurrentCFStartDate, lBlankDate;

	lpprInitializeErrStruct(&zErr);
	memset(&zSysSet, sizeof(zSysSet), 0);
	lpprSelectSyssetng(&zSysSet.zSyssetng, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	lpprSelectCFStartDate(&lCurrentCFStartDate, &lBlankDate, &zErr);
	zSysSet.lCFStartDate = lCurrentCFStartDate;

	strcpy_s(zSysvalues.sName, "FlowThreshold");
	lpprSelectSysvalues(&zSysvalues, &zErr);
	if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
	{
		zSysSet.fFlowThreshold = atof(zSysvalues.sValue) / 100;
		if (zSysSet.fFlowThreshold < 0 || zSysSet.fFlowThreshold > 100) 
			zSysSet.fFlowThreshold = NAVALUE;
	}
	else if (zErr.iSqlError == SQLNOTFOUND)
	{
		zSysSet.fFlowThreshold = NAVALUE;
		lpprInitializeErrStruct(&zErr);
	}
	else 
		return zErr;

	strcpy_s(zSysvalues.sName, "GainLossTaxAdjustment");
	lpprSelectSysvalues(&zSysvalues, &zErr);
	if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
		zSysSet.bGLTaxAdj = (BOOL) atoi(zSysvalues.sValue);
	else if (zErr.iSqlError == SQLNOTFOUND)
		lpprInitializeErrStruct(&zErr);
	else 
		return zErr;

	strcpy_s(zSysvalues.sName, "DailyPerformance");
	lpprSelectSysvalues(&zSysvalues, &zErr);
	if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
		zSysSet.bDailyPerf = (BOOL) atoi(zSysvalues.sValue);
	else if (zErr.iSqlError == SQLNOTFOUND)
		lpprInitializeErrStruct(&zErr);
	else 
		return zErr;

	strcpy_s(zSysSet.sSysCountry, "USA"); // default country
	strcpy_s(zSysvalues.sName, "SystemCountry");
	lpprSelectSysvalues(&zSysvalues, &zErr);
	if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
		strcpy_s(zSysSet.sSysCountry, zSysvalues.sValue);
	else if (zErr.iSqlError == SQLNOTFOUND)
		lpprInitializeErrStruct(&zErr);
	else 
		return zErr;

	strcpy_s(zSysvalues.sName, "InsertBatchSize");
	lpprSelectSysvalues(&zSysvalues, &zErr);
	if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
		zSysSet.iInsertBatchSize = atoi(zSysvalues.sValue);
	else if (zErr.iSqlError == SQLNOTFOUND)
		lpprInitializeErrStruct(&zErr);
	else 
		return zErr;

	return zErr;
}

ERRSTRUCT InitializeCalcPerfLibrary()
{
	ERRSTRUCT		zErr;
	int				iIndex;

	PARTTRANTYPE	zPTType;           
	PARTCURRENCY	zPCurrency;
	COUNTRY			zCountry;
	PTMPHDR			zTHeader;
	PTMPDET			zTDetail;

	lpprInitializeErrStruct(&zErr);

	if (bCPLibInitialized)
		return zErr;

	zErr = GetSysSettings();
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return(lpfnPrintError("Error in Sysvalues or SysSettings",0, 0, "", 996, 0, 0, "CALCPERF InitializeCalcPerfLibrary", FALSE));

	zTHdrDetTable.iTHdrDetCreated = 0;
	InitializePTmpHdrDetTable(&zTHdrDetTable);
	g_zSHdrDetTable.iSHdrDetCreated = 0;

	//InitializePScrHdrDetTable(&g_zSHdrDetTable);

	while (TRUE)
	{
		lpprSelectAllPartTrantype(&zPTType, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else if (zTTypeTable.iNumTType == NUMTRANTYPE)
			return(lpfnPrintError("TranType Table Too Small",0, 0, "", 996, 0, 0, "CALCPERF INITLIB2", FALSE));
		else
			zTTypeTable.zTType[zTTypeTable.iNumTType] = zPTType;

		zTTypeTable.iNumTType++;
	}	// get all trantype

	zErr = FillPartSectypeTable(&zPSTypeTable);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	while (TRUE)
	{
		lpprSelectAllPartCurrencies(&zPCurrency, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else if (zCurrTable.iNumCurrency == NUMCURRENCY)
			return(lpfnPrintError("Currency Table Too Small", 0, 0, "", 996, 0, 0, "CALCPERF INITLIB6", FALSE));
		else
			zCurrTable.zCurrency[zCurrTable.iNumCurrency] = zPCurrency;

		zCurrTable.iNumCurrency++;
	} // get all currencies

	while (TRUE)
	{
		lpprSelectAllCountries(&zCountry, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else if (zCountryTable.iNumCountry == NUMCOUNTRY)
			return(lpfnPrintError("Country Table Too Small", 0, 0, "", 997, 0, 0, "CALCPERF INITLIB7", FALSE));
		else
			zCountryTable.zCountry[zCountryTable.iNumCountry] = zCountry;

		zCountryTable.iNumCountry++;
	} // get all countries

	// Get all script header and detail
	zErr = FillScriptHeaderDetailTable(&g_zSHdrDetTable, FALSE);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	while (TRUE)
	{
		InitializePerfTmpHdr(&zTHeader);
		InitializePerfTmpDet(&zTDetail);

		lpprSelectAllTemplateHeaderAndDetails(&zTHeader, &zTDetail, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else
		{
			/* find the header in the table, if not there add it */
			zErr = AddTemplateHeaderToTable(&zTHdrDetTable, zTHeader, &iIndex);
			if (zErr.iBusinessError != 0)
				return zErr;

			if (zTDetail.lTmphdrNo == zTHeader.lTmphdrNo)
				/* add detail to the found/added header */
			{ 
				zErr = AddTemplateDetailToTable(&zTHdrDetTable, zTDetail, iIndex);
				if (zErr.iBusinessError != 0)
					return zErr;
			}
		} // no error in selecting record
	} // get all template headers and details


	/*
	** PerformanceType in Syssetng table can have following values
	**    M - Calculate Performance with Market Value Only
	**		I - Calculate Performance with Market Value + Accrued Interest
	**		D - Calculate Performance with Market Value + Accrued Dividends
	**    A - Calculate Performance with Market Value + Accrued Interest + Accrued Dividends
	** else - default to I
	** If this value is D or A then need to get accrued dividends (from ACCDIV and TRANS), else not
	*/
	if (strcmp(zSysSet.zSyssetng.sPerformanceType, "M") != 0 && 
		strcmp(zSysSet.zSyssetng.sPerformanceType, "I") != 0 &&
		strcmp(zSysSet.zSyssetng.sPerformanceType, "D") != 0 && 
		strcmp(zSysSet.zSyssetng.sPerformanceType, "A") != 0)
		strcpy_s(zSysSet.zSyssetng.sPerformanceType, "I");

	if (strcmp(zSysSet.zSyssetng.sPerformanceType, "D") == 0 || 
		strcmp(zSysSet.zSyssetng.sPerformanceType, "A") == 0)
		cCalcAccdiv = 'Y';
	else
		cCalcAccdiv = 'N';


	bCPLibInitialized = TRUE;

	return zErr;
} /* InitializeCalcPerfLibrary */


void CalcPerfCleanUp()
{
	InitializePTmpHdrDetTable(&zTHdrDetTable);
	InitializePScrHdrDetTable(&g_zSHdrDetTable);
	bCPLibInitialized = FALSE;
	FreePerformanceLibrary();
}

/*
** char LookupHoldName(char *sHoldingsName, char *sHoldCashName, long lDate);
** Purpose: determine which table set to use for Holdings & Holdcash
** Input:   long lDate           -- the test date
** Output:
**          char *sHoldingsName  -- Name of holdings table
**          char *sHoldCashName  -- Name of holdcash table
** Returns: char cType           -- <D>ate - tables exist for the asked date, use them
**															 -- <P>erformance temporary table (does not come from holdmap - names hardcoded in InitializeStarsIO when date = 1[12/31/1899])
**															 -- <E>rror - Error.
** Note: Will fill in sHoldingsName and sHoldCashName only if not null address,
**			 if null, caller is not interested. This is OK.
*/
char LookupHoldName(char *sHoldingsName, char *sHoldCashName, long lDate)
{
	typedef struct {
		long lAsOfDate;
		char sHoldings[STR80LEN];
		char sHoldCash[STR80LEN];
	} PARTHOLDMAP;

	static int			bFoundMap = FALSE;
	static int			iMonthMapItems = 0;
	static PARTHOLDMAP	zHoldMap[30];

	ERRSTRUCT	zErr;
	PARTHOLDMAP zTempHold;
	char		sWorkHoldings[STR80LEN], sWorkHoldCash[STR80LEN], sTemp[STR80LEN], sDataType[STR80LEN];
	char		cWorkType;
	int			i;

	lpprInitializeErrStruct(&zErr);

	// If first time, read all the holdmap records and put them in memory
	if (!bFoundMap)
	{
		while (zErr.iSqlError == 0)
		{
			lpprReadAllHoldmap(zTempHold.sHoldings, zTempHold.sHoldCash, sTemp, sTemp, sTemp, sTemp, 
							   sTemp, sDataType, &zTempHold.lAsOfDate, &zErr);
			if (zErr.iSqlError == SQLNOTFOUND)
			{
				zErr.iSqlError = 0;
				break;
			}
			else if (zErr.iSqlError == 0)
			{
				if (iMonthMapItems >= 30)
				{
					lpfnPrintError("Holdmap Table Too Small", 0, 0, "", 999, 9, 0, "CALCPERF LOOKUPHOLD1", FALSE);
					return 'E';
				}

				zHoldMap[iMonthMapItems++] = zTempHold;
			}
			else
				return 'E';

			bFoundMap = TRUE;
		} // while no sql error
	} // if holdmap has not been read in memory

	// Look for match. 
	cWorkType = 'P'; // temporary performance tables - names are hardcoded in InitializesStarsIO when date is 1(12/31/1899)
	strcpy_s(sWorkHoldings, "hdperf");
	strcpy_s(sWorkHoldCash, "hcperf");
	for (i = 0; i < iMonthMapItems; i++)
	{
		if (zHoldMap[i].lAsOfDate == lDate) // if the date we are looking for is found, we are done
		{
			cWorkType = 'D';
			strcpy_s(sWorkHoldings, zHoldMap[i].sHoldings);
			strcpy_s(sWorkHoldCash, zHoldMap[i].sHoldCash);
			break;
		}
	} // for i < iMonthMapItems

	// Copy Named output only if requested
	if (sHoldingsName != NULL)
		strcpy_s(sHoldingsName, STR80LEN, sWorkHoldings);
	if (sHoldCashName != NULL)
		strcpy_s(sHoldCashName, STR80LEN, sWorkHoldCash);

	return cWorkType;
} // LookupHoldName


/**
** Function to initialize dailyinfo structre
**/
void InitializeDailyInfo(DAILYINFO *pzDIVal)
{
	pzDIVal->lDate = 0;
	pzDIVal->fMktVal = pzDIVal->fBookValue = pzDIVal->fAccrInc = pzDIVal->fAccrDiv = 0;
	pzDIVal->fNetFlow = pzDIVal->fTodayFlow = pzDIVal->fCumFlow = pzDIVal->fWtdFlow = 0;
	pzDIVal->fIncome = pzDIVal->fTodayIncome = pzDIVal->fCumIncome = pzDIVal->fWtdInc = 0;
	pzDIVal->fFees = pzDIVal->fTodayFees = pzDIVal->fCumFees = pzDIVal->fWtdFees = 0;
	pzDIVal->fFeesOut = pzDIVal->fTodayFeesOut = pzDIVal->fCumFeesOut = pzDIVal->fWtdFeesOut = 0;
	pzDIVal->fCNFees = pzDIVal->fTodayCNFees = pzDIVal->fCumCNFees = pzDIVal->fWtdCNFees = 0;
	pzDIVal->fExchRateBase  = 1.0;
	pzDIVal->fPurchases = pzDIVal->fSales = pzDIVal->fContributions = pzDIVal->fWithdrawals = 0;
	pzDIVal->lDaysSinceNond = 0;
	pzDIVal->bPeriodEnd = pzDIVal->bGotMV = FALSE;
	pzDIVal->fEstAnnIncome = 0; pzDIVal->fFeesOut = pzDIVal->fNotionalFlow = pzDIVal->fTodayFeesOut = 0;
} /* initializedailyinfo */


/**
** Function to initialize wtddailyinfo structre
**/
void InitializeWtdDailyInfo(WTDDAILYINFO *pzWDIVal)
{
	pzWDIVal->lPerformNo = pzWDIVal->lDate = 0;
	pzWDIVal->fMktVal = pzWDIVal->fAccrInc = pzWDIVal->fAccrDiv = 0;
	pzWDIVal->fNetFlow = pzWDIVal->fWtdFlow = pzWDIVal->fIncome = 0;
	pzWDIVal->fWtdInc = pzWDIVal->fFees = pzWDIVal->fCNFees = pzWDIVal->fWtdFees = pzWDIVal->fWtdCNFees = pzWDIVal->fFeesOut = pzWDIVal->fWtdFeesOut = 0;
	pzWDIVal->lDaysSinceNond = 0;
	pzWDIVal->fGBaseRor = pzWDIVal->fGPrincipalRor = pzWDIVal->fGIncomeRor = 0;
	pzWDIVal->fNBaseRor = pzWDIVal->fNPrincipalRor = pzWDIVal->fNIncomeRor = 0;
	pzWDIVal->bPerformUsed = FALSE;
} /* InitializeWtdDailyInfo */


/**
** Function to initialize trantypetable structure
**/
void InitializeTranTypeTable(TRANTYPETABLE *pzTTypeTable)
{
	int i;

	pzTTypeTable->iNumTType = 0;
	for (i = 0; i < NUMTRANTYPE; i++)
	{
		pzTTypeTable->zTType[i].sTranType[0] = '\0';
		pzTTypeTable->zTType[i].sDrCr[0] = '\0';
		pzTTypeTable->zTType[i].sTranCode[0] = '\0';
		pzTTypeTable->zTType[i].lSecImpact = 0;
		pzTTypeTable->zTType[i].sPerfImpact[0] = '\0';
	}
} /* initializetrantypetable */


/**
** Function to initialize sectypetable structure
**/
void InitializeSecTypeTable(PARTSTYPETABLE *pzPSTypeTable)
{
	int i;

	pzPSTypeTable->iNumSType = 0;
	for (i = 0; i < NUMSECTYPES; i++)
	{
		pzPSTypeTable->zSType[i].iSecType = 0;
		pzPSTypeTable->zSType[i].sPrimaryType[0] = '\0';
		pzPSTypeTable->zSType[i].sSecondaryType[0] = '\0';
		pzPSTypeTable->zSType[i].sMktValInd[0] = '\0';
	}
} /* initializesectypetable */


/**
** Function to initialize currencytable structure
**/

void InitializeCurrencyTable(CURRENCYTABLE *pzCTable)
{
	int i;

	pzCTable->iNumCurrency = 0;
	for (i = 0; i < NUMCURRENCY; i++)
	{
		pzCTable->zCurrency[i].sCurrId[0] = '\0';
		pzCTable->zCurrency[i].sSecNo[0] = '\0';
		pzCTable->zCurrency[i].sWi[0] = '\0';
	}
} /* initializecurrencytable */


/**
** Function to initialize partialaccdiv structure
**/
void InitializePartialAccdiv(PARTACCDIV *pzPAccdiv)
{
	pzPAccdiv->iID = pzPAccdiv->lTransNo = pzPAccdiv->lDivintNo = 0;
	pzPAccdiv->sSecNo[0] = pzPAccdiv->sWi[0] = pzPAccdiv->sSecXtend[0] = '\0';
	pzPAccdiv->sAcctType[0] = pzPAccdiv->sTranType[0] = pzPAccdiv->sDrCr[0] = '\0';
	pzPAccdiv->fPcplAmt = pzPAccdiv->fIncomeAmt = 0;
	pzPAccdiv->lTrdDate = pzPAccdiv->lStlDate = pzPAccdiv->lEffDate = 0;
	pzPAccdiv->sCurrId[0] = pzPAccdiv->sCurrAcctType[0] = pzPAccdiv->sIncCurrId[0] = pzPAccdiv->sIncAcctType[0] = '\0';
	pzPAccdiv->fBaseXrate = pzPAccdiv->fIncBaseXrate = pzPAccdiv->fSecBaseXrate = pzPAccdiv->fAccrBaseXrate = 1.0;
} // initializepartialaccdiv


/**
** Function to initialize accdivtable
**/
void InitializeAccdivTable(ACCDIVTABLE *pzADTable)
{
	if (pzADTable->iAccdivCreated > 0 && pzADTable->pzAccdiv != NULL)
		free(pzADTable->pzAccdiv);

	pzADTable->pzAccdiv = NULL;
	pzADTable->iAccdivCreated = pzADTable->iNumAccdiv = 0;
} // initializeaccdivtable 


/**
** Function to initialize partialassets structure
**/

DllExport void InitializePartialAsset(PARTASSET2 *pzPAsset)
{
	if (pzPAsset->iDailyCount > 0 && pzPAsset->pzDAInfo != NULL)
		free(pzPAsset->pzDAInfo);

	memset(pzPAsset, 0, sizeof (*pzPAsset) - sizeof(pzPAsset->pzDAInfo));
	pzPAsset->iDailyCount = 0;
	pzPAsset->pzDAInfo = NULL;

	pzPAsset->fCurExrate = 1;
	pzPAsset->iSTypeIndex = -1;
} /* initializepartialasset */

/*
** Since the InitializePartialAsset function which frees up the dailyassetinfo is defined in performance dll,
** the memory allocation has to happen in Performance dll as well, otherwise if another dll (e.g. CreateHoldtot.dll)
** does malloc/realloc to assign memory for daily asset info and uses InitializePartialAsset function to
** free up the memory, it'll get an access violation at the free.
*/
DllExport ERRSTRUCT AllocateMemoryToDailyAssetInfo(PARTASSET2 *pzPAsset)
{
	ERRSTRUCT zErr;

	lpprInitializeErrStruct(&zErr);

	if (pzPAsset->iDailyCount <= 0)
		return zErr;

	pzPAsset->pzDAInfo = (DAILYASSETINFO *)realloc(pzPAsset->pzDAInfo, pzPAsset->iDailyCount * sizeof(DAILYASSETINFO));
	if (pzPAsset->pzDAInfo == NULL)
		return(lpfnPrintError("Insufficient Memory For DailyAssetInfo", 0, 0, "", 997, 0, 0, "CALCPERF ALLOCATEDAILYASSETMEMORY", FALSE));

	return zErr;
} /* AllocateMemoryToDailyAssetInfo

  /**
  ** Function to initialize assettable
  **/
DllExport void InitializeAssetTable(ASSETTABLE2 *pzATable)
{
	int		i;

	if (pzATable->iAssetCreated > 0 && pzATable->pzAsset != NULL)
	{
		for (i = 0; i < pzATable->iAssetCreated; i++)
			InitializePartialAsset(&pzATable->pzAsset[i]);

		free(pzATable->pzAsset);
	}

	pzATable->pzAsset = NULL;
	pzATable->iAssetCreated = pzATable->iNumAsset = 0;
} /* initializeassettable */


/**
** Function to initialize partialholding structure
**/
void InitializePartialHolding(PARTHOLDING *pzPHold)
{
	pzPHold->iID = pzPHold->sSecNo[0] = pzPHold->sWi[0] = '\0';
	pzPHold->sSecXtend[0] = '\0';
	pzPHold->lTransNo = 0;
	pzPHold->fTotCost = 0;
	pzPHold->fMktVal = 0;
	pzPHold->fMvBaseXrate = 1;
	pzPHold->fAccrInt  = 0;
	pzPHold->fAiBaseXrate = 1;
	pzPHold->fAnnualIncome = 0;

	pzPHold->bHoldCash = FALSE;
	pzPHold->iAssetIndex = -1;
} /* initializepartialholding */


/**
** Function to initialize holdingtable structure
**/
void InitializeHoldingTable(HOLDINGTABLE *pzHTable)
{
	pzHTable->lHoldDate = 0;

	if (pzHTable->iHoldingCreated > 0 && pzHTable->pzHold != NULL)
		free(pzHTable->pzHold);

	pzHTable->pzHold = NULL;
	pzHTable->iNumHolding = pzHTable->iHoldingCreated = 0;
} /* initializeholdingtable */


/**
** Function to initialize partialtrans structure
**/
void InitializePartialTrans(PARTTRANS *pzPTrans)
{
	pzPTrans->iID = '\0';
	pzPTrans->lTransNo = 0;
	pzPTrans->sTranType[0] = pzPTrans->sSecNo[0] = pzPTrans->sWi[0] = '\0';
	pzPTrans->sSecXtend[0] = '\0';
	pzPTrans->fPcplAmt = 0;
	pzPTrans->fAccrInt = pzPTrans->fIncomeAmt = pzPTrans->fPcplAmt;
	pzPTrans->fNetFlow = pzPTrans->fPcplAmt;
	pzPTrans->lTrdDate = pzPTrans->lStlDate = pzPTrans->lPerfDate = 0;
	pzPTrans->sCurrId[0] = pzPTrans->sIncCurrId[0] = '\0';

	pzPTrans->fBaseXrate = 1;
	pzPTrans->fIncBaseXrate = pzPTrans->fSecBaseXrate = pzPTrans->fBaseXrate;
	pzPTrans->fAccrBaseXrate = pzPTrans->fBaseXrate;
	pzPTrans->lEntryDate = 0;
	pzPTrans->sDrCr[0] = pzPTrans->sRevType[0] = '\0';
	pzPTrans->lTaxlotNo = pzPTrans->lTransNo = 0;
	pzPTrans->bReversal = FALSE;
	pzPTrans->iSecAssetIndex = -1;
	pzPTrans->iIncAssetIndex = pzPTrans->iTranTypeIndex = -1;
} /* InitializePartialTrans */


/**
** Function to initialize TransTable structure
**/
void InitializeTransTable(TRANSTABLE *pzTTable)
{
	if (pzTTable->iTransCreated > 0 && pzTTable->pzTrans != NULL)
		free(pzTTable->pzTrans);

	pzTTable->pzTrans = NULL;
	pzTTable->iNumTrans = pzTTable->iTransCreated = 0;
} /* initializetranstable */


/**
** Function to initialize perfrule structure
**/
DllExport void InitializePerfrule(PERFRULE *pzPrule)
{
	pzPrule->iPortfolioID = '\0';
	pzPrule->lRuleNo = 0;
	pzPrule->sCurrencyProcessingFlag[0] = pzPrule->sTotalRecordIndicator[0] = '\0';
	pzPrule->lTmphdrNo = pzPrule->lParentRuleNo = 0;
	pzPrule->lCreateDate = pzPrule->lDeleteDate = 0;
	pzPrule->sDescription[0] = pzPrule->sWeightedRecordIndicator[0] = '\0';
} /* initializeperfrule */


/**
** Function to initialize perfruletable structure
**/
DllExport void InitializePerfruleTable(PERFRULETABLE *pzPrTable)
{
	if (pzPrTable->iCapacity > 0 && pzPrTable->pzPRule != NULL)
	{
		free(pzPrTable->pzPRule);
		free(pzPrTable->piTHDIndex);
	}

	pzPrTable->pzPRule = NULL;
	pzPrTable->piTHDIndex = NULL;
	pzPrTable->iCapacity = pzPrTable->iCount = 0;
}


/**
** Function to initialize perfscriptheader structure
**/
void InitializePerfScrHdr(PSCRHDR *pzSHeader)
{
	pzSHeader->lScrhdrNo = pzSHeader->lTmphdrNo = pzSHeader->lHashKey = 0;
	pzSHeader->sOwner[0] = '\0';
	pzSHeader->lCreateDate = 0;
	pzSHeader->sCreatedBy[0]  = '\0';
	pzSHeader->sChangeable[0] = '\0';
	pzSHeader->sDescription[0] = '\0';
	pzSHeader->lChangeDate = 0;
	pzSHeader->sChangedBy[0] = '\0';
	pzSHeader->sHdrKey[0] = '\0';
} /* initializeperfscrhdr */


/**
** Function to initialize perfscriptdetail structure
**/
void InitializePerfScrDet(PSCRDET *pzSDetail)
{
	pzSDetail->lScrhdrNo = pzSDetail->lSeqNo = 0;
	pzSDetail->sSelectType[0] = pzSDetail->sComparisonRule[0] = '\0';
	pzSDetail->sBeginPoint[0] = pzSDetail->sEndPoint[0] = '\0';
	pzSDetail->sAndOrLogic[0] = pzSDetail->sIncludeExclude[0] = '\0';
	pzSDetail->sMaskRest[0] = pzSDetail->sMatchRest[0] = '\0';
	pzSDetail->sMaskWild[0] = pzSDetail->sMatchWild[0] = '\0';
	pzSDetail->sMaskExpand[0] = pzSDetail->sMatchExpand[0] = '\0';
	pzSDetail->sReportDest[0] = '\0';
	pzSDetail->lStartDate = pzSDetail->lEndDate;
} /* initializeperfscrdet */


/**
** Function to initialize perfscriptheaderdetail structure
**/
DllExport void InitializePScrHdrDet(PSCRHDRDET *pzPSHdrDet)
{
	InitializePerfScrHdr(&pzPSHdrDet->zHeader);

	if (pzPSHdrDet->iDetailCreated > 0 && pzPSHdrDet->pzDetail != NULL)
		free(pzPSHdrDet->pzDetail);

	pzPSHdrDet->iDetailCreated = pzPSHdrDet->iNumDetail = 0;
	pzPSHdrDet->pzDetail = NULL;
} /* initializePerfScrHdrDet */


/**
** Function to initialize pscrhdrdettable structure. Unlike other dynamically
** created tables, this one does not free dynamically allocated memory. Reason
** for doing this is that other initialize functions are used to initialize the
** structure as well as to free the dynamically allocated memory(at the end of
** the processing for an account) but the scrhdrdettable is a global table table
** which is never explicitely freed(when the program ends, the memory
** automatically gets freed).
**/
void InitializePScrHdrDetTable(PSCRHDRDETTABLE *pzPSHdrDetTable)
{
	int i;

	if (pzPSHdrDetTable->iSHdrDetCreated > 0 && pzPSHdrDetTable->pzSHdrDet != NULL)
	{
		for (i = 0; i < pzPSHdrDetTable->iSHdrDetCreated; i++)
			InitializePScrHdrDet(&pzPSHdrDetTable->pzSHdrDet[i]);

		free(pzPSHdrDetTable->pzSHdrDet);
	}

	pzPSHdrDetTable->iSHdrDetCreated = pzPSHdrDetTable->iNumSHdrDet = 0;
	pzPSHdrDetTable->pzSHdrDet = NULL;
} /* initializePScrHdrDetTable */


/**
** Function to initialize perftemplateheader structure
**/
DllExport void InitializePerfTmpHdr(PTMPHDR *pzTHeader)
{
	pzTHeader->lTmphdrNo = 0;
	pzTHeader->sOwner[0] = '\0';
	pzTHeader->lCreateDate = 0;
	pzTHeader->sCreatedBy[0] = '\0';
	pzTHeader->sChangeable[0] = '\0';
	pzTHeader->sDescription[0] = '\0';
	pzTHeader->lChangeDate = 0;
	pzTHeader->sChangedBy[0] = '\0';
} /* initializeperftmphdr */


/**
** Function to initialize perftemplatedetail structure
**/
DllExport void InitializePerfTmpDet(PTMPDET *pzTDetail)
{
	pzTDetail->lTmphdrNo = pzTDetail->lSeqNo = 0;
	pzTDetail->sSelectType[0] = pzTDetail->sComparisonRule[0] = '\0';
	pzTDetail->sBeginPoint[0] = pzTDetail->sEndPoint[0] = '\0';
	pzTDetail->sAndOrLogic[0] = pzTDetail->sIncludeExclude[0] = '\0';
	pzTDetail->sMaskRest[0] = pzTDetail->sMatchRest[0] = '\0';
	pzTDetail->sMaskWild[0] = pzTDetail->sMatchWild[0] = '\0';
	pzTDetail->sMaskExpand[0] = pzTDetail->sMatchExpand[0] = '\0';
	pzTDetail->sReportDest[0] = '\0';
	pzTDetail->lStartDate = pzTDetail->lEndDate;
} /* initializeperfTmpdet */


/**
** Function to initialize perftemplateheaderdetail structure
**/
void InitializePTmpHdrDet(PTMPHDRDET *pzTHdrDet)
{
	InitializePerfTmpHdr(&pzTHdrDet->zHeader);

	if (pzTHdrDet->iCapacity > 0 && pzTHdrDet->pzDetail != NULL)
		free(pzTHdrDet->pzDetail);

	pzTHdrDet->iCapacity = pzTHdrDet->iCount = 0;
	pzTHdrDet->pzDetail = NULL;
} /* initializePerftmpHdrDet */


/**
** Function to initialize ptmphdrdettable structure. Unlike other dynamically
** created tables, this one does not free dynamically allocated memory. Reason
** for doing this is that other initialize functions are used to initialize the
** structure as well as to free the dynamically allocated memory(at the end of
** the processing for an account) but the scrhdrdettable is a global table table
** which is never explicitely freed(when the program ends, the memory
** automatically gets freed).
**/
void InitializePTmpHdrDetTable(PTMPHDRDETTABLE *pzTHdrDetTable)
{
	int i;

	if (pzTHdrDetTable->iTHdrDetCreated > 0 && pzTHdrDetTable->pzTHdrDet != NULL)
	{
		for (i = 0; i < pzTHdrDetTable->iTHdrDetCreated; i++)
			InitializePTmpHdrDet(&pzTHdrDetTable->pzTHdrDet[i]);

		free(pzTHdrDetTable->pzTHdrDet);
	}

	pzTHdrDetTable->iTHdrDetCreated = pzTHdrDetTable->iNumTHdrDet = 0;
	pzTHdrDetTable->pzTHdrDet = NULL;
} /* initializePTmpHdrDetTable */


/**
** Function to initialize ALLRORS structure. ALLRORS structure is used in two
** different ways, one as the actual ROR and second as the index value(in which
** case the actual ROR can be calculated in conjunction with  another similar
** index streams). In first case the numbers are initialized as zero and in the
** second case the numbers are initialized as hundred. The second argument to
** the function, tells how to initialize the variable.
**/
void InitializeAllRors(ALLRORS *pzROR, BOOL bInitAsIndex)
{
	int   i;

	pzROR->iNumRorType = 0;
	for (i = 0; i < NUMRORTYPE_ALL; i++)
	{
		// SB 5/27/15 Simplified the structure
		InitializeRtrnset(&pzROR->fBaseRorIdx[i], bInitAsIndex);
		InitializeUnitValue(&pzROR->zUVIndex[i], bInitAsIndex, TRUE);
	}

} /* initializerorindexes */


/**
** Function to initialize PKeyStruct
**/
void InitializePKeyStruct(PKEYSTRUCT *pzPKey)
{
	pzPKey->zPK.lPerfkeyNo = 0;
	pzPKey->zPK.iID = '\0';
	pzPKey->zPK.lRuleNo = pzPKey->zPK.lScrhdrNo = 0;
	pzPKey->zPK.sCurrProc[0] = pzPKey->zPK.sTotalRecInd[0] = pzPKey->zPK.sParentChildInd[0]='\0';
	pzPKey->zPK.lParentPerfkeyNo = pzPKey->zPK.lInitPerfDate = 0;
	pzPKey->zPK.lLndPerfDate = pzPKey->zPK.lMePerfDate = pzPKey->zPK.lLastPerfDate = 0;
	pzPKey->zPK.lDeleteDate = 0;
	pzPKey->zPK.sDescription[0] = '\0';
	pzPKey->lParentRuleNo = 0;
	strcpy_s(pzPKey->sRecordType, "");
	pzPKey->iScrHDIndex = -1;
	pzPKey->fBeginMV = pzPKey->fBeginAI = pzPKey->fBeginAD = pzPKey->fBeginInc = 0;
	pzPKey->fBegFedataxAI = pzPKey->fBegFedataxAD = pzPKey->fBegFedetaxAI = pzPKey->fBegFedetaxAD = 0;
	/* state tax calcualtions have been disabled - 5/12/06 - vay
	pzPKey->fBegFedetaxAD = pzPKey->fBegStataxAI = pzPKey->fBegStataxAD = 0;
	pzPKey->fBegStetaxAI = pzPKey->fBegStetaxAD = 0;
	*/
	pzPKey->bGotBeginMVFromPerformance = FALSE;
	pzPKey->bGotBeginMVFromHoldings = FALSE;

	if (pzPKey->iDInfoCapacity > 0 && pzPKey->pzDInfo != NULL)
	{
		free(pzPKey->pzDInfo);
		if (pzPKey->pzTInfo != NULL)
			free(pzPKey->pzTInfo);
	}
	pzPKey->pzDInfo = NULL;
	pzPKey->iDInfoCapacity = pzPKey->iDInfoCount = 0;
	pzPKey->pzTInfo = NULL;

	if (pzPKey->iWDInfoCapacity > 0 && pzPKey->pzWDInfo != NULL)
		free(pzPKey->pzWDInfo);
	pzPKey->pzWDInfo = NULL;
	pzPKey->iWDInfoCapacity = pzPKey->iWDInfoCount = 0;

	pzPKey->fAbs10PrcntMV = 0.0;
	pzPKey->lScratchDate = 0;
	InitializeAllRors(&pzPKey->zBeginIndex, TRUE);
	InitializeAllRors(&pzPKey->zNewIndex, TRUE);
	pzPKey->bNewKey = pzPKey->bDeleteKey = pzPKey->bKeyCopied = pzPKey->bDeletedFromDB = FALSE;
} /* initialize pkeystruct */


/**
** Function to initialize PKeyTable
**/
void InitializePKeyTable(PKEYTABLE *pzPKeyTable)
{
	int i;

	if (pzPKeyTable->iCapacity > 0 && pzPKeyTable->pzPKey != NULL)
	{
		for (i = 0; i < pzPKeyTable->iCount; i++) /* free memory */
			InitializePKeyStruct(&pzPKeyTable->pzPKey[i]);
		free(pzPKeyTable->pzPKey);
	}

	pzPKeyTable->pzPKey = NULL;
	pzPKeyTable->iCapacity = pzPKeyTable->iCount = 0;
	//  pzPKeyTable->sTaxCalc[0] = '\0';
} /* Initializepkeytable */


/**
** Function to initialize PkeyAsset Table structure.
**/
void InitializePKeyAssetTable(PKEYASSETTABLE2 *pzPKATable)
{
	int i;

	if (pzPKATable->iKeyCount > 0 && pzPKATable->iNumAsset > 0 &&
		pzPKATable->pzStatusFlag != NULL)
	{
		for (i = 0; i < pzPKATable->iKeyCount * pzPKATable->iNumAsset; i++)
		{
			if (pzPKATable->pzStatusFlag[i].iNumDays > 0 && pzPKATable->pzStatusFlag[i].piResult != NULL)
			{
				free(pzPKATable->pzStatusFlag[i].piResult);
				pzPKATable->pzStatusFlag[i].piResult = NULL;
				pzPKATable->pzStatusFlag[i].iNumDays = 0;
			}
		} // for i < number of keys * number of assets

		free(pzPKATable->pzStatusFlag);
	}

	pzPKATable->pzStatusFlag = NULL;
	pzPKATable->iKeyCount = 0;
	pzPKATable->iNumAsset = 0;
} /* initializepkeyassettable */

/**
** Function to initialize SUMMDATA structure
**/
void InitializePerform(SUMMDATA *pzPerfVal, long lCurrentDate)
{
	pzPerfVal->iID = pzPerfVal->iPortfolioID = 0;
	pzPerfVal->fMktVal = pzPerfVal->fBookValue = pzPerfVal->fAccrInc = 0;
	pzPerfVal->fAccrDiv = pzPerfVal->fNetFlow = pzPerfVal->fCumFlow = 0;
	pzPerfVal->fWtdFlow = pzPerfVal->fPurchases = pzPerfVal->fSales = pzPerfVal->fIncome = 0;
	pzPerfVal->fCumIncome = pzPerfVal->fWtdInc = pzPerfVal->fFees = pzPerfVal->fCNFees = 0;
	pzPerfVal->fCumFees = pzPerfVal->fWtdFees = 0;
	pzPerfVal->fCumFeesOut = pzPerfVal->fWtdFeesOut = 0;
	pzPerfVal->fCumCNFees = pzPerfVal->fWtdCNFees = 0;
	pzPerfVal->fExchRateBase = 1;
	strcpy_s(pzPerfVal->sIntervalType, "MV");
	pzPerfVal->lDaysSinceNond = pzPerfVal->lDaysSinceLast = 0;
	pzPerfVal->lPerformDate = lCurrentDate;
	pzPerfVal->fCreateDate = pzPerfVal->fChangeDate = lCurrentDate;
	pzPerfVal->sPerformType[0] = '\0';
	pzPerfVal->fIncRclm = pzPerfVal->fDivRclm = pzPerfVal->fPrincipalPayDown = 0;
	pzPerfVal->fMaturity = pzPerfVal->fContribution = pzPerfVal->fWithdrawals = 0;
	pzPerfVal->fExpenses = pzPerfVal->fReceipts = pzPerfVal->fIncomeCash = 0;
	pzPerfVal->fPrincipalCash = pzPerfVal->fFeesOut = pzPerfVal->fTransfers = 0;
	pzPerfVal->fTransfersIn = pzPerfVal->fTransfersOut = pzPerfVal->fEstAnnIncome = 0;
	pzPerfVal->iCreatedBy = 0;
	pzPerfVal->fNotionalFlow = 0;
} /* initializeperform */


/**
** Function to initialize rorstruct
**/
void InitializeRorStruct(RORSTRUCT *pzRorStruct, long lDate)
{
	pzRorStruct->fBeginMV = pzRorStruct->fBeginAI = pzRorStruct->fBeginAD = 0.0;
	pzRorStruct->fBeginInc = pzRorStruct->fEndMV = pzRorStruct->fEndAI = 0.0;
	pzRorStruct->fEndAD = pzRorStruct->fEndInc = pzRorStruct->fNetFlow = 0.0;
	pzRorStruct->fWtFlow = pzRorStruct->fFees = pzRorStruct->fWtFees = pzRorStruct->fCNFees = pzRorStruct->fWtCNFees = pzRorStruct->fFeesOut = pzRorStruct->fWtFeesOut = 0.0;
	pzRorStruct->fIncome = pzRorStruct->fWtIncome = 0.0;

	pzRorStruct->fGFTWFFactor = pzRorStruct->fGFPcplFFactor = pzRorStruct->fNFTWFFactor = pzRorStruct->fCNTWFFactor = 0.0;
	pzRorStruct->fNFPcplFFactor = pzRorStruct->fIncFFactor = pzRorStruct->fGFTEFFactor = 0.0; 
	pzRorStruct->fNFTEFFactor = pzRorStruct->fGFATFFactor = pzRorStruct->fNFATFFactor = 0.0;

	pzRorStruct->fGFTWWtdFlow = pzRorStruct->fGFPcplWtdFlow = pzRorStruct->fNFTWWtdFlow = pzRorStruct->fCNTWWtdFlow = 0.0;
	pzRorStruct->fNFPcplWtdFlow = pzRorStruct->fIncWtdFlow = pzRorStruct->fGFTEWtdFlow = 0.0;
	pzRorStruct->fNFTEWtdFlow = pzRorStruct->fGFATWtdFlow = pzRorStruct->fNFATWtdFlow = 0.0;

	InitializePorttax(&pzRorStruct->zPTax, lDate);
	InitializeTaxinfo(&pzRorStruct->zTInfo);

	pzRorStruct->bTotalPortfolio = pzRorStruct->bInceptionRor = FALSE;
	pzRorStruct->bTerminationRor = FALSE;
	pzRorStruct->bCalcNetForSegments = FALSE;
	pzRorStruct->iReturnstoCalculate = 0;
	//strcpy_s(pzRorStruct->sTaxCalc, "");
	InitializeAllRors(&pzRorStruct->zAllRor, FALSE);
} /* initializerorstruct */


/**
** Function to initialize dwrorstruct
**/
void InitializeDWRorStruct(DWRORSTRUCT *pzDWRor)
{
	pzDWRor->fEndMV = pzDWRor->fEndAI = pzDWRor->fEndAD = pzDWRor->fEndInc = 0;
	if (pzDWRor->iNumFlows > 0 && pzDWRor->pfNetFlow != NULL &&
		pzDWRor->pfIncome != NULL)
	{
		free(pzDWRor->pfNetFlow);
		free(pzDWRor->pfIncome);
		free(pzDWRor->pfWeight);
	}
	pzDWRor->pfNetFlow = NULL;
	pzDWRor->pfIncome = NULL;
	pzDWRor->pfWeight = NULL;
	pzDWRor->iNumFlows = 0;

	pzDWRor->bTotalPortfolio = FALSE;
	pzDWRor->fBaseRor = pzDWRor->fIncomeRor = 0;
	//pzDWRor->fMRor = pzDWRor->fIRor = pzDWRor->fDRor = pzDWRor->fARor = pzDWRor->fNRor = 0;
} /* Initializedwror */


/**
** function to initialize RTRNSET structure
**/
void InitializeRtrnset(double *pfRorIdx, BOOL bInitAsIndex)
{
	double  fTempDouble;

	fTempDouble = (bInitAsIndex > 0) ? 100 : 0;          

	// Sb 5/27/15 Simplified the structure
	*pfRorIdx = fTempDouble;
	/*	pzRor->fPrincipalIdx = pzRor->fIncomeIdx = pzRor->fBaseRorIdx = fTempDouble;
	pzRor->fLocalRorIdx = pzRor->fCurrRorIdx = pzRor->fFedtaxRorIdx = fTempDouble;
	pzRor->fFedfreeRorIdx = pzRor->fStatetaxRorIdx = fTempDouble;
	pzRor->fStatefreeRorIdx = pzRor->fAtaxRorIdx = pzRor->fEtaxRorIdx = fTempDouble;*/
} // initializertrnset 

/**
** function to initialize UNITVALUE structure
**/
void InitializeUnitValue(UNITVALUE *pzUV, BOOL bInitAsIndex, BOOL bFullInit)
{
	if (bFullInit)
		memset(pzUV, 0, sizeof(UNITVALUE));

	pzUV->lStreamBeginDate = 0;
	pzUV->fUnitValue = (bInitAsIndex > 0) ? 100 : 0;          
} // initializeunitvalue



/**
** function to initialize structure which is used to store the intermediate reuslts of script/template
** details . This structure is used only if there is atleast one parenthesis in the scrip/template.
** For simple(w/o any parenthesis) scripts/templates, TestAsset function itself figures out the
** result, it does not need to store intermediate results anywhere.
*/
void InitializeResultList(RESULTLIST *pzList)
{
	if (pzList->iCapacity > 0 && pzList->sItem != NULL)
		free(pzList->sItem);

	pzList->sItem = NULL;
	pzList->iCapacity = pzList->iCount = 0;
}

/**
** Function to initialize taxperf structure
**/
void InitializeTaxperf(TAXPERF *pzTaxperf)
{
	memset(pzTaxperf, 0, sizeof(*pzTaxperf));
	pzTaxperf->fExchRateBase = pzTaxperf->fExchRateSys = 1.0;
} // initializetaxperf 


/**
** Function to initialize dailytaxinfo structure
**/
void InitializeDailyTaxinfo(DAILYTAXINFO *pzTaxinfo)
{
	memset(pzTaxinfo, 0, sizeof(*pzTaxinfo));
	pzTaxinfo->fExchRateBase = pzTaxinfo->fExchRateSys = 1;
} /* initializedailytaxinfo */

void CreateTaxperfFromDailyTaxInfo(DAILYTAXINFO zTI, int iPortfolioID, int iID, 
								   long lPerfDate, TAXPERF *pzTaxperf)
{
	InitializeTaxperf(pzTaxperf);

	pzTaxperf->iPortfolioID			= iPortfolioID;
	pzTaxperf->iID					= iID;
	pzTaxperf->lPerformDate			= lPerfDate;
	pzTaxperf->fFedinctaxWthld		= RoundDouble(zTI.fFedinctaxWthld, 2);
	pzTaxperf->fCumFedinctaxWthld	= RoundDouble(zTI.fCumFedinctaxWthld, 2);
	pzTaxperf->fWtdFedinctaxWthld	= RoundDouble(zTI.fWtdFedinctaxWthld, 2);

	/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	pzTaxperf->fStinctaxWthld		= RoundDouble(zTI.fStinctaxWthld, 2);
	pzTaxperf->fCumStinctaxWthld	= RoundDouble(zTI.fCumStinctaxWthld, 2);
	pzTaxperf->fWtdStinctaxWthld	= RoundDouble(zTI.fWtdStinctaxWthld, 2);
	*/
	pzTaxperf->fFedtaxRclm			= RoundDouble(zTI.fFedtaxRclm, 2);
	pzTaxperf->fCumFedtaxRclm		= RoundDouble(zTI.fCumFedtaxRclm, 2);
	pzTaxperf->fWtdFedtaxRclm		= RoundDouble(zTI.fWtdFedtaxRclm, 2);

	/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	pzTaxperf->fSttaxRclm			= RoundDouble(zTI.fSttaxRclm, 2);
	pzTaxperf->fCumSttaxRclm		= RoundDouble(zTI.fCumSttaxRclm, 2);
	pzTaxperf->fWtdSttaxRclm		= RoundDouble(zTI.fWtdSttaxRclm, 2);
	*/

	pzTaxperf->fFedetaxInc			= RoundDouble(zTI.fFedetaxInc, 2);
	pzTaxperf->fCumFedetaxInc		= RoundDouble(zTI.fCumFedetaxInc, 2);
	pzTaxperf->fWtdFedetaxInc		= RoundDouble(zTI.fWtdFedetaxInc, 2);
	pzTaxperf->fFedataxInc			= RoundDouble(zTI.fFedataxInc, 2);
	pzTaxperf->fCumFedataxInc		= RoundDouble(zTI.fCumFedataxInc, 2);
	pzTaxperf->fWtdFedataxInc		= RoundDouble(zTI.fWtdFedataxInc, 2);

	/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	pzTaxperf->fStetaxInc			= RoundDouble(zTI.fStetaxInc, 2);
	pzTaxperf->fCumStetaxInc		= RoundDouble(zTI.fCumStetaxInc, 2);
	pzTaxperf->fWtdStetaxInc		= RoundDouble(zTI.fWtdStetaxInc, 2);
	pzTaxperf->fStataxInc			= RoundDouble(zTI.fStataxInc, 2);
	pzTaxperf->fCumStataxInc		= RoundDouble(zTI.fCumStataxInc, 2);
	pzTaxperf->fWtdStataxInc		= RoundDouble(zTI.fWtdStataxInc, 2);
	*/
	pzTaxperf->fFedetaxStrgl		= RoundDouble(zTI.fFedetaxStrgl, 2);
	pzTaxperf->fCumFedetaxStrgl		= RoundDouble(zTI.fCumFedetaxStrgl, 2);
	pzTaxperf->fWtdFedetaxStrgl		= RoundDouble(zTI.fWtdFedetaxStrgl, 2);
	pzTaxperf->fFedetaxLtrgl		= RoundDouble(zTI.fFedetaxLtrgl, 2);
	pzTaxperf->fCumFedetaxLtrgl		= RoundDouble(zTI.fCumFedetaxLtrgl, 2);
	pzTaxperf->fWtdFedetaxLtrgl		= RoundDouble(zTI.fWtdFedetaxLtrgl, 2);
	pzTaxperf->fFedetaxCrrgl		= RoundDouble(zTI.fFedetaxCrrgl, 2);
	pzTaxperf->fCumFedetaxCrrgl		= RoundDouble(zTI.fCumFedetaxCrrgl, 2);
	pzTaxperf->fWtdFedetaxCrrgl		= RoundDouble(zTI.fWtdFedetaxCrrgl, 2);
	pzTaxperf->fFedataxStrgl		= RoundDouble(zTI.fFedataxStrgl, 2);
	pzTaxperf->fCumFedataxStrgl		= RoundDouble(zTI.fCumFedataxStrgl, 2);
	pzTaxperf->fWtdFedataxStrgl		= RoundDouble(zTI.fWtdFedataxStrgl, 2);
	pzTaxperf->fFedataxLtrgl		= RoundDouble(zTI.fFedataxLtrgl, 2);
	pzTaxperf->fCumFedataxLtrgl		= RoundDouble(zTI.fCumFedataxLtrgl, 2);
	pzTaxperf->fWtdFedataxLtrgl		= RoundDouble(zTI.fWtdFedataxLtrgl, 2);
	pzTaxperf->fFedataxCrrgl		= RoundDouble(zTI.fFedataxCrrgl, 2);
	pzTaxperf->fCumFedataxCrrgl		= RoundDouble(zTI.fCumFedataxCrrgl, 2);
	pzTaxperf->fWtdFedataxCrrgl		= RoundDouble(zTI.fWtdFedataxCrrgl, 2);

	/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	pzTaxperf->fStetaxStrgl			= RoundDouble(zTI.fStetaxStrgl, 2);
	pzTaxperf->fCumStetaxStrgl		= RoundDouble(zTI.fCumStetaxStrgl, 2);
	pzTaxperf->fWtdStetaxStrgl		= RoundDouble(zTI.fWtdStetaxStrgl, 2);
	pzTaxperf->fStetaxLtrgl			= RoundDouble(zTI.fStetaxLtrgl, 2);
	pzTaxperf->fCumStetaxLtrgl		= RoundDouble(zTI.fCumStetaxLtrgl, 2);
	pzTaxperf->fWtdStetaxLtrgl		= RoundDouble(zTI.fWtdStetaxLtrgl, 2);
	pzTaxperf->fStetaxCrrgl			= RoundDouble(zTI.fStetaxCrrgl, 2);
	pzTaxperf->fCumStetaxCrrgl		= RoundDouble(zTI.fCumStetaxCrrgl, 2);
	pzTaxperf->fWtdStetaxCrrgl		= RoundDouble(zTI.fWtdStetaxCrrgl, 2);
	pzTaxperf->fStataxStrgl			= RoundDouble(zTI.fStataxStrgl, 2);
	pzTaxperf->fCumStataxStrgl		= RoundDouble(zTI.fCumStataxStrgl, 2);
	pzTaxperf->fWtdStataxStrgl		= RoundDouble(zTI.fWtdStataxStrgl, 2);
	pzTaxperf->fStataxLtrgl			= RoundDouble(zTI.fStataxLtrgl, 2);
	pzTaxperf->fCumStataxLtrgl		= RoundDouble(zTI.fCumStataxLtrgl, 2);
	pzTaxperf->fWtdStataxLtrgl		= RoundDouble(zTI.fWtdStataxLtrgl, 2);
	pzTaxperf->fStataxCrrgl			= RoundDouble(zTI.fStataxCrrgl, 2);
	pzTaxperf->fCumStataxCrrgl		= RoundDouble(zTI.fCumStataxCrrgl, 2);
	pzTaxperf->fWtdStataxCrrgl		= RoundDouble(zTI.fWtdStataxCrrgl, 2);
	*/  
	pzTaxperf->fFedataxAccrInc		= RoundDouble(zTI.fFedataxAccrInc, 2);
	pzTaxperf->fFedataxAccrDiv		= RoundDouble(zTI.fFedataxAccrDiv, 2);
	pzTaxperf->fFedataxIncRclm		= RoundDouble(zTI.fFedataxIncRclm, 2);
	pzTaxperf->fFedataxDivRclm		= RoundDouble(zTI.fFedataxDivRclm, 2);
	pzTaxperf->fFedetaxAccrInc		= RoundDouble(zTI.fFedetaxAccrInc, 2);
	pzTaxperf->fFedetaxAccrDiv		= RoundDouble(zTI.fFedetaxAccrDiv, 2);
	pzTaxperf->fFedetaxIncRclm		= RoundDouble(zTI.fFedetaxIncRclm, 2);
	pzTaxperf->fFedetaxDivRclm		= RoundDouble(zTI.fFedetaxDivRclm, 2);

	/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	pzTaxperf->fStataxAccrInc		= RoundDouble(zTI.fStataxAccrInc, 2);
	pzTaxperf->fStataxAccrDiv		= RoundDouble(zTI.fStataxAccrDiv, 2);
	pzTaxperf->fStataxIncRclm		= RoundDouble(zTI.fStataxIncRclm, 2);
	pzTaxperf->fStataxDivRclm		= RoundDouble(zTI.fStataxDivRclm, 2);
	pzTaxperf->fStetaxAccrInc		= RoundDouble(zTI.fStetaxAccrInc, 2);
	pzTaxperf->fStetaxAccrDiv		= RoundDouble(zTI.fStetaxAccrDiv, 2);
	pzTaxperf->fStetaxIncRclm		= RoundDouble(zTI.fStetaxIncRclm, 2);
	pzTaxperf->fStetaxDivRclm		= RoundDouble(zTI.fStetaxDivRclm, 2);
	*/
	pzTaxperf->fExchRateBase		= pzTaxperf->fExchRateSys	= 1.0;

	pzTaxperf->fFedataxAmort		= RoundDouble(zTI.fFedataxAmort, 2);
	pzTaxperf->fCumFedataxAmort		= RoundDouble(zTI.fCumFedataxAmort, 2);
	pzTaxperf->fWtdFedataxAmort		= RoundDouble(zTI.fWtdFedataxAmort, 2);

	pzTaxperf->fFedetaxAmort		= RoundDouble(zTI.fFedetaxAmort, 2);
	pzTaxperf->fCumFedetaxAmort		= RoundDouble(zTI.fCumFedetaxAmort, 2);
	pzTaxperf->fWtdFedetaxAmort		= RoundDouble(zTI.fWtdFedetaxAmort, 2);
} // CreateTaxperfFromDailyTaxInfo


void CreateTaxinfoForAKey(PKEYTABLE zPTable, int iKey, int iDate, TAXINFO *pzTInfo)
{
	ERRSTRUCT		zErr;
	int				i, iBeginDate;
	DAILYTAXINFO	*pzDailyTI;

	lpprInitializeErrStruct(&zErr);
	InitializeTaxinfo(pzTInfo);

	pzDailyTI = &zPTable.pzPKey[iKey].pzTInfo[iDate];

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
		pzTInfo->fFedBegataxAccrInc = zPTable.pzPKey[iKey].fBegFedataxAI;
		pzTInfo->fFedBegataxAccrDiv = zPTable.pzPKey[iKey].fBegFedataxAD;
		pzTInfo->fFedBegetaxAccrInc = zPTable.pzPKey[iKey].fBegFedetaxAI;
		pzTInfo->fFedBegetaxAccrDiv = zPTable.pzPKey[iKey].fBegFedetaxAD;
	}
	else
	{
		pzTInfo->fFedBegataxAccrInc = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedataxAccrInc;
		pzTInfo->fFedBegataxAccrDiv = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedataxAccrDiv;
		pzTInfo->fFedBegetaxAccrInc = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedetaxAccrInc;
		pzTInfo->fFedBegetaxAccrDiv = zPTable.pzPKey[iKey].pzTInfo[iBeginDate].fFedetaxAccrDiv;
	}

	pzTInfo->fFedinctaxWthld 	= pzDailyTI->fFedinctaxWthld;
	pzTInfo->fWtdFedinctaxWthld	= pzDailyTI->fWtdFedinctaxWthld;
	pzTInfo->fFedtaxRclm		= pzDailyTI->fFedtaxRclm;
	pzTInfo->fWtdFedtaxRclm		= pzDailyTI->fWtdFedtaxRclm;
	pzTInfo->fFedetaxInc		= pzDailyTI->fFedetaxInc;
	pzTInfo->fWtdFedetaxInc		= pzDailyTI->fWtdFedetaxInc;
	pzTInfo->fFedataxInc 		= pzDailyTI->fFedataxInc;
	pzTInfo->fWtdFedataxInc		= pzDailyTI->fWtdFedataxInc;
	pzTInfo->fFedetaxStrgl		= pzDailyTI->fFedetaxStrgl;
	pzTInfo->fWtdFedetaxStrgl	= pzDailyTI->fWtdFedetaxStrgl;
	pzTInfo->fFedetaxLtrgl		= pzDailyTI->fFedetaxLtrgl;
	pzTInfo->fWtdFedetaxLtrgl	= pzDailyTI->fWtdFedetaxLtrgl;
	pzTInfo->fFedetaxCrrgl		= pzDailyTI->fFedetaxCrrgl;
	pzTInfo->fWtdFedetaxCrrgl	= pzDailyTI->fWtdFedetaxCrrgl;
	pzTInfo->fFedataxStrgl		= pzDailyTI->fFedataxStrgl;
	pzTInfo->fWtdFedataxStrgl	= pzDailyTI->fWtdFedataxStrgl;
	pzTInfo->fFedataxLtrgl		= pzDailyTI->fFedataxLtrgl;
	pzTInfo->fWtdFedataxLtrgl	= pzDailyTI->fWtdFedataxLtrgl;
	pzTInfo->fFedataxCrrgl		= pzDailyTI->fFedataxCrrgl;
	pzTInfo->fWtdFedataxCrrgl	= pzDailyTI->fWtdFedataxCrrgl;
	pzTInfo->fFedEndataxAccrInc	= pzDailyTI->fFedataxAccrInc;
	pzTInfo->fFedEndataxAccrDiv	= pzDailyTI->fFedataxAccrDiv;
	pzTInfo->fFedEndetaxAccrInc	= pzDailyTI->fFedetaxAccrInc;
	pzTInfo->fFedEndetaxAccrDiv	= pzDailyTI->fFedetaxAccrDiv;
	pzTInfo->fFedataxIncRclm	= pzDailyTI->fFedataxIncRclm;
	pzTInfo->fFedataxDivRclm	= pzDailyTI->fFedataxDivRclm;
	pzTInfo->fFedetaxIncRclm	= pzDailyTI->fFedetaxIncRclm;
	pzTInfo->fFedetaxDivRclm	= pzDailyTI->fFedetaxDivRclm;

	/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	pzTInfo->fStinctaxWthld 	= pzDailyTI->fStinctaxWthld;
	pzTInfo->fWtdStinctaxWthld	= pzDailyTI->fWtdStinctaxWthld;
	pzTInfo->fSttaxRclm			= pzDailyTI->fSttaxRclm;
	pzTInfo->fWtdSttaxRclm		= pzDailyTI->fWtdSttaxRclm;
	pzTInfo->fStetaxInc			= pzDailyTI->fStetaxInc;
	pzTInfo->fWtdStetaxInc		= pzDailyTI->fWtdStetaxInc;
	pzTInfo->fStataxInc 		= pzDailyTI->fStataxInc;
	pzTInfo->fWtdStataxInc		= pzDailyTI->fWtdStataxInc;
	pzTInfo->fStetaxStrgl		= pzDailyTI->fStetaxStrgl;
	pzTInfo->fWtdStetaxStrgl	= pzDailyTI->fWtdStetaxStrgl;
	pzTInfo->fStetaxLtrgl		= pzDailyTI->fStetaxLtrgl;
	pzTInfo->fWtdStetaxLtrgl	= pzDailyTI->fWtdStetaxLtrgl;
	pzTInfo->fStetaxCrrgl		= pzDailyTI->fStetaxCrrgl;
	pzTInfo->fWtdStetaxCrrgl	= pzDailyTI->fWtdStetaxCrrgl;
	pzTInfo->fStataxStrgl		= pzDailyTI->fStataxStrgl;
	pzTInfo->fWtdStataxStrgl	= pzDailyTI->fWtdStataxStrgl;
	pzTInfo->fStataxLtrgl		= pzDailyTI->fStataxLtrgl;
	pzTInfo->fWtdStataxLtrgl	= pzDailyTI->fWtdStataxLtrgl;
	pzTInfo->fStataxCrrgl		= pzDailyTI->fStataxCrrgl;
	pzTInfo->fWtdStataxCrrgl	= pzDailyTI->fWtdStataxCrrgl;
	pzTInfo->fStEndataxAccrInc	= pzDailyTI->fStataxAccrInc;
	pzTInfo->fStEndataxAccrDiv	= pzDailyTI->fStataxAccrDiv;
	pzTInfo->fStEndetaxAccrInc	= pzDailyTI->fStetaxAccrInc;
	pzTInfo->fStEndetaxAccrDiv	= pzDailyTI->fStetaxAccrDiv;
	pzTInfo->fStataxIncRclm		= pzDailyTI->fStataxIncRclm;
	pzTInfo->fStataxDivRclm		= pzDailyTI->fStataxDivRclm;
	pzTInfo->fStetaxIncRclm		= pzDailyTI->fStetaxIncRclm;
	pzTInfo->fStetaxDivRclm		= pzDailyTI->fStetaxDivRclm;
	*/
	pzTInfo->fFedataxAmort		= pzDailyTI->fFedataxAmort;
	pzTInfo->fWtdFedataxAmort	= pzDailyTI->fWtdFedataxAmort;
	pzTInfo->fFedetaxAmort		= pzDailyTI->fFedetaxAmort;
	pzTInfo->fWtdFedetaxAmort	= pzDailyTI->fWtdFedetaxAmort;
} // CreateTaxinfoForAKey

/**
** Function to initialize porttax structure
**/
void InitializePorttax(PORTTAX *pzPTax, long lDate)
{
	double fDefaultRate;
	short  iMDY[3];

	/*
	** The default taxrate is the maximum taxrate, starting from year 2002, maximum tax rate is
	** 38.6 %, for year 2001 it is 39.1 %, for years prior to 2001 it is 39.6 %
	** SB - 1/15/03 Taxrate for year 2003 is 38.1%.
	** SB - 4/9/03  Previous assumption of taxrate reduction for year 2003 was incorrect, it still
	** is 38.6%, changed the code back
	** SB/vay - 6/11/03  New bill passed, taxrate reduction for year 2003 resulted in 35% rate.
	** SB - 8/20/15 Highest tax rate effective 1/1/2013 is 39.6%
	*/
	lpfnrjulmdy(lDate, iMDY);
	if (iMDY[2] >= 2018)
		fDefaultRate = 37;
	if (iMDY[2] >= 2013)
		fDefaultRate = 39.6;
	if (iMDY[2] >= 2003)
		fDefaultRate = 35;
	else if (iMDY[2] >= 2002)
		fDefaultRate = 38.6;
	else if (iMDY[2] == 2001)
		fDefaultRate = 39.1;
	else
		fDefaultRate = 39.6;

	pzPTax->iID = 0;
	pzPTax->lTaxDate = lDate;
	pzPTax->fFedIncomeRate = fDefaultRate;
	pzPTax->fStIncomeRate = pzPTax->fStStGLRate = pzPTax->fStLtGLRate = fDefaultRate;
	pzPTax->fStockWithholdRate = pzPTax->fBondWithholdRate = 0;

	// for Gain/Loss Tax Treatment
	pzPTax->fFedStGLRate = 28.0; // max rate, also used for Currency GL

	if (iMDY[2] >= 2009)
		pzPTax->fFedLtGLRate = 20.0; // max rate for 2009 and beyond
	else
		pzPTax->fFedLtGLRate = 15.0; // max rate prior 2009 
} /* initializeporttax */


/**
** Function to initialize PorttaxTable
**/
void InitializePorttaxTable(PORTTAXTABLE *pzPTaxTable)
{
	if (pzPTaxTable->iCapacity > 0 && pzPTaxTable->pzPTax != NULL)
		free(pzPTaxTable->pzPTax);

	pzPTaxTable->iCapacity = pzPTaxTable->iCount = 0;
	pzPTaxTable->pzPTax = NULL;
} /* InitializePorttaxTable */

/**
** Function to initialize ParentRuleTable
**/
void InitializeParentRuleTable(PARENTRULETABLE *pzPTable)
{
	if (pzPTable->iCapacity > 0 && pzPTable->plPRule != NULL)
		free(pzPTable->plPRule);

	pzPTable->iCapacity = pzPTable->iCount = 0;
	pzPTable->plPRule = NULL;
} /* InitializeParentRuleTable */


void InitializeValidDatePrule(VALIDDATEPRULE *pzVDPR)
{
	pzVDPR->lStartDate = pzVDPR->lEndDate = 0;
	pzVDPR->lPerfkeyNo = pzVDPR->lPerfkeyNo = 0;
} /* InitializeValidDatePrule */

void InitializeValidDPTable(VALIDDPTABLE *pzVDPTable)
{
	if (pzVDPTable->iVDPRCreated > 0 && pzVDPTable->pzVDPR != NULL)
		free(pzVDPTable->pzVDPR);

	pzVDPTable->pzVDPR = NULL;
	pzVDPTable->iVDPRCreated = pzVDPTable->iNumVDPR = 0;
} /* InitializeValidDPTable */

/**
** Function to initialize taxinfo structure
**/
void InitializeTaxinfo(TAXINFO *pzTaxinfo)
{
	memset(pzTaxinfo, 0, sizeof(*pzTaxinfo));

} // initializetaxinfo

void InitializePerfAssetMerge(PERFASSETMERGE *pzPAMerge)
{
	memset(pzPAMerge, 0, sizeof(*pzPAMerge));
	pzPAMerge->lEndDate = 2958464; //12/30/9999
	pzPAMerge->iFromSecNoIndex = pzPAMerge->iToSecNoIndex = -1;
} //InitializePerfAssetMerge

void InitializePerfAssetMergeTable(PERFASSETMERGETABLE *pzAMTable)
{
	if (pzAMTable->iCapacity > 0 && pzAMTable->pzMergedAsset != NULL)
		free(pzAMTable->pzMergedAsset);

	pzAMTable->pzMergedAsset = NULL;
	pzAMTable->iCount = pzAMTable->iCapacity = 0;
} //InitializeAssetMergeTable


void CopyPartTransToTrans(PARTTRANS zPT, TRANS *pzTR)
{
	lpprInitTrans(pzTR);

	pzTR->iID =  zPT.iID;
	pzTR->lTransNo = zPT.lTransNo;
	strcpy_s(pzTR->sTranType, zPT.sTranType);
	strcpy_s(pzTR->sSecNo, zPT.sSecNo);
	strcpy_s(pzTR->sWi, zPT.sWi);
	strcpy_s(pzTR->sSecXtend, zPT.sSecXtend);
	strcpy_s(pzTR->sAcctType, zPT.sAcctType);
	pzTR->fPcplAmt = zPT.fPcplAmt;
	pzTR->fAccrInt = zPT.fAccrInt;
	pzTR->fIncomeAmt = zPT.fIncomeAmt;
	pzTR->fNetFlow = zPT.fNetFlow;
	pzTR->lTrdDate = zPT.lTrdDate;
	pzTR->lStlDate = zPT.lStlDate;
	pzTR->lEntryDate = zPT.lEntryDate;
	pzTR->lPerfDate = zPT.lPerfDate;
	strcpy_s(pzTR->sXSecNo, zPT.sXSecNo);
	strcpy_s(pzTR->sXWi, zPT.sXWi);
	strcpy_s(pzTR->sXSecXtend, zPT.sXSecXtend);
	strcpy_s(pzTR->sXAcctType, zPT.sXAcctType);
	strcpy_s(pzTR->sCurrId, zPT.sCurrId);
	strcpy_s(pzTR->sCurrAcctType, zPT.sCurrAcctType);
	strcpy_s(pzTR->sIncCurrId, zPT.sIncCurrId);
	strcpy_s(pzTR->sXCurrId, zPT.sXCurrId);
	strcpy_s(pzTR->sXCurrAcctType, zPT.sXCurrAcctType);
	pzTR->fBaseXrate = zPT.fBaseXrate;
	pzTR->fIncBaseXrate = zPT.fIncBaseXrate;
	pzTR->fSecBaseXrate = zPT.fSecBaseXrate;
	pzTR->fAccrBaseXrate = zPT.fAccrBaseXrate;
	strcpy_s(pzTR->sDrCr, zPT.sDrCr);
	pzTR->lTaxlotNo = zPT.lTaxlotNo;

	pzTR->fTotCost = zPT.fTotCost;
	pzTR->fOptPrem = zPT.fOptPrem;
	pzTR->fBaseOpenXrate = zPT.fBaseOpenXrate;
	pzTR->lEffDate = zPT.lEffDate;
	pzTR->lOpenTrdDate = zPT.lOpenTrdDate;
	strcpy_s(pzTR->sGlFlag, zPT.sGLFlag);
	pzTR->lRevTransNo = zPT.lRevTransNo;
	pzTR->fUnits = zPT.fUnits;

} //CopyPartTransToTrans


/**
** This function is used to select a record(by secno and whenissue) from assets
** table. When the asset is found, this function points iSTypeIndex to right
** element in SecType array in memory.
**/
ERRSTRUCT SelectAsset(char *sSecNo, char *sWi, int iVendorID, PARTASSET2 *pzPAsset,
					  LEVELINFO *pzLevels, short iLongShort, int iID, long lTransNo)
{
	ERRSTRUCT	zErr;
	char		sTemp[40];
	int			iRecFound;

	lpprInitializeErrStruct(&zErr);
	InitializePartialAsset(pzPAsset);
	memset(pzLevels, 0, sizeof(pzLevels));

	iRecFound = 0;
	while (TRUE)
	{
		lpprSelectPartAsset(pzPAsset, pzLevels, sSecNo, sWi, iVendorID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			if (iRecFound = 0)
			{
				sprintf_s(sTemp, "SecNo - %s, Wi - %s Not Found", sSecNo, sWi);
				return(lpfnPrintError(sTemp, iID, lTransNo, "T", 8, 0, 0, "CALCPERF SELECTASSET1", FALSE));
			} // if aset not found
			else
			{
				zErr.iSqlError = 0;
				break;
			} //if at least one record was found, we are good
		} // EOF reached

		iRecFound++;
		pzPAsset->iSTypeIndex = FindSecTypeInTable(zPSTypeTable, pzPAsset->iSecType);
		if (pzPAsset->iSTypeIndex < 0)
		{ 
			sprintf_s(sTemp, "Invalid Sectype For - %s, %s", sSecNo, sWi);
			return(lpfnPrintError(sTemp, iID, lTransNo, "T", 18, 0, 0, "CALCPERF SELECTASSET2", FALSE));
		}

		pzPAsset->iLongShort  = iLongShort;
	} 

	return zErr;
} /* selectasset */


/**
** This function is used to select a record(by secno and whenissue) from assets
** table. When the asset is found, this function points iSTypeIndex to right
** element in SecType array in memory.
**/
ERRSTRUCT SelectAssetAllLevels(ASSETTABLE2* pzATable, char* sSecNo, char* sWi, int iVendorID, PARTASSET2* pzPAsset,
	LEVELINFO* pzLevels, short iLongShort, int iID, long lTransNo, int* iToSecNoIndex, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT	zErr;
	char		sTemp[40];
	int			iRecFound;

	lpprInitializeErrStruct(&zErr);
	InitializePartialAsset(pzPAsset);

	iRecFound = 0;
	while (TRUE)
	{
		lpprSelectPartAsset(pzPAsset, pzLevels, sSecNo, sWi, iVendorID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			if (iRecFound = 0)
			{
				sprintf_s(sTemp, "SecNo - %s, Wi - %s Not Found", sSecNo, sWi);
				return(lpfnPrintError(sTemp, iID, lTransNo, "T", 8, 0, 0, "CALCPERF SELECTASSET1", FALSE));
			} // if aset not found
			else
			{
				zErr.iSqlError = 0;
				break;
			} //if at least one record was found, we are good
		} // EOF reached

		iRecFound++;
		pzPAsset->iSTypeIndex = FindSecTypeInTable(zPSTypeTable, pzPAsset->iSecType);
		if (pzPAsset->iSTypeIndex < 0)
		{
			sprintf_s(sTemp, "Invalid Sectype For - %s, %s", sSecNo, sWi);
			return(lpfnPrintError(sTemp, iID, lTransNo, "T", 18, 0, 0, "CALCPERF SELECTASSET2", FALSE));
		}

		zErr = AddAssetToTable(pzATable, *pzPAsset, *pzLevels, zPSTypeTable, iToSecNoIndex, lLastPerfDate, lCurrentPerfDate);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			break;

		pzPAsset->iLongShort = iLongShort;
	}

	return zErr;
} /* selectasset */

  /**
** This function is used to delete summdata, rtrnset, netflow, etc. records from period and  
** monthly tables between a certain date range.
**/
ERRSTRUCT DeletePeriodPerformSet(PKEYTABLE zPTable, int iPortfolioID, long lStartDate, long lEndDate, int bCalcSelected, long lInceptionDate)
{
	ERRSTRUCT zErr;
	SUMMDATA  zSummData;
	UNITVALUE zUV;
	int       i, iLastID;
	long		aDate;

	lpprInitializeErrStruct(&zErr);

	iLastID = -1;
	while (zErr.iSqlError == 0)
	{
		lpprSelectPeriodSummdata(&zSummData, iPortfolioID, lStartDate, lEndDate, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		// If id is same as lastid, we have already deleted all the records for this id, continue
		if (zSummData.iID == iLastID)
			continue;

		// SB 3/29/2013 - Up until now, rule was to delete records only if key is found in the table, but now it's changed.
		//								If a segment was created becuase assets didn't have correct industry levels defined but later on
		//								user fixes the asset and re run the performance, the old dat was not being cleaned out.So, now the
		//								delete the old data if the key is not found in the table AND performance is not being run only 
		//								for selected perfrules.
		i = FindPerfkeyByID(zPTable, zSummData.iID, 0, FALSE);
		if (i < 0 && bCalcSelected)
			continue;

		/* Delete the whole set from monthly database by using perform_no */
		lpprDeleteSummdata(zSummData.iID, lStartDate, -1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/* Delete the whole set from monthly database by using perform_no */
		lpprDeleteMonthSum(zSummData.iID, lStartDate, -1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		lpprMarkPeriodUVForADateRangeAsDeleted(zSummData.iPortfolioID, zSummData.iID, lStartDate, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;		 

		//Looking for moving of an inception date within a month
		//if StartDate and Inception Date are in the same month
		//remove any records between first day of month and new inception date
		if (lStartDate > lpfnLastMonthEnd(lStartDate)+1 && lpfnLastMonthEnd(lStartDate)+1 == lpfnLastMonthEnd(lInceptionDate)+1) {
			lpprDeleteSummdata(zSummData.iID, lpfnLastMonthEnd(lStartDate) + 1, lInceptionDate-1, &zErr);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			/* Delete the whole set from monthly database by using perform_no */
			lpprDeleteMonthSum(zSummData.iID, lpfnLastMonthEnd(lStartDate) + 1, lInceptionDate-1, &zErr);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			for (aDate = lpfnLastMonthEnd(lStartDate) + 1; aDate < lInceptionDate; aDate++) {
				lpprDeleteDailyUnitValueForADate(iPortfolioID, zSummData.iID, aDate, -12, &zErr);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					return zErr;

			}
		}

		iLastID = zSummData.iID; // save the last id for which we deleted records

		/* Delete the whole set from monthly database by using perform_no * /
		lpprDeleteSummdata(zPTable.pzPKey[i].zPK.iID, lStartDate, -1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

		/ * Delete the whole set from monthly database by using perform_no * /
		lpprDeleteMonthSum(zPTable.pzPKey[i].zPK.iID, lStartDate, -1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

		lpprMarkPeriodUVForADateRangeAsDeleted(zPTable.pzPKey[i].zPK.iPortfolioID, 
		zPTable.pzPKey[i].zPK.iID, lStartDate, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;		 

		iLastID = zPTable.pzPKey[i].zPK.iID; // save the last id for which we deleted records*/
	} /* while no error */

	lpprInitializeErrStruct(&zErr);

	//clean up any remaining orphaned unitvalue entries that don't have summdata entries.
	iLastID = -1;
	while (zErr.iSqlError == 0)
	{
		lpprSelectUnitValueRange2(&zUV, iPortfolioID, lStartDate, lEndDate, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		// If id is same as lastid, we have already deleted all the records for this id, continue
		if (zUV.iID == iLastID)
			continue;

		//  Delete records only if key is found in the table. 
		i = FindPerfkeyByID(zPTable, zUV.iID, 0, FALSE);
		if (i < 0)
			continue;


		lpprMarkPeriodUVForADateRangeAsDeleted(zPTable.pzPKey[i].zPK.iPortfolioID, 
			zPTable.pzPKey[i].zPK.iID, lStartDate, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;		 

		iLastID = zPTable.pzPKey[i].zPK.iID; // save the last id for which we deleted records
	} /* while no error */

	return zErr;
} /* DeleteMonthlyPerformSet */


/**
** Function to update all the perfkeys for the account.
**/
ERRSTRUCT UpdateAllPerfkeys(PKEYTABLE zPTable)
{
	ERRSTRUCT zErr;
	int       i;

	lpprInitializeErrStruct(&zErr);

	for (i = 0; i < zPTable.iCount; i++)
	{
		if (zPTable.pzPKey[i].bNewKey && zPTable.pzPKey[i].bDeleteKey)
			continue;

		if (zPTable.pzPKey[i].bNewKey == TRUE)
		{
			/*
			** If a new parent Perfkey is added its ParentPerfkeyNo will be zero
			** since this information is available only after the key is added to
			** the database, so update that value now.
			*/
			if (strcmp(zPTable.pzPKey[i].zPK.sParentChildInd, "P") == 0)
				zPTable.pzPKey[i].zPK.lParentPerfkeyNo = zPTable.pzPKey[i].zPK.lPerfkeyNo;

			lpprInsertPerfkey(zPTable.pzPKey[i].zPK, &zErr);
		}
		else
			lpprUpdateOldPerfkey(zPTable.pzPKey[i].zPK, &zErr);

		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
	} /* for i < numpkey */

	return zErr;
} /* updateallperfkeys */


ERRSTRUCT EquityAndFixedCashContWithForThePeriod(TRANSTABLE zTTable, ASSETTABLE2 zATable, 
												 long lEndDate, double *pfEqFlow, double *pfFiFlow)
{
	FLOWCALCSTRUCT	zFCS;
	ERRSTRUCT		zErr;
	TRANS			zTempTrans;
	long			lLastMonthEnd;
	int				i;
	double			fFlow;
	char			cFlowType;
	short			iMDY[3];

	lpprInitializeErrStruct(&zErr);
	*pfEqFlow = *pfFiFlow = 0;

	lLastMonthEnd = lpfnLastMonthEnd(lEndDate);
	for (i = 0; i < zTTable.iNumTrans; i++)
	{
		// We need the values which are entered in LastMonthEnd + 1 to lEndDate period only
		if (zTTable.pzTrans[i].lPerfDate <= lLastMonthEnd)
			continue;

		// no need to look at the rest of the transactions
		if (zTTable.pzTrans[i].lPerfDate > lEndDate)
			break;

		if (!IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], CASHSEGMENT) &&
			!IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], EQUITYSEGMENT) &&
			!IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], FIXEDSEGMENT) &&
			!IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], EQUITYCASHSEGMENT) &&
			!IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], FIXEDCASHSEGMENT))
			continue;

		lpfnrjulmdy(zTTable.pzTrans[i].lPerfDate, iMDY);

		// Calculate flow
		CopyPartTransToTrans(zTTable.pzTrans[i], &zTempTrans);
		zErr = lpfnCalcNetFlow(zTempTrans, zTTypeTable.zTType[zTTable.pzTrans[i].iTranTypeIndex].sPerfImpact, 
			&zFCS, false);
		if (zErr.iBusinessError != 0)
			return zErr;

		if (IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], CASHSEGMENT))
		{
			if (strcmp(zTTable.pzTrans[i].sXSecNo, "E.") == 0)
				cFlowType = 'E';
			else if (strcmp(zTTable.pzTrans[i].sXSecNo, "B.") == 0)
				cFlowType = 'B';
			else
				cFlowType = ' ';

			fFlow = zFCS.fBPcplSecFlow + zFCS.fBIncomeSecFlow + zFCS.fBPcplCashFlow + zFCS.fBIncomeCashFlow;
		}
		else
		{
			if (IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], EQUITYSEGMENT) ||
				IsThisASpecialAsset(zATable.pzAsset[zTTable.pzTrans[i].iSecAssetIndex], EQUITYCASHSEGMENT))
				cFlowType = 'E';
			else 
				cFlowType = 'B';

			fFlow = zFCS.fBPcplCashFlow + zFCS.fBIncomeCashFlow;
		}

		if (zTTable.pzTrans[i].bReversal)
			fFlow *= -1;

		if (cFlowType == 'E')
			*pfEqFlow += fFlow;
		else if (cFlowType == 'B')
			*pfFiFlow += fFlow;
	} // for i < NumTrans

	return zErr;
} // EquityAndFixedCashFlowsForTheMonth

BOOL SpecialRuleForEquityAndFixedExists(PKEYTABLE zPTable, PERFRULETABLE zRuleTable)
{
	BOOL bRuleExists;
	int	 i, EqRule, FiRule;

	EqRule = FindSpecialRule(zRuleTable, "E"); // get index of equity + equity cash rule
	FiRule = FindSpecialRule(zRuleTable, "F"); // get index of fixed + fixed cash rule
	// if both eq + eq cash and fi + fi cash do not exist in rule table, no further checking is required
	if (EqRule < 0 && FiRule < 0)
		return FALSE;

	// It is possible that even though special rule exists, no key is defined using these
	// rules, so do additional checking
	bRuleExists = FALSE;
	i = 0;
	while (!bRuleExists && i < zPTable.iCount)
	{
		if (!IsKeyDeleted(zPTable.pzPKey[i].zPK.lDeleteDate) &&
			(EqRule > -1 && zRuleTable.pzPRule[EqRule].lRuleNo == zPTable.pzPKey[i].zPK.lRuleNo ||
			FiRule > -1 && zRuleTable.pzPRule[FiRule].lRuleNo == zPTable.pzPKey[i].zPK.lRuleNo))
			bRuleExists = TRUE;

		i++;
	}

	return bRuleExists;
} // SpecialRuleForEquityAndFixedExists


int FindSpecialRule(PERFRULETABLE zRuleTable, char *sWhichRule)
{
	int i, iResult;

	iResult = -1;
	i = 0;
	while (i < zRuleTable.iCount && iResult == -1)
	{
		if (strcmp(zRuleTable.pzPRule[i].sWeightedRecordIndicator, sWhichRule) == 0)
			iResult = i;

		i++;
	}

	return iResult;
} // FindSpecialRule


/*
** This function is used to find the script for special group IDs, EQUITY, FIXED, CASH, 
** EQUITYPLUSCASH and FIXEDCASH. 
*/
int	FindScriptForASpecialSegmentType(int iSegmentType, int iStartPoint)
{
	int i, iResult;

	iResult = -1;
	if (iStartPoint < 0)
		i = 0;
	else
		i = iStartPoint + 1;

	while (iResult == -1 && i < g_zSHdrDetTable.iNumSHdrDet)
	{
		if (g_zSHdrDetTable.pzSHdrDet[i].zHeader.iGroupID == iSegmentType)
			iResult = i;

		i++;
	}

	return iResult;
} // FindScriptForASegmentType


int FindKeyForASegmentType(PKEYTABLE zPTable, int iSegmentType, long lDate)
{
	int		i, iResult, iScrhdr;
	long	lCurrentPerfDate;

	iResult = -1;
	i = 0;
	if (zPTable.iCount > 0)
		lCurrentPerfDate = zPTable.pzPKey[i].pzDInfo[zPTable.pzPKey[i].iDInfoCount-1].lDate;
	else
		lCurrentPerfDate = 0;

	while (iResult == -1 && i < zPTable.iCount)
	{
		if ((zPTable.pzPKey[i].zPK.lDeleteDate == 0 || 
			(zPTable.pzPKey[i].zPK.lDeleteDate > lDate && zPTable.pzPKey[i].zPK.lDeleteDate != lCurrentPerfDate))
			&& (zPTable.pzPKey[i].zPK.lInitPerfDate <= lDate || zPTable.pzPKey[i].zPK.lInitPerfDate == lCurrentPerfDate))
		{
			iScrhdr = FindScrHdrByHdrNo(g_zSHdrDetTable, zPTable.pzPKey[i].zPK.lScrhdrNo);
			if (iScrhdr > 0 && g_zSHdrDetTable.pzSHdrDet[iScrhdr].zHeader.iGroupID == iSegmentType)
				iResult = i;
		}

		i++;
	}

	return iResult;
} // FindKeyForASegmentType

/**
** Function to find out parents for all the live(or the one deleted in the
** current date range) children key. If for any child key, parent is not found
** this function does not return that as an error.
**/
ERRSTRUCT FindParentPerfkeys(PKEYTABLE *pzPTable, long lStartDate)
{
	ERRSTRUCT zErr;
	int       i, j;

	lpprInitializeErrStruct(&zErr);

	for (i = 0; i < pzPTable->iCount; i++)
	{
		if (pzPTable->pzPKey[i].zPK.lDeleteDate != 0 &&
			pzPTable->pzPKey[i].zPK.lDeleteDate <= lStartDate)
			continue;

		if (strcmp(pzPTable->pzPKey[i].zPK.sParentChildInd, "P") == 0)
			continue;

		for (j = 0; j < pzPTable->iCount; j++)
		{
			if (strcmp(pzPTable->pzPKey[j].zPK.sParentChildInd, "P") == 0 &&
				pzPTable->pzPKey[j].zPK.lRuleNo == pzPTable->pzPKey[i].lParentRuleNo)
			{
				if (pzPTable->pzPKey[j].zPK.lInitPerfDate <= pzPTable->pzPKey[i].zPK.lInitPerfDate &&
					(pzPTable->pzPKey[j].zPK.lDeleteDate == 0 ||
					pzPTable->pzPKey[j].zPK.lDeleteDate >= pzPTable->pzPKey[i].zPK.lDeleteDate))
				{
					pzPTable->pzPKey[i].zPK.lParentPerfkeyNo=pzPTable->pzPKey[j].zPK.lPerfkeyNo;
					break;
				}
			}
		} /* for j */
	} /* for i */

	return zErr;
} /* FindParentPerfkeys */


/*
** This function finds the correct porttax record which should be applicable for
** the supplied date. If there is no record or it is invalid, then it sets the
** default applicable rate for the date.
*/
PORTTAX FindCorrectPorttax(PORTTAXTABLE zPTaxTable, long lDate)
{
	PORTTAX zPTax;
	int		i;

	InitializePorttax(&zPTax, lDate);
	zPTax.lTaxDate = 0; // reset taxdate to zero otherwise the subsequent codition will not work for the first record

	for (i = zPTaxTable.iCount - 1; i >= 0; i--)
	{
		if ((zPTaxTable.pzPTax[i].lTaxDate <= lDate && zPTaxTable.pzPTax[i].lTaxDate > zPTax.lTaxDate)  && 
			(zPTaxTable.pzPTax[i].fFedIncomeRate > 0 && zPTaxTable.pzPTax[i].fFedIncomeRate < 100))
			zPTax = zPTaxTable.pzPTax[i];
	}	

	return zPTax;
} // FindcorrectPorttax


ERRSTRUCT GetEquityAndFixedPercent(PKEYTABLE zPKTable, PERFRULETABLE zPRTable, PARTPMAIN zPmain,
								   long lDate, double *pfEqPct, double *pfFiPct)
{
	ERRSTRUCT	zErr;
	int			iEqKey, iEqPlusCKey, iFiKey, iFiPlusCKey, iDateIndex;
	double		fEqMV, fEqPlusCMV, fFiMV, fFiPlusCMV, fEqCash, fFiCash, fPmainEq, fPmainFi;
	long		lLastPerfDate, lCurrentPerfDate, lLastMonthEnd;

	lpprInitializeErrStruct(&zErr);

	*pfEqPct = *pfFiPct = 0;
	if (!SpecialRuleForEquityAndFixedExists(zPKTable, zPRTable))
		return zErr;

	// -ve numbers are not likely but do an extra check anyway
	if (zPmain.fMaxEqPct < 0)
		zPmain.fMaxEqPct = 0;
	if (zPmain.fMaxFiPct < 0)
		zPmain.fMaxFiPct = 0;

	if (zPmain.fMaxEqPct + zPmain.fMaxFiPct == 0)
		fPmainEq = fPmainFi = 0; 
	else
	{
		fPmainEq = zPmain.fMaxEqPct / (zPmain.fMaxEqPct + zPmain.fMaxFiPct);
		fPmainFi = zPmain.fMaxFiPct / (zPmain.fMaxEqPct + zPmain.fMaxFiPct);
	}

	lLastPerfDate = zPKTable.pzPKey[0].pzDInfo[0].lDate;
	lCurrentPerfDate = zPKTable.pzPKey[0].pzDInfo[zPKTable.pzPKey[0].iDInfoCount - 1].lDate;
	lLastMonthEnd = lpfnLastMonthEnd(lDate);

	// check which key(s) existed on last month end(relative to the date we are interested in)
	iEqKey		= FindKeyForASegmentType(zPKTable, EQUITYSEGMENT, lLastMonthEnd);
	iEqPlusCKey = FindKeyForASegmentType(zPKTable, EQUITYPLUSCASHSEGMENT, lLastMonthEnd);
	iFiKey		= FindKeyForASegmentType(zPKTable, FIXEDSEGMENT, lLastMonthEnd);
	iFiPlusCKey	= FindKeyForASegmentType(zPKTable, FIXEDPLUSCASHSEGMENT, lLastMonthEnd);

	// to start with, percentage comes from portmain
	*pfEqPct = fPmainEq;
	*pfFiPct = fPmainFi;

	/*
	** If Equity not found and fixed found, fixed should get 100 %, else if equity found and
	** fixed not found, equity should get 100%
	*/
	if (iEqKey < 0 && iFiKey >= 0)
	{
		*pfEqPct = 0;
		*pfFiPct = 1;

		return zErr;
	}
	else if (iFiKey < 0 && iEqKey >= 0)
	{
		*pfEqPct = 1;
		*pfFiPct = 0;

		return zErr;
	}

	/*
	** If none of the key existed on lDate then this is most likely a new account, then check
	** the Equity and Fixed Key on the CurrentPerfDate. If both keys exist on the CurrentPerfDate
	** percents obtained from portmain is correct, if only one of them exists then the percent
	** for that sector should be 100 % and that for the other sector should be 0 %. If neither
	** of those two keys exist(highly unlikely) then both percentage should be whatever is defined in portmain.
	*/
	if (iEqKey < 0 && iEqPlusCKey < 0 && iFiKey < 0 && iFiPlusCKey < 0)
	{
		iEqKey = FindKeyForASegmentType(zPKTable, EQUITYSEGMENT, lCurrentPerfDate);
		iFiKey = FindKeyForASegmentType(zPKTable, FIXEDSEGMENT, lCurrentPerfDate);

		if (iEqKey >= 0 && iFiKey < 0) // equity found but no fixed
		{
			*pfEqPct = 1;
			*pfFiPct = 0;
		}
		else if (iFiKey >= 0 && iEqKey < 0) // fixed found but no equity
		{
			*pfEqPct = 0;
			*pfFiPct = 1;
		}
		// else the percentage remain what ever is given in portmain

		return zErr;
	} // if none of the key is found

	// this should not happen
	if (lLastMonthEnd < lLastPerfDate || lLastMonthEnd > lCurrentPerfDate)
		return zErr;

	iDateIndex = GetDateIndex(zPKTable.pzPKey[0], lLastMonthEnd);
	if (iDateIndex < 0)
		return zErr;


	if (iEqKey >= 0)
		fEqMV = zPKTable.pzPKey[iEqKey].pzDInfo[iDateIndex].fMktVal;
	else
		fEqMV = 0;

	if (iEqPlusCKey >= 0)
		fEqPlusCMV = zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fMktVal;
	else
		fEqPlusCMV = 0;

	if (iFiKey >= 0)
		fFiMV = zPKTable.pzPKey[iFiKey].pzDInfo[iDateIndex].fMktVal;
	else
		fFiMV = 0;

	if (iFiPlusCKey >= 0)
		fFiPlusCMV = zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fMktVal;
	else
		fFiPlusCMV = 0;

	fEqCash = fEqPlusCMV - fEqMV;
	fFiCash	= fFiPlusCMV - fFiMV;

	if (fEqCash < 0)
		fEqCash = 0;

	if (fFiCash < 0)
		fFiCash = 0;

	if (fEqCash + fFiCash == 0)
	{
		*pfEqPct = fPmainEq;
		*pfFiPct = fPmainFi;
	}
	else
	{
		*pfEqPct = fEqCash / (fEqCash + fFiCash);
		*pfFiPct = fFiCash / (fEqCash + fFiCash);
	}

	return zErr;
} //GetEquityAndFixedPercent


/*
** Function to figure out if an asset is of special type(EQUITY, FIXED or CASH). This function
** tries to find a script for the special type, if it finds the script, assets' level1 value
** is checked against the found script's first(non comment) detail record, if it matches the
** asset is of that special type, else not.
*/
BOOL IsThisASpecialAsset(PARTASSET2 zAsset, int iSpecialType)
{
	int		i, j;
	BOOL	bResult;
	//if (!SpecialRuleForEquityAndFixedExists(zPKTable, zPRTable))
	bResult = FALSE;
	i =	FindScriptForASpecialSegmentType(iSpecialType, 0);
	while (i >= 0)
	{
		j = 0;
		while (j < g_zSHdrDetTable.pzSHdrDet[i].iNumDetail && !bResult)
		{
			// not a comment, assets's level1 matches to the begin point and it should be included, then we have a match
			if (strcmp(g_zSHdrDetTable.pzSHdrDet[i].pzDetail[j].sComparisonRule, "C") != 0 &&
				atoi(g_zSHdrDetTable.pzSHdrDet[i].pzDetail[j].sBeginPoint) == zAsset.pzDAInfo[0].iIndustLevel1 &&
				strcmp(g_zSHdrDetTable.pzSHdrDet[i].pzDetail[j].sIncludeExclude, "I") == 0)
				bResult = TRUE;

			j++;
		}
		i =	FindScriptForASpecialSegmentType(iSpecialType, i);
	} // while there is any more script matching that requirement

	return bResult;
} // IsThisASpecialAsset


ERRSTRUCT ResolveResultList(RESULTLIST zList, BOOL *pbResult)
{
	ERRSTRUCT	zErr;
	RESULTLIST	zNewList;
	BOOL		bNegateResult, bInParenthesis, bAddToList, bAddOperator;
	int			i;

	lpprInitializeErrStruct(&zErr);

	zErr = IsResultListOK(zList);
	if (zErr.iBusinessError != 0) 
		return zErr;

	zNewList.iCapacity = 0;
	InitializeResultList(&zNewList);

	if (zList.iCount == 1)
	{
		*pbResult = StrToBool(zList.sItem[0].sValue);
		return zErr;
	}
	else
	{
		bInParenthesis = bAddToList = FALSE;
		for (i = 0; i < zList.iCount; i++)
		{
			if (strcmp(zList.sItem[i].sValue, "T") == 0 || strcmp(zList.sItem[i].sValue, "F") == 0)
			{
				if (!bInParenthesis)
				{
					if (i == 0)
						*pbResult = StrToBool(zList.sItem[i].sValue);
					else if (strcmp(zList.sItem[i-1].sValue, "A") == 0)
					{
						*pbResult = *pbResult && StrToBool(zList.sItem[i].sValue);
						bAddToList = TRUE;
					}
					else if (strcmp(zList.sItem[i-1].sValue, "O") == 0)
					{
						*pbResult = *pbResult || StrToBool(zList.sItem[i].sValue);
						bAddToList = TRUE;
					}
					else
						return(lpfnPrintError("Previous Item Is Not An Operator", 0, 0, "", 999, 0, 0, 
						"CALCPERF RESOLVERESULTLIST1", FALSE));
				} // Not in parenthesis
				else
				{
					if (strcmp(zList.sItem[i-1].sValue, "(") == 0 || strcmp(zList.sItem[i-1].sValue, "-(") == 0)
						*pbResult = StrToBool(zList.sItem[i].sValue);
					else if (strcmp(zList.sItem[i-1].sValue, "A") == 0)
						*pbResult = *pbResult && StrToBool(zList.sItem[i].sValue);
					else if (strcmp(zList.sItem[i-1].sValue, "O") == 0)
						*pbResult = *pbResult || StrToBool(zList.sItem[i].sValue);
					else
						return(lpfnPrintError("Previous Item Is Not An Operator", 0, 0, "", 999, 0, 0, 
						"CALCPERF RESOLVERESULTLIST2", FALSE));
				} // In parenthesis
			} // if current item is T or F
			else if (strcmp(zList.sItem[i].sValue, "A") == 0 || strcmp(zList.sItem[i].sValue, "O") == 0)
				bAddToList = bAddOperator;
			else if (strcmp(zList.sItem[i].sValue, "(") == 0 || strcmp(zList.sItem[i].sValue, "-(") == 0)
			{
				bInParenthesis = TRUE;
				if (strcmp(zList.sItem[i].sValue, "-(") == 0)
					bNegateResult = TRUE;
				else	
					bNegateResult = FALSE;			
			} // if ( or -(
			else if (strcmp(zList.sItem[i].sValue, ")") == 0)
			{
				if (bNegateResult)
					*pbResult = !*pbResult;

				bAddToList = TRUE;
			}

			if (bAddToList)
			{
				if (bAddOperator)
					zErr = AddAnItemToResultList(&zNewList, zList.sItem[i].sValue);
				else
					zErr = AddAnItemToResultList(&zNewList, BoolToStr(*pbResult));
				bAddToList = FALSE;
				if (zErr.iBusinessError != 0)
				{
					InitializeResultList(&zNewList);
					return zErr;
				}
				// If the next item in the old list is an operator, add it to the new list
				bAddOperator = !bAddOperator;
			} // if need to be added to the list
			else
				bAddOperator = FALSE;
		} // for i < sList.iCount
	} // list has more than one item

	zErr = ResolveResultList(zNewList, pbResult);
	InitializeResultList(&zNewList);
	return zErr;
} // ResolveReturnList


ERRSTRUCT IsResultListOK(RESULTLIST zList)
{
	ERRSTRUCT	zErr;
	int			i, iOpenParenthesis;

	lpprInitializeErrStruct(&zErr);

	iOpenParenthesis = 0;
	for (i = 0; i < zList.iCount; i++)
	{
		if (i == 0 && strcmp(zList.sItem[i].sValue, "(") != 0 &&  strcmp(zList.sItem[i].sValue, "-(") != 0 &&
			strcmp(zList.sItem[i].sValue, "T") != 0 && strcmp(zList.sItem[i].sValue, "F") != 0)
			return(lpfnPrintError("First Item Is Not Valid", 0, 0, "", 999, 0, 0, "CALCPERF ISRESULTOK1", FALSE));
		else if (i > 0 && (strcmp(zList.sItem[i].sValue, "T") == 0 || strcmp(zList.sItem[i].sValue, "F") == 0) &&
			(strcmp(zList.sItem[i-1].sValue, "(") != 0 &&  strcmp(zList.sItem[i-1].sValue, "-(") != 0 &&
			strcmp(zList.sItem[i-1].sValue, "A") != 0 &&  strcmp(zList.sItem[i-1].sValue, "O") != 0))
			return(lpfnPrintError("Previous Item Is Not A Parenthesis Or An Operator", 0, 0, "", 999, 0, 0, "CALCPERF ISRESULTOK2", FALSE));
		else if (i > 0 && (strcmp(zList.sItem[i].sValue, "A") == 0 || strcmp(zList.sItem[i].sValue, "O") == 0) &&
			(strcmp(zList.sItem[i-1].sValue, "T") != 0 &&  strcmp(zList.sItem[i-1].sValue, "F") != 0) )
			return(lpfnPrintError("Previous Item Is Not An Operand", 0, 0, "", 999, 0, 0, "CALCPERF ISRESULTOK3", FALSE));

		if (strcmp(zList.sItem[i].sValue, "(") == 0 || strcmp(zList.sItem[i].sValue, "-(") == 0)
		{
			if (i > 0 && (strcmp(zList.sItem[i-1].sValue, "O") != 0 &&  strcmp(zList.sItem[i-1].sValue, "A") != 0))
				return(lpfnPrintError("Previous Item Is Not An Operator", 0, 0, "", 999, 0, 0, "CALCPERF ISRESULTOK4", FALSE));

			iOpenParenthesis++;
		}
		else if (strcmp(zList.sItem[i].sValue, ")") == 0)
		{
			if (iOpenParenthesis <= 0)
				return(lpfnPrintError("Closing Parenthesis Found Without An Open Parenthesis", 0, 0, "", 999, 0, 0, "CALCPERF ISRESULTOK4", FALSE));
			else
				iOpenParenthesis--;
		}
	}
	if (iOpenParenthesis != 0)
		return(lpfnPrintError("No Closing Parenthesis Matching Open Parenthesis", 0, 0, "", 999, 0, 0, "CALCPERF ISRESULTOK5", FALSE));


	return zErr;
} //IsReultListOK


/*
** Function which return segmenttype(groupid) for the passed key. Asof now(8/6/99) following
** are the possible values for SegmentType(Group ID)
**		EQUITYSEGMENT			-	102
**		FIXEDSEGMENT			-	103
**		CASHSEGMENT				-	104
**		EQUITYPLUSCASHSEGMENT	-	112
**		FIXEDPLUSCASHSEGMENT	-	113
**		EQUITYCASHSEGMENT		-	122
**		FIXEDCASHSEGMENT		-	123
*/
int SegmentTypeForTheKey(PERFKEY zPK)
{
	int i;

	i = FindScrHdrByHdrNo(g_zSHdrDetTable, zPK.lScrhdrNo);
	if (i < 0)
		return 0;
	else
		return g_zSHdrDetTable.pzSHdrDet[i].zHeader.iGroupID;
} // SegmentTypeForTheKey

DllExport ERRSTRUCT FillPartSectypeTable(PARTSTYPETABLE *pzPSTTable)
{
	PARTSTYPE	zPSType;
	ERRSTRUCT	zErr;

	lpprInitializeErrStruct(&zErr);

	while (TRUE)
	{
		lpprSelectAllPartSectype (&zPSType, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else if (pzPSTTable->iNumSType == NUMSECTYPES)
			return(lpfnPrintError("SecTypes Table Too Small", 0, 0, "", 996, 0, 0, "CALCPERF FILLSECTYPETABLE", FALSE));
		else
			pzPSTTable->zSType[pzPSTTable->iNumSType] = zPSType;

		pzPSTTable->iNumSType++;
	} // get all sectype

	return zErr;
} // FillSectypeTable

// Function to free up memory used by asset, holdings, transactions pkeyasset and perfrule tables
void CalcPerfInitialize(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable, TRANSTABLE *pzTTable, PKEYASSETTABLE2 *pzPKATable, 
						PERFRULETABLE *pzRuleTable, ACCDIVTABLE *pzADTable, PERFASSETMERGETABLE *pzAMTable)
{
	InitializeAssetTable(pzATable);
	InitializeHoldingTable(pzHTable);
	InitializeTransTable(pzTTable);
	InitializePKeyAssetTable(pzPKATable);      
	InitializePerfruleTable(pzRuleTable);
	InitializeAccdivTable(pzADTable);
} /* calcperfinitialize */


BOOL TaxInfoRequired(int iReturnsToCalculate)
{
	if (iReturnsToCalculate & AfterTaxRorBit)
		return TRUE;
	else if (iReturnsToCalculate & TaxEquivRorBit)
		return TRUE; 
	else
		return FALSE;
}

/**
** Function to initialize Monthly Data Table
**/
void InitializeMonthlyTable(MONTHTABLE *pzMTable)
{
	if (pzMTable->iCapacity > 0 && pzMTable->pzMonthlyData != NULL)
		free(pzMTable->pzMonthlyData);

	pzMTable->iCapacity = pzMTable->iCount = 0;
	pzMTable->pzMonthlyData = NULL;
} /* InitializeMonthlyTable */

/**
** Function to fill  script header/detail table. Depending on the argument bAppendToTable
** this function may first clean existing records from the table, or just add new records
** to it. In either cases query always retrieves all the script header/detail from the table
**/
ERRSTRUCT FillScriptHeaderDetailTable(PSCRHDRDETTABLE *pzPSHdrDetTable, BOOL bAppendToTable)
{
	PSCRHDR		zSHeader;
	PSCRDET		zSDetail;
	ERRSTRUCT	zErr;
	long		lFirstScrhdrNoTobeAdded;
	int			iIndex;

	lpprInitializeErrStruct(&zErr);

	// If not appending records to the table then first initialize it
	if (!bAppendToTable)
		InitializePScrHdrDetTable(pzPSHdrDetTable);

	// Find the first script header number that should be added to the table, since the records in the table are 
	// added in the ascending order, just adding 1 to the last record's script header number should do the trick
	if (pzPSHdrDetTable->iNumSHdrDet > 0)
		lFirstScrhdrNoTobeAdded = pzPSHdrDetTable->pzSHdrDet[pzPSHdrDetTable->iNumSHdrDet-1].zHeader.lScrhdrNo + 1;
	else
		lFirstScrhdrNoTobeAdded = 1;

	// retrieve all the records from the database and add to table, if already not there
	while (TRUE)
	{
		lpprInitializeErrStruct(&zErr);
		InitializePerfScrHdr(&zSHeader);
		InitializePerfScrDet(&zSDetail);

		lpprSelectAllScriptHeaderAndDetails (&zSHeader, &zSDetail, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else
		{
			/* 
			** When this function is called to append records to the table, we'll have to see 
			** whether the record retrieved by the query aready exist in the table or not. 
			** The query SelectAllScriptHeaderAndDetails retrieves all script headers in ascending
			** order of script header number and the records in the memory table go in same order. 
			** So if the script header number retrieved by the query is less than the first script 
			** header number that should be added then this header already exist in the memory table 
			** and no need to add it.
			*/
			if (zSHeader.lScrhdrNo >= lFirstScrhdrNoTobeAdded)
			{
				zErr = AddScriptHeaderToTable(zSHeader, pzPSHdrDetTable, &iIndex);
				if (zErr.iBusinessError != 0)
					return zErr;

				/*
				** Script header and detail are in an outer join, it is possible to have
				** a script header without any detail record(e.g. Total Portfolio). If
				** indicator variable is -ve, a NULL value for seqno was returned.
				*/
				if (zSDetail.lScrhdrNo == zSHeader.lScrhdrNo)
				{
					/* add detail to the found/added header */
					zErr = AddDetailToScrHdrDet(zSDetail, &pzPSHdrDetTable->pzSHdrDet[iIndex]);
					if (zErr.iBusinessError != 0)
						return zErr;
				} /* if a detail record is found */
			} /* If this header record doesn't already exist in table */
		} /* no error in fetching the record */
	} // get all script headers and details

	return zErr;
} //FillScriptHeaderDetailTable

int FindDailyInfoOffset(long lDate, PARTASSET2 zPAsset, BOOL bMatchExactDate)
{
	int i;

	// if there is no daily info in asset then return -1
	if (zPAsset.iDailyCount <= 0)
		return -1;

	/*
	** The records in the dailyinfo array are in ascending order of the date, go through the records and find the 
	** last date greater than or equal to the date being searched. If the exact date being searched for is found 
	** then that's record we are looking for. If on the other hand we got the largest date less than the date being 
	** searched then whether we found a match or not depends on the value of last argument 'bMatchExactDate', if
	** it's true then we couldn't find a match if it's false then a match is found.
	*/
	i = 0;
	while (zPAsset.pzDAInfo[i].lDate <= lDate)
	{
		i++;
		if (i >= zPAsset.iDailyCount)
			break;
	}

	if (zPAsset.pzDAInfo[i-1].lDate == lDate || !bMatchExactDate)
		return i - 1;
	else 
		return -1;
} // FindDailyInfoOffset

ERRSTRUCT FindOrAddDailyAssetInfo(PARTASSET2 *pzPAsset, long lDate, long lCurrentPerfDate, int *piIndex)
{
	ERRSTRUCT	zErr;
	int			i;

	lpprInitializeErrStruct(&zErr);
	*piIndex = -1;

	// if the date being added is later than the last date for which performance is being calculated, don't add it
	if (lDate > lCurrentPerfDate) 
		return zErr;

	// try to find the date in the asset's daily info, if found, return the array index
	for (i = 0; i < pzPAsset->iDailyCount; i++)
	{
		if (pzPAsset->pzDAInfo[i].lDate == lDate)
		{
			*piIndex = i;
			return zErr;
		}
	}

	// Allocate memory for the additional item being added to the array
	pzPAsset->iDailyCount++;
	zErr = AllocateMemoryToDailyAssetInfo(pzPAsset);
	if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		return zErr;

	// The items to be added to the array can come in any order but they should be added to the
	// array in ascending order of the date. So, look at the date being added, if there are records
	// in the array for later dates, move them to the next element and then add the date in proper place.
	// Start assuming the current item will be added at the end then go backward until you keep finding dates 
	// greater than the date being added
	*piIndex = pzPAsset->iDailyCount - 1;
	for (i = pzPAsset->iDailyCount - 2; i >= 0; i--)
	{
		// since the records get added in ascending order of date and we are starting at the end
		// as soon as a date earlier than date being added is found we are done
		if (pzPAsset->pzDAInfo[i].lDate < lDate)
			break;

		// if control came here it means we found a date later than the date being added
		*piIndex = i;
	} // find the position where current record should be added

	// move all the records from the position where new date needs to be inserted, to the next array position
	for (i = pzPAsset->iDailyCount - 2; i >= *piIndex; i--)
		pzPAsset->pzDAInfo[i+1] = pzPAsset->pzDAInfo[i];

	// now add the new record at the proper position
	pzPAsset->pzDAInfo[*piIndex].lDate = lDate;
	pzPAsset->pzDAInfo[*piIndex].iIndustLevel1 = 0;
	pzPAsset->pzDAInfo[*piIndex].iIndustLevel2 = 0;
	pzPAsset->pzDAInfo[*piIndex].iIndustLevel3 = 0;
	pzPAsset->pzDAInfo[*piIndex].bGotMV = FALSE;
	pzPAsset->pzDAInfo[*piIndex].bGotAccrual = FALSE;
	pzPAsset->pzDAInfo[*piIndex].fMktVal = 0;
	pzPAsset->pzDAInfo[*piIndex].fAI = 0;
	pzPAsset->pzDAInfo[*piIndex].fAD = 0;
	pzPAsset->pzDAInfo[*piIndex].fMVExRate = 1.0;
	pzPAsset->pzDAInfo[*piIndex].fAccrExRate = 1.0;

	return zErr;
} // FindOrAddDailyAssetInfo

BOOL IsThisDailyValuesSameAsPrevious(PARTASSET2 zAsset, int iDateOffset)
{
	// if starting at the first array element, nothing to compare to previous element and hence return FALSE
	if (iDateOffset < 1 || iDateOffset >= zAsset.iDailyCount)
		return FALSE;

	if (zAsset.pzDAInfo[iDateOffset].iIndustLevel1 == zAsset.pzDAInfo[iDateOffset-1].iIndustLevel1 &&
		zAsset.pzDAInfo[iDateOffset].iIndustLevel2 == zAsset.pzDAInfo[iDateOffset-1].iIndustLevel2 &&
		zAsset.pzDAInfo[iDateOffset].iIndustLevel3 == zAsset.pzDAInfo[iDateOffset-1].iIndustLevel3)
		return TRUE;
	else
		return FALSE;
} // IsThisDailyValuesSameAsPrevious
