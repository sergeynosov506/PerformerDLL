#include "assets.h"
#include "commonheader.h"
#include "trans.h"

typedef int(CALLBACK *LPFNRMDYJUL)(short[], long *);
typedef int(CALLBACK *LPFNRJULMDY)(long, short[]);
typedef int(CALLBACK *LPFNNEWDATE)(long, BOOL, int, int, int, long *);

HINSTANCE hStarsUtilsDll;

LPFNRMDYJUL lpfnrmdyjul;
LPFNRJULMDY lpfnrjulmdy;
LPFNNEWDATE lpfnNewDateFromCurrent;
// LPFN1DOUBLE3PCHAR1LONGINT	lpfnCalculatePrice;

/* function prototypes */
void ApplySection988(double *pzCGL, double *pzSGL, double *pzTotGL);
void STLTGainLoss(TRANS zTR, double *pzSGL, double *pzSTGL, double *pzLTGL,
                  long lSecImpact, char *sPrimaryType, char *sSecondaryType);
BOOL InitializeCalcGainLossLibrary();
void FreeCalcGainLossLibrary();
