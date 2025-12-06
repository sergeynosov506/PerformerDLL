/*H*
* 
* FILENAME: holdtot.h
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
*H*/
// History
// 2010-09-16 VI#44799 Added capability to calculate average final bond maturity - mk.
// 2007-11-08 Added Vendor id    - yb
// 2/17/2004 added new fields to structure - vay
#ifndef HOLDTOT_H
#define HOLDTOT_H

#include "commonheader.h"


typedef struct 
{
	int     iId;
	int     iSegmentTypeID;
	int     iScrhdrNo;
	double  fNativeOrigCost; 
	double  fNativeTotCost;
	double  fNativeMarketValue;
	double  fNativeAccrual;
	double  fNativeGainorloss;
	double  fNativeAccrGorl;
	double  fNativeCurrGorl ;
	double  fNativeSecGorl;
	double  fNativeCostYield;
	double  fNativeCurrYield;
	double  fNativeAvgPriceChange;
	double  fNativeWtdavgChange;
	double  fNativeIncome;
	double  fNativeParvalue;
	double  fBaseOrigCost;
	double  fBaseTotCost;
	double  fBaseMarketValue;
	double  fBaseAccrual;
	double  fBaseGainorloss;
	double  fBaseAccrGorl;
	double  fBaseCurrGorl;
	double  fBaseSecGorl;
	double  fBaseCostYield;
	double  fBaseCurrYield;
	double  fBaseAvgPriceChange;
	double  fBaseWtdavgChange;
	double  fBaseIncome;
	double  fBaseParvalue;
	double  fSystemOrigCost;
	double  fSystemTotCost;
	double  fSystemMarketValue;
	double  fSystemAccrual;
	double  fSystemGainorloss;
	double  fSystemAccrGorl;
	double  fSystemCurrGorl;
	double  fSystemSecGorl;
	double  fSystemCostYield;
	double  fSystemCurrYield;
	double  fSystemAvgPriceChange;
	double  fSystemWtdavgChange;
	double  fSystemIncome;
	double  fSystemParvalue;
	double  fPercentTotmv;
	double  fCouponRate;
	double  fCurrentYieldtomaturity;
	double  fMaturity;
	double  fDuration;
	double  fRating;
	int     iNumberofsecurities;
	double  fPeriodreturn;
	double  fPctEPS1Yr;
	double  fPctEPS5Yr;
	double  fBeta;
	double  fPriceBook;
	double  fPriceSales;
	double  fTrail12mPE;
	double  fProj12mPE;
	double  fAvgWtdCap;
	double  fMedianCap;
	double  fPcTopTenHold;
	double  fROE;
	double  fDivYield;
	double  fMaturityReal;
} HOLDTOT;

typedef struct
{
	char	sRatingChar[4+NT];
	char	sRatingType[1+NT];
	int		iRatingVal;
} RATING;

typedef struct
{
	//  {-------- From holdings tables --------}
	int			iID;
	char		sSecNo[12+NT];
	char		sWi[1+NT];
	char		sSecXtend[2+NT];
	char		sAcctType[1+NT];
	long		lTransNo;
	double	fTotCost;
	double	fOrigCost;
	double	fMktVal;
	double	fAccrInt;
	double	fAccrualGl;
	double	fCurrencyGl;
  double	fSecurityGl;
	double	fMktEffMatYld;
	double	fCostEffMatYld;
	double	fMktCurYld;
	double	fAnnualIncome;
	double	fUnits;
	double	fUnitCost;
	double	fMvBaseXrate;
	double	fMvSysXrate;
	double	fBaseCostXrate;
	double	fSysCostXrate;
	double	fAiBaseXrate;
	double	fAiSysXrate;

	//  {-------- From assets tables --------}
	int			iSecType;
	char		sCurrId[5];
	char		sIncCurrId[5];
	int			iIndustLevel1;
	int			iIndustLevel2;
	int			iIndustLevel3;
	double	fTradUnit;
	char		sSpRating[5];
	char		sMoodyRating[5];
	char		sNbRating[5];
	char		sTaxStat[2];
	double	fAnnDivCpn;
	double	fCurExrate;
	int			iSTypeIndex;
	short		iLongShort;
	char		sSecDesc1[30+NT];
	char		sCountryIss[3+NT];
	char		sCountryIsr[3+NT];

	//  {-------- From other tables --------}
	char		sPrimaryType[1+NT];
	char		sSecondaryType[1+NT];
	double	fDuration;
	double	fMaturity;
	char		sRating[4 + NT];    	 
	double	fCouponRate;
	//Divint Table
	char		sCallFlag[1+NT];
	long		lPayDate;
	char		sTableName[1+NT];
	double  fClosePrice;
	double  fYieldToMaturity;

	// from histeqty table 
	// for further weighted averaging in dll code 
	double  fPctEPS1Yr;
	double	fPctEPS5Yr;
	double	fBeta;
	double	fEquityPerShare;
	double  fBookValue;
	double  fSales;
	double  fSharesOutstand;
	double  fTrail12mPE;
	double  fProj12mPE;
	double	fBaseMktValEqty;
	int	    iVendorID;
	double	fMaturityReal;

} HOLDINGSASSETS;


typedef void	(CALLBACK* LPPRHOLDTOT)(HOLDTOT, ERRSTRUCT *);
#endif // EQUITIES_H