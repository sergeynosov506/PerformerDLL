/*H*
* 
* FILENAME: fwtrans.h
* 
* DESCRIPTION: 
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: Shaojie Zhou
*
*H*/

#ifndef NT
  #define NT 1
#endif

typedef   struct
{
  int		 iID;
	long	 lTransNo;
  long   lDivintNo;
	char   sTranType[2+NT];
  char   sSecNo[12+NT];
  char   sWi[1+NT];
	int		 iSecID;
  char   sSecXtend[2+NT];
  char   sAcctType[1+NT];    
  char	 sDivType[2+NT];
  double fDivFactor;
  double fUnits;
  double fPcplAmt;
  double fIncomeAmt;
  long   lTrdDate;
  long   lStlDate;
  long   lEffDate;
  char   sCurrId[4+NT];
  char   sIncCurrId[4+NT];
  char   sIncAcctType[1+NT];
  char   sSecCurrId[4+NT];
  char   sAccrCurrId[4+NT];
  double fBaseXrate;
  double fIncBaseXrate;
  double fSecBaseXrate;
  double fAccrBaseXrate;
  double fSysXrate;
  double fIncSysXrate;
  char   sDrCr[2+NT];
  long   lCreateDate;
  char   sCreateTime[8+NT];
  char   sDescription[60+NT];
} FWTRANS;





typedef void	(CALLBACK* LPPRFWTRANS)(FWTRANS *, ERRSTRUCT *);
typedef void	(CALLBACK* LPPR1INT)(int, ERRSTRUCT *); 
//typedef void	(CALLBACK* LPPRSELECTONEACCDIV)(ACCDIV *, int, long, long, ERRSTRUCT *);
//typedef void	(CALLBACK* LPPRSELECTALLACCDIV)(ACCDIV *, int *, double *, double *, double *, char *, char *, 
//																							int, char *, char *, char *, char *, long, long, ERRSTRUCT *);
//typedef void	(CALLBACK* LPPRALLACCDIVFORANACCOUNT)(int, ACCDIV *, ERRSTRUCT *);
