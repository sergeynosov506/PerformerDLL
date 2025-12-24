/**
*
* SUB-SYSTEM: calcperf
*
* FILENAME: calcperf5.ec
*
* DESCRIPTION: Contains 3 functions required for Performance: 
*				AddNewSegment, AddNewSegmentType, AddReturnToBlob
*				(which previously were imported from DelphiCInterface.DLL)
*				as well as new functions to work with UNITVALUE table
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Valeriy Yegorov
*
*
**/
// HISTORY
// 2021-03-03 J# PER-11415 Rolled back chnages -mk.
// 2020-11-11 J# PER-11247 Changed logic on NCF returns -mk.
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2020-08-23 J# PER-11042 Disabled NET returns for segments -mk.
// 2020-04-17 J# PER-10655 Added CN transactions -mk.
// 2013-05-24 VI# 52693 Undid change -mk
// 2013-05-24 VI# 52693 Fix when no record gets updated -mk
// 2013-04-25 VI# 52373 Catch primary key violations -mk
// 2011-10-04 VI# 46694 Even more fixes for UV/summary values -mk
// 2011-10-02 VI# 46694 More fixed for UV/summary values -mk
// 2011-09-28 VI# 46694 Fixes for daily unitvalues -mk
// 2004-03-30 If ending UV becomes too big (and won't fit the table), break stream and continue
//						(rather than fail)- vay
// 2002-08-09 Fixed bug in AddReturnToBlob 
//				(1-Based ReturnType should be converted to 0-based index while accessing ROR array) - vay
// 2002-08-09 Initial testing started - vay

#include "calcperf.h" 

#define IT_SELF_ENTERED 9


int AddNewSegmentType(int iLevel1, int iLevel2, int iLevel3, char *sName, char *sAbbrev, 
					  char *sCode, char *sSecNo, char *sWi, char *sSecDesc1, 
					  BOOL bShouldExistInSegmap, BOOL bSingleSecurity)
{ 
	ERRSTRUCT	zErr;
	SEGMENTS	zSegment;
	int			iLevelID = 0;
	char		sMsg[80];

	lpprInitializeErrStruct(&zErr);
	memset(&zSegment, 0, sizeof(zSegment));

	if (bShouldExistInSegmap) 
	{
		zSegment.iID = lpfnSelectSegmentIDFromSegmap(iLevel1, iLevel2, iLevel3, &zErr);
		if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
		{
			if (zErr.iSqlError != SQLNOTFOUND) 
			{ // if error other then SQLNOTFOUND - set error flag
				sprintf_s(sMsg, "Error Reading Segmap for L1: %d, :2: %d, L3 :%d. Security: %s", iLevel1, iLevel2, iLevel3, sSecNo);
				lpfnPrintError(sMsg,0, 0, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE1", FALSE);
				return -1;
			}
		} 
		else 
		{	// no error and id matching requested levels found
			lpprSelectSegment(&zSegment, &zErr);    
			if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
			{
				if (zErr.iSqlError != SQLNOTFOUND) // if error other then SQLNOTFOUND - set error flag
					lpfnPrintError("Error reading Segments",0, zSegment.iID, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE2", FALSE);
				else
					lpfnPrintError("Combination Exists In Segmap, But ID Does Not Exist In Segments", zSegment.iID, 0, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE3", FALSE);
				
				return -1;
			} 
			else 
			{	// no error and segment id found
				strcpy_s(sName, STR60LEN, zSegment.sName); 
				strcpy_s(sAbbrev, STR20LEN, zSegment.sAbbrev); 
				strcpy_s(sCode, STR12LEN, zSegment.sCode); 
				return zSegment.iID;
			}
		}

		// the combination should exist in segmap but not found, now make sure individual IDs
		// exist in segments, if any one of them is required but does not exist, return with an error,
		// If on the other hand, all the IDs are found, reconstruct the code, name and abbrev for
		// the new combination.
		zSegment.iID = iLevel1;
		lpprSelectSegment(&zSegment, &zErr);	
		if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
		{
			if (zErr.iSqlError != SQLNOTFOUND) 
			{// if error other then SQLNOTFOUND - set error flag
				sprintf_s(sMsg, "Error reading Level1 Segment %d, Security: %s", zSegment.iID, sSecNo);
				lpfnPrintError(sMsg,0, zSegment.iID, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE4", FALSE);
				return -1;
			} 
			else 
			{
				sprintf_s(sMsg, "Level1 Segment %d does not exist in Segments, Security: %s", zSegment.iID, sSecNo);
				lpfnPrintError(sMsg, zSegment.iID, 0, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE5", FALSE);
				return -102;
			}
		} 

		if (iLevel2 == -1) // if no Level2 used - return Level1 ID
			return iLevel1;

		// continue checking on Level2 ID
		zSegment.iID = iLevel2;
		lpprSelectSegment(&zSegment, &zErr);	
		if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
		{
			if (zErr.iSqlError != SQLNOTFOUND) 
			{// if error other then SQLNOTFOUND - set error flag
				sprintf_s(sMsg, "Error reading Level2 Segment %d, Security: %s", zSegment.iID, sSecNo);
				lpfnPrintError(sMsg,0, zSegment.iID, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE6", FALSE);
				return -1;
			} 
			else 
			{
				sprintf_s(sMsg, "Level2 Segment %d does not exist in Segments, Security: %s", zSegment.iID, sSecNo);
				lpfnPrintError(sMsg, zSegment.iID, 0, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE7", FALSE);
				return -103;
			}
		} 

		iLevelID = 20;

		// continue checking on Level3 if required
		if (iLevel3 !=  -1) 
		{
	   	zSegment.iID = iLevel3;
			lpprSelectSegment(&zSegment, &zErr);	
			if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
			{
				if (zErr.iSqlError != SQLNOTFOUND) 
				{// if error other then SQLNOTFOUND - set error flag
					sprintf_s(sMsg, "Error reading Level3 Segment %d, Security: %s", zSegment.iID, sSecNo);
					lpfnPrintError(sMsg,0, zSegment.iID, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE8", FALSE);
					return -1;
				} 
				else 
				{				
					sprintf_s(sMsg, "Level3 Segment %d does not exist in Segments, Security: %s", zSegment.iID, sSecNo);
					lpfnPrintError(sMsg, zSegment.iID, 0, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE9", FALSE);
					return -104;
				}
			} 

			iLevelID = 30;
		} // if Level3
	} 
	else 
	{ // if segments Should Exist In Segmap
 		iLevelID = 40;

		strcpy_s(zSegment.sName, sName);
		strcpy_s(zSegment.sAbbrev, sAbbrev);
		strcpy_s(zSegment.sCode, sCode);

		if (bSingleSecurity) // if single security segment
		{
			iLevelID = 400;
			strcpy_s(zSegment.sCode, sSecNo);
			if (sSecDesc1) 
			{
				strcpy_s(zSegment.sName, sSecDesc1);
				sSecDesc1[sizeof(zSegment.sAbbrev)-1] = '\0';
				strcpy_s(zSegment.sAbbrev, sSecDesc1);
			}
		}
	}

	zSegment.iLevelID = iLevelID;
	// name/abbrev/code already set
	// sequence_no will be set automatically on Insert 
	
	lpprInsertSegment(&zSegment, &zErr);
	if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
	{
		lpfnPrintError("Error Creating New Segment ", 0, 0, "", 999, 0, 0, "ADDNEWSEGMENTTYPE10", FALSE);
		return -4;
	}

	strcpy_s(sName, STR60LEN, zSegment.sName); 
	strcpy_s(sAbbrev, STR20LEN, zSegment.sAbbrev);
	strcpy_s(sCode, STR12LEN, zSegment.sCode); 

	if (bSingleSecurity) // if single security segment
	{
		lpprInsertSecSegmap(sSecNo, sWi, zSegment.iID, &zErr);
		if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
		{
			lpfnPrintError("Error Adding Segment To SecSegmap Table ", 0, zSegment.iID, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE11a", FALSE);
			return -5;
		}
	}
	else
	if (iLevelID == 20 || iLevelID == 30) 
	{
		lpprInsertSegmap(zSegment.iID, iLevel1, iLevel2, iLevel3, &zErr); 
		if (zErr.iSqlError !=0 || zErr.iBusinessError !=0) 
		{
			lpfnPrintError("Error Adding Segment To Segmap Table ", 0, zSegment.iID, "G", 999, 0, 0, "ADDNEWSEGMENTTYPE11b", FALSE);
			return -5;
		}
	}

	return zSegment.iID;
} //AddNewSegmentType

// this is a functionally identical replica of DelphiCInterface.DLL's AddNewSegment
DllExport int AddNewSegment(int iPortID, char *sName, char *sAbbrev, int iSegmentType)
{
	ERRSTRUCT zErr;
	long			lSegmentID = 0;
	SEGMAIN		zSegmain;

	lpprInitializeErrStruct(&zErr);

	// check if Segment already exists
    lpprSelectSegmain(iPortID, iSegmentType, &lSegmentID, &zErr);
	if (zErr.iSqlError !=0 || zErr.iBusinessError != 0)  // if any error occured 
    {
		if (zErr.iSqlError != SQLNOTFOUND) 
		{
			//  and it's not SQLNOTFOUND then set a error flag
			lpfnPrintError("Error Selecting Segment", 0, iSegmentType, "G", zErr.iSqlError, 0, 0, "ADDNEWSEGMENT1", FALSE);
			return -1;
		}
	}

    if (lSegmentID <= 0) // create new segmain entry
	{
		memset(&zSegmain, 0, sizeof(zSegmain));
		zSegmain.iID = 0;
		zSegmain.iOwnerID = iPortID;
		zSegmain.iSegmentTypeID = iSegmentType;
		
		/*
		* 12/5/2001 According to SB, Segmain's Name/Abbrev should be blank upon creation
		* to force Performer's Reporting system to use Name/Abbrev from master Segments
		* if no user-defined name/abbrev specified in Segmain.

		// sName came from Segments table and it's length is up to 60 chars
		// Name/Abbrev on Segmain is 40/20 chars only, to avoid AV errors 
		// we have to truncate them here 
		strncpy(zSegmain.sSegmentName, sName, sizeof(zSegmain.sSegmentName)-1);
		strncpy(zSegmain.sSegmentAbbrev, sName, sizeof(zSegmain.sSegmentAbbrev)-1);
		*/

		lpprInsertSegmain(&zSegmain, &zErr); // this query will come back with new ID 
		if (zErr.iSqlError !=0 || zErr.iBusinessError != 0) // if any error occured 
		{
			lpfnPrintError("Error Inserting Segment", 0, iSegmentType, "G", zErr.iSqlError, 0, 0, "ADDNEWSEGMENT2", FALSE);
			return -1;
		}
		lSegmentID = zSegmain.iID;
	} //lSegmentID <= 0


	// check if Segment already exists in Segtree
    lpprSelectSegtree(iPortID, iSegmentType, &zErr);
	if (zErr.iSqlError !=0 || zErr.iBusinessError != 0) // if any error occured 
	{   //  and it's not SQLNOTFOUND 
		if (zErr.iSqlError != SQLNOTFOUND) 
		{
			lpfnPrintError("Error Selecting Segtree", 0, iSegmentType, "G", zErr.iSqlError, 0, 0, "ADDNEWSEGMENT3", FALSE);
			return -1;
		}
		else 
		{
			lpprInsertSegtree(iPortID, iSegmentType, lSegmentID, &zErr);
			if (zErr.iSqlError !=0 || zErr.iBusinessError != 0) 
			{ // if any error occured 
				lpfnPrintError("Error Inserting Segtree", 0, iSegmentType, "G", zErr.iSqlError, 0, 0, "ADDNEWSEGMENT2", FALSE);
				return -1;
			}
		}
	}

	return lSegmentID;
} //AddNewSegment


/**
** Function to initialize RorTable structure
**/
void InitializeRorTable(RORTABLE *pzRorTable)
{
  if (pzRorTable->iCount > 0 && pzRorTable->pzRor != NULL)
    free(pzRorTable->pzRor);
 
  pzRorTable->pzRor = NULL;
  pzRorTable->iCount = pzRorTable->iCapacity = 0;
} /* initializerorstable */

/**
** This function adds Ror record in the Ror table. This function does not
** check whether the passed Ror record already exist in the table or not.
**/
ERRSTRUCT AddRorToTable(RORTABLE *pzRorTable, ALLRORS zRor)
{

  ERRSTRUCT zErr;
	//char smsg[100];
 
  lpprInitializeErrStruct(&zErr);
 
  /* If table is full to its limit, allocate more space */
  if (pzRorTable->iCapacity == pzRorTable->iCount)
  {
    pzRorTable->iCapacity += 6; // add 6 more elements - half of an year if monthly rors
    pzRorTable->pzRor = (ALLRORS *)realloc(pzRorTable->pzRor, pzRorTable->iCapacity * sizeof(ALLRORS));
    if (pzRorTable->pzRor == NULL)
      return(lpfnPrintError("Insufficient Memory For ROR Table", 0, 0, "", 997, 0, 0, "CALCPERF ADDROR", FALSE));

		//sprintf_s(smsg, "Memory Address For Ror Table is; %Fp", pzRorTable->pzRor);
		//lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG", TRUE);
  }
 
  pzRorTable->pzRor[pzRorTable->iCount] = zRor;
  pzRorTable->iCount++;
 
  return zErr;
} /* addrortotable */

/**
** Function to initialize UVTable structure
void InitializeUVTable(UVTABLE *pzUVTable)
{
  if (pzUVTable->iCount > 0 && pzUVTable->pzUV != NULL)
    free(pzUVTable->pzUV);
 
  pzUVTable->pzUV = NULL;
  pzUVTable->iCount = pzUVTable->iCapacity = 0;
} /* initializeUVtable */

/**
** This function adds UV value to  the BLOB in memory. 
ERRSTRUCT AddUVToTable(UVTABLE *pzUVTable, double fUV)
{
  ERRSTRUCT zErr;
  UVTYPE_SQL uvLocal;
 
  lpprInitializeErrStruct(&zErr);
 
  // If table is full to its limit, allocate more space 
  if (pzUVTable->iCapacity == pzUVTable->iCount)
  {
    pzUVTable->iCapacity += 6; // add 6 more elements - half of an year if monthly rors
    pzUVTable->pzUV = (UVTYPE_SQL *)realloc(pzUVTable->pzUV, pzUVTable->iCapacity * sizeof(UVTYPE_SQL));
    if (pzUVTable->pzUV == NULL)
      return(lpfnPrintError("Insufficient Memory For UV Table", 0, 0, "", 997, 0, 0, "CALCPERF ADDUV", FALSE));
  }
 
  memset(&uvLocal, 0, sizeof(uvLocal));	// for debug only
  lpfnDblToExt(fUV, &uvLocal);
  pzUVTable->pzUV[pzUVTable->iCount] = uvLocal; 
  pzUVTable->iCount++;
 
  return zErr;
} /* addUVtotable */

/**
** This function copies data from original BLOB to new one
ERRSTRUCT CopyDataFromOrigBlob(UVTABLE *pzUVTable, UNITVALU zUnitValu)
{
  UVTYPE_SQL *uvLocal = NULL;	
  ERRSTRUCT zErr;
  lpprInitializeErrStruct(&zErr);
 
  /* If table is full to its limit, allocate more space 
  if (pzUVTable->iCapacity == pzUVTable->iCount)
  {
    pzUVTable->iCapacity += zUnitValu.ulNumValues + 6; // add 6 more elements - half of an year if monthly rors
    pzUVTable->pzUV = (UVTYPE_SQL *)realloc(pzUVTable->pzUV, pzUVTable->iCapacity * sizeof(UVTYPE_SQL));
    if (pzUVTable->pzUV == NULL)
      return(lpfnPrintError("Insufficient Memory For UV Table", 
							0, 0, "", 997, 0, 0, "CALCPERF COPYDATAFROMORIGBLOB", FALSE));
  }
 
  uvLocal = &pzUVTable->pzUV[pzUVTable->iCount];
  memcpy(uvLocal, zUnitValu.pzUVBlob, zUnitValu.ulNumValues * sizeof(UVTYPE_SQL));
  pzUVTable->iCount +=  zUnitValu.ulNumValues;
 
  return zErr;
} /* CopyDataFromOrigBlob*/

int GetNumberofMonthsInPeriod(int PeriodType)
//---------------------------------------------------------------------------------------
//		  This function returns how many months are in a period
//---------------------------------------------------------------------------------------
{
	switch  (PeriodType)
	{
		case ptUnknown:			return INVALID_VALUE;
		case ptDaily:			return 0;
		case ptMonthly:			return 1;
		case ptQuarterly:		return 3;
		case ptSemiAnnually:	return 6;
		case ptYearly:			return 12;
		default:				return INVALID_VALUE;
	}; // switch
}// GetNumberOfMonthsinPeriod

BOOL IsLeapYear(short YY)
{
	short iMDY[3];
	long lDate;

	// construct 03/01/YY date
	iMDY[0]=3; iMDY[1]=1; iMDY[2]=YY;
	// convert it to Julian date
	lpfnrmdyjul(iMDY, &lDate);
	// subtract 1 day to go back to February
	lDate--;
	// convert Julian date to MDY 
	lpfnrjulmdy(lDate,iMDY);
	// now check the Day part: if it is Feb 29 -- YY is leap year
	return (iMDY[1] == 29);
}

long PreviousPeriod(long lCurrentDate, int iPeriodType)
//-------------------------------------------------------------------------------------------
//   This function decrements the passed in date by time specified by period  type
//-------------------------------------------------------------------------------------------
{
	short iMDY[3];
	int MMSub, YYSub;
	div_t divres;
	long lTempDate;

	if (iPeriodType != ptUnknown) {

		lpfnrjulmdy(lCurrentDate, iMDY);

		// Calculate total Number of Months to Add to Original Date
		MMSub = GetNumberofMonthsInPeriod(iPeriodType);

		//  Calculate The Number of years to Subtract
		divres = div (MMSub, 12);
		YYSub = divres.quot;
		// Calculate the Remaining Months to Subtract
		MMSub = divres.rem;

		// Subtract the additional Months 
		if (iMDY[0] <= MMSub) {
			YYSub++;  //<--- increase the years to subtract
			iMDY[0] = 12 - (MMSub - iMDY[0]);
		} else 
			iMDY[0] -= MMSub;

		// Subtract the Years
		iMDY[2] -= YYSub;

		//Get the Last Day of the Month and Year
		iMDY[1] = MonthDays[IsLeapYear(iMDY[2])] [iMDY[0]-1];

	    //Encode the New Date
		lpfnrmdyjul(iMDY, &lTempDate);
		return lTempDate;

   } else 
      return INVALID_DATE;
} 

long NextPeriod(long lCurrentDate, int iPeriodType)
//----------------------------------------------------------------------------------------
//   This function increments the passed in date by time specified by period  type
//----------------------------------------------------------------------------------------
{
	short iMDY[3];
	int MMAdd, YYAdd;
	div_t divres;
	long lTempDate;

	if (iPeriodType != ptUnknown) {

		lpfnrjulmdy(lCurrentDate, iMDY);

		// Calculate total Number of Months to Add to Original Date
		MMAdd = GetNumberofMonthsInPeriod(iPeriodType);

		//  Calculate The Number of years to Add
		divres = div (MMAdd, 12);
		YYAdd = divres.quot;
		// Calculate the Remaining Months to Add
		MMAdd = divres.rem;

		// Add the additional Months 
		iMDY[0] += MMAdd;

		if (iMDY[0] > 12) {
			// Overlapped one year
			YYAdd++;
			iMDY[0] -= 12;
		};

		//Add the Years
		iMDY[2] += YYAdd;

		// Get the Last Day of the Month and Year
		iMDY[1] = MonthDays[IsLeapYear(iMDY[2])] [iMDY[0]-1];

		// Encode the New Date
		lpfnrmdyjul(iMDY, &lTempDate);
		return lTempDate;

	} else 
      return INVALID_DATE;

}//NextPeriod

double RORToEndingUV( double fROR, double fBUV)
//--------------------------------------------------------------------------------------------
//   This function calculates an endingUnit value for the given
//   Rate of Return and the Beginning Unit Value
//--------------------------------------------------------------------------------------------
{
   return fBUV * ((fROR / 100) + 1);
}

// this is almost functionally identical replica of Delphi's 
// T_ListOfReturns.CreateUnitValueFromBlobAndList
/*
void CreateUnitValueFromBlobAndList(UNITVALU *pzUnitValu, RORTABLE zRorList)
{
	long	lListStartDate, lBlobLastDate, lNextDate;
	ULONG	ulStartDeletingFrom = 0;
	double  fUV, fBUV; 
	double	fROR;
	int i, iListStartingIndex;
	UVTABLE	zNewUVBlob;

	ERRSTRUCT zErr;
	lpprInitializeErrStruct(&zErr);
	memset(&zNewUVBlob, 0, sizeof(zNewUVBlob));
	

	lListStartDate = zRorList.pzRor[0].zIndex[0].lRtnDate; // RtnDate is being passed at zIndex[0] !!!
   
	// Figure out the date upto which blob data will be kept
    lBlobLastDate = PreviousPeriod(lListStartDate, pzUnitValu->iPeriodType);

	//-------- load unit values from the blob --------
	pzUnitValu->lBeginDate = 0;
	lpprSelectUnitValu(pzUnitValu, &zErr);
	if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) // if error reading blob
		if (zErr.iSqlError != SQLNOTFOUND) { // other than not found
			lpfnPrintError("Error Loading Unitvalu", 
						0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST1", FALSE);
			return; // all returns must have some error flag ?!
		}


	// Figure out the item index at which BlobLastDate is found
	lNextDate = pzUnitValu->lBeginDate;
	ulStartDeletingFrom = 0;
	while (lNextDate <= lBlobLastDate) {
		lNextDate = NextPeriod(lNextDate, pzUnitValu->iPeriodType);
        ulStartDeletingFrom++;
	};

	// delete all the data from the list after BlobLastDate
	// Note:  we actually nullify requested part at the blob's tail
	if (pzUnitValu->pzUVBlob) {
		if (ulStartDeletingFrom < pzUnitValu->ulNumValues)	{
			memset(&pzUnitValu->pzUVBlob[ulStartDeletingFrom], 0, 
					(pzUnitValu->ulNumValues - ulStartDeletingFrom)*sizeof(UVTYPE_SQL));
			pzUnitValu->ulNumValues = ulStartDeletingFrom;
		}

		// copy unchanged part of blob data to new buffer
		zErr = CopyDataFromOrigBlob(&zNewUVBlob, *pzUnitValu);
		if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { 
			lpfnPrintError("Error Copying Data from orig BLOB", 
						0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST1a", FALSE);
			return; // all returns must have some error flag ?!
		}
	}

	// Now we have to add the returns(after converting them into unit values) from Self
	iListStartingIndex = -1;
	i = 0;
	while (iListStartingIndex == -1 && i < zRorList.iCount) {
		if (zRorList.pzRor[i].zIndex[0].lRtnDate == lListStartDate) // RtnDate is being passed at zIndex[0] !!!
			iListStartingIndex = i;

		i++;
    }

   // if a record for the ListStartingDate is found in the list then convert the
   // period ending rors from that point onward into unitvalue and add to list of unit values
	if (iListStartingIndex >= 0) {

		if (pzUnitValu->ulNumValues == 0) {

			pzUnitValu->lBeginDate = PreviousPeriod(lListStartDate, pzUnitValu->iPeriodType);
			strcpy_s(pzUnitValu->sDataHoleExists, "F");

			fBUV = 100;
			zErr = AddUVToTable(&zNewUVBlob, fBUV);
			if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { // if error 
				lpfnPrintError("Error Adding BUV", 
						0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST2", FALSE);
				return; // all returns must have some error flag ?!
			}

		} else
			if (!lpfnExtToDbl (pzUnitValu->pzUVBlob[pzUnitValu->ulNumValues-1], &fBUV)) {
				lpfnPrintError("Error Converting BUV", 
							0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST3", FALSE);
				return; // all returns must have some error flag ?!
			}

		lNextDate = lListStartDate;
		i = iListStartingIndex;

		while (i < zRorList.iCount) {
			fROR = zRorList.pzRor[i].zIndex[pzUnitValu->iReturnType - 1].fBaseRorIdx;

			if (zRorList.pzRor[i].zIndex[0].lRtnDate = lNextDate) { // RtnDate is being passed at zIndex[0] !!!
       
				// Calculate the ending Unit value
				if (fROR != NAVALUE) {

					if (fBUV == NAVALUE) 
						fBUV = 100;

					fUV = RORToEndingUV(fROR, fBUV);

					zErr = AddUVToTable(&zNewUVBlob, fUV);
					if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { // if error 
						lpfnPrintError("Error Adding UV", 
								0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST4", FALSE);
						return; // all returns must have some error flag ?!
					}
				} else { // if fROR is invalid
				
					fUV = fROR;
					zErr = AddUVToTable(&zNewUVBlob, fUV); //<--- invalid value in stream;
					if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { // if error 
						lpfnPrintError("Error Adding UV", 
								0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST4", FALSE);
						return; // all returns must have some error flag ?!
					}
					strcpy_s(pzUnitValu->sDataHoleExists, "T");

				} // if ROR is invalid 

				fBUV = fUV; //<--- make current uv beginning uv

				// -------- Get next date and also next record in the list --------
				lNextDate = NextPeriod(lNextDate, pzUnitValu->iPeriodType);
				i++;
			} // ReturnSet[i].Date = NextDate
		
			else if (zRorList.pzRor[i].zIndex[0].lRtnDate > lNextDate) { // RtnDate is being passed at zIndex[0] !!!

				// no return for date, add invalid for date, change date but don't go to the next item in the list
				fBUV = NAVALUE;
				zErr = AddUVToTable(&zNewUVBlob, fBUV);
				if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { // if error 
					lpfnPrintError("Error Adding BUV", 
							0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST5", FALSE);
					return; // all returns must have some error flag ?!
				}
				strcpy_s(pzUnitValu->sDataHoleExists, "T");
				lNextDate = NextPeriod(lNextDate, pzUnitValu->iPeriodType);
			}
			else if (zRorList.pzRor[i].zIndex[0].lRtnDate < lNextDate) { // RtnDate is being passed at zIndex[0] !!!
				//
				// There are two possibilities when date in the list is less than the next period
				// date, first, it is a inter-period valuation(10 % flow) ror, and the second is
				// it is the daily return. In the second situation the return will be the last
				// entry in the list. In both the cases the returns is used to calculate the next
				// unit value but the only one which is written to the list is daily ror(last ror in the list).
				//
				if (fROR != NAVALUE) {

					if (fBUV == NAVALUE)
						fBUV = 100;
           
					fUV = RORToEndingUV(fROR, fBUV);
					fBUV = fUV; //<--- make current uv beginning uv
				} else {
					fBUV = NAVALUE; // Begin UV is invalid
				}; //if ROR is invalid}

				// last item in the list, assume daily and add to the blob, others ignore
				if (i == (zRorList.iCount - 1)) {
					zErr = AddUVToTable(&zNewUVBlob, fUV);
					if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { // if error 
						lpfnPrintError("Error Adding UV", 
							0, pzUnitValu->iID, "G", 999, 0, 0, "CREATEUVFROMBLOB&LIST6", FALSE);
						return; // all returns must have some error flag ?!
					};
				};

				// go to the next item in the list, but don't change the next date
				i++;
			}; // ReturnSet[i].Date < NextDate
		}; // {while i < Self.count}

	}; // if ListStartingIndex


	// free memory of original blob which was allocated in OLEDBIO.DLL 
	// (must be freed by call to OLEDBIO's heap manager)
	lpprFreeUnitValu(pzUnitValu); 
	// and then pass back pointer to just created blob in memory
	pzUnitValu->pzUVBlob = zNewUVBlob.pzUV;
	pzUnitValu->ulNumValues = zNewUVBlob.iCount;
	
	//InitializeUVTable(&zNewUVBlob); this memory will be freed later on when finishing blob saving
}

int SaveUnitValu(UNITVALU zUnitValu)
{			
	ERRSTRUCT zErr;

	lpprSaveUnitValu(zUnitValu, &zErr);
	if (zErr.iSqlError!=0 || zErr.iBusinessError !=0) { // if error 
		lpfnPrintError("Error Saving UnitValu", 
			0, zUnitValu.iID, "G", zErr.iSqlError, zErr.iBusinessError, 0, "SAVEUNITVALU", FALSE);
		return 0; 
	};

	return 1;
}*/


/* This is obsolete code - no UV storage in BLOB anymore - vay, 8/13/03 
// this is a functionally identical replica of DelphiCInterface.DLL's AddReturnToBlob
int  AddReturnToBlob(long iSegmentID, long iReturnType, long lReturnDate,
                     double fROR, BOOL bStart, BOOL bFinish)
{
	static RORTABLE zRorList;
	BOOL bCreateNew = TRUE; // set to true on every call, will be changed later on if required
	ALLRORS zRor;
	UNITVALU zUnitValu;
	BOOL bUseTrans = FALSE;
	int i;

  
  // If starting a new segment returns the previous segment must have been finished and
  // at the time of finish zRorList gets freeed and niled. If zRorList is nil then create
  // a new one and if it is not then make it nil, print a warning and continue (this most
  // likely happens when there is some error in the previous portfolio in adding returns to the blob).
  
    //lpfnTimer(43);

	if (bStart) {
		// starting a new one without finishing the last one, print a warning and continue
		if (zRorList.iCount) 
			lpfnPrintError("Start Of the Process And Return List Is Not Nil. Making It Nil", 
							0, iSegmentID, "G", 999, 0, 0, "ADDRETURNTOBLOB1", TRUE);

		InitializeRorTable(&zRorList);
	}
	else if (!zRorList.iCount) {
		lpfnPrintError("Not The Start Of the Process And Return List Is Nil", 
						0, iSegmentID, "G", 999, 0, 0, "ADDRETURNTOBLOB2", FALSE);
		return  -2;
    };

  
  // If finishing an old segment, it must have previously been started and at the time
  // of start zRorList gets created. So if zRorList is nil, return with an error.

	if ((bFinish) && (!zRorList.iCount)) {
		lpfnPrintError("End Of the Process And Return List Is Nil", 
						0, iSegmentID, "G", 999, 0, 0, "ADDRETURNTOBLOB3", FALSE);
		return -3;
	}

  
  // This function will be called several times(= number of ReturnTypes defined in the system)
  // for the same return date. If the return date is same as the date of last item in the list,
  // no need to add a new record else if it is earlier than that of the last item it is an error.
  // If it is later than that of the last item in the list then add a new record to the list.

	if (zRorList.iCount) 
		if (zRorList.pzRor[zRorList.iCount - 1].zIndex[0].lRtnDate == lReturnDate) 
			bCreateNew = FALSE;
		else if (zRorList.pzRor[zRorList.iCount - 1].zIndex[0].lRtnDate > lReturnDate) {
			lpfnPrintError("Last Entry In Return List Is For A Date Greater Than Current", 
							0, iSegmentID, "G", 999, 0, 0, "ADDRETURNTOBLOB4", FALSE);
			return  -4;
		}


	if (bCreateNew) {
		memset(&zRor,0,sizeof(zRor));

		zRor.zIndex[0].iID = iSegmentID;
		zRor.zIndex[0].lRtnDate = lReturnDate;
		AddRorToTable(&zRorList, zRor);
	};

	if (iReturnType<GTWRor || iReturnType>NTWAfterTaxRor) {
		lpfnPrintError("Invalid Return Type", 0, iReturnType, "", 999, 0, 0, "ADDRETURNTOBLOB5", FALSE);
		return -5;
	}

	// save current ror in the table - don't forget that zIndex is zero based array
	// while iReturnType is 1-based index...
	zRorList.pzRor[zRorList.iCount - 1].zIndex[iReturnType - 1].fBaseRorIdx = fROR;

	if (bFinish) {

		// Create list of UnitValu entries based on current data in the BLOB
		// and RorList in the memory, then save UnitValu

		bUseTrans = (lpfnGetTransCount==0);
		if (bUseTrans)
			lpfnStartTransaction();

		__try
		{
			for (i=1; i<=NUMRORTYPE_ALL; i++) 
			{
   				//lpfnTimer(44);

 				if (i == GDWRor)  // right now (4/7/99) this is not being calculated 
					continue;

				zUnitValu.iID = zRorList.pzRor[0].zIndex[0].iID;
				zUnitValu.iReturnType = i;
				zUnitValu.iItemType = IT_SELF_ENTERED;
				zUnitValu.iPeriodType = ptMonthly;

				CreateUnitValueFromBlobAndList(&zUnitValu, zRorList); 
			
				//lpfnTimer(45);

				if (!zUnitValu.pzUVBlob) {
					lpfnPrintError("CreateUnitValue Date Greater Than Current", 0, 
							iSegmentID, "G", 999, 0, 0, "ADDRETURNTOBLOB6", FALSE);

					if (bUseTrans)
						lpfnRollbackTransaction();
					return -6;
				};
			

				if (!SaveUnitValu(zUnitValu)) {
				
					if (bUseTrans)
						lpfnRollbackTransaction();
					return -7; 
				}

				//lpfnTimer(46);

				// free BLOB  memory
				free(zUnitValu.pzUVBlob);

				//lpfnTimer(47);

			}// for
		}//try
		__except(lpfnAbortTransaction(bUseTrans)){}

		if (bUseTrans)
			lpfnCommitTransaction();

		InitializeRorTable(&zRorList);
	}  
	
	return 0;
}
*/

// this function copies UnitValue data into specified Index
void CopyUnitValueToBeginIndex(ALLRORS *pzUVAll, UNITVALUE zUnitValue)
{
	int iRorTypeIndex = zUnitValue.iRorType-1; // arrays are zero based
	pzUVAll->zUVIndex[iRorTypeIndex] = zUnitValue;

	// Sb 5/27/15 Simplified the structure
	pzUVAll->fBaseRorIdx[iRorTypeIndex] = zUnitValue.fUnitValue;
//	pzUVAll->zIndex[iRorTypeIndex].iRorType = zUnitValue.iRorType;
	//pzUVAll->zIndex[iRorTypeIndex].iID = zUnitValue.iID;
	//pzUVAll->zIndex[iRorTypeIndex].iPortfolioID = zUnitValue.iPortfolioID;
	//pzUVAll->zIndex[iRorTypeIndex].lRtnDate = zUnitValue.lUVDate;
}

void CopyUVIndex(ALLRORS *pzUVNew, ALLRORS zUVOld)
{
	int i;
	for (i=0; i<NUMRORTYPE_ALL; i++)
		pzUVNew->zUVIndex[i] = zUVOld.zUVIndex[i];
}

// This function fills in BeginIndex with Unit Values from the Database
ERRSTRUCT ReadBeginningUnitValues(PKEYTABLE *pzPKTable, int iPortfolioID, long lEarliestNondDate)
{
	ERRSTRUCT zErr;
	UNITVALUE zUnitValue;
	int iNumRec = 0; 
	int iKeyIndex = 0;
	
	lpprInitializeErrStruct(&zErr);
  
	while (zErr.iSqlError == 0) 
	{
		lpprSelectUnitValue(&zUnitValue, iPortfolioID, lEarliestNondDate,  &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		// save UV stream beginning values under the latest active key (not deleted from DB) 
		// that later on for newly incepted keys stream can be continued 
		iKeyIndex = FindPerfkeyByID(*pzPKTable, zUnitValue.iID, 0, TRUE); 
		if (iKeyIndex>=0)
		{
			CopyUnitValueToBeginIndex(&pzPKTable->pzPKey[iKeyIndex].zBeginIndex, zUnitValue);
		}
		else 
		{
			iKeyIndex = FindPerfkeyByIDBackward(pzPKTable, zUnitValue.iID, 0); 
			iKeyIndex = FindPerfkeyByIDBackward(pzPKTable, zUnitValue.iID, iKeyIndex); 

			if (iKeyIndex>=0)
				CopyUnitValueToBeginIndex(&pzPKTable->pzPKey[iKeyIndex].zNewIndex, zUnitValue);
		}	
		
 
		iNumRec++;
	} /* while no error */
  
	return zErr;
}

/**
** Function to find perfkey by matching id in the given PKeyTable, going backward.
**/
int FindPerfkeyByIDBackward(PKEYTABLE *pzPKTable, int iID, int iStartIndex)
{
  int i, iIndex;
 
	iIndex = -1;
	if (iStartIndex > 0 && iStartIndex <= pzPKTable->iCount)
		i = iStartIndex;
	else
		i = pzPKTable->iCount;


  while (iIndex == -1 && i>0)
  {
    i--;

 	if ((pzPKTable->pzPKey[i].zPK.iID == iID) &&
		!(pzPKTable->pzPKey[i].bDeletedFromDB))
      iIndex = i;
  }
 
  return iIndex;
} /* findperfkeyByIDBackward */

/**
** This function creates a UNITVALUE record for a key and rortype combination, by
** copying the values from NewIndex variable of the given key and then inserts
** or updates that record in the given table (daily or period, based on sDBName)
**
** Actually, this is a replacement for InsertOrUpdateRor
**/
ERRSTRUCT InsertOrUpdateUV(PKEYTABLE *pzPTable, int iKeyIndex, char *sDBName, RORSTRUCT zRorStruct, 
													 long lLastPerfDate, long lLndPerfDate, long lCurrPerfDate, BOOL bPeriodEnd, PARTPMAIN	zPmain)
{
	ERRSTRUCT	zErr;
	int				i; //, j;
	UNITVALUE	zUnitValue;
	BOOL			bInceptedNextMonth = FALSE;
	int				iBatchSize;
	long		aDate;
 
	lpprInitializeErrStruct(&zErr);

/*	if (TaxInfoRequired(zRorStruct.iReturnstoCalculate))
		j = NUMRORTYPE_ALL;
	else
		j = NUMRORTYPE_BASIC;*/
 
	// batch saving of UV only allowed when running daily perf
	if (zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval, "D") == 0) 
		iBatchSize = zSysSet.iInsertBatchSize;
	else
		iBatchSize = 0;

	// copy UV stream from old key to new one if they both are within one period
	// first, check if this is Inception entry - try to find previous Termination entry
	// if both are within one period (month), then keep UV stream
	if (zRorStruct.bInceptionRor)
	{
		int iPrevIndex = 0;
		long lNextPeriodEnd = 0;

		iPrevIndex = FindPerfkeyByIDBackward(pzPTable, pzPTable->pzPKey[iKeyIndex].zPK.iID, iKeyIndex);
		if ((iPrevIndex != -1) && (iPrevIndex<iKeyIndex))
		{
			lNextPeriodEnd = NextPeriod(pzPTable->pzPKey[iPrevIndex].zPK.lDeleteDate, ptMonthly);

			if (lNextPeriodEnd>=pzPTable->pzPKey[iKeyIndex].zPK.lInitPerfDate) 
			{
				CopyUVIndex(&pzPTable->pzPKey[iKeyIndex].zBeginIndex, pzPTable->pzPKey[iPrevIndex].zNewIndex);
				bInceptedNextMonth = TRUE;
			}
		}
	}

	for (i = 1; i <= NUMRORTYPE_ALL; i++)
	{
		// don't save these types of ROR - they are calculated on the fly by report engine
		//if ((i==GDWRor) || (i==NDWRor)) 
			//continue;

		// don't save return types which are not requested
		//if (!(pzPTable->bRorType[i-1]))
			//continue;

		/*
		** If the current return type we are looking at doesn't need to be calculated, obviously it doesn't
		** need to be stored. Also, for net returns even if it needs to be calculated but it's not total 
		** portfolio, no need to store sreturn (i.e. net return gets saves only for total account)
		* /
		SB 8/8/08 - After not saving net of fee returns was implemented, an issue with net of fee cbyc composites not
								working as expected was found, so for now, revert the changes regarding not saving net of fee returns 
								on segments back to its original state (i.e. continue saving net of fee returns even on segments)
								until issue with composite merge is fixed. 
		if (i == GTWRor && !(zRorStruct.iReturnstoCalculate & TWRorBit)) 
			continue;
		else if (i == GPcplRor && !(zRorStruct.iReturnstoCalculate & PcplRorBit))
			continue;
		else if (i == NTWRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & TWRorBit)))
			continue;
		else if (i == NPcplRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & PcplRorBit)))
			continue;
		else if (i == IncomeRor && !(zRorStruct.iReturnstoCalculate & IncomeRorBit))
			continue;
		else if (i == GDWRor || i == NDWRor)
			continue;
		else if (i == GTWTaxEquivRor && !(zRorStruct.iReturnstoCalculate & TaxEquivRorBit))
			continue;
		else if (i == NTWTaxEquivRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & TaxEquivRorBit)))
			continue;
		else if (i == GTWAfterTaxRor && !(zRorStruct.iReturnstoCalculate & AfterTaxRorBit))
			continue;
		else if (i == NTWAfterTaxRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & AfterTaxRorBit)))
			continue;*/

		/*
		** If the current return type we are looking at doesn't need to be calculated, obviously it doesn't
		** need to be stored. 
		*/
		if (i == GTWRor && !(zRorStruct.iReturnstoCalculate & TWRorBit)) 
			continue;
		else if (i == GPcplRor && !(zRorStruct.iReturnstoCalculate & PcplRorBit))
			continue;
		else if (i == NTWRor) {
			if (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & TWRorBit))
				continue;
			else {
				if (strcmp(sDBName, "D") == 0)
					aDate = pzPTable->pzPKey[iKeyIndex].zPK.lLastPerfDate;
				else // if changing monthly values
					aDate = pzPTable->pzPKey[iKeyIndex].zPK.lLndPerfDate;

				if (zSysSet.lCFStartDate > 0 && zPmain.iRorType == CNWRor && zSysSet.lCFStartDate > aDate)
					continue;
			}
		}
		//else if (i == CNWRor && !(zRorStruct.iReturnstoCalculate & TWNCFRorBit))
		else if (i == CNWRor) {
			if (zSysSet.lCFStartDate == -1 || !zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & TWRorBit))
				continue;
			else
			{
				if (strcmp(sDBName, "D") == 0)
					aDate = pzPTable->pzPKey[iKeyIndex].zPK.lLastPerfDate;
				else // if changing monthly values
					aDate = pzPTable->pzPKey[iKeyIndex].zPK.lLndPerfDate;

				if (zSysSet.lCFStartDate > 0 && zPmain.iRorType == NTWRor && zSysSet.lCFStartDate > aDate)
					continue;
			}
		}
		//else if (i == NPcplRor && !(zRorStruct.iReturnstoCalculate & PcplRorBit))
		else if (i == NPcplRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & PcplRorBit)))
			continue;
		else if (i == IncomeRor && !(zRorStruct.iReturnstoCalculate & IncomeRorBit))
			continue;
		else if (i == GDWRor || i == NDWRor)
			continue;
		else if (i == GTWTaxEquivRor && !(zRorStruct.iReturnstoCalculate & TaxEquivRorBit))
			continue;
		//else if (i == NTWTaxEquivRor && !(zRorStruct.iReturnstoCalculate & TaxEquivRorBit))
		else if (i == NTWTaxEquivRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & TaxEquivRorBit)))
			continue;
		else if (i == GTWAfterTaxRor && !(zRorStruct.iReturnstoCalculate & AfterTaxRorBit))
			continue;
		//else if (i == NTWAfterTaxRor && !(zRorStruct.iReturnstoCalculate & AfterTaxRorBit))
		else if (i == NTWAfterTaxRor && (!zRorStruct.bTotalPortfolio || !(zRorStruct.iReturnstoCalculate & AfterTaxRorBit)))
			continue;
		
		// Sb 5/27/15 Simplified the structure
		if (IsValueZero(pzPTable->pzPKey[iKeyIndex].zNewIndex.fBaseRorIdx[i-1]-NAVALUE,0) ||
						pzPTable->pzPKey[iKeyIndex].zNewIndex.fBaseRorIdx[i-1] < -100.00 || 
						pzPTable->pzPKey[iKeyIndex].zNewIndex.fBaseRorIdx[i-1] > +1e6)
		{
			// if no valid ROR calculated - initialize new UV index to default 100 and
			// reset Stream Begin Date also; do not calc/save UV for this date
			InitializeUnitValue(&pzPTable->pzPKey[iKeyIndex].zNewIndex.zUVIndex[i-1], TRUE, TRUE);
			continue;
		}
		
		// set ror_type and portfolio info fields
		zUnitValue.iRorType = i;
		zUnitValue.iPortfolioID = pzPTable->pzPKey[iKeyIndex].zPK.iPortfolioID;
		zUnitValue.iID = pzPTable->pzPKey[iKeyIndex].zPK.iID;

		// right now, there is no calc of Fudge factor avail in Perfomance.DLL
		// when this will be implemennted, the line below would to have be changed accordingly
		// to get correct value from passed in zRorStruct (actually, the RORSTRUCT would have 
		// to be modified too to keep RORs as array by RorTypes, not separate fields)
		//zUnitValue.fFudgeFactor = time(NULL) / 1e3;// debug only 0;
		zUnitValue.fFudgeFactor = 0;

		// default 	RorSource to Unknown -> will be changed accordingly below
		// (to Inception, Termination, Monthly or MonthToDate)
		zUnitValue.iRorSource = rsUnknown;
		// keep stream begin date (if known)
		zUnitValue.lStreamBeginDate = pzPTable->pzPKey[iKeyIndex].zBeginIndex.zUVIndex[i-1].lStreamBeginDate;
		// set beginning UV
		zUnitValue.fUnitValue = pzPTable->pzPKey[iKeyIndex].zBeginIndex.zUVIndex[i-1].fUnitValue;

		// if beginning UV has become 0 or negative for any reason, then stream can't continue
		// we must ensure new stream is starting (i.e. data hole appears)
		// (also, if BUV is extremely high and won't fit table field spec)
		if (IsValueZero(zUnitValue.fUnitValue, 7) || zUnitValue.fUnitValue<0 || zUnitValue.fUnitValue>1e15) 
		{
			InitializeUnitValue(&zUnitValue, TRUE, FALSE);
			zUnitValue.lStreamBeginDate = 0;
		}		

		// Sb 5/27/15 Simplified the structure
		// calculate ending UV
		zUnitValue.fUnitValue= RORToEndingUV(pzPTable->pzPKey[iKeyIndex].zNewIndex.fBaseRorIdx[i-1], zUnitValue.fUnitValue);

		// if current UV has become extremely high and won't fit table field spec,
		// then stream can't continue also
		// we must ensure new stream is starting (i.e. data hole appears)
		if (zUnitValue.fUnitValue>1e15) 
		{
			InitializeUnitValue(&zUnitValue, TRUE, FALSE);
			zUnitValue.lStreamBeginDate = 0;
		}		

		// if stream begin date is not carried from previous period - 
		if (zUnitValue.lStreamBeginDate == 0)
		{
			if (strcmp(pzPTable->pzPKey[iKeyIndex].zPK.sTotalRecInd, "T")==0)
			{	
				// for total portfolio check if it needs to be set either to 
				// LastPerfDate (if key continued after data hole)
				// or to InitPerfDate (if key re-incepted)
				if (pzPTable->pzPKey[iKeyIndex].zPK.lInitPerfDate <= lLastPerfDate)
					zUnitValue.lStreamBeginDate = lLndPerfDate; //lLastPerfDate;
				else
  				zUnitValue.lStreamBeginDate = pzPTable->pzPKey[iKeyIndex].zPK.lInitPerfDate;
			} 
			else
			{
				// however, for all other segments it simply becomes last non-daily date
				// or inception 
				if (lLndPerfDate!=0) 
					zUnitValue.lStreamBeginDate = lLndPerfDate;
				else
					zUnitValue.lStreamBeginDate = pzPTable->pzPKey[iKeyIndex].zPK.lInitPerfDate;
			}
		}

		if (zRorStruct.bInceptionRor)
		{
			/* 7/12/05 - keep it as TRUE inception UV, not an IPV - vay
									 this is necessary for Merge and IQ inquiries
			
			if (bInceptedNextMonth) // if stream is kept between adjacent termination & inception
				zUnitValue.iRorSource = rsInterPeriodValuation; // then this is IPV rather then truly inception ROR
			else
			*/
				zUnitValue.iRorSource = rsInception;
		}
		else
		if (zRorStruct.bTerminationRor)
			zUnitValue.iRorSource = rsTerminated;

		// if changing daily values 
		if (strcmp(sDBName, "D") == 0)
		{
			zUnitValue.lUVDate = pzPTable->pzPKey[iKeyIndex].zPK.lLastPerfDate;
			if (zUnitValue.iRorSource == rsUnknown) // if RorSource not set yet
			{
				if (lpfnIsItAMonthEnd(zUnitValue.lUVDate))
					zUnitValue.iRorSource = rsMonthly;
				else if ((zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval,"D")==0) || bPeriodEnd)
					zUnitValue.iRorSource = rsInterPeriodValuation;
				else
					zUnitValue.iRorSource = rsMonthToDate;
			}
		}
		else // if changing monthly values
		{
			zUnitValue.lUVDate = pzPTable->pzPKey[iKeyIndex].zPK.lLndPerfDate;

			if (zUnitValue.iRorSource==rsUnknown) // if RorSource not set yet
			{
				if (lpfnIsItAMonthEnd(zUnitValue.lUVDate))
					zUnitValue.iRorSource = rsMonthly;
				else
					zUnitValue.iRorSource = rsInterPeriodValuation;
			}
		}
		
		// save zUnitValue data back into global var - may be needed later (to keep StreamBeginDate)	
		// but don't link 1-day returns 
		if (!(zRorStruct.bInceptionRor && zRorStruct.bTerminationRor))
			pzPTable->pzPKey[iKeyIndex].zNewIndex.zUVIndex[i-1] = zUnitValue;
	  else 
		  pzPTable->pzPKey[iKeyIndex].zNewIndex.zUVIndex[i-1] = pzPTable->pzPKey[iKeyIndex].zBeginIndex.zUVIndex[i-1];

		// NOW all fields are set, let's save in DB
		// 1) do not save if it is Terminated same day as Incepted
		//    and even more - break the stream !

		if (zRorStruct.bInceptionRor && zRorStruct.bTerminationRor)
		{
			// 11/14/05 - we now want to save 1-day returns in the DB as un-linked UV
			zUnitValue.lStreamBeginDate = zUnitValue.lUVDate;
			zUnitValue.iRorSource = rsInterPeriodValuation;

			// pzPTable->pzPKey[iKeyIndex].zNewIndex.zUVIndex[i-1] = zUnitValue;
			//InitializeUnitValue(&pzPTable->pzPKey[iKeyIndex].zNewIndex.zUVIndex[i-1], TRUE, TRUE);
			//	continue;
		}

		// if changing daily values 
		if (strcmp(sDBName, "D") == 0)
		{
	    // If working in daily database and a scratch date is available then update record else insert it.
			if (pzPTable->pzPKey[iKeyIndex].lScratchDate != 0)
			{
				lpprUpdateUnitValue(zUnitValue, pzPTable->pzPKey[iKeyIndex].lScratchDate, &zErr);

				// if for some  reason nothing gets updated - force Insert
				if (zErr.iSqlError==DB_E_PKVIOLATION) {
					lpprDeleteDailyUnitValueForADate(zUnitValue.iPortfolioID, zUnitValue.iID, zUnitValue.lUVDate, 
																					 zUnitValue.iRorType, &zErr);
					lpprInitializeErrStruct(&zErr);
					lpprInsertUnitValueBatch(&zUnitValue, zSysSet.iInsertBatchSize, &zErr);
				}
				else if (zErr.iSqlError==SQLNOTFOUND) 
				/* 
				** SB 5/12/15 - Even though it's not directly related to VI 55741, this was discovered while testing changes
				** for this VI and this condition has been observed in the past as well. Sometimes becuase of some errors (somewhere else)
				** dsumdata exist but unitvalue doesn't, if that's the case then update unitvalue will fail. In this case 
				** insertunitvalue for the daily record
				*/
				{
					lpprInitializeErrStruct(&zErr);
					lpprInsertUnitValueBatch(&zUnitValue, zSysSet.iInsertBatchSize, &zErr);
				}
				else
					lpprInitializeErrStruct(&zErr);
			}
			else
			{
				// under "10% flow/daily perf logic"  record could have been already saved 
				// as a daily record from previous runs - delete it to avoid PK

				if (((zSysSet.fFlowThreshold != NAVALUE) && (strcmp(pzPTable->sCalcFlow, "Y") == 0)) 
							|| (zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval, "D")==0))
				{
					lpprDeleteDailyUnitValueForADate(zUnitValue.iPortfolioID, zUnitValue.iID, zUnitValue.lUVDate, 
																					 zUnitValue.iRorType, &zErr);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						break;
				}

				// but still, do not save inception/termination returns in daily DB
				// unless it is daily perf
				if ((!(zRorStruct.bInceptionRor || zRorStruct.bTerminationRor))
						|| (zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval, "D")==0))
				{
					lpprInsertUnitValueBatch(&zUnitValue, zSysSet.iInsertBatchSize, &zErr);

					// 12/12/05 vay
					// this is very weird scenario when duplicated perf keys have been created
					// I have to investigate it further but in meantime, to alleviate the problem
					// let's just delete violating records and repeat insert
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					{
						lpprDeleteDailyUnitValueForADate(zUnitValue.iPortfolioID, zUnitValue.iID, zUnitValue.lUVDate, 
																						 zUnitValue.iRorType, &zErr);
						if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
							break;

						lpprInsertUnitValueBatch(&zUnitValue, zSysSet.iInsertBatchSize, &zErr);
					}
				}
			}

			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				break;
		}
		else // if changing monthly values
		{
			// entries for Current Perf Date are being saved under daily logic 
			// (this is done to avoid an attempt to save Inception/Termination records twice)
			if 	(zUnitValue.lUVDate == lCurrPerfDate)
				continue;

			// yet another precaution - if saving Inception/Termination record,
			// make sure there is no Daily record left from previous runs
			// i.e 8/5/03 was created as daily when running for 8/5,
			// then we re-ran July to 8/6 performance and 8/5 now becomes Inception 
			// for whatever reason - insert may fail since 8/5 record already present,
			// so, it should be deleted now (this operation may actually do nothing)
			//lpfnTimer(21);
			if (zRorStruct.bInceptionRor || zRorStruct.bTerminationRor ||
					((zSysSet.fFlowThreshold != NAVALUE) && (strcmp(pzPTable->sCalcFlow, "Y") == 0))) 
			{
				lpprDeleteDailyUnitValueForADate(zUnitValue.iPortfolioID, zUnitValue.iID, zUnitValue.lUVDate, 
																				 zUnitValue.iRorType, &zErr);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					break;
			}
			//lpfnTimer(22);

			lpprInsertUnitValueBatch(&zUnitValue, zSysSet.iInsertBatchSize, &zErr);
			//lpfnTimer(23);

			//prevent overwrite of month to date return
			if (zErr.iSqlError!= 0 
					&& !lpfnIsItAMonthEnd(zUnitValue.lUVDate) 
					&& zUnitValue.iRorSource == rsInterPeriodValuation) {
					//lpprDeleteDailyUnitValueForADate(zUnitValue.iPortfolioID, zUnitValue.iID, zUnitValue.lUVDate, 
					//																 zUnitValue.iRorType, &zErr);
					lpprInitializeErrStruct(&zErr);
					//lpprInsertUnitValueBatch(&zUnitValue, zSysSet.iInsertBatchSize, &zErr);
				}
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				break;
		}	
	} // for i

 
  return zErr;
} /* insertorupdateruv */



/*
** This function is used to get all the return types 
**  which given portfolio has to calculate/save
* /
ERRSTRUCT GetAllReturnTypes(int iID, PKEYTABLE *pzPKTable)
{
  ERRSTRUCT  zErr;
  long		 iRorType;

  lpprInitializeErrStruct(&zErr);
  memset(pzPKTable->bRorType,0,sizeof(pzPKTable->bRorType));
 
  while (zErr.iSqlError == 0)
  {  
 		                                        
		lpprSelectActivePerfReturnType(&iRorType, iID, 0, 9, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		if ((iRorType<=0) || (iRorType>NUMRORTYPE_ALL))
		   continue;

		pzPKTable->bRorType[iRorType-1] = TRUE;
    

  } / * while no error * /

  return zErr;
} / * GetAllReturnTypes */
