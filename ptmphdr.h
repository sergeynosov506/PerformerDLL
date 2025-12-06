/*H*
*
* FILENAME: ptmphdr.h
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
  long lTmphdrNo;
	long lHashKey;
  char sOwner[8+NT];
  long lCreateDate;
  char sCreatedBy[8+NT];
  char sChangeable[1+NT];
  char sDescription[30+NT];
  long lChangeDate;
  char sChangedBy[8+NT];
} PTMPHDR;


typedef void (CALLBACK* LPPRSELECTALLTEMPLATEDETAILS)(PTMPHDR  *, PTMPDET *, ERRSTRUCT *);
typedef void (CALLBACK* LPPRPPTMPHDR)(PTMPHDR  *);

