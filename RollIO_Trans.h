#pragma once

#include "OLEDBIOCommon.h"
#include "commonheader.h"
#include "trans.h"


DLLAPI void STDCALL SelectForwardTrans(long iID, long lEffDate1, long lEffDate2,
                                       char *sSecNo, char *sWi,
                                       BOOL bSpecificSecNo, TRANS *pzTR,
                                       ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectBackwardTrans(long iID, long lEffDate1,
                                        long lEffDate2, char *sSecNo, char *sWi,
                                        BOOL bSpecificSecNo, TRANS *pzTR,
                                        ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectAsofTrans(long iID, long lEffDate, long lTransNo,
                                    char *sSecNo, char *sWi,
                                    BOOL bSpecificSecNo, TRANS *pzTR,
                                    ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectTransByTransNo(long iID, long lTransNo1,
                                         long lTransNo2, char *sSecNo,
                                         char *sWi, BOOL bSpecificSecNo,
                                         TRANS *pzTR, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectTransByTradeDate(long iID, long lTrdDate1,
                                           long lTrdDate2, char *sSecNo,
                                           char *sWi, BOOL bSpecificSecNo,
                                           TRANS *pzTR, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectTransByEffectiveDate(long iID, long lEffDate1,
                                               long lEffDate2, char *sSecNo,
                                               char *sWi, BOOL bSpecificSecNo,
                                               TRANS *pzTR, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectTransByEntryDate(long iID, long lEntryDate1,
                                           long lEntryDate2, char *sSecNo,
                                           char *sWi, BOOL bSpecificSecNo,
                                           TRANS *pzTR, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectTransBySettlementDate(long iID, long lStlDate,
                                                TRANS *pzTR, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectAsofTransCount(long iID, long lEffDate, long lTransNo,
                                         char *sSecNo, char *sWi,
                                         BOOL bSpecificSecNo, long *piCount,
                                         ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectRegularTransCount(long iID, long lEffDate1,
                                            long lEffDate2, char *sSecNo,
                                            char *sWi, BOOL bSpecificSecNo,
                                            long *piCount, ERRSTRUCT *pzErr);

DLLAPI void STDCALL CheckIfTransExists(long iID, long *piCount,
                                       ERRSTRUCT *pzErr);

DLLAPI void STDCALL UpdatePerfDate(long iId, long lStartDate, long lEndDate,
                                   ERRSTRUCT *pzErr);
