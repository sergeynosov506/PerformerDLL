#pragma once

#include "OLEDBIOCommon.h"
#include "payrec.h"

// Payrec related functions
DLLAPI void STDCALL DeletePayrec(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, 
					        long lDivintNo, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL InsertPayrec(PAYREC zPYR, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL SelectPayrec(PAYREC *pzPYR, int iID , char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
							long lTransNo, long lDivintNo, ERRSTRUCT *pzErr); 
