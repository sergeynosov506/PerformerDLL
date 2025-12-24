/*
 * SUB-SYSTEM: calcperf
 *
 * FILENAME: PerfAggregationMerge.h
 *
 * DESCRIPTION: Header file for PerfAggregateMerge table and other related
 * structure
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR: Shobhit Barman
 *
 */
#ifndef NT
#define NT 1
#endif

#include <windows.h>
struct ERRSTRUCT;

typedef struct {
  int iMergeFromID;
  char sMergeFromType[1 + NT];
  int iMergeToID;
  long lBeginDate;
  long lEndDate;
  char sDescription[80 + NT];
} PERFAGGREGATIONMERGE;

typedef struct {
  char sMergeFromSecNo[12 + NT];
  char sMergeToSecNo[12 + NT];
  long lBeginDate;
  long lEndDate;
  int iFromSecNoIndex;
  int iToSecNoIndex;
} PERFASSETMERGE;

typedef struct {
  int iCapacity;
  int iCount;
  PERFASSETMERGE *pzMergedAsset;
} PERFASSETMERGETABLE;

typedef void(CALLBACK *LPPRPERFASSETMERGE)(PERFASSETMERGE *, ERRSTRUCT *);