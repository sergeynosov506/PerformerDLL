/*H*
* 
* FILENAME: withhold_rclm.h
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
#ifndef WITHHOLD_RCLM_H
#define WITHHOLD_RCLM_H

#include "commonheader.h"

# ifndef NT
	#define NT
#endif

#define NUMWITHRECL				200	// maximum number of withhold recliam records

typedef struct
{
	char   sCurrId[4+NT];
  long   lValidDate;
  double fFixedincWithhold;
  double fFixedincRclm;
  double fEquityWithhold;
  double fEquityRclm;
} WITHHOLDRCLM;


typedef struct 
{
  char   sCurrId[5];
  double fFxWithRate;
  double fFxRclmRate;
  double fEqWithRate;
  double fEqRclmRate;
} PARTWITHRECL;

typedef struct 
{
  int           iNumWithRecl;
  PARTWITHRECL  zWithRecl[NUMWITHRECL];
} WITHRECLTABLE;


 
typedef void	(CALLBACK* LPPRWITHHOLDRCLM)(WITHHOLDRCLM *, ERRSTRUCT *);

#endif // WITHHOLD_RCLM_H