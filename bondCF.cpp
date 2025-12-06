
/*      bondCF.c

  bond library cashflow generation routines  */

#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bondlib.h"
#include "dates.h"
#include "commonheader.h"


/*    blUSTBondCashFlows:

  given a pointer to a blBond structure, fill out
  structure as a UST bond.  Fill in the structure's
  maturityDate, settleDate, coupon  */

blBond *blUSTBondCashFlows(long settleDate, long maturityDate, long issueDate, double cpn)
{
	blBond *b;

	b = blRegularCashFlowDates(settleDate, maturityDate, issueDate, 2);
	if (!b)
		return NULL;

	b->coupon = cpn;
	strcpy_s(b->bondCode, blCodeUST);

	blSetupDaycountConventions(b);
	blComputeTimeToCashFlows(b);
	blComputeCashFlowsFromDates(b);
	blCalcAI(b);
	return b;
}



/*    blMuniBondCashFlows:

  given a setlement date, maturity date, issue date, and coupon,
  allocate space for a return a blBond structure
  which has cashflow/cashflow date information set
  up appropriately.  Assumes semi-annual payment,
  nothing funky (odd coupons, etc.)  */

__declspec(dllexport) blBond *blMuniBondCashFlows(long settleDate, long maturityDate, long issueDate, double cpn)
{
	blBond *b;

	b = blRegularCashFlowDates(settleDate, maturityDate, issueDate, 2);
	if (!b)
		return NULL;

	b->coupon = cpn;
	strcpy_s(b->bondCode, blCodeMuni);

	blSetupDaycountConventions(b);
	blComputeTimeToCashFlows(b);
	blComputeCashFlowsFromDates(b);
	blCalcAI(b);

	return b;
}



/*	blSetupDaycountConventions

	given the bond code, map from the bond code to daycount convention.  */

__declspec(dllexport) void blSetupDaycountConventions(blBond *b)
{
	b->monthEndConv = 1;
	if ((strcmp(b->bondCode, blCodeUST) == 0) ||
		(strcmp(b->bondCode, blCodeZero) == 0))  {  /* treasury or zero */
		b->monthBasis = dlACT;
		b->yearBasis = dlACT;
	}
	else if (strcmp(b->bondCode, blCodeMuni) == 0)  {  /* muni */
		b->monthBasis = dl30;
		b->yearBasis = dl360;
	}
	else if (strcmp(b->bondCode, blCodeDiscount) == 0)  {  /* discount instrument */
		b->monthBasis = dlACT;
		b->yearBasis = dl360;
	}
	else  {   /* we don't know, so guess 30/360 */
		b->monthBasis = dl30;
		b->yearBasis = dl360;
	}

	return;
}



/*   blCalcAI:

	do some daycount computations to fill out the bond structure
	and compute the AI based on that information.  */

__declspec(dllexport) void blCalcAI(blBond *b)
{
	long dummy;
	double coupon;

	if (strcmp(b->bondCode, blCodeDiscount) == 0)  {  /* discount instrument */
		b->nDaysSettleToNextCoupon = dlDayCount(b->settleDate, b->CFdate[1], dlACT, dlACT, b->monthEndConv);
		b->AI = 0;
		return;
	}

	coupon = blCouponAtDate(b, b->CFdate[1]);
	/*  long coupons are funny  */
	if (blIsLongCoupon(b, 1))  {
		long *computedDates;
		int nQuasiCoupons, i;
		double AIfactor = 0, denom;

		computedDates = blComputeQuasiCashFlowDatesBackward(b, 0, 1, &nQuasiCoupons);
		/* figure out where settlement falls in our quasi world */
		i = 0;
		while ((i <= nQuasiCoupons) && (b->settleDate > computedDates[i]))
			i++;
		b->nDaysStartToSettle = dlDayCount(b->CFdate[0], b->settleDate, b->monthBasis, b->yearBasis, b->monthEndConv);
		if (i == 1)  { /* in first quasi date */
			denom = (double)dlDayCount(computedDates[0], computedDates[1], b->monthBasis, b->yearBasis, b->monthEndConv);
			AIfactor = (IsValueZero(denom, 8) ? 0 : (double)b->nDaysStartToSettle / denom);
		}
		else  {  /* first piece, plus wholes, plus last piece */
			denom = (double)dlDayCount(computedDates[0], computedDates[1], b->monthBasis, b->yearBasis, b->monthEndConv);
			if (!IsValueZero(denom, 8)) {   /*  ensure no floating-point exceptions  */
				AIfactor = (double)dlDayCount(b->CFdate[0], computedDates[1], b->monthBasis, b->yearBasis, b->monthEndConv) / denom;
				AIfactor += i - 2;  /* whole coupon periods */
				denom = (double)dlDayCount(computedDates[i-1], computedDates[i], b->monthBasis, b->yearBasis, b->monthEndConv);
				if (!IsValueZero(denom, 8))  {  /* ensure no floating-point exceptions */
					AIfactor += (double)dlDayCount(computedDates[i-1], b->settleDate, b->monthBasis, b->yearBasis, b->monthEndConv) / denom;
				}
				else  {    /* floating-point problem */
					AIfactor = 0;
				}
			}
			else  {   /* floating-point problem */
				AIfactor = 0;
			}
		}
		if (b->nCPerYear)   /* catch bad nCPerYear */
		  b->AI = coupon * AIfactor / (double)b->nCPerYear;
		else
		  b->AI = 0;

		blFreeLong1dArray(computedDates);
	}
	else  {  /* short or regular are easy */
		b->nDaysSettlementPeriod = blNormalCouponPeriodLength(b, 1, &dummy);
		b->nDaysStartToSettle = dlDayCount(b->CFdate[0], b->settleDate, b->monthBasis, b->yearBasis, b->monthEndConv);
		if (b->nCPerYear && b->nDaysSettlementPeriod)
			b->AI = coupon * (double)b->nDaysStartToSettle /
				((double)b->nCPerYear * (double)b->nDaysSettlementPeriod);	
		else   /* bad data or computation */
			b->AI = 0;
	}

	return;
}



/*    blDiscountInstrumentCashFlows:

  given a pointer to a blBond structure, fill out
  structure as a discount instrument (BA, Tbill, etc.)  */

blBond *blDiscountInstrumentCashFlows(long settleDate, long maturityDate)
{
	blBond *b;

	if (settleDate > maturityDate)
		return NULL;

	b = blAllocateBondStruct(1);   /* one cashflow for discount instrument */
	if (!b)
		return NULL;

	b->nCPerYear = 0;
	b->nCF = 1;
	b->settleDate = settleDate;
	b->maturityDate = maturityDate;

	/*  set up cashflows.  */
	b->CF[0] = 0;
	b->CF[1] = 100;

	/*  set up cashflow dates  */
	b->CFdate[0] = b->settleDate;
	b->CFdate[1] = b->maturityDate;

	strcpy_s(b->bondCode, blCodeDiscount);
	blSetupDaycountConventions(b);
	blCalcAI(b);

	return b;
}


/*  blAllocateBondStruct

  given an int which is the number of cashflows
  remaining on the bond, allocate space for a new blBond
  structure.  */

blBond *blAllocateBondStruct(int nCF)
{
	blBond *b;
	
	b = (blBond *)calloc(1, sizeof(blBond));
	if (!b)  {
		fprintf(stderr, "blAllocateBondStruct:  cannot allocate space.\n");
		return NULL;
	}

	b->nCF = nCF;

	b->CF = (double *)calloc(b->nCF + 1, sizeof(double));
	if (!b->CF)  {
		fprintf(stderr, "blAllocateBondStruct:  cannot allocate space for CF.\n");
		blFreeBondStruct(b);
		return NULL;
	}

	b->CFdate = (long *)calloc(b->nCF + 1, sizeof(long));
	if (!b->CFdate)  {
		fprintf(stderr, "blAllocateBondStruct:  cannot allocate space for CFdate.\n");
		blFreeBondStruct(b);
		return NULL;
	}

	b->tCF = (double *)calloc(b->nCF + 1, sizeof(double));
	if (!b->tCF)  {
		fprintf(stderr, "blAllocateBondStruct:  cannot allocate space for tCF.\n");
		blFreeBondStruct(b);
		return NULL;
	}

	/*  default to an empty coupon schedule, put schedule, call schedule  */
	b->nCouponSchedule = 0;
	b->couponSchedule = NULL;

	b->nCallSchedule = 0;
	b->callSchedule = NULL;

	b->nPutSchedule = 0;
	b->putSchedule = NULL;

	return b;
}



/*	blFreeBondStruct

	frees space allocated for a blBond  */

void blFreeBondStruct(blBond *b)
{
	if (!b)
		return;

	if (b->CF)
		free(b->CF);
	if (b->CFdate)
		free(b->CFdate);
	if (b->tCF)
		free(b->tCF);
	if (b->couponSchedule)
		free(b->couponSchedule);
	if (b->callSchedule)
		free(b->callSchedule);
	if (b->putSchedule)
		free(b->putSchedule);

	free(b);
	return;
}


/*  blPrintCashflows

	debugging routine; print a bond's cashflows to the screen  */

void blPrintCashFlows(blBond *b)
{
	int i;

	for (i=0; i <= b->nCF && i <= 10; i++)  {
		printf("n: %3d.  CFdate = %8d, tCF = %15.11f, CF = %8.6f\n",
			i, b->CFdate[i], b->tCF[i], b->CF[i]);
	}

	return;
}



/*   blForwardOneCashFlowDate

	given a bond structure and a date, go forwards one quasi-coupon from the given
	date, based on information in the bond structure.

  */

long blForwardOneCashFlowDate(blBond *b, long date)
{
	return blExtrapolateCashFlowDate(b, date, 1);
}


/*   blBackOneCashFlowDate

	given a bond structure and a date, go back one quasi-coupon from the given
	date, based on information in the bond structure.

  */

long blBackOneCashFlowDate(blBond *b, long date)
{
	return blExtrapolateCashFlowDate(b, date, -1);
}



/*   blExtrapolateCashFlowDate

	given a bond structure and a date, extrapolate a coupon date
	based on the given date, factor, and information in the bond
	structure.
  */

long blExtrapolateCashFlowDate(blBond *b, long date, int factor)
{
	int couponAtEnd = 0;
	int nMonths;
	long newDate;

	nMonths = 12 / blCatchBadIntData(b->nCPerYear, 2);

	/* determine if this security pays coupon on the last day of the month */
	if (dlWhichDay(b->maturityDate) ==
		dlDaysInMonth(dlWhichMonth(b->maturityDate), dlWhichYear(b->maturityDate)))
		couponAtEnd = 1;

	newDate = dlAddMonthsToDate(date, factor * nMonths);
	if (couponAtEnd)
		newDate = dlMakeDateLast(newDate);
	if (!dlIsValidDate(newDate))  {
		fprintf(stderr, "blExtrapolateCashFlowDate:  generated an invalid date!\n");
		return date;
	}

	return newDate;
}



/*     blComputeTimeToCashFlows:


  given a bond structure, fill out the tCF field appropriately.  */

__declspec(dllexport) void blComputeTimeToCashFlows(blBond *b)
{
	int i;
	double factor = 1.0/(double)b->nCPerYear;
	int nDaysStartToSettle,nDaysToFirstCoupon;

	for (i=0; i <= b->nCF; i++)    /* reset time-to-cashflow to zero */
		b->tCF[i] = 0;

	/*  for time to the first cashflow, determine the time from
		settlement date to the first cashflow, using correct conventions  */

	if (blIsLongCoupon(b, 1))  {    /* long first coupon? */
		long *computedDates;
		int nQuasiCoupons, /*nDaysToFirstCoupon,*/ nWholePeriods;

		computedDates = blComputeQuasiCashFlowDatesBackward(b, 0, 1, &nQuasiCoupons);
		// SB 2/2/2000 
		if (nQuasiCoupons == 0)
		{
			blFreeLong1dArray(computedDates);
			return;
		}
		i = 0;
		while ((i <= nQuasiCoupons) && (b->settleDate > computedDates[i]))
			i++;
		nWholePeriods = nQuasiCoupons - i;
		nDaysToFirstCoupon = dlDayCount(b->settleDate, computedDates[i], b->monthBasis, b->yearBasis, b->monthEndConv);
		b->nDaysSettlementPeriod = dlDayCount(computedDates[i-1], computedDates[i], b->monthBasis, b->yearBasis, b->monthEndConv);
		b->nDaysSettlementPeriod = blCatchBadIntData(b->nDaysSettlementPeriod, 181);
		b->tCF[1] = factor * (((double)nDaysToFirstCoupon / (double)b->nDaysSettlementPeriod) +
			(double)nWholePeriods);

		blFreeLong1dArray(computedDates);
	}
	else if (blIsShortCoupon(b, 1))  {   /* short first coupon? */
		long quasiCouponDate = blBackOneCashFlowDate(b, b->CFdate[1]);
		nDaysToFirstCoupon = dlDayCount(b->settleDate, b->CFdate[1], b->monthBasis, b->yearBasis, b->monthEndConv);
		b->nDaysSettlementPeriod = dlDayCount(quasiCouponDate, b->CFdate[1], b->monthBasis, b->yearBasis, b->monthEndConv);
		b->nDaysSettlementPeriod = blCatchBadIntData(b->nDaysSettlementPeriod, 181);
		b->tCF[1] = factor * (double)nDaysToFirstCoupon / (double)b->nDaysSettlementPeriod;
	}
	else  {   /* regular coupon period */
		// ACCORDING TO STANDARD SECURITIES CALCULATIONS METHDS BOOK  pp 65 nDaysToFirstCoupon 
		// should be nDaysSettlementeriod - (days from settlement day to first coupon day)
//		int nDaysToFirstCoupon = dlDayCount(b->settleDate, b->CFdate[1], b->monthBasis, b->yearBasis, b->monthEndConv);
		nDaysStartToSettle = dlDayCount(b->CFdate[0], b->settleDate, b->monthBasis, b->yearBasis, b->monthEndConv);
//
		b->nDaysSettlementPeriod = dlDayCount(b->CFdate[0], b->CFdate[1], b->monthBasis, b->yearBasis, b->monthEndConv);
		b->nDaysSettlementPeriod = blCatchBadIntData(b->nDaysSettlementPeriod, 181);
		b->tCF[1] = factor * ( (double)b->nDaysSettlementPeriod - (double)nDaysStartToSettle) / (double)b->nDaysSettlementPeriod;
	}

	for (i=1; i < b->nCF; i++)
		b->tCF[i+1] = b->tCF[i] + blTimeBetweenCashFlows(b, i, i+1);

	return;
}



/*	blTimeBetweenCashFlows:

  given a bond structure and the two cashflow indicies that we want
  to compute the time between, return a double which is the amount of
  time (in years) between the cashflows.

  Don't use for first coupon - conventions are slightly different

  Make sure that b->nDaysSettlementPeriod is set to the number of
  days in a regular coupon period.  */

double blTimeBetweenCashFlows(blBond *b, int cf1, int cf2)
{
	int i;
	double factor;
	
	factor = 1.0/(double)blCatchBadIntData(b->nCPerYear, 2);

	if (cf1 == cf2)     /* degenerate case */
		return 0;
	else if (cf1 > cf2)  {  /* backwards order from what we expect */
		i = cf1;
		cf1 = cf2;
		cf2 = i;   
	}    /* so cf1 is before cf2 */

	if (blIsLongCoupon(b, cf2))  {   /* handle long coupons */
		long *computedDates;
		int nQuasiCoupons, daysLastQuasiToCF, daysInPeriod;

		computedDates = blComputeQuasiCashFlowDatesForward(b, cf1, cf2, &nQuasiCoupons);
		if (nQuasiCoupons < 0) {
			blFreeLong1dArray(computedDates);
			return 0;
		}
		daysLastQuasiToCF = dlDayCount(b->CFdate[cf2], computedDates[nQuasiCoupons-1], b->monthBasis, b->yearBasis, b->monthEndConv);
		daysInPeriod = dlDayCount(computedDates[nQuasiCoupons-1], computedDates[nQuasiCoupons], b->monthBasis, b->yearBasis, b->monthEndConv);
		daysInPeriod = blCatchBadIntData(daysInPeriod, 180);

		blFreeLong1dArray(computedDates);
		return factor * (((double)daysLastQuasiToCF / (double)daysInPeriod) +
			(nQuasiCoupons - 1));
	}
	else if (blIsShortCoupon(b, cf2))  {  /* handle short coupons */
		long quasiCouponDate;
		int normalLengthCouponPeriod, daysLastCouponToThis;

		normalLengthCouponPeriod = blNormalCouponPeriodLength(b, cf2, &quasiCouponDate);
		normalLengthCouponPeriod = blCatchBadIntData(normalLengthCouponPeriod, 180);
		daysLastCouponToThis = dlDayCount(b->CFdate[cf1], b->CFdate[cf2], b->monthBasis, b->yearBasis, b->monthEndConv);

		return factor * ((double)daysLastCouponToThis / (double)normalLengthCouponPeriod);
	}
	else   /* just a regular coupon */
		return factor;
}



/*	blIsLongCoupon

	Given a bond structure and the number of the cashflow we want to
	check, determine if this is a long coupon period.  Returns true (1)
	if yes, otherwise 0.   */

int blIsLongCoupon(blBond *b, int cf)
{
	int nDaysThisPeriod;
	double r;
	
	nDaysThisPeriod = dlDayCount(b->CFdate[cf-1], b->CFdate[cf], dl30, dl360, b->monthEndConv);
	nDaysThisPeriod = blCatchBadIntData(nDaysThisPeriod, 180);
	r = 360.0 / (double)nDaysThisPeriod;

	/*  it's a LONG COUPON if the ratio is more than a few days off */
	if (r < (double)b->nCPerYear)
		return 1;

	return 0;
}



/*	blIsShortCoupon

	given a bond structure and a cashflow number, determine if this
	coupon is a short coupon.  Returns true (1) if yes, 0 otherwise.  */

int blIsShortCoupon(blBond *b, int cf)
{
	int nDaysThisPeriod;
	double r; 
	
	nDaysThisPeriod = dlDayCount(b->CFdate[cf-1], b->CFdate[cf], dl30, dl360, b->monthEndConv);
	nDaysThisPeriod = blCatchBadIntData(nDaysThisPeriod, 180);
	r = 360.0 / (double)nDaysThisPeriod;

	/*  it's a SHORT COUPON if the ratio is more than a few days off */
	if (r > (double)b->nCPerYear)
		return 1;

	return 0;
}


/*   blNormalCouponPeriodLength

  Given a bond structure and the number of the cashflow we're
  interested in, return the normal coupon period length for this
  particular cashflow period  */

int blNormalCouponPeriodLength(blBond *b, int CFnum, long *quasiDate)
{
	int normalLengthCouponPeriod;
	long quasiCouponDate = b->CFdate[CFnum-1];
	
	if (blIsShortCoupon(b, CFnum) || blIsLongCoupon(b, CFnum))  {  /* handle odd coupons */
		if (CFnum == b->nCF)  { /* last cashflow: step forward from previous */
			quasiCouponDate = blForwardOneCashFlowDate(b, b->CFdate[CFnum-1]);
			normalLengthCouponPeriod = dlDayCount(quasiCouponDate, b->CFdate[CFnum-1], b->monthBasis, b->yearBasis, b->monthEndConv);
		}
		else  {  /* step backwards from next  */
			quasiCouponDate = blBackOneCashFlowDate(b, b->CFdate[CFnum]);
			normalLengthCouponPeriod = dlDayCount(quasiCouponDate, b->CFdate[CFnum], b->monthBasis, b->yearBasis, b->monthEndConv);
		}
	}
	else  {  /* regular coupon period */
		normalLengthCouponPeriod = dlDayCount(b->CFdate[CFnum-1], b->CFdate[CFnum], b->monthBasis, b->yearBasis, b->monthEndConv);
	}

	*quasiDate = quasiCouponDate;
	return normalLengthCouponPeriod;
}



/*   blComputeQuasiCashFlowDatesForward

  set cf1 < cf2.  (procedure may switch the two)
  starting at b->CFdate[cf1], extrapolate normal-coupon-length cashflow
  dates forward until the cashflow date is >= b->CFdate[cf2].

  store the dates in QCdate, and return the maximum number of them in nQC.

  QCdate[0] = b->CFdate[cf1]
    QCdate[1]   ...  QCdate[nQC-1]
  QCdate[nQC] >= b->CFdate[cf2]

  assumes enough memory has already been allocated for QCdate[].

  */

long *blComputeQuasiCashFlowDatesForward(blBond *b, int cf1, int cf2, int *nQC)
{
	int i, nAlloc;
	long extrapolatedDate,lLastDate;
	long *QCdate;   /* will hold the array of dates */

	nAlloc = 10;
	QCdate = blAllocateLong1dArray(nAlloc);   /* initial allocation */

	/* ensure that cf1 < cf2) */
	if (cf1 == cf2)  {   /* degenerate case */
		*nQC = 1;
		QCdate[0] = b->CFdate[cf1];
		QCdate[1] = b->CFdate[cf1];
		return QCdate;
	}	
	else if (cf1 > cf2)  {  /* backwards order from what we expect */
		i = cf1;
		cf1 = cf2;
		cf2 = i;   
	}    /* so cf1 is before cf2 */

	i = 1;
	QCdate[0] = b->CFdate[cf1];
	QCdate[1] = b->CFdate[cf2];  /* fallback case */
	lLastDate = extrapolatedDate = b->CFdate[cf1];

	while (extrapolatedDate < b->CFdate[cf2])  {
		extrapolatedDate = blForwardOneCashFlowDate(b, QCdate[i-1]);
		//To avoid infinite loops kashif
		if (lLastDate == extrapolatedDate)
		{
			*nQC = 0;
			return QCdate;
		}
		lLastDate = extrapolatedDate;
		QCdate[i++] = extrapolatedDate;
		if (i >= nAlloc)  {  /* see if we need to reallocate */
			nAlloc += 10;
			QCdate = blReallocLong1dArray(QCdate, nAlloc);
			if (!QCdate) //if cannot reallocate break loop and return null 
			{
				i = 0;
				break;
			}
		}
	}
	if (i != 1)
		i--;

	*nQC = i;
	return QCdate;
}



/*   blComputeQuasiCashFlowDatesBackward

  set cf1 < cf2.  (procedure may switch the two)
  starting at b->CFdate[cf2], extrapolate normal-coupon-length cashflow
  dates backward until the cashflow date is <= b->CFdate[cf1].

  store the dates in QCdate, and return the maximum number of them in nQC.

  QCdate[0] <= b->CFdate[cf1]
    QCdate[1]   ...  QCdate[nQC-1]
  QCdate[nQC] = b->CFdate[cf2]

  assumes enough memory has already been allocated for QCdate[].

  */

long *blComputeQuasiCashFlowDatesBackward(blBond *b, int cf1, int cf2, int *nQC)
{
	int		i, nAlloc;
	long	extrapolatedDate, lLastDate;
	long	*QCdate;

	nAlloc = 10;
	QCdate = blAllocateLong1dArray(nAlloc);   /* initial allocation */

	/* ensure that cf1 < cf2 */
	if (cf1 == cf2)  {   /* degenerate case */
		*nQC = 1;
		QCdate[0] = b->CFdate[cf1];
		QCdate[1] = b->CFdate[cf1];
		return QCdate;
	}	
	else if (cf1 > cf2)  {  /* backwards order from what we expect */
		i = cf1;
		cf1 = cf2;
		cf2 = i;   
	}    /* so cf1 is before cf2 */

	/*  start at b->CFdate[cf2] and work backwards (periodically) until we find
		the cashflow date that is <= b->CFdate[cf1].  Then count forwards and
		store the extrapolated dates.  */

	lLastDate = extrapolatedDate = b->CFdate[cf2];
	while (extrapolatedDate > b->CFdate[cf1])
	{
		extrapolatedDate = blBackOneCashFlowDate(b, extrapolatedDate);
		// SB 2/2/2000 - to avoid infinite loop
		if (extrapolatedDate == lLastDate)
		{
			*nQC = 0;
			return QCdate;
		}
		lLastDate = extrapolatedDate;
	}

	i = 1;
	QCdate[0] = extrapolatedDate;
	lLastDate=QCdate[1] = b->CFdate[cf2];  /* fallback case */

	while (extrapolatedDate < b->CFdate[cf2])  {
		extrapolatedDate = blForwardOneCashFlowDate(b, QCdate[i-1]);

		//To avoid infinite loop
		if (lLastDate == extrapolatedDate)
		{
			*nQC = 0;
			return QCdate;
		}
		lLastDate = extrapolatedDate;
		QCdate[i++]= extrapolatedDate;
		if (i >= nAlloc)  {  /* we need to allocate more space */
			nAlloc += 10;
			QCdate = blReallocLong1dArray(QCdate, nAlloc);
		}

	}
	if (i != 1)
		i--;
	QCdate[i] = b->CFdate[cf2];

	*nQC = i;
	return QCdate;
}


/*  blComputeCashFlowsFromDates

  run through the given bond structure and compute the various
  cashflow payments on each date.  Assumes bullet principal payment at
  maturity.  Will handle long/short coupons but not stepped coupons.  */

__declspec(dllexport) void blComputeCashFlowsFromDates(blBond *b)
{
	int i;
	long dummy;
	double coupon, denom;

	for (i=1; i <= b->nCF; i++)  {
		coupon = blCouponAtDate(b, b->CFdate[i]);
		if (blIsShortCoupon(b, i))  {  /* short coupon */
			int nDaysQuasi = blNormalCouponPeriodLength(b, i, &dummy);
			int nDaysAct = dlDayCount(b->CFdate[i-1], b->CFdate[i], b->monthBasis, b->yearBasis, b->monthEndConv);
			if (b->nCPerYear && nDaysQuasi)  /* catch bad data */
				b->CF[i] = coupon * (double)nDaysAct / ((double)b->nCPerYear * (double)nDaysQuasi);
			else
				b->CF[i] = 0;
		}
		else if (blIsLongCoupon(b, i))  {  /* long coupon */
			int nQuasiCoupons;
			long *computedDates;
			double factor;

			if (i == 1)  {  /* long first coupon:  work backwards */
				computedDates = blComputeQuasiCashFlowDatesBackward(b, 0, 1, &nQuasiCoupons);
				denom = (double)dlDayCount(computedDates[0], computedDates[1], b->monthBasis, b->yearBasis, b->monthEndConv);
				if (IsValueZero(denom, 8))   /* catch bad data */
					factor = (double)dlDayCount(b->CFdate[0], computedDates[1], b->monthBasis, b->yearBasis, b->monthEndConv) / denom;
				else
					factor = 0;
				factor += nQuasiCoupons - 1;
			}
			else  {  /* long non-first coupon:  work forwards */
				computedDates = blComputeQuasiCashFlowDatesForward(b, i-1, i, &nQuasiCoupons);
				if (nQuasiCoupons < 0)
				{
					blFreeLong1dArray(computedDates);
					return;
				}
				denom = (double)dlDayCount(computedDates[nQuasiCoupons-1], computedDates[nQuasiCoupons], b->monthBasis, b->yearBasis, b->monthEndConv);
				if (IsValueZero(denom, 8))
					factor = (double)dlDayCount(b->CFdate[i], computedDates[nQuasiCoupons-1], b->monthBasis, b->yearBasis, b->monthEndConv) / denom;
				else
					factor = 0;
				factor += nQuasiCoupons - 1;
			}
			if (b->nCPerYear)  /* catch bad data */
				b->CF[i] = coupon * factor / (double)b->nCPerYear;
			else
				b->CF[i] = 0;
			blFreeLong1dArray(computedDates);
		}
		else  /* regular coupon */
			b->CF[i] = coupon / (double)blCatchBadIntData(b->nCPerYear, 2);
	}
	b->CF[b->nCF] += 100;  /* bullet principal */

	return;
}


/*    blRegularCashFlowDates

  given a settlement date, maturity date, issue date, and number of coupons per year,
  allocate a new bond structure and fill it in with the correct cashflow dates,
  based on periodic payments

  does not handle odd coupon periods!

*/

blBond *blRegularCashFlowDates(long settleDate, long maturityDate, long issueDate, int nCPerYear)
{
	double yearsLeft;
	int i, nCF;
	blBond *b, *bNew;

	if (settleDate >= maturityDate)
		return NULL;

	/*  determine how many coupon periods there are to go */
	yearsLeft = (double)dlDayCount(settleDate, maturityDate, dl30, dl360, 1) / 360.0;

	nCF = (int)(yearsLeft * nCPerYear) + 1;

	b = blAllocateBondStruct(nCF);
	if (!b)
		return NULL;

	b->settleDate = settleDate;
	b->maturityDate = maturityDate;
	b->nCPerYear = nCPerYear;
	b->issueDate = issueDate;

	/*  set up cashflow dates:  maturity  */
	b->CFdate[b->nCF] = b->maturityDate;

	/*  now work backwards from maturity  */
	for (i=b->nCF - 1; i >= 0; i--)
		b->CFdate[i] = blBackOneCashFlowDate(b, b->CFdate[i+1]);

	/* make sure we don't go back too far */
	if (dlIsValidDate(b->issueDate) && b->CFdate[0] < b->issueDate)
		b->CFdate[0] = b->issueDate;

	/* make sure first cashflow date isn't the settlement date */
	if (b->CFdate[1] == settleDate)  {  /* fix the structure, then */
		bNew = blAllocateBondStruct(nCF - 1);
		if (!bNew)  {
			blFreeBondStruct(b);
			return NULL;
		}
		bNew->settleDate = b->settleDate;
		bNew->maturityDate = b->maturityDate;
		bNew->nCPerYear = b->nCPerYear;
		bNew->issueDate = b->issueDate;
		for (i = 1; i <= b->nCF; i++)
			bNew->CFdate[i-1] = b->CFdate[i];
		blFreeBondStruct(b);
		return bNew;
	}

	return b;
}



/*    blCouponAtDate:

  given a date, look in the bond structure and return the coupon
  at that particular time.  This will handle stepped coupons by
  looking in the bond "couponStep" information, if it exists.  */

double blCouponAtDate(blBond *b, long date)
{
	int i;

	if (!b->couponSchedule || (b->nCouponSchedule == 0))
		return b->coupon;
	for (i=0; i < b->nCouponSchedule; i++)
		if ((b->couponSchedule[i].startDate <= date) &&
			(b->couponSchedule[i].endDate >= date))
			return b->couponSchedule[i].coupon;

	/* default */
	return b->couponSchedule[b->nCouponSchedule-1].coupon;
}


/*   blAllocateCouponSchedule

	given the number of steps in the coupon schedule, allocate
	enough space in the bond structure to hold the information.

	returns true (1) if success; false (0) if it failed.
*/

int blAllocateCouponSchedule(blBond *b, int nSteps)
{
	b->couponSchedule = (blCouponSchedule *)calloc(nSteps, sizeof(blCouponSchedule));
	if (!b->couponSchedule)  {
		fprintf(stderr, "WARNING:  could not allocate space for coupon schedule.\n");
		return 0;
	}

	b->nCouponSchedule = nSteps;
	return 1;
}


/*   blAllocateCallSchedule

	given the number of steps in the call schedule, allocate
	enough space in the bond structure to hold the information.

	returns true (1) if success; false (0) if it failed.
*/

int blAllocateCallSchedule(blBond *b, int nSteps)
{
	b->callSchedule = (blPutCallSchedule *)calloc(nSteps, sizeof(blPutCallSchedule));
	if (!b->callSchedule)  {
		fprintf(stderr, "WARNING:  could not allocate space for call schedule.\n");
		return 0;
	}

	b->nCallSchedule = nSteps;
	return 1;
}



/*   blAllocatePutSchedule

	given the number of steps in the put schedule, allocate
	enough space in the bond structure to hold the information.

	returns true (1) if success; false (0) if it failed.
*/

int blAllocatePutSchedule(blBond *b, int nSteps)
{
	b->putSchedule = (blPutCallSchedule *)calloc(nSteps, sizeof(blPutCallSchedule));
	if (!b->putSchedule)  {
		fprintf(stderr, "WARNING:  could not allocate space for put schedule.\n");
		return 0;
	}

	b->nPutSchedule = nSteps;
	return 1;
}




/*    blCopyBond:

  given a bond structure, allocate space for a new bond and copy
  all of the information from the old bond to the new bond.  */

blBond *blCopyBond(blBond *bOld)
{
	blBond *bNew;
	int i;

	/*  allocate space for a new bond  */
	bNew = blAllocateBondStruct(bOld->nCF);
	if (!bNew)  {
		fprintf(stderr, "blCopyBond:  cannot allocate space for bond structure.\n");
		return NULL;
	}

	/*  basic information  */
	strcpy_s(bNew->bondCode, bOld->bondCode);
	bNew->maturityDate = bOld->maturityDate;
	bNew->settleDate = bOld->settleDate;
	bNew->issueDate = bOld->issueDate;
	bNew->coupon = bOld->coupon;
	bNew->nCPerYear = bOld->nCPerYear;

	/*  copy coupon schedule  */
	bNew->nCouponSchedule = bOld->nCouponSchedule;
	if (bNew->nCouponSchedule)  {
		if (!blAllocateCouponSchedule(bNew, bNew->nCouponSchedule))  {
			fprintf(stderr, "blCopyBond:  cannot allocate coupon schedule.\n");
			blFreeBondStruct(bNew);
			return NULL;
		}
		for (i=0; i < bNew->nCouponSchedule; i++)  {
			bNew->couponSchedule[i].startDate = bOld->couponSchedule[i].startDate;
			bNew->couponSchedule[i].endDate = bOld->couponSchedule[i].endDate;
			bNew->couponSchedule[i].coupon = bOld->couponSchedule[i].coupon;
		}
	}

	/*  copy put schedule  */
	bNew->nPutSchedule = bOld->nPutSchedule;
	if (bNew->nPutSchedule)  {
		if (!blAllocatePutSchedule(bNew, bNew->nPutSchedule))  {
			fprintf(stderr, "blCopyBond:  could not allocate put schedule.\n");
			blFreeBondStruct(bNew);
			return NULL;
		}
		for (i=0; i < bNew->nPutSchedule; i++)  {
			bNew->putSchedule[i].date = bOld->putSchedule[i].date;
			bNew->putSchedule[i].value = bOld->putSchedule[i].value;
		}
	}

	/*  copy call schedule  */
	bNew->nCallSchedule = bOld->nCallSchedule;
	if (bNew->nCallSchedule)  {
		if (!blAllocateCallSchedule(bNew, bNew->nCallSchedule))  {
			fprintf(stderr, "blCopyBond:  could not allocate call schedule.\n");
			blFreeBondStruct(bNew);
			return NULL;
		}
		for (i=0; i < bNew->nCallSchedule; i++)  {
			bNew->callSchedule[i].date = bOld->callSchedule[i].date;
			bNew->callSchedule[i].value = bOld->callSchedule[i].value;
		}
	}

	/*  cashflow information  */
	for (i=0; i <= bNew->nCF; i++)  {
		bNew->CF[i] = bOld->CF[i];
		bNew->CFdate[i] = bOld->CFdate[i];
		bNew->tCF[i] = bOld->tCF[i];
	}

	/*  miscellaneous  */
	bNew->monthEndConv = bOld->monthEndConv;
	bNew->AI = bOld->AI;
	bNew->monthBasis = bOld->monthBasis;
	bNew->yearBasis = bOld->yearBasis;
	bNew->nDaysSettleToNextCoupon = bOld->nDaysSettleToNextCoupon;
	bNew->nDaysSettlementPeriod = bOld->nDaysSettlementPeriod;
	bNew->nDaysStartToSettle = bOld->nDaysStartToSettle;

	return bNew;
}



/*   blBondToCall

  given a fully populated bond structure, update the nCF and CF[nCF]
  elements such that the bond looks as if it matures at the given
  call   */

void blBondToCall(blBond *b, int nCall)
{
	int i;

	if (!b || !b->callSchedule || (nCall > b->nCallSchedule))
		return;

	i = 1;
	while ((i <= b->nCF) && (b->CFdate[i] != b->callSchedule[nCall].date))
		i++;

	if (i < b->nCF)  {
		b->nCF = i;
		b->CF[i] += b->callSchedule[nCall].value;  /* call amount */
		b->maturityDate = b->callSchedule[nCall].date;
	}

	return;
}


/*   blBondToPut

  given a fully populated bond structure, update the nCF and CF[nCF]
  elements such that the bond looks as if it matures at the given
  put   */

void blBondToPut(blBond *b, int nPut)
{
	int i;

	if (!b || !b->putSchedule || (nPut > b->nPutSchedule))
		return;

	i = 1;
	while ((i <= b->nCF) && (b->CFdate[i] != b->putSchedule[nPut].date))
		i++;

	if (i < b->nCF)  {
		b->nCF = i;
		b->CF[i] += b->putSchedule[nPut].value;  /* put amount */
		b->maturityDate = b->putSchedule[nPut].date;
	}

	return;
}


/*   blAllocateLong1dArray

  given the size of the array, allocate a 1-d array of longs  */

long *blAllocateLong1dArray(int s)
{
	long *a = NULL;

	if (s)  {  /* make sure s isn't zero */
		a = (long *)calloc(s, sizeof(long));
	}

	if (!a)  {
		fprintf(stderr, "blAllocateLong1dArray:  could not allocate.\n");
	}

	return a;
}


/*   blFreeLong1dArray

  given a pointer to a 1D array of longs, free the array, if the pointer
  isn't NULL  */

void blFreeLong1dArray(long *a)
{
	if (a)  {
		free(a);
	}
}


/*   blReallocLong1dArray

  given a pointer to a 1D array of longs, reallocate it to hold the
  number of longs passed as a parameter.  */

long *blReallocLong1dArray(long *aOld, int n)
{
	long *aNew;

	if (!n)  {  /* n=0, free the memory */
		blFreeLong1dArray(aOld);
		return NULL;
	}

	aNew = (long *)realloc(aOld, n * sizeof(long));
	if (!aNew)  {
		fprintf(stderr, "blReallocLong1dArray:  could not reallocate.\n");
		blFreeLong1dArray(aOld);
	}

	return aNew;
}



/*   blCatchBadIntData

	if the passed-in value is zero, return the default value  */

int blCatchBadIntData(int checkValue, int defaultValue)
{
	if (!checkValue)
		return defaultValue;
	return checkValue;
}

