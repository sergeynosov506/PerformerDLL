/*H*
*
*
* FILENAME: perfscrhdr.h
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
*H*/
 
#ifndef NT
	#define NT 1
#endif
 
typedef struct
{
	long	lScrhdrNo;
  long	lTmphdrNo;
  long	lHashKey;
	int		iSegmentTypeID;
  char	sOwner[8+NT];
  long	lCreateDate;
  char	sCreatedBy[8+NT];
  char	sChangeable[1+NT];
  char	sDescription[30+NT];
  long	lChangeDate;
  char	sChangedBy[8+NT];
	char  sHdrKey[64+NT];
	int		iGroupID; // comes from segment table and not from pscrhdr table
} PSCRHDR;

typedef struct 
{
	PSCRHDR	zHeader;
//	int			iSegmentType; // This should be part of PSCRHDR but for the time being have it seperate
  int     iDetailCreated;
  int     iNumDetail;
  PSCRDET *pzDetail;
} PSCRHDRDET;
 
typedef struct
{
  int        iSHdrDetCreated;
  int        iNumSHdrDet;
  PSCRHDRDET *pzSHdrDet;
} PSCRHDRDETTABLE;
  

typedef void			(CALLBACK* LPPRPERFSCRIPTHEADER)(PSCRHDR, ERRSTRUCT *);
typedef ERRSTRUCT (CALLBACK* LPFNPSCRHDR)(PSCRHDR, PSCRHDRDETTABLE *, int *);
typedef long			(CALLBACK* LPFNPSCRHDRDET)(PSCRHDRDET);
typedef ERRSTRUCT (CALLBACK* LPFN2PSCRHDRDET)(PSCRHDRDET *, PSCRHDRDETTABLE *, BOOL, char*, char*, char*);
typedef void			(CALLBACK* LPPRPSCRHDRDET)(PSCRHDRDET *);
typedef void			(CALLBACK* LPPRSELECTALLSCRIPTHEADERANDDETAILS)(PSCRHDR *, PSCRDET *, ERRSTRUCT *);
typedef int				(CALLBACK* LPFNFINDSCRIPT)(PSCRHDRDETTABLE, long, long);