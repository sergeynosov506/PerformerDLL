/*H*
*
*
* FILENAME: ptmpdet.h
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR:Shobhit Barman
*
*
*H*/
// 2010-06-10 VI# 42903 Added option calculate performance on selected rule - sergeyn

#ifndef NT
	#define NT 1
#endif

typedef struct PERFRULE
{
	int		iPortfolioID;
  long	lRuleNo;
  char	sCurrencyProcessingFlag[1+NT];
  char	sTotalRecordIndicator[1+NT];
  long	lTmphdrNo;
  long	lParentRuleNo;
  long	lCreateDate;
  long	lDeleteDate;
  char	sDescription[30+NT];
  char	sWeightedRecordIndicator[1+NT];
  BOOL bCalculate;

} PERFRULE;

typedef struct PERFRULETABLE
{
  int      iCapacity;
  int      iCount;
  PERFRULE *pzPRule;
	int			 *piTHDIndex;
} PERFRULETABLE;
 

typedef void			(CALLBACK* LPPRSELECTALLPERFRULE)(PERFRULE *, int, long, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRPPERFRULE)(PERFRULE * );
typedef void			(CALLBACK* LPPRPPERFRULETABLE)(PERFRULETABLE *);
