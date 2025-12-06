/*H*
* 
* FILENAME: CURRENCY.h
* 
* DESCRIPTION: 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: Shobhit Barman
*
*H*/
#ifndef CURRENCY_H
#define CURRENCY_H

#include "commonheader.h"

#ifndef NT
  #define NT 1
#endif

#define NUMCURRENCY				200	// maximum number of currency records
#define NUMCOUNTRY				200	// maximum number of country records

typedef struct
{
  char		sCurrId[4+NT];
	char		sCurrDesc[30+NT];
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  int			sSecid;
  char		sCountry[3+NT];
  char		sDisplayasdivide[1+NT];
  char		sCombinationflag[1+NT];
  char		sYieldflag[1+NT];
  char		sIsyieldbeforetax[1+NT];
  int			iDisplaydecimalposition;
  int			iActualdecimalposition;
  double	fDividendwithholdrate;
  double	fDividendreclaimrate;
  double	fInterestwithholdrate;
  double	fInterestreclaimrate;
} ACURRENCY;


typedef struct 
{
	char   sCurrId[4+NT];
	char   sSecNo[12+NT];
	char   sWi[1+NT];
	double fCurrExrate;
} PARTCURR;

typedef struct 
{
	int      iNumCurrency;
	PARTCURR zCurrency[NUMCURRENCY];
} CURRTABLE;

typedef struct 
{
	char sCurrId[4+NT];
  char sSecNo[12+NT];
  char sWi[1+NT];
} PARTCURRENCY;

typedef struct 
{
  int          iNumCurrency;
  PARTCURRENCY zCurrency[NUMCURRENCY];
} CURRENCYTABLE;
 

typedef struct 
{
	int		iId;
	char  sCode[3+NT];
	char  sDescription[28+NT];
} COUNTRY;

typedef struct 
{
	int				iNumCountry;
	COUNTRY		zCountry[NUMCOUNTRY];
} COUNTRYTABLE;


typedef void (CALLBACK* LPPRSELECTALLPARTCURRENCIES)(PARTCURRENCY *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRPPARTCURR)(PARTCURR *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTALLCOUNTRIES)(COUNTRY *, ERRSTRUCT *);

#endif // CURRENCY_H
