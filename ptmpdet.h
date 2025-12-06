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
* AUTHOR:
*
*
*H*/

#ifndef NT
	#define NT 1
#endif

typedef struct
{
  long lTmphdrNo;
  long lSeqNo;
  char sSelectType[6+NT];
  char sComparisonRule[2+NT];
  char sBeginPoint[30+NT];
  char sEndPoint[30+NT];
  char sAndOrLogic[1+NT];
  char sIncludeExclude[1+NT];
  char sMaskRest[1+NT];
  char sMatchRest[1+NT];
  char sMaskWild[1+NT];
 	char sMatchWild[1+NT];
  char sMaskExpand[1+NT];
  char sMatchExpand[1+NT];
  char sReportDest[2+NT];
  long lStartDate;
  long lEndDate;
} PTMPDET;


typedef void (CALLBACK* LPPRPPTMPDET)(PTMPDET  *);