/**
* 
* SUB-SYSTEM: Transaction Processor/Payments  
* 
* FILENAME: TipsProcessing.c
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
**/

#include "transengine.h"
#include "math.h"

/*F*
** Function to calculate inflation rate for a TIPS security
** 
*F*/
DLLAPI ERRSTRUCT STDCALL WINAPI CalculateInflationRate(int iID, char *sSecNo, char *sWi, long lTaxlotNo, 
												long lPreviousIncomeDate, long lCurrentStlDate, long lOriginalTrdDate,
												BOOL bSaveErrorToFile, BOOL bFirstIncome, double *pfCalculatedRate)
{
	PARTFINC	zPFinc;
  double		fPreviousInfRatio, fCurrentInfRatio, fOriginalInfRatio;
	char			sTempStr[120];
	ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

	lpprSelectPartFixedinc(sSecNo, sWi, &zPFinc, &zErr);
 	if (zErr.iSqlError) 
    return(PrintError("Error Selecting Issue Date From FIXEDINC", iID, lTaxlotNo, "L", 0, 
											zErr.iSqlError, zErr.iIsamCode, "TIPS CALCINFRATE1", FALSE));

  // Get the inflation ratio for the previous phantom income/purchase date, return with an error if don't have a valid ratio 
  fPreviousInfRatio = lpfnInflationIndexRatio(sSecNo, zPFinc.lIssueDate, 
																							lPreviousIncomeDate, zPFinc.lInflationIndexID);
  if (fPreviousInfRatio <= 0) 
  {
    sprintf_s(sTempStr, "Invalid Inflation Ratio %f, for Security %s, Date: %d", fPreviousInfRatio, sSecNo, lPreviousIncomeDate);
		zErr.iBusinessError = 999;
		if (bSaveErrorToFile)
			PrintError(sTempStr, iID, lTaxlotNo, "L",  zErr.iBusinessError, 0, 0, "TIPS CALCINFRATE2", FALSE);
    return zErr;
  }

  // Get the inflation ratio for current settlement date (sale/income date), return with an error if don't have a valid ratio
  fCurrentInfRatio = lpfnInflationIndexRatio(sSecNo, zPFinc.lIssueDate, 
		                                         lCurrentStlDate, zPFinc.lInflationIndexID);
  if (fCurrentInfRatio <= 0) 
  {
    sprintf_s(sTempStr, "Invalid Inflation Ratio %f, for Security %s, Date: %d", fCurrentInfRatio, sSecNo, lCurrentStlDate);
		zErr.iBusinessError = 999;
		if (bSaveErrorToFile)
			PrintError(sTempStr, iID, lTaxlotNo, "L",  zErr.iBusinessError, 0, 0, "TIPS CALCPINC3", FALSE);
    return zErr;
  }

  // Get the inflation ratio for the original trade (PS/FR), return with an error if don't have a valid ratio
	if (bFirstIncome)
		fOriginalInfRatio = 1.0;
	else
	{
		fOriginalInfRatio = lpfnInflationIndexRatio(sSecNo, zPFinc.lIssueDate, 
			                                         lOriginalTrdDate, zPFinc.lInflationIndexID);
		if (fOriginalInfRatio <= 0) 
		{
			sprintf_s(sTempStr, "Invalid Inflation Ratio %f, for Security %s, Date: %d", fOriginalInfRatio, sSecNo, lOriginalTrdDate);
			zErr.iBusinessError = 999;
			if (bSaveErrorToFile)
				PrintError(sTempStr, iID, lTaxlotNo, "L",  zErr.iBusinessError, 0, 0, "TIPS CALCPINC4", FALSE);
			return zErr;
		}
	}

  // Calculate the inflation rate since last payment 
  *pfCalculatedRate = (fCurrentInfRatio - fPreviousInfRatio)  / fOriginalInfRatio;

  return zErr;
}

/*F*
** Function to calculate phantom income for a TIPS security
** 
*F*/
DLLAPI ERRSTRUCT STDCALL WINAPI CalculatePhantomIncome(int iID, char *sSecNo, char *sWi, long lTaxlotNo, long lPreviousIncomeDate, 
																									long lCurrentStlDate, long lOriginalTrdDate, double fOrigTotCost, 
																									double fUnits, BOOL bFirstIncome, double *pfCalculatedPI)
{ 
  double		fIncAmount, fCurrentIncome, fIncUnits, fInflationRate;
  char			sTempStr[120], sTranType[3], sDrCr[3];
  long			lBeginDate, lCashImpact, lSecImpact;
  short			iMDY[3];
	ERRSTRUCT zErr;

  InitializeErrStruct(&zErr);

  // Get the inflation rate for the security since last payment
	zErr = CalculateInflationRate(iID, sSecNo, sWi, lTaxlotNo, 
												lPreviousIncomeDate, lCurrentStlDate, lOriginalTrdDate,
												TRUE, bFirstIncome, &fInflationRate);
	
 	if (zErr.iSqlError) 
  {
    sprintf_s(sTempStr, "Error Calculating Inflation Rate %f, for Security %s", fInflationRate, sSecNo);
    return(PrintError(sTempStr, iID, lTaxlotNo, "L",  999, 0, 0, "TIPS CALCPINC1", FALSE));
  }

  // Calculate the amount of phantom income (equal to adjustment in principal value from last time)
	*pfCalculatedPI = fInflationRate * fOrigTotCost;

  /*
  ** If amount > 0 then it is an inflationary period, unless there is a previous carry over
  ** djustment left, this is the amount of phantom income (as well as BA). If on the other hand
  ** the amount is < 0, it is a period of deflation, there should be no (positive) phantom
  ** income, in addtion, there should be an ardinary deduction to the extent of the income
  ** received this year from the security
  */
  if (*pfCalculatedPI >= 0)
  {
    // for the time being, if there is any carry over income ignore it, just the calculated amount
    // will be phantom income (no adjustment for carry over income) - will have to figure out how to do this
  }
  else
  {
		// Get the 1st of the year
    if (lpfnrjulmdy(lCurrentStlDate, iMDY) < 0)
      return(PrintError("Invalid Date", iID, lTaxlotNo, "L",  999, 0, 0, "TIPS CALCPINC2", FALSE));

    iMDY[0] = iMDY[1] = 1; // set month and day to be 1
    if (lpfnrmdyjul(iMDY, &lBeginDate) < 0) // begining of the year
      return(PrintError("Error Converting Date", iID, lTaxlotNo, "L",  999, 0, 0, "TIPS CALCPINC3", FALSE));

		// Get Income for the year for this taxlot
    fCurrentIncome = 0;
    while (zErr.iSqlError == 0 && zErr.iBusinessError == 0)
    {
			lpprGetIncomeForThePeriod(iID, lTaxlotNo, lBeginDate, lCurrentStlDate, sTranType, 
																sDrCr, &lCashImpact, &lSecImpact, &fIncAmount, &fIncUnits, &zErr);
      if (zErr.iSqlError == SQLNOTFOUND)
      {
        zErr.iSqlError = 0;
        break;
      }
      else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        return zErr;

			// If Units for the transaction was zero(maybe PI entered through
			// FrontEnd), assume Income Units equal to Current Units
			if (IsValueZero(fIncUnits, 2))
				fIncUnits = fUnits;

			// Take proportional income (possible that at the time of income, there were more units than now
      if (lCashImpact == 1 || lCashImpact == -1) // If an Open, Income or a close transaction
        fCurrentIncome += fIncAmount * lCashImpact * fUnits / fIncUnits;
      else if (lSecImpact == 1 || lSecImpact == -1) // PI transaction (what about FR/FD?)
        fCurrentIncome += fIncAmount * lSecImpact * fUnits / fIncUnits;
    } // more records to select

    // if loss in principal is more than the actual income received during the year
    if (fabs(*pfCalculatedPI) > fCurrentIncome)
		{
			if (fCurrentIncome > 0) // can take deduction only to the extent of income received during the year
        *pfCalculatedPI = -fCurrentIncome;
			else
				*pfCalculatedPI = 0;
		}
    else // else record decrease of principal value
      *pfCalculatedPI = *pfCalculatedPI;
  } // negative income

	// round it to 2 decimal places
	*pfCalculatedPI = RoundDouble(*pfCalculatedPI, 2);

  return zErr;
} // CalculatePhantomIncome
