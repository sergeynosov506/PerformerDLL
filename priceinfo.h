/*
*   
* FILENAME: priceinfo.h
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
*/
#ifndef PRICEINFO_H
#define PRICEINFO_H
#include "commonheader.h"

#ifndef NT
	#define NT 1
#endif

typedef struct
{
	// folowing fields come from histpric
	char	sSecNo[12+NT];
	char	sWi[1+NT];
	long	lPriceDate;
	long	lDatePriceUpdated;
	long	lDateExrateUpdated;
	char	sPriceSource[1+NT];
	double	fClosePrice;
	double	fExrate;
	double	fIncExrate;
	double	fAnnDivCpn;
	// following fields come from histfinc
	double	fAccrInt;
	double	fCurYld;
	double	fCurYtm;
	double	fVariablerate; //TIPS ratio
	// following field comes from histeqty
	double	fEps;
	// following fields come from assets
	char	sCurrId[4+NT];
	char	sIncCurrId[4+NT];
	double	fTradUnit;
	short	iSecType;
	char	sBenchmarkSecNo[12+NT];
	// following fields do not come from any table
	BOOL	bIsPriceValid; 
	BOOL	bIsExrateValid;
	BOOL	bRecordFound;
	int		iSTypeIdx; // location of this asset's sectype in the sectype table
} PRICEINFO;

typedef struct 
{
	// Even though write now there is no check, all the records in the table
	// should be for price date
	long		lPriceDate; 
	int			iPriceCreated;
	int			iNumPrice;
	PRICEINFO	*pzPrice;
} PRICETABLE;
 
typedef struct
{
	// folowing fields come from customhistprice
	char	sSecNo[12+NT];
	char	sWi[1+NT];
	long	lPriceDate;
	int		iCustomItemID;
	long	lDatePriceUpdated;
	double	fClosePrice;
} CUSTOMPRICEINFO;

typedef struct 
{
	// Even though write now there is no check, all the records in the table
	// should be for price date
	long			lPriceDate; 
	int				iCPriceCreated;
	int				iNumCPrice;
	CUSTOMPRICEINFO	*pzCPrice;
} CUSTOMPRICETABLE;

typedef void	(CALLBACK* LPPRSECPRICE)(char *, char *, long, PRICEINFO *, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRCUSTOMPRICE)(int, long, CUSTOMPRICEINFO *, ERRSTRUCT *);

		
#endif // PRICEINFO_H
