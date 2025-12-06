#pragma once

#include "OLEDBIOCommon.h"

// Lock and unLock a portfolio
DLLAPI int STDCALL InsertPortLock(int iId, char *sCreatedBy, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL SelectPortLock(int iId, char *sUserId, ERRSTRUCT *pzErr); 

DLLAPI void STDCALL DeletePortLock(int iId, ERRSTRUCT *pzErr); 
