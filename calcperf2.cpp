/**
*
* SUB-SYSTEM: pmr calcperf
*
* FILENAME: calcperf2.ec
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Shobhit Barman(Effron Enterprises, Inc.)
*
*
**/
// History
// 2023-10-13 J#PER12923 Modified logic to exclude standard FI accruals when flag is M - mk.
// 2021-07-03 J#PER11681 Changed to SYS-%d for segment codes - mk.
// 2021-06-01 J#PER11638 Broke out weighted fees out from weighted fees - mk.
// 2020-04-17 J# PER-10655 Added CN transactions -mk.
// 2019-07-30 J#PER10103 Factored in exchange rate in annual income - mk.
// 2012-08-18 VI# 49353 Zeroed out EAI for IPVs -mk
// 2010-11-19 VI# 45216 Added cost exchange rate for Book Value -mk
// 2010-07-14 VI# 44510 Improved daily deletions of unitvalues -mk
// 2010-06-30 VI# 44433 Added calculation of estimated annual income -mk
// 2010-06-10 VI# 42903 Added option calculate performance on selected rule - sergeyn
// 2007-11-08 Added Vendor id    - yb

#include "calcperf.h" 


/**
** This function tests the passed asset against the given scriptdetails and
** returns TRUE(asset passed) or FALSE(asset failed). If bSecCurr is TRUE then
** security currency of the asset is used in the test(if currency is involved)
** else Accrual currency is used.
**/
DllExport ERRSTRUCT TestAsset(PARTASSET2 zPAsset, PSCRDET *pzDetail, int iCount, BOOL bSecCurr, long lDate, 
							  PARTPMAIN zPmain, BOOL *pbResult, long lLastPerfDate, long lCurrPerfDate)
{
	ERRSTRUCT	 zErr;
	BOOL			 bThisResult, bInParenthesis, bAddResult, bNegateResult;
	int        i, iIntegerValue, iDateOffset;
	char       sAlphaValue[MAXAlphaValueLEN];
	double		 fFloatValue;
	RESULTLIST zResultList;	

	fFloatValue = 0;
	zResultList.iCapacity = 0;
	InitializeResultList(&zResultList);
	lpprInitializeErrStruct(&zErr);

	/* Start with assuming asset passes the test */
	*pbResult = TRUE;
	bAddResult = FALSE;

	iDateOffset = FindDailyInfoOffset(lDate, zPAsset, TRUE);
	if (iDateOffset < 0)
		return(lpfnPrintError("Invalid Date", 0, lDate, "O", 999, 0, 0, "CALCPERF TESTASSET", FALSE));

	for (i = 0; i < iCount; i++)
	{
		if (strcmp(pzDetail[i].sComparisonRule, "C") == 0) /* comment */
			continue;

		/*
		** if a non-zero date is passed to this function then it should be between
		** detail's start and end date.
		*/
		if (lDate != 0 &&
			(lDate < pzDetail[i].lStartDate && pzDetail[i].lStartDate != 0) ||
			(lDate > pzDetail[i].lEndDate && pzDetail[i].lStartDate != 0))
			continue;

		if (strcmp(pzDetail[i].sSelectType, "STARTP") == 0) // Start Parenthesis
		{
			if (bAddResult) // if previous result has not been added
			{
				zErr = AddAnItemToResultList(&zResultList, BoolToStr(*pbResult));
				if (zErr.iBusinessError == 0 && zErr.iSqlError == 0)
					zErr = AddAnItemToResultList(&zResultList, pzDetail[i].sAndOrLogic);
			}
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			{
				InitializeResultList(&zResultList);
				return zErr;
			}

			if (strcmp(pzDetail[i].sIncludeExclude, "E") == 0)
				bNegateResult = TRUE;
			else
				bNegateResult = FALSE;

			bInParenthesis = TRUE;
			*pbResult = TRUE; // start the next checks within parenthesis with TRUE
			continue;
		} // if Start Parenthesis
		else if (strcmp(pzDetail[i].sSelectType, "ENDP") == 0) // End Parenthesis
		{
			if (bNegateResult)
				*pbResult = !*pbResult;

			zErr = AddAnItemToResultList(&zResultList, BoolToStr(*pbResult));
			// if there is more 
			if (zErr.iBusinessError == 0 && zErr.iSqlError == 0 && i < iCount - 1)
				zErr = AddAnItemToResultList(&zResultList, pzDetail[i].sAndOrLogic);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			{
				InitializeResultList(&zResultList);
				return zErr;
			}

			bNegateResult = FALSE;
			bInParenthesis = FALSE;
			*pbResult = TRUE; // start the next checks within parenthesis with TRUE
			bAddResult = FALSE;
			continue;
		} // if Start Parenthesis

		zErr = GetSelectTypeValueForAnAsset(zPAsset, iDateOffset, pzDetail[i].sSelectType, bSecCurr, 
											sAlphaValue, &fFloatValue, &iIntegerValue);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		if (strcmp(pzDetail[i].sComparisonRule, "M") == 0 || strcmp(pzDetail[i].sComparisonRule, "R") == 0)
		{
			zErr = TestAlphaValue(pzDetail[i].sBeginPoint, pzDetail[i].sEndPoint, pzDetail[i].sComparisonRule,
								  pzDetail[i].sMatchExpand, pzDetail[i].sMatchWild,
								  pzDetail[i].sMatchRest, sAlphaValue, &bThisResult);
			if (zErr.iBusinessError != 0)
				return zErr;
		} /* if match or range */
		else if (strcmp(pzDetail[i].sComparisonRule, "DU") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "DL") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "DI") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "DE") == 0)
		{
			zErr = TestDoubleValue(pzDetail[i].sBeginPoint, pzDetail[i].sEndPoint,
								   pzDetail[i].sComparisonRule, fFloatValue, &bThisResult);
			if (zErr.iBusinessError != 0)
				return zErr;
		} /* if double range */
		else if (strcmp(pzDetail[i].sComparisonRule, "IU") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "IL") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "II") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "IE") == 0)
		{
			zErr = TestIntegerValue(pzDetail[i].sBeginPoint, pzDetail[i].sEndPoint,
									pzDetail[i].sComparisonRule, iIntegerValue, &bThisResult);
			if (zErr.iBusinessError != 0)
				return zErr;
		} /* if integer range */
		else if (strcmp(pzDetail[i].sComparisonRule, "FE") == 0 ||
				 strcmp(pzDetail[i].sComparisonRule, "FN") == 0)
		{
			zErr = TestFieldValue(pzDetail[i].sBeginPoint, pzDetail[i].sEndPoint,
								  pzDetail[i].sComparisonRule, sAlphaValue, zPmain, &bThisResult);
			if (zErr.iBusinessError != 0)
				return zErr;
		} /* compare fields */
		else if (strcmp(pzDetail[i].sComparisonRule, "S") == 0)
		{
			zErr = TestSpecialValue(pzDetail[i].sBeginPoint, iIntegerValue, &bThisResult);
			if (zErr.iBusinessError != 0)
				return zErr;
		}
		else
			return(lpfnPrintError("Invalid Comparison Rule",0, pzDetail[i].lScrhdrNo,
								  "S", 506, 0, 0, "CALCPERF TESTASSET1", FALSE));

		if (strcmp(pzDetail[i].sAndOrLogic, "A") == 0)
		{
			if (strcmp(pzDetail[i].sIncludeExclude, "I") == 0)
				*pbResult = *pbResult && bThisResult;
			else if (strcmp(pzDetail[i].sIncludeExclude, "E") == 0)
				*pbResult = *pbResult && !bThisResult;
			else
				return(lpfnPrintError("Invalid Include/Exclude Flag", 0, pzDetail[i].lScrhdrNo, 
									  "S", 507, 0, 0, "CALCPERF TESTASSET2", FALSE));
		} /* if andorlogic = 'A' */
		else if (strcmp(pzDetail[i].sAndOrLogic, "O") == 0)
		{
			if (strcmp(pzDetail[i].sIncludeExclude, "I") == 0)
				*pbResult = *pbResult || bThisResult;
			else if (strcmp(pzDetail[i].sIncludeExclude, "E") == 0)
				*pbResult = *pbResult || !bThisResult;
			else
				return(lpfnPrintError("Invalid Include/Exclude Flag", 0, pzDetail[i].lScrhdrNo, 
									  "S", 507, 0, 0, "CALCPERF TESTASSET2", FALSE));
		} /* if andorlogic = 'O' */
		else
			return(lpfnPrintError("Invalid AndOrLogic Flag", 0, pzDetail[i].lScrhdrNo,
								  "S", 508, 0, 0, "CALCPERF TESTASSET2", FALSE));
		bAddResult = TRUE;
	} /* for i < iCount */

	if (zResultList.iCount > 0)
	{
		zErr = ResolveResultList(zResultList, pbResult);
		InitializeResultList(&zResultList);
	}

	return zErr;
} /* TestAsset */


/**
** Using the give comparison rule (M - match low value or R - should be between
** low and high values), given string is tested to see if it passes or fails.
** Low and high values can have wild card character(matchwild) and then match
** expand(Y - yes expand or ' ' - blank, don't expand) will decide what to do
** with it. If the passed string is longer than low/high values then match rest
** decides what to do with it. This function assumes that MatchExpand and Match
** Rest have a valid value(Y or ' ').
*/
ERRSTRUCT TestAlphaValue(char *sLowVal, char *sHighVal, char *sComparisonRule, char *sMatchExpand, 
						 char *sMatchWild, char *sMatchRest, char *sTestString, BOOL *bResult)
{
	ERRSTRUCT zErr;

	lpprInitializeErrStruct(&zErr);

	if (strcmp(sComparisonRule, "M") == 0)
	{
		/*
		** Continue as long as Result is TRUE and either Test string or Low Value(
		** or both) has not reached its end.
		*/
		*bResult = TRUE;
		while (*bResult && (*sLowVal != '\0' || *sTestString != '\0'))
		{
			/*
			** There are three possibility, first, lowval string has reached its end,
			** it means that teststring is longer, in that case if MatchRest is blank,
			** no match else there is a match on string. In second case, teststring
			** has reached its end, meaning lowval is longer, in that case if current
			** character in lowval is the wildcard character, there might be a match,
			** elso no match. In the third case neither of the string has reached
			** their end, in that case if the current characters in both of the string
			** match then there might be a match, else if current character in the
			** LowVal is a wild card and expandwild is set to be 'Y', there might be
			** a match, else no match.
			*/
			if (*sLowVal == '\0')
			{
				if (sMatchRest[0] == 'N')
					*bResult = FALSE;
				else
					break; /* strings match */
			}
			else if (*sTestString == '\0')
			{
				if (*sLowVal != sMatchWild[0])
					*bResult = FALSE;
			}
			else
			{
				if (*sLowVal != *sTestString)
				{
					if (*sLowVal != sMatchWild[0] || sMatchExpand[0] != 'Y')
						*bResult = FALSE;
				}
			}

			if (*sLowVal != '\0')
				sLowVal++;
			if (*sTestString != '\0')
				sTestString++;
		} /* while result is true and either of the string has not reached its end*/
	} /* match - comparisonrule = 'M' */
	else if (strcmp(sComparisonRule, "R") == 0)
	{
		/*
		** Continue as long as Result is TRUE and any one of the strings has not
		** reached its end.
		*/
		*bResult = TRUE;
		while (bResult && (*sLowVal != '\0' || *sHighVal != '\0' ||
				*sTestString != '\0'))
		{
			/*
			** There are three possibility, first, lowval string has reached its end,
			** it means that teststring is longer, in that case if MatchRest is blank,
			** no match else there is a match on string. In second case, teststring
			** has reached its end, meaning lowval is longer, in that case if current
			** character in lowval is the wildcard character, there might be a match,
			** elso no match. In the third case neither of the string has reached
			** their end, in that case if the current characters in both of the string
			** match then there might be a match, else if current character in the
			** LowVal is a wild card and expandwild is set to be 'Y', there might be
			** a match, else no match.
			*/
			if (*sLowVal == '\0')
			{
				if (*sHighVal == '\0')
				{
					/*
					** if Both lowval and highval have ended, teststring can not end,
					** if matchrest is blank then the string does not match else it does
					*/
					if (sMatchRest[0] == 'N')
						*bResult = FALSE;
					else
						break;
				} /* if highval also ended */
				else
				{
					if (*sTestString == '\0')
					{
						/*
						** LowVal and TestStrings have ended, HighVal has not, only way the
						** string can match is if all the remaining characters in highval
						** are wild card characters.
						*/
						if (*sHighVal != sMatchWild[0])
							*bResult = FALSE;
					} /* if teststring ended */
					else
					{
						/*
						** Only LowVal has ended, test string can match if it is less than
						** or equal to HighVal or HighVal is a wild card character and
						** matchexpand is set to be 'Y'
						*/
						if (*sHighVal < *sTestString)
						{
							if (*sHighVal != sMatchWild[0] || sMatchExpand[0] != 'Y')
								*bResult = FALSE;
						}
					} /* if test string has not ended */
				} /* if highval has not ended */
			} /* if lowval ended */
			else if (*sHighVal == '\0')
			{
				if (*sLowVal == '\0')
				{
					/*
					** if Both lowval and highval have ended, teststring can not end,
					** if matchrest is blank then the string does not match else it does
					*/
					if (sMatchRest[0] == 'N')
						*bResult = FALSE;
					else
						break;
				} /* if Lowval also ended */
				else
				{
					if (*sTestString == '\0')
					{
						/*
						** HighVal and TestStrings have ended, HighVal has not, only way
						** the string can match is if all the remaining characters in
						** lowval are wild card characters.
						*/
						if (*sLowVal != sMatchWild[0])
							*bResult = FALSE;
					} /* if teststring ended */
					else
					{
						/*
						** Only HighVal has ended, test string can match if it is greater
						** that or equal to LowVal or LowVal is a wild card character and
						** matchexpand is set to be 'Y'
						*/
						if (*sLowVal > *sTestString)
						{
							if (*sLowVal != sMatchWild[0] || sMatchExpand[0] != 'Y')
								*bResult = FALSE;
						}
					} /* if test string has not ended */
				} /* if Lowval has not ended */
			} /* if highval ended */
			else if (*sTestString == '\0')
			{
				if (*sLowVal != '\0' && *sLowVal != sMatchWild[0])
					*bResult = FALSE;

				if (*sHighVal != '\0' && *sHighVal != sMatchWild[0])
					*bResult = FALSE;
			} /* test string has ended */
			else
			{
				/*
				** If lowval is greater than teststring or highval is less than the
				** teststring then the string can match if lowval is the wild card
				** character and maskwild is 'Y'(low > teststring) or if highval is
				** the wild card character and maskwild is 'Y'(highval < teststring)
				*/
				if (*sLowVal > *sTestString || *sHighVal < *sTestString)
				{
					if (*sLowVal > *sTestString &&
						(*sLowVal != sMatchWild[0] || sMatchExpand[0] != 'Y'))
						*bResult = TRUE;
					else if (*sHighVal > *sTestString &&
						(*sHighVal != sMatchWild[0] || sMatchExpand[0] != 'Y'))
						*bResult = TRUE;
				} /* if teststring not between low and high val range */
			} /* none of the string has ended */

			if (*sLowVal != '\0')
				sLowVal++;
			if (*sHighVal != '\0')
				sHighVal++;
			if (*sTestString != '\0')
				sTestString++;
		} /*while result is true and none of the strings has not reached their end*/
	} /* range - comparisonrule = 'R' */
	else
		zErr = lpfnPrintError("Invalid Comparison Rule", 0, 0, "", 506, 0, 0,
							  "CALCPERF TESTALPHA", FALSE);

	return zErr;
} /* TestAlphavalue */


/**
** This function does a numeric comparison on the passed double value,
** according to comparison rule. Note that the low value and high value are
** passed to this function as strings(that's how they are stored in the DB) and
** it is this function's responsibility to convert them into double.
**/
ERRSTRUCT TestDoubleValue(char *sLowStr, char *sHighStr, char *sComparisonRule,
						  double fTestVal, BOOL *bResult)
{
	ERRSTRUCT zErr;
	double    fLowVal, fHighVal;

	lpprInitializeErrStruct(&zErr);

	fLowVal = atof(sLowStr);
	fHighVal = atof(sHighStr);

	if (strcmp(sComparisonRule, "DL") == 0) // double compare - include lower bound
	{
		if (fLowVal <= fTestVal &&  fTestVal < fHighVal)
			*bResult = TRUE; 
		else
			*bResult = FALSE;
	}
	else if (strcmp(sComparisonRule, "DU") == 0) // double compare - include upper bound
	{
		if (fLowVal < fTestVal && fTestVal <= fHighVal)
			*bResult = TRUE;
		else
			*bResult = FALSE;
	}
	else if (strcmp(sComparisonRule, "DI") == 0) // double compare - include both upper and lower bounds
	{
		if (fLowVal <= fTestVal && fTestVal <= fHighVal)
			*bResult = TRUE;
		else
			*bResult = FALSE;
	}
	else if (strcmp(sComparisonRule, "DE") == 0) // double compare - exclude both lower and upper bounds
	{
		if (fLowVal < fTestVal && fTestVal < fHighVal)
			*bResult = TRUE;
		else
			*bResult = FALSE;
	}
	else
		zErr = lpfnPrintError("Invalid Comparison Rule", 0, 0, "", 506, 0, 0, "CALCPERF TESTFOUBLE1", FALSE);

	return zErr;
} /* Testdoublevalue */


/**
** This function does a numeric comparison on the passed integer value,
** according to comparison rule. Note that the low value and high value are
** passed to this function as strings(that's how they are stored in the DB) and
** it is this function's responsibility to convert them into integer.
**/
ERRSTRUCT TestIntegerValue(char *sLowStr, char *sHighStr, char *sComparisonRule,
						   int iTestVal, BOOL *bResult)
{
	ERRSTRUCT zErr;
	int				iLowVal, iHighVal;

	lpprInitializeErrStruct(&zErr);

	iLowVal = atoi(sLowStr);
	/*return(lpfnPrintError("Invalid Low Value String", 0, 0, "", 998, 0, 0,
	"CALCPERF TESTINTEGER1", FALSE));*/

	iHighVal = atoi(sHighStr);
	/*return(lpfnPrintError("Invalid High Value String", 0, 0, "", 998, 0, 0,
	"CALCPERF TESTINTEGER2", FALSE));*/

	if (strcmp(sComparisonRule, "IL") == 0) // integer compare - include lower bound
	{
		if (iLowVal <= iTestVal &&  iTestVal < iHighVal)
			*bResult = TRUE; 
		else
			*bResult = FALSE;
	}
	else if (strcmp(sComparisonRule, "IU") == 0) // integer compare - include upper bound
	{
		if (iLowVal < iTestVal && iTestVal <= iHighVal)
			*bResult = TRUE;
		else
			*bResult = FALSE;
	}
	else if (strcmp(sComparisonRule, "II") == 0) // integer compare - include both upper and lower bounds
	{
		if (iLowVal <= iTestVal && iTestVal <= iHighVal)
			*bResult = TRUE;
		else
			*bResult = FALSE;
	}
	else if (strcmp(sComparisonRule, "IE") == 0) // integer compare - exclude both lower and upper bounds
	{
		if (iLowVal < iTestVal && iTestVal < iHighVal)
			*bResult = TRUE;
		else
			*bResult = FALSE;
	}
	else
		zErr = lpfnPrintError("Invalid Comparison Rule", 0, 0, "", 506, 0, 0, "CALCPERF TESTINTEGER3", FALSE);

	return zErr;
} /* Testintegervalue */


/*
** This function does a string comparison of two fields for equality or non-equality. Right now (11/26/1999)
** the only field this function knows to compare is basecurrid from portmain table. More fields
** from other tables can be added later when required.
*/ 
ERRSTRUCT TestFieldValue(char *sTableName, char *sFieldName, char *sComparisonRule, 
						 char *sTestValue, PARTPMAIN zPmain, BOOL *pbResult)
{
	ERRSTRUCT zErr;
	char			sFieldValue[20];

	lpprInitializeErrStruct(&zErr);

	if (_stricmp(sTableName, "portmain") != 0)
		return(lpfnPrintError("Invalid Table Name For Field Comparison", 0, 0, "", 519, 0, 0, "CALCPERF TESTFIELD1", FALSE));

	if (_stricmp(sFieldName, "basecurrid") != 0)
		return(lpfnPrintError("Invalid Field Name For Field Comparison", 0, 0, "", 520, 0, 0, "CALCPERF TESTFIELD2", FALSE));

	strcpy_s(sFieldValue, zPmain.sBaseCurrId);

	if (strcmp(sComparisonRule, "FE") == 0) // field equal
	{
		if (strcmp(sTestValue, sFieldValue) == 0)
			*pbResult = TRUE;
		else
			*pbResult = FALSE;
	}
	else
	{
		if (strcmp(sTestValue, sFieldValue) == 0)
			*pbResult = FALSE;
		else
			*pbResult = TRUE;
	} // FN - field not equal

	return zErr;
} // TestFieldValue


/**
** This function does a bitwise comparison on the passed integer value. Note
** that the low value is passed to this function as strings(that's how they are
** stored in the DB) and it is this function's responsibility to convert them
** into appropriate bit value.
**/
ERRSTRUCT TestSpecialValue(char *sBeginVal, int iTestInt, BOOL *pbResult)
{
	ERRSTRUCT zErr;
	int       iBeginVal;

	lpprInitializeErrStruct(&zErr);

	if (strcmp(sBeginVal, "LONG") == 0)
		iBeginVal = SRESULT_LONG | ARESULT_LONG;
	else if (strcmp(sBeginVal, "SHORT") == 0)
		iBeginVal = SRESULT_SHORT | ARESULT_SHORT;
	else
		return(lpfnPrintError("Invalid Low Value String", 0, 0, "", 998, 0, 0,
							  "CALCPERF TESTSPECIAL", FALSE));

	if ((iBeginVal & iTestInt) == 0)
		*pbResult = FALSE;
	else
		*pbResult = TRUE;

	return zErr;
} /* TestSpecialValue */


DllExport ERRSTRUCT FindCreateScriptHeader(int iID, long lCurrentPerfDate, ASSETTABLE2 zATable, 
										   PSCRHDRDETTABLE *pzSHdrDetTable, PSCRHDRDET *pzVirtualSHD, 
										   BOOL bSingleSecurity, long lTmphdrNo, int iAssetIndex, int *iSegmentIndex)
{
	ERRSTRUCT     zErr;
	int						iIndex, n;
	BOOL					bUpdateHdrKey;

	lpprInitializeErrStruct(&zErr);
	bUpdateHdrKey = FALSE;
	if (iSegmentIndex)
		*iSegmentIndex = 0;

	/* create a hashkey for the virtual script using SHA1*/
	pzVirtualSHD->zHeader.lHashKey = CreateHashkeyForScript(pzVirtualSHD, FALSE);

	/*
	** Check if an actual script exits(similar to virtual script). If it
	** does not already exist, add the virtual script to the memory table
	** as well as the database. If it does exist, update scrhdr_no in header
	** as well as all the detail records of virtual perfscript.
	*/
	iIndex = FindScrHdrByHashkeyAndTmpNo(*pzSHdrDetTable, -1, lTmphdrNo, pzVirtualSHD->zHeader.sHdrKey, 
										 *pzVirtualSHD);

	/*
	** The script header and detail memory table is filled with all the script header and details
	** first time InitializeCalcPerfLibrary is called. After that if program creates a new script
	** header/detail it adds it to memory table as well. However, in nightly batch mode, if needed,
	** multiple copies of the program are simultaneously running and a copy of the program is not
	** aware of new script header/detail created by another copy. If the first copy of the program 
	** also needs to create a sccript header/detail (either same or different) then it fails becuase
	** of primary key violation on script header number. So, if the script we are looking for is
	** not found in the memory table, get new script header/details that may have been added to the
	** database since the table was first filled and add them to the memory table. After adding, 
	** search once more for the script we are looking for
	*/
	if (iIndex < 0)
	{
		zErr = FillScriptHeaderDetailTable(pzSHdrDetTable, TRUE);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		iIndex = FindScrHdrByHashkeyAndTmpNo(*pzSHdrDetTable, -1, lTmphdrNo, pzVirtualSHD->zHeader.sHdrKey,
											 *pzVirtualSHD);
	} // if not found the first time

	// if Index not found using SHA1 key, generate key using internal algorithm
	// this should happen only 1st time when new SHA keys are introduced
	if (iIndex < 0)
	{
		pzVirtualSHD->zHeader.lHashKey = CreateHashkeyForScript(pzVirtualSHD, TRUE);
		iIndex = FindScrHdrByHashkeyAndTmpNo(*pzSHdrDetTable, pzVirtualSHD->zHeader.lHashKey, lTmphdrNo,"",
											 *pzVirtualSHD); 
		// if script found in memory by old hashkey - it is time to update it to use SHA key next time
		// this will only happen one time
		bUpdateHdrKey = (iIndex >= 0);
	}

	// if all previous ways for looking for the script in memory table fail, create a new script
	if (iIndex < 0)
	{
		pzVirtualSHD->zHeader.lCreateDate = lCurrentPerfDate;
		/* createdby needs the userid but for the time being copy portfolio id */
		sprintf_s(pzVirtualSHD->zHeader.sCreatedBy, "%d", iID);
		/* regenrate the hashkey for the virtual script using SHA1, last value in it was using internal algorithm */
		pzVirtualSHD->zHeader.lHashKey = CreateHashkeyForScript(pzVirtualSHD, FALSE);

		zErr = CreateNewScript(pzVirtualSHD, pzSHdrDetTable, bSingleSecurity, zATable.pzAsset[iAssetIndex].sSecNo, 
			zATable.pzAsset[iAssetIndex].sWhenIssue, zATable.pzAsset[iAssetIndex].sSecDesc1);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		if (iSegmentIndex)
			*iSegmentIndex = pzSHdrDetTable->iNumSHdrDet - 1;	
	} /* if script not found in memory */
	else
	{
		if (iSegmentIndex)
			*iSegmentIndex = iIndex;

		pzVirtualSHD->zHeader.lScrhdrNo = pzSHdrDetTable->pzSHdrDet[iIndex].zHeader.lScrhdrNo;
		pzVirtualSHD->zHeader.iSegmentTypeID = pzSHdrDetTable->pzSHdrDet[iIndex].zHeader.iSegmentTypeID;

		if (bUpdateHdrKey)
		{
			// put newly generated hdr_key into memory record 
			strcpy_s(pzSHdrDetTable->pzSHdrDet[iIndex].zHeader.sHdrKey, pzVirtualSHD->zHeader.sHdrKey);
			// and save it in DB as well
			pzVirtualSHD->zHeader.lChangeDate = lCurrentPerfDate;
			sprintf_s(pzVirtualSHD->zHeader.sChangedBy, "%d", iID);
			lpprUpdatePerfscriptHeader(pzVirtualSHD->zHeader, &zErr);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
		}

		for (n = 0; n < pzVirtualSHD->iNumDetail; n++)
			pzVirtualSHD->pzDetail[n].lScrhdrNo = pzSHdrDetTable->pzSHdrDet[iIndex].zHeader.lScrhdrNo;
	} /* script found in the memory table */

	return zErr;
}

BOOL LoadPerfRule(int iID, long lPerfDate, PERFRULETABLE *pzRuleTable)
{
	ERRSTRUCT  zErr;
	int        i;
	int lp;
	PERFRULE	 zPerfRule;
	BOOL CalcSelected; 
	lpprInitializeErrStruct(&zErr);

	//lpfnTimer(30);
	CalcSelected = false;
	InitializePerfruleTable(pzRuleTable);
	while (!zErr.iSqlError)
	{
		lpprInitializeErrStruct(&zErr);
		InitializePerfrule(&zPerfRule);

		lpprSelectAllPerfrule (&zPerfRule, iID, lPerfDate, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return CalcSelected;
		else
		{
			CalcSelected = CalcSelected || zPerfRule.bCalculate;

			zErr = AddPerfruleToTable(pzRuleTable, zPerfRule, zTHdrDetTable);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				return CalcSelected;
		}
	}  /* While no error */
	//lpfnTimer(31);

	if (CalcSelected) 
	{
		lp=0;
		for (i=0; i<pzRuleTable->iCount; i++)
		{
			if (pzRuleTable->pzPRule[i].bCalculate)
			{
				pzRuleTable->pzPRule[lp] = pzRuleTable->pzPRule[i];
				pzRuleTable->piTHDIndex[lp] = pzRuleTable->piTHDIndex[i];
				lp++;
			}
			else
				memset(&pzRuleTable->pzPRule[i], 0, sizeof(PERFRULE));
		}
		pzRuleTable->iCount = lp;
	}
	return CalcSelected;
}

void DeletePerfkeyByPerfRule(PERFRULETABLE *pzRuleTable, PKEYTABLE *pzPTable)
{
	int i, j, lp;
	lp = 0;
	for (i = 0; i < pzPTable->iCount; i++)
	{
		j = FindRule(*pzRuleTable, pzPTable->pzPKey[i].zPK.lRuleNo);
		if (j != -1)
		{
			pzPTable->pzPKey[lp] = pzPTable->pzPKey[i];
			lp++;
		}
		else
			memset(&pzPTable->pzPKey[i], 0, sizeof(PERFKEY));
	}
	pzPTable->iCount = lp;
}

/**
** This function is used to create all the dynamically generated(according to
** perfrule and templates) perfkeys for an account.
** 6/26/98 SB : This function is also used to identify whether an existing key
** is for Weigthed Average Performance or not.
**/
ERRSTRUCT CreateDynamicPerfkeys(PKEYTABLE *pzPTable, PARTPMAIN zPmain, long lLastPerfDate, long lCurrentPerfDate, ASSETTABLE2 zATable, 
								PERFRULETABLE *pzRuleTable, PERFASSETMERGETABLE zPAMTable, long lInitPerfDate, BOOL bOnlyCreateScripts)
{
	ERRSTRUCT	zErr;
	int			i, j, k, l, m;
	PSCRHDRDET	zVirtualSHD;
	BOOL		bResult, bSecCurr;
	BOOL		bSingleSecurity;

	lpprInitializeErrStruct(&zErr);

	//lpfnTimer(30);

	/* if no rules are defined for the account, no dynamic key is generated */
	if (pzRuleTable->iCount == 0)
		return zErr;

	/*
	** Go through the perfkey table(which has existing keys) and for each key
	** try to find the perfrule which created that key in the perfrule table, if
	** found and its WtdRecInd is 'W' then the key is a Weighted Key.
	*/
	for (i = 0; i < pzPTable->iCount; i++)
	{
		j = FindRule(*pzRuleTable, pzPTable->pzPKey[i].zPK.lRuleNo);

		if (j != -1)
			strcpy_s(pzPTable->pzPKey[i].sRecordType, pzRuleTable->pzPRule[j].sWeightedRecordIndicator);
		else
			strcpy_s(pzPTable->pzPKey[i].sRecordType, "");       

		/*
		** If it is a child and the rule which created it not found, set its parent
		** rule to zero, else set its parent rule to be equal to the parent rule
		** of the rule which created this(child) key.
		*/
		if (strcmp(pzPTable->pzPKey[i].zPK.sParentChildInd, "C") == 0)
		{
			if (j == -1)
				pzPTable->pzPKey[i].lParentRuleNo = 0;
			else
				pzPTable->pzPKey[i].lParentRuleNo = pzRuleTable->pzPRule[j].lParentRuleNo;
		}
		else
			pzPTable->pzPKey[i].lParentRuleNo = pzPTable->pzPKey[i].zPK.lRuleNo;
	} /* for on i */

	/*
	** Examine all rules defined for the account against all the asset and for
	** each rule-asset combination check if a perfkey can be generated. If a
	** perfkey can be generated, check if it already exist, if it does not then
	** create it(both in memory and in the database).
	*/
	zVirtualSHD.iDetailCreated = 0;
	for (i = 0; i < pzRuleTable->iCount; i++)
	{
		/*
		** If the rule is for Weighted Average, key will be created based on its
		** children, not on its own criterion, so ignore the rule.
		*/
		if (strcmp(pzRuleTable->pzPRule[i].sWeightedRecordIndicator, "W") == 0)
			continue;

		m = pzRuleTable->piTHDIndex[i];
		for (j = 0; j < zATable.iNumAsset; j++)
		{
			// Go through all the days for this asset. 
			for (k = 0; k < zATable.pzAsset[j].iDailyCount; k++)
			{
				// SB - 5/14/09  if there is atleast another daily item in the array and the next date is last than or equal to 
				// lastperfdate then no need to look at current item. This is done becuase as far as the performance is
				// concerned, all the changes in asset except the last one on or prior to lastperfdate are useless
				if (k < zATable.pzAsset[j].iDailyCount - 1 && zATable.pzAsset[j].pzDAInfo[k+1].lDate <= lLastPerfDate)
					continue;

				/*
				** Each asset has two currencies defined, security currency and accrual
				** currency, both of these can generate diferent perfkeys(if we detail has
				** defined a selecttype for currency). In most of the cases both these
				** currencies will be same, do the whole process twice(with different
				** currencies of course) only if these two currencies are different.
				*/
				for (l = 0; l < 2; l++)
				{
					if (l == 0)
						bSecCurr = TRUE;
					else
					{
						bSecCurr = FALSE;
						/* both the currencies are same, break out of the "for l" loop */
						if (strcmp(zATable.pzAsset[j].sCurrId, zATable.pzAsset[j].sIncCurrId) == 0)
							break;
					} /* second pass */

					/* create a virtual perf script using current template and asset */
					zErr = CreateVirtualPerfScript(zTHdrDetTable.pzTHdrDet[m], zATable.pzAsset[j], k, bSecCurr, &zVirtualSHD);
					if (zErr.iBusinessError != 0)
					{
						InitializePScrHdrDet(&zVirtualSHD);
						return zErr;
					}

					//lpfnTimer(32);
					/*
					** Test the asset against virtual perf script. 
					** If it is a special rule(equity+equity cash or fixed+fixed cash), create a key
					** only if portfolio has Equity or Fixed, i.e. if the portfolio has fixed and cash
					** but no equity, only fixed + fixed cash key will be generated for this rule.
					*/
					if (strcmp(pzRuleTable->pzPRule[i].sWeightedRecordIndicator, "E") == 0 ||
						strcmp(pzRuleTable->pzPRule[i].sWeightedRecordIndicator, "F") == 0)
						zErr = TestAsset(zATable.pzAsset[j], zVirtualSHD.pzDetail, 1, bSecCurr, zATable.pzAsset[j].pzDAInfo[k].lDate, 
										 zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
					else
						zErr = TestAsset(zATable.pzAsset[j], zVirtualSHD.pzDetail, zVirtualSHD.iNumDetail, bSecCurr, 
										 zATable.pzAsset[j].pzDAInfo[k].lDate, zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
					if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
					{
						InitializePScrHdrDet(&zVirtualSHD);
						return zErr;
					}
					//lpfnTimer(33);

					/*
					** If the asset failed the virtual script test, nothing more to do with
					** the current perfrule, asset(and currency) combination.
					*/
					if (bResult == FALSE)
						continue;

					/* Asset passed the test, find a hashkey for the virtual script (or create if neccessary)*/
					bSingleSecurity = (strcmp(pzRuleTable->pzPRule[i].sWeightedRecordIndicator, "S") == 0);
					zErr = FindCreateScriptHeader(zPmain.iID, lCurrentPerfDate, zATable, &g_zSHdrDetTable, &zVirtualSHD, 
												  bSingleSecurity, zTHdrDetTable.pzTHdrDet[m].zHeader.lTmphdrNo, j, NULL);
					if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
					{
						InitializePScrHdrDet(&zVirtualSHD);
						return zErr;
					}	

					/*
					** Search for the possible perfkey this rule can generate, if not found
					** in the perfkey table in memory, add it
					*/
					if (!bOnlyCreateScripts && 
						FindPerfkeyByRule(*pzPTable, pzRuleTable->pzPRule[i].lRuleNo, zVirtualSHD.zHeader.lScrhdrNo, 0) < 0)
					{
						zErr = CreateNewPerfkey(pzPTable, pzRuleTable->pzPRule[i], zVirtualSHD.zHeader.lScrhdrNo, 
												zVirtualSHD.zHeader.iSegmentTypeID, lCurrentPerfDate);
						if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						{
							InitializePScrHdrDet(&zVirtualSHD);
							return zErr;
						}
					} /* if key not found */
				} /* for l < 2 */
			} /* for k < iEndIndex */
		} /* j < numasset */
	} /* i < numprule */

	/* Free up the memory */
	InitializePScrHdrDet(&zVirtualSHD);

	return zErr;
} /* CreateDynamicPerfkeys */


/**
** Function to create new Weighted Parent Perfkeys. This function is called
** after perfrule table has already been filled(in CreateDynamicPerfkeys) and
** the performance on children has already been calculated. Weighted Parent Keys
** are not created using their own scripts, they are created and deleted based
** on when their children are created and deleted. To create a weighted parent
** key first find an asset which passes (one of the) children rule. Use that
** asset and Parent Script to create a virtual script. Use that script(without
** checking whether parent passes that script or not) to create new key.
** /
ERRSTRUCT PerfkeyForWeightedParent(PKEYTABLE *pzPTable, PKEYASSETTABLE2 zPATable,
ASSETTABLE2 zATable, PERFRULETABLE zRTable,
long lParentRuleNo, long lKeyStartDate, long lKeyEndDate)
{
ERRSTRUCT  zErr;
int        i, j, iChild, iAsset, iRule;
PSCRHDRDET zVSHD;
BOOL       bSecCurr;

lpprInitializeErrStruct(&zErr);

/ * First, find a child for the current parent rule * /
iChild = -1;
for (i = 0; i < pzPTable->iCount; i++)
{
/ * If key is deleted from db but still in memory table, ignore it * /
if (pzPTable->pzPKey[i].bDeletedFromDB == TRUE)
continue;

if (strcmp(pzPTable->pzPKey[i].zPK.sParentChildInd, "P") == 0 ||
pzPTable->pzPKey[i].lParentRuleNo != lParentRuleNo)
continue;

if ((lKeyEndDate == 0 || pzPTable->pzPKey[i].zPK.lInitPerfDate <= lKeyEndDate)
&& (pzPTable->pzPKey[i].zPK.lDeleteDate == 0 ||
pzPTable->pzPKey[i].zPK.lDeleteDate >= lKeyStartDate))
{
iChild = i;
break;
}
} / * Find a child * /

if (iChild == -1)
return(lpfnPrintError("Programming Error", pzPTable->pzPKey[0].zPK.iID, 0, "",
999, 0, 0, "CALCPERF WTDPARENTKEY1", FALSE));

/ * Now find an asset which passes for the current perfkey * /
iAsset = -1;
for (i = 0; i < zPATable.iNumAsset; i++)
{
j = iChild * zPATable.iNumAsset + i;
if (zPATable.pzStatusFlag[j].piSResult[1] > 0)
{
bSecCurr = TRUE;
iAsset = i;
break;
}
else if (zPATable.pzStatusFlag[j].piAResult[1] > 0)
{
bSecCurr = FALSE;
iAsset = i;
break;
}
} / * Find an asset matching first found child * /

if (iAsset == -1)
return(lpfnPrintError("Programming Error", pzPTable->pzPKey[0].zPK.iID, 0, "",
999, 0, 0, "CALCPERF WTDPARENTKEY2", FALSE));

/ * Now find the parent rule in the rule table * /
iRule = -1;
for (i = 0; i < zRTable.iCount; i++)
{
if (zRTable.pzPRule[i].lRuleNo == lParentRuleNo)
{
iRule = i;
break;
}
} / * Find the parent rule in the rule table * /

if (iRule == -1)
return(lpfnPrintError("Programming Error", pzPTable->pzPKey[0].zPK.iID, 0, "",
999, 0, 0, "CALCPERF WTDPARENTKEY3", FALSE));

j = zRTable.piTHDIndex[iRule];
/ * create a virtual perf script using current template and asset * /
zErr = CreateVirtualPerfScript(zTHdrDetTable.pzTHdrDet[j],
zATable.pzAsset[iAsset], bSecCurr, &zVSHD);
if (zErr.iBusinessError != 0)
{
InitializePScrHdrDet(&zVSHD);
return zErr;
}

/ * find existing script header or create a new one for the virtual script * /
zErr = FindCreateScriptHeader(pzPTable->pzPKey[0].zPK.iID, lKeyEndDate, zATable, &g_zSHdrDetTable, 
&zVSHD, FALSE, zTHdrDetTable.pzTHdrDet[j].zHeader.lTmphdrNo, iAsset, NULL);
if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
{
InitializePScrHdrDet(&zVSHD);
return zErr;
}	

zErr = CreateNewPerfkey(pzPTable, zRTable.pzPRule[iRule], zVSHD.zHeader.lScrhdrNo, 
zVSHD.zHeader.iSegmentTypeID, lKeyStartDate);

return zErr;
} / * PerfkeyForWeightedParent */


/**
** Function to create a virtual perfscript header and its details using
** perf template header & its details and an Asset record.
**/
DllExport ERRSTRUCT CreateVirtualPerfScript(PTMPHDRDET zTmpHdrDet, PARTASSET2 zPAsset, int iDateOffset,
											BOOL bSecCurr, PSCRHDRDET *pzScrHdrDet)
{
	ERRSTRUCT zErr;
	int       i, iIntegerValue;
	char      sAlphaValue[31];
	double    fFloatValue;
	PSCRDET		zScrDetail;

	lpprInitializeErrStruct(&zErr);
	InitializePScrHdrDet(pzScrHdrDet);

	/* copy appropriate template header fields to script header */
	pzScrHdrDet->zHeader.lTmphdrNo = zTmpHdrDet.zHeader.lTmphdrNo;
	strcpy_s(pzScrHdrDet->zHeader.sOwner, zTmpHdrDet.zHeader.sOwner);
	strcpy_s(pzScrHdrDet->zHeader.sChangeable, zTmpHdrDet.zHeader.sChangeable);
	strcpy_s(pzScrHdrDet->zHeader.sDescription, zTmpHdrDet.zHeader.sDescription);
	pzScrHdrDet->zHeader.lChangeDate = 0;
	pzScrHdrDet->zHeader.sChangedBy[0] = '\0';

	/* create detail records */
	for (i = zTmpHdrDet.iCount - 1; i >= 0; i--)
	{
		/* Get the actual value of the field(based on selecttype) from the asset */
		if (strcmp(zTmpHdrDet.pzDetail[i].sSelectType, "STARTP") != 0 &&
			strcmp(zTmpHdrDet.pzDetail[i].sSelectType, "ENDP") != 0)
		{
			zErr = GetSelectTypeValueForAnAsset(zPAsset, iDateOffset, zTmpHdrDet.pzDetail[i].sSelectType,
												bSecCurr, sAlphaValue, &fFloatValue, &iIntegerValue);
			if (zErr.iBusinessError != 0)
				return zErr;
		}

		InitializePerfScrDet(&zScrDetail);
		/*
		** If a string match is required, make a new masked string using begin point
		** of the template detail, else if a string range comparison is required,
		** make a new masked string using begin point, and another using end point.
		** If a numeric comparison is required copy decimal value as it is, because
		** in the case of numeric comparison, wild card replacement makes no sense.
		*/
		if (strcmp(zTmpHdrDet.pzDetail[i].sSelectType, "STARTP") == 0 || 
			strcmp(zTmpHdrDet.pzDetail[i].sSelectType, "ENDP") == 0)
		{
			strcpy_s(zScrDetail.sBeginPoint, zTmpHdrDet.pzDetail[i].sBeginPoint);
			strcpy_s(zScrDetail.sEndPoint, zTmpHdrDet.pzDetail[i].sEndPoint);
		}
		else if (strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "M") == 0)
		{
			MaskStringUsingTemplate(zTmpHdrDet.pzDetail[i].sBeginPoint,
									zTmpHdrDet.pzDetail[i].sMaskRest,
									zTmpHdrDet.pzDetail[i].sMaskWild,
									zTmpHdrDet.pzDetail[i].sMaskExpand,
									sAlphaValue, zScrDetail.sBeginPoint);

			strcpy_s(zScrDetail.sEndPoint, zScrDetail.sBeginPoint);
		} /* Alphanumeric match */
		else if (strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "R") == 0)
		{
			MaskStringUsingTemplate(zTmpHdrDet.pzDetail[i].sBeginPoint,
									zTmpHdrDet.pzDetail[i].sMaskRest,
									zTmpHdrDet.pzDetail[i].sMaskWild,
									zTmpHdrDet.pzDetail[i].sMaskExpand,
									sAlphaValue, zScrDetail.sBeginPoint);

			MaskStringUsingTemplate(zTmpHdrDet.pzDetail[i].sEndPoint,
									zTmpHdrDet.pzDetail[i].sMaskRest,
									zTmpHdrDet.pzDetail[i].sMaskWild,
									zTmpHdrDet.pzDetail[i].sMaskExpand,
									sAlphaValue, zScrDetail.sEndPoint);
		} /* Alphanumeric range */
		else if (strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "S") == 0)
		{
			strcpy_s(zScrDetail.sBeginPoint, zTmpHdrDet.pzDetail[i].sBeginPoint);
			strcpy_s(zScrDetail.sEndPoint, zTmpHdrDet.pzDetail[i].sEndPoint);
		} /* Special(Bitwise) Testing */
		else if (strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "IL") == 0 ||
				 strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "IU") == 0 ||
				 strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "II") == 0 ||
				 strcmp(zTmpHdrDet.pzDetail[i].sComparisonRule, "IE") == 0)
		{
			if (strcmp(zTmpHdrDet.pzDetail[i].sMaskExpand, "Y") == 0)
			{
				sprintf_s(zScrDetail.sBeginPoint, "%d", iIntegerValue);
				strcpy_s(zScrDetail.sEndPoint, zScrDetail.sBeginPoint);
			}
			else
			{
				strcpy_s(zScrDetail.sBeginPoint, zTmpHdrDet.pzDetail[i].sBeginPoint);
				strcpy_s(zScrDetail.sEndPoint, zTmpHdrDet.pzDetail[i].sEndPoint);
			}
		} /* integer range */
		else
		{
			if (strcmp(zTmpHdrDet.pzDetail[i].sMaskExpand, "Y") == 0)
			{
				sprintf_s(zScrDetail.sBeginPoint, "%f", fFloatValue);
				strcpy_s(zScrDetail.sEndPoint, zScrDetail.sBeginPoint);
			}
			else
			{
				strcpy_s(zScrDetail.sBeginPoint, zTmpHdrDet.pzDetail[i].sBeginPoint);
				strcpy_s(zScrDetail.sEndPoint, zTmpHdrDet.pzDetail[i].sEndPoint);
			}
		} /* floating range */

		zScrDetail.lSeqNo = zTmpHdrDet.pzDetail[i].lSeqNo;
		strcpy_s(zScrDetail.sSelectType, zTmpHdrDet.pzDetail[i].sSelectType);
		strcpy_s(zScrDetail.sComparisonRule, zTmpHdrDet.pzDetail[i].sComparisonRule);
		strcpy_s(zScrDetail.sAndOrLogic, zTmpHdrDet.pzDetail[i].sAndOrLogic);
		strcpy_s(zScrDetail.sIncludeExclude, zTmpHdrDet.pzDetail[i].sIncludeExclude);
		strcpy_s(zScrDetail.sMaskRest, zTmpHdrDet.pzDetail[i].sMaskRest);
		strcpy_s(zScrDetail.sMatchRest, zTmpHdrDet.pzDetail[i].sMatchRest);
		strcpy_s(zScrDetail.sMaskWild, zTmpHdrDet.pzDetail[i].sMaskWild);
		strcpy_s(zScrDetail.sMatchWild, zTmpHdrDet.pzDetail[i].sMatchWild);
		strcpy_s(zScrDetail.sMaskExpand, zTmpHdrDet.pzDetail[i].sMaskExpand);
		strcpy_s(zScrDetail.sMatchExpand, zTmpHdrDet.pzDetail[i].sMatchExpand);
		strcpy_s(zScrDetail.sReportDest, zTmpHdrDet.pzDetail[i].sReportDest);
		zScrDetail.lStartDate = zTmpHdrDet.pzDetail[i].lStartDate;
		zScrDetail.lEndDate = zTmpHdrDet.pzDetail[i].lEndDate;

		zErr = AddDetailToScrHdrDet(zScrDetail, pzScrHdrDet);
		if (zErr.iBusinessError != 0)
			return zErr;
	} /* for loop */

	return zErr;
} /* createvirtualperfscript */


/**
** Function to get the value from an asset for the appropriate field based on a selecttype.
**/
ERRSTRUCT GetSelectTypeValueForAnAsset(PARTASSET2 zPAsset, int iDateOffset, char *sSelectType, BOOL bSecCurr, 
									   char *sAlphaValue, double *pfFloatValue,  int *piIntegerValue)
{
	ERRSTRUCT	zErr;
	char		sMsg[80];

	lpprInitializeErrStruct(&zErr);

	if (iDateOffset < 0 || iDateOffset >= zPAsset.iDailyCount)
	{
		sprintf_s(sMsg, "Invalid Date offset For Security %s", zPAsset.sSecNo);
		return(lpfnPrintError(sMsg, 0, 0, "", 505, 0, 0, "CALCPERF GETSTYPE", FALSE));
	}
	sAlphaValue[0] = '\0';
	*pfFloatValue = 0;
	*piIntegerValue = 0;

	if (strcmp(sSelectType, "LEVEL1") == 0)
		*piIntegerValue = zPAsset.pzDAInfo[iDateOffset].iIndustLevel1;
	else if (strcmp(sSelectType, "LEVEL2") == 0)
		*piIntegerValue = zPAsset.pzDAInfo[iDateOffset].iIndustLevel2;
	else if (strcmp(sSelectType, "LEVEL3") == 0)
		*piIntegerValue = zPAsset.pzDAInfo[iDateOffset].iIndustLevel3;
	else if (strcmp(sSelectType, "SECNO") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sSecNo);
	else if (strcmp(sSelectType, "WI") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sWhenIssue);
	else if (strcmp(sSelectType, "CURR") == 0)
	{
		if (bSecCurr) /* use security currency */
			strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sCurrId);
		else
			strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sIncCurrId); /* use incme currency */
	}
	else if (strcmp(sSelectType, "ISSUED") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sCountryIss);
	else if (strcmp(sSelectType, "ISSUER") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sCountryIsr);
	else if (strcmp(sSelectType, "SPRATE") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sSpRating);
	else if (strcmp(sSelectType, "MDRATE") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sMoodyRating);
	else if (strcmp(sSelectType, "INRATE") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sInternalRating);
	else if (strcmp(sSelectType, "TAXFED") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sTaxableCountry);
	else if (strcmp(sSelectType, "TAXST") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPAsset.sTaxableState);
	else if (strcmp(sSelectType, "CPNRT") == 0)
		*pfFloatValue = zPAsset.fAnnDivCpn;
	else if (strcmp(sSelectType, "SECTYP") == 0)
		*piIntegerValue = zPSTypeTable.zSType[zPAsset.iSTypeIndex].iSecType;
	else if (strcmp(sSelectType, "TYPE1") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPSTypeTable.zSType[zPAsset.iSTypeIndex].sPrimaryType);
	else if (strcmp(sSelectType, "TYPE2") == 0)
		strcpy_s(sAlphaValue, MAXAlphaValueLEN, zPSTypeTable.zSType[zPAsset.iSTypeIndex].sSecondaryType);
	/* else if (strcmp(sSelectType, "MATDT") == 0)
	else if (strcmp(sSelectType, "CURYLD") == 0)
	else if (strcmp(sSelectType, "CURYTM") == 0)
	else if (strcmp(sSelectType, "YLDTOW") == 0)
	else if (strcmp(sSelectType, "BEQYLD") == 0)
	else if (strcmp(sSelectType, "CONVEX") == 0)
	else if (strcmp(sSelectType, "CURDUR") == 0)
	else if (strcmp(sSelectType, "MODDUR") == 0)
	else if (strcmp(sSelectType, "EPS") == 0)
	else if (strcmp(sSelectType, "PE") == 0)
	else if (strcmp(sSelectType, "BETA") == 0)
	else if (strcmp(sSelectType, "EARN5") == 0)*/
	else if (strcmp(sSelectType, "LONGSH") == 0)
		*piIntegerValue = zPAsset.iLongShort;
	else
		zErr = lpfnPrintError("Invalid Select Type", 0, 0, "", 505, 0, 0, "CALCPERF GETSTYPE2", FALSE);

	return zErr;
} /* getselecttypecalueforanasset */


/**
** This function is used to create a string using a template string and a test
** string. Template String has some mask characters which are replaced by actual
** characters from test string(if maskexpand is 'Y'), others are just copied
** from template string(if not wild card or mask expand is not 'Y'). If at the
** end, test string is longer than the template string and mask rest is 'Y',
** remaining test string is appended to the result string.
**/
void MaskStringUsingTemplate(char *sTemplateString, char *sMaskRest, char *sMaskWild, 
							 char *sMaskExpand, char *sTestString, char *sResultString)
{
	int s1len, s2len;
	s1len = strlen(sTemplateString);
	s2len = strlen(sTestString);
	/* Check each character in the template string */
	while (*sTemplateString != '\0')
	{
		/*
		** If maskexpand is set to 'Y' and current character is a wild card
		** character, append corresponding character from the test string, else
		** append current character from template string.
		*/
		if (*sMaskExpand == 'Y' && *sTemplateString == *sMaskWild)
		{
			/*
			** If test string has already ended, copy the rest of template string to
			** the result string and break, else append current character from test
			** string to the result string.
			*/
			if (*sTestString == '\0')
			{
				strcat(sResultString, sTemplateString);
				break;
			}
			else
				strncat(sResultString, sTestString, 1);
		} /* if maskexpand = Y and current character in template is a wild card */
		else
			strncat(sResultString, sTemplateString, 1);

		/* Move to the next character in Template String as well as Test String */
		sTemplateString++;
		sTestString++;
		s2len--;
	} /* while templatestring has not reached its end */

	if ((*sMaskRest == 'Y')&&(s2len>0)) 
		strcat_s(sResultString, 15, sTestString);

} /* createstringusingtemplate */


/**
** Function to create new perf script in the memory and the database.
**/
DllExport ERRSTRUCT CreateNewScript(PSCRHDRDET *pzScrHdrDet, PSCRHDRDETTABLE *pzSHDTable,
									BOOL bSingleSecurity, char *sSecNo, char *sWi, char *sSecDesc1)
{
	ERRSTRUCT	zErr;
	int			i, j, lMaxScrNo, iLevelNo, iGroupID, iSegmentID;
	int			Level1ID, Level2ID, Level3ID;
	BOOL		bShouldExistInSegmap;
	char		sCode[STR12LEN], sName[STR60LEN], sAbbrev[STR20LEN], sLastSelectType[STR6LEN];
	char		sErrMsg[STR60LEN];
	char		sSegmentID[STR30LEN];
	int			iDetailCount, iDetailPos;

	lpprInitializeErrStruct(&zErr);

	lMaxScrNo = 0;
	for (i = 0; i < pzSHDTable->iNumSHdrDet; i++)
	{
		if (pzSHDTable->pzSHdrDet[i].zHeader.lScrhdrNo > lMaxScrNo)
			lMaxScrNo = pzSHDTable->pzSHdrDet[i].zHeader.lScrhdrNo;
	}

	pzScrHdrDet->zHeader.lScrhdrNo = lMaxScrNo + 1;

	if (pzScrHdrDet->zHeader.lHashKey == 0 && strcmp(pzScrHdrDet->zHeader.sHdrKey, "") == 0)
		pzScrHdrDet->zHeader.iSegmentTypeID = TOTALSEGMENTTYPE;
	else
	{
		sLastSelectType[0] = '\0';
		iSegmentID = iLevelNo = -1;
		Level1ID = Level2ID = Level3ID = -1;
		bShouldExistInSegmap = FALSE;
		for (i = 0; i < pzScrHdrDet->iNumDetail; i++)
		{
			iSegmentID = atoi(pzScrHdrDet->pzDetail[i].sBeginPoint);
			strcpy_s(sSegmentID, pzScrHdrDet->pzDetail[i].sBeginPoint);
			j = i;

			// if more than one codition on same select type or comparison rule is not "II"(integer
			// include - value should be a match(not a range)), can not figure it out from segmap
			if (strcmp(pzScrHdrDet->pzDetail[i].sSelectType, sLastSelectType) == 0 || 
				strcmp(pzScrHdrDet->pzDetail[i].sAndOrLogic, "O") == 0 ||
				strcmp(pzScrHdrDet->pzDetail[i].sIncludeExclude, "E") == 0 ||
				strcmp(pzScrHdrDet->pzDetail[i].sComparisonRule, "II") != 0 )
			{
				iLevelNo = 40;
				bShouldExistInSegmap = FALSE;
				break;
			}

			if (strcmp(pzScrHdrDet->pzDetail[i].sSelectType, "LEVEL1") == 0)
			{
				iLevelNo = 1;
				Level1ID = iSegmentID;
			}
			else if (strcmp(pzScrHdrDet->pzDetail[i].sSelectType, "LEVEL2") == 0)
			{
				iLevelNo = 20;
				Level2ID = iSegmentID;
				bShouldExistInSegmap = TRUE;
			}
			else if (strcmp(pzScrHdrDet->pzDetail[i].sSelectType, "LEVEL3") == 0)
			{
				iLevelNo = 30;
				Level3ID = iSegmentID;
				bShouldExistInSegmap = TRUE;
			}
			else
			{
				iLevelNo = 40;
				bShouldExistInSegmap = FALSE;
				break;
			}
		} // for i < numdetail

		if (Level1ID == -1 || (Level2ID == -1 && Level3ID != -1))
		{
			bShouldExistInSegmap = FALSE;
			iLevelNo = 40;
		}

		if (iLevelNo == 1) // coming only from level1
		{
			lpprSelectOneSegment(sName, &iGroupID, &iLevelNo, iSegmentID, &zErr);

			if (zErr.iSqlError == SQLNOTFOUND)
				iSegmentID = -1;
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

		} // if LevelNo <> -1*/
		else if (iLevelNo == 20 || iLevelNo == 30)
		{
			if (iLevelNo == 20)
				Level3ID = -1;

			iSegmentID = lpfnSelectSegmentIDFromSegmap(Level1ID, Level2ID, Level3ID, &zErr);
			if (zErr.iSqlError == SQLNOTFOUND)
				iSegmentID = -1;
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			iGroupID = 0;
			strcpy_s(sName, "");
		}
		else if (bSingleSecurity)
		{
			iSegmentID = lpfnSelectSecSegmap(sSecNo, sWi, &zErr);
			if (zErr.iSqlError == SQLNOTFOUND)
				iSegmentID = -1;
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			strcpy_s(sName, "");
			iGroupID = 0;
		}
		else 
			iSegmentID = -1;


		if (iSegmentID == -1)
		{
			if ((strcmp(pzScrHdrDet->pzDetail[j].sSelectType, "ISSUED") == 0) ||
				(strcmp(pzScrHdrDet->pzDetail[j].sSelectType, "ISSUER") == 0))
			{
				sprintf_s(sCode, "%s(%s)", sSegmentID, pzScrHdrDet->pzDetail[j].sSelectType);
				i	= FindCountryInTable(sSegmentID);
				if (i == -1)
				{
					sprintf_s(sErrMsg, "Invalid Country Code %s", sSegmentID);
					return(lpfnPrintError(sErrMsg, 0, 0, "", -999, 0, 0, "CALCPERF FindCountryInTable", FALSE));
				} 
				else
				{
					strncpy(sName, zCountryTable.zCountry[i].sDescription, sizeof(sName)-1);
					sName[sizeof(sName)-1] = '\0';
				}

			}
			else 
			{
				sprintf_s(sCode, "SYS-%d", lMaxScrNo+1);
				strncpy(sName, pzScrHdrDet->zHeader.sDescription, sizeof(sName)-1);
				sName[sizeof(sName)-1] = '\0';
			}
			strncpy(sAbbrev, sName, sizeof(sAbbrev)-1);
			sAbbrev[sizeof(sAbbrev)-1] = '\0';


			// 04/17/2001 vay 
			// for templates of INDIVIDUAL SECURITY use actual sec_no as
			// description of segment and script 
			iDetailCount = iDetailPos = 0;

			for (i = 0; i < pzScrHdrDet->iNumDetail; i++)
			{
				if (strcmp(pzScrHdrDet->pzDetail[i].sComparisonRule, "C") == 0) 
					continue;

				if (strcmp(pzScrHdrDet->pzDetail[i].sComparisonRule, "M") != 0) 
					break;

				if ((strcmp(pzScrHdrDet->pzDetail[i].sAndOrLogic, "A") == 0) &&
					(strcmp(pzScrHdrDet->pzDetail[i].sIncludeExclude, "I") == 0) &&
					(strcmp(pzScrHdrDet->pzDetail[i].sSelectType, "SECNO") == 0))
				{
					iDetailCount++;
					iDetailPos = i;

					continue;
				}

				if ((strcmp(pzScrHdrDet->pzDetail[i].sAndOrLogic, "A") == 0) &&
					(strcmp(pzScrHdrDet->pzDetail[i].sIncludeExclude, "I") == 0) &&
					(strcmp(pzScrHdrDet->pzDetail[i].sSelectType, "WI") == 0)) 
					iDetailCount++;									
			}		

			if (iDetailCount==2)
			{
				strcpy_s(sName, pzScrHdrDet->pzDetail[iDetailPos].sBeginPoint);
				strncpy(sAbbrev, pzScrHdrDet->pzDetail[iDetailPos].sBeginPoint, sizeof(sAbbrev)-1);
				sAbbrev[sizeof(sAbbrev)-1] = '\0';
			}

			pzScrHdrDet->zHeader.iSegmentTypeID = AddNewSegmentType(Level1ID, Level2ID, Level3ID, sName, sAbbrev, sCode, 
																	sSecNo, sWi, sSecDesc1, bShouldExistInSegmap, bSingleSecurity);
			if (pzScrHdrDet->zHeader.iSegmentTypeID <= -1)
				return(lpfnPrintError("Could Not Create New Segment Type", 0, pzScrHdrDet->zHeader.lTmphdrNo, "H", 
									  521, pzScrHdrDet->zHeader.iSegmentTypeID, 0, "CALCPERF CREATENEWSCRIPT1", FALSE));
			pzScrHdrDet->zHeader.iGroupID = 0;
		}
		else
		{
			pzScrHdrDet->zHeader.iSegmentTypeID = iSegmentID;
			pzScrHdrDet->zHeader.iGroupID = iGroupID;
		}

		strncpy(pzScrHdrDet->zHeader.sDescription, sName, sizeof(pzScrHdrDet->zHeader.sDescription)-1);
		pzScrHdrDet->zHeader.sDescription[sizeof(pzScrHdrDet->zHeader.sDescription)-1] = '\0';
	} // if hashkey not = 0, meaning it is not total

	lpprInsertPerfscriptHeader(pzScrHdrDet->zHeader, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr; 

	for (i = 0; i < pzScrHdrDet->iNumDetail; i++)
	{
		pzScrHdrDet->pzDetail[i].lScrhdrNo = pzScrHdrDet->zHeader.lScrhdrNo;
		lpprInsertPerfscriptDetail(pzScrHdrDet->pzDetail[i], &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

	}

	/* Now insert the records in the memory table */
	zErr = AddScriptHeaderToTable(pzScrHdrDet->zHeader, pzSHDTable, &j);
	if (zErr.iBusinessError != 0)
		return zErr;

	for (i = 0; i < pzScrHdrDet->iNumDetail; i++)
	{
		zErr= AddDetailToScrHdrDet(pzScrHdrDet->pzDetail[i], &pzSHDTable->pzSHdrDet[j]);
		if (zErr.iBusinessError != 0)
			return zErr;
	} /* for i < numdetail */

	return zErr;
} /* createnewscript */


/**
** Function to create new perfkey in the perfkey table. This function does not
** test whether a key with same scrhdrno and ruleno already exists in PKeyTable
** or not, because the calling program must already have done so.
** NOTE : IF THERE IS A PARENT-CHILD(ERN) RELATIONSHIP(AggBaseFlag - 'A' is the
**        Parent and CurrBaseFlag - 'L' & 'B' are children, although for testing
**        purposes, if rule's parentrule = 0 or itself, it is a parent, else a
**        child), IT IS REQUIRED THAT THE PARENT'S RULE IS DECLARED BEFORE ANY
**        OF THE CHILDREN'S RULE, SO THAT A PARENT'S PERFKEY CAN BE GENERATED
**        BEFORE A CHILD'S PERFKEY. WHEN CREATING A NEW KEY FOR A CHILD, THE
**        PERFSCRIPTS WHICH CAUSED THE CREATION OF THE CHILD KEY SHOULD PASS THE
**        TEST FOR ITS PARENT ALSO(CHECK ALL THE KEYS IN MEMORY WHICH ARE
**        CREATED USING PARENT PERFRULE AND MAKE THE CHILD's PERFSCRIPT PASS THE
**        TEST FOR ALL THESE PERFKEYS).
** 6/29/98 : THE ABOVE NOTE DOES NOT APPLY ANY MORE. NOW THERE ARE TWO WAYS OF
**           DEFINING PAREN-CHILD(REN) RELATIONSHIP, ONE BASED ON AGGBASEFLAG
**           AND THE SECOND BASED ON HOW WEIGHTED AVERAGE IS CALCULATED(e.g.
**           THERE MIGHT BE TWO CHILDREN EQUITY-SHORT AND EQUITY-LONG AND THE
**           TOTAL EQUITY WILL BE A PARENT WHOSE PERFORMANCE IS CALCULATED BY
**           WEIGHTED AVERAGE OF EQUITY-SHORT AND EQUITY-LONG PERFORMANCES).
**           THE NEW RULE FOR KEYS WITH PARENT-CHILDREN RELATIONSHIP IS THAT
**           IF THE PARENT PRERFRULE = PERFRULE, IT IS A PARENT ELSE A CHILD.
**           BECAUSE OF THIS CONDITION OF DETERMINING THE PARENT, IT IS
**           NECESSARY THAT THE PARENT PERFRULE SHOULD NOT BE ABLE TO CREATE
**           MORE THAN ONE PERFKEY(BUT THAT CONDITION HAS TO BE ENFORCED FROM
**           OUTSIDE, PERFORMANCE ENGINE DOES NOT DO ANY CHECKING FOR THAT).
**           WHEN A CHILD IS CREATED TRY TO FIND ITS PARENT, IF NOT FOUND
**           CONTINUE WITHOUT REPORTING IT AS AN ERROR. AT THE END, ANOTHER
**           FUNCTION TRIES TO FIND THE PARENT OF ALL THE CHILDREN WHOSE PARENT
**           PERFRULE IS 0, IT IT SUCCEEDS, IT UPDATES THE FIELD ELSE SEND A
**           WARNING MESSAGE(IT IS STILL NOT CONSIDERED AN ERROR).
**/
ERRSTRUCT CreateNewPerfkey(PKEYTABLE *pzPTable, PERFRULE zPrule,
						   long lScrhdrNo, int iSegmentTypeID, long lCurrentPerfDate)
{
	ERRSTRUCT  zErr;
	PKEYSTRUCT zNewPKey;
	//PERFKEY    zPerfkey;

	lpprInitializeErrStruct(&zErr);

	zNewPKey.iDInfoCapacity = zNewPKey.iWDInfoCapacity = 0;
	InitializePKeyStruct(&zNewPKey);

	zNewPKey.zPK.iPortfolioID = zPrule.iPortfolioID; 
	zNewPKey.zPK.lRuleNo = zPrule.lRuleNo;
	zNewPKey.zPK.lScrhdrNo = lScrhdrNo;

	strcpy_s(zNewPKey.zPK.sCurrProc, zPrule.sCurrencyProcessingFlag);
	strcpy_s(zNewPKey.zPK.sTotalRecInd, zPrule.sTotalRecordIndicator);
	if (zPrule.lParentRuleNo == 0 || zPrule.lParentRuleNo == zPrule.lRuleNo)
	{
		strcpy_s(zNewPKey.zPK.sParentChildInd, "P");
		zNewPKey.zPK.lParentPerfkeyNo = zNewPKey.zPK.lPerfkeyNo;
		zNewPKey.lParentRuleNo = zNewPKey.zPK.lRuleNo;
	}
	else
	{
		strcpy_s(zNewPKey.zPK.sParentChildInd, "C");
		zNewPKey.lParentRuleNo = zPrule.lParentRuleNo;
		/* Find out parent perfkey no, it not found, continue */
		zNewPKey.zPK.lParentPerfkeyNo = FindParentPerfkeyNo(*pzPTable, zPrule.lParentRuleNo);
	} /* Child key */

	strcpy_s(zNewPKey.sRecordType, zPrule.sWeightedRecordIndicator);

	zNewPKey.zPK.lInitPerfDate = lCurrentPerfDate;
	zNewPKey.zPK.lLndPerfDate = lCurrentPerfDate;

	zNewPKey.lParentRuleNo = zPrule.lParentRuleNo;
	zNewPKey.iScrHDIndex = FindScrHdrByHdrNo(g_zSHdrDetTable, zNewPKey.zPK.lScrhdrNo);
	strcpy_s(zNewPKey.zPK.sDescription, g_zSHdrDetTable.pzSHdrDet[zNewPKey.iScrHDIndex].zHeader.sDescription); 
	zNewPKey.bNewKey = TRUE;

	if (iSegmentTypeID == TOTALSEGMENTTYPE)
		zNewPKey.zPK.iID = zNewPKey.zPK.iPortfolioID;
	else
		zNewPKey.zPK.iID = AddNewSegment(zNewPKey.zPK.iPortfolioID, 
		g_zSHdrDetTable.pzSHdrDet[zNewPKey.iScrHDIndex].zHeader.sDescription,
		g_zSHdrDetTable.pzSHdrDet[zNewPKey.iScrHDIndex].zHeader.sDescription, iSegmentTypeID);
	if (zNewPKey.zPK.iID == -1)
		return(lpfnPrintError("Could Not Create New Segment", zNewPKey.zPK.iPortfolioID, zNewPKey.zPK.lScrhdrNo, 
		"S", 520, 0, 0, "CALCPERF CREATENEWPKEY2", FALSE));

	zErr = AddPerfkeyToTable(pzPTable, zNewPKey);
	if (zErr.iBusinessError != 0)
	{
		InitializePKeyStruct(&zNewPKey);
		return zErr;
	}
	pzPTable->pzPKey[pzPTable->iCount - 1].zPK.lPerfkeyNo = 0;

	/* Free up the memory */
	InitializePKeyStruct(&zNewPKey);
	return zErr;
} /* createnewperfkey */


/**
** Function to find a parent perf key. This function tries to find first
** perfkey which was created using paren rule no, if no key is found it is not
** an error.
**/
long FindParentPerfkeyNo(PKEYTABLE zPTable, long lParentRuleNo)
{
	ERRSTRUCT zErr;
	long      lParentPerfkeyNo;
	int       i;

	lpprInitializeErrStruct(&zErr);

	i = 0;
	lParentPerfkeyNo = 0;
	while (lParentPerfkeyNo == 0 && i < zPTable.iCount)
	{
		if (zPTable.pzPKey[i].zPK.lRuleNo == lParentRuleNo &&
			zPTable.pzPKey[i].zPK.lPerfkeyNo == zPTable.pzPKey[i].zPK.lParentPerfkeyNo)
			lParentPerfkeyNo = zPTable.pzPKey[i].zPK.lPerfkeyNo;

		i++;
	} /* While parent not found and i < number of pkey in the table */

	return lParentPerfkeyNo;
} /* FindParentPerfkeyNo */


/**
** This function call is to roll and value portfolio, if it has not
** already been rolled to the desired date, then it declares holdings/holdcash
** cursors, if they are not already declared for the desired dates. Next it gets
** records from holdings joined with assets cursor, adds it to holdings and
** assets(if it already does not exist) tables. In the end it gets records from
** holdacsh joined with assets cursor and adds those records to holdings and
** assets table.
**/
ERRSTRUCT FillHoldingAndAssetTables(PARTPMAIN *pzPmain, long lDate, ASSETTABLE2 *pzATable, 
									HOLDINGTABLE *pzHTable, long lLastPerfDate, long lCurrentPerfDate, char DoAccruals)
{
	PARTHOLDING	zTempHold;
	PARTASSET2  zTempAsset;
	LEVELINFO	zLevels;
	ERRSTRUCT	zErr;
	int			i, iAIdx, iDateIndex, iTotalPriorAssets, iSTIndex;
	long		lLastTransNo;
	char		cNewType, sHoldings[STR80LEN], sHoldcash[STR80LEN], sSecNo[STR12LEN], sWi[STR1LEN], sSecXtend[STR2LEN];
	BOOL		bGotAssetMV;

	lpprInitializeErrStruct(&zErr);
	zTempAsset.iDailyCount = 0;
	lLastTransNo = 0;
	iSTIndex = 0;

	/*
	** Get the date which the portfolio is rolled on.
	** Get the type of table to use from LookupHoldName
	**
	** If the date is the same .. do not roll or value
	** If the date is not the same, and LookupHoldName recommends a temp table,
	**    ROLL and VALUATE it. 
	*/
	cNewType = LookupHoldName(sHoldings, sHoldcash, lDate);
	if (cNewType == 'E')
		return(lpfnPrintError("Error In LookupHoldName", pzPmain->iID, 0, "", -1, 0, 0, "CALCPERF FILLHOLDINGS1", FALSE));

	// for the given date find out if market avlue for the assets was saved or not, note that it's not
	// not necessary that the day we are looking for exists for all the assets, but if it exist
	// even for one asset and that asset has market value then any other asset that has that day will
	// have market value as well.
	i = 0;
	bGotAssetMV = FALSE;
	while (i < pzATable->iNumAsset && !bGotAssetMV)
	{
		iDateIndex = FindDailyInfoOffset(lDate, pzATable->pzAsset[i], TRUE);
		if (iDateIndex >= 0 && iDateIndex < pzATable->pzAsset[i].iDailyCount)
			bGotAssetMV = pzATable->pzAsset[i].pzDAInfo[iDateIndex].bGotMV;

		i++;
	} // while i < numasset and bGotAssetMV is false

	//fprintf(fp, "Fill HoldingsAndAssets Called for %d, bGotAssetMv: %d\n", lDate, bGotAssetMV);
	// total number of assets in the table prior to possibly more being added in this function
	iTotalPriorAssets = pzATable->iNumAsset; 

	if (pzPmain->lValDate != lDate && cNewType == 'P')
	{
		zErr = InitializeAppropriateLibrary("", "", 'R', PERFORM_DATE, FALSE);
		//zErr = lpfnInitRoll(sDatabase, "", "", REORGDATE, sErrorFile);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		zErr = lpfnRoll(pzPmain->iID, "", "", 0, 0, "B", "", TRUE, 2, lDate, FALSE); 
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		pzPmain->lValDate = lDate;

		zErr = InitializeAppropriateLibrary("", "", 'V', PERFORM_DATE, FALSE);
		//zErr = lpfnInitValuation(REORGDATE, sDatabase, sErrorFile);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		zErr = lpfnValuation("A", pzPmain->iID, lDate, TRUE);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		// Now reset the error file name
		zErr = InitializeAppropriateLibrary("", "", ' ', 0, TRUE);
	}  /* End ROLL - VALUATION section */

	// Get the holdings record
	while (!zErr.iSqlError)
	{
		lpprInitializeErrStruct(&zErr);
		InitializePartialAsset(&zTempAsset);
		InitializePartialHolding(&zTempHold);

		lpprSelectPerformanceHoldings(&zTempHold, &zTempAsset, &zLevels, pzPmain->iID, sHoldings, pzPmain->iVendorID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		//(lpfnPrintError("Error selecting Holdings", pzPmain->iID, 0,"", zErr.iBusinessError, 
		//										zErr.iSqlError, zErr.iIsamCode, "CALCPERF FILLHOLDASSET2", FALSE));

		/* If the MktValue < 0, it is a short position, else a long position */
		if (zTempHold.fMktVal < 0.0)
			zTempAsset.iLongShort = SRESULT_SHORT | ARESULT_SHORT;
		else
			zTempAsset.iLongShort = SRESULT_LONG | ARESULT_LONG;

		/*
		** If any of the industry level of an asset changes, the query is going to return multiple records
		** for that asset, we do need new values of the industry level, however, we can't count that holdings 
		** taxlot (which will be repeated becuase of the join) twice, so if the taxlot number in this record 
		** is same as previous record, just add industry level of the asset, don't add values from taxlot again
		*/
		if (zTempHold.lTransNo == lLastTransNo)
		{
			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempHold.iAssetIndex, lLastPerfDate, lCurrentPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			continue;
		}
		lLastTransNo = zTempHold.lTransNo;

		iAIdx = FindAssetInTable(*pzATable, zTempHold.sSecNo, zTempHold.sWi, TRUE, zTempAsset.iLongShort);
		if (iAIdx < 0 || iAIdx >= pzATable->iNumAsset)
		{
			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempHold.iAssetIndex, lLastPerfDate, lCurrentPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
		} /* If Asset not already found in the table */
		else
			zTempHold.iAssetIndex = iAIdx;

		// find sectype (index) in the sectype table, for this asset
		iSTIndex = pzATable->pzAsset[zTempHold.iAssetIndex].iSTypeIndex;

		if (IsValueZero(zTempHold.fMvBaseXrate, 13)) // == 0.0)
			return(lpfnPrintError("Invalid Mv Base Xrate", pzPmain->iID, zTempHold.lTransNo, "T", 
			67, 0, 0, "CALCPERF FILLHOLDASSET3", FALSE));

		if (IsValueZero(zTempHold.fAiBaseXrate, 13)) // == 0.0)
			return(lpfnPrintError("Invalid Ai Base Xrate", pzPmain->iID, zTempHold.lTransNo, "T", 
			123, 0, 0, "CALCPERF FILLHOLDASSET4", FALSE));

		zTempHold.fMktVal  /= zTempHold.fMvBaseXrate;
		// if it is equity, accrued dividend gets calculated from accdiv/trans, reset accrued
		// dividend on holdings otherwise it will be double counted
		// SB - 2/10/09 (VI# 42033) - Treat fixed income mutual funds same as equity (i.e. don't take pending accrual from 
		// holdings), they typically have different ex and pay dates and hence were being double counted.
		if (strcmp(zPSTypeTable.zSType[iSTIndex].sPrimaryType, "E") == 0 || 
			(strcmp(zPSTypeTable.zSType[iSTIndex].sPrimaryType, "B") == 0 && strcmp(zPSTypeTable.zSType[iSTIndex].sSecondaryType, "U") == 0)
			|| (DoAccruals == 'N'))
			zTempHold.fAccrInt = 0;
		else
			zTempHold.fAccrInt /= zTempHold.fAiBaseXrate;

		zTempHold.fAnnualIncome /= zTempHold.fAiBaseXrate;
		zTempHold.bHoldCash = FALSE;
		zErr = AddHoldingToTable(pzHTable, zTempHold);
		if (zErr.iBusinessError != 0)
			return zErr;

		// if we didn't have asset market value for this day or if this is a new asset added to the table then
		// add this lot's market value to daily asset info for this asset if this date exist for this asset
		if (bGotAssetMV == FALSE || zTempHold.iAssetIndex >= iTotalPriorAssets)
		{
			iDateIndex = FindDailyInfoOffset(lDate, pzATable->pzAsset[zTempHold.iAssetIndex], TRUE);
			if (iDateIndex >= 0 && iDateIndex < pzATable->pzAsset[zTempHold.iAssetIndex].iDailyCount)
			{
				pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].fMktVal += zTempHold.fMktVal;
				pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].bGotMV = TRUE;
				if (strcmp(zPSTypeTable.zSType[iSTIndex].sPrimaryType, "E") != 0 &&
					!(strcmp(zPSTypeTable.zSType[iSTIndex].sPrimaryType, "B") == 0 && strcmp(zPSTypeTable.zSType[iSTIndex].sSecondaryType, "U") == 0))
				{
					pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].fAI += zTempHold.fAccrInt;
					pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].fAccrExRate += zTempHold.fAiBaseXrate;
					pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].bGotAccrual = TRUE;
				}
			} // if date exists in the daily info array for this asset
		} // if assets' mv needs tobe added
	} /* while no error */

	memset(sSecNo, sizeof(sSecNo), 0);
	memset(sWi, sizeof(sWi), 0);
	memset(sSecXtend, 0, sizeof(sSecXtend));

	/* Fetch records from holdcash and put them in asset and hold tables */
	while (!zErr.iSqlError)
	{
		lpprInitializeErrStruct(&zErr);
		InitializePartialAsset(&zTempAsset);
		InitializePartialHolding(&zTempHold);

		lpprSelectPerformanceHoldcash(&zTempHold, &zTempAsset, &zLevels, pzPmain->iID, sHoldcash, pzPmain->iVendorID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/* If the MktValue < 0, it is a short position, else a long position */
		if (zTempHold.fMktVal  < 0.0)
			zTempAsset.iLongShort = SRESULT_SHORT | ARESULT_SHORT;
		else
			zTempAsset.iLongShort = SRESULT_LONG | ARESULT_LONG;

		/*
		** If any of the industry level of an asset changes, the query is going to return multiple records
		** for that asset, we do need new values of the industry level, however, we can't count that holdcash
		** taxlot (which will be repeated becuase of the join) twice, so if the sec_no, wi & sec_xtend in this record 
		** is same as previous record, just add industry level of the asset, don't add values from taxlot again
		*/
		if (strcmp(zTempHold.sSecNo, sSecNo) == 0 && strcmp(zTempHold.sWi, sWi) == 0 && strcmp(zTempHold.sSecXtend, sSecXtend) == 0)
		{
			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempHold.iAssetIndex, lLastPerfDate, lCurrentPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			continue;
		}
		strcpy_s(sSecNo, zTempHold.sSecNo);
		strcpy_s(sWi, zTempHold.sWi);
		strcpy_s(sSecXtend, zTempHold.sSecXtend);

		iAIdx = FindAssetInTable(*pzATable, zTempHold.sSecNo, zTempHold.sWi, TRUE, zTempAsset.iLongShort);
		if (iAIdx < 0 || iAIdx >= pzATable->iNumAsset)
		{
			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempHold.iAssetIndex, lLastPerfDate, lCurrentPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
		} /* If Asset not already found in the table */
		else
			zTempHold.iAssetIndex = iAIdx;

		if (IsValueZero(zTempHold.fMvBaseXrate, 13)) // == 0.0)
			return(lpfnPrintError("Invalid Mv Base Xrate", pzPmain->iID, zTempHold.lTransNo, "T", 
			67, 0, 0, "CALCPERF FILLHOLDASSET7", FALSE));

		zTempHold.fMktVal /=  zTempHold.fMvBaseXrate;
		zTempHold.bHoldCash = TRUE;
		zErr = AddHoldingToTable(pzHTable, zTempHold);
		if (zErr.iBusinessError != 0)
			return zErr;

		// if we didn't have asset market value for this day or if this is a new asset added to the table then
		// add this lot's market value to daily asset info for this asset and this date
		if (bGotAssetMV == FALSE || zTempHold.iAssetIndex >= iTotalPriorAssets)
		{
			iDateIndex = FindDailyInfoOffset(lDate, pzATable->pzAsset[zTempHold.iAssetIndex], TRUE);
			if (iDateIndex >= 0 && iDateIndex < pzATable[zTempHold.iAssetIndex].iNumAsset)
			{
				pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].fMktVal += zTempHold.fMktVal;
				pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].bGotMV = TRUE;
				pzATable->pzAsset[zTempHold.iAssetIndex].pzDAInfo[iDateIndex].bGotAccrual = TRUE;
			} // if date exists in the asset's daily info array
		} // if assets' mv needs to be added
	} /* while no error */

	pzHTable->lHoldDate = lDate;
	InitializePartialAsset(&zTempAsset);

	return zErr;
} /* fillholdingandassettables */


/**
** This function gets all transaction(joined with assets), it is interested in
** and then adds them to the trans and assets table. Different trans joined
** with asset cursors are used for as-of and regular processings.
**/
ERRSTRUCT FillTransAndAssetTables(int iID, int iVendorID, long lDate1, long lDate2, ASSETTABLE2 *pzATable, 
								  TRANSTABLE *pzTTable, BOOL bAdjustPerfDate)
{
	ERRSTRUCT	zErr;
	int			iAIdx;
	long		lLastTransNo;
	PARTTRANS	zTempTrans;
	PARTASSET2	zTempAsset;
	LEVELINFO	zLevels;

	lpprInitializeErrStruct(&zErr);
	zTempAsset.iDailyCount = 0;
	lLastTransNo = 0;

	//fprintf(fp, "FillTransAndAsset Called With %d and %d", lDate1, lDate2);
	// Select all transactions and related assets and put them in asset and trans tables
	while (!zErr.iSqlError)
	{
		lpprInitializeErrStruct(&zErr);
		InitializePartialAsset(&zTempAsset);
		InitializePartialTrans(&zTempTrans);

		lpprSelectPerformanceTransaction(&zTempTrans, &zTempAsset, &zLevels, iID, lDate1, lDate2, iVendorID, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		//fprintf(fp, "Fetched TransNo : %d\n", zTempTrans.lTransNo);
		/*
		** TranType table has all the possible Tran types in the system, so we
		** should always find the trantype of the current transaction. In the
		** unlikely event of not finding it there, return with an error.
		*/
		if (strcmp(zTempTrans.sTranType, "RV") == 0)
		{
			strcpy_s(zTempTrans.sTranType, zTempTrans.sRevType);
			zTempTrans.bReversal = TRUE;
		}

		zTempTrans.iTranTypeIndex = FindTranTypeInTable(zTempTrans.sTranType, zTempTrans.sDrCr);
		if (zTempTrans.iTranTypeIndex == -1)
			continue;
		/*      return(lpfnPrintError("Invalid Tran Type Found", iID, zTempTrans.lTransNo,
		"T", 502, 0, 0, "CALCPERF FILLTRANSASSET3", FALSE));*/

		zTempAsset.iLongShort = LongShortBitForTrans(zTempTrans);

		/*
		** If any of the industry level of an asset changes, the query is going to return multiple records
		** for that asset, we do need new values of the industry level, however, we can't count that transaction 
		** (which will be repeated becuase of the join) twice, so if the trans number in this record 
		** is same as previous record, just add industry level of the asset, don't add transaction record again
		*/
		if (zTempTrans.lTransNo == lLastTransNo)
		{
			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempTrans.iSecAssetIndex, lDate1, lDate2);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			continue;
		}
		lLastTransNo = zTempTrans.lTransNo;

		iAIdx = FindAssetInTable(*pzATable, zTempTrans.sSecNo, zTempTrans.sWi, TRUE, zTempAsset.iLongShort);
		if (iAIdx < 0 || iAIdx >= pzATable->iNumAsset)
		{	
			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempTrans.iSecAssetIndex, lDate1, lDate2);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
		} /* If Asset not already found in the table */
		else
			zTempTrans.iSecAssetIndex = iAIdx;

		/* If transfer trade, check XSecNo  XWi also */
		if (strcmp(zTempTrans.sTranType, "TS") == 0 ||
			strcmp(zTempTrans.sTranType, "TC") == 0)
		{
			iAIdx = FindAssetInTable(*pzATable, zTempTrans.sXSecNo, zTempTrans.sXWi, TRUE, zTempAsset.iLongShort);
			if (iAIdx < 0 || iAIdx >= pzATable->iNumAsset)
			{
				zErr = SelectAsset(zTempTrans.sXSecNo, zTempTrans.sXWi, iVendorID, &zTempAsset, &zLevels, 
					zTempAsset.iLongShort, zTempTrans.iID, zTempTrans.lTransNo);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					return zErr;

				zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &zTempTrans.iXSecAssetIndex, lDate1, lDate2);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					return zErr;
			} /* If Asset not already found in the table */
			else
				zTempTrans.iXSecAssetIndex = iAIdx;
		} /* If Transfer trade */

		if (bAdjustPerfDate) 
			zTempTrans.lPerfDate = zTempTrans.lPerfDateAdj;

		zErr = AddTransToTable(pzTTable, zTempTrans);
		if (zErr.iBusinessError != 0)
			return zErr;

	} /* while no error */

	InitializePartialAsset(&zTempAsset);
	return zErr;
} /* filltransandassettables */


/**
** This function gets all undeleted accdiv and adds them to the accdiv table.
**/
ERRSTRUCT FillAccdivTable(int iID, long lLastPerfDate, long lCurrentPerfDate, 
						  ASSETTABLE2 *pzATable, ACCDIVTABLE *pzAccdivTable)
{
	ERRSTRUCT	zErr;
	PARTACCDIV	zTempAccdiv;

	lpprInitializeErrStruct(&zErr);

	// Select all undeleted accdiv
	while (!zErr.iSqlError)
	{
		lpprInitializeErrStruct(&zErr);
		InitializePartialAccdiv(&zTempAccdiv);

		// Select all accdiv entries where trade date <= CurrentPerfDate and settlement date > LastPerfDate - 1
		lpprSelectAllAccdivForAnAccount(iID, lCurrentPerfDate, lLastPerfDate - 1, &zTempAccdiv, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{ 
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		zErr = AddAccdivToTable(pzAccdivTable, zTempAccdiv);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;
	} /* while no error */
	zErr.iSqlError = 0;

	return zErr;
} // fillaccdivtable


/**
** This function is called after holdings and transaction tables are filled up.
** It goes through all the transactions and picks up accruals from "RI" and "RD"
** trades and puts them in the appropriate lot in holdings. It then gets
** from accdiv records and put them in holdings as well (if holdings does not
** exist a record with 0 market value is added in the holdings)
**/
ERRSTRUCT GetAccruals(ACCDIVTABLE zADTable, ASSETTABLE2 *pzATable, TRANSTABLE *pzTTable, HOLDINGTABLE *pzHTable, 
					  int iVendorID, long lPerfDate, long lLastPerfDate, long lCurrPerfDate)
{
	ERRSTRUCT   zErr;
	int         i;
	PARTACCDIV  zAccdiv;
	PARTTRANS	zTR;

	lpprInitializeErrStruct(&zErr);
	memset(&zAccdiv, 0, sizeof(zAccdiv));
	memset(&zTR, 0, sizeof(zTR));

	/*
	** Go through transaction table(in memory) and if any "RI", "RD" transaction found with
	** TrdDate <= PerfDate and StlDate > PerfDate add it to appropriate holdings lot(in the holdings table).
	*/
	for (i = 0; i < pzTTable->iNumTrans; i++)
	{
		// We need only "RI" and "RD" transactions 
		if (strcmp(pzTTable->pzTrans[i].sTranType, "RI") != 0 &&
			strcmp(pzTTable->pzTrans[i].sTranType, "RD") != 0)
			continue;

		// Unsupervised are not part of accruals 
		if (strcmp(pzTTable->pzTrans[i].sSecXtend, "UP") == 0)
			continue;

		// Need only those transactions for which TrdDate <= PerfDate and StlDate > PerfDate.
		if (pzTTable->pzTrans[i].lTrdDate > lPerfDate || pzTTable->pzTrans[i].lStlDate <= lPerfDate)
			continue;

		zErr = AddAccrualToHolding(pzTTable->pzTrans[i], zAccdiv, FALSE, iVendorID, pzATable, pzHTable, 
			lPerfDate, lLastPerfDate, lCurrPerfDate);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
	} // i < pzTTable->iNumTrans 

	/*
	** Now get records from the accrual table, if any for the account. If any "AD" transaction
	** is found with TrdDate <= PerfDate and StlDate > PerfDate and delete flag <> 'Y' then
	** add it to appropriate holdings lot(in the holdings table)
	*/
	for (i = 0; i < zADTable.iNumAccdiv; i++)
	{
		// Unsupervised are not added to accrual numbers 
		if (strcmp(zADTable.pzAccdiv[i].sSecXtend, "UP") == 0)
			continue;

		// If it has been marked deleted in the accdiv
		// and payment trans is settling prior the processing date, skip it
		if ((strcmp(zADTable.pzAccdiv[i].sDeleteFlag, "Y") == 0) &&
			(zADTable.pzAccdiv[i].lStlDate <= lCurrPerfDate))
			continue;

		// Accdiv found, if date checks pass, add accruals to holdings table 
		if (zADTable.pzAccdiv[i].lTrdDate <= lPerfDate && zADTable.pzAccdiv[i].lStlDate > lPerfDate)
		{
			zErr = AddAccrualToHolding(zTR, zADTable.pzAccdiv[i], TRUE, iVendorID, pzATable, pzHTable, 
									   lPerfDate, lLastPerfDate, lCurrPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

		} // If found accdiv record passes date check 
	} // for i < zADTable.iNumAccdiv

	/* for debug purpose only 
	fclose(OutputFile);
	*/

	return zErr;                 
} // GetAccruals 


ERRSTRUCT AddAccrualToHolding(PARTTRANS zTR, PARTACCDIV zAR, BOOL bUseAccdiv, int iVendorID, ASSETTABLE2 *pzATable, 
							  HOLDINGTABLE *pzHTable, long lDate, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT   zErr;
	PARTASSET2  zTempAsset;
	LEVELINFO	zLevels;
	PARTHOLDING zTempHold;
	char        sSecNo[13], sWi[2], sSecXtend[3];
	int         i, iID, iTHIdx, iAIdx, iDateIndex;
	long        lTransNo;
	double      fAmount, fExRate;
	short       iLongShort;
	BOOL		bGotAssetAccrual;

	lpprInitializeErrStruct(&zErr);
	zTempAsset.iDailyCount = 0;
	InitializePartialAsset(&zTempAsset);

	// for the given date find out if accrued interest for the assets was saved or not, note that it's not
	// not necessary that the day we are looking for exists for all the assets, but if it exist
	// even for one asset and that asset has accrural then any other asset that has that day will
	// have accrual as well.
	i = 0;
	bGotAssetAccrual = FALSE;
	while (i < pzATable->iNumAsset && !bGotAssetAccrual)
	{
		iDateIndex = FindDailyInfoOffset(lDate, pzATable->pzAsset[i], TRUE);
		if (iDateIndex >= 0 && iDateIndex < pzATable->pzAsset[i].iDailyCount)
			bGotAssetAccrual = pzATable->pzAsset[i].pzDAInfo[iDateIndex].bGotAccrual;

		i++;
	} // while i < numasset and bGotAssetAccrual is false

	if (bUseAccdiv)
	{
		iID = zAR.iID;
		strcpy_s(sSecNo, zAR.sSecNo);
		strcpy_s(sWi, zAR.sWi);
		strcpy_s(sSecXtend, zAR.sSecXtend);
		lTransNo = zAR.lTransNo;
		if (strcmp(zAR.sDrCr, "DR") == 0)
		{
			fAmount = (zAR.fPcplAmt + zAR.fIncomeAmt) * -1.0;
			iLongShort = SRESULT_SHORT | ARESULT_SHORT;
		}
		else
		{
			fAmount = zAR.fPcplAmt + zAR.fIncomeAmt;
			iLongShort = SRESULT_LONG | ARESULT_LONG;
		}
		fExRate = zAR.fAccrBaseXrate;
	} // Get from accdiv record 
	else
	{
		iID = zTR.iID;
		strcpy_s(sSecNo, zTR.sSecNo);
		strcpy_s(sWi, zTR.sWi);
		strcpy_s(sSecXtend, zTR.sSecXtend);

		if (zTR.lTaxlotNo!=0)
			lTransNo = zTR.lTaxlotNo;
		else
			lTransNo = zTR.lTransNo;

		if (strcmp(zTR.sDrCr, "DR") == 0)
			fAmount = (zTR.fPcplAmt + zTR.fIncomeAmt) * -1.0;
		else
			fAmount = zTR.fPcplAmt + zTR.fIncomeAmt;

		if (zTR.bReversal)
			fAmount *= -1;
		fExRate = zTR.fAccrBaseXrate;
		iLongShort = LongShortBitForTrans(zTR);
	} /* Get from trans record */

	/*
	** Find the lot in holdings table, if exist add principal amount to its
	** accruals, if not then add a lot with zero market value, total cost, etc,
	** with accruals equal to the transaction's(or accdiv's) pcpl amount.
	*/
	iTHIdx = FindHoldingInTable(*pzHTable, iID, lTransNo);
	if (iTHIdx >= 0 && iTHIdx < pzHTable->iNumHolding)
	{
		if (IsValueZero(pzHTable->pzHold[iTHIdx].fAiBaseXrate, 13)) 
			return(lpfnPrintError("Invalid Ai Base Xrate", iID, pzHTable->pzHold[iTHIdx].lTransNo, 
								  "T", 123, 0, 0, "CALCPERF ADDACCRUALS1", FALSE));

		fAmount /= pzHTable->pzHold[iTHIdx].fAiBaseXrate;

		pzHTable->pzHold[iTHIdx].fAccrInt += fAmount;
		if (bGotAssetAccrual == FALSE)
		{
			iDateIndex = FindDailyInfoOffset(lDate, pzATable->pzAsset[pzHTable->pzHold[iTHIdx].iAssetIndex], TRUE);
			if (iDateIndex >= 0 && iDateIndex < pzATable[pzHTable->pzHold[iTHIdx].iAssetIndex].iNumAsset)
			{
				pzATable->pzAsset[pzHTable->pzHold[iTHIdx].iAssetIndex].pzDAInfo[iDateIndex].fAD += fAmount;
				pzATable->pzAsset[pzHTable->pzHold[iTHIdx].iAssetIndex].pzDAInfo[iDateIndex].fAccrExRate = pzHTable->pzHold[iTHIdx].fAiBaseXrate;
				pzATable->pzAsset[pzHTable->pzHold[iTHIdx].iAssetIndex].pzDAInfo[iDateIndex].bGotAccrual = TRUE;
			}
		}
	}
	else
	{
		iAIdx = FindAssetInTable(*pzATable, sSecNo, sWi, TRUE, iLongShort);
		if (iAIdx < 0 || iAIdx >= pzATable->iNumAsset)
		{
			zErr = SelectAsset(sSecNo, sWi, iVendorID, &zTempAsset, &zLevels, iLongShort, iID, lTransNo);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, &iAIdx, lLastPerfDate, lCurrentPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			// if it's a new asset, obviously we haven't gotten accrual for it before
			bGotAssetAccrual = FALSE;
		} /* If Asset not already found in the table */

		InitializePartialHolding(&zTempHold);
		if (IsValueZero(fExRate, 8))
			fExRate = pzATable->pzAsset[iAIdx].fCurExrate;

		zTempHold.iID = iID;
		strcpy_s(zTempHold.sSecNo, sSecNo);
		strcpy_s(zTempHold.sWi, sWi);
		strcpy_s(zTempHold.sSecXtend, sSecXtend);
		zTempHold.lTransNo = lTransNo;
		// SB 10/29/2013 - VI #54038 - performance should not use current exchange rate from the asset, it should use
		// exchange rate from RD transaction (if dividend already paid) or exchange rate from accdiv.
		zTempHold.fAiBaseXrate = fExRate;
		zTempHold.iAssetIndex = iAIdx;
		if (IsValueZero(zTempHold.fAiBaseXrate, 13)) 
			return(lpfnPrintError("Invalid Ai Base Xrate", iID, pzHTable->pzHold[iTHIdx].lTransNo, 
								  "T", 123, 0, 0, "CALCPERF ADDACCRUALS2", FALSE));

		zTempHold.fAccrInt = fAmount / zTempHold.fAiBaseXrate;
		zErr = AddHoldingToTable(pzHTable, zTempHold);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		if (bGotAssetAccrual == FALSE)
		{
			iDateIndex = FindDailyInfoOffset(lDate, pzATable->pzAsset[iAIdx], TRUE);
			if (iDateIndex >= 0 && iDateIndex < pzATable[zTempHold.iAssetIndex].iNumAsset)
			{
				pzATable->pzAsset[iAIdx].pzDAInfo[iDateIndex].fAD += zTempHold.fAccrInt;
				pzATable->pzAsset[iAIdx].pzDAInfo[iDateIndex].fAccrExRate = zTempHold.fAiBaseXrate;
				pzATable->pzAsset[iAIdx].pzDAInfo[iDateIndex].bGotAccrual = TRUE;
			}
		}
	} // Holdings lot not found 

	InitializePartialAsset(&zTempAsset);
	return zErr;
} // AddAccrualToHolding 


/**
** This function is called after asset, holding and transaction tables are
** filled up. It goes through all the records in the asset table and using
** seccurid(for current entry), it finds corresponding sec_no and wi from
** currency table. Then it tries to find that sec_no and wi in the asset table,
** if it does not exist there, it retrieves it from asset table(in the database)
** and adds it to the asset array in memory. It does the same processing for
** inccurrid also. In the next loop it goes through the transaction table and
** does similar processing with transaction's CurrId(cash principal currency)
** and IncCurrId(cash income currency).
**/
ERRSTRUCT GetCurrencyAsset(ASSETTABLE2 *pzATable, TRANSTABLE zTTable, int iVendorID, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT zErr;
	int       i, iIndex;

	lpprInitializeErrStruct(&zErr);

	/* Check CurrId and IncomeCurrId of all asset records. */
	for (i = 0; i < pzATable->iNumAsset; i++)
	{
		/* Check CurrId */
		zErr = FindAndAddCurrency(pzATable->pzAsset[i].sCurrId, pzATable, 0, 0, iVendorID, &iIndex, lLastPerfDate, lCurrentPerfDate);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/* Check Income CurrId */
		zErr = FindAndAddCurrency(pzATable->pzAsset[i].sIncCurrId, pzATable, 0, 0, iVendorID, &iIndex, lLastPerfDate, lCurrentPerfDate);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
	} /* for i < pzATable->iNumAssets */

	/* Check CurrId of all trans records */
	for (i = 0; i < zTTable.iNumTrans; i++)
	{
		/* Check CurrId */
		zErr = FindAndAddCurrency(zTTable.pzTrans[i].sCurrId, pzATable,zTTable.pzTrans[i].iID, zTTable.pzTrans[i].lTransNo, 
			iVendorID, &zTTable.pzTrans[i].iCashAssetIndex, lLastPerfDate, lCurrentPerfDate);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/* Check IncCurrId */
		zErr = FindAndAddCurrency(zTTable.pzTrans[i].sIncCurrId, pzATable, zTTable.pzTrans[i].iID, zTTable.pzTrans[i].lTransNo,
								  iVendorID, &zTTable.pzTrans[i].iIncAssetIndex, lLastPerfDate, lCurrentPerfDate);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/* If a "TC" or "TS" trade, check XCurrId */
		if (strcmp(zTTable.pzTrans[i].sTranType, "TC") == 0 ||
			strcmp(zTTable.pzTrans[i].sTranType, "TS") == 0)
		{
			zErr = FindAndAddCurrency(zTTable.pzTrans[i].sXCurrId, pzATable, zTTable.pzTrans[i].iID, zTTable.pzTrans[i].lTransNo,
									  iVendorID, &zTTable.pzTrans[i].iXCashAssetIndex, lLastPerfDate, lCurrentPerfDate);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
		} /* If "TC" or "TS" trades */
	} /* for i < pzTTable->iNumTrans */

	return zErr;
} /* GetCurrencyAsset */


/**
** Function to look for a currency in the currency table, get its secno & wi
** and look for that asset in the Asset Table. If asset not found, it adds it.
**/
ERRSTRUCT FindAndAddCurrency(char *sCurrId, ASSETTABLE2 *pzATable, int iID, long lTransNo, int iVendorID,
							 int *piIndex, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT	zErr;
	PARTASSET2	zTempAsset;
	LEVELINFO	zLevels;
	char		sErrMsg[60], sSecNo[13], sWi[2];
	short		iLongShort;

	lpprInitializeErrStruct(&zErr);
	zTempAsset.iDailyCount = 0;
	InitializePartialAsset(&zTempAsset);

	/* find secno and wi for the currency */
	*piIndex = FindCurrencyInTable(sCurrId);
	if (*piIndex == -1)
	{
		sprintf_s(sErrMsg, "Invalid CurrId %s", sCurrId);
		return(lpfnPrintError(sErrMsg, iID, lTransNo, "T", 503, 0, 0, "CALCPERF FINDANDADDCURR1", FALSE));
	}

	strcpy_s(sSecNo, zCurrTable.zCurrency[*piIndex].sSecNo);
	strcpy_s(sWi, zCurrTable.zCurrency[*piIndex].sWi);

	/* find asset using currency's secno and wi */
	iLongShort = SRESULT_LONG | SRESULT_SHORT;
	*piIndex = FindAssetInTable(*pzATable, sSecNo, sWi, TRUE, iLongShort);
	if (*piIndex == -1)
	{
		zErr = SelectAsset(sSecNo, sWi, iVendorID, &zTempAsset, &zLevels, iLongShort,iID, lTransNo);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		zErr = AddAssetToTable(pzATable, zTempAsset, zLevels, zPSTypeTable, piIndex, lLastPerfDate, lCurrentPerfDate);
	}

	InitializePartialAsset(&zTempAsset);
	return zErr;
} /* FindAndAndCurrency */


/**
** This function construct a 2-dimensional array of CURRENCYRESULT structures.
** The number of element in the first dimension is equal to number of PerfKeys
** in PKeyTable and number of elements in the 2nd dimension is equal to number
** of assets in AssetTable. Element [i][j] in this array correspond to ith
** element in PKeyTable and jth element in AssetTable.
**/
ERRSTRUCT CreatePKeyAssetTable(PKEYASSETTABLE2 *pzPATable, PKEYTABLE zPKTable, ASSETTABLE2 zATable, 
							   PERFASSETMERGETABLE zPAMTable, PERFRULETABLE zRuleTable, long lDate, 
							   PARTPMAIN zPmain, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT	zErr;
	int			i, j, k, l, m, n, o, iResult, iAssetIndex;
	BOOL		bResult, bSpecialEqOrFi;
	PSCRDET		zPSDetail;
	//char		smsg[100];

	lpprInitializeErrStruct(&zErr);

	if (pzPATable->iKeyCount > 0)
		InitializePKeyAssetTable(pzPATable);

	if (zPKTable.iCount == 0 || zATable.iNumAsset == 0)
		return zErr;

	pzPATable->pzStatusFlag = (CURRENCYRESULT2 *)malloc(sizeof(CURRENCYRESULT2) * zPKTable.iCount * zATable.iNumAsset);
	if (pzPATable->pzStatusFlag == NULL)
		return(lpfnPrintError("Insufficient Memory For PerfKey Asset Table", 0, 0, "",
							  997, 0, 0, "CALCPERF CREATEPKEYASSET", FALSE));
	//sprintf_s(smsg, "Memory Address For Perfkey Asset Table is; %Fp", pzPATable->pzStatusFlag);
	//lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG", TRUE);

	for (i = 0; i < zATable.iNumAsset; i++)
	{
		for (j = 0; j < zPKTable.iCount; j++)
		{
			k = (j * zATable.iNumAsset) + i;

			pzPATable->pzStatusFlag[k].piResult = (short *)malloc(sizeof(short) * zATable.pzAsset[i].iDailyCount);
			if (pzPATable->pzStatusFlag[k].piResult == NULL)
				return(lpfnPrintError("Insufficient Memory To Store results For PerfKey Asset Table", 0, 0, "",
									  997, 0, 0, "CALCPERF CREATEPKEYASSET2", FALSE));
			//sprintf_s(smsg, "Memory Address For Result in PKeyAsset Table is; %Fp", pzPATable->pzStatusFlag[i].piResult);
			//lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG", TRUE);

			pzPATable->pzStatusFlag[k].iNumDays = zATable.pzAsset[i].iDailyCount;

			//sprintf_s(smsg, "Memory Address For Ror Table is; %Fp", pzPATable->pzStatusFlag[i].piResult);
			//lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG", TRUE);
		} // j < num keys
	} // i < numassets

	pzPATable->iKeyCount = zPKTable.iCount;
	pzPATable->iNumAsset = zATable.iNumAsset;

	for (i = 0; i < pzPATable->iKeyCount; i++)
	{
		if (strcmp(zPKTable.pzPKey[i].sRecordType, "E") == 0 ||
			strcmp(zPKTable.pzPKey[i].sRecordType, "F") == 0)
			bSpecialEqOrFi = TRUE;
		else
			bSpecialEqOrFi = FALSE;

		for (j = 0; j < pzPATable->iNumAsset; j++)
		{
			k = zPKTable.pzPKey[i].iScrHDIndex;
			m = i * pzPATable->iNumAsset + j;

			// Check for every day from lastperfdate to currentperfdate
			for (n = 0; n < pzPATable->pzStatusFlag[m].iNumDays; n++)
			{
				/*
				** If the key is for weighted average performance, special formulae
				** are used to calculate performance on it, the regular performance
				** should not do any calculations for the key, so fail this key for all
				** the assets.
				*/
				if (strcmp(zPKTable.pzPKey[i].sRecordType, "W") == 0)
				{
					pzPATable->pzStatusFlag[m].piResult[n] = 0;
					continue;
				}

				/*
				** VI # 57941 - If we are currently examining the asset being merged for a date that's between 
				** the begin and end dates on which asset should be merged then fail the key for the asset (being merged)
				*/
				iAssetIndex = CurrentOrMergedAssetIndexForToday(zPAMTable, j, zATable.pzAsset[j].pzDAInfo[n].lDate);
				if (iAssetIndex != j && iAssetIndex > 0)
				{
					pzPATable->pzStatusFlag[m].piResult[n] = 0;
					continue;
				}

				// Test the asset with security currency
				if (bSpecialEqOrFi)
					zErr = TestAsset(zATable.pzAsset[j], g_zSHdrDetTable.pzSHdrDet[k].pzDetail, 1, TRUE, zATable.pzAsset[j].pzDAInfo[n].lDate, 
									 zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
				else
					zErr = TestAsset(zATable.pzAsset[j], g_zSHdrDetTable.pzSHdrDet[k].pzDetail, g_zSHdrDetTable.pzSHdrDet[k].iNumDetail, 
									 TRUE, zATable.pzAsset[j].pzDAInfo[n].lDate, zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
				if (zErr.iBusinessError != 0)
					return zErr;

				if (bResult == FALSE)
					pzPATable->pzStatusFlag[m].piResult[n] = 0;
				else
				{
					iResult = SRESULT_LONG | SRESULT_SHORT;
					for (l = 0; l < g_zSHdrDetTable.pzSHdrDet[k].iNumDetail; l++)
					{
						zPSDetail = g_zSHdrDetTable.pzSHdrDet[k].pzDetail[l];
						if (strcmp(zPSDetail.sComparisonRule, "S") == 0 &&
							strcmp(zPSDetail.sSelectType, "LONGSH") == 0)
						{
							if (strcmp(zPSDetail.sBeginPoint, "SHORT") == 0)
							{
								if (strcmp(zPSDetail.sIncludeExclude, "I") == 0)
									iResult = iResult & SRESULT_SHORT;
								else
									iResult = iResult & SRESULT_LONG;
							}
							else
							{
								if (strcmp(zPSDetail.sIncludeExclude, "I") == 0)
									iResult = iResult & SRESULT_LONG;
								else
									iResult = iResult & SRESULT_SHORT;
							}
						} /* If LongShort test is required */
					} /* for l < NumDetails */

					pzPATable->pzStatusFlag[m].piResult[n] = iResult;
				} /* if Asset Passes the test */

				/*
				** If security currency is same as income currency then today's accrual result is same as today's security result.
				** If it's not the first day and asset has same daily values for all days then today's result is same
				** as yesterday. Even if daily value is not same for every day, it's still possible that today's values
				** are same as yesterday, if that's the case then also today's result is same as yesterday. If any of 
				** these three conditions are not true then test the security with income(accrual) currency.
				*/
				if (strcmp(zATable.pzAsset[j].sCurrId, zATable.pzAsset[j].sIncCurrId) == 0)
				{
					if (pzPATable->pzStatusFlag[m].piResult[n] & SRESULT_LONG)
						pzPATable->pzStatusFlag[m].piResult[n] |= ARESULT_LONG;

					if (pzPATable->pzStatusFlag[m].piResult[n] & SRESULT_SHORT)
						pzPATable->pzStatusFlag[m].piResult[n] |= ARESULT_SHORT;
				}
				else
				{
					if (bSpecialEqOrFi)
						zErr = TestAsset(zATable.pzAsset[j], g_zSHdrDetTable.pzSHdrDet[k].pzDetail, 1, FALSE, zATable.pzAsset[j].pzDAInfo[n].lDate, 
										 zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
					else
						zErr = TestAsset(zATable.pzAsset[j], g_zSHdrDetTable.pzSHdrDet[k].pzDetail, g_zSHdrDetTable.pzSHdrDet[k].iNumDetail, 
										 FALSE, zATable.pzAsset[j].pzDAInfo[n].lDate, zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
					if (zErr.iBusinessError != 0)
						return zErr;

					if (bResult)
					{
						iResult = ARESULT_LONG | ARESULT_SHORT;
						for (l = 0; l < g_zSHdrDetTable.pzSHdrDet[k].iNumDetail; l++)
						{
							zPSDetail = g_zSHdrDetTable.pzSHdrDet[k].pzDetail[l];
							if (strcmp(zPSDetail.sComparisonRule, "S") == 0 &&
								strcmp(zPSDetail.sSelectType, "LONGSH") == 0)
							{
								if (strcmp(zPSDetail.sBeginPoint, "SHORT") == 0)
								{
									if (strcmp(zPSDetail.sIncludeExclude, "I") == 0)
										iResult = iResult & ARESULT_SHORT;
									else
										iResult = iResult & ARESULT_LONG;
								}
								else
								{
									if (strcmp(zPSDetail.sIncludeExclude, "I") == 0)
										iResult = iResult & ARESULT_LONG;
									else
										iResult = iResult & ARESULT_SHORT;
								}
							} /* If LongShort test is required */
						} /* for l < NumDetails */

						pzPATable->pzStatusFlag[m].piResult[n] |= iResult;
					} /* if Asset Passes the test */
				} // if security currency != income currency
			} // for n < NumDays
		} /* for j < numasset */
	} /* for i < numkey */

	/*
	** VI # 57941 - If there is an asset that is being merged into other, we need to make sure all the perfkeys that were valid for asset 
	** being merged are tested for asset being merged into. The loop above will take care of all the perfkeys for both assets being merged 
	** and the one being merged into except the case of a perfkey for individual security. For exaple, if IBM is being merged into MSFT, 
	** the logic above will make sure that between start and end dates of the merge, all perfkeys fail for IBM and all "appropriate" perfkeys 
	** look at MSFT, however, when testing perfkey for MSFT, IBM will fail, even though it should pass so all IBM's flow and market value
	** are added to key for MSFT. the code below will take care of this last scenario for single security perfkey.
	*/
	for (i = 0; i < zPAMTable.iCount; i++)
	{
		// this shouldn't happen, all the entries in perfasset merge table should have valid security index, but if it's invalid, ignore the record
		if (zPAMTable.pzMergedAsset[i].iFromSecNoIndex < 0 || zPAMTable.pzMergedAsset[i].iFromSecNoIndex >= zATable.iNumAsset ||
			zPAMTable.pzMergedAsset[i].iToSecNoIndex < 0 || zPAMTable.pzMergedAsset[i].iToSecNoIndex >= zATable.iNumAsset)
			continue;

		j = zPAMTable.pzMergedAsset[i].iToSecNoIndex;
		if (j < 0 || j > zATable.iNumAsset)
			continue;

		/*
		** go through perfkeytable and find perfpkey for individual security rule matching "security being merged into". For this perfkey and
		** assets being merged into, the perfkeyassettable results must already be true (from the loop above), now make the results true for 
		** this perfkey and "security being merged from".
		*/
		for (k = 0; k < zPKTable.iCount; k++)
		{
			l = FindRule(zRuleTable, zPKTable.pzPKey[k].zPK.lRuleNo);
			m = zPKTable.pzPKey[k].iScrHDIndex;

			// if we found the rule and it's for a single security, then do further checking
			if (l != -1 && strcmp(zRuleTable.pzPRule[l].sWeightedRecordIndicator, "S") == 0)
			{
				// since we are looking for keys created by single security rule, the date dependent industry levels are immaterial, 
				// so just test the asset (against this perfkey) only for first date. If it passes, it should pass for all dates
				zErr = TestAsset(zATable.pzAsset[j], g_zSHdrDetTable.pzSHdrDet[m].pzDetail, g_zSHdrDetTable.pzSHdrDet[m].iNumDetail, 
								 TRUE, zATable.pzAsset[j].pzDAInfo[0].lDate, zPmain, &bResult, lLastPerfDate, lCurrentPerfDate);
				if (bResult)
				{
					iResult = SRESULT_LONG | SRESULT_SHORT | ARESULT_LONG | ARESULT_SHORT;
					// now find all the dates in the "asset being merged from" for which the merge rule is in effect and for all those dates
					// set the perfkey (for the asset being merged into) and asset (being merged from) combination to iResult
					o = zPAMTable.pzMergedAsset[i].iFromSecNoIndex;
					for (n = 0; n < zATable.pzAsset[o].iDailyCount; n++)
					{
						// if date is not between the dates for which the merge rule is valid, noting to do
						if (zATable.pzAsset[o].pzDAInfo[n].lDate < zPAMTable.pzMergedAsset[i].lBeginDate ||
							zATable.pzAsset[o].pzDAInfo[n].lDate > zPAMTable.pzMergedAsset[i].lEndDate)
							continue;

						pzPATable->pzStatusFlag[k * pzPATable->iNumAsset + zPAMTable.pzMergedAsset[i].iFromSecNoIndex].piResult[n] = iResult;
					}// for n
				} //if found the perfkey for asset being merged into
			} // if it's a key created by single security rule
		} // j < zPKTable.icount
	} // loop through performance asset merge table

	return zErr;
} /* createpkeyassettable */


/**
** This function is used to get the market value, accrued interest and accrued
** dividend for a key(or all the keys if iKeyIndex > -1) in perfkey table. If
** the holdings table is not rolled to(and valued at) the date we are interested
** in, then it is recreated by calling FillHoldingsAndAssetTable, also, in this
** case we have to recreate 2D perfkey-asset matrix which tells us whether an
** asset is interested in a perfkey or not.
** NOTE : This function is also used to get values for a new key on last perf
** date. When last perf date is passed to GetDateIndex, it correctly returns
** date index as 0, but in this special case we don't want to change values at
** that array index. So in this case, date index is reset to -1 and values are
** stored at begin MV, AI, AD & Inc instead of being saved at item 0 of daily
** info array of the key.
**/
ERRSTRUCT GetHoldingValues(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable, TRANSTABLE *pzTTable, 
						   PKEYASSETTABLE2 *pzPATable, PKEYTABLE zPKTable, PERFRULETABLE zRuleTable,
						   PARTPMAIN *pzPmain, ACCDIVTABLE zADTable, PERFASSETMERGETABLE zPAMTable,
						   long lDate, long lLastPerfDate, long lCurrPerfDate, char cDoAccruals)
{
	ERRSTRUCT	zErr;
	int			i, j, k, l, m, n, iDateIndex;
	int			iEqKey, iEqPlusCKey, iFiKey, iFiPlusCKey, iCashKey;
	long		lLastMonthEnd;
	BOOL		bHaveValues, bFixedInc, bSResult, bAResult;
	short		iSLongShort, iALongShort, iMDY[3];
	double		fEqPct, fFiPct, fPercent, fEqMV, fFiMV,CostXRate;
	double		fLastEqCash, fLastFiCash, fLastCash, fCurrentCash;

	lpprInitializeErrStruct(&zErr);

	/*
	** pzDInfo variable in all the perfkeys have same date ranges(lLastPerfDate
	** to lCurrentPerfDate), so getting the index out of first key is enough.
	*/
	iDateIndex = GetDateIndex(zPKTable.pzPKey[0], lDate);
	if (iDateIndex < 0 || iDateIndex >= zPKTable.pzPKey[0].iDInfoCount)
		return(lpfnPrintError("Invalid Date Supplied", pzPmain->iID, 0, "", 509, 0,
							  0, "CALCPERF GETHOLD1", FALSE));
	else if (iDateIndex == 0) /* Indicates begining value */
		iDateIndex = -1;

	/*
	** Do a test to figure out whether we already have all the values required.
	** If the function was called before to get the holding values of all the
	** keys(KeyIndex = -1) for the same date, perfkeys will already have the
	** values, no need to redo the whole process.
	*/
	bHaveValues = TRUE;
	j = 0;
	while (j <= zPKTable.iCount - 1 && bHaveValues)
	{
		if (iDateIndex == -1)
		{
			if (zPKTable.pzPKey[j].bGotBeginMVFromHoldings == FALSE)
				bHaveValues = FALSE;
		}
		else if (zPKTable.pzPKey[j].pzDInfo[iDateIndex].bGotMV == FALSE)
			bHaveValues = FALSE;

		j++;
	} /* Do we already have the values ? */

	//fprintf(fp, "GetHoldings Called With %d, Havevalues: %d\n", lDate, bHaveValues);
	/* Already have all the values, we are looking for, nothing to do */
	if (bHaveValues == TRUE)
		return zErr;

	/*
	** If holding is not rolled to(and valued at) the date we are interested in,
	** we have to recreate our holding and PKeyAsset tables and possibly add more
	** assets in the asset table(Ofcourse as a result of all this, we might end up
	** having some entries in the asset table which are not required by any (new)
	** holding record or transaction record, but that's OK).
	*/
	if (pzHTable->lHoldDate != lDate)
	{
		InitializeHoldingTable(pzHTable);

		zErr = FillHoldingAndAssetTables(pzPmain, lDate, pzATable, pzHTable, lLastPerfDate, lCurrPerfDate, cDoAccruals);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		zErr = CreatePKeyAssetTable(pzPATable, zPKTable, *pzATable, zPAMTable, zRuleTable, lDate, *pzPmain, lLastPerfDate, lCurrPerfDate);
		if (zErr.iBusinessError != 0)
			return zErr;

		/*
		** Now that we have the Holdings and Holdcash, we need the accdiv records
		** We then call GetAccruals to fill in the missing accrual records
		*/
		if (cDoAccruals == 'Y')
		{
			zErr = GetAccruals(zADTable, pzATable, pzTTable, pzHTable, pzPmain->iVendorID, lDate, lLastPerfDate, lCurrPerfDate);
			if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
				return zErr;
		}
	} /* if hTable.holddate != ldate */

	/*
	** An outer loop on holdings array and inner loop on perfkey array. If current
	** asset(pointed by current holding's AssetIndex) and perfkey combination's
	** security flag is TRUE(meaning current asset is of interest to current
	** PerfKey), then add current holding's market value to current PerfKey's
	** market value. If current asset and perfkey combination's accrual flag is
	** TRUE, add current holding's accrual to current perfkey's accrual.
	** If iKeyIndex is not -1 then this function updates only perfkey at that
	** index, in this case make sure index is valid.
	*/
	for (i = 0; i < pzHTable->iNumHolding; i++)
	{
		// VI 57941 - get the index of the holdings' asset or if that asset is being merged
		// into another for current date then get the index of merged asset
		//k = pzHTable->pzHold[i].iAssetIndex;
		k = CurrentOrMergedAssetIndexForToday(zPAMTable, pzHTable->pzHold[i].iAssetIndex, lDate);
		l = pzATable->pzAsset[k].iSTypeIndex;
		if (zPSTypeTable.zSType[l].sPrimaryType[0]=='B')
			bFixedInc = TRUE;
		else
			bFixedInc = FALSE;

		if (pzHTable->pzHold[i].fMktVal < 0.0)
		{
			iSLongShort = SRESULT_SHORT;
			iALongShort = ARESULT_SHORT;
		}
		else
		{
			iSLongShort = SRESULT_LONG;
			iALongShort = ARESULT_LONG;
		}

		for (j = 0; j <= zPKTable.iCount - 1; j++)
		{
			m = j * pzPATable->iNumAsset + k;
			n = FindDailyInfoOffset(lDate, pzATable->pzAsset[k], FALSE);
			if (n < 0 || n >= pzATable->pzAsset[k].iDailyCount) // if this asset has no daily info, not interested in this asset
				continue;

			bSResult = pzPATable->pzStatusFlag[m].piResult[n] & iSLongShort;
			bAResult = pzPATable->pzStatusFlag[m].piResult[n] & iALongShort;

			/* if security flag is TRUE, add market value, book value and Xrate */
			if (bSResult)
			{
				if (iDateIndex == -1) /* begining values */
				{
					// SB 10/29/13 VI #54083 - Usually there's no need to read any value from holdings on the last performance day
					// since the beginning values are read from performacne table, however, there are two cases when we need to read 
					// holdings on last performance day. First is when performance is being run for the first time and there is no
					// performance history available for the account and second is if a security changes classification
					// on the first day of the month (e.g. 10/1/13) and performance is being run from previous month end (e.g. 9/30/2103)
					// To avoid double counting the numbers from performance as well as holdings (which will happen In the second case), 
					// a new check is added to make sure numbers from holdings are added only if it's missing from performance
					if (!zPKTable.pzPKey[j].bGotBeginMVFromPerformance)
						zPKTable.pzPKey[j].fBeginMV += pzHTable->pzHold[i].fMktVal;
				}
				else
				{
					zPKTable.pzPKey[j].pzDInfo[iDateIndex].fMktVal += pzHTable->pzHold[i].fMktVal;
					CostXRate = pzHTable->pzHold[i].fBaseCostXrate;
					if (CostXRate == 0) CostXRate = 1;
					zPKTable.pzPKey[j].pzDInfo[iDateIndex].fBookValue += pzHTable->pzHold[i].fTotCost / CostXRate;

					if (strcmp(zPKTable.pzPKey[j].zPK.sCurrProc, "A") == 0)
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fExchRateBase = 1;
					else
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fExchRateBase = pzHTable->pzHold[i].fMvBaseXrate;

					if (lpfnIsItAMonthEnd(zPKTable.pzPKey[j].pzDInfo[iDateIndex].lDate))
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fEstAnnIncome += pzHTable->pzHold[i].fAnnualIncome;
				}
			} /* if security flag is true */

			/*
			** if accrual flag is TRUE, add accruals. If asset's primary type is
			** 'B'(fixed inc) then add accruals to accrued interest, else add it to
			** accrued dividend. Also put new xrate.
			** If tax work is required then do that too.
			*/
			if (bAResult)
			{
				if (iDateIndex == -1) /* begining values */
				{ // added a check to take numbers from holdings only if they are missing from performance, see full explanation
					// above at the place where beginning market value is being used
					if (!zPKTable.pzPKey[j].bGotBeginMVFromPerformance) 
					{
						if (bFixedInc)
							zPKTable.pzPKey[j].fBeginAI += pzHTable->pzHold[i].fAccrInt;
						else
							zPKTable.pzPKey[j].fBeginAD += pzHTable->pzHold[i].fAccrInt;
					}
				} /* dateindex = -1 */
				else
				{
					if (bFixedInc)
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fAccrInc += pzHTable->pzHold[i].fAccrInt;
					else
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fAccrDiv += pzHTable->pzHold[i].fAccrInt;

					if (strcmp(zPKTable.pzPKey[j].zPK.sCurrProc, "A") == 0)
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fExchRateBase = 1.0;
					else
						zPKTable.pzPKey[j].pzDInfo[iDateIndex].fExchRateBase = pzHTable->pzHold[i].fAiBaseXrate;
				} /* dateindex != -1 */

				/* Do the tax work, if required */
				if (TaxInfoRequired(pzPmain->iReturnsToCalculate))
				{
					// Taxfree at Federal Level 
					if (strcmp(pzATable->pzAsset[k].sTaxableCountry, "N") == 0) 
					{
						if (iDateIndex == -1)
						{
							// added a check to take numbers from holdings only if they are missing from performance, see full explanation
							// above at the place where beginning market value is being used
							if (!zPKTable.pzPKey[j].bGotBeginMVFromPerformance)
							{
								if (bFixedInc)
									zPKTable.pzPKey[j].fBegFedetaxAI += pzHTable->pzHold[i].fAccrInt;
								else
									zPKTable.pzPKey[j].fBegFedetaxAD += pzHTable->pzHold[i].fAccrInt;
							}
						} /* Dateindex = -1 */
						else
						{
							if (bFixedInc)
								zPKTable.pzPKey[j].pzTInfo[iDateIndex].fFedetaxAccrInc += pzHTable->pzHold[i].fAccrInt;
							else
								zPKTable.pzPKey[j].pzTInfo[iDateIndex].fFedetaxAccrDiv += pzHTable->pzHold[i].fAccrInt;
						} /* Dateindex = -1 */
					} /* If taxfree at federal level */
					else
					{
						if (iDateIndex == -1)
						{
							// added a check to take numbers from holdings only if they are missing from performance, see full explanation
							// above at the place where beginning market value is being used
							if (!zPKTable.pzPKey[j].bGotBeginMVFromPerformance)
							{
								if (bFixedInc)
									zPKTable.pzPKey[j].fBegFedataxAI += pzHTable->pzHold[i].fAccrInt;
								else if ((strcmp(zPKTable.sDRDElig, "Y") ==0) && (strcmp(pzATable->pzAsset[k].sDRDElig, "Y") == 0))
									zPKTable.pzPKey[j].fBegFedataxAD += pzHTable->pzHold[i].fAccrInt * DRDELIGPERCENTAGE;
								else
									zPKTable.pzPKey[j].fBegFedataxAD += pzHTable->pzHold[i].fAccrInt;
							}
						} /* Dateindex = -1 */
						else
						{
							if (bFixedInc)
								zPKTable.pzPKey[j].pzTInfo[iDateIndex].fFedataxAccrInc += pzHTable->pzHold[i].fAccrInt;
							else
								if ((strcmp(zPKTable.sDRDElig, "Y") ==0) && (strcmp(pzATable->pzAsset[k].sDRDElig, "Y") == 0))
									zPKTable.pzPKey[j].pzTInfo[iDateIndex].fFedataxAccrDiv += pzHTable->pzHold[i].fAccrInt * DRDELIGPERCENTAGE;
								else
									zPKTable.pzPKey[j].pzTInfo[iDateIndex].fFedataxAccrDiv += pzHTable->pzHold[i].fAccrInt;
						} /* Dateindex = -1 */
					} /* If taxable at federal level */

					// Tax at State Level 
					/* state tax calcualtions have been disabled - 5/12/06 - vay
					if (strcmp(pzATable->pzAsset[k].sTaxableState, "N") == 0) 
					{
					if (iDateIndex == -1)
					{
					if (bFixedInc)
					zPKTable.pzPKey[j].fBegStetaxAI += pzHTable->pzHold[i].fAccrInt;
					else
					zPKTable.pzPKey[j].fBegStetaxAD += pzHTable->pzHold[i].fAccrInt;
					} // Dateindex = -1 
					else
					{
					if (bFixedInc)
					zPKTable.pzPKey[j].pzTInfo[iDateIndex].fStetaxAccrInc += pzHTable->pzHold[i].fAccrInt;
					else
					zPKTable.pzPKey[j].pzTInfo[iDateIndex].fStetaxAccrDiv += pzHTable->pzHold[i].fAccrInt;
					} // Dateindex = -1 
					} // If taxfree at state level 
					else
					{
					if (iDateIndex == -1)
					{
					if (bFixedInc)
					zPKTable.pzPKey[j].fBegStataxAI += pzHTable->pzHold[i].fAccrInt;
					else
					zPKTable.pzPKey[j].fBegStataxAD += pzHTable->pzHold[i].fAccrInt;
					} // Dateindex = -1 
					else
					{
					if (bFixedInc)
					zPKTable.pzPKey[j].pzTInfo[iDateIndex].fStataxAccrInc += pzHTable->pzHold[i].fAccrInt;
					else
					zPKTable.pzPKey[j].pzTInfo[iDateIndex].fStataxAccrDiv += pzHTable->pzHold[i].fAccrInt;
					} // Dateindex = -1 
					} // If taxable at state level 

					state tax disabled code ends */

				} /* If tax calculations are required */
			} /* if accrual flag is TRUE */

		} /* for j < iEndj */
	} /* for i < pzHTable->inumHolding */

	/*
	** Everything was successful, set the flags indicating we have market values
	** for the required keys.
	*/
	for (j = 0; j <= zPKTable.iCount - 1; j++)
	{
		if (iDateIndex == -1)
			zPKTable.pzPKey[j].bGotBeginMVFromHoldings = TRUE;
		else 
			zPKTable.pzPKey[j].pzDInfo[iDateIndex].bGotMV = TRUE;
	} /* Now we have the values */

	/*
	** If doing special processing for EQ + EQ Cash and FI + FI Cash, get the percentage of
	** equity and fixed and add appropriate portions of cash to Equity + Equity cash and Fixed
	** + Fixed Cash (Even though the actual script for EQ + EQ Cash has condition to match equity
	** or cash, test asset is called with only the first condition - meaning only equity assets
	** will pass, so cash assets will not pass for equity + equity cash. Same is true for
	** fixed + fixed cash segment.)
	*/
	if (SpecialRuleForEquityAndFixedExists(zPKTable, zRuleTable))
	{
		zErr = GetEquityAndFixedPercent(zPKTable, zRuleTable, *pzPmain, lDate, &fEqPct, &fFiPct);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		// if both percentage are zero, can't do anything
		if (fEqPct == 0 && fFiPct == 0)
			return zErr;

		lpfnrjulmdy(lDate, iMDY);
		//		iCashKey = FindKeyForASegmentType(zPKTable, CASHSEGMENT, lDate);
		//	if (iCashKey < 0) // no key for cash segment on the given date
		//	return zErr;
		lLastMonthEnd = lpfnLastMonthEnd(lDate);

		// Get the indexes of all the keys we are interested in (for last month end)
		iEqKey		= FindKeyForASegmentType(zPKTable, EQUITYSEGMENT, lLastMonthEnd);
		iEqPlusCKey = FindKeyForASegmentType(zPKTable, EQUITYPLUSCASHSEGMENT, lLastMonthEnd);
		iFiKey		= FindKeyForASegmentType(zPKTable, FIXEDSEGMENT, lLastMonthEnd);
		iFiPlusCKey	= FindKeyForASegmentType(zPKTable, FIXEDPLUSCASHSEGMENT, lLastMonthEnd);
		iCashKey	= FindKeyForASegmentType(zPKTable, CASHSEGMENT, lLastMonthEnd);

		iDateIndex = GetDateIndex(zPKTable.pzPKey[0], lLastMonthEnd);
		if (iDateIndex < 0 || iDateIndex >= zPKTable.pzPKey[0].iDInfoCount) // this should not happen
			fLastEqCash = fLastFiCash = fLastCash = 0;
		else
		{
			// if either Equity or Equity plus cah doesn't exist for last month end, equity cash is zero,
			// else it is the difference between equitypluscash and equity market values
			if (iEqKey < 0 && iEqPlusCKey < 0)
				fLastEqCash = 0;
			else if (iEqKey < 0 && iEqPlusCKey >= 0)
				fLastEqCash = zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fMktVal;
			else if (iEqKey >= 0 && iEqPlusCKey < 0)
				fLastEqCash = zPKTable.pzPKey[iEqKey].pzDInfo[iDateIndex].fMktVal; 
			else
				fLastEqCash = zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fMktVal -
				zPKTable.pzPKey[iEqKey].pzDInfo[iDateIndex].fMktVal; 

			// if either Fixed or Fixed plus cah doesn't exist for last month end, Fixed cash is zero,
			// else it is the difference between Fixedpluscash and Fixed market values
			if (iFiKey < 0 && iFiPlusCKey < 0)
				fLastFiCash = 0;
			else if (iFiKey < 0 && iFiPlusCKey >= 0)
				fLastFiCash = zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fMktVal;
			else if (iFiKey >= 0 && iFiPlusCKey < 0)
				fLastFiCash = zPKTable.pzPKey[iFiKey].pzDInfo[iDateIndex].fMktVal;
			else
				fLastFiCash = zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fMktVal -
				zPKTable.pzPKey[iFiKey].pzDInfo[iDateIndex].fMktVal; 

			// get last month end's cash
			if (iCashKey < 0)
				fLastCash = 0;
			else
				fLastCash = zPKTable.pzPKey[iCashKey].pzDInfo[iDateIndex].fMktVal; 
		} // if a valid Date index is found for lastmonthend

		/*
		** Figure out how much money was directed to Equity and Fixed in the current period. That
		** money should directly go to equity + equity cash and fixed + fixed cash, rest of the 
		** money(CASH CHANGE - EQ MV - FI MV, where CASH CHANGE = CURRENT CASH - LAST MONTH END CASH) 
		** should get divided among Equity + Equity Cash and Fixed + Fixed Cash segment based on the 
		** Eq and Fi percents.
		*/
		zErr = EquityAndFixedCashContWithForThePeriod(*pzTTable, *pzATable, lDate, &fEqMV, &fFiMV);
		if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
			return zErr;

		iDateIndex = GetDateIndex(zPKTable.pzPKey[0], lDate);
		// Get the indexes of all the keys we are interested in (for current date)
		iEqKey		= FindKeyForASegmentType(zPKTable, EQUITYSEGMENT, lDate);
		iEqPlusCKey = FindKeyForASegmentType(zPKTable, EQUITYPLUSCASHSEGMENT, lDate);
		iFiKey		= FindKeyForASegmentType(zPKTable, FIXEDSEGMENT, lDate);
		iFiPlusCKey	= FindKeyForASegmentType(zPKTable, FIXEDPLUSCASHSEGMENT, lDate);
		iCashKey	= FindKeyForASegmentType(zPKTable, CASHSEGMENT, lDate);

		// add EqMV to Equity + Cash segment(if it exists)
		if (iEqPlusCKey < 0)
			fEqMV = 0;
		else
		{
			zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fMktVal += fLastEqCash + fEqMV;
			zPKTable.pzPKey[iEqPlusCKey].pzDInfo[iDateIndex].fBookValue += fLastEqCash + fEqMV;
		}

		// add FiMV to Fixed + Cash segment(if it exists)
		if (iFiPlusCKey < 0)
			fFiMV = 0;
		else
		{
			zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fMktVal += fLastFiCash + fFiMV;
			zPKTable.pzPKey[iFiPlusCKey].pzDInfo[iDateIndex].fBookValue += fLastFiCash + fFiMV;
		}

		// Get current cash
		if (iCashKey < 0)
			fCurrentCash = 0;
		else
			fCurrentCash = zPKTable.pzPKey[iCashKey].pzDInfo[iDateIndex].fMktVal;

		// divide rest of the money between two segments based on their percentage
		for (j = 1; j <= 2; j++)
		{
			if (j == 1)
			{
				k = iEqPlusCKey;
				fPercent = fEqPct;
			}
			else
			{
				k = iFiPlusCKey;
				fPercent = fFiPct;
			}

			if (k < 0 || fPercent <= 0) // no key for the segment, skip the segment
				continue;

			zPKTable.pzPKey[k].pzDInfo[iDateIndex].fMktVal += (fCurrentCash - fLastCash - fEqMV - fFiMV) * fPercent;
			zPKTable.pzPKey[k].pzDInfo[iDateIndex].fBookValue += (fCurrentCash - fLastCash - fEqMV - fFiMV) * fPercent;
			if (iCashKey > 0)
			{
				zPKTable.pzPKey[k].pzDInfo[iDateIndex].fAccrInc += zPKTable.pzPKey[iCashKey].pzDInfo[iDateIndex].fAccrInc * fPercent;
				zPKTable.pzPKey[k].pzDInfo[iDateIndex].fAccrDiv += zPKTable.pzPKey[iCashKey].pzDInfo[iDateIndex].fAccrDiv * fPercent;
			}

			// if taxable work is required
			if ((TaxInfoRequired(pzPmain->iReturnsToCalculate)) && iCashKey > 0)
			{
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fFedetaxAccrInc += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fFedetaxAccrInc * fPercent;
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fFedetaxAccrDiv += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fFedetaxAccrDiv * fPercent;
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fFedataxAccrInc += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fFedataxAccrInc * fPercent;
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fFedataxAccrDiv += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fFedataxAccrDiv * fPercent;
				/* state tax calcualtions have been disabled - 5/12/06 - vay
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fStetaxAccrInc += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fStetaxAccrInc * fPercent;
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fStetaxAccrDiv += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fStetaxAccrDiv * fPercent;
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fStataxAccrInc += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fStataxAccrInc * fPercent;
				zPKTable.pzPKey[k].pzTInfo[iDateIndex].fStataxAccrDiv += zPKTable.pzPKey[iCashKey].pzTInfo[iDateIndex].fStataxAccrDiv * fPercent;
				*/
			} // if taxable work is required
		} // for j <= 2
	} // if special eq + eq cash and/or fi + fi cash rule exists for this account

	return zErr;
} /* getholdingvalues */


/**
** Function to get values from daily perform on last perf date and to get begin
** values from monthly perform on (possibly different) last non daily date for
** all the keys in perfkey table.
**/
ERRSTRUCT GetLastAndBeginValues(int iPortfolioID, long lLastPerfDate, PKEYTABLE *pzPKTable, int iReturnsToCalculate)
{
	ERRSTRUCT zErr;
	long      lEarliestNondDate;
	int       i, iNumRec, iNonDelKeys, iKeyIndex, iLastDateIndex, iBeginDateIndex;
	DAILYINFO zTempDaily;
	SUMMDATA  zSummdata;
	TAXPERF		zTP;

	lpprInitializeErrStruct(&zErr);

	/* Date Index is same for all the perfkeys for getting Last Perform Record */
	iLastDateIndex = GetDateIndex(pzPKTable->pzPKey[0], lLastPerfDate);
	if (iLastDateIndex < 0)
		return(lpfnPrintError("Programming Error", iPortfolioID, 0, "", 999, 0, 0, "CALCPERF GETLASTANDBEGIN1", FALSE));

	//lpfnTimer(33);
	iNumRec = 0; /* Number of records read from daily table */
	while (zErr.iSqlError == 0 && zErr.iSqlError != 0) // SB 4/28/2000 - don't want to read from daily anymore
	{
		lpprInitializeErrStruct(&zErr);
		lpprSelectDailySummdata(&zSummdata, iPortfolioID, lLastPerfDate,  &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/* If the record is not for the date we are interested in, skip it */
		if (zSummdata.lPerformDate != lLastPerfDate)
			continue;

		// if key not found, delete the data
		iKeyIndex = FindPerfkeyByID(*pzPKTable, zSummdata.iID, 0, TRUE); 
		if (iKeyIndex < 0)
		{
			//lpfnTimer(34);
			lpprDeleteDSumdata(zSummdata.iID, zSummdata.lPerformDate, zSummdata.lPerformDate, &zErr);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
			//lpfnTimer(35);


			// delete unit values too (daily only)
			if (strcmp(zSummdata.sPerformType, "D") == 0)
			{
				lpprDeleteDailyUnitValueForADate(iPortfolioID, zSummdata.iID, zSummdata.lPerformDate, 0, &zErr);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					return zErr;	
			}

			continue;
		}

		CopySummDataToDailyInfo(zSummdata, &zTempDaily);
		/*M(monthend), F(10% Flow), I(Inception) and T(Termination) are period end*/
		/*
		** SB 3/16/98. Added extra check for period end(lastnond for key = date of
		** fetched record)
		*/
		if (strcmp(zSummdata.sPerformType, "M") == 0 || strcmp(zSummdata.sPerformType, "F") == 0 ||
			strcmp(zSummdata.sPerformType, "I") == 0 || strcmp(zSummdata.sPerformType, "T") == 0 ||
			pzPKTable->pzPKey[iKeyIndex].zPK.lLndPerfDate == zTempDaily.lDate)
			zTempDaily.bPeriodEnd = TRUE;

		pzPKTable->pzPKey[iKeyIndex].pzDInfo[iLastDateIndex] = zTempDaily;
		iNumRec++;
	} /* while no error */
	//lpfnTimer(37);

	/*
	** We have got last perform records for all the keys from Daily database, now
	** get the begin perform records(records on lastnondDate for each key) from
	** the monthly database.
	** Get the earliest non daily date in any key, 
	** but only if this is not daily performance (vay, 8/15/06)
	** 
	*/
	iNonDelKeys = 0;
	if (zSysSet.bDailyPerf && strcmp(pzPKTable->sPerfInterval,"D")==0) 
		lEarliestNondDate = lLastPerfDate;
	else 
	{
		for (i = 0, lEarliestNondDate = lLastPerfDate; i < pzPKTable->iCount; i++)
		{
			if (IsKeyDeleted(pzPKTable->pzPKey[i].zPK.lDeleteDate) == TRUE)
				continue;

			if (pzPKTable->pzPKey[i].zPK.lLndPerfDate < lEarliestNondDate)
				lEarliestNondDate = pzPKTable->pzPKey[i].zPK.lLndPerfDate;

			iNonDelKeys++;
		} /* for i < numpkeys */
	}



	/*
	** If earliest non daily date is same as lastPerfDate, it means for all the
	** keys LastNondDate is LastPerfDate(this will usually happen when
	** LastNondDate in all the keys is a month end), and number of records read
	** from daily DB is same as number of active keys in the perfkey table
	** then no need to read records from monthly DB.
	*/
	if (lEarliestNondDate == lLastPerfDate && iNumRec == iNonDelKeys && iNumRec > 0)
	{
		// If taxrelated calculations need to be done, these values are read from holdings
		//		if (!TaxInfoRequired(pzPKTable->sTaxCalc))
		//	{
		for (i = 0; i < pzPKTable->iCount; i++)
		{
			pzPKTable->pzPKey[i].fBeginMV = pzPKTable->pzPKey[i].pzDInfo[iLastDateIndex].fMktVal;
			pzPKTable->pzPKey[i].fBeginAI = pzPKTable->pzPKey[i].pzDInfo[iLastDateIndex].fAccrInc;
			pzPKTable->pzPKey[i].fBeginAD = pzPKTable->pzPKey[i].pzDInfo[iLastDateIndex].fAccrDiv;
			pzPKTable->pzPKey[i].fBeginInc = pzPKTable->pzPKey[i].pzDInfo[iLastDateIndex].fIncome;
			pzPKTable->pzPKey[i].bGotBeginMVFromPerformance = TRUE;
		}
		//		}
	} /* LastNonddate is equal to LastPerfDate */
	else
	{
		while (zErr.iSqlError == 0)
		{  
			lpprInitializeErrStruct(&zErr);

			lpprSelectPeriodSummdata(&zSummdata, iPortfolioID, lEarliestNondDate, lLastPerfDate, &zErr);
			if (zErr.iSqlError == SQLNOTFOUND)
			{	 
				zErr.iSqlError = 0;
				break;
			}
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			// If key not found and this record is for a date greater than lastperfdate, delete records for this key for any date greater than or equal to today(Summdata.lperfdate)
			iKeyIndex = FindPerfkeyByID(*pzPKTable, zSummdata.iID, 0, TRUE);
			if (iKeyIndex < 0) 
			{
				if (zSummdata.lPerformDate > lLastPerfDate)
				{
					//lpfnTimer(38);
					lpprDeleteSummdata(zSummdata.iID, zSummdata.lPerformDate, -1, &zErr);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						return zErr;

					//lpfnTimer(39);
					lpprDeleteMonthSum(zSummdata.iID, zSummdata.lPerformDate, -1, &zErr);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						return zErr;

					//lpfnTimer(40);

					// delete Period UnitValues for a date range here (what about IPVs?)
					lpprMarkPeriodUVForADateRangeAsDeleted(iPortfolioID, zSummdata.iID, zSummdata.lPerformDate, &zErr);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						return zErr;	
				}

				// skip this record anyway
				continue;
			}

			/*
			** Monthly cursor retrieves all the records for all the perfkeys
			** starting from the earliest non daily date in any key. It is possible that some
			** keys have multiple (fixed) records(in monthly database) but we are
			** interested only in the lastest fixed records for every key, so if a
			** record is found on a date, in which we are not interested in, continue
			**
			** But for daily peformance we will only find last (latest record), if any at all
			** so, this should become our beginning point
			*/
			if (!(zSysSet.bDailyPerf && strcmp(pzPKTable->sPerfInterval,"D")==0) &&
				(pzPKTable->pzPKey[iKeyIndex].zPK.lLndPerfDate != zSummdata.lPerformDate))
				continue;

			CopySummDataToDailyInfo(zSummdata, &zTempDaily);
			zTempDaily.bPeriodEnd = TRUE;
			iBeginDateIndex = GetDateIndex(pzPKTable->pzPKey[iKeyIndex], pzPKTable->pzPKey[iKeyIndex].zPK.lLndPerfDate);
			if (iBeginDateIndex >= 0)
				pzPKTable->pzPKey[iKeyIndex].pzDInfo[iBeginDateIndex] = zTempDaily;

			// If taxrelated calculations need to be done, these values are read from holdings
			//			if (!TaxInfoRequired(pzPKTable->sTaxCalc))
			//		{
			pzPKTable->pzPKey[iKeyIndex].fBeginMV = zTempDaily.fMktVal;
			pzPKTable->pzPKey[iKeyIndex].fBeginAI = zTempDaily.fAccrInc;
			pzPKTable->pzPKey[iKeyIndex].fBeginAD = zTempDaily.fAccrDiv;
			pzPKTable->pzPKey[iKeyIndex].fBeginInc = zTempDaily.fIncome;
			pzPKTable->pzPKey[iKeyIndex].bGotBeginMVFromPerformance = TRUE;
			//	}
		} /* while no error */
	} /* LastNondDate is not equal to LastPerfDate */
	//lpfnTimer(41);

	if (TaxInfoRequired(iReturnsToCalculate))
	{
		while (zErr.iSqlError == 0) 
		{
			lpprInitializeErrStruct(&zErr);
			lpprSelectTaxperf(iPortfolioID, lLastPerfDate, &zTP, &zErr);
			if (zErr.iSqlError == SQLNOTFOUND)
			{
				zErr.iSqlError = 0;
				break;
			}
			else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			iKeyIndex = FindPerfkeyByID(*pzPKTable, zTP.iID, 0, TRUE);
			if (iKeyIndex > -1)
			{
				pzPKTable->pzPKey[iKeyIndex].fBegFedetaxAI = zTP.fFedetaxAccrInc;
				pzPKTable->pzPKey[iKeyIndex].fBegFedetaxAD = zTP.fFedetaxAccrDiv;
				pzPKTable->pzPKey[iKeyIndex].fBegFedataxAI = zTP.fFedataxAccrInc;
				pzPKTable->pzPKey[iKeyIndex].fBegFedataxAD = zTP.fFedataxAccrDiv;
				/* state tax calcualtions have been disabled - 5/12/06 - vay
				pzPKTable->pzPKey[iKeyIndex].fBegStetaxAI = zTP.fStetaxAccrInc;
				pzPKTable->pzPKey[iKeyIndex].fBegStetaxAD = zTP.fStetaxAccrDiv;
				pzPKTable->pzPKey[iKeyIndex].fBegStataxAI = zTP.fStataxAccrInc;
				pzPKTable->pzPKey[iKeyIndex].fBegStataxAD = zTP.fStataxAccrDiv;
				*/
			} // if key found
		} // while no error
	} // if taxwork is required

	// now read Beginning Unit Values	
	zErr = ReadBeginningUnitValues(pzPKTable, iPortfolioID, lEarliestNondDate);

	return zErr;
} /* getlastandbeginvalues */


void CopySummDataToDailyInfo(SUMMDATA zSD, DAILYINFO *pzDInfo)
{
	InitializeDailyInfo(pzDInfo);

	pzDInfo->lDate = zSD.lPerformDate;
	pzDInfo->fMktVal = zSD.fMktVal;
	pzDInfo->fBookValue = zSD.fBookValue;
	pzDInfo->fAccrInc = zSD.fAccrInc;
	pzDInfo->fAccrDiv = zSD.fAccrDiv;
	pzDInfo->fNetFlow = zSD.fNetFlow;
	pzDInfo->fNotionalFlow = zSD.fNotionalFlow;
	pzDInfo->fCumFlow = zSD.fCumFlow;
	pzDInfo->fWtdFlow = zSD.fWtdFlow;
	pzDInfo->fPurchases = zSD.fPurchases;
	pzDInfo->fSales = zSD.fSales;
	pzDInfo->fIncome = zSD.fIncome;
	pzDInfo->fCumIncome = zSD.fCumIncome;
	pzDInfo->fWtdInc = zSD.fWtdInc;
	pzDInfo->fFees = zSD.fFees;
	pzDInfo->fCumFees = zSD.fCumFees;
	pzDInfo->fWtdFees = zSD.fWtdFees;
	pzDInfo->fExchRateBase = zSD.fExchRateBase;
	pzDInfo->lDaysSinceNond = zSD.lDaysSinceNond;
	pzDInfo->fFeesOut = zSD.fFeesOut;
	pzDInfo->fCumFeesOut = zSD.fCumFeesOut;
	pzDInfo->fWtdFeesOut = zSD.fWtdFeesOut;
	pzDInfo->fEstAnnIncome = zSD.fEstAnnIncome;
	pzDInfo->fCNFees = zSD.fCNFees;
	pzDInfo->fCumCNFees = zSD.fCumCNFees;
	pzDInfo->fWtdCNFees = zSD.fWtdCNFees;
} //CopySummDataToDailyInfo


/**
** Function to figure out if we have any scratch dsumdata records(and corresponding drtrnset 
** records) which we can update with new data on Current Perform Date. Daily database has data 
** on the latest two dates for all keys, so on current perf date we should have data on 
** last perf date and day before that the performance was run. Records on the day before 
** Last Perform Date are the scratch records. While searching for scratch record, only same 
** perfkey's records are searched. If for any key, scratch record does not exist, a record is 
** inserted. If a record is found for a key not in table, it is deleted.
**/
ERRSTRUCT GetScratchRecord(PKEYTABLE zPTable, int iPortfolioID, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT	zErr;
	SUMMDATA	zSummdata;
	int			i, j;
	long		lEarliestKeepDate;
	BOOL		bDeleteIt;


	lpprInitializeErrStruct(&zErr);

	bDeleteIt = FALSE;

	// want to keep any record which is for the current month(and last month end), if there is
	// any record in daily files, it should be deleted
	lEarliestKeepDate = lpfnLastMonthEnd(lCurrentPerfDate);
	while (zErr.iSqlError == 0)
	{
		lpprSelectDailySummdata(&zSummdata, iPortfolioID, 0, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
		else
		{
			bDeleteIt = FALSE;
			i = -1;
			while (TRUE)
			{
				// find a key which is not going to be deleted in CalculateReturn function
				j = i + 1;
				i = FindPerfkeyByID(zPTable, zSummdata.iID, j, TRUE); // start from the last key + 1
				if (i < 0 || i == j - 1) // if no more keys found or the same key found as the last time (to avoid infinite loop)
					break;
				else if (!zPTable.pzPKey[i].bDeleteKey) // live key found
					break;
			}
			if (i < 0) // if the key does not exist, delete dsumdata/drtrnset record(s)
			{
				bDeleteIt = TRUE;
				/*
				** SB 5/12/2015 (VI 55741)
				** Previous FindPerfkeyByID function (under above while (true) loop ignores newly created perfkeys when searching 
				** whether a perfkey for current id exist or not. If key is not found, the dsumdata record for perform date was 
				** deleted prior to following change. This caused problem in some cases (VI# 55741). So now, if a perfkey was not 
				** found, still delete dsumdata record, however instead of deleteing all the records from earliest found in dsumdata 
				** to current date, do another check - if a NEW perfkey does exist (i.e. it is being incepted between lastperfdate and
				** current perfdate) then delete only current record, leave any previous day record in dsumdata alone. This change 
				** MAY cause a different problem now - previous code ensured that if a dsumdata on a day prior to current perf date 
				** existed by mistake (e.g. a security was purchased causing a new segment to be created but then purchase was reversed 
				** and hence the data in the new segment shouldn't exist) then all records of that segment got deleted from dsummdata. 
				** Now this new code will delete the data from wrong segment only for current date, not for any previous day in dsummdata. 
				** New code is consistent with the idea that no prior record in dsumdata should be touched, program should fix only 
				** current day record.
				*/ 
				if (FindPerfkeyByID(zPTable, zSummdata.iID, 0, FALSE) >= 0 && zSummdata.lPerformDate < lCurrentPerfDate)
					zSummdata.lPerformDate = lCurrentPerfDate; //delete only current perf date record not any previous records
			}
			else
			{
				if (zSummdata.lPerformDate > lCurrentPerfDate) // any record greater than current perf date, should be deleted
					bDeleteIt = TRUE;
				else if (zPTable.pzPKey[i].lScratchDate == 0)
				{
					if (zSummdata.lPerformDate == lCurrentPerfDate)
						zPTable.pzPKey[i].lScratchDate = zSummdata.lPerformDate;
					else if (zSummdata.lPerformDate < lEarliestKeepDate)
						zPTable.pzPKey[i].lScratchDate = zSummdata.lPerformDate;
				}
				else if (zSummdata.lPerformDate == zPTable.pzPKey[i].lScratchDate) // found another record for the same date and id
				{
					bDeleteIt = TRUE; // there is no way to delete one record and leave the other, so delete both and reset scratch date
					zPTable.pzPKey[i].lScratchDate = 0;
				}
				else if (zSummdata.lPerformDate < lEarliestKeepDate)
					bDeleteIt = TRUE;
			} // if key found in perfkey

			if (bDeleteIt)
			{
				lpprDeleteDSumdata(zSummdata.iID, zSummdata.lPerformDate, zSummdata.lPerformDate, &zErr);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					return zErr;

				// delete unit values too (daily only)
				if (strcmp(zSummdata.sPerformType, "D") == 0)
				{
					lpprDeleteDailyUnitValueForADate(iPortfolioID, zSummdata.iID, zSummdata.lPerformDate, 0, &zErr);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						return zErr;
				}
			} // if record is to be deleted from daily table
		} // no error in selecting daily ror
	} /* while no error in daily loop*/

	return zErr;
} /* getscratchrecord */


ERRSTRUCT	FillPorttaxTable(int iID, long lLastPerfDate, long lCurrentPerfDate, PORTTAXTABLE *pzPTaxTable)
{
	ERRSTRUCT zErr;
	PORTTAX		zPTax;
	PORTTAX		zPTaxDefault;

	lpprInitializeErrStruct(&zErr);
	InitializePorttax(&zPTaxDefault, lCurrentPerfDate);

	while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
	{
		lpprSelectAllPorttax(iID, lCurrentPerfDate, &zPTax, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/*
		** The query gives all the porttax records for dates less than or equal to CurrentPerfDate
		** in descending order. If the record is for a date between lastperfdate and currentperfdate,
		** add it to the table, else if there is nothing in the table, add the record (even if it is 
		** prior to LastPerfDate), else (on or prior to lastperfdate)
		*/
		// verify correctnes of user supplied values, if not correct then default it 
		if (zPTax.fFedIncomeRate < 0 || zPTax.fFedIncomeRate > 100)
			zPTax.fFedIncomeRate = zPTaxDefault.fFedIncomeRate; 

		if (zPTax.fFedLtGLRate < 0 || zPTax.fFedLtGLRate > 100)
			zPTax.fFedLtGLRate = zPTaxDefault.fFedLtGLRate;

		if (zPTax.fFedStGLRate < 0 || zPTax.fFedStGLRate > 100)
			zPTax.fFedStGLRate = zPTaxDefault.fFedStGLRate;

		if (zPTax.lTaxDate > lLastPerfDate && zPTax.lTaxDate <= lCurrentPerfDate)
			zErr = AddPorttaxToTable(pzPTaxTable, zPTax);
		else if (pzPTaxTable->iCount == 0)
			zErr = AddPorttaxToTable(pzPTaxTable, zPTax);
		// need only the latest date which is less than the begining date, so if a date is
		// already in the memory meeting that criteria, don't bother adding more
		else if (zPTax.lTaxDate <= lLastPerfDate &&
			pzPTaxTable->pzPTax[pzPTaxTable->iCount - 1].lTaxDate > lLastPerfDate)
			zErr = AddPorttaxToTable(pzPTaxTable, zPTax);
		/* 08/02/2002 - vay
		**	but we still have to scroll through entire Recordset opened in StarsIO 
		**	in order to let it close, overwise if you re-run performance on same account/day again 
		**	it is causing wrong data to be picked up (by still opened StarsIO.SelectAllPorttax)
		**	or access violation in newest OLEDBIO (when internal roll/valuaition happens, database transaction 
		**	commit operation causes to invalidate currently open recordsets - i.e. state of SelectAllPorttax 
		**	becomes undefined and you cannot scroll through it anymore)

		**	So, you must not break out of the loop here !
		*/
		//		else // only dates less than last perfdate are left (and we already have ONE date lastperfdate in memory)
		//			break;

	} // while

	return zErr;
} // FillPorttaxTable


/**
** This function is used to get rid of all the keys(and related summdata/dsummdata,
** rtrnset/drtrnset) from the memory(mark as being deleted) and database tables which
** were incepted between lastperfdate(excluded) and currentperfdate(included).
** Also it undeletes(in the memory table) any key which was deleted between
** those date ranges. This is required before rerunning the performance on an
** account. When a key is undeleted, its termination record should be removed
** from the monthly database, but that will be taken care of at a later
** stage(CalculateReturnForAllKeys function) when the records for all the active
** keys are deleted from monthly database(between LastPDate+1 and CurrentPDate).
**/
ERRSTRUCT DeleteKeysMemoryDatabase(PKEYTABLE *pzPTable, long lLastPerfDate, long lCurrentPerfDate)
{
	ERRSTRUCT	zErr;
	SUMMDATA	zSummdata;
	int			i, j, iDeleteIndex, iLastID;
	long		lBeginDate;

	lpprInitializeErrStruct(&zErr);

	if (pzPTable->iCount == 0)
		return zErr;

	/*
	** Get all the monthly perform records(for the account) between
	** Last Perf Date + 1 and Current Perf Date.
	*/
	iLastID = -1;
	while (zErr.iSqlError == 0)
	{ 
		/* 
		** Get all the monthly perform records(for the account) between
		** Last Perf Date + 1 and Current Perf Date.
		*/
		lpprSelectPeriodSummdata(&zSummdata, pzPTable->pzPKey[0].zPK.iPortfolioID, lLastPerfDate + 1, lCurrentPerfDate, &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/*
		** If this ID is same as the last id, all the records for this ID that need to be 
		** deleted have already been deleted, so continue.
		*/
		if (zSummdata.iID == iLastID)
			continue;

		zErr = DeleteDataIfNew(*pzPTable, zSummdata.iID, lLastPerfDate, lCurrentPerfDate, FALSE);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		iLastID = zSummdata.iID;
	} /* While loop for select monthly summdata */

	/*
	** Get all the daily perform records(for the account) with
	** Date > Last Perf Date and <= lCurrent Perf Date
	*/
	while (zErr.iSqlError == 0)
	{
		lpprSelectDailySummdata(&zSummdata, pzPTable->pzPKey[0].zPK.iPortfolioID, lLastPerfDate + 1,  &zErr);
		if (zErr.iSqlError == SQLNOTFOUND)
		{   
			zErr.iSqlError = 0;
			break;
		}
		else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		/*
		** If this ID is same as the last id, all the records for this ID that need to be 
		** deleted have already been deleted, so continue.
		*/
		if (zSummdata.iID == iLastID)
			continue;

		zErr = DeleteDataIfNew(*pzPTable, zSummdata.iID, lLastPerfDate, lCurrentPerfDate, TRUE);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		iLastID = zSummdata.iID;
	} /* While loop for selecting daily summdata */

	/*
	** Now go through the in-memory Perfkey Table and if a key is found which was
	** incepted in LastPerfDate+1 & CurrentPerfDate range then delete it from
	** database table and mark it as deleted in the in-memory table else if a key
	** is found which was deleted in this date range, undelete it in the in-memory
	** table(deletion from Database will occur later).
	*/

	// SB 5/6/2013 - Don't know why the following condition for "daily" performance is going to last month end and deleting
	// a perfkey that system shouldn't touch. It doesn't make sense to do this, so getting rid of if condition and making
	// the begin date same for both daily and month to date performance.
	// Here is a daily example where the condition was wrong. Consider the case where a key was incepted on 2/26/2013. If user
	// was running performance from 3/1/2013 onwards, the last perfdate would be 2/28/2013 and the "original if" condition 
	// would have made the begin date to be 1/31/2013. Now the condition in the loop would think it's a new key and delete it, 
	// even though the key was incepted prior to last perf date.

	// if Daily performance, keep latest terminated key as undeleted 
	//if (zSysSet.bDailyPerf && strcmp(pzPTable->sPerfInterval, "D")==0)
	//lBeginDate = lpfnLastMonthEnd(lLastPerfDate);
	//else
	lBeginDate = lLastPerfDate;

	for (i = 0; i < pzPTable->iCount; i++)
	{
		if (strcmp(pzPTable->pzPKey[i].sRecordType, "W") == 0 &&
			strcmp(pzPTable->pzPKey[i].zPK.sParentChildInd, "P") == 0)
			continue;
		/*
		** SB 5/12/2015 (VI 55741)
		** The purpose of following set of statements is to mark any new perfkey deleted in the memory as well as from the database.
		** Prior to current change, the condition was identifying new keys as the one that's created between last perf date
		** and current perf date. This condition works fine if performance is being run till the last day (pricing or trade date)
		** of the current month, however, if performance is being run till an earlier day in the month and new key was incepted
		** between that day and pricing date, key was not being found and system would end up creating two keys rather than one.
		** Now, new condition doesn't check for inception to be less than current perf date, as long as inception is after the
		** last perf date, it is deleted. Similarly change us made in the else part of the statement that marks a key that was deleted
		** after last perf date, as a live key.
		** //	if (lBeginDate < pzPTable->pzPKey[i].zPK.lInitPerfDate)
		** //		pzPTable->pzPKey[i].zPK.lInitPerfDate <= lCurrentPerfDate)
		*/
		if (lBeginDate < pzPTable->pzPKey[i].zPK.lInitPerfDate)
		{
			if (pzPTable->pzPKey[i].zPK.lLastPerfDate == 0) // SB - 2/11/99 changed to take care of
				pzPTable->pzPKey[i].zPK.lDeleteDate = lLastPerfDate; // test w/o database transactions(last perf date on the key remains 0 if there is an error and since we have no rollbak transaction that's what get put in DB).
			else if (pzPTable->pzPKey[i].zPK.lLastPerfDate <= lCurrentPerfDate)
				pzPTable->pzPKey[i].zPK.lDeleteDate = pzPTable->pzPKey[i].zPK.lLastPerfDate;
			else
				pzPTable->pzPKey[i].zPK.lDeleteDate = lCurrentPerfDate;

			pzPTable->pzPKey[i].bDeletedFromDB = TRUE;

			// physically delete the perfkey from the database
			lpprDeletePerfkey(pzPTable->pzPKey[i].zPK.lPerfkeyNo, &zErr);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;
		} /* If a newly incepted key */
		else if (lBeginDate < pzPTable->pzPKey[i].zPK.lDeleteDate) 
			pzPTable->pzPKey[i].zPK.lDeleteDate = 0;
	} /* i < pzPTable->iCount */

	/*
	** Following situation will happen when rerunning performance past current
	** LndPerfDate(MePerfDate). In this case, correct the date(s) to a date
	** which is less than or equal to LastPerfDate.
	*/
	for (i = 0; i < pzPTable->iCount; i++)
	{
		if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate) == TRUE)
			continue;

		while (pzPTable->pzPKey[i].zPK.lLndPerfDate > lLastPerfDate)
			pzPTable->pzPKey[i].zPK.lLndPerfDate = lpfnLastMonthEnd(pzPTable->pzPKey[i].zPK.lLndPerfDate);

		while (pzPTable->pzPKey[i].zPK.lMePerfDate > lLastPerfDate)
			pzPTable->pzPKey[i].zPK.lMePerfDate = lpfnLastMonthEnd(pzPTable->pzPKey[i].zPK.lMePerfDate);
	} /* change LndPerfDate, if required */

	/*
	** Sometimes, after un-deleting a key, more than one key becomes active, to take care of that 
	** situation, delete the latest key.
	*/
	for (i = 0; i < pzPTable->iCount; i++)
	{
		// if the key is already deleted, nothing to do
		if (IsKeyDeleted(pzPTable->pzPKey[i].zPK.lDeleteDate))
			continue;

		for (j = i + 1; j < pzPTable->iCount; j++)
		{
			// if the key is already deleted, nothing to do
			if (IsKeyDeleted(pzPTable->pzPKey[j].zPK.lDeleteDate))
				continue;

			// if two keys are not for the same script header, nothing to do
			if (pzPTable->pzPKey[i].zPK.lScrhdrNo != pzPTable->pzPKey[j].zPK.lScrhdrNo)
				continue;

			// If control comes here, we have (at least) two active keys belonging to the same script, 
			// delete the key which was incepted later
			if (pzPTable->pzPKey[j].zPK.lInitPerfDate < pzPTable->pzPKey[i].zPK.lInitPerfDate)
				iDeleteIndex = i;
			else
				iDeleteIndex = j;


			pzPTable->pzPKey[iDeleteIndex].zPK.lDeleteDate = lLastPerfDate;
			pzPTable->pzPKey[iDeleteIndex].bDeletedFromDB = TRUE;

			// physically delete the perfkey from the database
			lpprDeletePerfkey(pzPTable->pzPKey[iDeleteIndex].zPK.lPerfkeyNo, &zErr);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			// if ith key is being deleted, don't compare any more keys with this key
			if (i == iDeleteIndex)
				break; 
		} // for j
	} // for i

	return zErr;
} /* DeleteKeysMemoryDatabase */

/**
** Function to delete summdata/dsumdata and rtrnset/drtrnset for the given perfkey, if
** the key was incepted between Last Perf Date + 1 and Current Perf Date.
**/
ERRSTRUCT DeleteDataIfNew(PKEYTABLE zPTable, int iID, long lLastPerfDate, long lCurrentPerfDate, BOOL bDaily)
{
	ERRSTRUCT zErr;
	int       iKeyIndex;
	//  long      lTempDate;

	lpprInitializeErrStruct(&zErr);

	if (zPTable.iCount == 0)
		return zErr;

	iKeyIndex = FindPerfkeyByID(zPTable, iID, 0, FALSE);
	if (iKeyIndex < 0) /* Perform without a  valid perfkey, although this should*/
		return zErr;     /* be cleaned up, for the time being skip it */

	/* If the key is a weighted parent, skip it */
	if (strcmp(zPTable.pzPKey[iKeyIndex].zPK.sParentChildInd, "P") == 0 &&
		strcmp(zPTable.pzPKey[iKeyIndex].sRecordType, "W") == 0)
		return zErr;

	/*
	** If LastPerfDate < Init_perf_date <= lCurrentPerfDate, and
	**    LastPerfDate < Record's lDate <= lCurrentPerfDate, delete this record.
	** Since the SQL gets records between LastPerfDate+1 and CurrentPerfDate,
	** the second check is automatic, we don't have to do it again.
	*/
	if (lLastPerfDate >= zPTable.pzPKey[iKeyIndex].zPK.lInitPerfDate ||
		zPTable.pzPKey[iKeyIndex].zPK.lInitPerfDate > lCurrentPerfDate)
		return zErr;


	/* Delete the record for this perform_no from gfror, nfror, perform, etc */
	if (bDaily)
	{
		/*for (lTempDate = lLastPerfDate + 1; lTempDate <= lCurrentPerfDate; lTempDate++)
		{
		lpprDeleteDSumdata(zPTable.pzPKey[iKeyIndex].zPK.iID, lTempDate, lTempDate, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

		lpprDeleteDailyUnitValueForADate(zPTable.pzPKey[iKeyIndex].zPK.iPortfolioID,
		zPTable.pzPKey[iKeyIndex].zPK.iID, lTempDate, 0, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;
		} // while*/

		/*
		** SB 5/12/2015 (VI# 55741)
		** If it's a true linked daily account then we can delete dusmdata and daily unitvalue from the earliest date forward,
		** otherwise delete data only for current date
		*/
		if (zSysSet.bDailyPerf && strcmp(zPTable.sPerfInterval, "D") == 0) 
			lpprDeleteDSumdata(zPTable.pzPKey[iKeyIndex].zPK.iID, lLastPerfDate + 1, lCurrentPerfDate, &zErr);
		else
			lpprDeleteDSumdata(zPTable.pzPKey[iKeyIndex].zPK.iID, lCurrentPerfDate, lCurrentPerfDate, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		if (zSysSet.bDailyPerf && strcmp(zPTable.sPerfInterval, "D") == 0) 
			lpprDeleteUnitValueSince2(zPTable.pzPKey[iKeyIndex].zPK.iPortfolioID, zPTable.pzPKey[iKeyIndex].zPK.iID, 
			lLastPerfDate + 1, 0, &zErr);
		else
			lpprDeleteUnitValueSince2(zPTable.pzPKey[iKeyIndex].zPK.iPortfolioID, zPTable.pzPKey[iKeyIndex].zPK.iID, 
			lCurrentPerfDate, 0, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;
	} // if daily
	else
	{
		lpprDeleteSummdata(zPTable.pzPKey[iKeyIndex].zPK.iID, lLastPerfDate + 1, -1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		lpprDeleteMonthSum(zPTable.pzPKey[iKeyIndex].zPK.iID, lLastPerfDate + 1, -1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;

		lpprMarkPeriodUVForADateRangeAsDeleted(zPTable.pzPKey[iKeyIndex].zPK.iPortfolioID,
			zPTable.pzPKey[iKeyIndex].zPK.iID, lLastPerfDate + 1, &zErr);
		if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
			return zErr;	
	}// monthly

	return zErr;
} /* DeleteKeyIfNew */

/**
** This function returns LONGSH_LONG or LONGSH_SHORT depending on whether the
** passed transaction is Long or Short. This function does a check for Short
** only and if the transaction passes the test, it is a short transaction, in
** all other cases it is automatically assumes to be a Long transaction.
** Following are the short transactions :
**    TranType "MP"
**    TranType "LD" and DrCr is "DR"
**    Tran Code "O"(open) or "S"(split) and SecImpact -1
**    Tran Code "C"(close) and SecImpact 1
**    Tran Code "I"(income) and DrCr is "DR"(except "MI" which is always long &
**                                           "MP" which is always short)
**/
short LongShortBitForTrans(PARTTRANS zTR)
{
	int i;

	if (strcmp(zTR.sTranType, "MI") == 0)
		return SRESULT_LONG | ARESULT_LONG;
	else if (strcmp(zTR.sTranType, "MP") == 0)
		return SRESULT_SHORT | ARESULT_SHORT;
	else if (strcmp(zTR.sTranType, "LD") == 0)
	{
		if (strcmp(zTR.sDrCr, "DR") == 0)
			return SRESULT_SHORT | ARESULT_SHORT;
		else
			return SRESULT_LONG | ARESULT_LONG;
	}

	i = zTR.iTranTypeIndex;
	if (i < 0 || i >= zTTypeTable.iNumTType)
		return SRESULT_LONG | ARESULT_LONG;

	if ( (strcmp(zTTypeTable.zTType[i].sTranCode, "C") == 0 &&
		zTTypeTable.zTType[i].lSecImpact == 1) ||
		((strcmp(zTTypeTable.zTType[i].sTranCode, "O") == 0 ||
		strcmp(zTTypeTable.zTType[i].sTranCode, "S") == 0) &&
		zTTypeTable.zTType[i].lSecImpact == -1) ||
		(strcmp(zTTypeTable.zTType[i].sTranCode, "I") == 0 &&
		strcmp(zTTypeTable.zTType[i].sDrCr, "DR") == 0) )
		return SRESULT_SHORT | ARESULT_SHORT;
	else
		return SRESULT_LONG | ARESULT_LONG;

	/*if (strcmp(zTR.sTranType, "MI") == 0)
	return LONGSH_LONG;
	else if (strcmp(zTR.sTranType, "MP") == 0)
	return LONGSH_SHORT;
	else if (strcmp(zTR.sTranType, "LD") == 0)
	{
	if (strcmp(zTR.sDrCr, "DR") == 0)
	return LONGSH_SHORT;
	else
	return LONGSH_LONG;
	}

	i = zTR.iTranTypeIndex;
	if (i < 0 || i >= zTTypeTable.iNumTType)
	return LONGSH_LONG;

	if ( (strcmp(zTTypeTable.zTType[i].sTranCode, "C") == 0 &&
	zTTypeTable.zTType[i].lSecImpact == 1) ||
	((strcmp(zTTypeTable.zTType[i].sTranCode, "O") == 0 ||
	strcmp(zTTypeTable.zTType[i].sTranCode, "S") == 0) &&
	zTTypeTable.zTType[i].lSecImpact == -1) ||
	(strcmp(zTTypeTable.zTType[i].sTranCode, "I") == 0 &&
	strcmp(zTTypeTable.zTType[i].sDrCr, "DR") == 0) )
	return LONGSH_SHORT;
	else
	return LONGSH_LONG;*/

} /* LongShortBitForTrans */


ERRSTRUCT GenerateNotionalFlow(ASSETTABLE2 *pzATable, HOLDINGTABLE *pzHTable, TRANSTABLE *pzTTable,
							   PKEYASSETTABLE2 *pzPATable, PKEYTABLE *pzPTable, PERFRULETABLE zRuleTable, 
							   PARTPMAIN *pzPmain, ACCDIVTABLE zADTable, PERFASSETMERGETABLE zPAMTable)
{
	ERRSTRUCT	zErr;
	int				i, j, k, l, iKeyDateIndex, iFromAssetIndex, iToAssetIndex, iFromDateIndex, iToDateIndex; //, m
	long			lLastPerfDate, lCurrentPerfDate;
	char			smsg[100];
	BOOL			bThisSResult, bPreviousSResult, bThisAResult, bPreviousAResult;

	lpprInitializeErrStruct(&zErr);

	lLastPerfDate = pzPTable->pzPKey[0].pzDInfo[0].lDate;
	lCurrentPerfDate = pzPTable->pzPKey[0].pzDInfo[pzPTable->pzPKey[0].iDInfoCount-1].lDate;

	/*
	** Go through all the assets for each day check if any of the dailyassetinfo (e.g. industry level) is different, 
	** if it is, on that day we need to gennerate notional flow equal to previous day's market value. For previous day 
	** if we don't have market value for that asset then do a roll/valuation for that day. Note that even though the
	** roll/valuation is called from one asset, market value on that is saved for all assets that have a record
	** in daily info array for that day, so we won't need to call roll/valuation again for the any other asset that 
	** needs market value on same day.
	*/
	for (i = 0; i < pzATable->iNumAsset; i++)
	{
		for (j = 1; j < pzATable->pzAsset[i].iDailyCount; j++)
		{
			// not interested in any date that's on or prior to last perf date or after current perf date
			if (pzATable->pzAsset[i].pzDAInfo[j].lDate <= lLastPerfDate || pzATable->pzAsset[i].pzDAInfo[j].lDate > lCurrentPerfDate)
				continue;

			// not interested in any date that's on or prior to portfolio's inception date
			if (pzATable->pzAsset[i].pzDAInfo[j].lDate <= pzPmain->lInceptionDate)
				continue;

			// notional flow will be generated only when a value changes, so if this day's values are same as previous record, nothing to do
			if (IsThisDailyValuesSameAsPrevious(pzATable->pzAsset[i], j))
				continue;

			/*
			** Now we know today's values are definately different than previous values. When an industry level changes on a day
			** and that day gets added to the array, its previous day is also supposed to be added to the array if the day being
			** added is between lastperfdate and currentperfdate. Also, becuase of the fact that the dates in the array are stored 
			** in ascending order, the previous record in the array should be for previous day, if it's not then return with an error
			*/
			if (pzATable->pzAsset[i].pzDAInfo[j].lDate != pzATable->pzAsset[i].pzDAInfo[j-1].lDate+1)
			{
				sprintf_s(smsg, "For security %s, date %d, record on previous day not found", pzATable->pzAsset[i].sSecNo, pzATable->pzAsset[i].pzDAInfo[j].lDate);
				return(lpfnPrintError(smsg, pzPTable->pzPKey[0].zPK.iID, 0, "", 999, 0, 0, "CALCPERF GENNOTIONALFLOW", FALSE));
			}

			// if don't have market value on previous day, do a roll/valuation for previous day
			if (!pzATable->pzAsset[i].pzDAInfo[j-1].bGotMV)
			{
				zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable, *pzPTable, zRuleTable, pzPmain, zADTable, 
										zPAMTable, pzATable->pzAsset[i].pzDAInfo[j-1].lDate, lLastPerfDate, lCurrentPerfDate, cCalcAccdiv);
				if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
					return zErr;
			} // if don't have market value on previous day then get it

			/*
			** Today will be a period end (IPV if not a month end) for all segments affected by industry level cahange,
			** so in addition to previous day, get today's market value also
			*/
			zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable, *pzPTable, zRuleTable, pzPmain, zADTable, 
									zPAMTable, pzATable->pzAsset[i].pzDAInfo[j].lDate, lLastPerfDate, lCurrentPerfDate, cCalcAccdiv);
			if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
				return zErr;

			//fprintf(fp, "Value for Sec: %s, on day: %d and previous day are: %f and %f\n", pzATable->pzAsset[j].sSecNo,
			//					lDate, pzATable->pzAsset[i].pzDAInfo[j].fMktVal, pzATable->pzAsset[i].pzDAInfo[j-1].fMktVal);

			iKeyDateIndex = GetDateIndex(pzPTable->pzPKey[0], pzATable->pzAsset[i].pzDAInfo[j].lDate);

			for (k = 0; k < pzPTable->iCount; k++)
			{
				if (pzPTable->pzPKey[k].bDeleteKey)
					continue;

				if (IsKeyDeleted(pzPTable->pzPKey[k].zPK.lDeleteDate))
					continue;

				/*
				** VI# 43738 SB 2/18/2010. 
				** If this is a new key, it's possible inception date for this is going to be after the 
				** current date and if that's the case, this key should not have a notional flow today.
				*/

				/* 
				** VI 52823, SB 6/6/2013
				** The fix that was put on 2/18/2010 for VI# 43738 is not correct. If new key is being created becuase of a industry
				** level change and not becuase the account bought the security, then it will have no flow. If notional flow is not 
				** added for this new key then initial return and gain loss will be wrong. So, at this point, commenting out the 
				** previous logic that was put in 2010. 
				** The situation in VI 43738 (security changes classification on a given day but it was not held in the account
				** until a later date) that the following code was trying to fix, could be fixed another way. If the account
				** doesn't hold the security until a later day, notional flow on the day security changed classification
				** will be zero, so even if a zero value is added to the key, it shouldn't create a problem. the only issue could
				** be that the piece of code (later in this function) that's adding the notional flow is also setting this particular
				** day to be a period end and for new keys, it's changing inception date. Now (6/6/2013) added another check to this, 
				** make this a period end AND change inception date for new keys only if the notional flow being added to the key is 
				** NON-ZERO. This change should take care of both VI# 43738 and 52823.
				*/

				/*if (pzPTable->pzPKey[k].bNewKey)
				{
				bFirstFlowFound = FALSE;
				m = 1;
				while (m <= iDateIndex && !bFirstFlowFound)
				{
				if (!IsValueZero(pzPTable->pzPKey[k].pzDInfo[m].fNetFlow , 3))
				bFirstFlowFound = TRUE;

				m++;
				}

				// if no flow prior to today, there should be no notional flow today
				if (!bFirstFlowFound)
				continue;
				}*/

				l = (k * pzATable->iNumAsset + i);

				// Even though the market value for the security should be divided into short and long and the check
				// also should be done accordingly, for now (because there are no template in use that have rule seperating
				// longs and shorts) combine short and long into one
				bThisSResult = pzPATable->pzStatusFlag[l].piResult[j] & (SRESULT_LONG | SRESULT_SHORT);
				bPreviousSResult = pzPATable->pzStatusFlag[l].piResult[j-1] & (SRESULT_LONG | SRESULT_SHORT);

				bThisAResult = pzPATable->pzStatusFlag[l].piResult[j] & (ARESULT_LONG | ARESULT_SHORT);
				bPreviousAResult = pzPATable->pzStatusFlag[l].piResult[j-1] & (ARESULT_LONG | ARESULT_SHORT);

				AddNotionalFlowToTheKey(pzPTable, *pzATable, k, iKeyDateIndex, i, j, bThisSResult, bPreviousSResult, 
										bThisAResult, bPreviousAResult);
				/*
				** if today security passes and yesterday it didn't then add a flow in, else if today security fails but
				** yesterday it passed then add a flow out, if neither of these conditions are met, nothing to do
				* /
				if (bThisSResult && !bPreviousSResult)
				{
					pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow += pzATable->pzAsset[i].pzDAInfo[j-1].fMktVal;

					// SB 6/6/2013 - see explanation above for VI 52823
					if (!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2))
						pzPTable->pzPKey[k].pzDInfo[iDateIndex].bPeriodEnd = TRUE;

					// SB 6/6/2013 - see explanation above for VI 52823
					if (pzPTable->pzPKey[k].bNewKey == TRUE && pzPTable->pzPKey[k].bDeleteKey == FALSE &&
						pzPTable->pzPKey[k].zPK.lInitPerfDate > pzATable->pzAsset[i].pzDAInfo[j].lDate &&
						!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2))  
					{
						pzPTable->pzPKey[k].zPK.lInitPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
						pzPTable->pzPKey[k].zPK.lLndPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
					}
				}
				else if (!bThisSResult && bPreviousSResult)
				{
					pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow -= pzATable->pzAsset[i].pzDAInfo[j-1].fMktVal;

					// SB 6/6/2013 - see explanation above for VI 52823
					if (!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2))
						pzPTable->pzPKey[k].pzDInfo[iDateIndex].bPeriodEnd = TRUE;

					// SB 6/6/2013 - see explanation above for VI 52823
					if (pzPTable->pzPKey[k].bNewKey == TRUE && pzPTable->pzPKey[k].bDeleteKey == FALSE &&
						pzPTable->pzPKey[k].zPK.lInitPerfDate > pzATable->pzAsset[i].pzDAInfo[j].lDate &&
						!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2)) 
					{
						pzPTable->pzPKey[k].zPK.lInitPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
						pzPTable->pzPKey[k].zPK.lLndPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
					}
				}				

				// do the same for accrual flow
				if (bThisAResult && !bPreviousAResult)
				{
					pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow += pzATable->pzAsset[i].pzDAInfo[j-1].fAI;
					pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow += pzATable->pzAsset[i].pzDAInfo[j-1].fAD;

					// SB 6/6/2013 - see explanation above for VI 52823
					if (!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2))
						pzPTable->pzPKey[k].pzDInfo[iDateIndex].bPeriodEnd = TRUE;

					if (pzPTable->pzPKey[k].bNewKey == TRUE && pzPTable->pzPKey[k].bDeleteKey == FALSE &&
						pzPTable->pzPKey[k].zPK.lInitPerfDate > pzATable->pzAsset[i].pzDAInfo[j].lDate &&
						!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2)) 
					{
						pzPTable->pzPKey[k].zPK.lInitPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
						pzPTable->pzPKey[k].zPK.lLndPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
					}
				}
				else if (!bThisAResult && bPreviousAResult)
				{
					pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow -= pzATable->pzAsset[i].pzDAInfo[j-1].fAI;
					pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow -= pzATable->pzAsset[i].pzDAInfo[j-1].fAD;

					// SB 6/6/2013 - see explanation above for VI 52823
					if (!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2))
						pzPTable->pzPKey[k].pzDInfo[iDateIndex].bPeriodEnd = TRUE;

					if (pzPTable->pzPKey[k].bNewKey == TRUE && pzPTable->pzPKey[k].bDeleteKey == FALSE &&
						pzPTable->pzPKey[k].zPK.lInitPerfDate > pzATable->pzAsset[i].pzDAInfo[j].lDate &&
						!IsValueZero(pzPTable->pzPKey[k].pzDInfo[iDateIndex].fNotionalFlow, 2)) 
					{
						pzPTable->pzPKey[k].zPK.lInitPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
						pzPTable->pzPKey[k].zPK.lLndPerfDate = pzATable->pzAsset[i].pzDAInfo[j].lDate;
					}
				}*/
			} // go through all perfkeys
		} // for j < NumDaily
	} // for i < NumAssets

	/*
	** VI 57941 - Now go though the performance asset merge table. if the merge rule for an asset starts between lastperfdate and
	** currentprerfdate then on the merge date, we need to generate notional flow 
	*/
	for (i = 0; i < zPAMTable.iCount; i++)
	{
		if (zPAMTable.pzMergedAsset[i].lBeginDate > lLastPerfDate &&  zPAMTable.pzMergedAsset[i].lBeginDate <= lCurrentPerfDate)
		{
			iFromAssetIndex	= zPAMTable.pzMergedAsset[i].iFromSecNoIndex;
			iToAssetIndex	= zPAMTable.pzMergedAsset[i].iToSecNoIndex;

			// Results for "from security" (security being merged) should be looked at day before the merger while that for "to security"
			// should be looked at on the day merge goes in effect.
			iFromDateIndex = FindDailyInfoOffset(zPAMTable.pzMergedAsset[i].lBeginDate - 1, pzATable->pzAsset[iFromAssetIndex], TRUE);
			iToDateIndex = FindDailyInfoOffset(zPAMTable.pzMergedAsset[i].lBeginDate, pzATable->pzAsset[iToAssetIndex], TRUE);

			/*
			** this shouldn't happen but if merge from asset doesn't have a daily info record on day before merge rule is to go in effect
			** or merge to asset doesn't have a daily info record on the merge day, then nothing to do. Note that since iFromDateIndex is 
			** finding the day prior to merge rule start, we need atleast one more record (on merge date) in the daily info array. So, for 
			** the iFromDateIndex, code below is checking for dailycount - 1 and not daily count. On the other side since for iToDateIndex 
			** is finding the record on the merge date, the code below is checking for iToDateIndex to be less than 1, not zero.
			*/
			if (iFromDateIndex < 0 || iFromDateIndex >= pzATable->pzAsset[iFromAssetIndex].iDailyCount -1 ||
				iToDateIndex < 1 || iToDateIndex >= pzATable->pzAsset[iToAssetIndex].iDailyCount)
				continue;

			// since all the perfkeys keys have same date range (from last perf date to current perf date) for them, getting the date index from 
			// first key should be enough
			iKeyDateIndex = GetDateIndex(pzPTable->pzPKey[0], pzATable->pzAsset[iToAssetIndex].pzDAInfo[iToDateIndex].lDate);

			for (j = 0; j < pzPTable->iCount; j++)
			{
				if (pzPTable->pzPKey[j].bDeleteKey)
					continue;

				if (IsKeyDeleted(pzPTable->pzPKey[j].zPK.lDeleteDate))
					continue;


				k = (j * pzATable->iNumAsset + iToAssetIndex);
				l = (j * pzATable->iNumAsset + iFromAssetIndex);

				// Even though the market value for the security should be divided into short and long and the check
				// also should be done accordingly, for now (because there are no template in use that have rule seperating
				// longs and shorts) combine short and long into one
				bThisSResult = pzPATable->pzStatusFlag[k].piResult[iToDateIndex] & (SRESULT_LONG | SRESULT_SHORT);
				bPreviousSResult = pzPATable->pzStatusFlag[l].piResult[iFromDateIndex] & (SRESULT_LONG | SRESULT_SHORT);

				bThisAResult = pzPATable->pzStatusFlag[k].piResult[iToDateIndex] & (ARESULT_LONG | ARESULT_SHORT);
				bPreviousAResult = pzPATable->pzStatusFlag[l].piResult[iFromDateIndex] & (ARESULT_LONG | ARESULT_SHORT);

				// only need to create notional flow if results for this perfkey have changed (from passed to failed or vice-versa)
				if (bThisSResult == bPreviousSResult && bThisAResult == bPreviousAResult)
					continue;

				// Even though we could have gottent market value before the perfkey loop, do it inside the loop only if at least a
				// key is affected. If more than one key os affected, after the first time GetHoldingsValue function is called, bGotMV 
				// variable for the asset will be true and then we won't have to call the function again for this day
				if (!pzATable->pzAsset[iFromAssetIndex].pzDAInfo[iFromDateIndex].bGotMV)
				{
					zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable, *pzPTable, zRuleTable, pzPmain, zADTable, zPAMTable, 
											pzATable->pzAsset[iFromAssetIndex].pzDAInfo[iFromDateIndex].lDate, lLastPerfDate, lCurrentPerfDate, cCalcAccdiv);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						return zErr;
				}
	
				AddNotionalFlowToTheKey(pzPTable, *pzATable, j, iKeyDateIndex, iFromAssetIndex, iFromDateIndex + 1, bThisSResult, 
										bPreviousSResult, bThisAResult, bPreviousAResult);
			} // go through all perfkeys
		} // if merged asset begin date falls between the date range for which we are calculating performance

		// need to do same (in reverse) for the end date of the rule, if it falls between last and current perf date, create appropriate notional flows
		if (zPAMTable.pzMergedAsset[i].lEndDate >= lLastPerfDate &&  zPAMTable.pzMergedAsset[i].lEndDate < lCurrentPerfDate)
		{
			iFromAssetIndex	= zPAMTable.pzMergedAsset[i].iFromSecNoIndex;
			iToAssetIndex	= zPAMTable.pzMergedAsset[i].iToSecNoIndex;

			// Results for "from security" (security that WAS being merged) should be looked at the day after the merge rule ends while that for 
			// "to security" should be looked at last day on which merge rule was still in effect.
			iFromDateIndex = FindDailyInfoOffset(zPAMTable.pzMergedAsset[i].lEndDate + 1, pzATable->pzAsset[iFromAssetIndex], TRUE);
			iToDateIndex = FindDailyInfoOffset(zPAMTable.pzMergedAsset[i].lEndDate, pzATable->pzAsset[iToAssetIndex], TRUE);

			/*
			** this shouldn't happen but if merge from asset doesn't have a daily info record on day after merge rule ends 
			** or merge to asset doesn't have a daily info record on the last merge day, then nothing to do. Note that since iFromDateIndex is 
			** finding the day after the merge rule ends, we need atleast one record prior to that date in the daily info array. So, for 
			** the iFromDateIndex, code below is checking for less than 1 not zero. On the other side since for iToDateIndex is finding the record 
			// on the merge date and we need notional flow on the following date, the code below is checking for iToDateIndex to be less than 
			// dailycount - 1 and not dailycount.
			*/
			if (iFromDateIndex < 1 || iFromDateIndex >= pzATable->pzAsset[iFromAssetIndex].iDailyCount ||
				iToDateIndex < 0 || iToDateIndex >= pzATable->pzAsset[iToAssetIndex].iDailyCount - 1)
				continue;

			// since all the perfkeys keys have same date range (from last perf date to current perf date) for them, getting the date index from 
			// first key should be enough
			iKeyDateIndex = GetDateIndex(pzPTable->pzPKey[0], pzATable->pzAsset[iFromAssetIndex].pzDAInfo[iFromDateIndex].lDate);

			for (j = 0; j < pzPTable->iCount; j++)
			{
				if (pzPTable->pzPKey[j].bDeleteKey)
					continue;

				if (IsKeyDeleted(pzPTable->pzPKey[j].zPK.lDeleteDate))
					continue;


				k = (j * pzATable->iNumAsset + iFromAssetIndex);
				l = (j * pzATable->iNumAsset + iToAssetIndex);

				// Even though the market value for the security should be divided into short and long and the check
				// also should be done accordingly, for now (because there are no template in use that have rule seperating
				// longs and shorts) combine short and long into one
				bThisSResult = pzPATable->pzStatusFlag[k].piResult[iFromDateIndex] & (SRESULT_LONG | SRESULT_SHORT);
				bPreviousSResult = pzPATable->pzStatusFlag[l].piResult[iToDateIndex] & (SRESULT_LONG | SRESULT_SHORT);

				bThisAResult = pzPATable->pzStatusFlag[k].piResult[iFromDateIndex] & (ARESULT_LONG | ARESULT_SHORT);
				bPreviousAResult = pzPATable->pzStatusFlag[l].piResult[iToDateIndex] & (ARESULT_LONG | ARESULT_SHORT);

				// only need to create notional flow if results for this perfkey have changed (from passed to failed or vice-versa)
				if (bThisSResult == bPreviousSResult && bThisAResult == bPreviousAResult)
					continue;

				// Even though we could have gottent market value before the perfkey loop, do it inside the loop only if at least a
				// key is affected. If more than one key os affected, after the first time GetHoldingsValue function is called, bGotMV 
				// variable for the asset will be true and then we won't have to call the function again for this day
				if (!pzATable->pzAsset[iFromAssetIndex].pzDAInfo[iFromDateIndex -1].bGotMV)
				{
					zErr = GetHoldingValues(pzATable, pzHTable, pzTTable, pzPATable, *pzPTable, zRuleTable, pzPmain, zADTable, zPAMTable, 
											pzATable->pzAsset[iFromAssetIndex].pzDAInfo[iFromDateIndex-1].lDate, lLastPerfDate, lCurrentPerfDate, cCalcAccdiv);
					if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
						return zErr;
				}

				// notional flow should be based on FromAsset MV on the previous day 
				AddNotionalFlowToTheKey(pzPTable, *pzATable, j, iKeyDateIndex, iFromAssetIndex, iToDateIndex, bThisSResult, 
										bPreviousSResult, bThisAResult, bPreviousAResult);
			} // go through all perfkeys
		} // if merged asset end date falls between the date range for which we are calculating performance

	} // for i <PAmtable.count

	return zErr;
} // GenerateNotionalFlow

void AddNotionalFlowToTheKey(PKEYTABLE *pzPKTable, ASSETTABLE2 zATable, int iKeyIndex, int iKeyDateIndex,
							 int iAssetIndex, int iAssetDateIndex, BOOL bThisSResult, BOOL bPreviousSResult,
							 BOOL bThisAResult, BOOL bPreviousAResult)
{
	float fSign;

	//This shouldn't happen but in case key index, keydateindex, asseindex or assetdateindex is not valid, return w/o doinng anything
	if (iKeyIndex < 0 || iKeyIndex >= pzPKTable->iCount || iKeyDateIndex < 0 || iKeyDateIndex >= pzPKTable->pzPKey[iKeyIndex].iDInfoCount)
		return;

	// Note that iassetDateIndex can't be less than 1 (not 0) becuase we need to access previous day's record as well. Also note that this function
	// assumes that the daily info record prior to assetdateindex is for the prior day but doesn't specifically check the dates.
	if (iAssetIndex < 0 || iAssetIndex >= zATable.iNumAsset || iAssetDateIndex < 1 || iAssetDateIndex >= zATable.pzAsset[iAssetIndex].iDailyCount)
		return;
	
	// if key passes both for today and previous days or fails for both days, then nothing to do. Do something only if
	// one day it passes and next day it fails or vice-versa
	if (bThisSResult == bPreviousSResult && bThisAResult == bPreviousAResult)
		return;

	// If Today security passes but previous day it failed for a key, it means this security is being added to this key, so a positive
	// notional flow (equal to asset's previous MV) should be added, on the other hand if today security failed but yesterday it passed,
	// it means security is being removed from the key and a negative notional flow should be added.
	if (bThisSResult && !bPreviousSResult)
		fSign = 1.0;
	else if (!bThisSResult && bPreviousSResult)
		fSign = -1.0;
	else
		fSign = 0.0;

	// When asset's industry level changes, we need to get previous day's market value as today's notional flow, but there are other cases
	// (e.g. when a security's performance stream is being merged into an existing one) when the security was not held previous day, in cases
	// like this take today's market value and add it as today's notional flow
	pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow += zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex-1].fMktVal * fSign;

	if (!IsValueZero(pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow, 2))
		pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].bPeriodEnd = TRUE;

	if (pzPKTable->pzPKey[iKeyIndex].bNewKey == TRUE && pzPKTable->pzPKey[iKeyIndex].bDeleteKey == FALSE &&
		pzPKTable->pzPKey[iKeyIndex].zPK.lInitPerfDate > zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex].lDate &&
		!IsValueZero(pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow, 2))  
	{
		pzPKTable->pzPKey[iKeyIndex].zPK.lInitPerfDate = zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex].lDate;
		pzPKTable->pzPKey[iKeyIndex].zPK.lLndPerfDate = zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex].lDate;
	}


	// do the same for accrual flow
	if (bThisAResult && !bPreviousAResult)
		fSign = 1.0;
	else if (!bThisAResult && bPreviousAResult)
		fSign = -1.0;
	else
		fSign = 0.0;

	pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow += zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex-1].fAI * fSign;
	pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow += zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex-1].fAD * fSign;

	if (!IsValueZero(pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow, 2))
		pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].bPeriodEnd = TRUE;

	if (pzPKTable->pzPKey[iKeyIndex].bNewKey == TRUE && pzPKTable->pzPKey[iKeyIndex].bDeleteKey == FALSE &&
		pzPKTable->pzPKey[iKeyIndex].zPK.lInitPerfDate > zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex].lDate &&
		!IsValueZero(pzPKTable->pzPKey[iKeyIndex].pzDInfo[iKeyDateIndex].fNotionalFlow, 2)) 
	{
		pzPKTable->pzPKey[iKeyIndex].zPK.lInitPerfDate = zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex].lDate;
		pzPKTable->pzPKey[iKeyIndex].zPK.lLndPerfDate = zATable.pzAsset[iAssetIndex].pzDAInfo[iAssetDateIndex].lDate;
	}

	return;
} //AddNotionalFlowToTheKey