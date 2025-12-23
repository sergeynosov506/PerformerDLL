/*
*
* SUB-SYSTEM:
*
* FILENAME: holdtot.h
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
* $Header: /sysprog/lib/pmr/rcs/calcperf.h,v 59.1 98/10/29 11:54:05 txl prod Loc
ker: jbg $
*
*/

// 2014-03-17 VI#53709 Update to params in BuildMergeCompSegMap() - yb.

#define PERFHASH 1                /* Needed for ../include/nbcHashPerf.h file */
#define c_Composite_Partnership 7 /*constant for composite partnerships */

#ifndef HOLDTOTH
#define HOLDTOTH 1
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include "CreateHoldtot.h"
#include "HashPerf.h"
#include "commonheader.h"
#include "dtransdesc.h"
#include "mapcompmemtransex.h"

#define BIGPRIMENUMBER 2147483629
#define DUPLICATE_KEY 9729
#define CASHHOLD_KEY_NOT_FOUND -1
#define MKT_VAL_INVALID -1.7e308

/* HOLDINGS TABLE*/
typedef struct {
  int iNumHoldCreated;
  int iNumHoldings;
  HOLDINGS *pzHoldings;
} HOLDTABLE;

/*HOLDCASH TABLE*/
typedef struct {
  int iNumHoldCashCreated;
  int iNumHoldCash;
  HOLDCASH *pzHoldCash;
} HOLDCASHTABLE;

/*HOLDCASH KEY*/
typedef struct {
  char sSecNo[25];
  char sWi[1 + NT];
  char sSecXtend[2 + NT];
  char sAcctType[1 + NT];
} HOLDCASHKEY;

/* Prototypes*/
typedef void(CALLBACK *LPPRSELECTALLMEMBERSOFACOMPOSITE)(int, long, int *,
                                                         ERRSTRUCT *);
typedef void(CALLBACK *LPPRSELECTPORTMAINTABLE)(PORTMAIN *, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRACCOUNTINSERTPORTMAIN)(long, char *, char *,
                                                  ERRSTRUCT *);
typedef void(CALLBACK *LPPRGETSUMMARIZEDDATAFORCOMPOSITE)(SEGMAIN *, SUMMDATA *,
                                                          UNITVALUE *, double *,
                                                          int, long, long, BOOL,
                                                          char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRSUMMARIZETAXPERF)(int, long, long, char *,
                                             ERRSTRUCT *);
typedef void(CALLBACK *LPPRSUMMARIZEMONTHSUM)(int, long, long, long, char *,
                                              ERRSTRUCT *);
typedef void(CALLBACK *LPPRSUMMARIZEINCEPTIONSUMMDATA)(int, long, char *,
                                                       ERRSTRUCT *);
typedef void(CALLBACK *LPPRSUBTRACTINCEPTIONSUMMDATA)(int, long, long,
                                                      ERRSTRUCT *);

typedef void(CALLBACK *LPPR3LONG1INT)(long, long, long, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRCOPYSUMMARYDATA)(int, long, char *, char *,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPRINSERTMAPCOMPMEMTRANSEX)(MAPCOMPMEMTRANSEX,
                                                    ERRSTRUCT *);
typedef void(CALLBACK *LPPRDELETEMAPCOMPMEMTRANSEX)(long, int, ERRSTRUCT *);
typedef void(CALLBACK *LPPRBUILDMERGECOMPSEGMAP)(int, long, long, char *,
                                                 ERRSTRUCT *);
typedef void(CALLBACK *LPPRBUILDMERGECOMPPORT)(int, long, char *, ERRSTRUCT *);
typedef void(CALLBACK *LPPRBUILDMERGESDATA)(int, long, long, char *,
                                            ERRSTRUCT *);
typedef void(CALLBACK *LPPRUPDATEUNITVALUEMONTHLYIPV)(char *, long, long,
                                                      ERRSTRUCT *);
typedef void(CALLBACK *LPPR1CHAR1LONG)(char *, long, ERRSTRUCT *);
// typedef (CALLBACK* LPPRHOLDINGSBATCH)(HOLDINGS *, long, ERRSTRUCT*);

LPPRSECTYPE lpprSelectSectype;
LPPRSELECTTRANTYPE lpprSelectTranType;
LPPRALLHOLDINGSFORANACCOUNT lpprSelectAllHoldingsForAnAccount;
LPPRINSERTMAPCOMPMEMTRANSEX lpprInsertMapCompMemTransEx;
LPPRDELETEMAPCOMPMEMTRANSEX lpprDeleteMapCompMemTransEx;
LPPRALLHOLDCASHFORANACCOUNT lpprSelectAllHoldcashForAnAccount;
LPPRSELECTALLMEMBERSOFACOMPOSITE lpprSelectAllMembersOfAComposite;
LPPRHOLDINGS lpprInsertHoldings, lpprUpdateHoldings;
// LPPRHOLDINGSBATCH				lpprInsertHoldingsBatch;
LPPRSELECTHOLDINGS lpprSelectHoldings;
LPPR1LONG3PCHAR1BOOL lpprAccountDeleteHoldings, lpprAccountDeleteHoldcash;
LPPRHOLDCASH lpprInsertHoldcash;
LPPR1INT1LONG lpprUpdatePortmainValDate;
LPPRSELECTPORTMAINTABLE lpprSelectAdhocPortmain;
LPPRPORTMAIN lpprSelectPortmain;
LPPRACCOUNTINSERTPORTMAIN lpprAccountInsertPortmain;
LPPRGETSUMMARIZEDDATAFORCOMPOSITE lpprGetSummarizedDataForCompositeEx;
LPPRBUILDMERGECOMPSEGMAP lpprBuildMergeCompSegMap;
LPPRBUILDMERGECOMPPORT lpprBuildMergeCompport;
LPPRBUILDMERGESDATA lpprBuildMergeSData;
LPPRBUILDMERGESDATA lpprBuildMergeUV;
LPPRUPDATEUNITVALUEMONTHLYIPV lpprUpdateUnitvalueMonthlyIPV;
LPPR1CHAR1LONG lpprDeleteMergeUVGracePeriod;
LPPR1CHAR1LONG lpprDeleteMergeSessionData;
LPPR2LONG lpprDeleteUnitvalueSince;
LPPR4LONG lpprDeleteUnitvalueSinceForSegment;
LPPR4LONG lpprDeleteUnitvalueSince2;
LPPRCOPYSUMMARYDATA lpprCopySummaryData;
LPPRSUMMARIZETAXPERF lpprSummarizeTaxperf;
LPPRSUMMARIZEMONTHSUM lpprSummarizeMonthsum;
LPPRSUMMARIZEINCEPTIONSUMMDATA lpprSummarizeInceptionSummdata;
LPPRSUBTRACTINCEPTIONSUMMDATA lpprSubtractInceptionSummdata;

/* Exported Functions*/
void STDCALL WINAPI CreateComposite(int iOwnerID, long lDate,
                                    int iPortfolioType, long iHoldingsFlag,
                                    ERRSTRUCT *zErr);
void STDCALL WINAPI InitializeDllRoutines(long lAsofDate, char *sDBAlias,
                                          char *sMode, char *sType,
                                          BOOL bPrepareQueries, char *sErrFile,
                                          ERRSTRUCT *zErr);
ERRSTRUCT STDCALL WINAPI MergeCompositePortfolio(int iID, long lFromDate,
                                                 long lToDate, BOOL bDaily,
                                                 long lEarliestDate,
                                                 char *sInSessionID);

/*LocalFunctions*/
void CreateCompositeRecord_Internal(int iCompositeID, HOLDTABLE *pHoldingsTable,
                                    HOLDCASHTABLE *pHoldcashTable,
                                    char *sHoldingsTable, char *sHoldCashTable,
                                    char *sTargetPortmain, long lValDate,
                                    BOOL bInTrans, int iPortType,
                                    ERRSTRUCT *zErr);
void AddTempHoldingToTable(HOLDTABLE *pHoldingsTable, HOLDINGS zHoldingsTemp,
                           ERRSTRUCT *zErr);
void AddTempHoldcashToTable(HOLDCASHTABLE *pHoldcashTable,
                            HOLDCASH zHoldcashTemp, ERRSTRUCT *zErr);
void GetHoldingsHoldcashInTables(int iPortfolioID, HOLDTABLE *pHoldingsTable,
                                 HOLDCASHTABLE *pHoldcashTable,
                                 ERRSTRUCT *zErr);
int FindHoldCashKey(HOLDCASHTABLE *pHoldCashTable, HOLDCASHKEY zHoldCashKey);
void InitializeHoldingsTable(HOLDTABLE *pHoldingsTable);
void InitializeHoldcashTable(HOLDCASHTABLE *pHoldcashTable);
void FreeCompositeCreateLibrary();

#endif