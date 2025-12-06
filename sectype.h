/*H*
* 
* FILENAME: sectype.h
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

#ifndef SECTYPE_H
#define SECTYPE_H

#ifndef NT
  #define NT 1
#endif

#define NUMSECTYPES				250	// maximum number of sectype records

typedef struct
{
	short iSecType;
	char	sSecTypeDesc[30+NT];
	char	sPrimaryType[1+NT];
	char	sSecondaryType[1+NT];
	char	sThirdType[1+NT];
	short iStlDays;
	char	sSecFeeFlag[1+NT];
	char	sPayType[1+NT];
	short iAccrualSched;
	char	sCommissionFlag[1+NT];
	char	sPositionInd[1+NT];
	char	sLotInd[1+NT];
	char	sCostInd[1+NT];
	char	sLotExistsInd[1+NT];
	char	sAvgInd[1+NT];
	char	sMktValInd[1+NT];
	char	sScreenInd[1+NT];
	char	sIndustLevel[4+NT];
	char	sYield[1+NT];
	int		iIntcalc;
} SECTYPE;

typedef struct 
{
	int			iNumSType;
	SECTYPE	zSType[NUMSECTYPES];
} SECTYPETABLE;

typedef struct 
{
  int  iSecType;
  char sPrimaryType[2];
  char sSecondaryType[2];
  char sMktValInd[2];
} PARTSTYPE;
 
typedef struct 
{
  int       iNumSType;
  PARTSTYPE zSType[NUMSECTYPES];
} PARTSTYPETABLE;
 


typedef void			(CALLBACK* LPPRSECCHAR)(char *, char *, char *, char *, char *, char *, char *, double *, char *, char *, char *, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRSECTYPE)(SECTYPE *, int, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRALLSECTYPES)(SECTYPE *, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRALLSECTYPE)(SECTYPE *, ERRSTRUCT *);
typedef void			(CALLBACK* LPPRSELECTALLPARTSECTYPE)(PARTSTYPE *, ERRSTRUCT *);
typedef ERRSTRUCT (CALLBACK* LPFNPPARTSECTYPETABLE)(PARTSTYPETABLE *);

#endif // SECTYPE_H

