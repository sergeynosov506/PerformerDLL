/*H*
* 
* FILENAME: payrec.h
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
*H*/
#ifndef PAYREC_H
#define PAYREC_H


#ifndef NT
  #define NT 1
#endif

typedef struct
{
  int			iID;
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  char		sSecXtend[2+NT];
  char		sAcctType[1+NT];
  long		lTransNo;
	int			iSecID;
  long		lAsofDate;
//  long		lPayrecTransNo;
  char		sTranType[2+NT];
  long		lDivintNo;
  double	fUnits;
  double	fBaseCostXrate;
  double	fSysCostXrate;
  double	fCurVal;
  long		lEffDate;
  double	fMvBaseXrate;
  double	fMvSysXrate;
  char		sValuationSrce[2+NT];
} PAYREC;


typedef void	(CALLBACK* LPPRPAYREC)(PAYREC, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRSELECTPAYREC)(PAYREC *, int, char *, char *, char *, char *, long, long, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRALLPAYRECFORANACCOUNT)(int, PAYREC *, ERRSTRUCT *);

#endif // PAYREC_H