/*H*
*
* SUB-SYSTEM: calcperf
*
* FILENAME: rtrnset.h
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
* 2020-04-17 J# PER-10655 Added CN transactions -mk.
*H*/
 

#ifndef NT
	#define NT 1
#endif

#define NUMRORTYPE_BASIC     7 
#define NUMRORTYPE_ALL      12 // last four are tax related return type

#define ROR_SUMMARY			0
#define GTWRor					1
#define GPcplRor				2
#define NTWRor					3
#define NPcplRor				4
#define IncomeRor				5
#define GDWRor					6
#define NDWRor					7
#define GTWTaxEquivRor	8
#define NTWTaxEquivRor	9
#define GTWAfterTaxRor	10
#define NTWAfterTaxRor	11
#define CNWRor					12

#define TWRorBit				1
#define IncomeRorBit		2
#define PcplRorBit			4
#define AfterTaxRorBit	8
#define TaxEquivRorBit	16
#define TWNCFRorBit	32

typedef struct
{
	long   iPortfolioID;
	int    iID;
  long   lRtnDate;
	int		 iRorType;
  double fBaseRorIdx;
  double fPrincipalIdx;
  double fIncomeIdx;
  double fLocalRorIdx;
  double fCurrRorIdx;
  double fFedtaxRorIdx;
  double fFedfreeRorIdx;
  double fStatetaxRorIdx;
  double fStatefreeRorIdx;
  double fAtaxRorIdx;
  double fEtaxRorIdx;
} RTRNSET;


typedef void  (CALLBACK* LPPRROR)(RTRNSET, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRRTRNSET1LONG)(RTRNSET, long, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRRTRNSETPOINTER1INT)(RTRNSET *, int, ERRSTRUCT *);
//  typedef void      (CALLBACK* LPPRSELECTDAILYROR)(RTRNSET *, int, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRSELECTPERIODROR)(RTRNSET *, long, long, ERRSTRUCT *);

