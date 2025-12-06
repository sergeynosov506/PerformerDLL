/*H*
* 
* FILENAME: syssettings.h
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
// 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
// 2010-09-14 VI#44798 Added flag to exclude zero-RAT/MATs' market values in average calculation in holdtot - mk.
// 2010-06-10 VI#44127 Added flag to include zero-YTMs' market values in average calculation in holdtot - mk.
*H*/

#ifndef SYSSETTINGS
#define SYSSETTINGS 
#ifndef NT
  #define NT 1
#endif

typedef struct
{
	char	sDateforaccrualflag[1+NT];
	long	lEarliestactiondate;
	int		iPaymentsStartDate;
	char	sSystemcurrency[4+NT];
	char	sWeightedStatisticsFlag[1+NT];
	char	sEquityRating[1+NT];
	char	sFixedRating[1+NT];
	char	sPerformanceType[1+NT];
	char	sYieldType[1+NT];
	int		iFlowWeightMethod;
} SYSSETTING;

typedef struct
{
	char	sName[80+NT];
	char	sValue[100+NT];
} SYSVALUES;

typedef struct
{
	double		fFlowThreshold;
	BOOL		bGLTaxAdj;
	BOOL		bRecalcCostEffMatYld;
	BOOL		bDailyPerf;
	BOOL		bSecurityHoldtot;
	char		sSysCountry[3+NT];
	int			iInsertBatchSize;
	int			iWeightedStatisticsExcludeYTM;
	int			iWeightedStatisticsExcludeRAT;
	BOOL		bUseCustomSecurityPrice;
	long		lCFStartDate;
	SYSSETTING	zSyssetng;
} SYSTEM_SETTINGS;

typedef void (CALLBACK* LPPRSELECTSYSSETTINGS)(SYSSETTING *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRSELECTSYSVALUES)(SYSVALUES *, ERRSTRUCT *);
#endif // if SYSSETTINGS not defined 