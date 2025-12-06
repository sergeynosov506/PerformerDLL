
/*    yield.c


  Bond library yield calculations    */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bondTypes.h"
#include "yield.h"
#include "dates.h"
#include "commonheader.h"

/*  Newton-Raphson solver, used for computing yield given price.
		Taken from Numerical Recipes, p. 365

    ARGUMENTS:  pointer to void function F(x) which takes arguments
						double x
						double *f    returns value F(x)
						double *df   returns values F'(x)
						blSolutionBag *bag   holds useful information 
				double xacc    accuracy to compute solution to
				blSolutionBag *sb     solution 'bag' to pass through  */
double blNewtonRaphson(void (*funcd)(double, double *, double *, blSolutionBag *),
											 double xacc, blSolutionBag *sb)
{
	int j, maxitt = 200;
	double df, dx, f, rtn;

	/* Initial guess */
//	rtn = sb->bond->coupon / 100.0;
	rtn = 0;
	for (j=1; j <= maxitt; j++)  
	{
		(*funcd)(rtn, &f, &df, sb);
		if (IsValueZero(df, 8))
			return -1;
		dx = f / df;
	  rtn -= dx;
		if (fabs(dx) < xacc)
			return rtn;    /* solution converged */
	}
	
	fprintf(stderr, "blNewtonRaphson:  exceeded maximum number of iterations.  Solution fails.\n");
	return rtn;//was -10000
}



/*     blYieldFunction

  given a yield, compute fval = price given yield - price of bond (as in the
								blSolutionBag structure)  refer to TIPS manual p. 132 (eq. 5)
						 df   = derivative of fval at the yield
		the blSolutionBag structure holds some useful information about the bond  */

void blYieldFunction(double yield, double *fval, double *df, blSolutionBag *sb)
{
	double deriv = 0;

	/*  function value is the price given yield, minus what our actual price is */
	/*  it also returns the derivative  */
	*fval = blYieldToPrice(yield, sb->bond, &deriv) - sb->flatPrice;
	*df = deriv;

	return;
}



/*      blYieldToPrice

  given a yield and a bond structure (time to cashflows, etc.), compute
  the price for the bond given this yield.  Refer to TIPS, p. 66   */

__declspec(dllexport) double blYieldToPrice(double yield, blBond *b, double *deriv)
{
	int k;
	double p = 0, dp = 0;

	/* discount instrument */
	if (strcmp(b->bondCode, blCodeDiscount) == 0)  {
		int nDaysInYear = (b->yearBasis == dlACT ? 365 : 360);
		p = b->CF[1] / (1.0 + ((double)b->nDaysSettleToNextCoupon * yield / (double)nDaysInYear));
		return p;
	}

	/* last-coupon case: simple yield */
	if (b->nCF == 1)  
	{
//		double denom = 1 + ((yield / (double)b->nCPerYear) * b->tCF[1] * 2.0);
		double denom = 1 + (yield * b->tCF[1]);

		p = (b->CF[1] / denom) - b->AI;
		return p;
	}

	for (k=1; k <= b->nCF; k++)  
	{
		double power = -((double)b->nCPerYear * b->tCF[k]);
		double x = (1.0 + (yield / (double)b->nCPerYear));
		p += b->CF[k] * pow(x, power);
		dp += b->CF[k] * power * pow(x, power - 1);
	}

	p -= b->AI;     /* subtract off accrued interest */
	*deriv = dp;

	return p;
}

	

/*		blPriceToYield

	given a flat price and a bond structure (time to cashflows, etc.), compute
	the yield for the bond given this price.  */

__declspec(dllexport) double blPriceToYield(double price, blBond *b)
{
	blSolutionBag sb;
	double yield;

	// SB 6/15/2000 - Extra checks to prevent divide by zero
	if (IsValueZero(price, 6))
		return -1;

	/* discount instruments */
	if (strcmp(b->bondCode, blCodeDiscount) == 0)  
	{
		int nDaysInYear = (b->yearBasis == dlACT ? 365 : 360);
		yield = ((b->CF[1] - price) / price) *
			((double)nDaysInYear / (double)b->nDaysSettleToNextCoupon);
		return yield;
	}

	// SB - 3/8/2001 - Added Zero Coupon calculations (Formula 20 & 22 
/*	if (strcmp(b->bondCode, blCodeZero) == 0)  
	{
		if (b->nCF == 1)
		{
			int nDaysInYear = (b->yearBasis == dlACT ? 365 : 360);

			yield = ((b->CF[1] - price) / price) * ((double)b->nCPerYear * nDaysInYear / (double)b->nDaysSettleToNextCoupon);
		  return yield;
		}
		else
		{
			double power = 1.0/((double)b->nCF - 1 + ((double)b->nDaysStartToSettle / (double)b->nDaysSettlementPeriod));
			yield = (pow((b->CF[b->nCF] / price), power) - 1) * (double)b->nCPerYear;
			return yield;
		}
	} // Zero Coupon*/

	// SB 6/15/2000 - Extra checks to prevent divide by zero
	if (b->nCF == 1 && IsValueZero(b->tCF[1], 8))
		return -1;

	/* last-coupon case */
	if (b->nCF == 1)  
	{
		double v = b->AI + price;
    // SB 4/12/02 - It's wrong to multiply denominator by 2, it should be multiplied by payment frequency
//		double dateAdj = (double)b->nCPerYear / (b->tCF[1] * 2.0);
		double dateAdj = (double)b->nCPerYear / (b->tCF[1] * b->nCPerYear);

		// SB 6/15/2000 - Extra checks to prevent divide by zero
		if (IsValueZero(v, 8))
			return -1;

		yield = dateAdj * ((b->CF[1] - v) / v);
		return yield;
	}

	sb.bond = b;
	sb.flatPrice = price;

	yield = blNewtonRaphson(&blYieldFunction, 1e-9, &sb);
	return yield;
}


/*      blDiscountRateToPrice

  given a discount rate and a bond structure (time to cashflows, etc.) for
  a discount instrument, compute the price of the security.  */

double blDiscountRateToPrice(double rate, blBond *b)
{
	double price = 0;
	int nDaysInYear = (b->yearBasis == dlACT ? 365 : 360);

	price = b->CF[1] * (1 - rate * ((double)b->nDaysSettleToNextCoupon / (double)nDaysInYear));
	return price;
}


/*		blPriceToDiscountRate

	given a price and a bond structure for a discount instrument,
	compute the discount rate for the security.  */

double blPriceToDiscountRate(double price, blBond *b)
{
	int nDaysInYear = (b->yearBasis == dlACT ? 365 : 360);
	double rate;

	rate = ((b->CF[1] - price) * (double)nDaysInYear) / (b->CF[1] * (double)b->nDaysSettleToNextCoupon);
	return rate;
}

