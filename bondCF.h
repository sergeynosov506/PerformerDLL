
/*		bondCF.h

  header file for bondCF.c   */

#include "bondTypes.h"

blBond *blUSTBondCashFlows(long, long, long, double);
blBond *blDiscountInstrumentCashFlows(long, long);
blBond *blAllocateBondStruct(int);
void blFreeBondStruct(blBond *);
void blPrintCashFlows(blBond *);
long blForwardOneCashFlowDate(blBond *, long);
long blBackOneCashFlowDate(blBond *, long);
long blExtrapolateCashFlowDate(blBond *, long, int);
double blTimeBetweenCashFlows(blBond *, int, int);
int blIsLongCoupon(blBond *, int);
int blIsShortCoupon(blBond *, int);
int blNormalCouponPeriodLength(blBond *, int, long *);
long *blComputeQuasiCashFlowDatesForward(blBond *, int, int, int *);
long *blComputeQuasiCashFlowDatesBackward(blBond *, int, int, int *);
blBond *blRegularCashFlowDates(long, long, long, int);
double blCouponAtDate(blBond *, long);
int blAllocateCouponSchedule(blBond *, int);
int blAllocatePutSchedule(blBond *, int);
int blAllocateCallSchedule(blBond *, int);
blBond *blCopyBond(blBond *);
void blBondToCall(blBond *, int);
void blBondToPut(blBond *, int);

long *blAllocateLong1dArray(int);
void blFreeLong1dArray(long *);
long *blReallocLong1dArray(long *, int);

int blCatchBadIntData(int, int);

__declspec(dllexport) blBond *blMuniBondCashFlows(long, long, long, double);
__declspec(dllexport) void blSetupDaycountConventions(blBond *);
__declspec(dllexport) void blCalcAI(blBond *);
__declspec(dllexport) void blComputeTimeToCashFlows(blBond *);
__declspec(dllexport) void blComputeCashFlowsFromDates(blBond *);




