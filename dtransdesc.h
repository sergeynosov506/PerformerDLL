/*H*
* 
* FILENAME: dtransdesc.h
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
#endif


typedef struct
{
	int			iID;
  long		lDtransNo;
  short		iSeqno;
  char		sCloseType[2+NT];
  long		lTaxlotNo;
  double	fUnits;
  char		sDescInfo[50+NT];
} DTRANSDESC;


typedef void	(CALLBACK* LPPRDTRANSDESCSELECT)(DTRANSDESC *, int, long, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRTRANSDESC)(DTRANSDESC, ERRSTRUCT *);  //use DtransDesc as TransDesc

typedef void	(CALLBACK* LPPRPDTRANSDESC)(DTRANSDESC *);



