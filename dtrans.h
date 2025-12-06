/*H*
* 
* FILENAME: dtrans.h
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
#ifndef DTRANS_H
#define DTRANS_H

#include "trans.h"

#ifndef NT
  #define NT 1
#endif

typedef struct
{
	TRANS	zT;
	long	lProcessTag;
    char	sStatusFlag[1+NT];
    long	iErrStatusCode;

} DTRANS;

#endif // !DTRANS_H
