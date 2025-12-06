/*H*
* 
* FILENAME: Equities.h
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
#ifndef EQUITIES_H
#define EQUITIES_H

#include "commonheader.h"

#ifndef NT
  #define NT 1
#endif


typedef struct
{
	char		sSecNo[12+NT];
	char		sWi[1+NT];
	double	fEps;
	BOOL		bRecordFound;
} PARTEQTY;

typedef struct
{
	int				iEqtyCreated;
	int				iNumEqty;
	PARTEQTY	*pzEqty;
} EQTYTABLE;
	
/*typedef struct 
{
  int   iAssetIndex;
  double fEps;
  double fPeRatio;
  double fBeta;
  double fEarningsFive;
} PARTEQUITY;

typedef struct 
{
	int        iNumEquity;
  int        iEquityCreated;
  PARTEQUITY *pzEquity;
} EQUITYTABLE;*/
 

typedef void	(CALLBACK* LPPRSELECTPARTEQTY)(char *, char *, PARTEQTY *, ERRSTRUCT *);
#endif // !EQUITIES_H
