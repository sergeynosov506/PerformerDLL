/*H*
* 
* FILENAME: trantype.h
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
*
*H*/

#ifndef TRANTYPE_H
#define TRANTYPE_H
#include "commonheader.h"


#ifndef NT
  #define NT 1
#endif

#define NUMTRANTYPE				 125 // maximum number of trantype records

typedef struct
{
	char	sTranType[2+NT];
  char	sAbbrvDesc[8+NT];
  char	sFullDesc[30+NT];
  char	sTranCode[1+NT];
  char	sRptCode[1+NT];
  long	lCashImpact;
  long	lSecImpact;
  char	sPerfImpact[1+NT];
  long	lPortbalImpact;
  char	sDrCr[2+NT];
  short iTradeDateInd;
  short iSettleDateInd;
  char	sTradeFees[1+NT];
  char	sTradeReverse[1+NT];
  char	sJournal[1+NT];
  char	sTradeScreen[2+NT];
  short iTradeSort;
  char	sAutoGen[1+NT];
} TRANTYPE;


typedef struct 
{
	char sTranType[2+NT];
  char sDrCr[2+NT];
  char sTranCode[1+NT];
  long lSecImpact;
  char sPerfImpact[1+NT];
} PARTTRANTYPE;

typedef struct 
{
	int          iNumTType;
  PARTTRANTYPE zTType[NUMTRANTYPE];
} TRANTYPETABLE;
 

typedef struct 
{
	int         iNumTType;
	TRANTYPE	zTType[NUMTRANTYPE];
} TRANTYPETABLE1;  

typedef void	(CALLBACK* LPPRTRANTYPE)(TRANTYPE, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRSELECTTRANTYPE)(TRANTYPE *, char *, char *, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSELECTALLPARTTRANTYPE)(PARTTRANTYPE *, ERRSTRUCT *);


#endif // !TRANTYPE_H
