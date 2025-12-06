/*H*
* 
* FILENAME: porttax.h
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
*H*/
#ifndef PORTTAX_H
#define PORTTAX_H

#include "commonheader.h"


typedef struct
{
	int			iID;
	long		lTaxDate;
	double	fFedIncomeRate;
	double	fFedStGLRate;
	double	fFedLtGLRate;
	double	fStIncomeRate;
	double	fStStGLRate;
	double	fStLtGLRate;
	double	fStockWithholdRate;
	double	fBondWithholdRate;
} PORTTAX;


typedef struct
{
  int     iCount;
  int     iCapacity;
  PORTTAX	*pzPTax;
} PORTTAXTABLE;

typedef void	(CALLBACK* LPPRPORTTAX)(int, long, PORTTAX *, ERRSTRUCT *);

#endif // PORTTAX_H