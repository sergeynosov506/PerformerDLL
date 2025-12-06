/*H*
*
* FILENAME: summdata.h
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Shobhit Barman
*
*
* 2021-06-01 J#PER11638 Broke out weighted fees out from weighted fees - mk.
* 2020-04-17 J# PER-10655 Added CN transactions -mk.
*H*/
#ifndef SUMMDATA_H
#define SUMMDATA_H

#include "commonheader.h"

#ifndef NT
	#define NT 1
#endif
 
typedef struct
{
	int			iPortfolioID;
	int			iID;
  long		lPerformDate;
  double	fMktVal;
  double	fBookValue;
  double	fAccrInc;
  double	fAccrDiv;
	double	fIncRclm;
	double	fDivRclm;
  double	fNetFlow;
  double	fCumFlow;
  double	fWtdFlow;
  double	fPurchases;
  double	fSales;
  double	fIncome;    
  double	fCumIncome;
  double	fWtdInc;
  double	fFees;    
  double	fCumFees;
  double	fWtdFees;
  double	fExchRateBase;
  char		sIntervalType[2+NT];
  long		lDaysSinceNond;
  long		lDaysSinceLast;
  double	fCreateDate;
  double	fChangeDate;
  char		sPerformType[1+NT];
  double	fPrincipalPayDown;
	double	fMaturity;
  double	fContribution;
	double	fWithdrawals;
	double	fExpenses;
	double	fReceipts;
	double	fIncomeCash;
  double	fPrincipalCash;
	double	fFeesOut;
	double	fCumFeesOut;
	double	fWtdFeesOut;
	double	fTransfers;
	double	fTransfersIn;
	double	fTransfersOut;
  int			iCreatedBy;
	double  fEstAnnIncome;
	double	fNotionalFlow;
    double  fCNFees;
    double	fCumCNFees;
    double	fWtdCNFees;
} SUMMDATA;

typedef void  (CALLBACK* LPPRSUMMDATA)(SUMMDATA, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSUMMDATA1LONG)(SUMMDATA, long, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRPSUMMDATA2LONG)(SUMMDATA *, long, long, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRPSUMMDATA3LONG)(SUMMDATA *, long, long, long, ERRSTRUCT *);
//typedef void  (CALLBACK* LPPRMONTHLYWEIGHTEDINFO)(SUMMDATA *, RTRNSET *, long, long,  ERRSTRUCT *);

#endif // SUMMDATA_H