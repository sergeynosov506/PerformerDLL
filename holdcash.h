/*H*
* 
* FILENAME: holdcash.h
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

#ifndef HOLDCASH_H
#define HOLDCASH_H

#ifndef NT
  #define NT 1
#endif

typedef struct
{
	int		 iID;
	char	 sSecNo[12+NT];
	char	 sWi[1+NT];
	char	 sSecXtend[2+NT];
	char	 sAcctType[1+NT];
	int		 iSecID;
	long	 lAsofDate;
	char	 sSecSymbol[12+NT];
	double fUnits;
	double fTotCost;
	double fUnitCost;
	double fBaseCostXrate;
	double fSysCostXrate;
	long	 lTrdDate;
	long	 lEffDate;
	long	 lStlDate;
	long	 lLastTransNo;
	double fMktVal;
	double fMvBaseXrate;
	double fMvSysXrate;
	double fCurrencyGl;
	double fCollateralUnits;
	double fHedgeValue;
} HOLDCASH;

// holdcah related functions
typedef void	(CALLBACK* LPPRINITHOLDCASH)(HOLDCASH *);
typedef void	(CALLBACK* LPPRHOLDCASH)(HOLDCASH, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRSELECTHOLDCASH)(HOLDCASH *, int, char *, char *, char *, char *, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRALLHOLDCASHFORANACCOUNT)(int, HOLDCASH *, ERRSTRUCT *);

#endif // HOLDCASH_H

