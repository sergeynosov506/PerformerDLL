#include <stdio.h>

typedef struct 
{
   // key
    char sSecNo[13];
    char sWhenIssue[2]; 
    //  data
    short int iSecType;
    char sCurrId[5];
    char sIncCurrId[5];
    char  sIndustLevel1[5];
    char  sIndustLevel2[5];
    char  sIndustLevel3[5];
    char  sSpRating[5];
    char  sMoodyRating[5];
    char  sNbRating[5];
    char  sTaxStat[2];
    double fAnnDivCpn;
    double fCurExRate;
	  char  bValidFlag;
} PERF_ASSET;

#define SECNOSIZE 12
#define WHENISSUESIZE 1
#define CURRIDSIZE 4
#define BRACCTSIZE 8
#define BASECURRIDSIZE 4
#define TAXSIZE 1
#define TRANTYPESIZE 2
#define ACCTTYPESIZE 1
/*#define MGRSIZE 7
#define PRICESOURCESIZE 1
#define CAVAILSIZE 1
#define FAVAILSIZE 1

/*#define ASSETGETNOTFOUND 0
#define ASSETGETFOUND 1
 
#ifndef FALSE
	#define FALSE 0
#endif
 
#ifndef TRUE
	#define TRUE 1
#endif
 
#define PERF_ASSETKEYSIZE SECNOSIZE+1+WHENISSUESIZE+1
*/
/*#ifdef PERFHASH
	#define ESTIMATEASSETS 40000
#endif
*/
 
/*
** IMPORTANT *** DEFINE PERFHASH in your program unless using nbcHashVal.h
*/
#ifdef PERFHASH
	#define PORTDIRKEYSIZE BRACCTSIZE+1
	#define ESTIMATEPORTDIR 100
#endif
 /*
 * structure for port_dir
 */
typedef struct
{
    // key
    char iID[BRACCTSIZE + 1];
 
     //  data
    char sBaseCurrId[BASECURRIDSIZE + 1];
    char sTax[TAXSIZE + 1];
    long lValDate;
    long lInceptDate;
    long lFiscal;
} PERF_PORTDIR;
 
/*
** IMPORTANT *** DEFINE PERFHASH in your program unless using nbcHashVal.h
*/
#ifdef PERFHASH
	#define ACCDIVKEYSIZE BRACCTSIZE+1+sizeof(long int)
	#define ESTIMATEACCDIV 100
#endif
#define PERFSECXTENDSIZE 2
	
/*
 * structure for acc_div
 */
typedef struct
{
    // key
    int iID;
    long int iTransNo;
 
    // data
    long int iDivIntNo;
//    dec_t tPcplAmt;    
		double fPcplAmt;
    char sTranType[TRANTYPESIZE + 1];
    char sDrCr[2+NT];
    char sSecNo[SECNOSIZE + 1];
    char sWhenIssue[WHENISSUESIZE + 1];
    char sAcctType[ACCTTYPESIZE + 1];
    char sSecXtend[PERFSECXTENDSIZE + NT];
    long lTrdDate;
    long lStlDate;
    long lEffDate;
    char sCurrId[4+NT];
    char sCurrAcctType[1+NT];
    char sIncCurrId[4+NT];
    char sIncAcctType[1+NT];
    char bValidFlag;
} PERF_ACCDIV;
