/*H*
* 
* FILENAME: Dcontrol.h
* 
* DESCRIPTION: 
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: vay
*
*H*/
#ifndef Dcontrol_H
#define Dcontrol_H

#ifndef NT
  #define NT 1
#endif

  
typedef struct
{
	long	lRecordDate;
	char	sCountry[3+NT];
	char	sMarketClosed[1+NT];
	char	sBAnkClosed[1+NT];
	char	sRecordDescription[30+NT];
} DCONTROL;

#endif // !Dcontrol_H
