/**
		Main Source File
  2018-10-01 VI#61958 Update to handle merge specific level - yb.
  2015-04-02 VI#57005 Fix for when qtrly composite's members data ends before qtr end - yb.
  2014-07-01 VI#54971 More updates for handling IPVs and daily - yb.
  2014-06-23 VI#54971 Updates for handling IPVs and daily - yb.
  2013-06-11 VI#52878 sum notional flows - yb.
  2013-04-16 VI#52266 inserts can be for non-period end date. delete all dsumdata for period - yb.
  03/10/2011 VI#45707 Prevented negative UVs from making it into unitvalue -mk 	
  03-27/2006 only update MAPCOmpMemTransEx for partnership composites, portfolio type = 7 - ssi 	
  10-26-2005 Added code to write to MapCompMemTransEx, and to merge a composite partnership account - ssi
**/
#include "CompositeCreate.h" 
#include "perfdll.h" 
#include <ctype.h>

/*Global Vars*/
char			sInitDBAlias[80], sInitMode[5], sInitType[5], sInitErrFile[40];
BOOL			bGracePeriod = FALSE;
HINSTANCE	hStarsIODll,hStarsUtilsDll,hTransEngineDll,hPerformanceDll;
BOOL			bCompCreateInit = FALSE;


/*F*
** Main function for the dll
*F*/
BOOL APIENTRY DllMain (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
							break;

		case DLL_PROCESS_DETACH:
							FreeCompositeCreateLibrary();
							break;

		default : break;
	}

  return TRUE;
} //DllMain


/**
**This routine creates compsite for holdings and hold cash,given the Composite ID 
**/
/**  int iPortfolioType, long iHoldingsFlag, **add as parameters** */
DLLAPI void STDCALL WINAPI CreateComposite(int iOwnerID,long  lDate,int iPortfolioType, long lHoldingsFlag, ERRSTRUCT *zErr)
{
	HOLDTABLE			zHoldingsTable;
	HOLDCASHTABLE zHoldcashTable;
  int						iPortfolioID;
  int						iNumComposite = 0;
	PORTMAIN			zPortmainTable;
	char					sHoldingsName[18+NT], sHoldcashName[18+NT], sPortmainName[18+NT], sPortbalName[18+NT],
                sPayrecName[18+NT], sHXrefName[18+NT], sHoldtotName[18+NT];
	BOOL					bUseTrans;
	long					lInitDate;
	

	/*Initilization*/
	zHoldingsTable.iNumHoldCreated = 0;
	zHoldcashTable.iNumHoldCashCreated = 0;
	InitializeHoldingsTable(&zHoldingsTable);
	InitializeHoldcashTable(&zHoldcashTable);	
  lpprInitializeErrStruct(zErr);

	memset(&zPortmainTable,0,sizeof(PORTMAIN));
	lpprSelectPortmain(&zPortmainTable,iOwnerID,zErr);
	if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
		return; 

	lInitDate = lDate;
	lpprSelectHoldmap(lInitDate,sHoldingsName, sHoldcashName, sPortmainName, sPortbalName,
                    sPayrecName, sHXrefName, sHoldtotName,zErr);
	
	/*If the date does not exist in holdmap then its adhoc date*/
	if (zErr->iSqlError == SQLNOTFOUND)
	{
		lInitDate = ADHOC_DATE;
		lpprSelectHoldmap(lInitDate,sHoldingsName, sHoldcashName, sPortmainName, sPortbalName,
                      sPayrecName, sHXrefName, sHoldtotName,zErr);
	};

	if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
		return;

	/*If iPortfolioType	= 7 = Partnership Composite, use tables indicated in lHoldingsFlag*/
	if (iPortfolioType == c_Composite_Partnership )
	{
    	switch (lHoldingsFlag)
			{
				case DEFAULT_DATE:  //use holdings tables from HoldMap
								lInitDate = lDate;   //DEFAULT_DATE;
								break;

				case ADHOC_DATE:  //use adhoc tables
								strcpy_s (sHoldingsName,"HDADHOC");
								strcpy_s (sHoldcashName,"HCADHOC");
								strcpy_s (sPortmainName,"PMADHOC");
								strcpy_s (sPortbalName,"PBADHOC");
								strcpy_s (sPayrecName,"PRADHOC");
								strcpy_s (sHXrefName,"HXADHOC");
								strcpy_s (sHoldtotName,"HTADHOC");
								lInitDate = ADHOC_DATE;
								break;

				case SETTLEMENT_DATE:  //use SETTLEMENT tables
								strcpy_s (sHoldingsName,"HDSTLMNT");
								strcpy_s (sHoldcashName,"HCSTLMNT");
								strcpy_s (sPortmainName,"PMSTLMNT");
								strcpy_s (sPortbalName,"PBSTLMNT");
								strcpy_s (sPayrecName,"PRSTLMNT");
								strcpy_s (sHXrefName,"HXSTLMNT");
								strcpy_s (sHoldtotName,"HTSTLMNT");
								lInitDate = SETTLEMENT_DATE;
								break;
			}
	}
		
	lpprOLEDBIOInit(sInitDBAlias, sInitMode, sInitType, lInitDate, 0, zErr);
	if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
		return;
  
 	lpprInitializeErrStruct(zErr);
  
	memset ( &zHoldcashTable, 0, sizeof ( HOLDCASHTABLE));
  memset ( &zHoldingsTable, 0, sizeof ( HOLDTABLE));

  while ( TRUE )
	{ 
    lpprInitializeErrStruct(zErr);

		/*Get member of given composite*/
    lpprSelectAllMembersOfAComposite (iOwnerID, lDate, &iPortfolioID, zErr );
    if (zErr->iSqlError == SQLNOTFOUND)
    { 
			if (iNumComposite != 0)
 				lpprInitializeErrStruct(zErr);
			
			break;
    }
	  else if (zErr->iSqlError)
		{
			lpfnPrintError("Error Reading COMPPORT Table cursor", iPortfolioID, 0, "", 0, zErr->iBusinessError, 
											zErr->iIsamCode, "COMPCREATE SELECTALLMEMBERSOFACOMPOSITE", FALSE);
      return;
		}

		memset(&zPortmainTable,0,sizeof(PORTMAIN));
		lpprSelectAdhocPortmain(&zPortmainTable,iPortfolioID,zErr);
    // if any db error - ignore and continue
		if (zErr->iSqlError != 0 ||  zErr->iBusinessError != 0)
			continue;	

		// skip deleted members
		if (zPortmainTable.lDeleteDate > 0) 
			continue;

		// skip members which are not valued for the requested date
		if (zPortmainTable.lValDate != lDate) 
			continue;

		iNumComposite++;

		/*Get holdings and holdcash for the input portfolio*/
		GetHoldingsHoldcashInTables (iPortfolioID, &zHoldingsTable, &zHoldcashTable, zErr);
  }/*while*/

 	if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
		return;
  
	/*Creates the composite for all the holdings and hold cash for members of composite*/
	bUseTrans = (lpfnGetTransCount()==0);
	if (bUseTrans)
		lpfnStartTransaction();

	__try
	{
		CreateCompositeRecord_Internal(iOwnerID, &zHoldingsTable, &zHoldcashTable,
																	 sHoldingsName, sHoldcashName, sPortmainName, lDate,
																	 bUseTrans, iPortfolioType, zErr);
	
	}//try
	__except(lpfnAbortTransaction(bUseTrans)){}

	if (bUseTrans)
	{
		if (zErr->iBusinessError != 0 || zErr->iSqlError != 0)
			lpfnRollbackTransaction();
		else
			lpfnCommitTransaction();
	}

	/*Free up the allocated memory*/
	 InitializeHoldingsTable(&zHoldingsTable);		
	 InitializeHoldcashTable(&zHoldcashTable);
	
}/*CreateComposite*/

/**
	This routine fills the holdcash,holdings table in the memory for the given portfolio.
**/

void GetHoldingsHoldcashInTables(int iPortfolioID, HOLDTABLE *pHoldingsTable, 
																 HOLDCASHTABLE *pHoldcashTable,ERRSTRUCT *zErr)
{
  HOLDINGS		zHoldings;
  HOLDCASH		zHoldcash;
	HOLDINGS		zHoldingsTemp;
	HOLDCASHKEY zHoldCashKey;
	int					iNumHoldings,iNumHoldcash,HoldCashKeyIndex = -1;
  
  lpprInitializeErrStruct(zErr);
	iNumHoldings = 0;

	while (!zErr->iSqlError)
  {
		lpprInitializeErrStruct(zErr);
    
		/*Initialization*/
		memset (&zHoldings, 0, sizeof (HOLDINGS));
		memset(&zHoldcash,0,sizeof(HOLDCASH));
		memset(&zHoldCashKey,0,sizeof(HOLDCASHKEY));

		/*Get holdings for the input member of composite */
		lpprSelectAllHoldingsForAnAccount (iPortfolioID, &zHoldings, zErr);
    if (zErr->iSqlError == SQLNOTFOUND)
			break;
		else if (zErr->iSqlError)
		{
			lpfnPrintError("Error Reading Holdings Table cursor", iPortfolioID, 0, "", 0, zErr->iBusinessError, 
											zErr->iIsamCode, "COMPCREATE SELECTALLHOLDINGSFORANACCOUNT", FALSE);
			return;
		}
    
		++iNumHoldings;

		zHoldingsTemp= zHoldings;
		/*Add holdings to the table in the memory*/
    AddTempHoldingToTable(pHoldingsTable, zHoldingsTemp,zErr);
    if (zErr->iBusinessError != 0)
      return;
  }/* While No Error */

	lpprInitializeErrStruct(zErr);
	iNumHoldcash = 0;

  while (!zErr->iSqlError)
  {
	  lpprInitializeErrStruct(zErr);
		memset (&zHoldcash, 0, sizeof (HOLDCASH));

		lpprSelectAllHoldcashForAnAccount ( iPortfolioID, &zHoldcash, zErr );
    if (zErr->iSqlError == SQLNOTFOUND)
		{
      zErr->iSqlError = 0;
			break;
		}
		else if (zErr->iSqlError)
		{
			lpfnPrintError("Error Reading HOLDCASHTABLE Table cursor", iPortfolioID, 0, "", 0,zErr->iBusinessError,
											zErr->iIsamCode, "COMPCREATE SELECTALLHOLDCASHFORANACCOUNT", FALSE);
			return;
		}
		
		++iNumHoldcash;

    /*if its not first hold cash then check if the key(sec_no,wi,secXtend,acct_type) already exist
			in the table of collected hold cash in the memory.if the key does not exist add the selected hold cash to 
			the table(memory).If the key exist then add the units,tot_cost,mkt_value to the existing holdcash*/
//		if (iNumHoldcash > 1)
//		{
		strcpy_s(zHoldCashKey.sSecNo, zHoldcash.sSecNo);
		strcpy_s(zHoldCashKey.sWi, zHoldcash.sWi);
		strcpy_s(zHoldCashKey.sSecXtend,zHoldcash.sSecXtend);
		strcpy_s(zHoldCashKey.sAcctType,zHoldcash.sAcctType);
		HoldCashKeyIndex = CASHHOLD_KEY_NOT_FOUND;
		HoldCashKeyIndex = FindHoldCashKey(pHoldcashTable,zHoldCashKey);
		memset(&zHoldCashKey,0,sizeof(HOLDCASHKEY));
//		}
		
		/*If the key is not found then add the cash hold to the cash hold table(memory)*/
		if (HoldCashKeyIndex == CASHHOLD_KEY_NOT_FOUND)
		{
			AddTempHoldcashToTable(pHoldcashTable, zHoldcash,zErr);
			if (zErr->iBusinessError != 0)
				return;
		}
		/* Key is found add the units,tot_cost,mkt_value to matched cash hold*/
		else
		{
			pHoldcashTable->pzHoldCash[HoldCashKeyIndex].fUnits += zHoldcash.fUnits;
			pHoldcashTable->pzHoldCash[HoldCashKeyIndex].fTotCost += zHoldcash.fTotCost;
			pHoldcashTable->pzHoldCash[HoldCashKeyIndex].fMktVal += zHoldcash.fMktVal;
		}/*if else*/
	}/* While No Error */
  

  if ((iNumHoldcash != 0)  || (iNumHoldings != 0))
		zErr->iSqlError = 0;
} /* GetHoldingsHoldcashInTables */

/**
** This function adds a holding/HOLDCASHTABLE record in the holding table. This
** function does not check whether the passed holding record already exist in
** the table or not.
**/

void AddTempHoldingToTable(HOLDTABLE *pHoldingsTable,HOLDINGS zHoldingsTemp,ERRSTRUCT *zErr)
{
 
  lpprInitializeErrStruct(zErr);
 
  /* If table is full to its limit, allocate more space */
  if (pHoldingsTable->iNumHoldCreated == pHoldingsTable->iNumHoldings)
  {
    pHoldingsTable->iNumHoldCreated += EXTRAHOLD;
		pHoldingsTable->pzHoldings = (HOLDINGS*)realloc(pHoldingsTable->pzHoldings,pHoldingsTable->iNumHoldCreated * sizeof(HOLDINGS));
    if (pHoldingsTable->pzHoldings == NULL)
		{
			lpfnPrintError("Insufficient Memory For HoldTable", 0, 0, "", 997, 0, 0, "COMPCREATE ADDTEMPHOLD", FALSE);
			return;
		}
  }
	pHoldingsTable->pzHoldings[pHoldingsTable->iNumHoldings++]  = zHoldingsTemp;

}/* AddTempHoldingToTable */

/**
** This function adds a holdcash record in the holding table. This
** function does not check whether the passed holding record already exist in
** the table or not.
**/
void AddTempHoldcashToTable(HOLDCASHTABLE *pHoldcashTable, HOLDCASH zHoldcashTemp,ERRSTRUCT * zErr)
{
  lpprInitializeErrStruct(zErr);
 
  /* If table is full to its limit, allocate more space */
  if (pHoldcashTable->iNumHoldCashCreated == pHoldcashTable->iNumHoldCash)
  {
    pHoldcashTable->iNumHoldCashCreated += EXTRAHOLD;
    pHoldcashTable->pzHoldCash = (HOLDCASH *)realloc(pHoldcashTable->pzHoldCash,
    pHoldcashTable->iNumHoldCashCreated * sizeof(HOLDCASH));
    if (pHoldcashTable->pzHoldCash == NULL)
		{
			lpfnPrintError("Insufficient Memory For HoldcashTable", 0, 0, "", 997, 0, 0, "COMPCREATE ADDTEMPHOLD", FALSE);
      return;
		}
  }
 
  pHoldcashTable->pzHoldCash[pHoldcashTable->iNumHoldCash]=zHoldcashTemp;
  pHoldcashTable->iNumHoldCash++;
 
}/* AddTempHoldcashToTable */

void CreateCompositeRecord_Internal(int iCompositeID, HOLDTABLE *pHoldingsTable, HOLDCASHTABLE *pHoldcashTable,
																		char *sHoldingsTable,char *sHoldCashTable,char *sTargetPortmain, long lValDate, 
																		BOOL bInTrans, int iPortType, ERRSTRUCT* zErr)
{
	int								iNumHoldings,iNumHoldcash;
  char							sSecNo[12+NT],sWi[1+NT];
  HOLDINGS					zHoldingsRec,zHoldingsTemp;
  HOLDCASH					zHoldcashTemp;
  HOLDCASH					zHoldCashRec; 
  MAPCOMPMEMTRANSEX zMapCompMemTransEx;
  long							iInsertCount;
  
	lpprInitializeErrStruct(zErr);

	/*Delete all holdings for the input composite*/
	lpprAccountDeleteHoldings( iCompositeID, sSecNo, sWi, sHoldingsTable, FALSE,  zErr );

	/*Delete all MAPCOMPMEMTRANSEX record for the input composite and date*/
	/*Only delete MapCompMemTransEx for partnership composites*/
	if (iPortType == c_Composite_Partnership)
	{
		lpprDeleteMapCompMemTransEx( lValDate, iCompositeID, zErr );
	}

		
	/*Go through the collected holdings and insert them into holdings with new id(composite id)*/
	iInsertCount = 0;
	for (iNumHoldings = 0; iNumHoldings < pHoldingsTable->iNumHoldings;++iNumHoldings )  
	{ 
		lpprInitializeErrStruct(zErr);
		memcpy (&zHoldingsTemp,&pHoldingsTable->pzHoldings[iNumHoldings],sizeof(HOLDINGS));

		zHoldingsRec = zHoldingsTemp;   
		zHoldingsRec.iID = iCompositeID;
		zHoldingsRec.lTransNo = iNumHoldings+1;

		lpprInsertHoldings ( zHoldingsRec, zErr );
		//lpprInsertHoldingsBatch (&zHoldingsRec, INSERT_BATCH_SIZE, zErr);
		if ( (zErr->iSqlError != 0) || ( zErr->iBusinessError != 0))
			return;
		
		/*Only update MapCompMemTransEx for partnership composites*/
		if (iPortType == c_Composite_Partnership)
		{
			//build MapCompMemTransEx
			zMapCompMemTransEx.lCompDate = lValDate;
			zMapCompMemTransEx.iCompID = iCompositeID;
			zMapCompMemTransEx.lCompTrans = zHoldingsRec.lTransNo;
			zMapCompMemTransEx.iCompMem = zHoldingsTemp.iID;
			zMapCompMemTransEx.lCompMemTrans = zHoldingsTemp.lTransNo;
			
			//insert records into MapCompMemTransEx
			lpprInsertMapCompMemTransEx (zMapCompMemTransEx, zErr);
			if ( (zErr->iSqlError != 0) || ( zErr->iBusinessError != 0))
				return;
		}//	if (iPortType == c_Composite_Partnership)

		// if using DBTrans and reached the limit - commit and start new trans
		if (bInTrans)
		{	
			iInsertCount++;
			if (iInsertCount >= INSERT_BATCH_SIZE)
			{
				lpfnCommitTransaction();
				lpfnStartTransaction();
				iInsertCount = 0;
			}
		}
	}/* End Of For NumHoldings loop   */

	// if any pending changes - apply them now	
	/* 
	lpprInsertHoldingsBatch (NULL, 0, zErr);
	if ( (zErr->iSqlError != 0) || ( zErr->iBusinessError != 0))
		return;
	*/
	lpprInitializeErrStruct(zErr);
	
	/* Delete hold cash for the input composite from holdcash table*/
	lpprAccountDeleteHoldcash ( iCompositeID, sSecNo, sWi, sHoldCashTable, FALSE, zErr );

	/*Insert all the hold cash for the compositte*/
	for (iNumHoldcash = 0; iNumHoldcash < pHoldcashTable->iNumHoldCash;++iNumHoldcash )  
	{ 
		lpprInitializeErrStruct(zErr);
		memcpy ( &zHoldcashTemp,  &pHoldcashTable->pzHoldCash[iNumHoldcash], sizeof (HOLDCASH));
		zHoldCashRec = zHoldcashTemp;   
		zHoldCashRec.iID = iCompositeID;
		lpprInsertHoldcash ( zHoldCashRec, zErr );
		if ((zErr->iSqlError != 0) ||  (zErr->iBusinessError != 0))
			return;

		if (bInTrans)
		{	
			iInsertCount++;
			if (iInsertCount >= INSERT_BATCH_SIZE)
			{
				lpfnCommitTransaction();
				lpfnStartTransaction();
				iInsertCount = 0;
			}
		}
	}
	
	/*Get the val date from portmain */ 
	if (lValDate > 0)
	{
		/*Update the valdate*/
		lpprUpdatePortmainValDate(iCompositeID,lValDate,zErr);

		/*Check if failed to update the valdate*/
		if (zErr->iSqlError !=0 || zErr->iBusinessError !=0)
		{
			/*if the composite portfolio does not exist in portmain_yyyymmdd or adhoc*/
			if (zErr->iSqlError == SQLNOTFOUND)
			{
				lpprAccountInsertPortmain(iCompositeID,sTargetPortmain, "PORTMAIN",zErr);
				/*if it fails to insert the porfolio into portmain_yyyymmdd or adhoc*/
				if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
				{
					lpfnPrintError("Error Inserting into Portmain Table ", iCompositeID, 0, "", 0, zErr->iBusinessError, 
													zErr->iIsamCode, "COMPCREATE AccountInsertPortmain", FALSE);
					return;
				}
				/*Successfully Inserted the portfolio into portmain;Now Update the val date*/
				else
				{
					lpprUpdatePortmainValDate(iCompositeID,lValDate,zErr);
					if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
						return;
				}
			}
			/*Some Other Error occured*/
			else
			{
				lpfnPrintError("Error Updating Portmain Table cursor", iCompositeID, 0, "", 0, zErr->iBusinessError, 
												zErr->iIsamCode, "COMPCREATE UpdatePortmainValDate", FALSE);
				return;
			}	
		}
	}
		
	if (zErr->iSqlError == SQLNOTFOUND)
		zErr->iSqlError = 0;
}/*CreateCompositeRecord_Internal*/

/**
	Load dlls and functions 
**/
DLLAPI void STDCALL InitializeDllRoutines(long lAsofDate, char *sDBAlias, char *sMode, char *sType, 
																		 BOOL bPrepareQueries, char *sErrFile,ERRSTRUCT *zErr)
{
/*	static long lLastDate = -100;
	static char sLastAlias[80] = '\0';
	static char sLastMode[5] = '\0';
	statis char sLastType[5] = '\0';*/
	SYSVALUES		zSysvalues;

  zErr->iBusinessError = 0;
  zErr->iID = 0;
  zErr->iIsamCode = 0;
  zErr->iSqlError = 0;
  zErr->lRecNo = 0;
  strcpy_s(zErr->sRecType,"");

	/*Load DLLs and functions the first time only*/
	if (!bCompCreateInit)
	{
		/*Load StarsIO.Dll */
		hStarsIODll = LoadLibrarySafe("oledbio.dll");
		if (hStarsIODll == NULL)
		{
			zErr->iBusinessError = GetLastError();
			return ;
		}
		/*Load TransEngine.Dll */
		hTransEngineDll = LoadLibrarySafe("TransEngine.dll");
		if (hTransEngineDll == NULL)
		{
			zErr->iBusinessError = GetLastError();
			return ;
		}
		/*Load StarsUtils.*/
		hStarsUtilsDll = LoadLibrarySafe("StarsUtils.dll");
		if (hStarsUtilsDll == NULL)
		{
			zErr->iBusinessError = GetLastError();
			return ;
		}
		/*Load Performance.dll*/
		hPerformanceDll = LoadLibrarySafe("Performance.dll");
		if (hPerformanceDll == NULL)
		{
			zErr->iBusinessError = GetLastError();
			return ;
		}

		lpfnPrintError = (LPFNPRINTERROR)GetProcAddress(hTransEngineDll, "PrintError");
		if (!lpfnPrintError)
		{
			zErr->iBusinessError = GetLastError();
			return ;
		}

		lpprOLEDBIOInit =	(LPPR3PCHAR1LONG1BOOL)GetProcAddress(hStarsIODll, "InitializeOLEDBIO");
		if (!lpprOLEDBIOInit)
		{
			lpfnPrintError("Unable To Load InitializeOLEDBIO function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT1", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpprInitializeErrStruct =	(LPPRERRSTRUCT)GetProcAddress(hTransEngineDll, "InitializeErrStruct");
		if (!lpprInitializeErrStruct)
		{
			lpfnPrintError("Unable To Initialize Error Structure function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT2", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpfnStartTransaction = (LPFNVOID)GetProcAddress(hStarsIODll, "StartDBTransaction");
		if (!lpfnStartTransaction )
		{
			lpfnPrintError("Unable To Load StartDBTransaction function", 0, 0, "", GetLastError(), 0, 0, "CompCreate INIT2a", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpfnCommitTransaction = (LPFNVOID)GetProcAddress(hStarsIODll, "CommitDBTransaction");
		if (!lpfnCommitTransaction )
		{
			lpfnPrintError("Unable To Load CommitDBTransaction function", 0, 0, "", GetLastError(), 0, 0, "CompCreate INIT2b", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpfnRollbackTransaction = (LPFNVOID)GetProcAddress(hStarsIODll, "RollbackDBTransaction");
		if (!lpfnRollbackTransaction )
		{
			lpfnPrintError("Unable To Load RollbackDBTransaction function", 0, 0, "", GetLastError(), 0, 0, "CompCreate INIT2c", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpfnAbortTransaction = (LPFN1BOOL)GetProcAddress(hStarsIODll, "AbortDBTransaction");
		if (!lpfnAbortTransaction )
		{
			lpfnPrintError("Unable To Load AbortDBTransaction function", 0, 0, "", GetLastError(), 0, 0, "CompCreate INIT2d", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpfnGetTransCount = (LPFNVOID)GetProcAddress(hStarsIODll, "GetTransCount");
		if (!lpfnGetTransCount )
		{
			lpfnPrintError("Unable To Load GetTransCount function", 0, 0, "", GetLastError(), 0, 0, "CompCreate INIT2e", FALSE);
			zErr->iBusinessError = -1;
			return ;
		}

		lpprSelectAllMembersOfAComposite = (LPPRSELECTALLMEMBERSOFACOMPOSITE)GetProcAddress(hStarsIODll,"SelectAllMembersOfAComposite");
		if (!lpprSelectAllMembersOfAComposite)
		{
			
			lpfnPrintError("Unable To Load SelectAllMembersOfAComposite function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT5", FALSE);
			zErr->iBusinessError = -1;
			return;
		
		}
		
		//load insert for mapcompmemtransex
		lpprInsertMapCompMemTransEx = (LPPRINSERTMAPCOMPMEMTRANSEX)GetProcAddress(hStarsIODll,"InsertMapCompMemTransEx"); 
		if (!lpprInsertMapCompMemTransEx)
		{
			lpfnPrintError("Unable To Load InsertMapCompMemTransEx function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT-new1", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		
		//load Delete for mapcompmemtransex
		lpprDeleteMapCompMemTransEx = (LPPRDELETEMAPCOMPMEMTRANSEX)GetProcAddress(hStarsIODll,"DeleteMapCompMemTransEx"); 
		if (!lpprDeleteMapCompMemTransEx)
		{
			lpfnPrintError("Unable To Load DeleteMapCompMemTransEx function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT-new2", FALSE);
			zErr->iBusinessError = -1;
			return;
		}


		lpprSelectAllHoldingsForAnAccount = (LPPRALLHOLDINGSFORANACCOUNT)GetProcAddress(hStarsIODll,"SelectAllHoldingsForAnAccount"); 
		if (!lpprSelectAllHoldingsForAnAccount)
		{
			lpfnPrintError("Unable To Load SelectAllHoldingsForAnAccount function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT6", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		
		lpprSelectAllHoldcashForAnAccount = (LPPRALLHOLDCASHFORANACCOUNT)GetProcAddress(hStarsIODll,"SelectAllHoldcashForAnAccount");
		if (!lpprSelectAllHoldcashForAnAccount)
		{
			lpfnPrintError("Unable To Load SelectAllHoldcashForAnAccount function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT7", FALSE);
		  zErr->iBusinessError = -1;
			return;
		}
		
		lpprInsertHoldings = (LPPRHOLDINGS)GetProcAddress(hStarsIODll,"InsertHoldings");					
		if (!lpprInsertHoldings)
		{
			lpfnPrintError("Unable To Load InsertHoldings function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT8", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		/*
		lpprInsertHoldingsBatch = (LPPRHOLDINGSBATCH)GetProcAddress(hStarsIODll,"InsertHoldingsBatch");					
		if (!lpprInsertHoldingsBatch)
		{
			lpfnPrintError("Unable To Load InsertHoldingsBatch function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT8a", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		*/
		lpprAccountDeleteHoldings = (LPPR1LONG3PCHAR1BOOL)GetProcAddress(hStarsIODll,"AccountDeleteHoldings");
		if (!lpprAccountDeleteHoldings) 
		{
			lpfnPrintError("Unable To Load lpprAccountDeleteHoldings function", 0, 0, "", GetLastError(),
											0, 0, "CompCreate INIT9", FALSE);
			zErr->iBusinessError = -1;
			return;

		}
		
		lpprInsertHoldcash = (LPPRHOLDCASH)GetProcAddress(hStarsIODll, "InsertHoldcash");
		if (!lpprInsertHoldcash)
		{
			lpfnPrintError("Unable To Load InsertHoldcash function", 0, 0, "", GetLastError(),
												0, 0, "CompCreate INIT10", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		
		lpprAccountDeleteHoldcash = (LPPR1LONG3PCHAR1BOOL) GetProcAddress(hStarsIODll, "AccountDeleteHoldcash");
		if(!lpprAccountDeleteHoldcash)
		{
			lpfnPrintError("Error Loading AccountDeleteHoldcash Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT11", FALSE);
			zErr->iBusinessError = -1;
			return;
		}

		lpprUpdatePortmainValDate = (LPPR1INT1LONG)GetProcAddress(hStarsIODll,"UpdatePortmainValDate");
		if (!lpprUpdatePortmainValDate){
			lpfnPrintError("Error Loading UpdatePortmainValDate Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT12", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		
		lpprSelectAdhocPortmain = (LPPRSELECTPORTMAINTABLE)GetProcAddress(hStarsIODll,"SelectAdhocPortmain");
		if (!lpprSelectAdhocPortmain){
			lpfnPrintError("Error Loading SelectAdhocPortmain Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT13", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		/*		lpprSelectHoldtotFor = (LPPRSELECTHOLDTOTFOR)GetProcAddress(hStarsIODll,"SelectHoldtotFor");
		if (!lpprSelectHoldtotFor){
			lpfnPrintError("Error Loading SelectHoldtotFor Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT14", FALSE);
			zErr->iBusinessError = -1;
			return;
		}*/

		lpprSelectHoldmap = (LPPR1LONG7PCHAR)GetProcAddress(hStarsIODll,"SelectHoldmap");
		if (!lpprSelectHoldmap)
		{
			lpfnPrintError("Error Loading lpprSelectHoldmap Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT14", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		
		lpprSelectPortmain =(LPPRPORTMAIN)GetProcAddress(hStarsIODll,"SelectPortmain");
		if (!lpprSelectPortmain)
		{
			lpfnPrintError("Error Loading lpprSelectPortmain Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT15", FALSE);
			zErr->iBusinessError = -1;
			return;
		}

		lpprAccountInsertPortmain =(LPPRACCOUNTINSERTPORTMAIN)GetProcAddress(hStarsIODll,"AccountInsertPortmain") ;
		if(!lpprAccountInsertPortmain)
		{
			lpfnPrintError("Error Loading lpprAccountInsertPortmain Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT16", FALSE);
			zErr->iBusinessError = -1;
			return;
		}
		
		lpprGetSummarizedDataForCompositeEx = (LPPRGETSUMMARIZEDDATAFORCOMPOSITE)GetProcAddress(hStarsIODll, "GetSummarizedDataForCompositeEx") ;
		if(!lpprGetSummarizedDataForCompositeEx)
		{
			lpfnPrintError("Error Loading GetSummarizedDataForCompositeEx Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17a", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		
		
		lpprBuildMergeCompSegMap = (LPPRBUILDMERGECOMPSEGMAP)GetProcAddress(hStarsIODll, "BuildMergeCompSegMap") ;
		if(!lpprBuildMergeCompSegMap)
		{
			lpfnPrintError("Error Loading BuildMergeCompSegMap Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17b", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprBuildMergeCompport = (LPPRBUILDMERGECOMPPORT)GetProcAddress(hStarsIODll, "BuildMergeCompport") ;
		if(!lpprBuildMergeCompport)
		{
			lpfnPrintError("Error Loading BuildMergeCompport Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17c", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprBuildMergeSData = (LPPRBUILDMERGESDATA)GetProcAddress(hStarsIODll, "BuildMergeSData") ;
		if(!lpprBuildMergeSData)
		{
			lpfnPrintError("Error Loading BuildMergeSData Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17d", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprBuildMergeUV = (LPPRBUILDMERGESDATA)GetProcAddress(hStarsIODll, "BuildMergeUV") ;
		if(!lpprBuildMergeUV)
		{
			lpfnPrintError("Error Loading BuildMergeUV Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17e", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprDeleteMergeSessionData = (LPPR1CHAR1LONG)GetProcAddress(hStarsIODll, "DeleteMergeSessionData") ;
		if(!lpprDeleteMergeSessionData)
		{
			lpfnPrintError("Error Loading DeleteMergeSessionData Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17f", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprUpdateUnitvalueMonthlyIPV = (LPPRUPDATEUNITVALUEMONTHLYIPV)GetProcAddress(hStarsIODll, "UpdateUnitvalueMonthlyIPV") ;
		if(!lpprUpdateUnitvalueMonthlyIPV)
		{
			lpfnPrintError("Error Loading UpdateUnitvalueMonthlyIPV Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17g", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprDeleteMergeUVGracePeriod = (LPPR1CHAR1LONG)GetProcAddress(hStarsIODll, "DeleteMergeUVGracePeriod") ;
		if(!lpprDeleteMergeUVGracePeriod)
		{
			lpfnPrintError("Error Loading DeleteMergeUVGracePeriod Function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT17h", FALSE);
			zErr->iBusinessError = -1;
			return;
		}		

		lpprDeleteSummdata = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteSummdataForPortfolio");
		if (!lpprDeleteSummdata )
		{
		  lpfnPrintError("Unable To Load DeleteSummdataForPortfolio function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT18", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteSummdataForSegment = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteSummdata");
		if (!lpprDeleteSummdataForSegment )
		{
		  lpfnPrintError("Unable To Load DeleteSummdata function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT18a", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteMonthSum = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteMonthSumForPortfolio");
		if (!lpprDeleteMonthSum )
		{
		  lpfnPrintError("Unable To Load DeleteMonthSumForPortfolio function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT19", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteMonthSumForSegment = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteMonthSum");
		if (!lpprDeleteMonthSumForSegment )
		{
		  lpfnPrintError("Unable To Load DeleteMonthSum function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT19a", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteDSumdata = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteDSumdataForPortfolio");
		if (!lpprDeleteDSumdata )
		{
		  lpfnPrintError("Unable To Load DeleteDSumdataForPortfolio function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT20", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteDSumdataForSegment = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteDSumdata");
		if (!lpprDeleteDSumdataForSegment )
		{
		  lpfnPrintError("Unable To Load DeleteDSumdata function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT20a", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteUnitvalueSince2 = (LPPR4LONG)GetProcAddress(hStarsIODll, "DeleteUnitValueForPortfolioSince2");
		if (!lpprDeleteUnitvalueSince2)
		{
		  lpfnPrintError("Unable To Load DeleteUnitValueForPortfolioSince2 function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT21", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteUnitvalueSinceForSegment = (LPPR4LONG)GetProcAddress(hStarsIODll, "DeleteUnitValueSince");
		if (!lpprDeleteUnitvalueSinceForSegment )
		{
		  lpfnPrintError("Unable To Load DeleteUnitValueSince function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT21a", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprInsertPeriodSummdata = (LPPRSUMMDATA)GetProcAddress(hStarsIODll, "InsertPeriodSummdata");
		if (!lpprInsertPeriodSummdata )
		{
		  lpfnPrintError("Unable To Load InsertPeriodSummdata function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT22", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprInsertUnitValue = (LPPRINSERTUNITVALUE)GetProcAddress(hStarsIODll, "InsertUnitValue");
		if (!lpprInsertUnitValue )
		{
		  lpfnPrintError("Unable To Load InsertUnitValue function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT23", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprInsertMonthlySummdata = (LPPRSUMMDATA)GetProcAddress(hStarsIODll, "InsertMonthlySummdata");
		if (!lpprInsertMonthlySummdata )
		{
		  lpfnPrintError("Unable To Load InsertMonthlySummdata function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT24", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprInsertDailySummdata = (LPPRSUMMDATA)GetProcAddress(hStarsIODll, "InsertDailySummdata");
		if (!lpprInsertDailySummdata )
		{
		  lpfnPrintError("Unable To Load InsertDailySummdata function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT25", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprCopySummaryData = (LPPRCOPYSUMMARYDATA)GetProcAddress(hStarsIODll, "CopySummaryData");
		if (!lpprCopySummaryData )
		{
		  lpfnPrintError("Unable To Load CopySummaryData function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT26", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteTaxperf = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteTaxperf");
		if (!lpprDeleteTaxperf )
		{
		  lpfnPrintError("Unable To Load DeleteTaxperf function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT27", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprDeleteTaxperfForSegment = (LPPR3LONG)GetProcAddress(hStarsIODll, "DeleteTaxperfForSegment");
		if (!lpprDeleteTaxperfForSegment )
		{
		  lpfnPrintError("Unable To Load DeleteTaxperfForSegment function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT27a", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprSummarizeTaxperf = (LPPRSUMMARIZETAXPERF)GetProcAddress(hStarsIODll, "SummarizeTaxPerf");
		if (!lpprSummarizeTaxperf )
		{
		  lpfnPrintError("Unable To Load SummarizeTaxperf function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT28", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprSummarizeMonthsum = (LPPRSUMMARIZEMONTHSUM)GetProcAddress(hStarsIODll, "SummarizeMonthsum");
		if (!lpprSummarizeMonthsum )
		{
		  lpfnPrintError("Unable To Load SummarizeMonthsum function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT29", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprSummarizeInceptionSummdata = (LPPRSUMMARIZEINCEPTIONSUMMDATA)
									   GetProcAddress(hStarsIODll, "SummarizeInceptionSummdata");
		if (!lpprSummarizeInceptionSummdata )
		{
		  lpfnPrintError("Unable To Load SummarizInceptionSummdata function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INIT30", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprSubtractInceptionSummdata = (LPPRSUBTRACTINCEPTIONSUMMDATA) GetProcAddress(hStarsIODll, "SubtractInceptionSummdata");
		if (!lpprSubtractInceptionSummdata )
		{
		  lpfnPrintError("Unable To Load SubtractInceptionSummdata function", 0, 0, "", 
											GetLastError(), 0, 0, "CompCreate INIT31", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpprSelectSysvalues =	(LPPRSELECTSYSVALUES)GetProcAddress(hStarsIODll, "SelectSysvalues");
		if (!lpprSelectSysvalues)
		{
			lpfnPrintError("Unable To Load SelectSysvalues", 0, 0, "", 
												   GetLastError(), 0, 0, "CompCreate INIT32", FALSE);
			zErr->iBusinessError = -1;
			return;
		}

		lpprInitPerformance = (LPFN1LONG3PCHAR1BOOL1PCHAR)GetProcAddress(hPerformanceDll, "InitPerformance");
		if (!lpprInitPerformance)
		{
		  lpfnPrintError("Unable To Load InitPerformance function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INITP01", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpfnAddNewSegment = (LPFN1INT2PCHAR1INT	)GetProcAddress(hPerformanceDll, "AddNewSegment");
		if (!lpfnAddNewSegment )
		{
		  lpfnPrintError("Unable To Load AddNewSegment function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INITP02", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpfnCurrentDateAndTime =	(LPFN2VOID)GetProcAddress(hStarsUtilsDll, "CurrentDateAndTime");
		if (!lpfnCurrentDateAndTime)
		{
		  lpfnPrintError("Unable To Load CurrentDateAndTime function", 0, 0, "", 
													GetLastError(), 0, 0, "CompCreate INITS01", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpfnLastMonthEnd = (LPFN1INT)GetProcAddress(hStarsUtilsDll, "LastMonthEnd");
		if (!lpfnLastMonthEnd)
		{
			lpfnPrintError("Error Loading LastMonthEnd Function", 0, 0, "",
													GetLastError(), 0, 0, "CompCreate INITS02", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		lpfnIsItAMonthEnd = (LPFN1INT)GetProcAddress(hStarsUtilsDll,"IsItAMonthEnd");   
		if(!lpfnIsItAMonthEnd)
		{
			lpfnPrintError("Error Loading IsItAMonthEnd Function", 0, 0, "",
													GetLastError(), 0, 0, "CompCreate INITS03", FALSE);			
			zErr->iBusinessError = -1;
			return;
		}

		*zErr = lpprInitPerformance(lAsofDate, sDBAlias, sMode, sType, FALSE, sErrFile );
		if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
			return;

		strcpy_s(sInitDBAlias, sDBAlias);
		strcpy_s(sInitMode, sMode);
		strcpy_s(sInitType, sType);
		strcpy_s(sInitErrFile, sErrFile);

		strcpy_s(zSysvalues.sName, "GracePeriodLen");
		lpprSelectSysvalues(&zSysvalues, zErr);
	  if (zErr->iSqlError == 0 && zErr->iBusinessError == 0) 
		  bGracePeriod = (BOOL) atoi(zSysvalues.sValue) > 0;
	  else if (zErr->iSqlError == SQLNOTFOUND)
		  lpprInitializeErrStruct(zErr);
    else 
		   return;

		bCompCreateInit = TRUE;
	}/*if bTProcInitialized*/
}/*InitializeDllRoutines*/

/**
**	Initializes/Free the HoldingsTable structure.
**/
void InitializeHoldingsTable(HOLDTABLE *pHoldingsTable)
{
 
  if (pHoldingsTable->iNumHoldCreated > 0 && pHoldingsTable->pzHoldings != NULL)
    free(pHoldingsTable->pzHoldings);
 
  pHoldingsTable->pzHoldings = NULL;
  pHoldingsTable->iNumHoldings = pHoldingsTable->iNumHoldCreated = 0;
}/*InitializeHoldingsTable*/                                    /* InitializeHoldingsTable */

/**
** Function to initialize holdcash table structure
**/
void InitializeHoldcashTable(HOLDCASHTABLE *pHoldcashTable)
{
	if (pHoldcashTable->iNumHoldCashCreated > 0 && pHoldcashTable->pzHoldCash != NULL)
		free(pHoldcashTable->pzHoldCash);
	pHoldcashTable->pzHoldCash = NULL;
	pHoldcashTable->iNumHoldCashCreated = pHoldcashTable->iNumHoldCash = 0;
}/*InitializeHoldcashTable*/

/**
	This function checks if the input key already exist in the holdcash table(memory) 
	if it does exist then return the index of the matched holdcash record.
**/
int FindHoldCashKey(HOLDCASHTABLE *pHoldCashTable,HOLDCASHKEY zHoldCashKey)
{
	int i; 
	
	for (i = 0;i < pHoldCashTable->iNumHoldCash;i++)
	{
		/* Check if the same key exists*/	
		if (strcmp(pHoldCashTable->pzHoldCash[i].sSecNo,zHoldCashKey.sSecNo) == 0 &&
				strcmp(pHoldCashTable->pzHoldCash[i].sWi,zHoldCashKey.sWi) == 0 &&
				strcmp(pHoldCashTable->pzHoldCash[i].sSecXtend,zHoldCashKey.sSecXtend) ==0 &&
				strcmp(pHoldCashTable->pzHoldCash[i].sAcctType,zHoldCashKey.sAcctType) == 0)
		{
			return i;
		}
	}/* for loop*/
	return -1;
}/*FindHoldCashKey*/

/**
**This routine Merges Performance Data for a composite
**/
DLLAPI ERRSTRUCT STDCALL WINAPI MergeCompositePortfolio(int iID, long lFromDate, long lToDate, 
																									 BOOL bDaily, long lEarliestDate, char *sInSessionID)
{
	ERRSTRUCT zErr, zErr2;
	SEGMAIN   zSegmain;
	SUMMDATA  zMonthsum, zSummdata;
	UNITVALUE zUV;
	BOOL			bInTrans = FALSE;
	BOOL			bDelete  = TRUE;
	BOOL			bTaxPerf = FALSE;
	BOOL			bNeedMonthlyFlows;
	BOOL			bSinceInception;
	BOOL			bMergeOnAFly = FALSE;
	BOOL			bIsItAMonthEnd;
	int				i = 0;
	double		fROR;
	long			lTempDate, lTempDate1, lTempDate2;
	int				iCurrSegTypeID = 0;
	int				iCurrSegID = 0;
	int				iLastRorType = 0;
	char			sSessionID[36+NT];
 	lpprInitializeErrStruct(&zErr);
 	lpprInitializeErrStruct(&zErr2);
	memset(&zSummdata, 0, sizeof(zSummdata));

	__try
	{

	__try
	{
		// if required, then build segment map first
		// ("merge on a fly" call from reports creates it's own segment map)
		bMergeOnAFly = (sInSessionID != NULL);
		if (!bMergeOnAFly)
		{
			lpprBuildMergeCompSegMap(iID, lFromDate, lToDate, (char *)&sSessionID, &zErr);
		} 
		else 
		{
			strcpy_s(sSessionID, sInSessionID);
			// if called from reports (SessionID == NULL) then do not work with daily values
			bDaily = FALSE;
			// need to determine composite tax rate (when called from reports)
			lpprBuildMergeCompport(iID, lToDate, sSessionID, &zErr);
		}	
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		// fix month end unitvalues with incorrect ror source (like 3 if EOM is also IPV) 
		lpprUpdateUnitvalueMonthlyIPV(sSessionID, lFromDate, lToDate, &zErr);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		// build intermidate data in Merge_SData table
		lpprBuildMergeSData(iID, lFromDate, lToDate, sSessionID, &zErr);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		// build intermediate data in Merge_UV table
		lpprBuildMergeUV(iID, lFromDate, lToDate, sSessionID, &zErr);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		// check for Grace Period and remove returns which qualify
		if (bGracePeriod) 
        {
			lpprDeleteMergeUVGracePeriod(sSessionID, iID, &zErr);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			   return zErr;
		}	

		lTempDate2 = lToDate;
		lTempDate1 = lpfnLastMonthEnd(lTempDate2);
		bNeedMonthlyFlows = (lFromDate < lTempDate1);
		bIsItAMonthEnd = lpfnIsItAMonthEnd(lToDate);
	    bSinceInception = (lFromDate<=lEarliestDate && !lpfnIsItAMonthEnd(lEarliestDate));
		while (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
		{ 
			// build and/or pickup summarized data from "prebuilt" values in Merge_SData
			lpprGetSummarizedDataForCompositeEx(&zSegmain,	&zMonthsum, &zUV, &fROR, 
												iID, lFromDate, lToDate, bDaily, sSessionID, &zErr);
			
			if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) 
			{
					if (zMonthsum.lPerformDate != 0 && zUV.lUVDate != 0) // if have both summary record and return
					{ 
							
							if (zSummdata.iID != 0 && zSummdata.iID != zMonthsum.iID && 
							   (!bDaily || bIsItAMonthEnd))
							{	// save Period record for previous segment we were working with
								lpprInsertPeriodSummdata(zSummdata, &zErr);
								if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									break;

								zSummdata.iID = 0;
							}
							if (zSummdata.iID != 0 && zSummdata.iID == zMonthsum.iID && 
								zSummdata.lPerformDate > zMonthsum.lPerformDate && 

							   (!bDaily || bIsItAMonthEnd))
							{	// save Period record for previous segment we were working with
								lpprInsertPeriodSummdata(zSummdata, &zErr);
								if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									break;
							}
							// if this is "Merge on a fly" then delete has to happen for each segment
							// (but once per ror_type)
 							// while for regular merge it only happens once for entire composite
							// and therefore, bDelete will always be set to FALSE after 1st delete occurs
							bDelete = bDelete || (bMergeOnAFly && (zUV.iRorType >= iLastRorType));
							iLastRorType = zUV.iRorType;

							if (bDelete) 
							{ // started processing -- delete old summary data for entire composite or segment
								if (lpfnGetTransCount()==0)
								{
									lpfnStartTransaction();
									bInTrans = TRUE;
								}
								
								if (!bMergeOnAFly)
								{ 
									lpprDeleteSummdata (iID, lFromDate + 1, -1, &zErr);
									if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									   break;

									if (bSinceInception)
									   lpprDeleteSummdata (iID, lEarliestDate, lEarliestDate, &zErr);
								} 
								else
								{ 
									lpprDeleteSummdataForSegment (zSegmain.iID, lFromDate + 1, lToDate, &zErr);
									if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									   break;

									if (bSinceInception)
									   lpprDeleteSummdataForSegment (zSegmain.iID, lEarliestDate, lToDate, &zErr);
								}

								if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									break;


								if (!bMergeOnAFly)
									lpprDeleteMonthSum (iID, lFromDate + 1, -1, &zErr);
								else
									lpprDeleteMonthSumForSegment (zSegmain.iID, lFromDate + 1, lToDate, &zErr);

								if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									break;


								if (!bMergeOnAFly)
									lpprDeleteTaxperf (iID, lToDate, -1, &zErr);
								else
									lpprDeleteTaxperfForSegment (zSegmain.iID, lToDate, lToDate, &zErr);

								if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									break;

								// delete portfolio's unitvalues
								if (!bMergeOnAFly)
								{
									// for entire composite, ALL return types in one shot
									if (!bDaily) 
										lpprDeleteUnitvalueSince2(iID, lFromDate + 1, 
												GTWRor, NTWAfterTaxRor, &zErr);
									else
										lpprDeleteUnitvalueSince2(iID, lToDate, 
												GTWRor, NTWAfterTaxRor, &zErr);
								} 
								else 
								{
									// for segment, each return type individually
									for (i = iLastRorType; i > ROR_SUMMARY; i--)
									{
										
										// do not delete PCT-related return types 
										if (i> NTWAfterTaxRor) 
											continue;

										if (!bDaily) 
											lpprDeleteUnitvalueSinceForSegment(iID, zSegmain.iID, lFromDate + 1, i, &zErr);
										else
											lpprDeleteUnitvalueSinceForSegment(iID, zSegmain.iID, lToDate, i, &zErr);
									}
								} 
	
								if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
									break;

								// Delete DSUMDATA entirely for a portfolio 
								// except for a current month
								if (bDaily)
								{		
									if (!bMergeOnAFly)
										lpprDeleteDSumdata (iID, lToDate, lToDate, &zErr);
									else	
										lpprDeleteDSumdataForSegment (zSegmain.iID, 0, lFromDate, &zErr);
									
									if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
										break;

									if (!bMergeOnAFly)
										lpprDeleteDSumdata (iID, lToDate, -1, &zErr);
//										lpprDeleteDSumdata (iID, lFromDate, -1, &zErr);
									else	
										lpprDeleteDSumdataForSegment (zSegmain.iID, lToDate, -1, &zErr);
//										lpprDeleteDSumdataForSegment (zSegmain.iID, lFromDate, -1, &zErr);

									if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
										break;

									// and then copy last period into DSUMDATA
									if (!bMergeOnAFly)
										lpprCopySummaryData (iID, lToDate, "SUMMDATA", "DSUMDATA", &zErr);

									if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
										break;
								}
								
								// set bDelete to FALSE always after 1st occurence
								bDelete = FALSE;
							}

							// for PCT-related returns, delete them 
							// only if we're about to produce new calcualted values
							// since code above purposely does not delete them by default
							if (iLastRorType > NTWAfterTaxRor)
							{   
							  if (!bDaily) 
								lpprDeleteUnitvalueSinceForSegment(iID, zSegmain.iID, lFromDate + 1, iLastRorType, &zErr);
							  else
								lpprDeleteUnitvalueSinceForSegment(iID, zSegmain.iID, lToDate, iLastRorType, &zErr);
							}  

							if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
								break;

							if (zSegmain.iID == 0) // if new segment for a composite - create it	
							{
								if (zSegmain.iSegmentTypeID != iCurrSegTypeID)
								{
									zSegmain.iID = lpfnAddNewSegment(iID, "", "", zSegmain.iSegmentTypeID);
								
									if (zSegmain.iID == -1)	// if failed to create segment -exit
									{
										zErr.iBusinessError = 520;
										break;
									}
									iCurrSegTypeID = zSegmain.iSegmentTypeID;
									iCurrSegID = zSegmain.iID;
								}
								else
									zSegmain.iID = iCurrSegID;
							}

							// orig query is sorted by segments and ror_type (in reversed order, 
							// from NTWAfterTaxRor to GTWRor)
							// so, when I go through all return types, 
							// I can adjust summarized values and then save them all at once
							// once I reach record for GTWRor 
							switch (zUV.iRorType)
							{
								case GTWTaxEquivRor:

											bTaxPerf = TRUE;
											break;
								
								case NTWTaxEquivRor:

											bTaxPerf = TRUE;
											break;
								
								case GTWAfterTaxRor:

											bTaxPerf = TRUE;
											break;
								
								case NTWAfterTaxRor:

											bTaxPerf = TRUE;
											break;
								
								case  ROR_SUMMARY:

										// set up missing fields for Month & Period Summary records
										zMonthsum.iID = zSegmain.iID;
										zMonthsum.fCreateDate = lpfnCurrentDateAndTime();

										if (!bDaily || bIsItAMonthEnd)
										{
											strcpy_s(zMonthsum.sIntervalType, "MV");
											strcpy_s(zMonthsum.sPerformType, "M");
										}
										else
										{
											strcpy_s(zMonthsum.sIntervalType, "MV");
											strcpy_s(zMonthsum.sPerformType, "D");
										}	
										
										// copy Monthly record to the Period one
											zSummdata = zMonthsum;

/* removed 7/1/2014
										if (((bDaily) ||(zMonthsum.lPerformDate == lToDate)) || ((zMonthsum.lPerformDate < lToDate) & (!lpfnIsItAMonthEnd(zMonthsum.lPerformDate))))
										{
											zSummdata = zMonthsum;
										}
										else
										{
											// if working with quarterly composites/monthly members
											// then query will return records for each month (with "to-date" returns)
											// So, we have to aggregate data first
											// (i.e. set Mkt Val to Invalid on MontSum for non-quarter date
											// and summarize flows, etc for a quarter end)
											 
											zSummdata.fNetFlow += zMonthsum.fNetFlow;
											zSummdata.fWtdFlow += zMonthsum.fWtdFlow;
											zSummdata.fPurchases += zMonthsum.fPurchases;
											zSummdata.fSales += zMonthsum.fSales;
											zSummdata.fIncome += zMonthsum.fIncome;
											zSummdata.fWtdInc += zMonthsum.fWtdInc;
											zSummdata.fFees += zMonthsum.fFees;
											zSummdata.fWtdFees += zMonthsum.fWtdFees;
											zSummdata.fPrincipalPayDown += zMonthsum.fPrincipalPayDown;
											zSummdata.fMaturity += zMonthsum.fMaturity;
											zSummdata.fContribution += zMonthsum.fContribution;
											zSummdata.fWithdrawals += zMonthsum.fWithdrawals;
											zSummdata.fExpenses += zMonthsum.fExpenses;
											zSummdata.fReceipts += zMonthsum.fReceipts;
											zSummdata.fIncomeCash += zMonthsum.fIncomeCash;
											zSummdata.fPrincipalCash += zMonthsum.fPrincipalCash;
											zSummdata.fFeesOut += zMonthsum.fFeesOut;
											zSummdata.fTransfers += zMonthsum.fTransfers;
											zSummdata.fTransfersIn += zMonthsum.fTransfersIn;
											zSummdata.fTransfersOut += zMonthsum.fTransfersOut;
											zSummdata.fEstAnnIncome += zMonthsum.fEstAnnIncome;
											zSummdata.fNotionalFlow += zMonthsum.fNotionalFlow;											

									 
											// for Interim months of  Quarterly composite
											// WF = 0 and Mkt Val = INVALID 
											zMonthsum.fWtdFlow = 0;
											zMonthsum.fMktVal = MKT_VAL_INVALID;

											strcpy_s(zSummdata.sIntervalType, "QV");
											strcpy_s(zSummdata.sPerformType, "Q");
										}
*/

										break;
							} // switch

							// now save summary/monthly records
							if (zUV.iRorType == ROR_SUMMARY)
							{
									if (!bDaily) 
									{
										if (!bNeedMonthlyFlows) 
											lpprInsertMonthlySummdata(zMonthsum, &zErr);

									}
									else
									{   
										if (zMonthsum.lPerformDate == lToDate)
										{
											lpprInsertDailySummdata(zMonthsum, &zErr);
											if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
												break;
										}

										if (bIsItAMonthEnd)
										{
											if (!bNeedMonthlyFlows ||zMonthsum.lPerformDate == lToDate)
											{
												lpprInsertMonthlySummdata(zMonthsum, &zErr);
											}
										}
									}

									if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
										break;

									/* don't save monthly records in summdata
									if (zMonthsum.lPerformDate != lToDate) 
									{	// save using Month End data (for Interim periods)
										lpprInsertPeriodSummdata(zMonthsum, &zErr);
										if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
											break;
									}
									*/
							} // if

							// now save unitvalues - for ToDate only
							if ((zMonthsum.lPerformDate == lToDate) && 
								  (zUV.iRorType != ROR_SUMMARY)) 
							{
					
								// if UV has become 0 or negative for any reason, then stream can't continue
								// we must ensure new stream is starting (i.e. data hole appears)
								// (also, if UV is extremely high and won't fit table field spec)
								// and recalc UV using assumed BUV = 100
								if (IsValueZero(zUV.fUnitValue, 7) 
									    //|| zUV.fUnitValue<0 
										|| fabs(zUV.fUnitValue)>1e15) 
								{
									zUV.fUnitValue = fROR + 100;
									lTempDate = lpfnLastMonthEnd(zUV.lUVDate);
									if (zUV.lStreamBeginDate < lTempDate)
										zUV.lStreamBeginDate = lTempDate;
								}	

								if (zUV.fUnitValue > 0)
								{

									if (!((IsValueZero(zUV.fUnitValue, 7) 
											//|| zUV.fUnitValue<0 
											|| fabs(zUV.fUnitValue)>1e15))) 
									{
										zUV.iID = zSegmain.iID;
										if (bDaily && !bIsItAMonthEnd)
											zUV.iRorSource = rsMonthToDate;

										lpprInsertUnitValue(zUV, &zErr);
										if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
											break;
									}
								}
							} // if (zMonthsum.lPerformDate != lToDate) 
					}

			} else if (zErr.iSqlError == SQLNOTFOUND)
			{
				lpprInitializeErrStruct(&zErr);
			    break;
			} else 
				return zErr;
		}/*while*/

		// if any operations from the loop above failed, exit with error
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		// save last remaining Period record for previous segment we were working with
		if (zSummdata.iID != 0) 
		{
			if (!bDaily || bIsItAMonthEnd) 
			{
		 		lpprInsertPeriodSummdata(zSummdata, &zErr);
				if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
					return zErr;
			}
		}

		if (bTaxPerf && !bMergeOnAFly) 
		{
			lpprSummarizeTaxperf (iID, lFromDate, lToDate, sSessionID, &zErr);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				return zErr;
		}

		if (bNeedMonthlyFlows)
		{
			// if merging more than 1 month period (i.e. Quarterly composite)
			// then need to summarize MonthSum records separately 
			// to get Monthly flows
			while (lFromDate <= lTempDate1) 
			{
					
			  lpprSummarizeMonthsum (iID, lTempDate1, lTempDate2, lToDate, sSessionID, &zErr);
			  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				  return zErr;

				lTempDate2 = lTempDate1;
				lTempDate1 = lpfnLastMonthEnd(lTempDate2);
			}
		} // bNeedMonthlyFlows

		// create inception record for the composite if needed
		// make sure the data actually has been generated for the period
		// (i.e. clean up logic occured above and bDelete is FALSE now
/*
		if (bSinceInception & !bDelete)
		{
			lpprSummarizeInceptionSummdata (iID, lEarliestDate, sSessionID, &zErr);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				return zErr;

			lpprSubtractInceptionSummdata (iID, lEarliestDate, lToDate, &zErr);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				return zErr;
		}
*/

	}//try..except

	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		zErr.iBusinessError = -999;
	}

	}//try..finally

	__finally
	{
		if (bInTrans)
		{
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				lpfnRollbackTransaction();
			else
				lpfnCommitTransaction();
		}	

		// temporary data was created outside of DB trans, 
		// so, clean up should occur outside DB trans as well
		if (!bMergeOnAFly)
		{  
			lpprDeleteMergeSessionData(sSessionID, iID, &zErr2);

			// if all operations above were successful, then use latest error code, zErr2,
			// (from cleanup call) as final result 
			// otherwise, report and return original error code, zErr
			if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
				zErr = zErr2;
		}		

	}

	return zErr;	
}/*MergeCompositePortfolio*/


/*
DLLAPI void STDCALL WINAPI CreateCompositeHoldTot(int iCompositeId,long  lDate,char *sHoldTotTable,ERRSTRUCT *zErr)
{
	int iPortfolioID;
	HOLDTOT zHoldTot;
	HOLDTOTTABLE zCompositeHoldTot;

  lpprInitializeErrStruct(zErr);
  InitializeCompositeHoldTot(&zCompositeHoldTot);
	zCompositeHoldTot.pzHoldTot->iId = iCompositeId;
	while (zErr->iSqlError == 0)
	{ 
    lpprInitializeErrStruct(zErr);

		/*Get member of given composite*/
    /*lpprSelectAllMembersOfAComposite (iCompositeId, lDate, &iPortfolioID, zErr );
    if (zErr->iSqlError == SQLNOTFOUND)
				break;
		memset(&zHoldTot,sizeof(HOLDTOT),0);
		while (zErr->iSqlError == 0)
		{	
			lpprSelectHoldtotFor(iCompositeId,&zHoldTot,sHoldTotTable,zErr);
			if (zErr->iSqlError != 0 || zErr->iBusinessError != 0)
					break;
			AddHoldtoToComposite(&zCompositeHoldTot,zHoldTot);
		}//while
	}//while
	//iLastSegmentId = zCompositeHoldTot.pzHoldTot[0].iSegmentTypeID;  
	
}//CreateCompositeHoldTot

int IsSegmentExist(HOLDTOTTABLE* zCompositeHoldTot,int iSegmentTypeId)
{
	int i;

	for (i=0;i<zCompositeHoldTot->iTotRecords;i++)
	{
		if (iSegmentTypeId == zCompositeHoldTot->pzHoldTot[i].iSegmentTypeID)
				return i;
	}
	return -1;
}

void 	AddHoldtoToComposite(HOLDTOTTABLE* zCompositeHoldTot,HOLDTOT zHoldTot)
{
	int index;

	if (zCompositeHoldTot->iCreated == zCompositeHoldTot->iTotRecords){
		zCompositeHoldTot->iCreated += EXTRA;
		zCompositeHoldTot->pzHoldTot = (HOLDTOT*)realloc(zCompositeHoldTot->pzHoldTot,zCompositeHoldTot->iCreated*sizeof(HOLDTOT));
    if (zCompositeHoldTot->pzHoldTot == NULL)
		{
			lpfnPrintError("Insufficient Memory For HoldTable", 0, 0, "", 997,
                        0, 0, "COMPCREATE AddHoldtoToComposite", FALSE);
			return;
		}
		index = IsSegmentExist(zCompositeHoldTot,zHoldTot.iSegmentTypeID);
		if (index = -1) 
			zCompositeHoldTot->pzHoldTot[zCompositeHoldTot->iTotRecords++] = zHoldTot;
		else
			SumUpHoldTot(&zCompositeHoldTot->pzHoldTot[index],zHoldTot);
	}
}//AddHoldtoToComposite


/* This routine free up the allocated memory*/
/*
void InitializeCompositeHoldTot(HOLDTOTTABLE* zCompositeHoldTot)
{
	if (zCompositeHoldTot->iCreated > 0 && zCompositeHoldTot->pzHoldTot != NULL)
		free(zCompositeHoldTot->pzHoldTot);
	zCompositeHoldTot->pzHoldTot = NULL;
	zCompositeHoldTot->iCreated = zCompositeHoldTot->iTotRecords = 0;
}

void SumUpHoldTot(HOLDTOT *zFinalHoldTot,HOLDTOT zHoldTot)
{
	zFinalHoldTot->fNativeOrigCost += zHoldTot.fNativeOrigCost; 
	zFinalHoldTot->fNativeTotCost += zHoldTot.fNativeTotCost;
	zFinalHoldTot->fNativeMarketValue += zHoldTot.fNativeMarketValue;
	zFinalHoldTot->fNativeAccrual += zHoldTot.fNativeAccrual;
	zFinalHoldTot->fNativeGainorloss += zHoldTot.fNativeGainorloss;
	zFinalHoldTot->fNativeAccrGorl += zHoldTot.fNativeAccrGorl;
	zFinalHoldTot->fNativeCurrGorl += zHoldTot.fNativeCurrGorl;
	zFinalHoldTot->fNativeSecGorl += zHoldTot.fNativeSecGorl;
	zFinalHoldTot->fNativeCostYield += zHoldTot.fNativeCostYield;
	zFinalHoldTot->fNativeCurrYield += zHoldTot.fNativeCurrYield;
	zFinalHoldTot->fNativeAvgPriceChange+= zHoldTot.fNativeAvgPriceChange;
	zFinalHoldTot->fNativeWtdavgChange += zHoldTot.fNativeWtdavgChange;
	zFinalHoldTot->fNativeIncome += zHoldTot.fNativeIncome;
	zFinalHoldTot->fNativeParvalue += zHoldTot.fNativeParvalue;
	zFinalHoldTot->fBaseOrigCost += zHoldTot.fBaseOrigCost;
	zFinalHoldTot->fBaseTotCost += zHoldTot.fBaseTotCost;
	zFinalHoldTot->fBaseMarketValue += zHoldTot.fBaseMarketValue;
	zFinalHoldTot->fBaseAccrual += zHoldTot.fBaseAccrual;
	zFinalHoldTot->fBaseGainorloss += zHoldTot.fBaseGainorloss;
	zFinalHoldTot->fBaseAccrGorl += zHoldTot.fBaseAccrGorl;
	zFinalHoldTot->fBaseCurrGorl += zHoldTot.fBaseCurrGorl;
	zFinalHoldTot->fBaseSecGorl += zHoldTot.fBaseSecGorl;
	zFinalHoldTot->fBaseCostYield += zHoldTot.fBaseCostYield;
	zFinalHoldTot->fBaseCurrYield += zHoldTot.fBaseCurrYield;
	zFinalHoldTot->fBaseAvgPriceChange += zHoldTot.fBaseAvgPriceChange;
	zFinalHoldTot->fBaseWtdavgChange += zHoldTot.fBaseWtdavgChange;
	zFinalHoldTot->fBaseIncome += zHoldTot.fBaseIncome;
	zFinalHoldTot->fBaseParvalue += zHoldTot.fBaseParvalue;
	zFinalHoldTot->fSystemOrigCost += zHoldTot.fSystemOrigCost;
	zFinalHoldTot->fSystemTotCost += zHoldTot.fSystemTotCost;
	zFinalHoldTot->fSystemMarketValue += zHoldTot.fSystemMarketValue;
	zFinalHoldTot->fSystemAccrual += zHoldTot.fSystemAccrual;
	zFinalHoldTot->fSystemGainorloss += zHoldTot.fSystemGainorloss;
	zFinalHoldTot->fSystemAccrGorl += zHoldTot.fSystemAccrGorl;
	zFinalHoldTot->fSystemCurrGorl += zHoldTot.fSystemCurrGorl;
	zFinalHoldTot->fSystemSecGorl += zHoldTot.fSystemSecGorl;
	zFinalHoldTot->fSystemCostYield += zHoldTot.fSystemCostYield;
	zFinalHoldTot->fSystemCurrYield += zHoldTot.fSystemCurrYield;
	zFinalHoldTot->fSystemAvgPriceChange += zHoldTot.fSystemAvgPriceChange;
	zFinalHoldTot->fSystemWtdavgChange += zHoldTot.fSystemWtdavgChange;
	zFinalHoldTot->fSystemIncome += zHoldTot.fSystemIncome;
	zFinalHoldTot->fSystemParvalue += zHoldTot.fSystemParvalue;
}
*/


void FreeCompositeCreateLibrary()
{
	if (bCompCreateInit)
	{
		FreeLibrary(hStarsIODll);
		FreeLibrary(hStarsUtilsDll);
		FreeLibrary(hTransEngineDll);
		FreeLibrary(hPerformanceDll);

		bCompCreateInit = FALSE;
	}
} // FreeCompositeCreateLibrary