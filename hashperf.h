/*
* SUB-SYSTEM: pmr calcperf
*
* FILENAME: nbcHashPerf.h
*
* DESCRIPTION: Structure definitions for Hash tables for calcperf
*              It is also used by valprep
*
* PUBLIC FUNCTION(S):
*
* NOTES: Define "EXEC SQL DEFINE PERFHASH 1" if using me from calcperf
*        Do NOT define PERFHASH is using me from valprep.
*        (reason: constant definition overlap with nbcHashVal.h)
*
* USAGE:
*
* AUTHOR: JBG (adapted from work by SKL)
*
* $Header: /sysprog/lib/pmr/rcs/nbcHashPerf.h,v 59.1 98/10/29 11:54:18 txl prod
Locker: jbg $
*
*/

#ifndef NT
	#define NT 1
#endif

/*
** IMPORTANT *** DEFINE PERFHASH in your program unless using nbcHashVal.h
*/

#ifndef HPERF
#define HPERF 1
#define SECNOSIZE 12
#define WHENISSUESIZE 1
#define BRACCTSIZE 8
#define BASECURRIDSIZE 4
#define CAVAILSIZE 1
#define FAVAILSIZE 1
#define TRANTYPESIZE 2
#define CURRIDSIZE 4
#define MGRSIZE 7
#define PRICESOURCESIZE 1
#define TAXSIZE 1
#define ACCTTYPESIZE 1
 
/*
 * return code for assetget function
 */


#define ASSETGETNOTFOUND 0
#define ASSETGETFOUND 1
 
#ifndef FALSE
#define FALSE 0
#endif
 
#ifndef TRUE
#define TRUE 1
#endif
 
#define PERF_ASSETKEYSIZE SECNOSIZE+1+WHENISSUESIZE+1

/*
** IMPORTANT *** DEFINE PERFHASH in your program unless using nbcHashVal.h
*/

#ifdef PERFHASH
	#define ESTIMATEASSETS 40000
#endif

/*
 * structure used for assets and nb_hist_price
 */
/*typedef struct 
{
    // key
    char sSecNo[SECNOSIZE + 1];
    char sWhenIssue[WHENISSUESIZE + 1];
 
    //  data
    short int iSecType;
    char sCurrId[CURRIDSIZE + 1];
    char sIncCurrId[CURRIDSIZE + 1];
    char  sIndustLevel1[5];
    char  sIndustLevel2[5];
    char  sIndustLevel3[5];
    char  sSpRating[5];
    char  sMoodyRating[5];
    char  sNbRating[5];
    char  sTaxStat[2];
//  dec_t tAnnDivCpn;
    //dec_t tCurExRate;
    double fAnnDivCpn;
    double fCurExRate;

	  char  bValidFlag;
} PERF_ASSET;
*/ 
 
/*
** IMPORTANT *** DEFINE PERFHASH in your program unless using nbcHashVal.h
*/
/*
EXEC SQL ifdef PERFHASH;
EXEC SQL define PORTDIRKEYSIZE BRACCTSIZE+1;
EXEC SQL define ESTIMATEPORTDIR 100;
EXEC SQL endif;
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
    /*
     * key
     */
    char iID[BRACCTSIZE + 1];
 
    /*
     *  data
     */
    char sBaseCurrId[BASECURRIDSIZE + 1];
    char sTax[TAXSIZE + 1];
    long lValDate;
    long lInceptDate;
    long lFiscal;
} PERF_PORTDIR;
 
/*
** IMPORTANT *** DEFINE PERFHASH in your program unless using nbcHashVal.h
*/

/*
EXEC SQL ifdef PERFHASH;
EXEC SQL define ACCDIVKEYSIZE BRACCTSIZE+1+sizeof(long int);
EXEC SQL define ESTIMATEACCDIV 100;
EXEC SQL endif;
EXEC SQL define PERFSECXTENDSIZE 2;
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
    /*
     * key
     */
    int iID;
    long int iTransNo;
 
    /*
     * data
     */
    long int iDivIntNo;
/*    dec_t tPcplAmt;    */
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

#endif
/*   EXEC SQL END DECLARE SECTION;   */
