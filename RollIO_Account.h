#pragma once

#include "commonheader.h"
#include "OLEDBIOCommon.h"

DLLAPI void STDCALL AccountDeleteHoldings(long iID, char *sSecNo, char *sWi, char *sHoldingsName, 
									 BOOL bSecSpecific, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountDeleteHoldcash(long iID, char *sSecNo, char *sWi, char *sHoldcashName, 
									 BOOL bSecSpecific, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountDeletePortmain(long iID, char *sPortmainName, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountDeleteHedgxref(long iID, char *sSecNo, char *sWi, char *sHedgxrefName, 
									 BOOL bSecSpecific, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountDeletePayrec(long iID, char *sSecNo, char *sWi, char *sPayrecName, 
								   BOOL bSecSpecific, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountInsertHoldings(long iID, char *sSecNo, char *sWi, char *sDestHoldings, char *sSrceHoldings, 
									 BOOL bSpecificSecNo, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountInsertHoldcash(long iID, char *sSecNo, char *sWi, char *sDestHoldcash, char *sSrceHoldcash, 
							         BOOL bSpecificSecNo, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountInsertPortmain(long iID, char *sDestPortmain, char *sSrcePortmain, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountInsertHedgxref(long iID, char *sSecNo, char *sWi, char *sDestHedgxref, char *sSrceHedgxref, 
									 BOOL bSpecificSecNo, ERRSTRUCT *pzErr);

DLLAPI void STDCALL AccountInsertPayrec(long iID, char *sSecNo, char *sWi, char *sDestPayrec, char *sSrcePayrec, 
								   BOOL bSpecificSecNo, ERRSTRUCT *pzErr);
