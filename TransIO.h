/**
* 
* SUB-SYSTEM: Database Input/Output for TranProc  
* 
* FILENAME: TransIO.h
* 
* DESCRIPTION:	Defines functions
*				used for DB IO operations in TransEngine.DLL . 
*
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDB.DLL project. 
*
* AUTHOR:	Valeriy Yegorov. (C) 2001 Effron Enterprises, Inc. 
*
*
**/

// History.
// 08/15/2001  Initial version.

#include <time.h>
#include <string>
using namespace std;

#include "commonheader.h"
#include "dcontrol.h"
#include "assets.h"
#include "trans.h"
#include "trndesc.h"
#include "fixedinc.h"
#include "hedgexref.h"
#include "holdcash.h"
#include "holdings.h"
#include "holddel.h"
#include "payrec.h"
#include "portmain.h"
#include "priceinfo.h"
#include "sectype.h"
#include "trantype.h"
#include "subacct.h"
#include "segmain.h"
#include "OLEDBIOCommon.h"
#include "TransIO_Holdings.h"
#include "TransIO_Holdcash.h"
#include "TransIO_Divhist.h"
#include "TransIO_AccDiv.h"
#include "TransIO_Holddel.h"

#include "TransIO_DControl.h"
#include "TransIO_Deriv.h"
#include "TransIO_DTrans.h"
#include "TransIO_DTransDesc.h"
#include "TransIO_Fixedinc.h"
#include "TransIO_Hedgexref.h"
#include "TransIO_Holdmap.h"
#include "TransIO_Payrec.h"
#include "TransIO_Portmain.h"
#include "TransIO_Sectype.h"
#include "TransIO_SysSettings.h"
#include "TransIO_StarsDat.h"
#include "TransIO_Subaccount.h"
#include "TransIO_Assets.h"
#include "TransIO_Currency.h"
#include "TransIO_Income.h"
#include "TransIO_Portlock.h"
#include "TransIO_Trans.h"
#include "TransIO_TransDesc.h"
#include "TransIO_Trantype.h"
#include "TransIO_SecurityPrice.h"

extern	PORTMAIN		zSavedPMain;
extern	ASSETS			zSavedAsset;
extern	SECTYPETABLE	zSTable;
extern	SUBACCTTABLE1	zSATable;
extern	TRANTYPETABLE1	zTTable;

extern LPFN1LONG2PCHAR1PLONG	lpfnLastBusinessDay;


ERRSTRUCT InitializeTransEngineIO(void); 
void FreeTransIO(void);
void CloseTransIO(void);

 

// Lock abd unLock a portfolio
// All functions moved to respective TransIO_*.h files

// SelectCurrencySecno moved to TransIO_Currency.h 

 


// get Income related info from trans
// Income functions moved to TransIO_Income.h



      		

      		
