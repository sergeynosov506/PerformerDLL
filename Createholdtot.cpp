/*
* 
* FILENAME: CreateHoldtot.c
* 
* DESCRIPTION: To create Holdtot records from holdings/holdcash of a portfolio for the given date
* 
* PUBLIC FUNCTION(S): 
*                      1. DLLAPI ERRSTRUCT STDCALL WINAPI InitHoldtot(long lAsofDate, char *sDBAlias, char *sErrFile);
*                      2. DLLAPI ERRSTRUCT STDCALL WINAPI CreateHoldtot(int iPortfolioID, long lDate);
* 
* NOTES: Calling program MUST call InitHoldtot once before calling CreateHoldtot to initialize the Dll
* 
* USAGE:
* 
* AUTHOR: Shobhit Barman
*
**/
// History
// 2010-09-16 VI#44799 Added capability to calculate average final bond maturity - mk.
// 2010-09-14 VI#44798 Added flag to exclude zero-RAT/MATs' market values in average calculation in holdtot - mk.
// 2010-06-10 VI#44127 Added flag to include zero-YTMs' market values in average calculation in holdtot - mk.
// 2007-11-08 Added Vendor id    - yb
// 07/22/2004 - Fixed IsAssetInternational (was looking for pointer <> 0 rather than contents) for Level2 - mk
// 05/25/2004 - Added calcualtions of equity statistics (had to rename to *.cpp) - vay

#include "CreateHoldtot.h"
#include "perfdll.h"
#include "OLEDBIOCommon.h"
#import "StatisticsSrv.tlb" no_namespace

HINSTANCE		PerfCalcDll, OledbIODll, TransEngineDll; 
static BOOL bInit = FALSE;

	
BOOL APIENTRY DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:	break;
		case DLL_PROCESS_DETACH:	CleanUpSystemTables();
															break;
		default:									break;
	}

	return TRUE;
}

/*
** Main function for the dll
*/ 
DLLAPI ERRSTRUCT STDCALL WINAPI CreateHoldtot(int iPortfolioID, long lDate)
{
	ERRSTRUCT						zErr;
	HOLDINGSASSETSTABLE	zHoldingsAssetsTable;
	SEGMAINTABLE				zSegmainTable;
	PARTPMAIN						zPmain;
	PERFRULETABLE				zPortfolioRules;
	ASSETTABLE2					zATable;
	long							  lTransCount;
	BOOL						bUseTrans;

	lpprInitializeErrStruct(&zErr);
	
	CoInitialize(0);

	zHoldingsAssetsTable.iHoldingsAssetsCreated = 0;
	zSegmainTable.iCapacity = 0;
	zATable.iAssetCreated = 0;
	zPortfolioRules.iCapacity = 0;
	CleanUpPortfolioTables(&zHoldingsAssetsTable, &zSegmainTable, &zPortfolioRules, &zATable);

	// Initialize it for the given date, if necessary
	zErr = InitHoldtot(lDate, "", "");
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
		return zErr;

	// get account informationm
	lpprSelectOnePartPortmain(&zPmain, iPortfolioID, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
	{
		if (zErr.iSqlError == SQLNOTFOUND)
      lpfnPrintError("Portfolio Not Found", iPortfolioID, 0, "", 1, zErr.iSqlError, 0, "CREATEHOLDTOT1", FALSE);

    return zErr;
	}

	bUseTrans = (lpfnGetTransCount()==0);
	if (bUseTrans)
		lpfnStartTransaction();

	__try
	{
		// Delete holdtot records
		lpprDeleteHoldtotForAnAccount(zPmain.iID, &zErr);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) 
		{
			if (bUseTrans)
				lpfnRollbackTransaction();
			return zErr;
		}

		// Get all the segments (from segmain) for this portfolio
		zErr = GetAllSegmentsFromSegmainForPortId(zPmain.iID, &zSegmainTable);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) 
		{
			if (bUseTrans)
				lpfnRollbackTransaction();
			return zErr;
		}

		// Get holdings, haldcash and assets
		zErr = GetHoldingsAndAssetTables(zPmain.iID, zPmain.iVendorID, lDate, &zHoldingsAssetsTable, &zATable);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
			CleanUpPortfolioTables(&zHoldingsAssetsTable, &zSegmainTable, &zPortfolioRules, &zATable);
			if (bUseTrans)
				lpfnRollbackTransaction();
			return zErr;
		}

		// Get all perfrules for this portfolio
		zErr = GetAllPerfrulesForAPortfolio(&zPortfolioRules, zPmain.iID, lDate);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
			CleanUpPortfolioTables(&zHoldingsAssetsTable, &zSegmainTable, &zPortfolioRules, &zATable);
			if (bUseTrans)
				lpfnRollbackTransaction();
			return zErr;
		}

		// For Stock & Bond accounts, don't want to create any segments here and those accounts
		// have no transactions, so check if there are any transaction for this portfolio

		lpprRegTransCount(zPmain.iID, &lTransCount, &zErr);
		if (zErr.iSqlError || zErr.iBusinessError)
		{
			CleanUpPortfolioTables(&zHoldingsAssetsTable, &zSegmainTable, &zPortfolioRules, &zATable);
			if (bUseTrans)
				lpfnRollbackTransaction();
			return(lpfnPrintError("Error Reading Regular Count From Trans", zPmain.iID, 0, "", 0, 
			                    zErr.iSqlError, zErr.iIsamCode, "CREATEHOLDTOT2", FALSE));
		}

		
		/*
		** There are some portfolios where performance is not calculated at all or is calculated only 
		** for total portfolio or level1 but some of the reports (account summary, analytical summary)
		** require holdtoy information upto level2. If portfolio is calculating performance on all the
		** rules used by system then create segments for all its perfules, else create the segments
		** for all the system rules.
		** Don't do this for stock and bond portfolios (even composites will be missed because they
		** don't have any transaction but that's OK, composite merge of performance will create any
		** segments, if required.
		*/
		if (lTransCount > 0 || zPmain.bIsMarketIndex) 
		{
			if (zPortfolioRules.iCount >= zSystemRules.iCount)
				zErr = CreateSegmainsIfRequired(&zSegmainTable, zPmain, lDate, zATable, zPortfolioRules);
			else
				zErr = CreateSegmainsIfRequired(&zSegmainTable, zPmain, lDate, zATable, zSystemRules);

			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			{
				CleanUpPortfolioTables(&zHoldingsAssetsTable, &zSegmainTable, &zPortfolioRules, &zATable);
				if (bUseTrans)
					lpfnRollbackTransaction();
				return zErr;
			}
		} // if have transactions

		// Create the actual holdtot record
		zErr = CreateHoldtotRecord(zPmain, lDate, &zHoldingsAssetsTable,&zSegmainTable); 

		// memory cleanup
		CleanUpPortfolioTables(&zHoldingsAssetsTable, &zSegmainTable, &zPortfolioRules, &zATable);

	}//try
	__except(lpfnAbortTransaction(bUseTrans)){}

	if (bUseTrans)
	{
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			lpfnRollbackTransaction();
		else
			lpfnCommitTransaction();
	}

	::CoUninitialize();
	return zErr;
} // Create Holdtot

HRESULT CoCreateStatSrv(IStatisticsPtr& obj)
{
	struct __declspec(uuid("729052B6-696B-4E48-A295-53E79D1DB159")) CLASS_Statistics;
	struct __declspec(uuid("00000001-0000-0000-C000-000000000046")) IID_CLASS_FACTORY;
	HMODULE hDll = LoadLibrarySafe("StatisticsSrv.DLL");
	if (!hDll)
		return HRESULT_FROM_WIN32(GetLastError()); // library not found

	BOOL(WINAPI*DllGetClassObject)(REFCLSID, REFIID, LPVOID) =
		(BOOL(WINAPI*)(REFCLSID, REFIID, LPVOID))GetProcAddress(hDll, "DllGetClassObject");

	if (!DllGetClassObject)
		return HRESULT_FROM_WIN32(GetLastError());

	CComPtr<IClassFactory> pClassFactory;
	HRESULT hr = DllGetClassObject(__uuidof(CLASS_Statistics), __uuidof(IID_CLASS_FACTORY), &pClassFactory);
	if (FAILED(hr))
		return hr;

	hr = pClassFactory->CreateInstance(NULL, __uuidof(IStatistics), (void**)&obj);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

/*
** For each script it checks whether the Perfkey exists. If it exists then for each Asset
** it tests the asset against the script. If it passes then for every
** Holding, if an asset exists, it adds the values to create a Holdtot 
** Record. It does the same for Holdcash records. For every script, it then 
** inserts the Holdtot record in the table.
*/ 
ERRSTRUCT CreateHoldtotRecord(PARTPMAIN zPartPortMain, long lDate, HOLDINGSASSETSTABLE *zHoldingsAssetsTable, SEGMAINTABLE* pzSegmainTable)
{
	ERRSTRUCT		zErr;
	int					i, j, Level2, iInternationalLvlId;
	int					iSegmentId = 0, IntIndex = 0, iLevel = 0, iUnsupervisedSegmentId, iPlIndex = 0;
	int					iPledgedSegmentId = 0, iPledgeSegment = 0, iSegTypeId, iSegLvlId;
	double			TotalMktValue = 0, TotalCost = 0, fTemp = 0;
	long				lScrhdrNo = 0;
	char				sCheckFor, sSecurity[1+NT], sSecNo[12+NT], sWi[1+NT], sSecXtend[2+NT]; 
	BOOL				bResult, bAddToHoldtot, bInternational, bLevel2 = FALSE;
	MKTVALUES		zTotalMV, zSegmentMV, 
							zInterSegmentTotals, zOtherSegmentTotals, 
							zInternationalHoldTotTotal[10], zPledgedHoldTotTotal[10];
	PARTASSET2	zPartAsset;
	HOLDTOT			zHoldtot, zInterHoldtot, zOtherHoldtot, 
							zInternationalHoldTot[10], zUnsupervisedHoldTot, zPledgedHoldTot[10];
	LEVELINFO		zLevels;
	

	// initialize error structure & other variables 
	lpprInitializeErrStruct(&zErr);
	
	IStatisticsPtr pStats;
	HRESULT hr = CoCreateStatSrv(pStats);
		//pStats.CreateInstance(__uuidof(Statistics));
	if FAILED(hr) 
	{
		zErr.iBusinessError = hr;
		return zErr;
	}
		
	zPartAsset.iDailyCount = 0;
	lpprInitializePartAsset(&zPartAsset);

	memset(&zInternationalHoldTot, 0, sizeof(zInternationalHoldTot));
	memset(&zInternationalHoldTotTotal, 0, sizeof(zInternationalHoldTotTotal));
	memset(&zUnsupervisedHoldTot, 0, sizeof(zUnsupervisedHoldTot));
	memset(&zPledgedHoldTotTotal, 0, sizeof(zPledgedHoldTotTotal));
	memset(&zPledgedHoldTot, 0, sizeof(zPledgedHoldTot));
	memset(&zTotalMV, 0, sizeof(zTotalMV));

	// assign enough memory for one daily value
	zPartAsset.iDailyCount = 1;
	zErr = lpfnAllocateMemoryToDailyAssetInfo(&zPartAsset);
	if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		return zErr;

	/*
	** Add up the market value totals to use for calculating the weighted values. Unsupervised 
	** taxlots are not included in the totals. If the corresponding value to be weighted is 0, 
	** the mkt val for the security is not added i.e. the weightage will be calculated for non 
	** zero values to be weighted.
	*/	
	for (i = 0; i < zHoldingsAssetsTable->iNumHoldingsAssets;  i++)
	{
		if (strcmp(zHoldingsAssetsTable->pzHoldingsAssets[i].sSecXtend,"UP") == 0) 
			continue; // no unsupervised

		zTotalMV.fNativeMktVal += zHoldingsAssetsTable->pzHoldingsAssets[i].fMktVal;

		if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fMvSysXrate,12)) // mkt val in sys curr
			zTotalMV.fSysMktVal += zHoldingsAssetsTable->pzHoldingsAssets[i].fMktVal / zHoldingsAssetsTable->pzHoldingsAssets[i].fMvSysXrate;

		// if exchange rate is 0, default it to 1
		if (IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fMvBaseXrate,12)) 
		  zHoldingsAssetsTable->pzHoldingsAssets[i].fMvBaseXrate = 1.0;

		fTemp = zHoldingsAssetsTable->pzHoldingsAssets[i].fMktVal / zHoldingsAssetsTable->pzHoldingsAssets[i].fMvBaseXrate;

		if (IsValueZero(fTemp,2)) 
			continue; // no sense if the value to be added is 0

		// base mkt val
		zTotalMV.fBaseMktVal += fTemp;

		// coupon rate			
		if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fCouponRate, 5))		
			zTotalMV.fMktValForCoupon	+= fTemp;

		// yield to maturity				
		if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fMktEffMatYld, 5))	
			zTotalMV.fMktValForYTM += fTemp;

		// current yield
		if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fMktCurYld, 5))		
			zTotalMV.fMktValForCurYield	+= fTemp;

		// duration - only for bonds, cash and money market
		if (zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0] == 'C' || 
				zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0] == 'B' || 
				zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0] == 'M')			
			zTotalMV.fMktValForDuration	+= fTemp;
		
		// ratings
		//if (GetNumericalRating(zHoldingsAssetsTable->pzHoldingsAssets[i].sRating, zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0]) != 0)	zTotalMV.fMktValForRatings	+= fTemp;
		// GK Changed 2000-11-14 
		if (zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0] == 'E') 
			sCheckFor = zSysSet.zSyssetng.sEquityRating[0];
		else 
			sCheckFor = zSysSet.zSyssetng.sFixedRating[0];

		if (sCheckFor == 'M')
		{
			if (GetNumericalRating(zHoldingsAssetsTable->pzHoldingsAssets[i].sMoodyRating , zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0]) != 0)  
				zTotalMV.fMktValForRatings  += fTemp;
		}

		if (sCheckFor == 'S' || sCheckFor == 'B') 
		{
		  if (GetNumericalRating(zHoldingsAssetsTable->pzHoldingsAssets[i].sSpRating , zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0]) != 0)	
			  zTotalMV.fMktValForRatings	+= fTemp;
		}
		
		// maturity
		if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fMaturity, 3))		
			zTotalMV.fMktValForMaturity	+= fTemp;

		// maturity real
		if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fMaturityReal, 3))		
			zTotalMV.fMktValForMaturityReal	+= fTemp;

		// for equity statistics (PctEPS1Yr...DivYield)
		if (zHoldingsAssetsTable->pzHoldingsAssets[i].sPrimaryType[0] == 'E' )			
		{
			zTotalMV.fMktValForEquity	+= fTemp;

			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fPctEPS1Yr, 3))		
				zTotalMV.fMktValForPctEPS1Yr += fTemp;
	
			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fPctEPS5Yr, 2))		
				zTotalMV.fMktValForPctEPS5Yr += fTemp;

			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fBookValue, 2) &&		
				  !IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fClosePrice, 6))
				zTotalMV.fMktValForPriceBook += fTemp;

			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fSales, 0) &&
					!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fSharesOutstand, 0)	&&			
				  !IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fClosePrice, 6))
					zTotalMV.fMktValForPriceSales += fTemp;
	
			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fTrail12mPE, 3))		
				zTotalMV.fMktValForTrail12mPE += fTemp;
	
			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fProj12mPE, 3))		
				zTotalMV.fMktValForProj12mPE += fTemp;

			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fSharesOutstand, 0)	&&			
				  !IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fClosePrice, 6))
					zTotalMV.fMktValForAvgWtdCap += fTemp;
	
			if (!IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fAnnDivCpn, 7) &&		
				  !IsValueZero(zHoldingsAssetsTable->pzHoldingsAssets[i].fClosePrice, 6))
				zTotalMV.fMktValForDivYield += fTemp;
		}

	} // for each record of zHoldingsAssetsTable

	/*
	** Go through each script in the system and for each one of them, find the appropriate
	** segment type, if the portfolio does not have that segment type, skip the record. 
	** For each segment type, go through all the holdings record and test each one of them
	** to see if they pass current script or not, if they do it should be added to current
	** holdtot record.
	*/
	for (i = 0; i < g_zSHdrDetTable.iNumSHdrDet; i++)
	{
		if (g_zSHdrDetTable.pzSHdrDet[i].zHeader.lScrhdrNo != lScrhdrNo)
		{
			iSegTypeId = g_zSHdrDetTable.pzSHdrDet[i].zHeader.iSegmentTypeID;
		
			// if segment type does not exist for this portfolio, skip this script
			if (!CheckIfSegmentExist(iSegTypeId,pzSegmainTable)) 
				continue;
		}

		iSegLvlId = GetSegmentLevelId(g_zSHdrDetTable.pzSHdrDet[i].zHeader.iSegmentTypeID);
		// skip step if segment is for single security 
		// and this system is not set to keep security segments in Holdtot
		if ((iSegLvlId == SECURITY_SEGMENT) && !zSysSet.bSecurityHoldtot)
			continue;

		bLevel2 = (iSegLvlId == 20);
		
		memset(&zSegmentMV,						0, sizeof(MKTVALUES));
		memset(&zInterSegmentTotals,	0, sizeof(MKTVALUES));
		memset(&zOtherSegmentTotals,	0, sizeof(MKTVALUES));
		memset(&zHoldtot,							0, sizeof(HOLDTOT));
		memset(&zInterHoldtot,				0, sizeof(HOLDTOT));
		memset(&zOtherHoldtot,				0, sizeof(HOLDTOT));

		bAddToHoldtot = FALSE;
		bInternational = FALSE;
		sSecurity[0] = sSecNo[0] = sWi[0] = '\0';

		// go through each holdings/holdcash record
		for (j = 0; j < zHoldingsAssetsTable->iNumHoldingsAssets; j++)
		{
			GetPAssetsFromHoldingsAssets(&zPartAsset, &zLevels, zHoldingsAssetsTable->pzHoldingsAssets[j], lDate);

			// test asset against perf script detail and process if passes
			zErr = lpprTestAsset(zPartAsset, g_zSHdrDetTable.pzSHdrDet[i].pzDetail, g_zSHdrDetTable.pzSHdrDet[i].iNumDetail, 
													 TRUE, zPartAsset.pzDAInfo[0].lDate, zPartPortMain, &bResult, lDate, lDate);
			if (zErr.iBusinessError != 0) 
			{
				lpprInitializePartAsset(&zPartAsset);
				return zErr;
			}

			// if the record does not pass the script, skip it
			if (bResult == FALSE) 
				continue;

			bAddToHoldtot = TRUE;
			AddHoldingsToHoldtot(zHoldingsAssetsTable->pzHoldingsAssets[j], &zHoldtot, lDate, &zSegmentMV);
			
			iInternationalLvlId = GetSegmentLevelId(g_zSHdrDetTable.pzSHdrDet[i].zHeader.iSegmentTypeID);
			strcpy_s(sSecurity, "O");

			if (strcmp(zHoldingsAssetsTable->pzHoldingsAssets[j].sSecXtend, "PL") == 0 && bLevel2)
			{
				iPledgedSegmentId = GetPledgedSegmentId(zHoldingsAssetsTable->pzHoldingsAssets[j].iIndustLevel1, -1);
				if (iPledgedSegmentId != 0)
				{
					if (iPledgeSegment == iPledgedSegmentId)
					{
						AddHoldingsToHoldtot(zHoldingsAssetsTable->pzHoldingsAssets[j],&zPledgedHoldTot[iPlIndex], lDate, &zPledgedHoldTotTotal[iPlIndex]);
					}
					else 
					{
						iPlIndex++;
						AddHoldingsToHoldtot(zHoldingsAssetsTable->pzHoldingsAssets[j],&zPledgedHoldTot[iPlIndex], lDate, &zPledgedHoldTotTotal[iPlIndex]);
						iPledgeSegment = iPledgedSegmentId;
					} // if ipledgedsegment - ipledgedsegmentid
					if (iPledgedSegmentId != 0)
					{
						if (strcmp(sSecNo, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecNo) != 0 || strcmp(sWi, zHoldingsAssetsTable->pzHoldingsAssets[j].sWi) != 0
								|| strcmp(sSecXtend, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecXtend) != 0)
							zPledgedHoldTot[iPlIndex].iNumberofsecurities++;
						strcpy_s(sSecNo, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecNo);
						strcpy_s(sWi, zHoldingsAssetsTable->pzHoldingsAssets[j].sWi);
						strcpy_s(sSecXtend, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecXtend);
						zPledgedHoldTot[iPlIndex].iSegmentTypeID = iPledgedSegmentId;
						strcpy_s(sSecurity,"P");
					} // if ipledgedsegmentid != 0
				} // if pledgedid != 0
			} // if pl and specialflag
				
			if (iInternationalLvlId == INTERNATIONALLEVELID && strcmp(sSecurity, "P") != 0 && bLevel2)
			{
				bInternational = IsAssetInternational(zPartPortMain, zHoldingsAssetsTable->pzHoldingsAssets[j].sCurrId, zHoldingsAssetsTable->pzHoldingsAssets[j].iIndustLevel1, &Level2);
				if (bInternational)
				{
					if (strcmp(sSecNo,zHoldingsAssetsTable->pzHoldingsAssets[j].sSecNo) != 0 || strcmp(sWi,zHoldingsAssetsTable->pzHoldingsAssets[j].sWi) != 0
							|| strcmp(sSecXtend, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecXtend) != 0)
					{
						zInterHoldtot.iNumberofsecurities++;
						zInternationalHoldTot[IntIndex].iNumberofsecurities++;
					}
					strcpy_s(sSecNo, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecNo);
					strcpy_s(sWi, zHoldingsAssetsTable->pzHoldingsAssets[j].sWi);
					strcpy_s(sSecXtend, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecXtend);
					iSegmentId = GetSegmapid(zHoldingsAssetsTable->pzHoldingsAssets[j].iIndustLevel1,Level2,-1);
					if (iSegmentId != 0)
					{ 
						strcpy_s(sSecurity,"I");						
						if (iLevel == iSegmentId)
							AddHoldingsToHoldtot(zHoldingsAssetsTable->pzHoldingsAssets[j],&zInternationalHoldTot[IntIndex], lDate, &zInternationalHoldTotTotal[IntIndex]);//&zInterHoldtot
						else 
						{
							IntIndex++;
							AddHoldingsToHoldtot(zHoldingsAssetsTable->pzHoldingsAssets[j],&zInternationalHoldTot[IntIndex], lDate, &zInternationalHoldTotTotal[IntIndex]);//&zInterHoldtot
							iLevel = iSegmentId;
						} // if isegmentid
					} // if isegmentid <> 0
				} // if binternational
			} // if international and not pledged

			if (strcmp(sSecurity,"O") == 0) 
			{
				AddHoldingsToHoldtot(zHoldingsAssetsTable->pzHoldingsAssets[j], &zOtherHoldtot, lDate, &zOtherSegmentTotals);
				 // Note that even though current secno and wi is bieng compared to previous values,
				 // current secxtend is not. Reason is USD, IN & RP should be treated as same, but PL
				 // and RP/IN should be treated differently, that's why if previous secxtend is PL, then
				 // it does not matter what current sec_xtend is (it can only be RP/IN), it should be 
				 // considered a different segment.
				if (strcmp(sSecNo,zHoldingsAssetsTable->pzHoldingsAssets[j].sSecNo) != 0 || 
					  strcmp(sWi,zHoldingsAssetsTable->pzHoldingsAssets[j].sWi) != 0 ||
					  strcmp(sSecXtend, "PL") == 0)
				{
					zOtherHoldtot.iNumberofsecurities++;
					
					if (zOtherSegmentTotals.bIncludeMedianCap) 
						pStats->AddValue(zOtherHoldtot.fMedianCap);
				}

				strcpy_s(sSecNo, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecNo);
				strcpy_s(sWi, zHoldingsAssetsTable->pzHoldingsAssets[j].sWi);
				strcpy_s(sSecXtend, zHoldingsAssetsTable->pzHoldingsAssets[j].sSecXtend);
			} // if other
		} // for each asset

		if (bAddToHoldtot)
		{
			if (zInterHoldtot.iNumberofsecurities > 0)
			{
				FillAdditionalHoldTotData(&zInterHoldtot, zInterSegmentTotals, zTotalMV); 
				
				zInterHoldtot.iId = zPartPortMain.iID;
				zInternationalHoldTot[IntIndex].iId = zPartPortMain.iID;
				
				zInterHoldtot.iSegmentTypeID = iSegmentId;
				zInternationalHoldTot[IntIndex].iSegmentTypeID = iSegmentId;
				if (zErr.iBusinessError != 0)
				{	
					lpprInitializePartAsset(&zPartAsset);
					return zErr;
				}

				zInterHoldtot.iScrhdrNo = g_zSHdrDetTable.pzSHdrDet[i].zHeader.lScrhdrNo;
				zInternationalHoldTot[IntIndex].iScrhdrNo = g_zSHdrDetTable.pzSHdrDet[i].zHeader.lScrhdrNo;
			}
			
			if (zPledgedHoldTot[iPlIndex].iNumberofsecurities > 0)
			{
				zPledgedHoldTot[iPlIndex].iId = zPartPortMain.iID;
				zPledgedHoldTot[iPlIndex].iScrhdrNo = g_zSHdrDetTable.pzSHdrDet[i].zHeader.lScrhdrNo;
			}

			if (zOtherHoldtot.iNumberofsecurities > 0 )
			{
				// For Total segment
				FillAdditionalHoldTotData(&zHoldtot, zSegmentMV, zTotalMV); 
				FillAdditionalHoldTotData(&zOtherHoldtot, zOtherSegmentTotals, zTotalMV);

				zOtherHoldtot.iId = zPartPortMain.iID;
				zOtherHoldtot.iSegmentTypeID = g_zSHdrDetTable.pzSHdrDet[i].zHeader.iSegmentTypeID;
				zOtherHoldtot.iScrhdrNo = g_zSHdrDetTable.pzSHdrDet[i].zHeader.lScrhdrNo;

				pStats->Median(&zOtherHoldtot.fMedianCap);
				pStats->Clear();
				
				if (RoundDouble(zOtherHoldtot.fMedianCap, 0) == NAVALUE) 
					zOtherHoldtot.fMedianCap = 0;
				
				if (zOtherHoldtot.iNumberofsecurities > 0)
				{		
					lpprInsertHoldtot(zOtherHoldtot, &zErr);
					memset(&zOtherHoldtot,0,sizeof(HOLDTOT));
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
					{	
						lpprInitializePartAsset(&zPartAsset);
						return zErr;
					}
				}
			}
		} // if bAddHoldTot
	} // for zshdrdettable.inumshdrdet

	for (i = 1; i <= IntIndex; i++)
	{
		FillAdditionalHoldTotData(&zInternationalHoldTot[i], zInternationalHoldTotTotal[i], zTotalMV);
		lpprInsertHoldtot(zInternationalHoldTot[i], &zErr);
		memset(&zInternationalHoldTot[i],0,sizeof(HOLDTOT));
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
		{
			lpprInitializePartAsset(&zPartAsset);
			return zErr;
		}
	}
	
	// processing unsupervised
	iUnsupervisedSegmentId = GetUnsupervisedSegmentId();
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
	{
		lpprInitializePartAsset(&zPartAsset);
		return zErr;
	}
	
	lpprSelectUnsupervised(zPartPortMain.iID, UNSUPERVISED_SECXTEND, &TotalMktValue, &TotalCost, &zErr);
	if (zErr.iSqlError == SQLNOTFOUND)  
		zErr.iSqlError = 0;
	else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
	{	
		lpprInitializePartAsset(&zPartAsset);
		return zErr;
	}
	else if (!IsValueZero(TotalCost, 3) || !IsValueZero(TotalMktValue , 2))
	{
		zUnsupervisedHoldTot.iId				= zPartPortMain.iID;
		zUnsupervisedHoldTot.iSegmentTypeID		= iUnsupervisedSegmentId;
		zUnsupervisedHoldTot.iScrhdrNo			= 0;
		zUnsupervisedHoldTot.fBaseTotCost		= TotalCost;
		zUnsupervisedHoldTot.fBaseMarketValue	= TotalMktValue;
		lpprInsertHoldtot(zUnsupervisedHoldTot, &zErr);
		memset(&zUnsupervisedHoldTot,0,sizeof(HOLDTOT));
	}
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
	{
		lpprInitializePartAsset(&zPartAsset);
		return zErr;
	}

	// inserting pledged
	for (i = 1; i <= iPlIndex; i++)
	{
		if (zPledgedHoldTot[i].iNumberofsecurities > 0) 
		{
			FillAdditionalHoldTotData(&zPledgedHoldTot[i], zPledgedHoldTotTotal[i], zTotalMV);
			if (zPledgedHoldTot[i].iNumberofsecurities > 0) 
			{		
				lpprInsertHoldtot(zPledgedHoldTot[i], &zErr);
				memset(&zPledgedHoldTot[i],0,sizeof(HOLDTOT));
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0) 
				{
					lpprInitializePartAsset(&zPartAsset);
					return zErr;
				}
			}
		}
	}

	// free up the memory for daily info
	lpprInitializePartAsset(&zPartAsset);

	return zErr;
} // CreateHoldtotRecord

/*==============================================================================================================
** This routine reads all the Holdings and Holdcash record for an account. It searches 
** the Asset table. If the Holdings record is not there, it Selects the
** Asset and adds it to the Asset Table. It creates an entry for the Asset 
** in the Holdings Table. It does the same for Holdcash records.
==============================================================================================================*/ 

ERRSTRUCT GetHoldingsAndAssetTables(int iPortfolioID, int iVendorID, long lDate, HOLDINGSASSETSTABLE *pHoldingsAssetsTable,
																		ASSETTABLE2 *pzATable)
{ 
	HOLDINGSASSETS	zHoldingsAssets;
	ERRSTRUCT				zErr;
	PARTASSET2			zTempAsset;
	LEVELINFO				zLevels;
	char						sLastSecNo[12+NT], sLastWi[1+NT], sLastSecXtend[2+NT], sLastAcctType[1+NT];
	int							iAIdx, iLastPortfolioId = 0;
	long						lLastTransNo = 0;

	lpprInitializeErrStruct(&zErr);

	memset(&zHoldingsAssets,   0, sizeof (HOLDINGSASSETS));
	sLastSecNo[0] = sLastWi[0] = sLastSecXtend[0] = sLastAcctType[0] = '\0';
	
	zTempAsset.iDailyCount = 0;
	lpprInitializePartAsset(&zTempAsset);

	// allocate memory for one day
	zTempAsset.iDailyCount = 1;
	zErr = lpfnAllocateMemoryToDailyAssetInfo(&zTempAsset);
	if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		return zErr;

	while (!zErr.iSqlError)
	{
		lpprInitializeErrStruct(&zErr);
		memset(&zHoldingsAssets,   0, sizeof (HOLDINGSASSETS));
		lpprSelectAllForHoldTot(iPortfolioID, lDate, &zHoldingsAssets, iVendorID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError || zErr.iBusinessError)
		{
			lpprInitializePartAsset(&zTempAsset);
			return(lpfnPrintError("Error Reading Holdings and assets Table ", iPortfolioID, 0, "", zErr.iBusinessError, zErr.iSqlError, zErr.iIsamCode, "HOLDTOT GETHOLDINGSASSETS1", FALSE));
		}
		
		GetPAssetsFromHoldingsAssets(&zTempAsset, &zLevels, zHoldingsAssets, lDate);

    if (zHoldingsAssets.fMktVal < 0.0)
      zTempAsset.iLongShort = LONGSH_SHORT;
    else
      zTempAsset.iLongShort = LONGSH_LONG;

    iAIdx = lpfnFindAssetInTable(*pzATable, zHoldingsAssets.sSecNo, zHoldingsAssets.sWi, TRUE, zTempAsset.iLongShort);
    if (iAIdx < 0 || iAIdx >= pzATable->iNumAsset)
    {
      zErr = lpfnAddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &iAIdx, lDate, lDate);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			{
				lpprInitializePartAsset(&zTempAsset);
        return zErr;
			}
		}

		/*If there is a duplicate fetch next records and discard the duplicate record*/
		if (iLastPortfolioId == zHoldingsAssets.iID && strcmp(sLastSecNo,zHoldingsAssets.sSecNo) == 0 && strcmp(sLastWi,zHoldingsAssets.sWi) == 0 && 
				strcmp(sLastSecXtend,zHoldingsAssets.sSecXtend) == 0 && lLastTransNo == zHoldingsAssets.lTransNo && strcmp(sLastAcctType,zHoldingsAssets.sAcctType) ==0)
			continue;

		/*Save the last Record info to skip the duplicate record*/
		iLastPortfolioId = zHoldingsAssets.iID;
		lLastTransNo= zHoldingsAssets.lTransNo;
		strcpy_s(sLastSecNo,zHoldingsAssets.sSecNo);
		strcpy_s(sLastWi,zHoldingsAssets.sWi);
		strcpy_s(sLastSecXtend,zHoldingsAssets.sSecXtend);
		strcpy_s(sLastAcctType,zHoldingsAssets.sAcctType);
		
		/*if asset is from holdcash calculate yield,annualincome,yieldtomaturity */
		if (strcmp(zHoldingsAssets.sTableName, "C") == 0)
		{
			zHoldingsAssets.fMktCurYld = zHoldingsAssets.fYieldToMaturity;//zHoldingsAssets.fAnnDivCpn / zHoldingsAssets.fClosePrice;
			zHoldingsAssets.fMktEffMatYld = zHoldingsAssets.fYieldToMaturity;//zHoldingsAssets.fAnnDivCpn / zHoldingsAssets.fClosePrice;
			zHoldingsAssets.fAnnualIncome = zHoldingsAssets.fUnits * zHoldingsAssets.fAnnDivCpn * zHoldingsAssets.fTradUnit / 100;
		}/**/
		
		// Unsupervised, tech-short and tech-long don't get included in the calculations
		if (strcmp(zHoldingsAssets.sSecXtend, "UP") == 0) continue;

		if (zHoldingsAssets.fMktVal < 0.0)
			zHoldingsAssets.iLongShort = LONGSH_SHORT;
		else
			zHoldingsAssets.iLongShort = LONGSH_LONG;

		//zHoldingsAssets.iSTypeIndex is initialized to -1 in StarsIO, will be loaded here later


    if (zHoldingsAssets.fMvBaseXrate == 0.0)
      return(lpfnPrintError("Invalid Mv Base Xrate", iPortfolioID, 0, 
														"T", 67, 0, 0, "HOLDTOT GETHOLDINGSASSET2", FALSE));
 
    if (zHoldingsAssets.fAiBaseXrate == 0.0)
      return(lpfnPrintError("Invalid Ai Base Xrate", iPortfolioID, 0, 
														"T", 123, 0, 0, "HOLDTOT GETHOLDINSASSET3", FALSE));
 
    if (zHoldingsAssets.fBaseCostXrate == 0.0)
      return(lpfnPrintError("Invalid Base Cost Xrate", iPortfolioID, 0, 
														"T", 123, 0, 0, "HOLDTOT GETHOLDINSASSET4", FALSE));

		zErr = AddHoldingsAssetsToTable(pHoldingsAssetsTable, zHoldingsAssets);    
		if (zErr.iBusinessError != 0)
		{
			lpprInitializePartAsset(&zTempAsset);
      return zErr;
		}
	} /* while no error */

	// free up the memory that was allocated
	lpprInitializePartAsset(&zTempAsset);

  return zErr;
} // GetHoldingsAndAssetTables 


//  This routine creates the Holdtot Record from the Holdings Record.
void AddHoldingsToHoldtot(HOLDINGSASSETS zHoldingsAssets, HOLDTOT *pzHoldtot, long lDate, MKTVALUES *pzSegmentMV)
{
	double iRatingVal = 0;
	char sCheckFor;

	// If it is a equity, make the yield and units 0 (shobhit)
	if (zHoldingsAssets.sPrimaryType[0] == 'E')
	{
//		zHoldingsAssets.fMktEffMatYld = 0; 6/14/00 Changed back because this causes a descripency between CAP and Analytical Summary
		zHoldingsAssets.fUnits = 0;
	}

	// median capitalization is not cumulative 
	// and, therefore, has to be reset each time
	pzHoldtot->fMedianCap = 0;
	pzSegmentMV->bIncludeMedianCap = FALSE;

	// Values in Native currency
	pzSegmentMV->fNativeMktVal		+= zHoldingsAssets.fMktVal;
	pzHoldtot->fNativeCostYield		+= zHoldingsAssets.fCostEffMatYld * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
	pzHoldtot->fNativeCurrYield		+= zHoldingsAssets.fMktCurYld * zHoldingsAssets.fMktVal/zHoldingsAssets.fMvBaseXrate;
	pzHoldtot->fNativeOrigCost		+= zHoldingsAssets.fOrigCost;
	pzHoldtot->fNativeTotCost			+= zHoldingsAssets.fTotCost;
	pzHoldtot->fNativeMarketValue	+= zHoldingsAssets.fMktVal;
	pzHoldtot->fNativeAccrual			+= zHoldingsAssets.fAccrInt;
  pzHoldtot->fNativeGainorloss	+= zHoldingsAssets.fSecurityGl + zHoldingsAssets.fCurrencyGl + zHoldingsAssets.fSecurityGl;
	pzHoldtot->fNativeAccrGorl		+= zHoldingsAssets.fAccrualGl;
	pzHoldtot->fNativeCurrGorl		+= zHoldingsAssets.fCurrencyGl;
	pzHoldtot->fNativeSecGorl			+= zHoldingsAssets.fSecurityGl;
	pzHoldtot->fNativeIncome			+= zHoldingsAssets.fAnnualIncome;	
	pzHoldtot->fNativeParvalue		+= zHoldingsAssets.fUnits;

	// Values in base currency
	if(!IsValueZero(zHoldingsAssets.fMvBaseXrate,12))
	{
		pzSegmentMV->fBaseMktVal		+= zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
		pzHoldtot->fBaseMarketValue	+= zHoldingsAssets.fMktVal/zHoldingsAssets.fMvBaseXrate;
		pzHoldtot->fBaseCurrGorl		+= zHoldingsAssets.fCurrencyGl / zHoldingsAssets.fMvBaseXrate;
		pzHoldtot->fBaseSecGorl			+= zHoldingsAssets.fSecurityGl / zHoldingsAssets.fMvBaseXrate;
		pzHoldtot->fBaseCostYield		+= zHoldingsAssets.fCostEffMatYld* zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
		pzHoldtot->fBaseParvalue		+= zHoldingsAssets.fUnits; 
			
		// If WeightedStatisticsFlag determines whether the market value for this security is added 
		// to total(for figuring out average of the statistics) or not if the statistics is zero
		if (zSysSet.zSyssetng.sWeightedStatisticsFlag[0] == STATISTICSFLAG_2)
		{
			// Yield To Maturity
			if (!IsValueZero(zHoldingsAssets.fMktEffMatYld, 5) || zSysSet.iWeightedStatisticsExcludeYTM == 2)
			{
				pzHoldtot->fCurrentYieldtomaturity	+= zHoldingsAssets.fMktEffMatYld * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForYTM += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Current Yield
			if (!IsValueZero(zHoldingsAssets.fMktCurYld, 5))
			{
				pzHoldtot->fBaseCurrYield	+= zHoldingsAssets.fMktCurYld * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForCurYield += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Coupon Rate
			if (!IsValueZero(zHoldingsAssets.fCouponRate, 5))
			{
				pzHoldtot->fCouponRate	+= zHoldingsAssets.fCouponRate * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForCoupon += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Duration - Right now this is special - even if the duration is zero, its mkt val is 
			// being used to calculate average unlike any other statistics.
			if (zHoldingsAssets.sPrimaryType[0] == 'B' || zHoldingsAssets.sPrimaryType[0] == 'C' || zHoldingsAssets.sPrimaryType[0] == 'M') 
			{
				pzHoldtot->fDuration	+= zHoldingsAssets.fDuration * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForDuration += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}
			// Rating
			//iRatingVal = GetNumericalRating(zHoldingsAssets.sRating, zHoldingsAssets.sPrimaryType[0]);
			//if (iRatingVal != 0)
			//{
			//	pzHoldtot->fRating	+=  (iRatingVal * zHoldingsAssets.fMktVal) / zHoldingsAssets.fMvBaseXrate;
			//	pzSegmentMV->fMktValForRatings += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			//}
			// Rating
			// GK Changed 2000-11-14
			if (zHoldingsAssets.sPrimaryType[0] == 'E') 
			   sCheckFor = zSysSet.zSyssetng.sEquityRating[0];
			else 
			   sCheckFor = zSysSet.zSyssetng.sFixedRating[0];

			if (sCheckFor == 'M')
			{ 
				iRatingVal = GetNumericalRating(zHoldingsAssets.sMoodyRating, zHoldingsAssets.sPrimaryType[0]);  
			}
			if (sCheckFor == 'S' || sCheckFor == 'B') 
			{
				iRatingVal = GetNumericalRating(zHoldingsAssets.sSpRating, zHoldingsAssets.sPrimaryType[0]);
			}
			if (iRatingVal != 0)
			{
				pzHoldtot->fRating	+=  (iRatingVal * (zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate));
				pzSegmentMV->fMktValForRatings += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Maturity
			if (!IsValueZero(zHoldingsAssets.fMaturity, 3))
			{
				pzHoldtot->fMaturity	+= zHoldingsAssets.fMaturity * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForMaturity += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Maturity Real
			if (!IsValueZero(zHoldingsAssets.fMaturityReal, 3))
			{
				pzHoldtot->fMaturityReal	+= zHoldingsAssets.fMaturityReal * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForMaturityReal += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// equity stats
			if (zHoldingsAssets.sPrimaryType[0] == 'E')
			{
				double fTemp;
				fTemp = zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForEquity += fTemp;

				if (pzHoldtot->iNumberofsecurities <= 10) // limit to top 10 equity holdings
					pzHoldtot->fPcTopTenHold	+= fTemp;

				if (!IsValueZero(zHoldingsAssets.fPctEPS1Yr, 3))		
				{
					pzHoldtot->fPctEPS1Yr += zHoldingsAssets.fPctEPS1Yr * fTemp;
					pzSegmentMV->fMktValForPctEPS1Yr += fTemp;
				}
	
				if (!IsValueZero(zHoldingsAssets.fPctEPS5Yr, 2))		
				{
					pzHoldtot->fPctEPS5Yr	+= zHoldingsAssets.fPctEPS5Yr * fTemp;				
					pzSegmentMV->fMktValForPctEPS5Yr += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fBeta, 2))		
				{
					pzHoldtot->fBeta	+= zHoldingsAssets.fBeta * fTemp;				
					pzSegmentMV->fMktValForBeta += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fBookValue, 2) &&		
						!IsValueZero(zHoldingsAssets.fClosePrice, 6))
				{
					pzHoldtot->fPriceBook	+= zHoldingsAssets.fClosePrice / zHoldingsAssets.fBookValue * fTemp; 				
					pzSegmentMV->fMktValForPriceBook += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fPctEPS1Yr, 3) &&		
						!IsValueZero(zHoldingsAssets.fEquityPerShare, 2))
				{
					pzHoldtot->fROE	+= zHoldingsAssets.fPctEPS1Yr / zHoldingsAssets.fEquityPerShare * fTemp; 				
					pzSegmentMV->fMktValForROE += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fSales, 0) &&
						!IsValueZero(zHoldingsAssets.fSharesOutstand, 0)	&&			
						!IsValueZero(zHoldingsAssets.fClosePrice, 6))
				{
					pzHoldtot->fPriceSales += zHoldingsAssets.fClosePrice * zHoldingsAssets.fSharesOutstand * fTemp /
																		zHoldingsAssets.fSales;
					pzSegmentMV->fMktValForPriceSales += fTemp;
				}
	
				if (!IsValueZero(zHoldingsAssets.fTrail12mPE, 3))		
				{
					pzHoldtot->fTrail12mPE += zHoldingsAssets.fTrail12mPE * fTemp;
					pzSegmentMV->fMktValForTrail12mPE += fTemp;
				}
	
				if (!IsValueZero(zHoldingsAssets.fProj12mPE, 3))		
				{
					pzHoldtot->fProj12mPE += zHoldingsAssets.fProj12mPE * fTemp;
					pzSegmentMV->fMktValForProj12mPE += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fSharesOutstand, 0)	&&			
					  !IsValueZero(zHoldingsAssets.fClosePrice, 6))
				{
					pzHoldtot->fAvgWtdCap += zHoldingsAssets.fClosePrice * zHoldingsAssets.fSharesOutstand * fTemp;
					pzSegmentMV->fMktValForAvgWtdCap += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fAnnDivCpn, 7) &&		
					  !IsValueZero(zHoldingsAssets.fClosePrice, 6))
				{
					pzHoldtot->fDivYield += zHoldingsAssets.fAnnDivCpn / zHoldingsAssets.fClosePrice * 100 * fTemp;
					pzSegmentMV->fMktValForDivYield += fTemp;
				}			

				// Median Cap value is treated differently
				if (!IsValueZero(zHoldingsAssets.fSharesOutstand, 0)	&&			
					  !IsValueZero(zHoldingsAssets.fClosePrice, 6))
				{
					pzHoldtot->fMedianCap = zHoldingsAssets.fClosePrice * zHoldingsAssets.fSharesOutstand;
					pzSegmentMV->bIncludeMedianCap = TRUE;
				} 
			}
		} // don't want to add market value if statistics is zero
		else /*if (zSysSettings.sWeightedStatisticsFlag[0] == STATISTICSFLAG_1)*/
		{
			// YTM
			pzHoldtot->fCurrentYieldtomaturity	+= zHoldingsAssets.fMktEffMatYld * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			pzSegmentMV->fMktValForYTM += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;

			// Current Yield
			pzHoldtot->fBaseCurrYield	+= zHoldingsAssets.fMktCurYld * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			pzSegmentMV->fMktValForCurYield += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;

			// Coupon Rate
			pzHoldtot->fCouponRate	+= zHoldingsAssets.fCouponRate * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			pzSegmentMV->fMktValForCoupon += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;

			// Duration
			pzHoldtot->fDuration	+= zHoldingsAssets.fDuration * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			pzSegmentMV->fMktValForDuration += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;

			// Ratings
			//iRatingVal = GetNumericalRating(zHoldingsAssets.sRating, zHoldingsAssets.sPrimaryType[0]);
			//pzHoldtot->fRating	+=  (iRatingVal * zHoldingsAssets.fMktVal) / zHoldingsAssets.fMvBaseXrate;
			//pzSegmentMV->fMktValForRatings += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			// Ratings
			// GK Changed 2000-11-14
			if (zHoldingsAssets.sPrimaryType[0] == 'E') 
			   sCheckFor = zSysSet.zSyssetng.sEquityRating[0];
			else 
			   sCheckFor = zSysSet.zSyssetng.sFixedRating[0];

			if (sCheckFor == 'M')
			{
				iRatingVal = GetNumericalRating(zHoldingsAssets.sMoodyRating, zHoldingsAssets.sPrimaryType[0]);
			}

			if (sCheckFor == 'S' || sCheckFor == 'B') 
			{
			   iRatingVal = GetNumericalRating(zHoldingsAssets.sSpRating, zHoldingsAssets.sPrimaryType[0]);
			}

			if ((iRatingVal != 0 || zSysSet.iWeightedStatisticsExcludeRAT == 2) || (zHoldingsAssets.sPrimaryType[0] != 'M' && zHoldingsAssets.sPrimaryType[0] != 'C'))
			{
				pzHoldtot->fRating	+=  (iRatingVal * (zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate));
				pzSegmentMV->fMktValForRatings += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Maturity
			if ((zHoldingsAssets.fMaturity != 0 || zSysSet.iWeightedStatisticsExcludeRAT == 2) || (zHoldingsAssets.sPrimaryType[0] != 'M' && zHoldingsAssets.sPrimaryType[0] != 'C'))
			{
				pzHoldtot->fMaturity	+= zHoldingsAssets.fMaturity * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForMaturity += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// Maturity Real
			if ((zHoldingsAssets.fMaturityReal != 0 || zSysSet.iWeightedStatisticsExcludeRAT == 2) || (zHoldingsAssets.sPrimaryType[0] != 'M' && zHoldingsAssets.sPrimaryType[0] != 'C'))
			{
				pzHoldtot->fMaturityReal	+= zHoldingsAssets.fMaturityReal * zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForMaturityReal += zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
			}

			// equity stats
			if (zHoldingsAssets.sPrimaryType[0] == 'E')
			{
				double fTemp;
				fTemp = zHoldingsAssets.fMktVal / zHoldingsAssets.fMvBaseXrate;
				pzSegmentMV->fMktValForEquity += fTemp;

				if (pzHoldtot->iNumberofsecurities < 10) 
					pzHoldtot->fPcTopTenHold	+= fTemp;

				pzHoldtot->fPctEPS1Yr += zHoldingsAssets.fPctEPS1Yr * fTemp;
				pzSegmentMV->fMktValForPctEPS1Yr += fTemp;
	
				pzHoldtot->fPctEPS5Yr	+= zHoldingsAssets.fPctEPS5Yr * fTemp;				
				pzSegmentMV->fMktValForPctEPS5Yr += fTemp;

				pzHoldtot->fBeta	+= zHoldingsAssets.fBeta * fTemp;				
				pzSegmentMV->fMktValForBeta += fTemp;

				if (!IsValueZero(zHoldingsAssets.fBookValue, 2))
				{
					pzHoldtot->fPriceBook	+= zHoldingsAssets.fClosePrice / zHoldingsAssets.fBookValue * fTemp; 				
					pzSegmentMV->fMktValForPriceBook += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fEquityPerShare, 2))
				{
					pzHoldtot->fROE	+= zHoldingsAssets.fPctEPS1Yr / zHoldingsAssets.fEquityPerShare * fTemp; 				
					pzSegmentMV->fMktValForROE += fTemp;
				}

				if (!IsValueZero(zHoldingsAssets.fSales, 0))
				{
					pzHoldtot->fPriceSales += zHoldingsAssets.fClosePrice * zHoldingsAssets.fSharesOutstand * fTemp /
																		zHoldingsAssets.fSales;
					pzSegmentMV->fMktValForPriceSales += fTemp;
				}
	
				pzHoldtot->fTrail12mPE += zHoldingsAssets.fTrail12mPE * fTemp;
				pzSegmentMV->fMktValForTrail12mPE += fTemp;
	
				pzHoldtot->fProj12mPE += zHoldingsAssets.fProj12mPE * fTemp;
				pzSegmentMV->fMktValForProj12mPE += fTemp;

				pzHoldtot->fAvgWtdCap += zHoldingsAssets.fClosePrice * zHoldingsAssets.fSharesOutstand * fTemp;
				pzSegmentMV->fMktValForAvgWtdCap += fTemp;

				if (!IsValueZero(zHoldingsAssets.fClosePrice, 6))
				{
					pzHoldtot->fDivYield += zHoldingsAssets.fAnnDivCpn / zHoldingsAssets.fClosePrice * 100 * fTemp;
					pzSegmentMV->fMktValForDivYield += fTemp;
				}			

				// Median Cap value is treated differently
				pzHoldtot->fMedianCap = zHoldingsAssets.fClosePrice * zHoldingsAssets.fSharesOutstand;
				pzSegmentMV->bIncludeMedianCap = TRUE;
			}
		}
	} // if base mv exrate is not zero

	if(!IsValueZero(zHoldingsAssets.fBaseCostXrate,12))
	{
		pzHoldtot->fBaseOrigCost	+= zHoldingsAssets.fOrigCost/zHoldingsAssets.fBaseCostXrate;
		pzHoldtot->fBaseTotCost		+= zHoldingsAssets.fTotCost/zHoldingsAssets.fBaseCostXrate;
	}
	if (!IsValueZero(zHoldingsAssets.fAiBaseXrate,12) && !IsValueZero(zHoldingsAssets.fMvBaseXrate,12))
		pzHoldtot->fBaseGainorloss	+= zHoldingsAssets.fAccrualGl/zHoldingsAssets.fAiBaseXrate
																+ zHoldingsAssets.fCurrencyGl/zHoldingsAssets.fMvBaseXrate
																+ zHoldingsAssets.fSecurityGl/zHoldingsAssets.fMvBaseXrate;
	
	if(!IsValueZero(zHoldingsAssets.fAiBaseXrate,12))
	{
		pzHoldtot->fBaseAccrual		+= zHoldingsAssets.fAccrInt/zHoldingsAssets.fAiBaseXrate;
		pzHoldtot->fBaseAccrGorl	+= zHoldingsAssets.fAccrualGl/zHoldingsAssets.fAiBaseXrate;
		pzHoldtot->fBaseIncome		+= zHoldingsAssets.fAnnualIncome/zHoldingsAssets.fAiBaseXrate;
	}
  

	// Values in system currency
	pzHoldtot->fSystemParvalue		+= zHoldingsAssets.fUnits; 

	if (!IsValueZero(zHoldingsAssets.fSysCostXrate,12))
	{
		pzHoldtot->fSystemOrigCost	+= zHoldingsAssets.fOrigCost/zHoldingsAssets.fSysCostXrate;
		pzHoldtot->fSystemTotCost		+= zHoldingsAssets.fTotCost/zHoldingsAssets.fSysCostXrate;
	}

	if (!IsValueZero(zHoldingsAssets.fAiSysXrate,12) && !IsValueZero(zHoldingsAssets.fMvSysXrate,12))
	{
		pzHoldtot->fSystemGainorloss	+= zHoldingsAssets.fAccrualGl/zHoldingsAssets.fAiSysXrate
																	+ zHoldingsAssets.fCurrencyGl/zHoldingsAssets.fMvSysXrate
																	+ zHoldingsAssets.fSecurityGl/zHoldingsAssets.fMvSysXrate;
	}

	if (!IsValueZero(zHoldingsAssets.fAiSysXrate,12))
	{
		pzHoldtot->fSystemAccrGorl	+= zHoldingsAssets.fAccrualGl/zHoldingsAssets.fAiSysXrate;
		pzHoldtot->fSystemIncome		+= zHoldingsAssets.fAnnualIncome/zHoldingsAssets.fAiSysXrate;
		pzHoldtot->fSystemAccrual		+= zHoldingsAssets.fAccrInt/zHoldingsAssets.fAiSysXrate;
	}
	
	if (!IsValueZero(zHoldingsAssets.fMvSysXrate,12))
	{
		pzSegmentMV->fSysMktVal				+= zHoldingsAssets.fMktVal / zHoldingsAssets.fMvSysXrate;
		pzHoldtot->fSystemCurrGorl		+= zHoldingsAssets.fCurrencyGl/zHoldingsAssets.fMvSysXrate;
		pzHoldtot->fSystemSecGorl			+= zHoldingsAssets.fSecurityGl/zHoldingsAssets.fMvSysXrate;
		pzHoldtot->fSystemMarketValue	+= zHoldingsAssets.fMktVal/zHoldingsAssets.fMvSysXrate;
		pzHoldtot->fSystemCostYield		+= zHoldingsAssets.fCostEffMatYld* zHoldingsAssets.fMktVal / zHoldingsAssets.fMvSysXrate;
		pzHoldtot->fSystemCurrYield		+= zHoldingsAssets.fMktCurYld* zHoldingsAssets.fMktVal / zHoldingsAssets.fMvSysXrate;
	}

}//AddHoldingsToHoldtot


/**
** This function copys the asset part of the HoldingsAssets struct
** to PartAsset struct
**/
void GetPAssetsFromHoldingsAssets(PARTASSET2 *pzPartAsset, LEVELINFO *pzLevels, HOLDINGSASSETS zHoldingsAssets, long lDate)
{
	strcpy_s(pzPartAsset->sSecNo, zHoldingsAssets.sSecNo);
	strcpy_s(pzPartAsset->sWhenIssue, zHoldingsAssets.sWi);
	pzPartAsset->iSecType = zHoldingsAssets.iSecType;
	strcpy_s(pzPartAsset->sCurrId, zHoldingsAssets.sCurrId);
	strcpy_s(pzPartAsset->sIncCurrId, zHoldingsAssets.sIncCurrId);
	strcpy_s(pzPartAsset->sSpRating, zHoldingsAssets.sSpRating);
	strcpy_s(pzPartAsset->sMoodyRating, zHoldingsAssets.sMoodyRating);
	strcpy_s(pzPartAsset->sInternalRating, zHoldingsAssets.sNbRating);
	pzPartAsset->fAnnDivCpn = zHoldingsAssets.fAnnDivCpn;
	pzPartAsset->fCurExrate = zHoldingsAssets.fCurExrate;
	pzPartAsset->iSTypeIndex = zHoldingsAssets.iSTypeIndex;
	pzPartAsset->iLongShort = zHoldingsAssets.iLongShort;
	strcpy_s(pzPartAsset->sSecDesc1, zHoldingsAssets.sSecDesc1);
	strcpy_s(pzPartAsset->sCountryIss, zHoldingsAssets.sCountryIss);
	strcpy_s(pzPartAsset->sCountryIsr, zHoldingsAssets.sCountryIsr);
	pzLevels->iIndustLevel1 = zHoldingsAssets.iIndustLevel1;
	pzLevels->iIndustLevel2 = zHoldingsAssets.iIndustLevel2;
	pzLevels->iIndustLevel3 = zHoldingsAssets.iIndustLevel3;
	pzLevels->lEffDate1 = pzLevels->lEffDate2 = pzLevels->lEffDate3 = 0;
	if (pzPartAsset->iDailyCount > 0 && pzPartAsset->pzDAInfo != NULL)
	{
		pzPartAsset->pzDAInfo[0].lDate = lDate;
		pzPartAsset->pzDAInfo[0].iIndustLevel1 = zHoldingsAssets.iIndustLevel1;
		pzPartAsset->pzDAInfo[0].iIndustLevel2 = zHoldingsAssets.iIndustLevel2;
		pzPartAsset->pzDAInfo[0].iIndustLevel3 = zHoldingsAssets.iIndustLevel3;
	}
}

/**
** This function translates char rating to numerical rating
**/
double GetNumericalRating(char * sRating, char sPrimaryType)
{
	int	i;
	char sCheckFor;

	if (sPrimaryType == 'E') 
		sCheckFor = zSysSet.zSyssetng.sEquityRating[0];
	else 
		sCheckFor = zSysSet.zSyssetng.sFixedRating[0];

	if ((sPrimaryType == 'E') && (sCheckFor == 'M'))
		sCheckFor = 'B';

	for (i = 0; i < zRatingTable.iNumRating; i++)
	{
		if (_stricmp(zRatingTable.pzRating[i].sRatingChar, sRating) == 0 && (zRatingTable.pzRating[i].sRatingType[0] == sCheckFor))
			return zRatingTable.pzRating[i].iRatingVal;
	}

	return 0;
}

 
/**
** Function to initialize holdingsAssetstable structure
**/
void InitializeHoldingsAssetsTable(HOLDINGSASSETSTABLE *pzHSTable)
{
  if (pzHSTable->iHoldingsAssetsCreated > 0 && pzHSTable->pzHoldingsAssets != NULL)
    free(pzHSTable->pzHoldingsAssets);
 
  pzHSTable->pzHoldingsAssets = NULL;
  pzHSTable->iNumHoldingsAssets = pzHSTable->iHoldingsAssetsCreated = 0;
} /* initializeholdingsAssetstable */


/**
** Function to initialize perfscriptheader structure
**/
void InitializePerfScrHdr(PSCRHDR *pzSHeader)
{
  pzSHeader->lScrhdrNo = pzSHeader->lTmphdrNo = pzSHeader->lHashKey = 0;
  pzSHeader->sOwner[0] = '\0';
  pzSHeader->lCreateDate = 0;
  pzSHeader->sChangeable[0] = '\0';
  pzSHeader->sDescription[0] = '\0';
  pzSHeader->lChangeDate = 0;
  pzSHeader->sChangedBy[0] = '\0';
} // InitializePerfScrDdr 
 
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
} //InitializePerfScrDet 
 
 
/**
** This function finds the passed script header in the global script header
** table. If it finds it, the index at which it was found is returned else
** -1 is returned.
** /
int FindScrHdrByHdrNo(PSCRHDRDETTABLE zSHdrDetTable, long lSHdrNo)
{
  int i, iIndex;
 
  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zSHdrDetTable.iNumSHdrDet)
  {
    if (zSHdrDetTable.pzSHdrDet[i].zHeader.lScrhdrNo == lSHdrNo)
      iIndex = i;
 
    i++;
  }
 
  return iIndex;
} / * findscrhdrbyhdrno */
 
/**
** This function adds a script header to the global script header detail table,
** if it doesn't already exist there. The third argument is set to the index at
** which it found/added the passed script header.
**/
/*ERRSTRUCT AddScriptHeaderToTable(PSCRHDR zPSHeader, PSCRHDRDETTABLE *pzSHdrDetTable, int *piArrayIndex)
{
  ERRSTRUCT zErr;
 
  lpprInitializeErrStruct(&zErr);
 
  *piArrayIndex = FindScrHdrByHdrNo(*pzSHdrDetTable, zPSHeader.lScrhdrNo);
  if (*piArrayIndex >= 0) // script header already exist in the table 
    return zErr;
 
  // If table is full to its limit, allocate more space 
  if (pzSHdrDetTable->iSHdrDetCreated == pzSHdrDetTable->iNumSHdrDet)
  {
    zSHdrDetTable.iSHdrDetCreated += EXTRASHDRDET;
    zSHdrDetTable.pzSHdrDet= (PSCRHDRDET *)realloc(zSHdrDetTable.pzSHdrDet,
                            zSHdrDetTable.iSHdrDetCreated * sizeof(PSCRHDRDET));
    if (zSHdrDetTable.pzSHdrDet == NULL)
      return(lpfnPrintError("Insufficient Memory For ScrHdrDetTable", 0, 0, "",
                        997, 0, 0, "HOLDTOT ADDSCRHEADER", FALSE));
  }
 
  *piArrayIndex = zSHdrDetTable.iNumSHdrDet++;
  zSHdrDetTable.pzSHdrDet[*piArrayIndex].zHeader = zPSHeader;
  zSHdrDetTable.pzSHdrDet[*piArrayIndex].iDetailCreated = 0;
  zSHdrDetTable.pzSHdrDet[*piArrayIndex].iNumDetail = 0;
  zSHdrDetTable.pzSHdrDet[*piArrayIndex].pzDetail = NULL;
 
  return zErr;
} // AddScrHeaderToTable 
 

/**
** This function adds perf script detail record to a script header-detail
** structure. When the number of details created for the header is zero(the
** function is called with the first detail record for the header), memory for
** elements equal to sequence number of the passed Detail is allocated. After
** that there should be no need to allocate more memory for the key(although
** this function will allocate more memory if need be). The reason for all this
** is that whenevr the records are added in the scriptdetail table, SeqNo is
** alway incremented in sequence, starting at 1 for each header and the cursor
** which retrieves them does that in DESCending order of SeqNo, so the first
** fetched detail record, for each header, should tell us how many detail
** records exist for the header.  The passed detail record is not added to the
** header at the next available space in the array, rather it is added at the
** ith element in the array, where i = SeqNo of the passed detail record.
** /
ERRSTRUCT AddDetailToScrHdrDet(PSCRDET zSDetail, PSCRHDRDET *pzScrHdrDet)
{
  ERRSTRUCT zErr;
  BOOL      bAllocateMemory;
 
  lpprInitializeErrStruct(&zErr);
 
  / *
  ** If the records in detail table(in the database) are entered in right order
  ** (in sequence, starting at 1, for every header) and cursor declaration has
  ** not been changed(ORDER BY scrhdr_no, seq_no DESC) then the first time this
  ** function is called for a header, enough memory will be allocated for all
  ** the details record for that header. But this function should not blow up if
  ** somebody changed the cursor or detail records with wrong seq_no are entered
  * /
  if (pzScrHdrDet->iNumDetail == 0)
  {
    bAllocateMemory = TRUE;
    pzScrHdrDet->iDetailCreated = zSDetail.lSeqNo;
  }
  else if (pzScrHdrDet->iDetailCreated == pzScrHdrDet->iNumDetail)
  {
    bAllocateMemory = TRUE;
    if (zSDetail.lSeqNo > pzScrHdrDet->iDetailCreated)
      pzScrHdrDet->iDetailCreated = zSDetail.lSeqNo;
    else
      pzScrHdrDet->iDetailCreated++;
  }/ * array is full * /
  else
    bAllocateMemory = FALSE;
 
  if (zSDetail.lSeqNo > pzScrHdrDet->iDetailCreated)
    return(lpfnPrintError("Invalid SeqNo in PerfScrDet Table", 0, 0, "", 504, 0, 0,
                      "HOLDTOT ADDSCRDET2", FALSE));
 
  if (bAllocateMemory)
  {
    pzScrHdrDet->pzDetail = (PSCRDET *)realloc(pzScrHdrDet->pzDetail,
                             pzScrHdrDet->iDetailCreated * sizeof(PSCRDET));
    if (pzScrHdrDet->pzDetail == NULL)
      return(lpfnPrintError("Insufficient Memory For PerfScrDet", 0, 0, "", 997,
                        0, 0, "HOLDTOT ADDSCRDET3", FALSE));
  }
 
  pzScrHdrDet->pzDetail[zSDetail.lSeqNo - 1] = zSDetail;
  pzScrHdrDet->iNumDetail++;
 
  return zErr;
} / * adddetailtoscrhdrdet */
 
 
/**
** This function adds a holding record in the holding table. This function does not 
** check whether the passed holding record already exist in the table or not.
**/
ERRSTRUCT AddHoldingsAssetsToTable(HOLDINGSASSETSTABLE *pHoldingsAssetsTable, HOLDINGSASSETS zHoldingsAssets)
{
  ERRSTRUCT zErr;
 
  lpprInitializeErrStruct(&zErr);
 
  /* If table is full to its limit, allocate more space */
  if (pHoldingsAssetsTable->iHoldingsAssetsCreated == pHoldingsAssetsTable->iNumHoldingsAssets)
  {
    pHoldingsAssetsTable->iHoldingsAssetsCreated += EXTRAHOLD;
    pHoldingsAssetsTable->pzHoldingsAssets = (HOLDINGSASSETS *)realloc(pHoldingsAssetsTable->pzHoldingsAssets, pHoldingsAssetsTable->iHoldingsAssetsCreated * sizeof(HOLDINGSASSETS));
    if (pHoldingsAssetsTable->pzHoldingsAssets == NULL)
      return(lpfnPrintError("Insufficient Memory For HoldingsAssetsTable", 0, 0, "", 997, 0, 0, "HOLDTOT ADDHOLDINGSASSETS", FALSE));
  }
 
  pHoldingsAssetsTable->pzHoldingsAssets[pHoldingsAssetsTable->iNumHoldingsAssets] = zHoldingsAssets;
  pHoldingsAssetsTable->iNumHoldingsAssets++;
 
  return zErr;
} /* AddHoldingsAssetsToTable */
 

ERRSTRUCT  LoadRatingTable(RATINGTABLE *pzRatingTable)
{
  ERRSTRUCT zErr;
  RATING   zTempRating;
 
  lpprInitializeErrStruct(&zErr);

	//Allocate memery for zRatingTable
	pzRatingTable->iRatingCreated = NUMRATINGS;
	pzRatingTable->pzRating = (RATING *) realloc(pzRatingTable->pzRating, pzRatingTable->iRatingCreated * sizeof(RATING));
	if (pzRatingTable->pzRating == NULL)
		return(lpfnPrintError("Insufficient Memory For pzRatingTable", 0, 0, "", 997, 0, 0, "HOLDTOT LoadRatingTable", FALSE));


  pzRatingTable->iNumRating = 0;
  while (zErr.iSqlError == 0)
  {
    lpprSelectAllRatings(&zTempRating, &zErr);   
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
      break;
		}
    else if (zErr.iSqlError)
      return(lpfnPrintError("Error Fetching Rating Cursor", 0, 0, "", 0, 
													  zErr.iSqlError, zErr.iIsamCode, "Creating Holdtot1", FALSE));

    if (pzRatingTable->iNumRating == NUMRATINGS)
      return(lpfnPrintError("Rating Table Is Full", 0, 0, "", 997, 0, 0,
														"Creat Holdtot2", FALSE));
      
    pzRatingTable->pzRating[pzRatingTable->iNumRating++] = zTempRating;
  } /* No error in fetching Rating cursor */

  return zErr;
}

void FillAdditionalHoldTotData(HOLDTOT *pzHoldtot, MKTVALUES zMV, MKTVALUES zTotalMV)
{			
	if (!IsValueZero(zTotalMV.fBaseMktVal, 2))
		pzHoldtot->fPercentTotmv = (zMV.fBaseMktVal / zTotalMV.fBaseMktVal) * 100;

	if (!IsValueZero(zMV.fMktValForYTM, 2))
		pzHoldtot->fCurrentYieldtomaturity /= zMV.fMktValForYTM;
		
	if (!IsValueZero(zMV.fMktValForCurYield, 2))
		pzHoldtot->fBaseCurrYield /= zMV.fMktValForCurYield;
		
	if (!IsValueZero(zMV.fMktValForCoupon, 2))	
		pzHoldtot->fCouponRate /= zMV.fMktValForCoupon;
				
	if (!IsValueZero(zMV.fMktValForDuration, 2))
		pzHoldtot->fDuration /= zMV.fMktValForDuration;

	if (!IsValueZero(zMV.fMktValForRatings, 2))
		pzHoldtot->fRating /= zMV.fMktValForRatings;

	if (!IsValueZero(zMV.fMktValForMaturity, 2))
		pzHoldtot->fMaturity /= zMV.fMktValForMaturity;

	if (!IsValueZero(zMV.fMktValForMaturityReal, 2))
		pzHoldtot->fMaturityReal /= zMV.fMktValForMaturityReal;

	if (!IsValueZero(zMV.fBaseMktVal, 2))
	{
		//pzHoldtot->fBaseCurrYield /= zMV.fBaseMktVal;		
		pzHoldtot->fBaseCostYield /= zMV.fBaseMktVal;
	}
		
	if (!IsValueZero(zMV.fNativeMktVal, 2))
	{
		pzHoldtot->fNativeCostYield /= zMV.fNativeMktVal;
		pzHoldtot->fNativeCurrYield /= zMV.fNativeMktVal;
	}
	else
	{
		pzHoldtot->fNativeCostYield = 0;
		pzHoldtot->fNativeCurrYield = 0;
	}

	if (!IsValueZero(zMV.fSysMktVal, 2))
	{
		pzHoldtot->fSystemCostYield /= zMV.fSysMktVal;
		pzHoldtot->fSystemCurrYield /= zMV.fSysMktVal;
	}
	else
	{
		pzHoldtot->fSystemCostYield = 0;
		pzHoldtot->fSystemCurrYield = 0;
	}

	// equity stats
	if (!IsValueZero(zMV.fMktValForEquity, 2))
		pzHoldtot->fPcTopTenHold	= (pzHoldtot->fPcTopTenHold / zMV.fMktValForEquity) * 100;

	if (!IsValueZero(zMV.fMktValForPctEPS1Yr, 2))
		pzHoldtot->fPctEPS1Yr /= zMV.fMktValForPctEPS1Yr;
	
	if (!IsValueZero(zMV.fMktValForPctEPS5Yr, 2))
		pzHoldtot->fPctEPS5Yr	/= zMV.fMktValForPctEPS5Yr;

	if (!IsValueZero(zMV.fMktValForBeta, 2))
		pzHoldtot->fBeta /= zMV.fMktValForBeta;

	if (!IsValueZero(zMV.fMktValForPriceBook, 2))
		pzHoldtot->fPriceBook	/= zMV.fMktValForPriceBook;

	if (!IsValueZero(zMV.fMktValForROE, 2))
		pzHoldtot->fROE	/= zMV.fMktValForROE;

	if (!IsValueZero(zMV.fMktValForPriceSales, 2))
		pzHoldtot->fPriceSales /= zMV.fMktValForPriceSales;
	
	if (!IsValueZero(zMV.fMktValForTrail12mPE, 2))
		pzHoldtot->fTrail12mPE /= zMV.fMktValForTrail12mPE;
	
	if (!IsValueZero(zMV.fMktValForProj12mPE, 2))
		pzHoldtot->fProj12mPE /= zMV.fMktValForProj12mPE;

	if (!IsValueZero(zMV.fMktValForAvgWtdCap, 2))
		pzHoldtot->fAvgWtdCap /= zMV.fMktValForAvgWtdCap;

	if (!IsValueZero(zMV.fMktValForDivYield, 2))
		pzHoldtot->fDivYield /= zMV.fMktValForDivYield;
}

void InitializeSegmentsTable(SEGMENTSTABLE *pzSegmentsTable)
{
	if (pzSegmentsTable->iCapacity > 0 && pzSegmentsTable->pzSegments != NULL)
		free(pzSegmentsTable->pzSegments);

	pzSegmentsTable->pzSegments = NULL;
	pzSegmentsTable->iCapacity = pzSegmentsTable->iCount = 0;
}

ERRSTRUCT AddSegmentToTable(SEGMENTSTABLE  *pzSegmentsTable, SEGMENTS zSegments)
{
	ERRSTRUCT zErr;

	lpprInitializeErrStruct(&zErr);

	if (pzSegmentsTable->iCapacity == pzSegmentsTable->iCount)
	{
		pzSegmentsTable->iCapacity += ALLOCATEDSEGMENTS;
		pzSegmentsTable->pzSegments = (SEGMENTS *) realloc(pzSegmentsTable->pzSegments, sizeof(SEGMENTS) * pzSegmentsTable->iCapacity);
		
		if (pzSegmentsTable->pzSegments == NULL)
			return(lpfnPrintError("Insufficient Memory", 0, 0, "", 999, 0, 0, "HOLDTOT AddSegmentToTable", FALSE));
	}

	pzSegmentsTable->pzSegments[pzSegmentsTable->iCount++] = zSegments;

	return zErr;
} // AddHedgexrefToTable

void InitializeSegmapTable(SEGMAPTABLE *pzSegmapTable)
{
	if (pzSegmapTable->iCapacity > 0 && pzSegmapTable->pzSegmap != NULL)
		free(pzSegmapTable->pzSegmap);

	pzSegmapTable->pzSegmap = NULL;
	pzSegmapTable->iCapacity = pzSegmapTable->iCount = 0;
}

ERRSTRUCT AddSegmapToTable(SEGMAPTABLE *pzSegmapTable, SEGMAP zSegmap)
{
	ERRSTRUCT zErr;

	lpprInitializeErrStruct(&zErr);

	if (pzSegmapTable->iCapacity == pzSegmapTable->iCount)
	{
		pzSegmapTable->iCapacity += ALLOCATEDSEGMAP;
		pzSegmapTable->pzSegmap = (SEGMAP *) realloc(pzSegmapTable->pzSegmap, sizeof(SEGMAP) * pzSegmapTable->iCapacity);
		
		if (pzSegmapTable->pzSegmap == NULL)
			return(lpfnPrintError("Insufficient Memory", 0, 0, "", 999, 0, 0, "HOLDTOT AddSegmapToTable", FALSE));
	}
	
	pzSegmapTable->pzSegmap[pzSegmapTable->iCount++] = zSegmap;

	return zErr;
} // AddHedgexrefToTable

ERRSTRUCT LoadSegmapTable(SEGMAPTABLE *pzSegmapTable)
{
	ERRSTRUCT zErr;
	SEGMAP zSegmap;
  
	lpprInitializeErrStruct(&zErr);
	InitializeSegmapTable(pzSegmapTable);
	while (zErr.iSqlError == 0) 
	{
		lpprSelectAllSegmap(&zSegmap,&zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		AddSegmapToTable(pzSegmapTable,zSegmap);
	}
	
	return zErr;
}

ERRSTRUCT LoadSegmnentsTable(SEGMENTSTABLE *pzSegments)
{
	ERRSTRUCT zErr;
	SEGMENTS zSegments;
  
	lpprInitializeErrStruct(&zErr);
	InitializeSegmentsTable(pzSegments);
	while (zErr.iSqlError == 0) 
	{
		lpprSelectAllSegments(&zSegments,&zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		
		AddSegmentToTable(pzSegments,zSegments);
	}

	return zErr;
}

int GetSegmentLevelId(const int iId)
{

	int i;

	for(i = 0; i < zSegmentsTable.iCount; i++)
	{
		if (iId == zSegmentsTable.pzSegments[i].iID)
			return zSegmentsTable.pzSegments[i].iLevelID;
	}

	return 0;
}

int GetSegmapid(const int iIndustLevel1,const int iIndustLevel2,const int iIndustLevel3)
{
	int j;

	for (j = 0; j < zSegmapTable.iCount; j++)
	{
		if (iIndustLevel1 == zSegmapTable.pzSegmap[j].iSegmentLevel1ID && 
				iIndustLevel2 ==  zSegmapTable.pzSegmap[j].iSegmentLevel2ID && 
				iIndustLevel3 == zSegmapTable.pzSegmap[j].iSegmentLevel3ID)
			return zSegmapTable.pzSegmap[j].iSegmentID;
	}/*for j*/

	return 0;
}


int GetPledgedSegmentId(const int iIndustLevel1,const int iIndustLevel3)
{
	int i,iGroupId;

	for (i = 0; i < zSegmentsTable.iCount; i++)
	{
		if (iIndustLevel1 == 	zSegmentsTable.pzSegments[i].iID)
		{
			iGroupId = zSegmentsTable.pzSegments[i].iGroupID;
			if (iGroupId != 0)
				return GetSegmapid(iIndustLevel1,iGroupId,iIndustLevel3);
		}/*if iIndustLevel1 == */
	}/*for i*/

  return 0;
}

int GetInternational(const int SegmentId)
{
	int i,International=0;
	
	for(i = 0; i < zSegmentsTable.iCount; i++)
	{
		if (SegmentId == zSegmentsTable.pzSegments[i].iID)
		{
			International = zSegmentsTable.pzSegments[i].iInternational;
			return International;
		}
	}/*for*/

	return 0;
}

BOOL IsAssetInternational(PARTPMAIN zPmain, char sCurrId[], const int IndustLevel1, int* Level2)
{
	if (strcmp(sCurrId,zPmain.sBaseCurrId) != 0)
	{
		*Level2 = GetInternational(IndustLevel1);
		/*if international is non zero*/
		if (*Level2 != 0)
			return TRUE;
		else
			return FALSE;
	}/*if bIsItInternational*/
	else
		return FALSE;
}

int GetUnsupervisedSegmentId(void)
{
	int i,UnSupervised=0;
	
	for (i = 0; i < zSegmentsTable.iCount; i++)
	{
		if (zSegmentsTable.pzSegments[i].iLevelID == UNSUPERVISEDLEVELID && zSegmentsTable.pzSegments[i].iSequenceNbr == UNSUPERVISEDSQNNBR)
		{
			UnSupervised = zSegmentsTable.pzSegments[i].iID;
			return UnSupervised;
		}
	}/*for*/

	return 0;
}

void InitializeSegmainTable(SEGMAINTABLE* zSegmainTable)
{
	if (zSegmainTable->iCapacity > 0 && zSegmainTable->pzSegmain != NULL)
		free(zSegmainTable->pzSegmain);

	zSegmainTable->pzSegmain = NULL;
	zSegmainTable->iCapacity = zSegmainTable->iCount = 0;
}

/*This routine gets all the segments for the portfolio and put them in memory table*/
ERRSTRUCT GetAllSegmentsFromSegmainForPortId(int iPortfolioID,SEGMAINTABLE *pzSegmainTable)
{
	SEGMAIN		zSegmain;
	ERRSTRUCT zErr;

	lpprInitializeErrStruct(&zErr);
	InitializeSegmainTable(pzSegmainTable);
	memset(&zSegmain,sizeof(SEGMAIN),0);

	while(zErr.iSqlError == 0)
	{
		lpprSelectSegmainForPortfolio(&zSegmain,iPortfolioID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError !=0 )
			return zErr;

		zErr = AddSegmainToTable(pzSegmainTable, zSegmain);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

	}//while

	return zErr;
}

/*This routine checks if the segment exist in the segmain table*/
int CheckIfSegmentExist(int iSegTypeId,SEGMAINTABLE* pzSegmainTable)
{
	int i;

	for (i = 0; i < pzSegmainTable->iCapacity; i++)
	{
		if (iSegTypeId == pzSegmainTable->pzSegmain[i].iSegmentTypeID)
			return 1;
	}
	return 0;
}

ERRSTRUCT AddSegmainToTable(SEGMAINTABLE *pzSegmainTable, SEGMAIN zSegmain)
{
	ERRSTRUCT zErr;

	lpprInitializeErrStruct(&zErr);

	if (pzSegmainTable->iCount == pzSegmainTable->iCapacity)
	{
		pzSegmainTable->iCapacity += EXTRASEGMAIN;
		pzSegmainTable->pzSegmain = (SEGMAIN *) realloc (pzSegmainTable->pzSegmain, 
			                                               pzSegmainTable->iCapacity * sizeof(SEGMAIN));
		if (pzSegmainTable->pzSegmain == NULL)
			return(lpfnPrintError("Insufficient Memory For Segmain Table", 0, 0, "", 997, 0, 0, "HOLDTOT ADDSEGMAIN", FALSE));
	}
	
	pzSegmainTable->pzSegmain[pzSegmainTable->iCount++] = zSegmain;

	return zErr;
} // AddSegmainToTable 


/**
** This function is used to create segmain records, if required for a portfolio. This function
** is used by createholdtot dll to create segments without running performance. It is written
** in performance dll so that all the functions it needs don't need to be exported. When createholdtot
** dll calls this function, perfruletable has all the standard rules used by the system rather than
** the rules used by the portfolio, this makes sure that even if all levels of performance is
** not being run for this portfolio, it still creates required segments for the portfolio on which
** we need summary values in holdtot table for some reports (e.g. analytical summary and account summary)
**/
ERRSTRUCT CreateSegmainsIfRequired(SEGMAINTABLE *pzSTable, PARTPMAIN zPmain, long lCurrentDate,
																   ASSETTABLE2 zATable, PERFRULETABLE zRuleTable)
{
  ERRSTRUCT   zErr;
  int         i, j, k, m, iIndex;
  PSCRHDRDET  zVirtualSHD;
	SEGMAIN			zSegmain;
  BOOL        bResult, bSecCurr;
 
  lpprInitializeErrStruct(&zErr);
  bResult = FALSE;
 
  /* if no rules are defined for the account, no dynamic key is generated */
  if (zRuleTable.iCount == 0)
    return zErr;

  /*
  ** Examine all rules in the table against all the asset and for each rule-asset combination 
	** check if a segmain record can be generated. If a segmain record can be generated, check if 
	** it already exist, if it does not then create it(both in memory and in the database).
  */
  zVirtualSHD.iDetailCreated = 0;
  for (i = 0; i < zRuleTable.iCount; i++)
  {
    /*
    ** If the rule is for Weighted Average, key will be created based on its
    ** children, not on its own criterion, so ignore the rule.
    */
    if (strcmp(zRuleTable.pzPRule[i].sWeightedRecordIndicator, "W") == 0)
   	  continue;

    /*
    ** If the rule is for Single Security Segment, segment on individual/composite accounts
		** should be created by Performance/Performance Merge, 
		** Holdtot does not need this type of segments
    */
		if (strcmp(zRuleTable.pzPRule[i].sWeightedRecordIndicator, "S") == 0)
   	  continue;

		m = zRuleTable.piTHDIndex[i];
    for (j = 0; j < zATable.iNumAsset; j++)
    {
      /*
      ** Each asset has two currencies defined, security currency and accrual
      ** currency, both of these can generate diferent segmains(if the detail has
      ** defined a selecttype for currency). In most of the cases both these
      ** currencies will be same, do the whole process twice(with different
      ** currencies of course) only if these two currencies are different.
      */
      for (k = 0; k < 2; k++)
      {
        if (k == 0)
          bSecCurr = TRUE;
        else
        {
          bSecCurr = FALSE;
          /* both the currencies are same, break out of the "for k" loop */
          if (strcmp(zATable.pzAsset[j].sCurrId, zATable.pzAsset[j].sIncCurrId) == 0)
            break;
        } /* second pass */
 
        /* create a virtual perf script using current template and asset */
        zErr = lpfnCreateVirtualPerfScript(zTHdrDetTable.pzTHdrDet[m], zATable.pzAsset[j], 0, bSecCurr, &zVirtualSHD);
        if (zErr.iBusinessError != 0)
        {
          lpfnInitializePScrHdrDet(&zVirtualSHD);
          return zErr;
        }
		if (zATable.pzAsset[j].iDailyCount > 0)
		{
        /*
				** Test the asset against virtual perf script. 
				** If it is a special rule(equity+equity cash or fixed+fixed cash), create a key
				** only if portfolio has Equity or Fixed, i.e. if the portfolio has fixed and cash
				** but no equity, only fixed + fixed cash key will be generated for this rule.
				*/
				if (strcmp(zRuleTable.pzPRule[i].sWeightedRecordIndicator, "E") == 0 ||
						strcmp(zRuleTable.pzPRule[i].sWeightedRecordIndicator, "F") == 0)
					zErr = lpprTestAsset(zATable.pzAsset[j], zVirtualSHD.pzDetail, 1, 
															bSecCurr, zATable.pzAsset[j].pzDAInfo[0].lDate, zPmain, &bResult, lCurrentDate, lCurrentDate);
				else
					zErr = lpprTestAsset(zATable.pzAsset[j], zVirtualSHD.pzDetail, zVirtualSHD.iNumDetail, 
															bSecCurr, zATable.pzAsset[j].pzDAInfo[0].lDate, zPmain, &bResult, lCurrentDate, lCurrentDate);
		}
        if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        {
          lpfnInitializePScrHdrDet(&zVirtualSHD);
          return zErr;
        }

        /*
        ** If the asset failed the virtual script test, nothing more to do with
        ** the current perfrule, asset(and currency) combination.
        */
        if (bResult == FALSE)
				{
				  lpfnInitializePScrHdrDet(&zVirtualSHD);
          continue;
				}
 
        /* Asset passed the test, find or create script header for the virtual script */
				zErr = lpfnFindCreateScriptHeader(zPmain.iID, lCurrentDate, zATable, 
																			&g_zSHdrDetTable, &zVirtualSHD, FALSE, 
																			zTHdrDetTable.pzTHdrDet[m].zHeader.lTmphdrNo, j, &iIndex);
        if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        {
					lpfnInitializePScrHdrDet(&zVirtualSHD);
					return zErr;
				}	
        
				/*
        ** Search for the possible  this rule can generate, if not found
        ** in the perfkey table in memory, add it
        */
				if (FindSegmentByType(*pzSTable, g_zSHdrDetTable.pzSHdrDet[iIndex].zHeader.iSegmentTypeID) < 0)
        {
					zSegmain.iID = lpfnAddNewSegment(zPmain.iID, g_zSHdrDetTable.pzSHdrDet[iIndex].zHeader.sDescription,
																					 g_zSHdrDetTable.pzSHdrDet[iIndex].zHeader.sDescription, 
																					 g_zSHdrDetTable.pzSHdrDet[iIndex].zHeader.iSegmentTypeID);
					zSegmain.iSegmentTypeID = g_zSHdrDetTable.pzSHdrDet[iIndex].zHeader.iSegmentTypeID;
					zSegmain.iOwnerID = zPmain.iID;

					zErr = AddSegmainToTable(pzSTable, zSegmain);
          if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          {
            lpfnInitializePScrHdrDet(&zVirtualSHD);
            return zErr;
          }
        } /* if key not found */

			  /* Free up the memory */
			  lpfnInitializePScrHdrDet(&zVirtualSHD);
      } /* for k < 2 */
    } /* j < numasset */
  } /* i < numprule */
 
  return zErr;
} /* CreateSegmentsIfRequired */
 

/*
** This function finds a segment type in the given segments table.
*/
int FindSegmentByType(SEGMAINTABLE zSTable, int iSegmentTypeID)
{
	int i, j;

	j = -1;
	for (i = 0; i < zSTable.iCount; i++)
	{
		if (zSTable.pzSegmain[i].iSegmentTypeID == iSegmentTypeID)
		{
			j = i;
			break;
		}
	}

	return j;
} //FindSegmentByType


ERRSTRUCT GetSystemTables()
{
	ERRSTRUCT zErr;
  PSCRHDR		zSHeader;
  PSCRDET		zSDetail;
  PTMPHDR		zTHeader;
  PTMPDET		zTDetail;
  SYSVALUES	zSysvalues;

	int				iNumRecord, iIndex;

	lpprInitializeErrStruct(&zErr);
	zSystemRules.iCapacity = 0;

	zErr = LoadSegmnentsTable(&zSegmentsTable);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  
	zErr = LoadSegmapTable(&zSegmapTable);	
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  
	zErr = lpfnFillPartSectypeTable(&zPSTypeTable);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;
  
	iNumRecord = 0;
	while (TRUE)
	{
		InitializePerfScrHdr(&zSHeader);
		InitializePerfScrDet(&zSDetail);

		lpprSelectAllScriptHeaderAndDetails(&zSHeader, &zSDetail, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{     
			if (iNumRecord == 0)
				return(lpfnPrintError("No Script Header and Detail Defined Found", 0, 0, "", 0, 
															zErr.iSqlError, zErr.iIsamCode, "HOLDTOT INITLIB", FALSE));
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError)
			return(lpfnPrintError("Error Fetching Script Header and Detail", 0, 0, "", 0, 
														zErr.iSqlError, zErr.iIsamCode, "HOLDTOT INITLIB", FALSE));
		else
		{
			zErr = lpfnAddScriptHeaderToTable(zSHeader, &g_zSHdrDetTable, &iIndex);
			if (zErr.iBusinessError != 0)
				return zErr;

			iNumRecord++;
  
			/*
			** Script header and detail are in an outer join, it is possible to have
			** a script header without any detail record(e.g. Total Portfolio). If
			** indicator variable is -ve, a NULL value for seqno was returned.
			*/
			if (zSDetail.lScrhdrNo == zSHeader.lScrhdrNo)
			{
				// add detail to the found/added header 
		     zErr = lpfnAddDetailToScrHdrDet(zSDetail, &g_zSHdrDetTable.pzSHdrDet[iIndex]);
			   if (zErr.iBusinessError != 0)
			    return zErr;
			} // if a detail record is found 
		} // no error in fetching the record 
	} // while TRUE - for script header and detail
	
  while (TRUE)
  {
    lpprInitializePerfTmpHdr(&zTHeader);
    lpprInitializePerfTmpDet(&zTDetail);
 
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
      zErr = lpfnAddTemplateHeaderToTable(&zTHdrDetTable, zTHeader, &iIndex);
      if (zErr.iBusinessError != 0)
        return zErr;
            
      if (zTDetail.lTmphdrNo == zTHeader.lTmphdrNo)
        /* add detail to the found/added header */
			{ 
				zErr = lpfnAddTemplateDetailToTable(&zTHdrDetTable, zTDetail, iIndex);
        if (zErr.iBusinessError != 0)
          return zErr;
			}
		} // no error in selecting record
	} // get all template headers and details

  
	memset(&zSysSet, sizeof(zSysSet), 0);
	lpprSelectSysSettings(&zSysSet.zSyssetng, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
	  return zErr;

	strcpy_s(zSysvalues.sName, "SecurityHoldtot");
	lpprSelectSysvalues(&zSysvalues, &zErr);
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
	   zSysSet.bSecurityHoldtot = atoi(zSysvalues.sValue);
	else if (zErr.iSqlError == SQLNOTFOUND)
	{
		zSysSet.bSecurityHoldtot = FALSE;
		lpprInitializeErrStruct(&zErr);
	}
  else 
		return zErr;

	strcpy_s(zSysvalues.sName, "WEIGHTEDSTATISTICSEXCLUDEYTM");
	lpprSelectSysvalues(&zSysvalues, &zErr);
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
	{
	  if (strcmp(zSysvalues.sValue, "No") == 0)
		zSysSet.iWeightedStatisticsExcludeYTM = 1;
	  else
		zSysSet.iWeightedStatisticsExcludeYTM = 2;
  }
  else if (zErr.iSqlError == SQLNOTFOUND)
	{
		zSysSet.iWeightedStatisticsExcludeYTM = 0;
		lpprInitializeErrStruct(&zErr);
	}
  else 
		return zErr;


	strcpy_s(zSysvalues.sName, "WEIGHTEDSTATISTICSEXCLUDERAT");
	lpprSelectSysvalues(&zSysvalues, &zErr);
  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
	{
	  if (strcmp(zSysvalues.sValue, "No") == 0)
		zSysSet.iWeightedStatisticsExcludeRAT = 1;
	  else
		zSysSet.iWeightedStatisticsExcludeRAT = 2;
  }
  else if (zErr.iSqlError == SQLNOTFOUND)
	{
		zSysSet.iWeightedStatisticsExcludeRAT = 0;
		lpprInitializeErrStruct(&zErr);
	}
  else 
		return zErr;

  zErr = LoadRatingTable(&zRatingTable);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

	zErr = GetAllPerfrulesForAPortfolio(&zSystemRules, 0, 0);

	return zErr;
} //GetSystemTable

ERRSTRUCT GetAllPerfrulesForAPortfolio(PERFRULETABLE *pzRuleTable, int iID, long lDate)
{
	ERRSTRUCT zErr;
	PERFRULE  zPerfRule;

	lpprInitializeErrStruct(&zErr);

	lpprInitPerfruleTable(pzRuleTable);

  while (!zErr.iSqlError)
  {
    lpprInitPerfrule(&zPerfRule);
    lpprSelectAllPerfrule(&zPerfRule, iID, lDate, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
	    break;
		}
    else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
    else
		{
			zErr = lpfnAddPerfruleToTable(pzRuleTable, zPerfRule, zTHdrDetTable);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
		}
	}  /* While no error */

	return zErr;
} // GetAllPerfrulesForAPortfolio

// function to clean up (release memory) of all portfolio specific dynamic tables
void CleanUpPortfolioTables(HOLDINGSASSETSTABLE *pzHATable, SEGMAINTABLE	*pzSTable,
														PERFRULETABLE *pzPTable, ASSETTABLE2 *pzATable)
{
	InitializeHoldingsAssetsTable(pzHATable);
	InitializeSegmainTable(pzSTable);
	lpprInitializeAssetTable(pzATable);
	lpprInitPerfruleTable(pzPTable);
} //CleanUpPortfolioTables

// function to clean up (release memory) of all global dynamic tables
void CleanUpSystemTables()
{
	InitializeSegmentsTable(&zSegmentsTable);
	InitializeSegmapTable(&zSegmapTable);
	FreeLibrary(PerfCalcDll);
	FreeLibrary(OledbIODll);
	FreeLibrary(TransEngineDll); 
} //CleanUpSystemTables

/**
** Function which initializes this Dll. It loads other dlls required by this 
** Dll and loads all the functions/procedures used from those dlls.
**/
DLLAPI ERRSTRUCT STDCALL WINAPI InitHoldtot(long lAsofDate, char *sDBAlias, char *sErrFile)
{
	long lTradeDate, lPricingDate;
	static long lLastAsofDate = -999;

	static char sLastAlias[80];
	ERRSTRUCT zErr;
	int	iError;
	char sHoldingsName[HOLDMAPNAMESIZE], sHoldcashName[HOLDMAPNAMESIZE], sPortmainName[HOLDMAPNAMESIZE], sPortbalName[HOLDMAPNAMESIZE];
	char sPayrecName[HOLDMAPNAMESIZE], sHXrefName[HOLDMAPNAMESIZE], sHoldtotName[HOLDMAPNAMESIZE];

	if ((strcmp(sDBAlias, "") != 0)&&(_stricmp(sLastAlias, sDBAlias)!=0))
	{
		bInit = FALSE;
		CleanUpSystemTables();
	}
	
	if (!bInit)
	{
		PerfCalcDll = LoadLibrarySafe("Performance.dll");
		if (PerfCalcDll == NULL)
		{
			zErr.iBusinessError = GetLastError();
			return zErr;
		}

		OledbIODll = LoadLibrarySafe("oledbio.dll");
		if (OledbIODll == NULL)
		{
			zErr.iBusinessError = GetLastError();
			return zErr;
		}

		TransEngineDll = LoadLibrarySafe("TransEngine.dll");
		if (TransEngineDll == NULL)
		{
			zErr.iBusinessError = GetLastError();
			return zErr;
		}

		// Load functions from Transengine Dll
		lpprInitializeErrStruct =	(LPPRERRSTRUCT)GetProcAddress(TransEngineDll, "InitializeErrStruct");
		if (!lpprInitializeErrStruct)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Initialize Error Structure function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT1", FALSE));
		}
		lpprInitializeErrStruct(&zErr);

		lpfnPrintError = (LPFNPRINTERROR)GetProcAddress(TransEngineDll, "PrintError");
		if (!lpfnPrintError)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load PrintError function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT2", FALSE));
		}

		// Load functions from OledbIO
		lpfnStartTransaction = (LPFNVOID)GetProcAddress(OledbIODll, "StartDBTransaction");
		if (!lpfnStartTransaction )
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load StartDBTransaction function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT2a", FALSE));
		}

		lpfnCommitTransaction = (LPFNVOID)GetProcAddress(OledbIODll, "CommitDBTransaction");
		if (!lpfnCommitTransaction )
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load CommitDBTransaction function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT2b", FALSE));
		}

		lpfnRollbackTransaction = (LPFNVOID)GetProcAddress(OledbIODll, "RollbackDBTransaction");
		if (!lpfnRollbackTransaction )
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load RollbackDBTransaction function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT2c", FALSE));
		}

		lpfnAbortTransaction = (LPFN1BOOL)GetProcAddress(OledbIODll, "AbortDBTransaction");
		if (!lpfnAbortTransaction )
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AbortDBTransaction function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT2d", FALSE));
		}

		lpfnGetTransCount = (LPFNVOID)GetProcAddress(OledbIODll, "GetTransCount");
		if (!lpfnGetTransCount )
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load GetTransCount function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT2e", FALSE));
		}

		lpprDeleteHoldtotForAnAccount = (LPPR1LONG)GetProcAddress(OledbIODll, "DeleteHoldtotForAnAccount");
		if (!lpprDeleteHoldtotForAnAccount )
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load DeleteHoldtotForAnAccount function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT3", FALSE));
		}

		lpprSelectHoldmap = (LPPR1LONG7PCHAR)GetProcAddress(OledbIODll,  "SelectHoldmap");
		if (!lpprSelectHoldmap)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectHoldmap function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT4", FALSE));
		}

		lpprInsertHoldtot = (LPPRHOLDTOT)GetProcAddress(OledbIODll, "InsertHoldtot");
		if (!lpprInsertHoldtot)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InsertHoldtot function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT5", FALSE));
		}

		lpprSelectAllForHoldTot = (LPPRSELECTALLFORHOLDTOT)GetProcAddress(OledbIODll,  "SelectAllForHoldTot");
		if (!lpprSelectAllForHoldTot)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectAllForHoldTot function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT6", FALSE));
		}

		lpprSelectAllPerfrule = (LPPRSELECTALLPERFRULE)GetProcAddress(OledbIODll, "SelectAllPerfrule");
		if (!lpprSelectAllPerfrule)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectAllPerfrule function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT7", FALSE));
		}

		lpprSelectOnePartPortmain = (LPPRSELECTONEPARTPORTMAIN)GetProcAddress(OledbIODll, "SelectOnePartPortmain");
		if (!lpprSelectOnePartPortmain)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectOnePartPortmain function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT8", FALSE));
		}
												
		lpprSelectAllRatings = (LPPRSELECTALLRATING)GetProcAddress(OledbIODll,  "SelectAllRatings");
		if (!lpprSelectAllRatings)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectAllRatings function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT9", FALSE));
		}

		lpprSelectAllScriptHeaderAndDetails = (LPPRSELECTALLSCRIPTHEADERANDDETAILS)GetProcAddress(OledbIODll, "SelectAllScriptHeaderAndDetails");
		if (!lpprSelectAllScriptHeaderAndDetails)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectAllScriptHeaderAndDetails function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT10", FALSE));
		}

		lpprSelectSegmainForPortfolio =(LPPRSELECTSEGMAINFORPORTFOLIO)GetProcAddress(OledbIODll,"SelectSegmainForPortfolio");
		if (!lpprSelectSegmainForPortfolio)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectSegmainForPortfolio Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT11", FALSE));
		}

		lpprSelectStarsDate = (LPPR2PLONG)GetProcAddress(OledbIODll,  "SelectStarsDate");
		if (!lpprSelectStarsDate)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectStarsDate function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT12", FALSE));
		}

		lpprSelectUnsupervised = (LPPRSELECTUNSUPERVISED)GetProcAddress(OledbIODll,"SelectUnsupervised");
		if (!lpprSelectUnsupervised)
		{
			iError = GetLastError();
			return (lpfnPrintError("Unable to load SelectUnSupervised function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT13",FALSE));
		}
   
		lpprRegTransCount = (LPPR1LONG1PLONG) GetProcAddress(OledbIODll, "CheckIfTransExists");
		if(!lpprRegTransCount)
		{
			iError = GetLastError();
			return(lpfnPrintError("Error Loading CheckIfTransExists function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT14", FALSE));
		}

		lpprSelectAllSegments = (LPPRSELECTALLSEGMENTS)GetProcAddress(OledbIODll,"SelectAllSegments");
		if(!lpprSelectAllSegments)
		{
			iError = GetLastError();
			return (lpfnPrintError("Unable to load SelectAllSegments function",0,0,"",iError,0,0, "CREATEHOLDTOT INIT15",FALSE));
		}

		lpprSelectAllSegmap = (LPPRSELECTALLSEGMAP)GetProcAddress(OledbIODll,"SelectAllSegmap");
		if (!lpprSelectAllSegmap)
		{
			iError = GetLastError();
			return (lpfnPrintError("Unable to load SelectAllSegmap function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT16",FALSE));
		}

		lpprSelectSysSettings =(LPPRSELECTSYSSETTINGS)GetProcAddress(OledbIODll,"SelectSysSettings");
		if (!lpprSelectSysSettings)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectSysSettings Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT17", FALSE));
		}
	 
		lpprSelectSysvalues =	(LPPRSELECTSYSVALUES)GetProcAddress(OledbIODll, "SelectSysvalues");
		if (!lpprSelectSysvalues)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectSysvalues", 0, 0, "", iError, 0, 0, "CALCPERF INIT18", FALSE));
		}

		lpprSelectAllTemplateHeaderAndDetails = (LPPRSELECTALLTEMPLATEDETAILS)GetProcAddress(OledbIODll, "SelectAllTemplateHeaderAndDetails");
		if (!lpprSelectAllTemplateHeaderAndDetails)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load SelectAllTemplateHeaderAndDetails function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT19", FALSE));
		}

		// Load functions from Performance dll
		lpfnAddAssetToTable =(LPFNADDASSET)GetProcAddress(PerfCalcDll,"AddAssetToTable");
		if (!lpfnAddAssetToTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddAssetToTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT20", FALSE));
		}

		lpfnFindAssetInTable =(LPFNFINDASSET)GetProcAddress(PerfCalcDll,"FindAssetInTable");
		if (!lpfnFindAssetInTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load FindAssetInTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT21", FALSE));
		}

		lpfnCreateNewScript =(LPFN2PSCRHDRDET)GetProcAddress(PerfCalcDll,"CreateNewScript");
		if (!lpfnCreateNewScript)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load CreateNewScript Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT22", FALSE));
		}

		lpprInitPerformance = (LPFN1LONG3PCHAR1BOOL1PCHAR)GetProcAddress(PerfCalcDll, "InitPerformance");
		if (!lpprInitPerformance)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitPerformance function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT23", FALSE));
		}

		lpprInitializeAssetTable = (LPPRASSETTABLE)GetProcAddress(PerfCalcDll,  "InitializeAssetTable"); 
		if (!lpprInitializeAssetTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializeAssetTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT24", FALSE));
		}

		lpprTestAsset = (LPPRTESTASSET)GetProcAddress(PerfCalcDll,  "TestAsset");
		if (!lpprTestAsset)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load TestAsset function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT25", FALSE));
		}

		lpprInitPerfrule = (LPPRPPERFRULE)GetProcAddress(PerfCalcDll, "InitializePerfrule");
		if (!lpprInitPerfrule)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializePerfrule Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT26", FALSE));
		}
                                                               
		lpprInitPerfruleTable = (LPPRPPERFRULETABLE)GetProcAddress(PerfCalcDll, "InitializePerfruleTable");
		if (!lpprInitPerfruleTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializePerfruleTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT27", FALSE));
		}

		lpfnAddPerfruleToTable = (LPFNADDPERFRULE)GetProcAddress(PerfCalcDll, "AddPerfruleToTable");
		if (!lpfnAddPerfruleToTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddPerfruleToTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT28", FALSE));
		}

		lpfnFindCreateScriptHeader =(LPFNFINDCREATESCRIPTHEADER)GetProcAddress(PerfCalcDll,"FindCreateScriptHeader");
		if (!lpfnFindCreateScriptHeader)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load FindCreateScriptHeader Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT29", FALSE));
		}

		lpfnCreateVirtualPerfScript =(LPFNCREATEPERFSCRIPT)GetProcAddress(PerfCalcDll,"CreateVirtualPerfScript");
		if (!lpfnCreateVirtualPerfScript)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load CreateVirtualPerfScript Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT30", FALSE));
		}

		lpfnAddScriptHeaderToTable = (LPFNPSCRHDR)GetProcAddress(PerfCalcDll,"AddScriptHeaderToTable");
		if (!lpfnAddScriptHeaderToTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddScriptHeaderToTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT31", FALSE));
		}

		lpfnAddDetailToScrHdrDet = (LPFNADDSCRDETAIL)GetProcAddress(PerfCalcDll,"AddDetailToScrHdrDet");
		if (!lpfnAddDetailToScrHdrDet)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddDetailToScrHdrDet Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT32", FALSE));
		}

		lpfnInitializePScrHdrDet = (LPPRPSCRHDRDET)GetProcAddress(PerfCalcDll,"InitializePScrHdrDet");
		if (!lpfnInitializePScrHdrDet)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializePScrHdrDet Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT33", FALSE));
		}

		lpfnFillPartSectypeTable =(LPFNPPARTSECTYPETABLE)GetProcAddress(PerfCalcDll,"FillPartSectypeTable");
		if (!lpfnFillPartSectypeTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load FillPartSectypeTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT34", FALSE));
		}

		lpfnAddNewSegment = (LPFN1INT2PCHAR1INT)GetProcAddress(PerfCalcDll,"AddNewSegment");
		if (!lpfnAddNewSegment)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddNewSegment Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT35", FALSE));
		}

		lpprInitializePerfTmpHdr =(LPPRPPTMPHDR)GetProcAddress(PerfCalcDll,"InitializePerfTmpHdr");
		if (!lpprInitializePerfTmpHdr)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializePerfTmpHdr Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT36", FALSE));
		}

		lpprInitializePerfTmpDet =(LPPRPPTMPDET)GetProcAddress(PerfCalcDll,"InitializePerfTmpDet");
		if (!lpprInitializePerfTmpDet)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializePerfTmpDet Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT37", FALSE));
		}

		lpfnAddTemplateHeaderToTable =(LPFNADDTEMPLATEHEADER)GetProcAddress(PerfCalcDll,"AddTemplateHeaderToTable");
		if (!lpfnAddTemplateHeaderToTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddTemplateHeaderToTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT38", FALSE));
		}

		lpfnAddTemplateDetailToTable =(LPFNADDTEMPLATEDETAIL)GetProcAddress(PerfCalcDll,"AddTemplateDetailToTable");
		if (!lpfnAddTemplateDetailToTable)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load AddTemplateDetailToTable Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT39", FALSE));
		}

		lpprInitializePartAsset = (LPPRPARTASSET2)GetProcAddress(PerfCalcDll,"InitializePartialAsset");
		if (!lpprInitializePartAsset)
		{
			iError = GetLastError();
			return(lpfnPrintError("Unable To Load InitializePartialAsset Function", 0, 0, "", iError, 0, 0, "CREATEHOLDTOT INIT40", FALSE));
		}

		lpfnAllocateMemoryToDailyAssetInfo = (LPFNPARTASSET2)GetProcAddress(PerfCalcDll, "AllocateMemoryToDailyAssetInfo");
		if (!lpfnAllocateMemoryToDailyAssetInfo)
			return(lpfnPrintError("Unable To Load AllocateMemoryToDailyAssetInfo Function", 0, 0, "", GetLastError(), 0, 0, "CREATEHOLDTOT INIT41", FALSE));
	} // if not initialized before
	
	lpprInitializeErrStruct(&zErr);

	if (lLastAsofDate != lAsofDate)
	{
		lpprSelectStarsDate(&lTradeDate, &lPricingDate, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

	
		lpprSelectHoldmap(lAsofDate, sHoldingsName, sHoldcashName, sPortmainName, sPortbalName,
			                sPayrecName, sHXrefName, sHoldtotName, &zErr);
		if (zErr.iBusinessError != 0)
			return zErr;	
		else if (zErr.iSqlError == SQLNOTFOUND)
		{// if the AsOfDate is Trade date or Pricing Date, do the current holdtot
			if (lAsofDate == lTradeDate || lAsofDate == lPricingDate)
				lAsofDate = -1;
			else//do the Adhoc
				lAsofDate = 0;
		}
		else if (zErr.iSqlError != 0)
			return zErr;
	
		zErr = lpprInitPerformance (lAsofDate, sDBAlias, "", "", FALSE, sErrFile );
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		lLastAsofDate = lAsofDate;
	} // if has not been already initialized for this date

	if(!bInit)
	{
		zErr = GetSystemTables();
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
	}

	bInit = TRUE;
	if (strcmp(sDBAlias, "") != 0)
	  strcpy_s(sLastAlias, sDBAlias);

	return zErr;
} // InitHoldtot