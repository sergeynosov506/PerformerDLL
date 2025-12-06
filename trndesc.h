/*H*
* 
* FILENAME: TrnDesc.h
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
#ifndef TrnDesc_H
#define TrnDesc_H

#ifndef NT
  #define NT 1
#endif


typedef struct
{
    int		iID;
    int		lDtransNo;
    short	iSeqno;
    char	sCloseType[2+NT];
    int		lTaxlotNo;
    double	fUnits;
    char	sDescInfo[50+NT];
} DTRNDESC;

typedef struct
{
    int		iID;
    int		lDtransNo;
    short	iSeqno;
    char	sCloseType[2+NT];
    int		lTaxlotNo;
    double	fUnits;
    char	sDescInfo[50+NT];
} TRNDESC;

  
#endif // TrnDesc_H