/*H*
* 
* FILENAME: holddel.h
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
#ifndef HOLDDEL_H
#define HOLDDEL_H

#ifndef NT
  #define NT 1
#endif

typedef struct
{
	long		lCreateTransNo;
  long		lCreateDate;
  long		lRevTransNo;
  int			iID;
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  char		sSecXtend[2+NT];
  char		sAcctType[1+NT];
  long		lTransNo;
	int			iSecID;
  long		lAsofDate;
  char		sSecSymbol[12+NT];
  double	fUnits;
  double	fOrigFace;
  double	fTotCost;
  double	fUnitCost;
  double	fOrigCost;
  double	fOpenLiability;
  double	fBaseCostXrate;
  double	fSysCostXrate;
  long		lTrdDate;
  long		lEffDate;
  long		lEligDate;
  long		lStlDate;
  double	fOrigYield;
  long		lEffMatDate;
  double	fEffMatPrice;
  double	fCostEffMatYld;
  long		lAmortStartDate;
  char		sOrigTransType[2+NT];
  char		sOrigTransSrce[1+NT];
  char		sLastTransType[2+NT];
  long		lLastTransNo;
  char		sLastTransSrce[1+NT];
  long		lLastPmtDate;
  char		sLastPmtType[2+NT];
  long		lLastPmtTrNo;
  long		lNextPmtDate;
  double	fNextPmtAmt;
  long		lLastPdnDate;
  char		sLtStInd[2+NT];
  double	fMktVal;
  double	fCurLiability;
  double	fMvBaseXrate;
  double	fMvSysXrate;
  double	fAccrInt;
  double	fAiBaseXrate;
  double	fAiSysXrate;
  double	fAnnualIncome;
  double	fAccrualGl;
  double	fCurrencyGl;
  double	fSecurityGl;
  double	fMktEffMatYld;
  double	fMktCurYld;
  char		sSafekInd[1+NT];
  double	fCollateralUnits;
  double	fHedgeValue;
  char		sBenchmarkSecNo[12+NT];
  char		sPermLtFlag[1+NT];
  char		sValuationSrce[2+NT];
  char	    sPrimaryType[1+NT];
  int		iRestrictionCode;

} HOLDDEL;

typedef void	(CALLBACK* LPPRHOLDDEL)(HOLDDEL, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRSELECTHOLDDEL)(HOLDDEL *, long, long, int, char *, char *, char *, char *, long, ERRSTRUCT *);

#endif // HOLDDEL_H