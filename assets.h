/*H*
* 
* SUB-SYSTEM: pmr nbcTranProc  
* 
* FILENAME: assets.h
* 
* DESCRIPTION: 
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: 
* 2007-11-08 Added iVendorid for market index vendor - yb.
*
* $Header:   J:/Performer/Data/archives/Master/C/ASSETS.H-arc   1.16   04 Aug 2011 13:12:56   GriffinJ  $
*H*/
#ifndef ASSETS_H
#define ASSETS_H
#include "commonheader.h"


#ifndef NT
  #define NT 1
#endif

typedef struct
{
	char		sSecNo[12+NT];
	char		sWhenIssue[1+NT];
	int			iID;
	char		sCusip[15+NT]; 
	char		sExchange[2+NT];
	char		sExchangeLock[1+NT];
	short		iSecType;
	char		sSecDesc1[30+NT];
	char		sSecDesc2[30+NT];
	char		sSecDesc3[30+NT];
	char		sSecDesc4[30+NT];
	char		sSecDesc5[30+NT];
	char		sSortName[16+NT];
	char		sShortName[20+NT];
	char		sDescLock[1+NT];
	char		sDataSource[1+NT];
	char		sRecordLock[1+NT];
	char		sCurrId[4+NT];
	char		sIncCurrId[4+NT];
	char		sMargin[1+NT];
	char		sSpRating[4+NT];
	char		sMoodyRating[4+NT];
	char		sInternalRating[4+NT];
	char		sAutoDivint[1+NT];
	char		sAutoAction[1+NT];
	char		sAutoMature[1+NT];
	double	fTradUnit;
	double	fUnits;
	double	fAnnDivCpn;
	char		sAnnDivCpnLock[1+NT];
	char		sAnnDivFnote[1+NT];
	long		lCurPrdate;
	double	fCurClose;
	double	fCurBid;
	double	fCurAsk;
	char		sCurSource[1+NT];
	char		sCurPrExch[2+NT];
	double	fCurExrate;
	double	fCurIncExrate;
	long		lPriorPrdate;
	double	fPriorClose;
	double	fPriorBid;
	double	fPriorAsk;
	char		sPriorSource[1+NT];
	char		sPriorPrExch[2+NT];
	double	fPriorExrate;
	double	fPriorIncExrate;
	char		sDtcFlag[1+NT];
	char		sCnsFlag[1+NT];
	char		sMarketMaker[1+NT];
	char		sIssueStatus[1+NT];
	char		sOwnerGrp[6+NT];
	long		lCreateDate;
	char		sCreatedBy[8+NT];
	long		lDatePriced;
	long		lModifyDate;
	char		sModifiedBy[8+NT];
	long		lDeleteDate;
	int			iIndustLevel1;
	int			iIndustLevel2;
	int			iIndustLevel3;
	char		sSic[4+NT];
	char		sCountryIss[2+NT];
	char		sCountryIsr[2+NT];
	char		sEuroClear[1+NT];
	int			iVolume;
	double	fInvPct;
	char		sBenchmarkSecNo[12+NT];
	int		iVendorID;
} ASSETS;


typedef struct 
{
  char		sSecNo[12+NT];		
  char		sWhenIssue[1+NT];
  int			iSecType;
  char		sCurrId[4+NT];
  char		sIncCurrId[4+NT];
  int			iIndustLevel1;
  int			iIndustLevel2;
  int			iIndustLevel3;
  char		sSpRating[4+NT];
  char		sMoodyRating[4+NT];
  char		sInternalRating[4+NT];
  double	fAnnDivCpn;
  double	fCurExrate;
  char		sTaxableCountry[1+NT]; // Whether taxable at country level
	char		sTaxCountry[3+NT]; // country which issued this bond, if taxfree it will be taxfree for this country
	char		sTaxableState[1+NT]; // Whether taxable at state level
	char		sTaxState[3+NT]; // state which issued this bond, if taxfree it will be taxfree for this state*/
  int			iSTypeIndex;
  short		iLongShort;
	char		sSecDesc1[30+NT]; // 1st line of asset description, need it for single security segmets
	char		sCountryIss[3+NT]; // Country ID where security is issued
	char		sCountryIsr[3+NT]; // Country issuer
	char		sDRDElig[1+NT]; // From equities taxable DRD flag
	int			iVendorID;
} PARTASSET;

typedef struct 
{
	int       iNumAsset;
  int       iAssetCreated;
  PARTASSET *pzAsset;
} ASSETTABLE;


// for now the only thing that can change daily and we care about is industry level, eventually other fields
// that can change daily like rating, p/e ratio, maturity, etc, wil be added as well
typedef struct
{
	long		lDate;
  int			iIndustLevel1;
  int			iIndustLevel2;
  int			iIndustLevel3;
	BOOL		bGotMV;
	BOOL		bGotAccrual;
	double	fMktVal;
	double	fAI;
	double	fAD;
	double	fMVExRate;
	double	fAccrExRate;
} DAILYASSETINFO;

typedef struct 
{
  char						sSecNo[12+NT];		
  char						sWhenIssue[1+NT];
	char						sSecDesc1[30+NT]; // 1st line of asset description, need it for single security segmets
  int							iSecType;
  char						sCurrId[4+NT];
  char						sIncCurrId[4+NT];
  char						sSpRating[4+NT];
  char						sMoodyRating[4+NT];
	char						sInternalRating[4+NT];
  double					fAnnDivCpn;
  double					fCurExrate;
	char						sCountryIss[3+NT]; // Country ID where security is issued
	char						sCountryIsr[3+NT]; // Country issuer
  char						sTaxableCountry[1+NT]; // Whether taxable at country level
	char						sTaxCountry[3+NT]; // country which issued this bond, if taxfree it will be taxfree for this country
	char						sTaxableState[1+NT]; // Whether taxable at state level
	char						sTaxState[3+NT]; // state which issued this bond, if taxfree it will be taxfree for this state*/
	char						sDRDElig[1+NT]; // From equities taxable DRD flag
  int							iSTypeIndex;
  short						iLongShort;
	int							iDailyCount;
	DAILYASSETINFO	*pzDAInfo;
} PARTASSET2;

typedef struct 
{
	int					iNumAsset;
  int					iAssetCreated;
//	int					iNumDays;
  PARTASSET2	*pzAsset;
} ASSETTABLE2;

typedef struct
{
  int		iIndustLevel1;
	long	lEffDate1;
  int		iIndustLevel2;
	long	lEffDate2;
  int		iIndustLevel3;
	long	lEffDate3;
} LEVELINFO;

typedef void	(CALLBACK* LPPRASSETS)(ASSETS *, char *, char *, int, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRPARTASSET2)(PARTASSET2 *);
typedef void	(CALLBACK* LPPRASSETTABLE)(ASSETTABLE2 *);
typedef int		(CALLBACK* LPFNFINDASSET)(ASSETTABLE2, char *, char *, BOOL, short);
typedef void	(CALLBACK* LPPRPARTASSET)(PARTASSET2 *, LEVELINFO *, char *, char *, int, ERRSTRUCT *);

#endif // !ASSETS_H
