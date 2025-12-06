#pragma once

#include "OLEDBIOCommon.h"
#include "SysSettings.h"

// Syssettings related functions
DLLAPI void STDCALL SelectSyssettings(SYSSETTING *pzSyssetng, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectSysvalues(SYSVALUES *pzSysvalues, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectCFStartDate(VARIANT *pvCFStartDate, ERRSTRUCT *pzErr);
