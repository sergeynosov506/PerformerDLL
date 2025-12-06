#pragma once

#include "OLEDBIOCommon.h"
#include "trantype.h"

// Trantype related functions
DLLAPI void STDCALL SelectTrantype(TRANTYPE *pzTrantype, char *sTranType, char *sDrCr, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL SelectTrancode(char *sTranCode, char *sTranType, char *sDrCr, ERRSTRUCT *pzErr);

// Helper functions (formerly internal to TransIO.cpp)
int FindTranType(char *sTranType, char *sDrCr);
void AddTranType(TRANTYPE zTranType);
