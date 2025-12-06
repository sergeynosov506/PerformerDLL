/*
*
* SUB-SYSTEM:
*
* FILENAME: CreateHoldtot.h
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
*/

// History
// 2010-09-16 VI#44799 Added capability to calculate average final bond maturity - mk.
// 2008-02-14 Increased number of ratings    - mk
// 2007-11-08 Added Vendor id    - yb

#include "commonheader.h"
#include "calcperf.h"
#include "holdtot.h"

#define EXTRAPKEY     10 // number of keys(in perfkey table) created at a time
#define EXTRAASSET    25 // number of asset table records created at a time 
#define EXTRAHOLD     25 // number of holding table records created at a time 
//#define EXTRASHDRDET  100 // script header detail records 
#define	EXTRASEGMAIN	25 // number of segmain created at a time
#define LONGSH_LONG    1
#define LONGSH_SHORT   2
	 
#define NUMRATINGS						 200	 // maximum number of ratings records
#define INTERNATIONALLEVELID	  20 //International Level id
#define UNSUPERVISED_SECXTEND "UP"
#define UNSUPERVISEDLEVELID			 9
#define UNSUPERVISEDSQNNBR		 999
#define ALLOCATEDSEGMENTS				25
#define ALLOCATEDSEGMAP					25
#define	TOTALSEGMENT						 1
#define STATISTICSFLAG_1			 '0'
#define STATISTICSFLAG_2			 '1'

#define SECURITY_SEGMENT	  400
	
   
typedef struct 
{
  int       iNumHolding;
  int       iHoldingCreated;
  HOLDINGS	*pzHold;
} HOLDINGSTABLE;


typedef struct
{
  int							iNumHoldingsAssets;
	int							iHoldingsAssetsCreated;
	HOLDINGSASSETS	*pzHoldingsAssets;
} HOLDINGSASSETSTABLE;

typedef struct
{
	int     iNumRating;
	int     iRatingCreated;
	RATING	*pzRating;
}	RATINGTABLE;

typedef struct
{
	double fNativeMktVal;
	double fBaseMktVal;
	double fSysMktVal;
	double fMktValForYTM;
	double fMktValForCurYield;
	double fMktValForCoupon;
	double fMktValForDuration;
	double fMktValForRatings;
	double fMktValForMaturity;
	double fMktValForMaturityReal;
	
	// mkt values for weighting equity stats
	double fMktValForEquity;
	double fMktValForPctEPS1Yr;
	double fMktValForPctEPS5Yr;
	double fMktValForBeta;
	double fMktValForROE;
	double fMktValForPriceBook;
	double fMktValForPriceSales;
	double fMktValForTrail12mPE;
	double fMktValForProj12mPE;
	double fMktValForAvgWtdCap;
	double fMktValForDivYield;

	BOOL	 bIncludeMedianCap;

} MKTVALUES;
 

typedef ERRSTRUCT (CALLBACK* LPPRTESTASSET)(PARTASSET2, PSCRDET *, int, BOOL, long, PARTPMAIN, BOOL *, long, long);

typedef void (CALLBACK* LPPRSELECTALLFORHOLDTOT)(int, long, HOLDINGSASSETS *, int, ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTALLRATING)(RATING *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTUNSUPERVISED)(int, char *, double *, double *,ERRSTRUCT *);
typedef int	 (CALLBACK* LPFNSELECTUNSUPERVISEDSEGMENTID)(int, int, ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTALLSEGMENTS)(SEGMENTS *,ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTALLSEGMAP)(SEGMAP *, ERRSTRUCT *);  
typedef void (CALLBACK* LPPRSELECTSYSSETTINGS)(SYSSETTING *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTSEGMAINFORPORTFOLIO)(SEGMAIN *, int, ERRSTRUCT *);

//***** GLOBAL VARIABLES *****
RATINGTABLE			zRatingTable;
SEGMENTSTABLE		zSegmentsTable;
SEGMAPTABLE			zSegmapTable;
PERFRULETABLE		zSystemRules;

//LPFNPRINTERROR											lpfnPrintError;
LPFN1LONG3PCHAR1BOOL1PCHAR					lpprInitPerformance;
LPPRTESTASSET												lpprTestAsset;
LPPRASSETTABLE											lpprInitializeAssetTable;
LPFNADDASSET												lpfnAddAssetToTable;
LPFNFINDASSET												lpfnFindAssetInTable;
//LPPRERRSTRUCT											  lpprInitializeErrStruct;
//LPFNPSCRHDRDET											lpfnCreateHashkeyForScript;
LPFNCREATEPERFSCRIPT								lpfnCreateVirtualPerfScript;
LPFNFINDCREATESCRIPTHEADER					lpfnFindCreateScriptHeader;
//LPPRSELECTALLSCRIPTHEADERANDDETAILS	lpprSelectAllScriptHeaderAndDetails;
LPPRSELECTALLFORHOLDTOT							lpprSelectAllForHoldTot;
LPPR1LONG7PCHAR											lpprSelectHoldmap;
LPPRHOLDTOT													lpprInsertHoldtot;
LPPR1LONG														lpprDeleteHoldtotForAnAccount;
LPPRPPERFRULE												lpprInitPerfrule;
LPPRPPERFRULETABLE									lpprInitPerfruleTable;
LPFNADDPERFRULE											lpfnAddPerfruleToTable;
LPFNSELECTUNSUPERVISEDSEGMENTID			lpfnSelectPledgedSegment;
LPPRSELECTALLRATING									lpprSelectAllRatings;   
LPFN2PSCRHDRDET											lpfnCreateNewScript;
LPPRPSCRHDRDET											lpfnInitializePScrHdrDet;
//LPFNFINDSCRIPT											lpfnFindScrHdrByHashkeyAndTmpNo;
LPFNPSCRHDR													lpfnAddScriptHeaderToTable;
LPFNADDSCRDETAIL										lpfnAddDetailToScrHdrDet;
LPPRSELECTSEGMAINFORPORTFOLIO				lpprSelectSegmainForPortfolio;
LPFNPPARTSECTYPETABLE								lpfnFillPartSectypeTable;
LPPRSELECTALLSEGMAP									lpprSelectAllSegmap;
LPFN1INT2PCHAR1INT									lpfnAddNewSegment;
LPPRSELECTALLSEGMENTS								lpprSelectAllSegments;
//LPPR2PLONG													lpprSelectStarsDate;
LPPRSELECTSYSSETTINGS								lpprSelectSysSettings;
LPPRPPTMPHDR												lpprInitializePerfTmpHdr;
LPPRPPTMPDET												lpprInitializePerfTmpDet;
LPFNADDTEMPLATEHEADER								lpfnAddTemplateHeaderToTable;
LPFNADDTEMPLATEDETAIL								lpfnAddTemplateDetailToTable;
LPPR1LONG1PLONG											lpprRegTransCount;
LPPRSELECTUNSUPERVISED							lpprSelectUnsupervised;
LPFNSELECTUNSUPERVISEDSEGMENTID			lpfnSelectUnsupervisedSegmentId;
LPPRPARTASSET2											lpprInitializePartAsset;
LPFNPARTASSET2											lpfnAllocateMemoryToDailyAssetInfo;
//LPPRVOID														lpprHeapDump;

//Function prototypes
DLLAPI ERRSTRUCT STDCALL WINAPI CreateHoldtot(int iPortfolioID, long lDate);
ERRSTRUCT	CreateHoldtotRecord(PARTPMAIN zPartPortMain, long lDate, HOLDINGSASSETSTABLE *zHoldingsAssetsTable, SEGMAINTABLE* pzSegmainTable);
ERRSTRUCT GetHoldingsAndAssetTables(int iPortfolioID, int iVendorID, long lDate, HOLDINGSASSETSTABLE *pHoldingsAssetsTable,
																		ASSETTABLE2 *pzATable);
void			AddHoldingsToHoldtot(HOLDINGSASSETS zHoldingsAssets, HOLDTOT *pzHoldtot, long lDate, MKTVALUES *pzSegmentMV);
void			GetPAssetsFromHoldingsAssets(PARTASSET2 *pzPartAsset, LEVELINFO *pzLevels, HOLDINGSASSETS zHoldingsAssets, long lDate);
double		GetNumericalRating(char * sRating, char sPrimaryType);
void			InitializePartialAsset(PARTASSET *pzPAsset);
void			InitializeHoldingsAssetsTable(HOLDINGSASSETSTABLE *pzHSTable);
void			InitializePerfScrHdr(PSCRHDR *pzSHeader);
void			InitializePerfScrDet(PSCRDET *pzSDetail);
ERRSTRUCT AddHoldingsAssetsToTable(HOLDINGSASSETSTABLE *pHoldingsAssetsTable, HOLDINGSASSETS zHoldingsAssets);
ERRSTRUCT LoadRatingTable(RATINGTABLE *pzRatingTable);
void			FillAdditionalHoldTotData(HOLDTOT *pzHoldtot, MKTVALUES zMV, MKTVALUES zTotalMV);
void			InitializeSegmentsTable(SEGMENTSTABLE *pzSegmentsTable);
ERRSTRUCT AddSegmentToTable(SEGMENTSTABLE  *pzSegmentsTable, SEGMENTS zSegments);
void			InitializeSegmapTable(SEGMAPTABLE *pzSegmapTable);
ERRSTRUCT AddSegmapToTable(SEGMAPTABLE *pzSegmapTable, SEGMAP zSegmap);
ERRSTRUCT LoadSegmapTable(SEGMAPTABLE *pzSegmapTable);
ERRSTRUCT LoadSegmnentsTable(SEGMENTSTABLE *pzSegments);
int				GetSegmentLevelId(const int iId);
int				GetSegmapid(const int iIndustLevel1,const int iIndustLevel2,const int iIndustLevel3);
int				GetPledgedSegmentId(const int iIndustLevel1,const int iIndustLevel3);
int				GetInternational(const int SegmentId);
BOOL			IsAssetInternational(PARTPMAIN zPmain, char sCurrId[], const int IndustLevel1, int* Level2);
int				GetUnsupervisedSegmentId(void);
void			InitializeSegmainTable(SEGMAINTABLE* zSegmainTable);
ERRSTRUCT GetAllSegmentsFromSegmainForPortId(int iPortfolioID,SEGMAINTABLE *pzSegmainTable);
int				CheckIfSegmentExist(int iSegTypeId,SEGMAINTABLE* pzSegmainTable);
ERRSTRUCT AddSegmainToTable(SEGMAINTABLE *pzSegmainTable, SEGMAIN zSegmain);
ERRSTRUCT CreateSegmainsIfRequired(SEGMAINTABLE *pzSTable, PARTPMAIN zPmain, long lCurrentDate,
																   ASSETTABLE2 zATable, PERFRULETABLE zRuleTable);
int				FindSegmentByType(SEGMAINTABLE zSTable, int iSegmentTypeID);
ERRSTRUCT GetSystemTables();
ERRSTRUCT GetAllPerfrulesForAPortfolio(PERFRULETABLE *pzRuleTable, int iID, long lDate);
void			CleanUpPortfolioTables(HOLDINGSASSETSTABLE *pzHATable, SEGMAINTABLE	*pzSTable,
														PERFRULETABLE *pzPTable, ASSETTABLE2 *pzATable);
void			CleanUpSystemTables();
DLLAPI ERRSTRUCT STDCALL WINAPI InitHoldtot(long lAsofDate, char *sDBAlias, char *sErrFile);

/*ERRSTRUCT	AddDetailToScrHdrDet(PSCRDET zSDetail, PSCRHDRDET *pzScrHdrDet);
ERRSTRUCT	AddHoldingsAssetsToTable(HOLDINGSASSETSTABLE *pHoldingsAssetsTable, HOLDINGSASSETS zHoldingsAssets);
void			AddHoldingsToHoldtot(HOLDINGSASSETS zHoldingsAssets, HOLDTOT *pzHoldtot, long lDate, MKTVALUES *pzSegmentTotals);
ERRSTRUCT	AddScriptHeaderToTable(PSCRHDR zPSHeader, int *piArrayIndex);
ERRSTRUCT	CreateHoldtotRecord(PARTPMAIN zPartPortMain, long lDate, HOLDINGSASSETSTABLE *pHoldingsAssetsTable,SEGMAINTABLE *pzSegmainTable);
ERRSTRUCT LoadRatingTable(RATINGTABLE *pzRatingTable);
double		GetNumericalRating(char * sRating, char sPrimaryType);
ERRSTRUCT GetHoldingsAndAssetTables(int iPortfolioID, long lDate, HOLDINGSASSETSTABLE *pHoldingsAssetsTable, ASSETTABLE *pzATable);
void			GetPAssetsFromHoldingsAssets(PARTASSET *pzPartAsset, HOLDINGSASSETS zHoldingsAssets);
void      InitializePerfScrHdr(PSCRHDR *pzHeader);
void      InitializePerfScrDet(PSCRDET *pzDetail);
void			InitializePartialAsset(PARTASSET *pzPAsset);
void			InitializeHoldingsAssetsTable(HOLDINGSASSETSTABLE *pzHSTable);
void			FillAdditionalHoldTotData(HOLDTOT *pzHoldtot, MKTVALUES zMV, MKTVALUES zTotMV);
ERRSTRUCT LoadSegmnentsTable(SEGMENTSTABLE *pzSegments);
void			InitializeSegmentsTable(SEGMENTSTABLE *pzSegmentsTable);
int				GetSegmentLevelId(const int iId);
int				GetPledgedSegmentId(const int iIndustLevel1,const int iIndustLevel3);
ERRSTRUCT LoadSegmapTable(SEGMAPTABLE *pzSegmapTable);
void			InitializeSegmapTable(SEGMAPTABLE *pzSegmapTable);
int				GetSegmapid(const int iIndustLevel1,const int iIndustLevel2,const int iIndustLevel3);
BOOL			IsAssetInternational(PARTPMAIN zPmain,char sCurrId[],const int IndustLevel1,int* Level2);
int				GetUnsupervisedSegmentId(void);
ERRSTRUCT	GetAllSegmentsFromSegmainForPortId(int iPortfolioID,SEGMAINTABLE *pzSegmainTable);
int				CheckIfSegmentExist(int iSegTypeId,SEGMAINTABLE* pzSegmainTable);
void			InitializeSegmainTable(SEGMAINTABLE *pzSegmainTable);
ERRSTRUCT AddSegmainToTable(SEGMAINTABLE *pzSegmainTable, SEGMAIN zSegmain);
ERRSTRUCT CreateSegmainsIfRequired(SEGMAINTABLE *pzSTable, PARTPMAIN zPmain, long lCurrentDate,
																   ASSETTABLE zATable, PERFRULETABLE zRuleTable);
int				FindSegmentByType(SEGMAINTABLE zSTable, int iSegmentTypeID);
void			CleanUpPortfolioTables(HOLDINGSASSETSTABLE *pzHATable, SEGMAINTABLE	*pzSTable, PERFRULETABLE *pzPTable, ASSETTABLE *pzATable);
ERRSTRUCT GetAllPerfrulesForAPortfolio(PERFRULETABLE *pzRuleTable, int iID, long lDate);
ERRSTRUCT GetSystemTables();
void			CopyHoldingsAssetsToAsset(HOLDINGSASSETS zHoldingsAssets, PARTASSET *pzTempAsset);
void			CleanUpSystemTables();
DLLAPI ERRSTRUCT STDCALL WINAPI InitHoldtot(long lAsofDate, char *sDBAlias, char *sErrFile);
*/