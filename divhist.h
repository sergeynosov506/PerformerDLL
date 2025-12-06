/*H*
* 
* SUB-SYSTEM: pmr di_genrt  
* 
* FILENAME: divhist.h
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
* $Header:   P:\Effron\C\DIVHIST.H__   1.5   May 06 1999 14:16:04   shobhit  $
*
*H*/
#ifndef DIVHIST_H
#define DIVHIST_H

#ifndef NT
  #define NT 1
#endif

typedef struct
{
	int		 iID;
  long   lTransNo;
  long   lDivintNo;
  char   sTranType[2+NT];
	char   sSecNo[12+NT];
	char   sWi[1+NT];
	int		 iSecID;
	char   sSecXtend[2+NT];
	char   sAcctType[1+NT];
  double fUnits;
  long   lDivTransNo;
  char   sTranLocation[1+NT];
  long   lExDate;
  long   lPayDate;
} DIVHIST;

typedef void	(CALLBACK* LPPRDIVHIST)(DIVHIST *, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRUPDATEDIVHIST)(int, long, long, double, long, long, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRALLDIVHISTFORANACCOUNT)(int, DIVHIST *, ERRSTRUCT *);

#endif