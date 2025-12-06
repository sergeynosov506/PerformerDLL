#pragma once

#include "commonheader.h"
#include "OLEDBIOCommon.h"

DLLAPI void STDCALL ReadAllHoldmap(char *sHoldingsName, char *sHoldcashName, char *sPortbalName, char *sPortmainName, 
							  char *sHedgxrefName, char *sPayrecName, char *sHoldtotName, char *sDataType, 
							  long *plAsofDate, ERRSTRUCT *pzErr);
