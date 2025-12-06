#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define DLLAPI __declspec (dllexport)
#define NAVALUE  -999 /* NA Value for return */
#define MAXFLOWS  100 /* maximum array size for flows in DWROR structure */
#define MAXITERATIONS 50 /* maximum number of iterations for Dollar Weithed ROR calculation */
#define GFTOTAL		   1
#define GFPRNCPL	   2
#define NFTOTAL		   3
#define NFPRNCPL	   4
#define INCOME	 	   5

// RORSTRUCT - for Gross and Net of Fees Ror calculations
typedef struct {
  // inputs
  double    fBegMV; /* begin market value */
  double	fBegAccr; /* begin accrued interest + accrued dividend */
  double	fEndMV;
  double	fEndAccr;
  double    fNetFlow;
  double    fWtFlow;
  double    fFees;
  double    fWtFees;
  double    fIncome;
  double    fWtIncome;
  double	fGFTotalFFactor; // Fudge factor for base ror
  double	fGFPrncplFFactor;
  double	fNFTotalFFactor;
  double	fNFPrncplFFactor;
  double	fIncFFactor;
  BOOL      bTotalPortfolio;
  BOOL      bInceptionRor;
  // outputs
  double    fGFTotalRor;
  double	fGFPrncplRor;
  double    fNFTotalRor;
  double	fNFPrncplRor;
  double	fIncRor;
} RORSTRUCT;


// DWRORSTRUCT - for dollar weighted Ror calculations
typedef struct {
  // inputs
  double fEndMV; /* End Market value */
  double fEndAccr; /* Ending Accrued Interest + accrued dividends */
  double fBegAccr;
  double afNetFlow[MAXFLOWS];
  double afWeight[MAXFLOWS];
  int    iNumFlows;
  BOOL   bTotalPortfolio;
  // output
  double fRor; 
} DWRORSTRUCT;
  

// Function prototypes  
DLLAPI void STDCALL WINAPI CalculateGFAndNFRor(RORSTRUCT *pzRS);
DLLAPI void STDCALL WINAPI CalculateDWRor(DWRORSTRUCT *pzDWRor);
DLLAPI int STDCALL WINAPI CalculateFudgeFactor(RORSTRUCT *pzRS, int iWhichFudge);
DLLAPI int STDCALL WINAPI CalculateWeightedFlow(RORSTRUCT *pzRS, int iWhichFlow);
double TimeWeightedBaseRor(double fEMV, double fBMV, double fNFlow, double fWFlow, 
                           double fFudgeFactor, BOOL bInceptionRor);
double TimeWeightedFudgeFactor(double fEndMV, double fBegMV, double fNFlow, double fWtFlow, 
  	 			               double fRor, BOOL bInceptionRor);
double TimeWeightedWeightedFlow(double fEndMV, double fBegMV, double fNFlow, double fWtFlow, 
  	 			                double fRor, BOOL bInceptionRor);
double TimeWeightedIncomeROR(double fEndAccr, double fBegAccr, double fIncome, double fBegMV, 
						double fIncFudge);
double TimeWeightedIncomeFudgeFactor(double fEndAccr, double fBegAccr, double fIncome, 
  	 			                     double fBegMV, double fRor);
