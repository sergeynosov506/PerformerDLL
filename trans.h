/*H*
* 
* FILENAME: trans.h
* 
* DESCRIPTION: 
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: Shobhit Barman
*
*H*/
// History
// 2007-11-08 Updated Callback() - yb
#ifndef TRANS_H
#define TRANS_H
#include "commonheader.h"
#include "assets.h"

#ifndef NT
  #define NT 1
#endif

typedef struct
{
  int			iID;
  long		lTransNo;
  char		sTranType[STR3LEN];
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  char		sSecXtend[2+NT];
  char		sAcctType[1+NT];
	int			iSecID;
  char		sSecSymbol[12+NT];
  double	fUnits;
  double	fOrigFace;
  double	fUnitCost;
  double	fTotCost;
  double	fOrigCost;
  double	fPcplAmt;
  double	fOptPrem;
  double	fAmortVal;
  double	fBasisAdj;
  double	fCommGcr;
  double	fNetComm;
  char		sCommCode[STR5LEN];
  double	fSecFees;
  double	fMiscFee1;
  char		sFeeCode1[STR5LEN];
  double	fMiscFee2;
  char		sFeeCode2[STR5LEN];
  double	fAccrInt;
  double	fIncomeAmt;
  double	fNetFlow;
  char		sBrokerCode[6+NT];
  char		sBrokerCode2[6+NT];
  long		lTrdDate;
  long		lStlDate;
  long		lEffDate;
  long		lEntryDate;
  long		lTaxlotNo;
  long		lXrefTransNo;
  long		lPendDivNo;
  long		lRevTransNo;
  char		sRevType[2+NT];
  long		lNewTransNo;
  long		lOrigTransNo;
  long		lBlockTransNo;
  int		iXID;
  long		lXTransNo;
  char		sXSecNo[12+NT];
  char		sXWi[1+NT];
  char		sXSecXtend[2+NT];
  char		sXAcctType[1+NT];
	int			iXSecID;
  char		sCurrId[4+NT];
  char		sCurrAcctType[1+NT];
  char		sIncCurrId[4+NT];
  char		sIncAcctType[1+NT];
  char		sXCurrId[4+NT];
  char		sXCurrAcctType[1+NT];
  char		sSecCurrId[4+NT];
  char		sAccrCurrId[4+NT];
  double	fBaseXrate;
  double	fIncBaseXrate;
  double	fSecBaseXrate;
  double	fAccrBaseXrate;
  double	fSysXrate;
  double	fIncSysXrate;
  double	fBaseOpenXrate;
  double	fSysOpenXrate;
  long		lOpenTrdDate;
  long		lOpenStlDate;
  double	fOpenUnitCost;
  double	fOrigYld;
  long		lEffMatDate;
  double	fEffMatPrice;
  char		sAcctMthd[1+NT];
  char		sTransSrce[STR8LEN];
  char		sAdpTag[5+NT];
  char		sDivType[2+NT];
  double	fDivFactor;
  long		lDivintNo;
  long		lRollDate;
  long		lPerfDate;
  char		sMiscDescInd[1+NT];
  char		sDrCr[2+NT];
  char		sBalToAdjust[4+NT];
  char		sCapTrans[1+NT];
  char		sSafekInd[1+NT];
  char		sDtcInclusion[1+NT];
  char		sDtcResolve[2+NT];
  char		sReconFlag[1+NT];
  char		sReconSrce[3+NT];
  char		sIncomeFlag[2+NT];
  char		sLetterFlag[1+NT];
  char		sLedgerFlag[1+NT];
  char		sGlFlag[1+NT];
  char		sCreatedBy[8+NT];
  long		lCreateDate;
  char		sCreateTime[8+NT];
  long		lPostDate;
  char		sBkofFrmt[2+NT];
  long		lBkofSeqNo;
  long		lDtransNo;
  double	fPrice;
  int			iRestrictionCode;
} TRANS;


typedef struct 
{
	int			iID;
  long		lTransNo;
  char		sTranType[2+NT];
  char		sSecNo[12+NT];
  char		sWi[1+NT];
  char		sSecXtend[2+NT];
  char		sAcctType[1+NT];
  double	fPcplAmt;
  double	fAccrInt;
  double	fIncomeAmt;
  double	fNetFlow;
  long		lTrdDate;
  long		lStlDate;
  long		lEntryDate;
  char		sRevType[2+NT];
  long		lPerfDate;
  char		sXSecNo[12+NT];
  char		sXWi[1+NT];
  char		sXSecXtend[2+NT];
  char		sXAcctType[1+NT];
  char		sCurrId[4+NT];
  char		sCurrAcctType[1+NT];
  char		sIncCurrId[4+NT];
  char		sXCurrId[4+NT];
  char		sXCurrAcctType[1+NT];
  double	fBaseXrate;
  double	fIncBaseXrate;
  double	fSecBaseXrate;
  double	fAccrBaseXrate;
  char		sDrCr[2+NT];
	long		lTaxlotNo;	
	long		lDivintNo;
  BOOL		bReversal;
  int			iSecAssetIndex;
  int			iCashAssetIndex;
  int			iIncAssetIndex;
  int			iXSecAssetIndex; // XSec and XCash will be filled only for Xfer 
  int			iXCashAssetIndex; // trades(TS and TC) 
  int			iTranTypeIndex;
	
	double	fTotCost;
	double	fOptPrem;
	double	fBaseOpenXrate;
	long		lEffDate;
	long		lOpenTrdDate;
	char		sGLFlag[1+NT];
	long		lRevTransNo;
	double	fUnits;
	long		lPerfDateAdj;

} PARTTRANS;

typedef struct
{
  int			iID;
  long		lTransNo;
	long		lPayeeID;
  char		sDsc[255+NT];
} PAYTRAN;


typedef struct 
{
	int       iNumTrans;
  int       iTransCreated;
  PARTTRANS *pzTrans;
} TRANSTABLE;


typedef struct 
{
  int		iSize;  // # of pzTrans elements created using malloc 
	int		iCount; // # of pzTrans elements actually used (added to the table)
  TRANS	*pzTrans;
} TRANSTABLE2;

 
typedef void			(CALLBACK* LPPRINITTRANS)(TRANS *);
typedef void			(CALLBACK* LPPRTRANS)(TRANS, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRTRANSPOINTER)(TRANS *, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRSELECTTRANS)(int, long, TRANS *, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRSELECTPERFORMANCETRANSACTION)(PARTTRANS *, PARTASSET2 *, LEVELINFO *, int, long, long, int, ERRSTRUCT *);
typedef void			(CALLBACK* LPPR3LONG2PCHAR1BOOL1PTRANS)(long, long, long, char *, char *, BOOL, TRANS *, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRINITTRANSTABLE2)(TRANSTABLE2 *);
typedef ERRSTRUCT	(CALLBACK* LPFNADDTRANSTOTRANSTABLE2)(TRANSTABLE2 *, TRANS);
typedef void			(CALLBACK* LPPRDTRANSSELECT)(TRANS *, int, char *, char *, long, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRSELECTPAYTRAN)(PAYTRAN *, int, long, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRINSERTPAYTRAN)(PAYTRAN, ERRSTRUCT *);
typedef ERRSTRUCT	(CALLBACK* LPFNCALCULATEINFLATIONRATE)(int, char *, char *, long, long, long, long, BOOL, BOOL, double *);

// gain-loss function
typedef int		(CALLBACK* LPFNCALCGAINLOSS)(TRANS, char *, long, char *, char *, char *, double *, double *, 
																					 double *, double *,	double *, double *);

// calc flow function	
typedef ERRSTRUCT	(CALLBACK* LPFNCALCFLOW)(TRANS, char *, FLOWCALCSTRUCT *, bool);


#endif // !TRANS_H