/*H*
*
* SUB-SYSTEM: calcperf
*
* FILENAME: unitvalu.h
*
* DESCRIPTION: header file for UnitValu (obsolete) and UNITVALUE (new) table 
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Valeriy Yegorov, Effron 
*
*H*/
// 2013-01-14 VI# 51154 Added cleanup for orphaned unitvalue entries to be marked as deleted -mk
#ifndef NT
  #define NT 1
#endif

#define INVALID_VALUE	-999
#define DATE_NEVER_SET	0
#define	INVALID_DATE	1

					  
static short MonthDays[2][12] = 
	//Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
	{31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31, // non-leap year
	 31,  29,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31}; // leap year

enum T_PeriodType 
{	ptUnknown,
	ptMonthly,
	ptQuarterly,
	ptSemiAnnually,
	ptYearly,
	ptDaily,
//   ptFortnightly,
//   ptWeekly,
	ptNone,
	ptDefaultPeriodType
};

enum T_RorSource // used in UNITVALUE table
{	rsUnknown, 
	rsDataHoleEnd, 
	rsInception, 
	rsInterPeriodValuation, 
	rsMonthly, 
	rsMonthToDate, 
	rsQuarterly,
	rsTerminated
};

/* OBSOLETE - Returns now are saved in UNITVALUE only - vay 
typedef  struct 
{	byte value[10];
}
UVTYPE_SQL; // to represent Delphi's extended (sizeof(extended)=10 <> sizeof(long double)=8)

typedef double UVTYPE_C;

typedef struct
{
	int			iItemType;
	int			iID;
	int			iReturnType;
	long		lBeginDate;
	int			iPeriodType;
	char		sDataHoleExists[1+NT];
	UVTYPE_SQL	*pzUVBlob;
	ULONG		ulNumValues; // number of UV's in the blob
} UNITVALU;

typedef struct
{	int			iCapacity;
	int			iCount;
	UVTYPE_SQL	*pzUV;
} UVTABLE;
*/

typedef struct
{
	int			iPortfolioID;
	int			iID;
	int			iRorType;
	long		lUVDate;
	double		fUnitValue;
	double		fFudgeFactor;
	long		lStreamBeginDate;
	int			iRorSource;
} UNITVALUE;

/* OBSOLETE - Returns now are saved in UNITVALUE only - vay 
typedef void  (CALLBACK* LPPRSELECTUNITVALU)(UNITVALU *, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRFREEUNITVALU)(UNITVALU *);
typedef void  (CALLBACK* LPPRSAVEUNITVALU)(UNITVALU, ERRSTRUCT *);

typedef BOOL  (CALLBACK* LPFNEXTTODBL)(UVTYPE_SQL, UVTYPE_C *);
typedef void  (CALLBACK* LPFNDBLTOEXT)(UVTYPE_C, UVTYPE_SQL *);
*/


typedef void  (CALLBACK* LPPRINSERTUNITVALUE)(UNITVALUE, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSELECTUNITVALUE)(UNITVALUE *, int, long, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSELECTUNITVALUERANGE2)(UNITVALUE *, int, long, long, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRUPDATEUNITVALUE)(UNITVALUE, long, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRINSERTUNITVALUEBATCH)(UNITVALUE *, long, ERRSTRUCT *);


