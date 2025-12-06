/*H*
* 
* FILENAME: divint.h
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
  char	 sSecNo[12+NT];
  char	 sWi[1+NT];
  long	 lDivintNo;
	int		 iID;
  char	 sDivType[2+NT];
  long	 lExDate;
  long	 lRecDate;
  long	 lPayDate;
  char	 sCurrId[4+NT];
  double fDivRate;
  char	 sPostStatus[1+NT];
  long	 lCreateDate;
  char	 sCreatedBy[8+NT];
  long	 lModifyDate;
  char	 sModifyBy[8+NT];
  long	 lFwdDivintNo;
  long	 lPrevDivintNo;
	char	 sDeleteFlag[1+NT];
} DIVINT;

