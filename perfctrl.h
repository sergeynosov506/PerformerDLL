/*H*
*
*
* FILENAME: perfctrl.h
*
* DESCRIPTION:  This is for table "perform:perfctrl1"
*
* PUBLIC FUNCTION(S):
*
* USAGE:
*
* AUTHOR: Shobhit Barman
*
*
*H*/
#ifndef PERFCTRL_H
#define PERFCTRL_H

#include "commonheader.h"


#ifndef NT
	#define NT 1
#endif

typedef struct
{
  int  iPortfolioID;
  char sAggBaseFlag[1+NT];
  char sCurrBaseFlag[1+NT];
  char sCurrLocalFlag[1+NT];
  char sTaxCalc[1+NT];
  char sPerfInterval[1+NT];
  char sCalcFlow[1+NT];
  long lLastPerfDate;
  char sReprocessed[1+NT];
	char sDRDElig[1+NT];
	long lFlowStartDate;
} PERFCTRL;

 
typedef void  (CALLBACK* LPPRPPERFCTRL1INT)(PERFCTRL *, int, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRPERFCTRL)(PERFCTRL, ERRSTRUCT *);

#endif // PERFCTRL_H