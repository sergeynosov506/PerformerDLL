/**
* 
* SUB-SYSTEM: Database Input/Output for Dividend Payment   
* 
* FILENAME: DipayIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in Payments.DLL . 
*
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDB.DLL project. 
*
* AUTHOR:	Valeriy Yegorov. (C) 2001 Effron Enterprises, Inc. 
*
*
**/

// History.
// 09/04/2001  Started.

#include "commonheader.h"
#include "accdiv.h"
#include "porttax.h"
#include "withhold_rclm.h"

DLLAPI ERRSTRUCT STDCALL InitializeDivIntPayIO(char *sMode, char *sType);
DLLAPI void STDCALL FreeDivIntPayIO(void);


// Accdiv related functions
DLLAPI void STDCALL SelectAllAccdiv(ACCDIV *pzAccdiv, int * iSecType, double *fTradingUnit, double *fCurExrate,
                          double *fCurIncExrate, char *sMode, char *sType, int iID, char *sSecNo, char *sWi, 
						  char *sSecXtend, char *sAcctType, long lTransNo, long lEffDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdateAccdivDeleteFlag(char *sDeleteFlag, int iID, char *sSecNo, char *sWi, char *sSecXtend,
									char *sAcctType, long lTransNo, long lDivintNo, ERRSTRUCT *pzErr);

// Porttax related functions
DLLAPI void STDCALL SelectPorttax(int iID, long lTaxDate, PORTTAX *pzPTax, ERRSTRUCT *pzErr);

// Withrclm related functions
DLLAPI void STDCALL SelectWithrclm(WITHHOLDRCLM *pzWH, ERRSTRUCT *pzErr);
