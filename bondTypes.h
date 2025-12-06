
/*      bondTypes.h

	definitions for bond-library structures.   */

#ifndef _h_blBONDTYPES_H
#define _h_blBONDTYPES_H

#define blCodeUST "UST"
#define blCodeMuni "MUNI"
#define blCodeDiscount "DISCOUNT"
#define blCodeZero "ZERO"
/*  structure to hold stepped coupons  */

typedef struct {
	long startDate;    /* between this date... */
	long endDate;      /* ...and this date... */
	double coupon;     /* ...this is the coupon */
} blCouponSchedule;


/*  structure to hold put/call dates and amounts  */

typedef struct  {
	long date;         /* put or call date */
	double value;      /* redemption amount */
} blPutCallSchedule;


/*  structure which holds bond information  */

typedef struct {
	/*  this part of the structure filled in by the user */
	int nCPerYear;
	char bondCode[10];

	long maturityDate;
	long settleDate;
	long issueDate;
	double coupon;

	int nCouponSchedule;  /* how many steps in the coupon */
	blCouponSchedule *couponSchedule;  /* holds the schedule information */

	int nPutSchedule;     /* how many different put dates do we have?  */
	blPutCallSchedule *putSchedule;     /* holds the put information */

	int nCallSchedule;    /* how many different call dates do we have?  */
	blPutCallSchedule *callSchedule;    /* holds the call information */
	
	int nCF;             /* number of cashflows */
	double *CF;          /* the cashflow, in dollars per $100 of bond */
	long *CFdate;        /* cashflow dates */
	int monthEndConv;    /* 1 for end-of-month convention */
	double *tCF;         /* time-to-cashflow */

	/* everything from here down is computed by the yield routines */
	double AI;

	/*  daycount stuff */
	int monthBasis;
	int yearBasis;
	int nDaysSettleToNextCoupon;
	int nDaysSettlementPeriod;
	int nDaysStartToSettle;

} blBond;


typedef struct {
	blBond *bond;
	double flatPrice;
} blSolutionBag;


#endif