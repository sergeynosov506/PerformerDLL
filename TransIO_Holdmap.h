#pragma once

#include "OLEDBIOCommon.h"

// Holdmap related functions
DLLAPI void STDCALL SelectHoldmap(long lAsofDate, char *sHoldingsName, char *sHoldcashName, char *sPortmainName, 
							 char *sPortbalName, char *sPayrecName, char *sHXrefName, char *sHoldtotName,
							 ERRSTRUCT *pzErr); 
