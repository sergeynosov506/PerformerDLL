/*H*
*
*
* FILENAME: perfscrdet.h
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
 
 
#ifndef NT
	#define NT 1
#endif NT
 
typedef struct
{
	long lScrhdrNo;
  long lSeqNo;
  char sSelectType[STR6LEN];
  char sComparisonRule[STR2LEN];
  char sBeginPoint[STR30LEN];
  char sEndPoint[STR30LEN];
  char sAndOrLogic[STR1LEN];
  char sIncludeExclude[STR1LEN];
  char sMaskRest[STR1LEN];
  char sMatchRest[STR1LEN];
  char sMaskWild[STR1LEN];
  char sMatchWild[STR1LEN];
  char sMaskExpand[STR1LEN];
  char sMatchExpand[STR1LEN];
  char sReportDest[STR2LEN];
  long lStartDate;
  long lEndDate;
} PSCRDET;

typedef void (CALLBACK* LPPRPERFSCRIPTDETAIL)(PSCRDET, ERRSTRUCT *);
typedef void (CALLBACK* LPPPRPSCRDET)(PSCRDET *);
