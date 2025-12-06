#pragma once
#include "trans.h"
#include "OLEDBIOCommon.h"

// Dtrans related functions
DLLAPI void STDCALL SelectDtrans(TRANS *pzTR, int iID, char *sSecNo, char *sWi, long lProcessTag, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateDtrans(int iID, char *sSecNo, char *sWi, long lProcessTag, char *sStatusFlag, int iBusinessError, 
							int iSqlError, int iIsamCode, int iErrorDate, char *sErrorTime, ERRSTRUCT *pzErr); 
