/*H*
* 
* SUB-SYSTEM: pmr di_pay  
* 
* FILENAME: dipay.h
* 
* DESCRIPTION: 
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: 
*
* $Header: /sysprog/lib/pmr/rcs/dipay.h,v 42.1 98/03/11 09:17:00 mxs2 prod Locker: mxs2 $
*
*H*/

//cw EXEC SQL INCLUDE "/nb/lib/include/common_pmr.h";
//cw EXEC SQL INCLUDE "/nb/lib/include/accdiv.h";

#include "common_pmr.h"
//#include "accdiv.h"
//#include "withhold_rclm.h"
/*cw
  #define INITTRANALLOCFUNC        200 
  #define INITIALIZEACCDIVFUNC     201
  #define INITIALIZEPORTTABLEFUNC  202
  #define INITIALIZEDIVHISTFUNC    203
  #define BUILDPORTDIRTABLEFUNC    204
  #define BUILDCURRENCYTABLEFUNC   205
  #define BUILDSUBACCTTABLEFUNC    206
  #define FINDCURRIDINCURRENCYTABLEFUNC   207
  #define ACCDIVPAYFUNC            208 
  #define ACCDIVUPDFUNC            209 
  #define FIXEDDATEDFUNC           210
  #define WITHSTATEMENTFUNC        211
  #define BUILDSECTYPETABLEFUNC    212
  #define FINDSECTYPEFUNC          213

  typedef ERRSTRUCT (CALLBACK* LPFNINITTRANALLOC)(long);
  typedef void (CALLBACK* LPFNINITIALIZEACCDIV)(ACCDIV *);  
  typedef void (CALLBACK* LPFNINITIALIZEPORTTABLE)(PORTTABLE *);  
  typedef void (CALLBACK* LPFNINITIALIZEDIVHIST)(DIVHIST *);  
  typedef ERRSTRUCT (CALLBACK* LPFNBUILDPORTDIRTABLE)(PORTTABLE *, char *, char *, CURRTABLE);
  typedef ERRSTRUCT (CALLBACK* LPFNBUILDCURRENCYTABLE)(CURRTABLE *);
  typedef ERRSTRUCT (CALLBACK* LPFNBUILDSUBACCTTABLE)(SUBACCTTABLE *);
  typedef int  (CALLBACK* LPFNFINDCURRIDINCURRENCYTABLE)(CURRTABLE, char *);

  typedef void (CALLBACK* LPFNACCDIVPAY)(ACCDIV *, long, char *, char *, char *, char *, char *, char *, long, ERRSTRUCT *);
	typedef void (CALLBACK* LPFNACCDIVUPD)(char *, char *, char *, char *, char *, char *,
                                         long, long, ERRSTRUCT *);
	typedef void (CALLBACK* LPFNFIXEDDATED)(long *, char *, char *, ERRSTRUCT *);
  typedef void (CALLBACK* LPFNWITHSTATEMENT)(WITHHOLDRCLM *, char *, ERRSTRUCT *);

  typedef ERRSTRUCT (CALLBACK* LPFNBUILDSECTYPETABLE)(SECTYPETABLE *); 
  typedef int  (CALLBACK* LPFNFINDSECTYPE)(SECTYPETABLE, int); 
*/	
	HINSTANCE hTransEngineDll;
  LPFNERRSTRUCT		    lpfnInitializeErrStruct;
  LPFNPRINTERROR	    lpfnPrintError;
  LPFNTRANSPOINTER    lpfnInitializeTransStruct;
	LPFNINITTRANALLOC   lpfnInitTranAlloc;
	LPFNTRANALLOC       lpfnTranAlloc;

  HINSTANCE hTengineIODll;
	LPFN3PCHAR1LONG      lpfnTengineIOInit;
  LPFNSELECTTRANTYPE   lpfnSelectTranType;

  HINSTANCE hDateFunctionsDll;
	LPFNRMDYJUL			lpfnrmdyjul;
  
	HINSTANCE hDigenerateIODll;
	LPFNDIVHIST    lpfnInsertDivhist;

	HINSTANCE hDigenerateDll;
	LPFNINITIALIZEACCDIV		   lpfnInitializeAccdiv;
  LPFNINITIALIZEPORTTABLE    lpfnInitializePortTable;
  LPFNINITIALIZEDIVHIST			 lpfnInitializeDivhist;
	LPFNBUILDPORTDIRTABLE			 lpfnBuildPortdirTable;
  LPFNBUILDCURRENCYTABLE     lpfnBuildCurrencyTable;
  LPFNBUILDSUBACCTTABLE      lpfnBuildSubacctTable;
	LPFNFINDCURRIDINCURRENCYTABLE   lpfnFindCurrIdInCurrencyTable;

  HINSTANCE hMaturityDll;
	LPFNBUILDSECTYPETABLE      lpfnBuildSecTypeTable;
	LPFNFINDSECTYPE            lpfnFindSecType;

	HINSTANCE hDipayIODll;
	LPFNACCDIVPAY       lpfnAccdivPay;
	LPFNACCDIVUPD       lpfnAccdivUpd;
	LPFNFIXEDDATED      lpfnFixedDated;
	LPFNWITHSTATEMENT   lpfnWithStatement;
  

/* Global Variables */
SUBACCTTABLE zSATable;
SECTYPETABLE zSTTable;
CURRTABLE    zCTable;
WHRCTABLE    zWRTable;
TRANTYPE     zTTypeSP;
TRANTYPE     zTTypeSX;
TRANTYPE     zTTypeSD;
TRANTYPE     zTTypeSB;
TRANTYPE     zTTypeLD;
TRANTYPE     zTTypeRD;
TRANTYPE     zTTypeRI;
TRANTYPE     zTTypeWH;
TRANTYPE     zTTypeAR;
TRANTYPE     zTTypeRS;
TRANTYPE     zTTypeRX;
TRANTYPE     zTTypeCB;
TRANTYPE     zTTypeCD;
TRANTYPE     zTTypeML;
TRANTYPE     zTTypeMS;

/* Function Prototypes */
ERRSTRUCT PayDivint(long lValDate, char *sMode, char *sProcessFlag,
                       char *sBrAcct, char *sSecNo, char *sWi, char *sSecXtend,
                       char *sAcctType, long lTransNo);
ERRSTRUCT InitializeDivintPayLib(char *sMode, char *sProcessingFlag);
ERRSTRUCT DivintPayCursor(char *sMode, char *sProcessFlag);
ERRSTRUCT WithReclCursor();
ERRSTRUCT OpenWithRecl();

ERRSTRUCT GetPortdirInfo(PORTTABLE zPortTable, char *sBrAcct,
                         double *pzPortBaseXrate, char *sIncByLot, 
                         double *pzSwh, double *pzBwh);       
ERRSTRUCT OpenAccdivPay(char *sMode, char *sBrAcct, char *sSecNo, char *sWi,
                        char *sSecXtend, char *sAcctType, long lTransNo,  
                        long lValDate);
ERRSTRUCT FetchAccdivRecord(ACCDIV *pzAccdiv, int *piSecType,double *pzTradUnit,
                            double *pzExRate, double *pzIncExRate);
ERRSTRUCT UpdateAccdivDeleteFlag(char *sBrAcct2, char *sSecNo2, char *sWi2,
                                 char *sSecXtend2, char *sAcctType2, 
                                 long lTransNo2, long lDivintNo2);
ERRSTRUCT  CreateTransFromAccdiv(TRANS *pzTR, ACCDIV zAccdiv, 
                                       CURRTABLE zCTable, double zPortBaseXrate,
                                       double zExRate, double zIncExRate,
                                       char *sPrimaryType, long lValDate);
ERRSTRUCT PostTrades(TRANS zTrans, TRANS zTransWH, TRANS zTransAR, 
                     DTRANSDESC zDTr[], TRANTYPE zTranType, SECTYPE zSecType,
                     ASSETS zAssets);
int FindCurrIdInWithRclTable(WHRCTABLE zWRTable, char *sCurrId);
void      CopyFieldsFromTransToAssets(TRANS zTR, double zTrdUnit, 
                                      ASSETS *pzAssets);

ERRSTRUCT BuildWithReclTable(WHRCTABLE *pzWRTable);