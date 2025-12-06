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

#ifndef NT
  #define NT 1
#endif

  
typedef struct
{
	long	lID;
    char	sContactType[30+NT];
    char	sUniqueName[20+NT];
    char	sAbbrev[20+NT];
    char	sDescription[60+NT];
    char	sAddress1[60+NT];
    char	sAddress2[60+NT];
    char	sAddress3[60+NT];
    long	lCityID;
    long	lStateID;
    char	sZip[10+NT];
    long	lCountryID;
    char	sPhone1[20+NT];
    char	sPhone2[20+NT];
    char	sFax1[20+NT];
    char	sFax2[20+NT];
    char	sWebAddress[100+NT];
    long	lDomicileID;

} CONTACTS;


