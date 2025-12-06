#pragma once

#include "OLEDBIOCommon.h"
#include "hedgexref.h"

// Hedgexref related functions
DLLAPI void STDCALL DeleteHedgxref(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertHedgxref(HEDGEXREF *pzHG, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectHedgxref(HEDGEXREF *pzHG, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateHedgxref(HEDGEXREF *pzHG, ERRSTRUCT *pzErr);
