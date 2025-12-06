/**
* 
* SUB-SYSTEM: TranProc  
* 
* FILENAME: TranProc.c
* 
* DESCRIPTION: This routine will retrieve a group of transactions by portfolio ID,
*              security id and process tag.  It verifies and complete the data and 
*              co-ordinate the posting of the trades with TranAlloc and
*              UpdateHold.
* 
* PUBLIC FUNCTION(S): TranProc    
* 
* NOTES:  
*        
* 
* USAGE:  ERRSTRUCT WINAPI TranProc(int iID, char *sSecNo, char *sWi, long lTagNo, char *sCurrPrior, BOOL bRebook, BOOL bDoTransaction)
*   WHERE        
*          iID						is the account identifier                          
*          sSecNo					is the security number of the assets that will be processed
*					 sWi						is the when issue of the assets that will be processed
*          lTagNo					identifies the block of transactions to be processed
*					 sCurrPrior			identifies whether the program ios called for Current or Prior
*					 bRebook				Flag to tell whether to Rebook trades that are eligible for rebooking
*													while doing a reversal
*					 bDoTransaction	Whether to do database transaction (begin .. commit/rollback) or not
* 
* 
* AUTHOR: Shobhit Barman (Effron Enterprises, Inc.)
*
*
**/


#include "transengine.h"
#include <time.h>
#include <string.h>


/**
** Main function for the dll
**/
BOOL APIENTRY DllMain (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
							break;
		
		case DLL_PROCESS_DETACH:
							FreeTranProc();
							break;
		
		default : break;
	}

  return TRUE;
} //DllMain


/**
** Function that calls InternalTranProc and upon return it calls the function to update all 
** the entries in Dtrans which were successfully or unsuccessfully processed
** by InternalTranProc(all entries that match the given iID, SecNo, Wi and TagNo).
**/
DLLAPI ERRSTRUCT STDCALL WINAPI TranProc(int iID, char *sSecNo, char *sWi, long lTagNo, char *sCurrPrior, BOOL bRebook, BOOL bDoTransaction)
{
	ERRSTRUCT   zErr, zErr1;
  char        sTime[9], sDate[12];
	long				lDate;


	bDoTransaction = bDoTransaction && (lpfnGetTransCount()==0);
	if (bDoTransaction)
	{
		zErr.iBusinessError = lpfnStartTransaction();
		if (zErr.iBusinessError != 0)
			return zErr;
	}
	
	__try
	{
		zErr = InternalTranProc(iID, sSecNo, sWi, lTagNo, sCurrPrior, bRebook, "", FALSE);
	}
	__except(lpfnAbortTransaction(bDoTransaction)){}

	if (bDoTransaction)
	{
		if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
			lpfnCommitTransaction();
		else
			lpfnRollbackTransaction();
	} 
	
	_strtime(sTime);
	_strdate(sDate);
	lpfnrstrdate(sDate, &lDate);

	if (zErr.iSqlError == 0 && zErr.iBusinessError == 0) // Posted with no errors
	  lpprUpdateDtrans(iID, sSecNo, sWi, lTagNo, "P", 0, 0, 0, lDate, sTime, &zErr1);
	else // Error while posting
	  lpprUpdateDtrans(iID, sSecNo, sWi, lTagNo, "E", zErr.iBusinessError, 
										 zErr.iSqlError, zErr.iIsamCode, lDate, sTime, &zErr1);

	return zErr;
} // Tranproc


DLLAPI ERRSTRUCT STDCALL WINAPI TranProcInfo(int iID, char *sSecNo, char *sWi, long lTagNo,char *sFileName)
{
  ERRSTRUCT   zErr, zErr1;
  char        sTime[9], sDate[12];
	long				lDate;

	zErr = InternalTranProc(iID, sSecNo, sWi, lTagNo, "I", TRUE, sFileName, FALSE);
	_strtime(sTime);
	_strdate(sDate);
	lpfnrstrdate(sDate, &lDate);

  if (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
	  lpprUpdateDtrans(iID, sSecNo, sWi, lTagNo, "I", 0, 0, 0, lDate, sTime, &zErr1);
  else
	  lpprUpdateDtrans(iID, sSecNo, sWi, lTagNo, "E", zErr.iBusinessError, 
										 zErr.iSqlError, zErr.iIsamCode, lDate, sTime, &zErr1);

	return zErr;
} // TranprocInfo

/**
** Main function which does all the processing but it is never called by other programs.
** Other programs call TranProc(or TranProcInfo) which calls this program and then updates
** the dtrans entry appropriately based on the error codes returned by this function.
**/
ERRSTRUCT InternalTranProc(int iID, char *sSecNo, char *sWi, long lTagNo, char *sCurrPrior,
													 BOOL bRebook, char *sFileName, BOOL bDoTransaction)
{
  TRANS				zTrans;    
  ASSETS			zAssets;
  TRANTYPE		zTranType;
  SECTYPE		  zSecType;
  PORTMAIN		zPortmain;
  PTABLE			zProcessTable;
  TRANSINFO		zTInfo;
  BOOL				bRecFound, bAnyAsOfs, bAllFlag;
  long			  lTradeDate, lPricingDate;
  char				sRevTranCode[2], sSellAcctType[2];
  ERRSTRUCT		zErr;

	
  InitializeErrStruct(&zErr);

  zProcessTable.iSize = 0;
  InitializePTable(&zProcessTable);
	zAssets.sSecNo[0] = zAssets.sWhenIssue[0] = '\0';

  bAnyAsOfs = FALSE;

  // Fetch the first record from dtrans 
	InitializeTransStruct(&zTrans);
  lpprSelectDtrans(&zTrans, iID, sSecNo, sWi, lTagNo, &zErr);
  if (zErr.iSqlError == SQLNOTFOUND)
		return(PrintError("No Record In Dtrans Table", iID, 0, "", 106, 0, 0, "TRANPROC1", FALSE));
  else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

	/*
  ** verify that account exist in portdir, security exist in assets and sectypes exist for that 
	** security. Since this program is called for one account and security at a time, we don't 
	** need to check this in the loop.
  */
	lpprSelectPortmain(&zPortmain, iID, &zErr);
  if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    return zErr;

  // Get the primary characteristics of the security 
  bAllFlag = FALSE;
  if (strcmp(zTrans.sSecNo, zAssets.sSecNo) != 0 || strcmp(zTrans.sWi, zAssets.sWhenIssue) != 0)
    bAllFlag = TRUE;
  zErr = PrepareForTranAlloc(&zTranType, &zAssets, &zSecType,zTrans,bAllFlag);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) 
    return zErr;

  // Retrieve the Process day, this value will be assigned to the post_date on the transaction record 
	lpprSelectStarsDate(&lTradeDate, &lPricingDate, &zErr);
	if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		return zErr;

  /* 
  ** Fetch all subsequent dtrans rows, check the branch account, security number, account type 
	** and transaction type.  If all are valid, place the entry on the process table
  */
	bRecFound = TRUE;
  while (bRecFound)
  {
    // Verify the transaction type of the trade                   
    zErr = VerifyTransaction(zTrans, &zTranType, sRevTranCode, sSellAcctType); 
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    {
      InitializePTable(&zProcessTable); // free memory 
      return zErr;
    }
     
    // Set the default data 
    zErr = SetDefaultDatainTranproc(&zTrans, zPortmain, zAssets, zTranType, lPricingDate, 
																		lTradeDate, zSecType.sPrimaryType);
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
    {
      InitializePTable(&zProcessTable); // free memory 
      return zErr;
    }
             
    // Set the appropriate values in a temporary transinfo table structure 
    InitializeTransInfo(&zTInfo);
   
    zTInfo.zTrans = zTrans;
    zTInfo.zTrans.lTransNo = 0;
    zTInfo.lDtransNo = zTrans.lDtransNo;
	zTInfo.lBlockTransNo = zTrans.lBlockTransNo;
    zTInfo.zTranType.iTradeSort = zTranType.iTradeSort;
    strcpy_s(zTInfo.zTranType.sTranCode, zTranType.sTranCode);
    strcpy_s(zTInfo.sRevTranCode, sRevTranCode);
    strcpy_s(zTInfo.zTranType.sAutoGen, zTranType.sAutoGen);
    strcpy_s(zTInfo.sSellAcctType, sSellAcctType);

    /* 
    ** If the effective date of the transaction is less than the current processing date, set 
		** the as-of flag, unless the transaction is an opening with no trade date, an income or a 
		** money transaction or a trade for a money market security or a trade for single lot 
		** security (like cash or Money market) 
    */
    if (zTrans.lEffDate < lTradeDate && strcmp(zSecType.sPrimaryType, "M") != 0 &&
				strcmp(zSecType.sLotInd, "S") != 0) 
    {
      if (zTranType.sTranCode[0] != 'I' && zTranType.sTranCode[0] != 'M' &&
          zTranType.sTranCode[0] != 'O' && strcmp(zTrans.sTranType, "EX") != 0
          && strcmp(zTrans.sTranType, "ES") != 0) 
      {
        zTInfo.bAsOfFlag = TRUE;
        bAnyAsOfs = TRUE;
      }
      // an open with zero trade date 
      else if (zTranType.sTranCode[0] == 'O' && (zTrans.lTrdDate != 0))
      {
        zTInfo.bAsOfFlag = TRUE;
        bAnyAsOfs = TRUE;
      }
    } // if zTR.effdate < tradedate 

    /* 
    ** If the tran code from the trantype table is set to "R" (signifies a cancel trade), set 
		** the asof-flag to true.  Reversal transactions are always as-of by definition.
    ** If the transaction is an auto generated income or a split transaction, check the tax lot 
		** column contains a value, if it does not contain a value fail the trade
    */
    if (zTranType.sTranCode[0] == 'R')
    {
      zTInfo.bAsOfFlag = TRUE;
      bAnyAsOfs = TRUE;
    }
   
    // Add the transaction to the process table 
    zErr = AddTransToPTable(&zProcessTable, zTInfo);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    {
      InitializePTable(&zProcessTable); // free memory 
      return zErr;
    }

    // Fetch next transaction 
		InitializeTransStruct(&zTrans);
    lpprSelectDtrans(&zTrans, iID, sSecNo, sWi, lTagNo, &zErr);
    if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
    {
      InitializePTable(&zProcessTable); // free memory 
      return zErr;
    }
    if (zErr.iSqlError == SQLNOTFOUND)
		{
      zErr.iSqlError = 0;
			bRecFound = FALSE;
		}
  } // while(bRecFound) 

  // Process all the enteries on the process table 
  zErr = LockPortAndProcess(zProcessTable, zAssets, zPortmain, zTranType, zSecType, lTradeDate, 
														bAnyAsOfs, sCurrPrior, bRebook, sFileName, bDoTransaction);

  // free memory allocated to processtable 
  InitializePTable(&zProcessTable);

  return zErr;
} // TranProc 


/** 
** This is the controlling function of this routine, it locks the portfolio for processing, 
** builds a cancel table of trades to be reversed if there were any as-of trades that trigger 
** cancellations, merges the process table with the cancel table enteries that need to be 
** rebooked and sends the trades to the TranAlloc routine for final processing 
** before being sent to Updhold for updating
*/
ERRSTRUCT LockPortAndProcess(PTABLE zProTable, ASSETS zAssts, PORTMAIN zPmain, TRANTYPE zTrType,
	SECTYPE zSType, long lCurrDate, BOOL bAnyAsOfs, char *sCurrPrior,
	BOOL bRebook, char *sFileName, BOOL bDoTransaction)
{
	long			 lRevtrans, lTransNo, lEffDate, lDtransNo, lBlockTransNo;
	TRANSINFO  *pzTempTI = nullptr;
	PTABLE     zCancelTable, zMergeTable;
	DTRANSDESC zDTranDescrip[MAXDTRANDESC];
	PAYTRAN		 zPayTran;
	ERRSTRUCT  zErr;
	TRANS      zTrans;
	BOOL       bAllFlag;
	int        i, iDTransDescCount;
	char			 sTranCode[2];


	InitializeErrStruct(&zErr);

	zCancelTable.iSize = 0;
	InitializePTable(&zCancelTable);
	zMergeTable.iSize = 0;
	InitializePTable(&zMergeTable);

	/*
	** If the AnyAsOfs flag is true, first fill the cancel table with all the transactions falling
	  ** in the required date range and then search the process table for the as-of enteries and
	  ** send these enteries to the ProcessAsOf function for further processing
	*/

	if (bAnyAsOfs)
	{
		/*
		** We have to find out the first trade out of the ProcessTable for which AsOfFlag is TRUE,
			** this will give us the earliest effdate. The cancel table will only contain transactions
			** whose effective date is greater than or equal to the effective date of the 'as of' transaction
		*/
		//    lFirstTransNo = 0;
		for (i = 0; i < zProTable.iCount; i++)
		{
			if (zProTable.pzTInfo[i].bAsOfFlag)
			{
				//      if (zProTable.pzTInfo[i].sTranCode[0] == 'R')
					  //{
						// if (lFirstTransNo == 0 || zProTable.pzTInfo[i].zTrans.lRevTransNo < lFirstTransNo)
						 //{
						   //lFirstTransNo = zProTable.pzTInfo[i].zTrans.lRevTransNo;
						   //pzTempTI = &(zProTable.pzTInfo[i]);
						 //}
					  //}
					  //else
					  //{
				pzTempTI = &(zProTable.pzTInfo[i]);
				break;
				//}  
			}
		} // zProTable.iCount 
		if (pzTempTI != nullptr)
		{
			/*
			** In case of an AsOf, the cancel table is filled up with all the transaction belonging to
				** an account with an effective date equal to or greater than the given date, but in case
				** of an Reversal, table is filled up with transactions belonging to the given account with
				** an effective date equal to or greater than the givan date AND having transaction number
				** equal to or greater than the number of the transaction we are trying to reverse. So if it's
				** a reversal, assign the reversal transaction number to the transno field otherwise assign 1
			*/
			if (pzTempTI->zTranType.sTranCode[0] == 'R')
			{
				lEffDate = 0;
				lTransNo = pzTempTI->zTrans.lRevTransNo;
				lDtransNo = pzTempTI->zTrans.lDtransNo;
				lBlockTransNo = pzTempTI->zTrans.lBlockTransNo;
			}
			else
			{
				// SB - Changed on 9/24/99 As per Leno's request - it should be strictly greater than 
				// the effective date of the date and not greater than or equal to
				lEffDate = pzTempTI->zTrans.lEffDate + 1;
				lTransNo = 1;
				lDtransNo = lBlockTransNo = 0;
			}

			zErr = FillOutCancelTable(&zCancelTable, pzTempTI->zTrans.iID, lEffDate, lTransNo, lDtransNo, lBlockTransNo, pzTempTI->zTrans.sSecNo, pzTempTI->zTrans.sWi);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			{
				InitializePTable(&zCancelTable); /* free memory */
				return zErr;
			}
		}
		/*
		** Start processing the ProcessTable from the first AsOf entry and send any eligble enteries
			** to the ProcessAsOfTrade function.
		*/
		for (i = 0; i < zProTable.iCount; i++)
		{
			pzTempTI = &(zProTable.pzTInfo[i]);

			if (pzTempTI->bAsOfFlag)
			{
				zErr = ProcessAsOfTrade(zPmain.sAcctMethod, pzTempTI->zTrans, pzTempTI->zTranType.sTranCode,
					pzTempTI->sRevTranCode, pzTempTI->zTranType.sAutoGen, pzTempTI->sSellAcctType,
					zSType.sLotInd, sCurrPrior, bRebook, &zCancelTable);
				if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				{
					InitializePTable(&zCancelTable); // free memory 
					return zErr;
				}
			} // If AsOf 
		}// for (i < zProTable.iCount) 

		/*
		** Second pass thru the cancel table to mark any trades that are
		** cross linked to another transaction and/or security
		*/
		for (i = 0; i < zCancelTable.iCount; i++)
		{
			pzTempTI = &(zCancelTable.pzTInfo[i]);
			// If the trade is not marked to be cancel or the xtrans_no column is zero, skip the trade 
			if (pzTempTI->bReversable == FALSE)
				continue;
			else if (pzTempTI->zTrans.lXTransNo == 0 && pzTempTI->zTranType.sTranCode[0] != 'X')
				continue;

			if (pzTempTI->zTrans.iID != pzTempTI->zTrans.sXID)
				continue;

			if (pzTempTI->zTranType.sTranCode[0] == 'X' && pzTempTI->zTranType.lSecImpact == 0)
				continue;

			zErr = ProcessXTransNoTrade(zPmain.sAcctMethod, &zCancelTable, i, sCurrPrior, bRebook);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			{
				InitializePTable(&zCancelTable); // free memory 
				return zErr;
			}

		} // for i < canceltable.count 
	} // If bAnyAsOfs 

	  /*
	  ** When user wants to reverse a trade in FrontEnd, it(frontend) first calls TranProcInfo
	  ** function which in turns call tranproc with CurrProc flag = "I". In this situation
	  ** tranproc doesn't actually have to do the reversal but it just creates the list of
	  ** transaction which it will reverse and write that information to a file(file name is
	  ** generated in fronend) and then returns.
	  */
	if (strcmp(sCurrPrior, "I") == 0)
	{
		zErr = WriteReversalInformation(zCancelTable, sFileName, zPmain.sUniqueName);
		return zErr;
	}

	// Cancel any enteries that have been placed on the cancel table 
	if (zCancelTable.iCount > 0)
	{
		zErr = ProcessCancelTable(zCancelTable, lCurrDate, sCurrPrior, bDoTransaction);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
			InitializePTable(&zCancelTable); // free memory 
			return zErr;
		}
	} // if cancelcount > 0 

	// Merge the process table with the cancel table enteries that are eligible for rebooking 
	zErr = CreateMergeTable(zProTable, zCancelTable, &zMergeTable);
	if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
	{
		InitializePTable(&zCancelTable); // free memory 
		InitializePTable(&zMergeTable);
		return zErr;
	}

	// Process the enteries on the merge table 
	for (i = 0; i < zMergeTable.iCount; i++)
	{
		pzTempTI = &(zMergeTable.pzTInfo[i]);
		/*
		** If the entry is a rebooking entry, set the original trans_no to the value
		** stored in the trans_no column and reset the trans_no to zero
		*/
		if (pzTempTI->bRebookFlag)
		{
			pzTempTI->zTrans.lOrigTransNo = pzTempTI->zTrans.lTransNo;
			pzTempTI->zTrans.lTransNo = pzTempTI->zTrans.lXTransNo = 0;

			pzTempTI->zTrans.lEntryDate = lCurrDate;
			pzTempTI->zTrans.lPostDate = lCurrDate;
			pzTempTI->zTrans.lCreateDate = lCurrDate;

			if (strcmp(pzTempTI->zTrans.sSecXtend, "TL") == 0 ||
				strcmp(pzTempTI->zTrans.sSecXtend, "TS") == 0)
				strcpy_s(pzTempTI->zTrans.sSecXtend, "RP"); //SB 5/5/98

		}

		switch (pzTempTI->zTranType.sTranCode[0])
		{
		case 'O': zErr = ProcessOpeninTranproc(&pzTempTI->zTrans, zAssts, zPmain, zTrType, lCurrDate);
			break;

		case 'C': zErr = ProcessCloseinTranproc(&pzTempTI->zTrans, zAssts, zPmain, zTrType, lCurrDate);
			break;

		case 'X': break;

		case 'I': break;

		case 'M': break;

		case 'A': break;

		case 'S': break;

		case 'R': break;

		default: zErr = PrintError("Invalid TranCode", pzTempTI->zTrans.iID, pzTempTI->zTrans.lDtransNo,
			"D", 103, 0, 0, "TPROC LOCKPORT2", FALSE);
		} // switch 

		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
			InitializePTable(&zCancelTable); // free memory 
			InitializePTable(&zMergeTable);
			return zErr;
		}

		/* retrieve payee info associated with the trans if any
		*/
		memset(&zPayTran, 0, sizeof(zPayTran));
		if (pzTempTI->zTranType.sTranCode[0] == 'M' ||
			strcmp(pzTempTI->zTrans.sTranType, "FD") == 0 ||
			strcmp(pzTempTI->zTrans.sTranType, "FR") == 0)
		{
			/*
			** If the entry is a rebooking, use the original trans_no to locate the entry in the
				  ** PAYTRAN table, otherwise use the dtrans_no to locate the entry in the DPAYTRAN table
			*/
			if (pzTempTI->bRebookFlag)
				lTransNo = pzTempTI->zTrans.lOrigTransNo;
			else
				lTransNo = pzTempTI->zTrans.lDtransNo;

			zErr = GetDtransPayee(pzTempTI->zTrans.iID, lTransNo, &zPayTran, pzTempTI->bRebookFlag);

			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			{
				InitializePTable(&zCancelTable); // free memory 
				InitializePTable(&zMergeTable);
				return zErr;
			}
		}

		/*
		** If the miscellaneous description indicator on the transaction row
		** is set to 'Y', retrieve the descriptive information
		*/
		if (pzTempTI->zTrans.sMiscDescInd[0] == 'Y')
		{
			/*
			** If the entry is a rebooking, use the original trans_no to locate the entry in the
				  ** trans_desc table, otherwise use the dtrans_no to locate the entry in the dtrans_desc table
			*/
			if (pzTempTI->bRebookFlag)
				lTransNo = pzTempTI->zTrans.lOrigTransNo;
			else
				lTransNo = pzTempTI->zTrans.lDtransNo;


			zErr = GetDtransDescript(pzTempTI->zTrans.iID, lTransNo, zDTranDescrip, &iDTransDescCount, pzTempTI->bRebookFlag);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			{
				InitializePTable(&zCancelTable); // free memory 
				InitializePTable(&zMergeTable);
				return zErr;
			}

			/*
				  ** If the entry is marked as a versus trade, mark back to acct
			** method, it will be set back to versus by tranalloc
			*/
			if (strcmp(pzTempTI->zTrans.sAcctMthd, "V") == 0)
				strcpy_s(pzTempTI->zTrans.sAcctMthd, zPmain.sAcctMethod);
		} // MiscInd = Y 
		else
			iDTransDescCount = 0;

		/*
		** Retrieve the assets, sectype and trantype rows for the transaction. If the security
			** number and the when issue columns on the assets match the security number and when issue
			** on the trans, there is no need to retrieve a new assets and sectype.  If they do not
		** match, set the bAllflag to true, and the function will retrieve all three rows
		*/
		bAllFlag = FALSE;
		if (strcmp(pzTempTI->zTrans.sSecNo, zAssts.sSecNo) != 0 ||
			strcmp(pzTempTI->zTrans.sWi, zAssts.sWhenIssue) != 0)
			bAllFlag = TRUE;

		zErr = PrepareForTranAlloc(&zTrType, &zAssts, &zSType, pzTempTI->zTrans, bAllFlag);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
			InitializePTable(&zCancelTable); // free memory 
			InitializePTable(&zMergeTable);
			return zErr;
		}

		/*
		** SB : 1/14/1999
		** If rebooking a transaction that was a "Sell" or "Maturity" of a
		** discounted item, reset the pcpl_amt and income_amt, tranalloc will
	**  calculate the numbers
	  */
		if (pzTempTI->bRebookFlag && zSType.sPrimaryType[0] == 'B' && zSType.sPayType[0] == 'D' &&
			(strcmp(pzTempTI->zTrans.sTranType, "SL") == 0 ||
				strcmp(pzTempTI->zTrans.sTranType, "ML") == 0))
		{
			pzTempTI->zTrans.fPcplAmt += pzTempTI->zTrans.fIncomeAmt;
			pzTempTI->zTrans.fIncomeAmt = 0;
		}

		// Send the transaction to TranAlloc for further processing and posting 
		InitializeTransStruct(&zTrans);
		zTrans = pzTempTI->zTrans;
		zErr = TranAlloc(&zTrans, zTrType, zSType, zAssts, zDTranDescrip, iDTransDescCount,
			&zPayTran, sCurrPrior, bDoTransaction);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
		{
			InitializePTable(&zCancelTable); // free memory 
			InitializePTable(&zMergeTable);
			return zErr;
		}

		/*
		** If the transaction is a rebooking entry, update the original and 'RV' transaction with
			** the new transaction number assigned to the trade. If the transaction is a close,
			** transfer or an auto generated trade (receipt of dividend, stock split etc), use the
			** cross reference transaction number.
		*/
		if (pzTempTI->bRebookFlag)
		{
			if (zTrType.sTranCode[0] == 'C' || zTrType.sTranCode[0] == 'X' ||
				zTrType.sTranCode[0] == 'S' || (zTrType.sTranCode[0] == 'A' &&
				(strcmp(zTrans.sTranType, "LD") == 0 || strcmp(zTrans.sTranType, "BA") == 0)))
				pzTempTI->zTrans.lTransNo = zTrans.lXrefTransNo;
			else
				pzTempTI->zTrans.lTransNo = zTrans.lTransNo;

			/*
			** select the reversing trans no off the original transaction so we can
			** update both the orig and reversed trade with the rebook transno
			*/
			lpprSelectRevNoAndCode(&lRevtrans, sTranCode, pzTempTI->zTrans.iID, pzTempTI->zTrans.lOrigTransNo, &zErr);
			if (zErr.iSqlError)
			{
				InitializePTable(&zCancelTable); // free memory 
				InitializePTable(&zMergeTable);
				return(PrintError("Error Reading TRANS", pzTempTI->zTrans.iID, pzTempTI->zTrans.lDtransNo,
					"D", 0, zErr.iSqlError, zErr.iIsamCode, "TPROC LOCKPORT3", FALSE));
			}

			lpprUpdateNewTransNo(pzTempTI->zTrans.lTransNo, pzTempTI->zTrans.iID,
				pzTempTI->zTrans.lOrigTransNo, &zErr);
			if (zErr.iSqlError)
			{
				InitializePTable(&zCancelTable); // free memory 
				InitializePTable(&zMergeTable);
				return(PrintError("Error Updating TRANS", pzTempTI->zTrans.iID, pzTempTI->zTrans.lOrigTransNo,
					"T", 0, zErr.iSqlError, zErr.iIsamCode, "TPROC LOCKPORT4", FALSE));
			}

			lpprUpdateNewTransNo(pzTempTI->zTrans.lTransNo, pzTempTI->zTrans.iID, lRevtrans, &zErr);
			if (zErr.iSqlError)
			{
				InitializePTable(&zCancelTable);  // free memory 
				InitializePTable(&zMergeTable);
				return(PrintError("Error Updating TRANS", pzTempTI->zTrans.iID, lRevtrans, "T", 0,
					zErr.iSqlError, zErr.iIsamCode, "TPROC LOCKPORT5", FALSE));
			}
		} // if mergetable.pzTInfo[i].bRebookFlag 
	} // for i < MergeTabple.iCount  

	InitializePTable(&zCancelTable);
	InitializePTable(&zMergeTable);

	return zErr;
} // LockPortAndProcess 


/**
** The purpose of this function is to identify subsequent transactions that need to be cancelled,
** mark the transaction on the cancel table to be cancelled and if necessary rebooked 
**/
ERRSTRUCT ProcessAsOfTrade(char *sAcctMethod, TRANS zTrans, char *sOrgTranCode, char *sRevTranCode, 
													 char *sAutoGen, char *sSellAcctType, char *sLotInd, char *sCurrPrior, 
													 BOOL bRebook, PTABLE *pzCancelTable)
{
  ERRSTRUCT zErr;
  int       i;
  TRANSINFO *pzTempTI;

  InitializeErrStruct(&zErr);

  for (i = 0; i < pzCancelTable->iCount; i++ )
  {
    pzTempTI = &(pzCancelTable->pzTInfo[i]);

    /* 
		** If the transaction is a cancel and the trans_no  on the cancel table is less than the
    ** rev_trans_no on the cancel trade  - skip. If the effective date of the transaction on the
		** cancel table is less than or equal to the effective date of the 'as of' trade, skip it, 
		** unless the current trade is a split.
    */
    if (sOrgTranCode[0] == 'R' && pzTempTI->zTrans.lTransNo < zTrans.lRevTransNo)
      continue;
    else if (sOrgTranCode[0] != 'R' && pzTempTI->zTrans.lEffDate <= zTrans.lEffDate &&
						 strcmp(pzTempTI->zTranType.sTranCode, "S") != 0)
			continue;

    /* 
    ** Check that the security no, when-issue and security extension match to the transaction 
		** that triggered the 'as-of' process if they do not match, skip the transaction 
    */
    if (strcmp(pzTempTI->zTrans.sSecNo, zTrans.sSecNo) == 0 &&
        strcmp(pzTempTI->zTrans.sWi, zTrans.sWi) == 0)
    {  
      /* 
      ** if the sec_xtends do not match and the sec_xtend on the cancel table is not a tech-long 
			** or tech-short, skip If the secxtend on the trans is 'sh' or 'up'  - skip 
      */
      if (strcmp(pzTempTI->zTrans.sSecXtend, zTrans.sSecXtend) != 0)
      {
          if ((strcmp(zTrans.sSecXtend, "UP") == 0 || strcmp(zTrans.sSecXtend, "SH") == 0) ||
             (strcmp(pzTempTI->zTrans.sSecXtend, "TL") != 0 && strcmp(pzTempTI->zTrans.sSecXtend, "TS") != 0))
            continue;
      } 
      /* 
      ** if the account type on the cancel table does not match to the 'as of' 
      ** account type and does not match to the 'as of' sell account type, skip 
      */
      else if (strcmp(pzTempTI->zTrans.sAcctType, zTrans.sAcctType) != 0 &&
               strcmp(pzTempTI->zTrans.sAcctType, sSellAcctType) != 0)
        continue;
    } // if same security 
    else
      continue;

    /* 
    ** If the 'as of' transaction is a cancel and the entry on the cancel table matches, there 
		** is no further work to be done, mark the transactions to be cancelled and continue 
    */
    if (sOrgTranCode[0] == 'R' &&
       (zTrans.lRevTransNo == pzTempTI->zTrans.lTransNo ||
        zTrans.lRevTransNo == pzTempTI->zTrans.lXrefTransNo))
    { 
      pzTempTI->bReversable = TRUE;
      pzTempTI->bRebookFlag = FALSE;
        
      strcpy_s(pzTempTI->zTrans.sCreatedBy, zTrans.sCreatedBy);
			_strtime(pzTempTI->zTrans.sCreateTime);
      continue;
    } 

    /* 
		** If the 'as of' transaction is a cancel and the lot indicator is set for a single lot 
		** security, and this is not the cancel record there is no subsequent cancel work to be 
		** done, i.e. skip all subsequent trades 
    */
    if (sOrgTranCode[0] == 'R' && sLotInd[0] == 'S') 
       continue;
  
    // If the transaction has already been marked for cancellation, skip it 
    if (pzTempTI->bReversable)
      continue;

    /* 
    ** If the 'as of' transaction is an income transaction, skip it, TranProc 
    ** will not cancel any subsequent income payments as per Lawrence J. Cohn's instructions
    */
    if (pzTempTI->zTranType.sTranCode[0] == 'I')
      continue;

    // do not reverse any subsequent IS & TC 
    if (pzTempTI->zTranType.sTranCode[0] == 'X' && pzTempTI->zTranType.lSecImpact == 0)
      continue;


    // Process Transaction according to transaction code 
    switch(pzTempTI->zTranType.sTranCode[0])
    {
                // Opening Transactions 
      case 'O': zErr = ProcessAsOfOpen(sAcctMethod, zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook);
                break;
                 
                // Closing Transactions 
      case 'C': zErr = ProcessAsOfOthers(zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook);
                break;

                // Transfer Transactions 
      case 'X': zErr = ProcessAsOfOthers(zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook);
                break;
 
                // Adjustment Transactions 
      case 'A': zErr = ProcessAsOfAdjustment(zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook);
                break;

                // Split Transactions 
      case 'S': zErr = ProcessAsOfOthers(zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook);
                break;

                // Money Transactions 
      case 'M': zErr = ProcessAsOfMoney(zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode);
                break;
            
                // Income Transactions 
      case 'I': zErr = ProcessAsOfIncome(zTrans, pzCancelTable, i, sOrgTranCode, sRevTranCode, sAutoGen, sCurrPrior, bRebook);
                break;
   
      case 'R': break; // get out of switch 

			default : return(PrintError("Invalid Transaction Code", zTrans.iID, zTrans.lDtransNo, 
																	"D", 103, 0, 0,  "TPROC PROCESS ASOF3", FALSE));
    }// switch (sTranCode[0]) 
  
    if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;
  }// for i < pzCancelTable.iCount 

  return zErr;
} // ProcessAsOfTrade 


/**
** Function To Process As-Of or Cancellations of Opening Transactions
** Subsequent opening transactions are not effected by 'as of' transactions
** unless the'asof' transaction is a cancel of an opening transaction.
**/
ERRSTRUCT ProcessAsOfOpen(char *sAcctMethod, TRANS zTrans, PTABLE *pzCancelTable, int iCIndex, 
													char *sOrgTranCode, char *sRevTranCode, char *sCurrPrior, BOOL bRebook)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

	// Need to reverse and rebook (certain types of)trade if it is an average cost portfolio
	if (strcmp(sAcctMethod, "A") != 0)
		return zErr;

	/*
	** If the trade is an asof income, money or adjustment other than
	** UC, LD, AM, PO, BA, PZ, SZ no need to reverse and rebook this trade
	*/
	if (sOrgTranCode[0] == 'I' || sOrgTranCode[0] == 'M' ||
		  (sOrgTranCode[0] == 'A' && (strcmp(zTrans.sTranType, "UC") != 0
			 && strcmp(zTrans.sTranType, "LD") != 0 && strcmp(zTrans.sTranType, "AM") != 0
			 && strcmp(zTrans.sTranType, "PO") != 0 && strcmp(zTrans.sTranType, "BA") != 0
			 && strcmp(zTrans.sTranType, "PZ") != 0 && strcmp(zTrans.sTranType, "SZ") != 0)))
		return zErr;

	/*
	** If the trade is a cancel of income, money or adjustment other than
	** UC, LD, AM, PO, BA, PZ, SZ no need to reverse and rebook this trade
	*/
	if (sOrgTranCode[0] == 'R' &&
		  (sRevTranCode[0] == 'M' || sRevTranCode[0] == 'I' ||
		  (sOrgTranCode[0] == 'A' && (strcmp(zTrans.sTranType, "UC") != 0
			 && strcmp(zTrans.sTranType, "LD") != 0 && strcmp(zTrans.sTranType, "AM") != 0
			 && strcmp(zTrans.sTranType, "PO") != 0 && strcmp(zTrans.sTranType, "BA") != 0
			 && strcmp(zTrans.sTranType, "PZ") != 0 && strcmp(zTrans.sTranType, "SZ") != 0))))
		return zErr;


	/*
	** If the trade is not a cancel and the effective date of the trade
	** is equal to the effective date of the open, nothing to do
	*/
	if (sOrgTranCode[0] != 'R' && zTrans.lEffDate == pzCancelTable->pzTInfo[iCIndex].zTrans.lEffDate)
		return zErr;

	// Mark the open trade to be cancelled and rebooked
	strcpy_s(pzCancelTable->pzTInfo[iCIndex].zTrans.sTransSrce, "S");
	pzCancelTable->pzTInfo[iCIndex].bReversable = TRUE;
	
	// SB 7/24/00 While doing reversal, now user will have an option not to rebook the trades
	// (to avoid multiple subsequent reverasal and rebooking if user is doing multiple reversals).
	if (strcmp(sOrgTranCode, "R") == 0 && strcmp(sCurrPrior, "I") != 0 && !bRebook)
		pzCancelTable->pzTInfo[iCIndex].bRebookFlag = FALSE;
	else
		pzCancelTable->pzTInfo[iCIndex].bRebookFlag = TRUE;

  return zErr;
} /* ProcessAsOfOpen */


/**
** For FUTURE use
** Function To Process AsOf Money transactions.  This function can only be effected by a cancel 
** of a money transactions, no other as of transactions can effect money
**/
ERRSTRUCT ProcessAsOfMoney(TRANS zTrans, PTABLE *pzCancelTable, int iCIndex, 
                           char *sOrgTranCode, char *sRevTranCode)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  return zErr;
} /* ProcessAsOfMoney */

/**
** For FUTURE use
** Function To Process AsOf Income transactions
**/
ERRSTRUCT ProcessAsOfIncome(TRANS zTrans, PTABLE *pzCancelTable, int iCIndex, char *sOrgTranCode, 
														char *sRevTranCode, char *sAutoGen, char *sCurrPrior, BOOL bRebook)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  /* 
  ** If the original transaction is a income or money transaction, there 
  ** is no work to be done, skip the transaction 
  */
  if (sOrgTranCode[0] == 'I' || sOrgTranCode[0] == 'M')
    return zErr;
 
  // If a Cancel Trade started this processing 
  if (sOrgTranCode[0] == 'R')
  {
    // If cancel of a money transaction, skip the trade 
    if (sRevTranCode[0] == 'M')
      return zErr;
        
    /* 
    ** If cancel of an opening or a split or an adjustement, and the the tax
    ** lot ids do not match - skip, else add the transaction to the cancel table
    */
    if (sRevTranCode[0] == 'O' || sRevTranCode[0] == 'S' || sRevTranCode[0] == 'A') 
    {
      pzCancelTable->pzTInfo[iCIndex].bReversable = TRUE;

      /* 
      ** If the transaction is not an opening trade (split or adjustment), the income 
			** transaction must be rebooked as the effect of cancelling a split or adjustment changes
			** the lot but unlike a cancel of an open, does not remove it.
			** SB 7/24/00 - If user does not want to rebook the trade, don't rebook it.
      */
      if (strcmp(sRevTranCode, "O") == 0 && (bRebook || strcmp(sCurrPrior, "I") == 0))
        pzCancelTable->pzTInfo[iCIndex].bRebookFlag = TRUE;

    } // if open, split or adjustment 

    /* 
    ** If cancel of a closing transaction,  cancel and rebook all subsequent income transactions
		** SB 7/24/00 - If user does not want to rebook the trade, don't rebook it.
    */
    if (sRevTranCode[0] == 'C' || sRevTranCode[0] == 'X')
    {
      pzCancelTable->pzTInfo[iCIndex].bReversable = TRUE;
      if (bRebook || strcmp(sCurrPrior, "I") == 0)
				pzCancelTable->pzTInfo[iCIndex].bRebookFlag = TRUE;
			else
				pzCancelTable->pzTInfo[iCIndex].bRebookFlag = FALSE;
    } // if cancel of Closing or transfer 
  } // end of cancel 
  // If the transaction is an as-of adjustment or a split and the tax lot id match, process the trade 
  else if (sOrgTranCode[0] == 'A' || sOrgTranCode[0] == 'S') 
  {
    pzCancelTable->pzTInfo[iCIndex].bReversable = TRUE;
		pzCancelTable->pzTInfo[iCIndex].bRebookFlag = TRUE;
  } // Adjustment or Split 
  // If the transaction is an as-of close, transfer or an open, all subsequent income enteries must be cancelled.
  else
  {
    pzCancelTable->pzTInfo[iCIndex].bReversable = TRUE;
		pzCancelTable->pzTInfo[iCIndex].bRebookFlag = TRUE;
  } /* Others - Close, Open or Transfres, rest have already been taken care of*/

  return zErr;
} // ProcessAsOfIncome 


/**
** Function to process AsOf Adjustment transactions
**/
ERRSTRUCT ProcessAsOfAdjustment(TRANS zTrans, PTABLE *pzCancelTable, int iCIndex, char *sOrgTranCode, 
																char *sRevTranCode, char *sCurrPrior, BOOL bRebook)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  // If the transaction is an as-of money or as-of income, skip 
  if (sOrgTranCode[0] == 'I' || sOrgTranCode[0] == 'M')
    return zErr;

  // If the transaction is a cancel of income or money, skip 
  if (sOrgTranCode[0] == 'R' && (sRevTranCode[0] == 'M' || sRevTranCode[0] == 'I')) 
    return zErr;

  /* 
  ** If the current transaction is an SE, FY, AC, AG, SK, SG, SR, UR, UG, UK, UB
	** and the trigger transaction is not a cancel of this trade, skip 
  */
  if (strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "SE") == 0 ||
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "FY") == 0 ||
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "AC") == 0 ||
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "AG") == 0 ||
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "SK") == 0 ||
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "SG") == 0 ||    
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "SR") == 0 ||    
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "UR") == 0 ||    
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "UG") == 0 ||    
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "UK") == 0 ||   
      strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "UB") == 0)   
     return zErr;

  // If the transaction is a cancel 
  if (sOrgTranCode[0] == 'R') 
  {
		// of an opening trans and current trans is UD, but on different lot - skip       
		if ((sRevTranCode[0] == 'O') && 
			  strcmp(pzCancelTable->pzTInfo[iCIndex].zTrans.sTranType, "UD") == 0 &&   
				(zTrans.lRevTransNo != pzCancelTable->pzTInfo[iCIndex].zTrans.lTaxlotNo))
        return zErr;
		else
		// of an adjustment and the the transactions is a 
	  // 'SE', 'FY", 'AG' or 'AC', 'SK', 'SG', 'SR', 'UR', 'UG', 'UK', 'UD', 'UB' skip 
    if (sRevTranCode[0] == 'A') 
    {
      if (strcmp(zTrans.sRevType, "SE") == 0 || strcmp(zTrans.sRevType, "FY") == 0 || 
          strcmp(zTrans.sRevType, "AC") == 0 || strcmp(zTrans.sRevType, "AG") == 0 ||
          strcmp(zTrans.sRevType, "SK") == 0 || strcmp(zTrans.sRevType, "SG") == 0 || 
          strcmp(zTrans.sRevType, "SR") == 0 || strcmp(zTrans.sRevType, "UR") == 0 || 
          strcmp(zTrans.sRevType, "UG") == 0 || strcmp(zTrans.sRevType, "UK") == 0 ||    
					strcmp(zTrans.sRevType, "UD") == 0 || 
					strcmp(zTrans.sRevType, "UB") == 0) 
        return zErr;
      else
        return(ProcessAsOfOthers(zTrans, pzCancelTable, iCIndex, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook));
    } // RevTranCode = "A" 
    else
      return(ProcessAsOfOthers(zTrans, pzCancelTable, iCIndex, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook));
  } // OrgTranCode = "R" 
  else
    zErr = ProcessAsOfOthers(zTrans, pzCancelTable, iCIndex, sOrgTranCode, sRevTranCode, sCurrPrior, bRebook);
  
  return zErr; 
} // ProcessAsOfAdjustments 


/**
** Process AsOf Others(Than Open, Money, Income and Adjustments) 
**/
ERRSTRUCT ProcessAsOfOthers(TRANS zTrans, PTABLE *pzCancelTable, int iCIndex, char *sOrgTranCode, 
														char *sRevTranCode, char *sCurrPrior, BOOL bRebook)
{
  ERRSTRUCT zErr;
  TRANSINFO *pzTI;

  InitializeErrStruct(&zErr);

  pzTI = &(pzCancelTable->pzTInfo[iCIndex]);

  // If the transaction is an as-of money or as-of income, skip 
  if (sOrgTranCode[0] == 'I' || sOrgTranCode[0] == 'M')
    return zErr;

  if (sOrgTranCode[0] == 'R')
  {
    // If the transaction is a cancel of income or money, skip 
    if (sRevTranCode[0] == 'M' || sRevTranCode[0] == 'I') 
      return zErr;

    // If the transaction is a cancel of an adjustment and the the transactions is a 
		// 'SE', 'FY', 'AG', 'AC', 'SK', 'SG', 'SR', 'UR', 'UG', 'UK', 'UD', 'UB', skip 
    if (sRevTranCode[0] == 'A' &&
        (strcmp(zTrans.sRevType, "SE") == 0 || strcmp(zTrans.sRevType, "FY") == 0 ||
         strcmp(zTrans.sRevType, "AC") == 0 || strcmp(zTrans.sRevType, "AG") == 0 || 
         strcmp(zTrans.sRevType, "SK") == 0 || strcmp(zTrans.sRevType, "SG") == 0 || 
         strcmp(zTrans.sRevType, "SR") == 0 || strcmp(zTrans.sRevType, "UR") == 0 || 
				 strcmp(zTrans.sRevType, "UG") == 0 || strcmp(zTrans.sRevType, "UK") == 0 || 
				 strcmp(zTrans.sRevType, "UB") == 0))
      return zErr;
    
		// If cancel of an open, UC, BA, AM, PO, RP, PZ, SZ then pick up transactions against that lot
    if (sRevTranCode[0] == 'O' || (sRevTranCode[0] == 'A' &&
        (strcmp(zTrans.sRevType, "UC") == 0 || strcmp(zTrans.sRevType, "BA") == 0 ||
         strcmp(zTrans.sRevType, "AM") == 0 || strcmp(zTrans.sRevType, "PO") == 0 ||
				 strcmp(zTrans.sRevType, "RP") == 0)))
    {
      strcpy_s(pzTI->zTrans.sTransSrce, "S");
      pzTI->bReversable = TRUE;
			
			// SB 8/23/99 - Don't rebook the splits if open transaction is being cancelled
			// SB 7/24/00 - If user does not want to rebook trade, don't rebook
			// vay 5/10/04 - Don't rebook UD for the taxlot where open transaction is being cancelled
			if (strcmp(pzTI->zTranType.sTranCode, "S") == 0 || 
				 (strcmp(sCurrPrior, "I") != 0 && !bRebook) ||
				 (strcmp(pzTI->zTrans.sTranType,"UD") == 0 && (pzTI->zTrans.lTaxlotNo == zTrans.lRevTransNo))) 
				pzTI->bRebookFlag = FALSE;
			else
				pzTI->bRebookFlag = TRUE;
    } // If Reversal of an Open, UC, AM or PO 
  
    /*
    ** If cancel of a close and this is the trade, aggregate the trade and add to table, if this 
		** is a subsequent trade, check the tax lot against the lot table and if a match, aggregate and add 
    */
    if (sRevTranCode[0] == 'C' || sRevTranCode[0] == 'S' ||
        (sRevTranCode[0] == 'X' && strcmp(zTrans.sRevType, "TC") != 0) || 
        (sRevTranCode[0] == 'A' && (strcmp(zTrans.sRevType, "LD") == 0 || strcmp(zTrans.sRevType, "BA") == 0)))
    {
      if (zTrans.lRevTransNo != pzTI->zTrans.lTransNo && 
          zTrans.lRevTransNo != pzTI->zTrans.lXrefTransNo)
      {
				pzTI->bReversable = TRUE;
				// SB 9/24/99 Splits should never get rebooked
				// SB 7/24/00 - If user does not want to rebook trade, don't rebook
				if (strcmp(pzTI->zTranType.sTranCode, "S") == 0 || (strcmp(sCurrPrior, "I") != 0 && !bRebook)) 
					pzTI->bRebookFlag = FALSE;
				else
          pzTI->bRebookFlag = TRUE;
      } 
    } // Cancel of a close, Xfer, Split, "LD" or "BA" 

    return zErr;
  } // Cancel Trades 

  // If the transaction is an as-of adjustment and the transaction is 'SE', 'FY', 'AC', 
	// 'SK', 'AG', 'SG', 'UG', 'UK', 'SR', 'UR', 'UD', 'UB', skip it
  if (sOrgTranCode[0] == 'A' &&
      (strcmp(zTrans.sTranType, "SE") == 0 || strcmp(zTrans.sTranType, "FY") == 0 || 
       strcmp(zTrans.sTranType, "AC") == 0 || strcmp(zTrans.sTranType, "AG") == 0 ||
       strcmp(zTrans.sTranType, "SK") == 0 || strcmp(zTrans.sTranType, "SG") == 0 || 
       strcmp(zTrans.sTranType, "UG") == 0 || strcmp(zTrans.sTranType, "UK") == 0 ||    
       strcmp(zTrans.sTranType, "SR") == 0 || strcmp(zTrans.sTranType, "UR") == 0 ||    
			 strcmp(zTrans.sTranType, "UD") == 0 || strcmp(zTrans.sTranType, "UB") == 0))
     return zErr;

  /*
  ** If the trade is an as-of close,transfer, or open, all subsequent closes
  ** must be cancelled, no skipping
  */
  if (sOrgTranCode[0] == 'C' || sOrgTranCode[0] == 'O' || 
		  (sOrgTranCode[0] == 'X' && strcmp(zTrans.sTranType, "TC") != 0))
  {
    strcpy_s(pzTI->zTrans.sTransSrce, "S");
    pzTI->bReversable = TRUE;
		// SB 9/24/99 Splits should never get rebooked
		if (strcmp(pzTI->zTranType.sTranCode, "S") == 0) 
			pzTI->bRebookFlag = FALSE;
		else
			pzTI->bRebookFlag = TRUE;

    return zErr;
  } // if origtrancode is "C", "O" or "X" 

  // If an as of split, 'UC', 'AM', 'BA', 'LD', 'PO', 'RP', 'PZ', 'SZ', pick up transactions against that lot
  if (sOrgTranCode[0] == 'S' || (sOrgTranCode[0] == 'A' && 
      (strcmp(zTrans.sTranType, "UC") == 0 || strcmp(zTrans.sTranType, "BA") == 0 ||
       strcmp(zTrans.sTranType, "AM") == 0 || strcmp(zTrans.sTranType, "PO") == 0 ||
       strcmp(zTrans.sTranType, "LD") == 0 || strcmp(zTrans.sTranType, "RP") == 0 ||
       strcmp(zTrans.sTranType, "PZ") == 0 || strcmp(zTrans.sTranType, "SZ") == 0)))
  {
    strcpy_s(pzTI->zTrans.sTransSrce, "S");
    pzTI->bReversable = TRUE;
	
		// SB 9/24/99 Splits should never get rebooked
		if (strcmp(pzTI->zTranType.sTranCode, "S") == 0) 
			pzTI->bRebookFlag = FALSE;
		else
			pzTI->bRebookFlag = TRUE;
 
    return zErr;
  } // If AsOf Split, UC, BA, AM, PO, LD, RP, PZ, SZ
 
  return zErr;
} // ProcessAsOfOthers 


ERRSTRUCT ProcessXTransNoTrade(char *sAcctMethod, PTABLE *pzCancelTable, int iCIndex, 
															 char *sCurrPrior, BOOL bRebook)
{
  int       i;
  char      sTranCode[3], sSellAcctType[2], sLotInd[2], sAcctType[2];
  BOOL      bMatchFound;
  TRANS     zTrans;
  ERRSTRUCT zErr;
  TRANSINFO *pzTI;

	InitializeErrStruct(&zErr);
  i = 0;
  bMatchFound = FALSE;
 
  strcpy_s(sLotInd, " ");

  pzTI = &pzCancelTable->pzTInfo[iCIndex];
  /* 
  ** Loop thru the cancel table looking for the transaction number that matches 
  ** to the xtrans number, if found set the bool variable match found to true 
  ** If the transaction is a transfer, there is no search work to be done
  */
  if (pzTI->zTranType.sTranCode[0] != 'X')
  {
    i = 0;
    while (bMatchFound == FALSE && i < pzCancelTable->iCount)
    {
      if (pzCancelTable->pzTInfo[i].zTrans.lTransNo == pzTI->zTrans.lXTransNo)
      {
        bMatchFound = TRUE;
        strcpy_s(pzCancelTable->pzTInfo[i].zTrans.sTransSrce, pzTI->zTrans.sTransSrce);
      }

      i++;
    }   
  }
  else
  {
    i = iCIndex + 1;
    bMatchFound = TRUE; 
  }  
 
  // If no match is found, return with an error 
  if (bMatchFound == FALSE)
    return(PrintError("Invalid Cross Transaction Match", 0, 0, "", 994, 0, 0, "TPROC XTRANSNO", FALSE));
  else
    pzTI = &(pzCancelTable->pzTInfo[i - 1]);
 
  // If the transaction is already marked as a cancel, there is no further work to be done, return 
  if (pzTI->zTranType.sTranCode[0] != 'X')
  {
    if (pzTI->bReversable)
      return zErr;

    // Mark the trade for cancellation 
    pzTI->bReversable = TRUE;
 
    /* 
    ** If the transaction is not an exercise transaction, mark it to be rebooked, exercise 
		** transaction are not rebooked due to the fact that they are automatically generated 
		** by the 'tranalloc' function
    */
    if (strcmp(pzTI->zTrans.sTranType, "EX") != 0 &&
        strcmp(pzTI->zTrans.sTranType, "ES") != 0)
      pzTI->bRebookFlag = TRUE;
  }
 
  // Make the transaction a cancel transaction 
	InitializeTransStruct(&zTrans);
  zTrans = pzTI->zTrans;
  strcpy_s(sSellAcctType, pzTI->sSellAcctType);
  strcpy_s(zTrans.sRevType, zTrans.sTranType);
  strcpy_s(zTrans.sTranType, "RV");
  strcpy_s(sTranCode, pzTI->zTranType.sTranCode);

  // If the transaction is a transfer, the cancel transaction must look like a cancel of an open
  if (sTranCode[0] == 'X')
  {
    // swap the security information 
    strcpy_s(zTrans.sSecNo, zTrans.sXSecNo);
    strcpy_s(zTrans.sWi, zTrans.sXWi);
    strcpy_s(zTrans.sSecXtend, zTrans.sXSecXtend);
    strcpy_s(zTrans.sAcctType, zTrans.sXAcctType);
    strcpy_s(sAcctType, zTrans.sAcctType);
    zTrans.iSecID = zTrans.iXSecID;

    // set the transaction code 
    strcpy_s(sTranCode, "O");
    strcpy_s(zTrans.sRevType, "FR");
    zTrans.lTaxlotNo = zTrans.lTransNo;
 
    // get the sell account type that matches to the new account type 
		lpprSelectSellAcctType(sSellAcctType, sAcctType, &zErr);
    if (zErr.iSqlError)
      return(PrintError("Error Selecting Sell Account Type", pzTI->zTrans.iID, pzTI->zTrans.lTransNo, 
												"T", 0, zErr.iSqlError, zErr.iIsamCode,"TPROC XTRANS1",FALSE));
  } // If transfer 
 
  /* 
  ** If the transaction is a close, transfer or auto generated trade 
  ** set the reversal transaction to the xref trans no, otherwise
  ** use the trans no to set the reversal transaction nubmer 
  */
  if (sTranCode[0] == 'C' || sTranCode[0] == 'X' || pzCancelTable->pzTInfo[i].zTranType.sAutoGen[0] == 'Y')
    zTrans.lRevTransNo = zTrans.lXrefTransNo;
  else 
    zTrans.lRevTransNo = zTrans.lTransNo;
 
  // Send the transaction to the process asof function for further processing 
  zErr = ProcessAsOfTrade(sAcctMethod, zTrans, "R", sTranCode, pzTI->zTranType.sAutoGen, 
													sSellAcctType, sLotInd, sCurrPrior, bRebook, pzCancelTable);

  return zErr;
} // ProcessXtransNoTrades 


/**
**/
ERRSTRUCT AggregateTrades(PTABLE zPTable, int iPStartIndex, int *piPEndIndex, TRANS *pzTrans)
{
  ERRSTRUCT zErr;
  int       i;
  long      lXrefTransNo;
  TRANSINFO *pzTI;

  InitializeErrStruct(&zErr);

  if (iPStartIndex < 0 || iPStartIndex > zPTable.iCount)
    return(PrintError("Programming Error", 0, 0, "", 999, 0, 0, "TPROC AGGREGATETRADES1", FALSE));

  *pzTrans = zPTable.pzTInfo[iPStartIndex].zTrans;
  *piPEndIndex = iPStartIndex;

  // Transaction's zero XrefTransNo means this is a complete trade 
  if (pzTrans->lXrefTransNo == 0)
    return zErr;

  // TransNo != XrefTransNo, it is not a master transaction 
  if (pzTrans->lTransNo != pzTrans->lXrefTransNo)
     return(PrintError("Not A Master Transaction", pzTrans->iID, pzTrans->lDtransNo, "D", 
											 50, 0, 0, "TPROC AGGREGATETRADES2", FALSE));

  /* 
  ** Right now output transaction is the master transaction. Go through passed table and keep 
	** adding transactions(to the master) for which XrefTransNo is same as TransNo(or XrefTransNo) 
	** of master transaction. Since all the lots are in sequence, as soon as a different XrefTransNo 
	** is found, break out of loop, aggregation is done.
  */ 
  lXrefTransNo = zPTable.pzTInfo[iPStartIndex].zTrans.lTransNo;

  i = iPStartIndex + 1;

  pzTI = &(zPTable.pzTInfo[i]);
  while (i < zPTable.iCount && pzTI->zTrans.lXrefTransNo == lXrefTransNo)
  {
     pzTrans->fUnits += pzTI->zTrans.fUnits;
     pzTrans->fOrigFace += pzTI->zTrans.fOrigFace;
     pzTrans->fTotCost += pzTI->zTrans.fTotCost;
     pzTrans->fOrigCost += pzTI->zTrans.fOrigCost;
     pzTrans->fPcplAmt += pzTI->zTrans.fPcplAmt;
     pzTrans->fOptPrem += pzTI->zTrans.fOptPrem;
     pzTrans->fAmortVal += pzTI->zTrans.fAmortVal;
     pzTrans->fBasisAdj += pzTI->zTrans.fBasisAdj;
     pzTrans->fCommGcr += pzTI->zTrans.fCommGcr;
     pzTrans->fNetComm += pzTI->zTrans.fNetComm;
     pzTrans->fSecFees += pzTI->zTrans.fSecFees;
     pzTrans->fMiscFee1 += pzTI->zTrans.fMiscFee1;
     pzTrans->fMiscFee2 += pzTI->zTrans.fMiscFee2;
     pzTrans->fAccrInt += pzTI->zTrans.fAccrInt;
     pzTrans->fIncomeAmt += pzTI->zTrans.fIncomeAmt;
     pzTrans->fNetFlow += pzTI->zTrans.fNetFlow;     

     i++;
     pzTI = &(zPTable.pzTInfo[i]);
  } //i < Ptable.Count && pzTI->zTrans.lXrefTransNo=lXrefTransNo

  *piPEndIndex = i - 1;

  return zErr;
} // Aggregate trades 

 
/**
** PROCESS THE CANCEL TABLE
*/
ERRSTRUCT ProcessCancelTable(PTABLE zCanTable, long lCurrentDate, char *sCurrPrior, BOOL bDoTransaction)
{
  TRANS				zTrans;
  ASSETS			zAssets;
  SECTYPE			zSecType;
  DTRANSDESC	zDescTab[MAXDTRANDESC];
  TRANTYPE		zTranType;
  ERRSTRUCT		zErr;
  BOOL				bAllFlag;
  int					i, iID, iNumDTItems;
  long				lOrigTransNo, lXrefTransNo;

  InitializeErrStruct(&zErr);

  lXrefTransNo = 0;
  iNumDTItems = 0;

  // cancel all the trade in reverse order (of the transno which in effect is effdate + tradesort)
  for (i = zCanTable.iCount - 1; i >= 0 ; i--)
  {
    // If the transaction is not marked to be cancelled, skip it 
    if (zCanTable.pzTInfo[i].bReversable == FALSE)
			continue;

		/*
    ** If a trade is broken into more than one lot, send only master transactions to TranAlloc, 
		** which in turn will find other lots and process the whole trade
    */           
    if (zCanTable.pzTInfo[i].zTrans.lXrefTransNo != 0 &&
        (zCanTable.pzTInfo[i].zTrans.lTransNo != zCanTable.pzTInfo[i].zTrans.lXrefTransNo))
      continue;

		InitializeTransStruct(&zTrans);
    zTrans = zCanTable.pzTInfo[i].zTrans;
  
    zCanTable.pzTInfo[i].zTrans.lOrigTransNo = zTrans.lTransNo;

    strcpy_s(zTrans.sRevType, zTrans.sTranType);
    strcpy_s(zTrans.sTranType, "RV");
    zTrans.lRevTransNo = zTrans.lTransNo;
    zTrans.lOrigTransNo = zTrans.lTransNo;
    zTrans.lPostDate = lCurrentDate;
    zTrans.lCreateDate = lCurrentDate;
    zTrans.lEntryDate = lCurrentDate;
		if (zCanTable.pzTInfo[i].lDtransNo != 0)
			zTrans.lDtransNo = zCanTable.pzTInfo[i].lDtransNo;
		if (zCanTable.pzTInfo[i].lBlockTransNo != 0)
			zTrans.lBlockTransNo = zCanTable.pzTInfo[i].lBlockTransNo;

    lOrigTransNo = zTrans.lTransNo;
    iID = zTrans.iID;

    /* 
    ** Retrieve the assets, sectype and trantype rows for the transaction 
    ** If the security number and the when issue columns on the assets
    ** match the security number and when issue on the trans, there is 
    ** no need to retrieve a new assets and sectype.  If they do not match
    ** set the bAllflag to true, and the function will retrieve all three rows 
    */
    bAllFlag = FALSE;
    if (strcmp(zTrans.sSecNo, zAssets.sSecNo) != 0 || strcmp(zTrans.sWi, zAssets.sWhenIssue) != 0)
      bAllFlag = TRUE;

    zErr = PrepareForTranAlloc(&zTranType, &zAssets, &zSecType,zTrans,bAllFlag);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) 
      return zErr;

    zErr = TranAlloc(&zTrans, zTranType, zSecType, zAssets, zDescTab, iNumDTItems, 
			               NULL, sCurrPrior, bDoTransaction);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0) 
      return zErr;

    // Update the original transaction with the reversal transaction number 
		lpprUpdateRevTransNo(zTrans.lTransNo, iID, lOrigTransNo, &zErr);
    if (zErr.iSqlError)
      return(PrintError("Error Updating TRANS", zTrans.iID, lOrigTransNo, "T", 0, 
												zErr.iSqlError, zErr.iIsamCode, "TPROC PCANCEL TABLE", FALSE));   
  }// for (i = iCancelCount-1; i >=0; i--) 

  return zErr;
}// ProcessCancelTable function 
  

/**
** This function merges process table and cancel table together 
*/
ERRSTRUCT CreateMergeTable(PTABLE zProcessTab, PTABLE zCancelTab, PTABLE *pzMergeTab)
{
  int       i, j, iEndIndex;
  ERRSTRUCT zErr;
  TRANSINFO zTInfo;

  InitializeErrStruct(&zErr);
 
  i = j = iEndIndex = 0;
  while (i < zProcessTab.iCount  || j < zCancelTab.iCount)
  {  
    // If the entry in the process table is a cancel, skip 
    if (i < zProcessTab.iCount && zProcessTab.pzTInfo[i].zTranType.sTranCode[0] == 'R')
    {
      i++;
      continue;
    }

    // If the entry in the cancel table is not to be rebooked, skip 
    if (j < zCancelTab.iCount && zCancelTab.pzTInfo[j].bRebookFlag == FALSE)
    {
      j++;
      continue;
    }

    /*
    ** All the entries from Canceltable have been used OR the current entry in
    ** process table has earlier effecteive date than current entry on Cancel
    ** table, so put the current entry from ProcessTable on to Merge Table
    */
    if (j >= zCancelTab.iCount ||
        (i < zProcessTab.iCount && zProcessTab.pzTInfo[i].zTrans.lEffDate < 
         zCancelTab.pzTInfo[j].zTrans.lEffDate))
    {
      zErr = AddTransToPTable(pzMergeTab, zProcessTab.pzTInfo[i]);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      i++;
    } // processtable.effdate < canceltable.effdate 
    /*
    ** All the entries from Canceltable have been used OR the current entry in
    ** process table has earlier effecteive date than current entry on Cancel
    ** table, so put the current entry from ProcessTable on to Merge Table.
    ** If a trade is broken in multiple lot, Cancel Table has all the lots(in
    ** sequence, starting with master transaction), but TranAlloc expects an
    ** aggregated trade so befor putting the trade on MergeTable, aggregate it
    */
    else if (i >= zProcessTab.iCount ||
             (j < zCancelTab.iCount && zProcessTab.pzTInfo[i].zTrans.lEffDate > 
              zCancelTab.pzTInfo[j].zTrans.lEffDate))
    {
      InitializeTransInfo(&zTInfo);
      zTInfo = zCancelTab.pzTInfo[j];

     // SB : 1/14/1999 if (zCancelTab.pzTInfo[j].zTrans.lXrefTransNo == 0)
     if (zCancelTab.pzTInfo[j].zTrans.lXrefTransNo == 0 ||
         zCancelTab.pzTInfo[j].zTranType.sTranCode[0] == 'S' ||
         strcmp (zCancelTab.pzTInfo[j].zTrans.sTranType, "LD") == 0 ||
				 strcmp (zCancelTab.pzTInfo[j].zTrans.sTranType, "BA") == 0)
        iEndIndex = j;
      else
      {
        zErr = AggregateTrades(zCancelTab, j, &iEndIndex, &zTInfo.zTrans);
        if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
          return zErr;
      }

      zCancelTab.pzTInfo[j] = zTInfo;

      zErr = AddTransToPTable(pzMergeTab, zCancelTab.pzTInfo[j]);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

      j = iEndIndex + 1;          
    }// canceltable.effdate < processtable.effdate 
    else  
    {
      if (j < zCancelTab.iCount && 
		  zCancelTab.pzTInfo[j].zTranType.iTradeSort <= zProcessTab.pzTInfo[i].zTranType.iTradeSort)
      {
        InitializeTransInfo(&zTInfo);
        zTInfo = zCancelTab.pzTInfo[j];

        if (zCancelTab.pzTInfo[j].zTrans.lXrefTransNo == 0 || 
			zCancelTab.pzTInfo[j].zTranType.sTranCode[0] == 'S' ||
            strcmp(zCancelTab.pzTInfo[j].zTrans.sTranType, "LD") == 0 ||
						strcmp(zCancelTab.pzTInfo[j].zTrans.sTranType, "BA") == 0)
        {
          zCancelTab.pzTInfo[j].zTrans.lXrefTransNo = 0;
          zTInfo.zTrans.lXrefTransNo = 0;
          iEndIndex = j;
        }
        else
        {
          zErr = AggregateTrades(zCancelTab, j, &iEndIndex,&zTInfo.zTrans);
          if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
            return zErr;
        }

        zCancelTab.pzTInfo[j] = zTInfo;

        zErr = AddTransToPTable(pzMergeTab, zCancelTab.pzTInfo[j]);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

        j = iEndIndex + 1;          
      }// cancetable.tradesort <= processtable.radesort 
      else
      {
        zErr = AddTransToPTable(pzMergeTab, zProcessTab.pzTInfo[i]);
        if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
          return zErr;

        i++;
      }// cancetable.tradesort > processtable.radesort 
    }//end of equal effdate 
  } // while  

  return zErr;
}// createmergetable 


/**
** Opening Trade Processing - TranCode = 'O'
**/
ERRSTRUCT ProcessOpeninTranproc(TRANS *pzHTrans, ASSETS zAst, PORTMAIN zPmain, TRANTYPE zTtype, long lCurrDate)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);
   
  if (zTtype.lCashImpact == 0)// no cash impact 
  {
    if (pzHTrans->lStlDate == 0)
      pzHTrans->lStlDate = pzHTrans->lEffDate; //lCurrDate;
  }// if no cash impact 

  return zErr;
}// end of processopen function 


/**
** Closing Trade Processing - TranCode = 'C'
**/
ERRSTRUCT ProcessCloseinTranproc(TRANS *pzHTrans, ASSETS zAst, PORTMAIN zPmain, 
                                 TRANTYPE zTtype, long lCurrDate)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  if (zTtype.lCashImpact == 0) // no cash impact 
  {
    if (pzHTrans->lStlDate == 0)
      pzHTrans->lStlDate = pzHTrans->lEffDate; //lCurrDate;
  }// if no cash impact 

  return zErr;
}/* processclose */

/**
** This function makes sure that the transaction it's called with is a valid transaction.
**/
ERRSTRUCT VerifyTransaction(TRANS zTrns, TRANTYPE *pzTranType, 
														char *psRevTranCode, char *psSellAcctType)
{
  ERRSTRUCT zErr;
  long  lRevTransNo;

	InitializeErrStruct(&zErr);
  
  // Make sure dr_cr is valid 
  if (strcmp(zTrns.sDrCr, "DR") != 0 && strcmp (zTrns.sDrCr, "CR") != 0)
    return(PrintError("Invalid DrCr on Trans", zTrns.iID, zTrns.lDtransNo, 
										  "D", 88, 0, 0, "TPROC VERIFYTRANS1", FALSE));

  // Make sure CurrId, Inc. CurrId, Sec. CurrId and Accr CurrId are all valid 
  if (zTrns.sCurrId[0] == '\0' || zTrns.sCurrId[0] == ' ')
    return(PrintError("Invalid Currency Id on Trans", zTrns.iID,  zTrns.lDtransNo, 
											"D", 14, 0, 0, "TPROC VERIFYTRANS2", FALSE));

  if (zTrns.sIncCurrId[0] == '\0' || zTrns.sIncCurrId[0] == ' ')
    return(PrintError("Invalid Income Currency Id on Trans", zTrns.iID,
                      zTrns.lDtransNo, "D", 117, 0, 0, "TPROC VERIFYTRANS3", FALSE));

  if (zTrns.sSecCurrId[0] == '\0' || zTrns.sSecCurrId[0] == ' ')
    return(PrintError("Invalid Security Currency Id on Trans", zTrns.iID,
                      zTrns.lDtransNo, "D", 119, 0, 0, "TPROC VERIFYTRANS4", FALSE));

  if (zTrns.sAccrCurrId[0] == '\0' || zTrns.sAccrCurrId[0] == ' ')
    return(PrintError("Invalid Accrual Currency Id on Trans", zTrns.iID,
                      zTrns.lDtransNo, "D", 120, 0, 0, "TPROC VERIFYTRANS5", FALSE));

  // make sure SysXrate and InccomeSysXrate are valid 
	if (zTrns.fSysXrate == 0)
     return(PrintError("Invalid SysXrate", zTrns.iID, zTrns.lDtransNo, 
                       "D", 68, 0, 0, "TPROC VERIFYTRANS6", FALSE));

    if (zTrns.fIncSysXrate == 0)
	  return(PrintError("Invalid Income SysXrate", zTrns.iID, zTrns.lDtransNo, 
											"D", 125, 0, 0, "TPROC VERIFYTRANS7", FALSE));


  // make sure a valid trantype record exist 
  lpprSelectTrantype(pzTranType, zTrns.sTranType, zTrns.sDrCr, &zErr);
  if (zErr.iSqlError != 0 && zErr.iSqlError != SQLNOTFOUND)
    return(PrintError("Error Reading TRANTYPE", zTrns.iID, zTrns.lDtransNo, "D",  0, 
											zErr.iSqlError, zErr.iIsamCode, "TPROC VERIFYTRANS8", FALSE));
  /* 
  ** if sqlnotfound or trancode is not any of I(income), M(money), O(open),
  ** C(close), X(transfer), A(adjustment) or R(reversal), then it's an error
  */ 
  else if (zErr.iSqlError == SQLNOTFOUND ||
           (pzTranType->sTranCode[0] != 'I' && pzTranType->sTranCode[0] != 'M'&&
            pzTranType->sTranCode[0] != 'O' && pzTranType->sTranCode[0] != 'C'&&
            pzTranType->sTranCode[0] != 'X' && pzTranType->sTranCode[0] != 'S'&&
            pzTranType->sTranCode[0] != 'A' && pzTranType->sTranCode[0] != 'R'))
    return(PrintError("Invalid TranType", zTrns.iID, zTrns.lDtransNo, "D",
                      46, 0, 0, "TPROC VERIFYTRANS9", FALSE));
  
  // if it's a reversal, make sure that original trade exist and it's not a reversal 
  if (pzTranType->sTranCode[0] == 'R')
  {
 	  lpprSelectRevNoAndCode(&lRevTransNo, psRevTranCode, zTrns.iID, zTrns.lRevTransNo, &zErr);
	  if (zErr.iSqlError == SQLNOTFOUND)
      return(PrintError("Invalid Reversal Trade", zTrns.iID,zTrns.lDtransNo,
                         "D", 53, 0, 0, "TPROC VERIFYTRANS10", FALSE));
    else if (zErr.iSqlError)
      return(PrintError("Error Reading TRANS", zTrns.iID, zTrns.lDtransNo, "D", 0, 
												zErr.iSqlError, zErr.iIsamCode, "TPROC VERIFYTRANS11", FALSE));
    
    // if the record retrieved successfully but it's a reversal, it's an error
    if (psRevTranCode[0] == 'R' || lRevTransNo != 0)
      return(PrintError("Invalid Reversal Trade", zTrns.iID,zTrns.lDtransNo,
                        "D", 109, 0, 0, "TPROC VERIFYTRANS12", FALSE));
  } 
  else
    strcpy_s(psRevTranCode, 2, " ");

  // verify that a valid subaccount record exist 
  lpprSelectSellAcctType(psSellAcctType, zTrns.sAcctType, &zErr);
  if (zErr.iSqlError == SQLNOTFOUND)
    return(PrintError("Invalid Subaccount", zTrns.iID, zTrns.lDtransNo, "D",
                      110, 0, 0, "TPROC VERIFYTRANS13", FALSE));
  else if (zErr.iSqlError)
    return(PrintError("Error Reading SUBACCT", zTrns.iID, zTrns.lDtransNo, "D", 0, 
											zErr.iSqlError, zErr.iIsamCode, "TPROC VERIFYTRANS14", FALSE));

  return zErr;
}// VerifyTransaction 



/**
* This function will set the default data for the Transaction Record
**/
ERRSTRUCT SetDefaultDatainTranproc(TRANS *pzTrans, PORTMAIN zPortmain, ASSETS zAssets, 
																	 TRANTYPE zTranType, long lPriceDate, long lTradeDate, char *sPrimaryType)
{
  ERRSTRUCT zErr;
  double    fPrice, fExrate, fAccrInt, fShares;
  int       iErr = 0;
 
  fPrice = fAccrInt = fShares = 0;
  fExrate = 1;
  InitializeErrStruct(&zErr);

	if (pzTrans->sAcctMthd[0] == ' ')
    strcpy_s (pzTrans->sAcctMthd, zPortmain.sAcctMethod);

  if (pzTrans->lEffDate == 0)
  {
    if (zTranType.iTradeDateInd == 1)
      pzTrans->lEffDate = pzTrans->lTrdDate;
    else
      pzTrans->lEffDate = pzTrans->lStlDate;
  }

  pzTrans->lPostDate = lTradeDate;

  /* 
  ** if transactions' currency is not same as portfolio's base currency, and basexrate is 0,
	** return with an error. Else(same currency id) and basexrate is zero, make it one.
  ** Do the same with IncBaseXrate, SecBaseXrate and AccrBaseXrate
  */
  if (strcmp(pzTrans->sCurrId, zPortmain.sBaseCurrId) != 0)
  {
		if (IsValueZero(pzTrans->fBaseXrate, 12))
			return(PrintError("Invalid BaseXrate", pzTrans->iID, pzTrans->lDtransNo, 
												"D", 67, 0, 0, "TPROC SET DEFAULT1", FALSE));
 } // CurrId != Portmain.basecurrid 
  else if (IsValueZero(pzTrans->fBaseXrate, 12))
    pzTrans->fBaseXrate = 1;  

  // IncomeBaseXrate 
  if (strcmp(pzTrans->sIncCurrId, zPortmain.sBaseCurrId) != 0)
  {
		if (pzTrans->fIncBaseXrate == 0)
		  return(PrintError("Invalid Income BaseXrate", pzTrans->iID, pzTrans->lDtransNo, 
												"D", 121, 0, 0, "TPROC SET DEFAULT2", FALSE));
  } // IncCurrId != Portmain.basecurrid 
  else if (IsValueZero(pzTrans->fIncBaseXrate, 12))
    pzTrans->fIncBaseXrate = 1;  

  // SecBaseXrate 
  if (strcmp(pzTrans->sSecCurrId, zPortmain.sBaseCurrId) != 0)
  {
		if (IsValueZero(pzTrans->fSecBaseXrate, 12))
       return(PrintError("Invalid Security BaseXrate", pzTrans->iID, pzTrans->lDtransNo, 
												 "D", 122, 0, 0, "TPROC SET DEFAULT3", FALSE));
  } // SecCurrId != Portmain.basecurrid 
  else if (IsValueZero(pzTrans->fSecBaseXrate, 12))
     pzTrans->fSecBaseXrate = 1;  

  // AccrBaseXrate 
  if (strcmp(pzTrans->sAccrCurrId, zPortmain.sBaseCurrId) != 0)
  {
		if (IsValueZero(pzTrans->fAccrBaseXrate, 12)) 
       return(PrintError("Invalid Accrual BaseXrate", pzTrans->iID, pzTrans->lDtransNo, 
												 "D", 123, 0, 0, "TPROC SET DEFAULT4", FALSE));
  } // AccrCurrId != Portmain.basecurrid 
  else if (IsValueZero(pzTrans->fAccrBaseXrate, 12))
     pzTrans->fAccrBaseXrate = 1;  

  return zErr;
}// SetDefaultDataInTranproc 


/**
** Function to read dtrans_desc file
**/
ERRSTRUCT GetDtransDescript(int iID, long lTransNo, DTRANSDESC *pzDTranDes,
                            int *piDTranDesCount, BOOL bRebookFlag)
{
  DTRANSDESC zDtransDesc;
  ERRSTRUCT   zErr;

  InitializeErrStruct(&zErr);

  *piDTranDesCount = 0;

  while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
  {
    // Fetch the record from dtransdesc/transdesc 
    if (bRebookFlag == FALSE)
       lpprSelectDtransDesc(&zDtransDesc, iID, lTransNo, &zErr);
    else
			 lpprSelectTransDesc(&zDtransDesc, iID, lTransNo, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
      break;
		}
    else if (zErr.iSqlError != 0)
       return zErr;

    if (*piDTranDesCount == MAXDTRANDESC)     
      return(PrintError("Versus Table Too Small", iID, lTransNo, "T",
												113, 0, 0, "TPROC GETDTRANSDESC", FALSE));
 
    pzDTranDes[*piDTranDesCount].iID = zDtransDesc.iID; 
    strcpy_s(pzDTranDes[*piDTranDesCount].sCloseType, zDtransDesc.sCloseType); 
    strcpy_s(pzDTranDes[*piDTranDesCount].sDescInfo, zDtransDesc.sDescInfo); 
    pzDTranDes[*piDTranDesCount].fUnits = zDtransDesc.fUnits;
    pzDTranDes[*piDTranDesCount].lDtransNo = zDtransDesc.lDtransNo;
    pzDTranDes[*piDTranDesCount].iSeqno = zDtransDesc.iSeqno;
		//pzDTranDes[*piDTranDesCount].lTaxlotNo = zDtransDesc.lTaxlotNo;

    if (bRebookFlag == FALSE || (strcmp(zDtransDesc.sCloseType,"VS") == 0)) 
       pzDTranDes[*piDTranDesCount].lTaxlotNo = zDtransDesc.lTaxlotNo;
    else
       pzDTranDes[*piDTranDesCount].lTaxlotNo = 0;
 
    (*piDTranDesCount)++;
  }  

  return zErr;
}// GetDtransDescript 

/**
** Function to read DPAYTRAN
**/
ERRSTRUCT GetDtransPayee(int iID, long lTransNo, 
												 PAYTRAN *pzPayTran, BOOL bRebookFlag)
{
  ERRSTRUCT   zErr;

  InitializeErrStruct(&zErr);

  // Fetch the record from dtransdesc/transdesc 
  if (bRebookFlag == FALSE)
     lpprSelectDPayTran(pzPayTran, iID, lTransNo, &zErr);
  else
     lpprSelectPayTran(pzPayTran, iID, lTransNo, &zErr);

  if (zErr.iSqlError == SQLNOTFOUND)
		zErr.iSqlError = 0;

  return zErr;
}// GetDtransPayee

/**
** This function adds the given Transaction to the passed DynamicPTable. If the
** table is full, it uses realloc to allocate more space before adding trans
*/
ERRSTRUCT AddTransToPTable(PTABLE *pzPTable, TRANSINFO zTInfo)
{
  ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);


  // If the table is full, create more space 
  if (pzPTable->iCount == pzPTable->iSize)
  {
    pzPTable->iSize += NUMEXTRAELEMENTS;
    pzPTable->pzTInfo = (TRANSINFO *) realloc(pzPTable->pzTInfo, sizeof(TRANSINFO) * pzPTable->iSize);
    if (pzPTable->pzTInfo == NULL)
      return(PrintError("Insufficient Memory To Create Table", 0, 0, "", 997, 0, 0, "TPROC ADDTRANSTODPTABLE", FALSE));
  }

  pzPTable->pzTInfo[pzPTable->iCount] = zTInfo;
  pzPTable->iCount++;

  return zErr;
} // AddTransToPTable 


/**
** This function fills out CancelTable with all the trades of tha given account
** with effective date greater or equal to the given effective date.  
**/
ERRSTRUCT FillOutCancelTable(PTABLE *pzCancelTable, int iID, long lEffDate, 
							 long lTransNo, long lDtransNo, long lBlockTransNo, char* sSecNo, char* sWI)
{
  ERRSTRUCT zErr;
  TRANSINFO zTInfo;

  InitializeErrStruct(&zErr);

  while(zErr.iBusinessError == 0 && zErr.iSqlError == 0)
  {
    InitializeTransInfo(&zTInfo);

    // Fetch the subsequent transactions 
	lpprSelectTrans(&zTInfo.zTrans, &zTInfo.zTranType, iID, lEffDate, lTransNo, sSecNo, sWI, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND)
	{
      // it is perfectly OK not to find any trades to cancel 
      zErr.iSqlError = 0;
      break;
    }
    else if (zErr.iSqlError != 0)
      return zErr;

    if (zTInfo.zTranType.sTradeReverse[0] == 'N')
      continue;

    // Add the entry to the CancelTable 
    if (lTransNo == 0 || (lTransNo != 0 && (zTInfo.zTrans.lTransNo != lTransNo 
		&& zTInfo.zTrans.lXrefTransNo != lTransNo)))
      strcpy_s(zTInfo.zTrans.sCreatedBy, "tranproc");
    zTInfo.lDtransNo = lDtransNo;
		zTInfo.lBlockTransNo = lBlockTransNo;

	zErr = AddTransToPTable(pzCancelTable, zTInfo);

  } // while businesserror = 0 and sqlerror = 0 

  return zErr;
} // FillOutCancelTable 



/** 
** This function writes the needed data of reversed transactions to 
** input log file for further processing by FrontEnd
**/
ERRSTRUCT WriteReversalInformation(PTABLE zCancelTable, char* sFileName, char *sPortfolioName)
{
  ERRSTRUCT Err;
  FILE			*OutputFile;
  short			Tmdy[3],Smdy[3],Emdy[3];
  int				i;
	char			sMsg[80];
 
  
  InitializeErrStruct(&Err);

  OutputFile = fopen(sFileName,"w");
	if (OutputFile == NULL)
	{
		sprintf_s(sMsg, "Error Opening File **%s**", sFileName);
    return(PrintError(sMsg, 0, 0, "", 999, 0, 0, "TPROC WRITEREVERSAL", FALSE));
	}

	fprintf(OutputFile,"TransNo\tType\tTaxLotNo\t\tUnits\tTrade Date\tSettle Date\tEffective Date\tPrincipal  \t\tIncome\t\tRebookFlag \n");
  fprintf(OutputFile,"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
  
	for(i = 0; i < zCancelTable.iCount; i++)
	{
		// If the trade has not been marked to be reversed, don't show it
		if (!zCancelTable.pzTInfo[i].bReversable)
			continue;

	  // Convert dates to mm, dd, yyyy
    lpfnrjulmdy(zCancelTable.pzTInfo[i].zTrans.lTrdDate, Tmdy);
    lpfnrjulmdy(zCancelTable.pzTInfo[i].zTrans.lStlDate, Smdy);
		lpfnrjulmdy(zCancelTable.pzTInfo[i].zTrans.lEffDate, Emdy);
    
		fprintf(OutputFile,"%d\t%s\t%d\t%13.2lf\t%d/%d/%d \t%d/%d/%d \t%d/%d/%d \t%13.2lf \t%13.2lf\t%c\n",
						zCancelTable.pzTInfo[i].zTrans.lTransNo, zCancelTable.pzTInfo[i].zTrans.sTranType,
						zCancelTable.pzTInfo[i].zTrans.lTaxlotNo, zCancelTable.pzTInfo[i].zTrans.fUnits,
						Tmdy[0], Tmdy[1], Tmdy[2], Smdy[0], Smdy[1], Smdy[2], Emdy[0], Emdy[1], Emdy[2],
						zCancelTable.pzTInfo[i].zTrans.fPcplAmt, zCancelTable.pzTInfo[i].zTrans.fIncomeAmt,
						BoolToChar(zCancelTable.pzTInfo[i].bRebookFlag));
  }//for
     
  fclose(OutputFile);

  return Err;
} // WriteReversalInformation