#pragma once

#include "DigenerateQueries.h"
#include "DigenerateOps.h"
#include "DigenerateSelectors.h"
#include "DigenerateUtils.h"
#include "commonheader.h"
#include "accdiv.h"

#ifdef __cplusplus
extern "C" {
#endif

// DllExport Prototypes for Operations
DLLAPI void InsertAccdiv(ACCDIV *zAccdiv, ERRSTRUCT *pzErr);
DllExport void SelectOneAccdiv(ACCDIV *pzAccdiv, int iID, long lDivintNo, long lTransNo, ERRSTRUCT *pzErr);
DllExport void UpdateAccdiv(ACCDIV *zAccdiv, ERRSTRUCT *pzErr);
DllExport void SelectPartCurrency(CURRENCY *pzCurrency, ERRSTRUCT *pzErr);
DllExport void InsertDivhist(DIVHIST *zDH, ERRSTRUCT *pzErr);
DllExport void UpdateDivhist(int iID, long lDivintNo, long lTransNo, double fUnits, long lExDate, long lPayDate, ERRSTRUCT *pzErr);
DllExport void DeleteDivhist(int iID, long lDivintNo, long lTransNo, ERRSTRUCT *pzErr);
DllExport void DivintUnload(DILIBSTRUCT *pzDL, long lStartExDate, long lEndExDate, char *sMode, char *sType,
							int iID, int iEndId, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
							long lTransNo, ERRSTRUCT *pzErr);
DllExport void DeleteFWTrans(int iID, ERRSTRUCT *pzErr);
DllExport void InsertFWTrans(FWTRANS *zFWTrans, ERRSTRUCT *pzErr);
DllExport void SelectAllPartPortmain(PARTPMAIN *pzPortmain, ERRSTRUCT *pzErr);
DllExport void SelectOnePartPortmain(PARTPMAIN *pzPortmain, int iID, ERRSTRUCT *pzErr);
DllExport void SelectAllSubacct(char *sAcctType, char *sXrefAcctType, ERRSTRUCT *pzErr);

#ifdef __cplusplus
}
#endif
