#pragma once

#include "OLEDBIOCommon.h"
#include "trans.h"
#include "trantype.h"
#include "payrec.h"

// Trans related functions
DLLAPI void STDCALL UpdateXTransNo(long lXTransNo, int iID, long lTransNo, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateXrefTransNo(long lXrefTransNo, int iID, long lTransNo, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL SelectTrans(TRANS * pzTR, TRANTYPE * pzTType, int iID, long lEffDate, long lTransNo, char * sSecNo, char * sWI, ERRSTRUCT * pzErr);
DLLAPI void STDCALL SelectRevTransNoAndCode(long *plRevTransNo, char *sTranCode, int iID, long lTransNo, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL UpdateNewTransNo(long lNewTransNo, int iID, long lTransNo, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL UpdateRevTransNo(long lRevTransNo, int iID, long lTransNo, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL InsertTrans(TRANS pzTrans, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL InsertPayTran(PAYTRAN zPayTran, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL UpdateBrokerInTrans(TRANS zTR, ERRSTRUCT *pzErr); 
DLLAPI void STDCALL SelectTransForMatchingXref(int iID, long lXrefTransNo, TRANS * pzTR,  ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectOneTrans(int iID, long lTransNo, TRANS *pzTR,  ERRSTRUCT *pzErr);
