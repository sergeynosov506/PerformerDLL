/**
*
* SUB-SYSTEM: calcperf
*
* FILENAME: perfkey.h
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR:	Shobhit Barman
*
*
**/
#ifndef PERFKEY_H
#define PERFKEY_H

#include "commonheader.h"

#ifndef NT
	#define NT 1
#endif
 
typedef struct
{
	long lPerfkeyNo;
	int  iPortfolioID;
  int  iID;
  long lRuleNo;
  long lScrhdrNo;
  char sCurrProc[1+NT];
  char sTotalRecInd[1+NT];
  char sParentChildInd[1+NT];
  long lParentPerfkeyNo;
  long lInitPerfDate;                         
  long lLndPerfDate;                        
  long lMePerfDate;                              
  long lLastPerfDate;
  char sDescription[30+NT];
  long lDeleteDate;
	char sPermDelFlg[1+NT];
} PERFKEY;
 
	
typedef void	(CALLBACK* LPPRPERFKEY)(PERFKEY, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRPPERFKEY1INT)(PERFKEY *, int, ERRSTRUCT *); 

#endif // PERFKEY_H