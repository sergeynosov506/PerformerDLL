/*H*
* 
* FILENAME: Priclist.h
* 
* DESCRIPTION: Defines structure of Priclist table on SQL Server
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: vay
*
*H*/

#ifndef NT
  #define NT 1
#endif

  
typedef struct
{
	char			sCusip[12+NT];
	char			sPriceExchange[3+NT];
	long			lPriceDate;
	char			sPriceSource[1+NT];
	long			lDatePriceUpdated;
	long			lDateExrateUpdated;
	double			fClosePrice;
	double			fBidPrice;
	double			fAskPrice;
	double			fHighPrice;
	double			fLowPrice;
	double			fExrate;
	double			fIncExrate;
	double			fAnnDivCpn;
	char			sAnnDivFnote[1+NT];
	int				iVolume;
	char			sSpRating[4+NT];
	char			sMoodyRating[4+NT];
	char			sInternalRating[4+NT];
	double			fAccrInt;
	double			fCurYld;
	double			fCurYtm;
	double			fBondEqYld;
	double			fYldToWorst;
	char			sYtwType[1+NT];
	double			fYldToBest;
	char			sYtbType[1+NT];
	double			fYldToEarliest;
	char			sYteType[1+NT];
	double			fCurDur;
	double			fCurModDur;
	double			fMaturity;
	double			fConvexity;
	double			fVariableRate;
	double			fEps;
	double			fPeRatio;
	double			fBeta;
	double			fDebtPerShare;
	double			fEquityPerShare;
	double			fEarningFive;
	double			fNavFactor;
	double			fSharOutstand;
	double			fCumulativeSplitFactor;
	double			fBookValue;
	int				iPriority;
	char			sProcessed[1+NT];
	long			lCreateDate;
	double			fNextYearProjectedPE;
	double			fSales;

} PRICLIST;


