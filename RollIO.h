#pragma once

/**
* 
* SUB-SYSTEM: Database Input/Output for Roll   
* 
* FILENAME: RollIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in Roll.DLL . 
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
// 08/29/2001  Started.


#define  UNPREP_THISQUERY				 1
#define  UNPREP_TRANSQUERIES             2
#define  UNPREP_ACCNTSOURCEQUERIES       3
#define  UNPREP_ACCNTDESTINATIONQUERIES  4
#define  UNPREP_FIRMSOURCEQUERIES        5
#define  UNPREP_FIRMDESTINATIONQUERIES   6
#define  UNPREP_MGRSOURCEQUERIES         7
#define  UNPREP_MGRDESTINATIONQUERIES    8
#define  UNPREP_COMPSOURCEQUERIES        9
#define  UNPREP_COMPDESTINATIONQUERIES   10

#include "RollIO_Holdmap.h"
#include "RollIO_Trans.h"
#include "RollIO_Account.h"
#include "RollIO_Portmain.h"
#include "RollIO_Position.h"

void CloseRollIO(void);
DLLAPI void STDCALL FreeRollIO(void);
DLLAPI int STDCALL UnprepareRollQueries(char *sTableOrQueryName, int iAction);
