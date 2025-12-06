/**
* 
* SUB-SYSTEM: Database Input/Output for EffronTransImport  
* 
* FILENAME: TransImportIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in EffronTransImport app. 
*
* 
* PUBLIC FUNCTIONS(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDBIO.DLL project. 
*
* AUTHOR:	Valeriy Yegorov. (C) 2002 Effron Enterprises, Inc. 
*
*
**/

// History.
// 04/12/2002  Started.
#include "commonheader.h"
#include "dtrans.h"
#include "maptransnoex.h"
#include "trndesc.h"
#include "holdings.h"

DLLAPI void STDCALL SelectPortClos(long iID, long lCloseDate, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertDTrans (DTRANS *pzDTrans, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertDTrnDesc(TRNDESC *pzDTrnDesc, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertDPayTran(PAYTRAN *pzDPayTran, ERRSTRUCT *pzErr);


DLLAPI void STDCALL SelectTransNoByDtransNo (TRANS *pzTrans, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectTransNoByUnitsAndDates (TRANS *pzTrans, ERRSTRUCT *pzErr);

DLLAPI void STDCALL SelectMapTransNoEx(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertMapTransNoEx(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateMapTransNoEx(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeleteMapTransNoEx(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectMapTransNoExByTransNo(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr);


DLLAPI void STDCALL InsertTaxlotRecon(TAXLOTRECON *pzTaxlotRecon, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertOneTaxlotRecon(TAXLOTRECON *pzTaxlotRecon, long lBatchSize, ERRSTRUCT *pzErr);
DLLAPI void STDCALL InsertAllTaxlotRecon(ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertTradeExchange(TRADEEXCHANGE *pzTradeExchange, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateTradeExchangeByPK(TRADEEXCHANGE *pzTradeExchange, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertBnkSet(BNKSET *pzBnkSet, ERRSTRUCT *pzErr);

DLLAPI void STDCALL InsertBnkSetEx(BNKSETEX *pzBnkSetEx, ERRSTRUCT *pzErr);
DLLAPI void STDCALL UpdateBnkSetExRevNo(BNKSETEX *pzBnkSetEx, ERRSTRUCT *pzErr);
DLLAPI void STDCALL SelectBnksetexNoByUnitsAndDates (BNKSETEX *pzBnksetex, ERRSTRUCT *pzErr);

DLLAPI void STDCALL GrouppedHoldingsFor(HOLDINGS *pzHoldings, 
										int iID, char *sSecNo, char *sWi, 
										char *sSecXtend, char *sAcctType, long lTrdDate, 
										ERRSTRUCT *pzErr); 

void CloseTransImportIO(void);
void FreeTransImportIO(void);