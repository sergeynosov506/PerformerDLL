/**
* 
* SUB-SYSTEM: Database Input/Output for Asset Pricing  
* 
* FILENAME: AssetPricingIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in Delphi's written
*				AssetPricing.exe. 
*
* 
* PUBLIC FUNCTIONS(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDB.DLL project. 
*
* AUTHOR:	Valeriy Yegorov. (C) 2002 Effron Enterprises, Inc. 
*
*
**/

// History.
// 2025/11/25 Modernized to C++20 + nanodbc - Sergey
// 08/27/2002  Started.

/**
* 
* SUB-SYSTEM: Database Input/Output for Asset Pricing  
* 
* FILENAME: AssetPricingIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in Delphi's written
*				AssetPricing.exe. 
*
* 
* PUBLIC FUNCTIONS(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDB.DLL project. 
*
* AUTHOR:	Valeriy Yegorov. (C) 2002 Effron Enterprises, Inc. 
*
*
**/

// History.
// 2025/11/25 Modernized to C++20 + nanodbc - Sergey
// 08/27/2002  Started.

#ifndef ASSETPRICINGIO_H
#define ASSETPRICINGIO_H

#include "priclist.h"
#include "OLEDBIOCommon.h"

// Initialization and cleanup
DLLAPI ERRSTRUCT STDCALL InitializeAssetPricingIO();
DLLAPI void STDCALL FreeAssetPricingIO(void);

// Priclist related functions
DLLAPI void STDCALL InsertPriclist(PRICLIST zPriclist, ERRSTRUCT *pzErr);
DLLAPI void STDCALL DeletePriclist(PRICLIST zPriclist, ERRSTRUCT *pzErr);

#endif
