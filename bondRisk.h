
/*		bondRisk.h

  header file for bondRisk.c   */

#include "bondTypes.h"

__declspec(dllexport) double blMacaulayDurationFromPrice(blBond *, double);
__declspec(dllexport) double blMacaulayDurationFromYield(blBond *, double);
__declspec(dllexport) double blMacaulayDuration(blBond *, double, double);
double blPriceValueOfBasisPoint(blBond *, double, double, double);
__declspec(dllexport) double blModifiedDurationFromPrice(blBond *, double);
__declspec(dllexport) double blModifiedDurationFromYield(blBond *, double);
__declspec(dllexport) double blModifiedDuration(blBond *, double, double);

