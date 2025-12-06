/*H*
* 
* SUB-SYSTEM: pmr tranproc  
* 
* FILENAME: portmain.h
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
*
* $Header:   P:\Effron\C\portdir.h__   1.4   08 Jun 1998 16:40:10   shobhit  $
/*H*
* 
* SUB-SYSTEM: pmr tranproc  
* 
* FILENAME: portmain.h
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
*
* $Header:   P:\Effron\C\portdir.h__   1.4   08 Jun 1998 16:40:10   shobhit  $
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2009-03-04 VI# 41539: added accretion functionality - mk
*  2007-08-01 Added iVendorID and bIsMarketIndex 
*H*/


#ifndef PORTMAIN_H
#define PORTMAIN_H

#ifndef NT
  #define NT 1
#endif


typedef struct
{
  int			iID;
	char		sUniqueName[20+NT];
	char		sAbbrev[20+NT];
	char		sDescription[60+NT];
	long		lDateHired;
	double	fIndividualMinAnnualFee;
	double	fIndividualMinAcctSize;
	double	fTotalAssetsManaged;
	int			iInvestmentStyle;
	int			iScope;
	int			iDecisionMaking;
	int			iDefaultReturnType;
	int			iProductType;
	double	fExpenseRatio;
	int			iMarketCap;
	int			iMaturity;
	long		lAsofDate;
	short		iFiscalYearEndMonth;
	short		iFiscalYearEndDay;
	char		sPeriodType[20+NT];
	long		lInceptionDate;
	BOOL		bUserInceptionDate;
  char		sPortfolioType[1+NT];
	char		sAdministrator[3+NT];
	char		sManager[3+NT];
	char		sAddress1[40+NT];
	char		sAddress2[40+NT];
	char		sAddress3[40+NT];
  char		sAcctMethod[1+NT];
  char		sTax[1+NT];
  char		sBaseCurrId[4+NT];
  BOOL		bIncome;
  BOOL		bActions;
  BOOL		bMature;
  BOOL		bCAvail;
  BOOL		bFAvail;
  char		sAlloc[8+NT];
  double	fMaxEqPct;
  double	fMaxFiPct;
  double	fMinCashPct;
  int			iEqLotSize;
	int			iFiLotSize;
  long		lValDate;
  long		lDeleteDate;
	BOOL		bIsInactive;
  char		sCurrHandler[1+NT];
  BOOL		bAmortMuni;
  BOOL		bAmortOther;
  BOOL		bAccreteDisc;
  long		lAmortStartDate;
  BOOL		bAccretMuni;
  BOOL		bAccretOther;
	BOOL		bIncByLot;
	BOOL		bDiscretionaryAuthority;
	BOOL		bVotingAuthority;
	BOOL		bSpecialArrangements;
	int			iIncomeMoneyMarketFund;
	int			iPrincipalMoneyMarketFund;
	char		sIncomeProcessing[1+NT];
	long		lPricingEffectiveDate;
  long		lLastTransNo;
  long		lPurgeDate;
  long		lLastActivity;
  long		lRollDate;
  int			iVendorID;
  BOOL		bIsMarketIndex;
} PORTMAIN;

/*typedef struct {
    int  iID;
    char sPortComp[2];
    char sBaseCurrId[5];
    long lValDate;
    long lDeleteDate;
    long lInceptDate;
    long lFiscal;
  } PARTPORTDIR;*/
 
typedef struct 
{
	int			iID;
	char		sUniqueName[20+NT];
	short		iFiscalMonth;
	short		iFiscalDay;
	long		lInceptionDate;
	char		sAcctMethod[1+NT];
	char		sBaseCurrId[4+NT];
	BOOL		bIncome;
	BOOL		bActions;
	BOOL		bMature;
	BOOL		bCAvail;
	BOOL		bFAvail;
	long		lValDate;
	long		lDeleteDate;
	BOOL		bIsInactive;
	BOOL		bAmortMuni;
	BOOL		bAmortOther;
	BOOL		bAccretMuni;
	BOOL		bAccretOther;
	long		lAmortStart;
	BOOL		bIncByLot;
	long		lPurgeDate;
	long		lRollDate;
	char		sTax[1+NT];
	int			iPortfolioType;
	long		lPricingEffectiveDate;
  double	fMaxEqPct;
	double	fMaxFiPct;
	int			iCurrIndex;// points to the entry for basecurrid in currency table
	int			iVendorID;
	int			iReturnsToCalculate;
	int			iRorType;
  BOOL		bIsMarketIndex;
} PARTPMAIN;


typedef struct 
{
	int       iPmainCreated;
	int       iNumPmain;
	PARTPMAIN *pzPmain;
} PORTTABLE;

// portmain related functions
typedef void	(CALLBACK* LPPRPORTMAIN)(PORTMAIN *, int, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRPMAINPOINTER)(PORTMAIN *);
typedef void	(CALLBACK* LPPRPARTPMAINALL)(PARTPMAIN *, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRPARTPMAINONE)(PARTPMAIN *, int, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSELECTONEPARTPORTMAIN)(PARTPMAIN *, long, ERRSTRUCT *);

#endif // PORTMAIN_H