// 2009-03-04 VI# 41539: added accretion functionality - mk
// 02/24/2009 VI# 41916: moved variable rate to end - mk
#ifndef PAYMENTSIO_H
#define PAYMENTSIO_H
#include "commonheader.h"


#define NUMDIRECORD     25
#define NUMPDIRRECORD   10
#define NUMPINFORECORD	10
#define NUMMATRECORD		25
#define NUMAMORT        10
#define	FORWARDDAYS			360
#define XRATEPERCISION	12


typedef struct 
{
	int			iID; /* Fields from holdings table */
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  char		sSecXtend[2+NT];
  char		sSecSymbol[12+NT];
  char		sAcctType[1+NT];
  long		lTransNo;
	int			iSecID;
  double	fUnits;
  long		lTrdDate;
  long		lStlDate;
	long		lEffDate;
	long		lEligDate;
  double	fOrigFace;
  double	fOrigYield;
  double	fTotCost;
  double	fOrigCost;
  double	fCostEffMatYld;
  double	fCurExrate;
  double	fCurIncExrate;
  long		lEffMatDate;
  double	fEffMatPrice;
  char		sOrigTransType[2+NT];
  short		iSecType;/* Fields from assets table */
  double	fTradUnit; 
  char		sCurrId[4+NT];
  char		sIncCurrId[4+NT];
	long		lMaturityDate; /* Fields from fixedinc table */
	double	fRedemptionPrice;
  char		sPayType[1+NT]; 
  char		sDefeased[1+NT];
	int			iAccrualSched;
	long		lFirstCpnDate;
	long		lLastCpnDate;
	char		sAmortFlag[1+NT];
	char		sAccretFlag[1+NT];
} AMORTSTRUCT;

typedef struct
{
	AMORTSTRUCT *pzAmort;
  int         iAmortCreated;
  int         iNumAmort;
} AMORTTABLE;

typedef struct 
{
	int			iID; /* Fields from holdings table */
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  char		sSecXtend[2+NT];
  char		sSecSymbol[12+NT];
  char		sAcctType[1+NT];
  long		lTransNo;
	int			iSecID;
  double	fUnits;
  long		lTrdDate;
  long		lStlDate;
  long		lEffDate;
  double	fTotCost;
  double	fOrigCost;
  short		iSecType;/* Fields from assets table */
  double	fTradUnit; 
  char		sCurrId[4+NT];
  char		sIncCurrId[4+NT];
	long		lMaturityDate; /* Fields from fixedinc table */
} PITIPSSTRUCT;

typedef struct
{
	PITIPSSTRUCT *pzPI;
  int         iPICreated;
  int         iNumPI;
} PITIPSTABLE;

typedef struct 
{
  char   sTableName[1+NT]; // 'D' - from holddel, 'H' - from holdings 
  int		 iID; // Fields from holdings/holddel table 
  char   sSecNo[12+NT];
  char   sWi[1+NT];
  char   sSecXtend[2+NT];
  char   sSecSymbol[12+NT];
  char   sAcctType[1+NT];
  long   lTransNo;
  double fUnits;
  long   lEffDate;
  long   lEligDate;
  long   lStlDate;
  double fOrigFace;
  double fOrigYield;
  long   lEffMatDate;
  double fEffMatPrice;
	char	 sOrigTransType[2+NT];
  long   lCreateDate;//Holdings - 0,holddel - date holddel record was created
  char   sSafekInd[1+NT];
	int		 iSecID;  // Fields from assets table 
  double fTrdUnit;
	short	 iSecType;
  char   sAutoAction[1+NT];
  char   sAutoDivint[1+NT];
  char   sCurrId[4+NT];
  char   sIncCurrId[4+NT];
  double fCurExrate;
  double fCurIncExrate;
  long   lDivintNo; // Fields from divint table 
  char   sDivType[2+NT];
  long   lExDate;
  long   lRecDate;
	long   lPayDate;
  double fDivRate;
  char   sPostStatus[1+NT];
	long	 lDivCreateDate;
  long   lModifyDate;
  long   lFwdDivintNo;
  long   lPrevDivintNo;
  char   sDeleteFlag[1+NT];
  char   sProcessFlag[1+NT]; // Fields from divtype table 
  char   sProcessType[1+NT];
  char   sShortSettle[1+NT];
  char   sInclUnits[1+NT];
  char   sSplitInd[1+NT];
	char   sTranType[2+NT]; // Fields from divhist table 
  long   lDivTransNo;
  char   sTranLocation[1+NT]; 
	char   sDTTranType[2+NT];
  BOOL   bNullDivhist; // Is divhist NULL(divhist OUTER joins other tables) 
  BOOL   bProcessLot; // Is lot eligible 
  char   sAddUpdFlag[1+NT];
  char   sTruncRnd[1+NT]; // Indicates whether to use trunc or rounded 
  double fTruncUnits; // New Units Truncated 
  double fRndUnits; // New Units Rounded 
  double fIncomeAmt;
} DILIBSTRUCT;

typedef struct
{
  DILIBSTRUCT *pzDIRec;
  int         iDIRecCreated;
  int         iNumDIRec;
} DILIBTABLE;

typedef struct
{
	char		sAcctType[1+NT];
	double	fClosePrice;
	double	fCurExrate;
	double	fCurIncExrate;
	double	fCurliability;
	char		sCurrId[4+NT];
	long		lExpDate;
	char		sIncCurrId[4+NT];
	int			iID;
	double	fOpenliability;
	double	fOrigFace;
	int			iSecID;
	char		sSecNo[12+NT];
	short		iSecType;
	char		sSecXtend[2+NT];
	double	fTrdUnit;
	long		lTransNo;
	double	fUnits;
	char		sWi[1+NT];
}FMATSTRUCT;

typedef struct
{
 int       iMatCreated;
 int       iNumMat;
 FMATSTRUCT *pzForwardMat;
}FORWARDMATSTRUCT;

typedef struct {
  int		 iID;             // field from holdings
  char   sSecNo[12+NT];
  char   sWi[1+NT];
  char   sSecXtend[2+NT];
  char   sAcctType[1+NT]; 
	long	 lTransNo;//only used by forward transactions
  double fUnits;
  double fOrigFace;
  char   sSecSymbol[12+NT];
	int		 iSecID;						// fields from assets
  short  iSecType;
  double fTrdUnit;
  char   sCurrId[4+NT];
  char   sIncCurrId[4+NT];
  double fCurExrate;
  double fCurIncExrate;
	long	 lMaturityDate;			// fields from fixedinc
	double fRedemptionPrice;
	double fTotCost;
	double fVariableRate;
} MATSTRUCT;

typedef struct {
  int       iMatCreated;
  int       iNumMat;
  MATSTRUCT *pzMaturity;
} MATTABLE;

typedef struct {
  int  iID;
  long lStartPosition;
} PINFO;

typedef struct {
  PINFO *pzPRec;
  int   iPICreated;
  int   iNumPI;
} PINFOTABLE;

#endif // !PAYMENTSIO_H
