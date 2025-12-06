/*H*
*
* FILENAME: dailyflows.h
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
*
* 2020-04-17 J# PER-10655 Added CN transactions -mk.
*H*/
 
#ifndef NT
	#define NT 1
#endif
 
typedef struct
{
	int			iSegmainID;
  long		lPerformDate;
  double	fNetFlow;
  double	fFees;    
	double	fFeesOut;
  double	fCreateDate;
  double    fCNFees;
} DAILYFLOWS;

typedef void  (CALLBACK* LPPRDAILYFLOWS)(DAILYFLOWS, ERRSTRUCT *);
