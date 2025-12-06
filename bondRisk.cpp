
/*		bondRisk.c

  bond library risk-measurement routines  */

#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bondlib.h"
#include "dates.h"


/*   blMacaulayDurationFromPrice:

  given a price, compute the Macaulay Duration
	D = - (dP/dy) * (1+y)/P    */

__declspec(dllexport) double blMacaulayDurationFromPrice(blBond *b, double price)
{
	double yield = blPriceToYield(price, b);
	if (yield < 0) // SB 6/15/2000
		return -1;
	return blMacaulayDuration(b, price, yield);
}


/*   blMacaulayDurationFromYield:

  given a yield, compute the Macaulay Duration
	D = - (dP/dy) * (1+y)/P     */

__declspec(dllexport) double blMacaulayDurationFromYield(blBond *b, double yield)
{
	double dummy;

	double price = blYieldToPrice(yield, b, &dummy);
	if (price < 0) // SB 6/15/2000
		return -1;
	return blMacaulayDuration(b, price, yield);
}


/*   blMacaulayDuration:

  given a yield, a price, and a bond, compute the Macaulay
  Duration  D = -(dP/dy)*(1+y)/P.

  Uses a finite-difference approximation  */

__declspec(dllexport) double blMacaulayDuration(blBond *b, double price, double yield)
{
	double fDenominator = (price + b->AI);
	if (fDenominator !=0)
		return blPriceValueOfBasisPoint(b, price, yield, 0.001) * (1.0 + yield) / fDenominator;
	else
		return -1;
}



/*    blPriceValueOfBasisPoint:

  computes -(dP/dY), using finite difference approximation.  */

double blPriceValueOfBasisPoint(blBond *b, double price, double yield, double shift)
{
	double priceUp, priceDown, dummy, dP, dY;

	if (shift == 0)
		shift = 0.001;   /* 10 bp shift */

	priceUp = blYieldToPrice(yield + shift, b, &dummy);
	priceDown = blYieldToPrice(yield - shift, b, &dummy);

	dP = priceUp - priceDown;
	dY = 2.0 * shift;
	if (dY!=0)
		return -(dP/dY);
	else
		return -1;

}




/*   blModifiedDurationFromPrice:

  given a price, compute the Modified Duration
	MD = - (dP/dy) * 1/P    */

double blModifiedDurationFromPrice(blBond *b, double price)
{
	double yield = blPriceToYield(price, b);
	if (yield < 0)
		return -1;
	return blModifiedDuration(b, price, yield);
}


/*   blModifiedDurationFromYield:

  given a yield, compute the Modified Duration
	MD = - (dP/dy) * 1/P     */

double blModifiedDurationFromYield(blBond *b, double yield)
{
	double dummy, price, maturity;
	int	   iDenominator;
	double fDenominator;
	// SB 9/2/05 - For zero coupon bond Macaulay duration = maturity and 
	// modified duration = macaulay duration / (1 + yield/number of coupon per year)
	if (strcmp(b->bondCode, blCodeZero) == 0)
	{
		// if month and year basis not set, default them to be Actual
		if (b->yearBasis == 0)
			b->yearBasis = dlACT;

		if (b->monthBasis == 0)
			b->monthBasis = dlACT;

		if (b->yearBasis == dl360 || b->yearBasis == dl360E || b->yearBasis == dl360E) 
			iDenominator = 360;
		else
			iDenominator = 365;
		maturity = (double) dlDayCount(b->settleDate, b->maturityDate, b->monthBasis, 
									   b->yearBasis, b->monthEndConv) / iDenominator;
		fDenominator = (1 + yield/b->nCPerYear);
		if (fDenominator !=0)
		{
			return maturity / fDenominator;
		}
		else
		{
			return -1;
		}
	}
	
	price = blYieldToPrice(yield, b, &dummy);
	if (price < 0)
		return -1;

	return blModifiedDuration(b, price, yield);
}


/*   blModifiedDuration:

  given a yield, a price, and a bond, compute the Modified
  Duration  MD = -(dP/dy)*1/P.

  Uses a finite-difference approximation  */

__declspec(dllexport) double blModifiedDuration(blBond *b, double price, double yield)
{
	double fDenominator = (price + b->AI);
	if (fDenominator !=0 )
		return blPriceValueOfBasisPoint(b, price, yield, 0.001) / fDenominator;
	else
		return -1;
}


