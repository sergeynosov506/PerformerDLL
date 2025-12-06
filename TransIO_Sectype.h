#pragma once

#include "OLEDBIOCommon.h"
#include "sectype.h"

// Sectype related functions
DLLAPI void STDCALL SelectSectype(SECTYPE *pzSectype, short iSecType, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL SecurityCharacteristics(char *sPrimaryType, char *sSecondaryType, char *sPositionInd, char *sLotInd,
									   char *sCostInd, char *sLotExistsInd, char *sAvgInd, double *pfTradUnit,
									   char *sCurrId, char *sSecNo, char *sWhenIssue, ERRSTRUCT *pzErr); 
