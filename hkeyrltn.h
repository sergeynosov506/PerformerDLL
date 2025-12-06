/*H*
* 
* FILENAME: hkeyrltn.h
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
  long lAsofDate;
  int  iID;
  char sSecNo[12+NT];
  char sWi[1+NT];
  char sSecXtend[2+NT];
  char sAcctType[1+NT];
  int  iTransNo;
  int  iScrhdrNo;
} HKEYRLTN;


typedef void	(CALLBACK* LPPRHKEYRLTN)(HKEYRLTN, ERRSTRUCT *);
