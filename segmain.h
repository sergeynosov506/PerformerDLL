/*H*
* 
* FILENAME: segmain.h
* 
* DESCRIPTION: Header file for Performer's SEGMAIN table
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: Valeriy Yegorov
*
*H*/
#ifndef SEGMAIN_H
#define SEGMAIN_H

#include "commonheader.h"

#ifndef NT
  #define NT 1
#endif

typedef struct
{
	int		iID;
	int		iOwnerID;
	int		iSegmentTypeID;
	char	sSegmentName[STR40LEN];
	char	sSegmentAbbrev[STR20LEN];
	char	sIsInactive[STR1LEN];
	short	iSegLevel; // must be of the same type like in CreateHoldtot.h.SEGMAIN
	char	sCalculated[STR1LEN];
	short	iSequenceNo; // must be of the same type like in CreateHoldtot.h.SEGMAIN
}	SEGMAIN;

typedef struct
{
	int			iCapacity;
	int			iCount;
	SEGMAIN *pzSegmain;
} SEGMAINTABLE;

typedef struct
{
	int		iID;
	int		iLevelID;
	char	sAbbrev[STR20LEN];
	char	sName[STR60LEN];
	char	sCode[STR12LEN];
	int		iSequenceNbr;
	int		iGroupID;
	int		iInternational;
} SEGMENTS;

typedef struct
{
	int				iCapacity;
	int				iCount;
  SEGMENTS	*pzSegments;
}	SEGMENTSTABLE;

typedef struct 
{
 	int iSegmentID ;
 	int iSegmentLevel1ID;
 	int iSegmentLevel2ID;
 	int iSegmentLevel3ID;
} SEGMAP;

typedef struct
{
	int			iCapacity;
	int			iCount;
	SEGMAP	*pzSegmap;
} SEGMAPTABLE;

typedef void	(CALLBACK* LPPRSELECTSEGMAIN)(int, int, long *, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRINSERTSEGMAIN)(SEGMAIN *, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSELECTSEGTREE)(int, int, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRINSERTSEGTREE)(int, int, long, ERRSTRUCT *);

typedef void  (CALLBACK* LPPRSELECTSEGMENT)(SEGMENTS *, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRINSERTSEGMENT)(SEGMENTS *, ERRSTRUCT *);
typedef int		(CALLBACK* LPFNSELECTSEGMAP)(int, int, int, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRINSERTSEGMAP)(int, int, int, int, ERRSTRUCT *);
typedef int		(CALLBACK* LPFNSELECTSEGMENTIDFROMSEGMAP)(int,int,int,ERRSTRUCT*);
typedef void  (CALLBACK* LPPRINSERTSECSEGMAP)(char *, char *, int, ERRSTRUCT *);

#endif // SEGMAIN_H
