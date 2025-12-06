/*H*
* 
* FILENAME: Fixedinc.h
* 
* DESCRIPTION: 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: Shobhit Barman
*
// 2009-03-04 VI# 41539: added accretion functionality - mk
*H*/
#ifndef Fixedinc_H
#define Fixedinc_H

#ifndef NT
  #define NT 1
#endif


typedef struct
{
	char		sSecNo[12+NT];
	char		sWi[1+NT];
	long		lMaturityDate;
	double	fRedemptionPrice;
	char		sFlatCode[1+NT];
	char		sPayType[1+NT];
	long		lDatedDate;
	char		sAmortFlag[1+NT];
	char		sAccretFlag[1+NT];
	char		sOriginalFace[1+NT];
	double	fCurYld;
	double	fCurYtm;
	char		sZeroCoupon[1+NT];
	long		lIssueDate;
	long		lInflationIndexID;
	BOOL		bRecordFound;
} PARTFINC;


typedef struct
{
	int				iFincCreated;
	int				iNumFinc;
	PARTFINC	*pzFinc;
} FINCTABLE;


/*typedef struct 
{
	int			iAssetIndex;
  double	fCurYield;
  double	fCurYtm;
  double	fYldToWorst;
  double	fBondEqYld;
  double	 fCurDur;
  double	fCurModDur;
  double	fConvexity;
  double	fMaturityDate;
} PARTFIXEDINC;

	
typedef struct 
{
  int          iNumFInc;
  int          iFIncCreated;
  PARTFIXEDINC *pzFInc;
} FIXEDINCTABLE;
*/ 

typedef void	(CALLBACK* LPPRSELECTPARTFINC)(char *, char *, PARTFINC *, ERRSTRUCT *);

#endif // !Fixedinc_H