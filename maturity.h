/*H*
 *
 * SUB-SYSTEM: pmr maturity
 *
 * FILENAME: maturity.h
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
 * $Header: /sysprog/lib/pmr/rcs/maturity.h,v 46.1 98/03/25 18:50:23 mxs2 prod $
 *
 *H*/

// cw EXEC SQL INCLUDE "/nb/lib/pmr/include/common_pmr.h";
#include "common_pmr.h"
#ifdef __cplusplus
extern "C" {
#endif
/*cw
  typedef struct
  {
    char  sBrAcct[9];
    char  sSecNo[13];
    char  sWi[2];
    char  sSecXtend[3];
    char  sAcctType[2];
    double fUnits;
    double fOrigFace;
    char  sSecSymbol[13];
    int   iSecType;
    double fTrdUnit;
    char  sCurrId[5];
    char  sIncCurrId[5];
    double fCurExrate;
    double fCurIncExrate;
    char  sBschdType[2];
    long  lBschdDate;
    double fPrice;
  } MATSTRUCT;

  typedef struct
  {
    int       iMatCreated;
    int       iNumMat;
    MATSTRUCT *pzMaturity;
  } MATTABLE;


  #define INITTRANALLOCFUNC        200
  #define INITIALIZEPORTTABLEFUNC  202
  #define BUILDPORTDIRTABLEFUNC    204
  #define BUILDCURRENCYTABLEFUNC   205
  #define BUILDSUBACCTTABLEFUNC    206

  typedef ERRSTRUCT (CALLBACK* LPFNINITTRANALLOC)(long);
  typedef void (CALLBACK* LPFNINITIALIZEPORTTABLE)(PORTTABLE *);
  typedef ERRSTRUCT (CALLBACK* LPFNBUILDPORTDIRTABLE)(PORTTABLE *, char *, char
*, CURRTABLE); typedef ERRSTRUCT (CALLBACK* LPFNBUILDCURRENCYTABLE)(CURRTABLE
*); typedef ERRSTRUCT (CALLBACK* LPFNBUILDSUBACCTTABLE)(SUBACCTTABLE *);

//**************************************************
//new needs for maturity. All above defined in dipay

  #define INITIALIZEPINFOTABLEFUNC  221
        #define BUILDPINFOTABLEFUNC  222
        #define MAKEPRICINGFILENAMEFUNC  223
        #define FILEEXISTSFUNC  224
        #define INITIALIZEDTRANSDESCFUNC  225
        #define STYPESTATEMENTFUNC  226
        #define MATUNLSTATEMENTFUNC  227

  typedef void (CALLBACK* LPFNINITIALIZEPINFOTABLE)(PINFOTABLE *);
        typedef ERRSTRUCT (CALLBACK* LPFNBUILDPINFOTABLE)(PINFOTABLE *, char *,
long, char *); typedef char * (CALLBACK* LPFNMAKEPRICINGFILENAME)(long, char *,
char *, char *); typedef BOOL (CALLBACK* LPFNFILEEXISTS)(char *); typedef void
(CALLBACK* LPFNINITIALIZEDTRANSDESC)(DTRANSDESC *); typedef void (CALLBACK*
LPFNSTYPESTATEMENT)(SECTYPE *, ERRSTRUCT *); typedef void (CALLBACK*
LPFNMATUNLSTATEMENT)(MATSTRUCT *, long, long, char *, char *, char *, char *,
char *, ERRSTRUCT *);

//**************************************************
*/
HINSTANCE hTransEngineDll;
LPFNERRSTRUCT lpfnInitializeErrStruct;
LPFNPRINTERROR lpfnPrintError;
LPFNTRANSPOINTER lpfnInitializeTransStruct;
LPFNINITTRANALLOC lpfnInitTranAlloc;
LPFNTRANALLOC lpfnTranAlloc;

LPFNINITIALIZEDTRANSDESC lpfnInitializeDtransDesc;

HINSTANCE hTengineIODll;
LPFN3PCHAR1LONG lpfnTengineIOInit;
LPFNSELECTTRANTYPE lpfnSelectTranType;

HINSTANCE hDateFunctionsDll;
LPFNRMDYJUL lpfnrmdyjul;

HINSTANCE hDigenerateDll;
LPFNINITIALIZEPORTTABLE lpfnInitializePortTable;
LPFNBUILDPORTDIRTABLE lpfnBuildPortdirTable;
LPFNBUILDCURRENCYTABLE lpfnBuildCurrencyTable;
LPFNBUILDSUBACCTTABLE lpfnBuildSubacctTable;

LPFNINITIALIZEPINFOTABLE lpfnInitializePInfoTable;
LPFNBUILDPINFOTABLE lpfnBuildPInfoTable;
LPFNMAKEPRICINGFILENAME lpfnMakePricingFileName;
LPFNFILEEXISTS lpfnFileExists;

HINSTANCE hMaturityIODll;
LPFNSTYPESTATEMENT lpfnSTypeStatement;
LPFNMATUNLSTATEMENT lpfnMatUnlStatement;

/*cw EXEC SQL BEGIN DECLARE SECTION;
  EXEC SQL DEFINE NUMMATRECORD 100;*/
#define NUMMATRECORD 100

// cw EXEC SQL END DECLARE SECTION;

/* Global Variables */
SECTYPETABLE zSTypeTable;
CURRTABLE zCTable;
SUBACCTTABLE zSATable;
TRANTYPE zTTypeCB;
TRANTYPE zTTypeCD;
TRANTYPE zTTypeMS;
TRANTYPE zTTypeML;

int LoadFunction(HINSTANCE hDll, void *lpfn, char *sFunctionName, int iFType);

/* Prototype of functions */
ERRSTRUCT GenerateMaturity(long lValDate, char *sMode, char *sBrAcct,
                           char *sSecNo, char *sWi, char *sSecXtend,
                           char *sAcctType);
ERRSTRUCT InitializeMaturityLibrary();
void InitializeMatStruct(MATSTRUCT *pzMStruct);
void InitializeMatTable(MATTABLE *pzMTable);
ERRSTRUCT MaturityGeneration(PORTTABLE zPdirTable, PINFOTABLE zPInfoTable,
                             SUBACCTTABLE zSTable, CURRTABLE zCTable,
                             char *sMode, char *sSecNo, char *sWi,
                             char *sSecXtend, char *sAcctType, long lStartDate,
                             long lValDate, long lForwardDate);
ERRSTRUCT MaturityGeneralCursor();
ERRSTRUCT MaturityUnloadCursor(char *sMode, long lDate);
ERRSTRUCT OpenMaturityUnload(char *sMode1, long lStartDate, long lValDate1,
                             char *sBrAcct, char *sSecNo, char *sWi,
                             char *sSecXtend, char *sAcctType);
ERRSTRUCT FetchMaturityUnload(MATSTRUCT *pzMatStruct);
ERRSTRUCT MaturityUnloadAndSort(long lValDate, long lStartDate);
ERRSTRUCT BuildMatTable(char *sMode, char *sFileName, int iID, char *sSecNo,
                        char *sWi, char *sSecXtend, char *sAcctType,
                        long lStartDate, long lvalDate, long lStartingPosition,
                        MATTABLE *pzMatTable);
ERRSTRUCT AddRecordToMatTable(MATTABLE *pzMTab, MATSTRUCT zMatStruct);
ERRSTRUCT CreateTransAndCallTranAlloc(MATSTRUCT zMat, double fUnit,
                                      double fOrigFace, double fBaseCurrExrate,
                                      char *sAcctMethod, char *sIncAcctType,
                                      long lValDate);

DLLAPI ERRSTRUCT STDCALL WINAPI BuildSecTypeTable(SECTYPETABLE *pzSTable);
DLLAPI int STDCALL WINAPI FindSecType(SECTYPETABLE zSTypeTable, int iSType);

#ifdef __cplusplus
}
#endif