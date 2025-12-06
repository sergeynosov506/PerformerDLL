/*H*
* 
* FILENAME: mapcompmemtransex.h
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

typedef struct
{
  long	lCompDate;
  int		iCompID;
  long	lCompTrans;
  int		iCompMem;
  long	lCompMemTrans;
} MAPCOMPMEMTRANSEX;

typedef struct
{
	char		sSessionID[GUID_STR_SIZE+NT];
	long		lOwner_ID;
	long		lID;
	long		lMemberPortID;
	long		lMemberSegID;
	long		lSegmentType_ID;
	long		lParentRuleID;
	long		lMemberSegType;
	long		lLevelNumber;
	long		lCatValue;
	double	fTaxRate;
	char		sName[60+NT];
} MERGE_COMPSEGMAP;
