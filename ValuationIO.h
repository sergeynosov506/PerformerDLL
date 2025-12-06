/**
* 
* SUB-SYSTEM: Database Input/Output for Valuation   
* 
* FILENAME: ValuationIO.h
* 
* DESCRIPTION:	Defines function prototypes
*				used for DB IO operations in Valuation.DLL . 
*
* 
* PUBLIC FUNCTIONS(S): 
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
// 09/10/2001  Started.
#ifndef VALUATIONIO_H
#define VALUATIONIO_H

#include "commonheader.h"
#include "equities.h"
#include "holdtot.h"
#include "priceinfo.h"

typedef struct
{
    char sSecNo[12+NT];
    char sWi[1+NT];
    long lPriceDate;
    int iId;
    long lDatePriceUpdated;
    long lDateExrateUpdated;
    char sPriceSource[1+NT];
    char sPriceExchange[3+NT];
    double fClosePrice;
    double fBidPrice;
    double fAskPrice;
    double fHighPrice;
    double fLowPrice;
    double fExrate;
    double fIncExrate;
    double fAnnDivCpn;
    int iVolume;
} HISTPRIC;

typedef struct
{
    char sSecNo[12+NT];
    char sWi[1+NT];
    long lPriceDate;
    int iId;
    double fAccrInt;
    double fBondEqYld;
    double fYldToWorst;
    char sYtwType[1+NT];
    double fYldToBest;
    char sYtbType[1+NT];
    double fYldToEarliest;
    char sYteType[1+NT];
    double fCurDur;
    double fCurModDur;
    double fConvexity;
    double fVariableRate;
    double fCurYld;
    double fCurYtm;
} HISTFINC;

// Modular Includes
#include "ValuationIO_Utils.h"      // Init/Free/Close
#include "ValuationIO_Accdiv.h"
#include "ValuationIO_Divhist.h"
#include "ValuationIO_Histpric.h"
#include "ValuationIO_Holdings.h"   // Holdings, Holdcash, Hedgxref
#include "ValuationIO_Holdtot.h"    // Holdtot, Ratings
#include "ValuationIO_Payrec.h"
#include "ValuationIO_Segments.h"

#endif // VALUATIONIO_H