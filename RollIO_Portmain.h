#pragma once

#include "commonheader.h"
#include "OLEDBIOCommon.h"

DLLAPI void STDCALL SelectLastTransNo(long iID, char *sPortmainName, long *plLastTransNo, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectLastRollDate(long iID, char *sPortmainName, long *plLastRollDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdateRollDateAndLastTransNo(long iID, long lRollDate, long lLastTransNo, char *sPortmainName, ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdateRollDate(long iID, long lRollDate, char *sPortmainName, ERRSTRUCT *pzErr);
