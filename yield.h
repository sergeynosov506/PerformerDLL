
/*		yield.h

  header file for yield.c  - bond library yield calculations   */

#include "bondTypes.h"

double blNewtonRaphson(void (*)(double, double *, double *, blSolutionBag *),
					   double, blSolutionBag *);
void blYieldFunction(double, double *, double *, blSolutionBag *);
//double blYieldToPrice(double, blBond *, double *);
__declspec(dllexport) double blYieldToPrice(double, blBond *, double *);
//double blPriceToYield(double, blBond *);
__declspec(dllexport) double blPriceToYield(double, blBond *);
double blDiscountRateToPrice(double, blBond *);
double blPriceToDiscountRate(double, blBond *);

