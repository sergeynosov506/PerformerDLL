/**
* FILENAME: commonheader.h
* DESCRIPTION: Common include files and definitions for all DLLs
* NOTES:
*   - Modernized guard (#pragma once), but keeps original constants & typedefs
*   - Adds DLLAPI/STDCALL macros and C exports for InitializeErrStruct/PrintError
*   - Safe with or without MFC; <afx.h> guarded
* USAGE: Shared by OLEDBIO, TransIO, etc.
* AUTHOR: Shobhit Barman
* History:
*   2018-07-31 VI#61746 Added futures cost/G&L for secondary type 'N' -mk
*   2025-11-09 Modernized for ODBC/nanodbc bridge (Delphi-compatible exports)
*/

#pragma once

#pragma warning(disable : 4244)  // type conversions

// -----------------------------------------------------------------------------
// Export & calling-convention macros (consistent across DLLs; Delphi-friendly)
// -----------------------------------------------------------------------------
#ifndef DLLAPI
#  define DLLAPI extern "C" __declspec(dllexport)
#endif
#ifndef STDCALL
#  define STDCALL __stdcall
#endif

// Keep the original import/export names for legacy code that includes this
#ifndef DllImport
#  define DllImport __declspec(dllimport)
#endif
#ifndef DllExport
#  define DllExport __declspec(dllexport)
#endif

// -----------------------------------------------------------------------------
// Core constants (unchanged from original)
// -----------------------------------------------------------------------------
#define COMMONHEADER 1

#define NT                1   // null terminator
#define MAX_BULK_ROWS     1000
#define GUID_STR_SIZE     36

#define DEFAULTMMLOT      9999999  // default money market lot #

#define SQLNOTFOUND       100       // record not found
#define ERR_INVALIDFILENAME  -100
#define ERR_INVALIDFUNCTION  -101

// local DataSet batch size before pushing to SQL Server
#define INSERT_BATCH_SIZE 0 // 5000
#define MKT_VAL_INVALID   -1.7e308

// primary type (sectype)
#define PTYPE_FUTURE   'F'
#define PTYPE_EQUITY   'E'
#define PTYPE_BOND     'B'
#define PTYPE_CASH     'C'
#define PTYPE_OPTION   'O'
#define PTYPE_MMARKET  'M'
#define PTYPE_UNITS    'U'
#define PTYPE_LOAN     'L'
#define PTYPE_DISCOUNT 'D'

// secondary type (sectype)
#define STYPE_TBILLS         'T'
#define STYPE_MFUND          'U'
#define STYPE_GOVTBONDNOTES  'G'
#define STYPE_CONTRACTS      'R'
#define STYPE_INDEX          'I'
#define STYPE_MUNICIPALS     'M'
#define STYPE_FOREIGN        'N'

// pay type (sectype)
#define PAYTYPE_DISCOUNT   'D'
#define PAYTYPE_INFPROTCTD 'I'

// account type (subacct)
#define ACCTYPE_IHLONG          '0'
#define ACCTYPE_OHLONG          'A'
#define ACCTYPE_IHSHORT         '5'
#define ACCTYPE_OHSHORT         'B'
#define ACCTYPE_WHENISSUE       'W'
#define ACCTYPE_IHINCOME        '3'
#define ACCTYPE_OHINCOME        'N'
#define ACCTYPE_FUTURE          'F'
#define ACCTYPE_SUSPENDED       '6'
#define ACCTYPE_NONPURPOSELOAN  '4'

// -----------------------------------------------------------------------------
// Includes (MFC optional; kept for compatibility)
// -----------------------------------------------------------------------------
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef _AFXDLL
#  include <afx.h>
#endif

// Date markers (unchanged)
#define DEFAULT_DATE    -1 // 12/31/1899 resolved as pricing date
#define ADHOC_DATE       0 // 01/01/1900 adhoc positions
#define PERFORM_DATE     1 // 01/02/1900 performance adhoc roll
#define SETTLEMENT_DATE  2 // 01/03/1900 settlement date holdings
#define MAXDATE     219510 // 12/31/2500

// Legacy VARIANT helper (used in some old modules)
#ifndef SETVARDATE
#  define SETVARDATE(r, v) ( (r.vt = VT_DATE), (r.date = v))
#endif

// -----------------------------------------------------------------------------
// Query-preparation flags (must match Delphi StarsIODefinitionsUnit)
// -----------------------------------------------------------------------------
#define PREPQRY_NONE         0
#define PREPQRY_TRANSENGINE  1
#define PREPQRY_DIGENERATE   2
#define PREPQRY_DIPAY        3
#define PREPQRY_MATURITY     4
#define PREPQRY_AMORTIZE     5
#define PREPQRY_ROLL         6
#define PREPQRY_PERFORM      7
#define PREPQRY_VALUATION    8
#define PREPQRY_PAYMENTS    20 // di-generate + di-pay + maturity
#define PREPQRY_ALL        100

// String sizes
#define STR1LEN   (1+NT)
#define STR2LEN   (2+NT)
#define STR3LEN   (3+NT)
#define STR5LEN   (5+NT)
#define STR6LEN   (6+NT)
#define STR7LEN   (7+NT)
#define STR8LEN   (8+NT)
#define STR12LEN (12+NT)
#define STR15LEN (15+NT)
#define STR20LEN (20+NT)
#define STR30LEN (30+NT)
#define STR36LEN (36+NT)
#define STR40LEN (40+NT)
#define STR60LEN (60+NT)
#define STR64LEN (64+NT)
#define STR80LEN (80+NT)

// PortfolioType (unchanged)
enum PortfolioType { ptUknown, ptComposite, ptFamily, ptIndividual, ptModel };

// -----------------------------------------------------------------------------
// ERRSTRUCT (unchanged layout; used across all DLLs)
// -----------------------------------------------------------------------------
typedef struct ERRSTRUCT
{
    int  iID;
    long lRecNo;              /* RecType - D(Dtransno), F(perfFormno), H(templateHdrno) */
    char sRecType[1 + NT];      /* I(divIntno), P(Perfkey), R(perfRule), S(Scripthdrno) */
    int  iBusinessError;      /* or T(TemplatehdrNo) */
    int  iSqlError;
    int  iIsamCode;
} ERRSTRUCT;

// Flow calculator struct (unchanged)
typedef struct FLOWCALCSTRUCT {
    double fBPcplSecFlow;
    double fLPcplSecFlow;
    double fBIncomeSecFlow;
    double fLIncomeSecFlow;
    double fBPcplCashFlow;
    double fLPcplCashFlow;
    double fBAccrualFlow;
    double fLAccrualFlow;
    double fBIncomeCashFlow;
    double fLIncomeCashFlow;
    double fBSecFees;
    double fLSecFees;
    double fBSecIncome;
    double fLSecIncome;
} FLOWCALCSTRUCT;

// -----------------------------------------------------------------------------
// Typedefs for dynamically loaded function pointers (unchanged)
// -----------------------------------------------------------------------------
typedef int        (CALLBACK* LPFN1INT)(int);
typedef int        (CALLBACK* LPFN1BOOL)(BOOL);
typedef void       (CALLBACK* LPPR1PCHAR)(char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1INT1LONG)(int, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1INT2LONG1PCHAR)(int, long, long, char*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN1INT2PCHAR1INT)(int, char*, char*, int);
typedef ERRSTRUCT(CALLBACK* LPFN1INT2PCHAR4LONG2DOUBLE1BOOL1PDOUBLE)(int, char*, char*, long, long, long, long, double, double, BOOL, double*);
typedef void       (CALLBACK* LPPR1INT3LONG1PCHAR)(int, long, long, long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1INT4PCHAR1LONG)(int, char*, char*, char*, char*, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1INT4PCHAR2LONG)(int, char*, char*, char*, char*, long, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1INT4PCHAR2LONG1PDOUBLE)(int, char*, char*, char*, char*, long, long, double*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1INT4PCHAR3LONG1PDOUBLE)(int, char*, char*, char*, char*, long, long, long, double*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN3INT3PCHAR1BOOL)(int, int, int, char*, char*, char*, BOOL);
typedef void       (CALLBACK* LPPRDTRANSUPDATE)(int, char*, char*, long, char*, int, int, int, long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PINT3LONG2PCHAR)(int*, long, long, long, char*, char*, ERRSTRUCT*);

typedef void       (CALLBACK* LPPR1LONG)(long, ERRSTRUCT*);
typedef BOOL(CALLBACK* LPFN1LONG)(long);
typedef long       (CALLBACK* LP2FN1LONG)(long);
typedef int        (CALLBACK* LPFN1LONG1PCHAR)(long, char*);
typedef void       (CALLBACK* LPPR1LONG1PCHAR)(long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1LONG1PLONG)(long, long*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1LONG1PCHAR1BOOL)(long, char*, BOOL, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1LONG1PCHAR1LONG)(long, char*, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1LONG1PCHAR1PLONG)(long, char*, long*, ERRSTRUCT*);

typedef ERRSTRUCT(CALLBACK* LPFN1LONG2PCHAR)(long, char*, char*);
typedef void       (CALLBACK* LPPR1LONG2PCHAR)(long, char*, char*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN1LONG2PCHAR1PLONG)(long, char*, char*, long*);
typedef void       (CALLBACK* LPPR1LONG3PCHAR1BOOL)(long, char*, char*, char*, BOOL, ERRSTRUCT*);
typedef ERRSTRUCT(CALLBACK* LPFN1LONG3PCHAR1BOOL1PCHAR)(long, char*, char*, char*, BOOL, char*);
typedef void       (CALLBACK* LPPR1LONG4PCHAR)(long, char*, char*, char*, char*, ERRSTRUCT*);
typedef ERRSTRUCT(CALLBACK* LPFN1LONG4PCHAR)(long, char*, char*, char*, char*);
typedef void       (CALLBACK* LPPR1LONG4PCHAR1BOOL)(long, char*, char*, char*, char*, BOOL, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1LONG6PCHAR)(long, char*, char*, char*, char*, char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1LONG7PCHAR)(long, char*, char*, char*, char*, char*, char*, char*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN2LONG)(long, long);
typedef void       (CALLBACK* LPPR2LONG)(long, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2LONG1PCHAR)(long, long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR3LONG)(long, long, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR3LONG1PCHAR)(long, long, long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR3LONG1PLONG1PINT)(long, long, long, long*, int*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN3LONG1DOUBLE2BOOL)(long, long, long, double, BOOL, BOOL);
typedef void       (CALLBACK* LPPR3LONG2PCHAR1BOOL1PINT)(long, long, long, char*, char*, BOOL, int*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR4LONG)(long, long, long, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR4LONG2PCHAR2PLONG2PDOUBLE)(long, long, long, long, char*, char*, long*, long*, double*, double*);

typedef void       (CALLBACK* LPPR1PLONG5PCHAR)(long*, char*, char*, char*, char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2PLONG)(long*, long*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PLONG1LONG)(long*, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PLONG2PCHAR)(long*, char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PLONG2PCHAR1LONG)(long*, char*, char*, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PLONG1PCHAR)(long*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PLONG1PCHAR2LONG)(long*, char*, long, long, ERRSTRUCT*);

typedef int        (CALLBACK* LPFN1PCHAR)(char*);
typedef void       (CALLBACK* LPPR1PCHAR)(char*, ERRSTRUCT*);
typedef void       (CALLBACK* LP2PR1PCHAR)(char*);
typedef int        (CALLBACK* LPFN1PCHAR1INT)(char*, int);
typedef int        (CALLBACK* LPFN1PCHAR1PLONG)(char*, long*);
typedef void       (CALLBACK* LPPR1PCHAR1LONG)(char*, long, ERRSTRUCT*);
typedef ERRSTRUCT(CALLBACK* LPFN1PCHAR1INT1LONG1BOOL)(char*, int, long, BOOL);
typedef void       (CALLBACK* LPPR1PCHAR2PINT1INT)(char*, int*, int*, int, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PCHAR1LONG2PCHAR)(char*, long, char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PCHAR1INT4PHAR1LONG)(char*, int, char*, char*, char*, char*, long, long, ERRSTRUCT*);
typedef double     (CALLBACK* LPFN1PCHAR3LONG)(char*, long, long, long);
typedef void       (CALLBACK* LPPR1PCHAR2LONG)(char*, long, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR1PCHAR3LONG1PCHAR)(char*, long, long, long, char*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN2PCHAR)(char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2PCHAR)(char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2PCHAR1PINT1PCHAR)(char*, char*, int*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2PCHAR1LONG1PDOUBLE1PLONG)(char*, char*, long, double*, long*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2PCHAR3LONG1PCHAR)(char*, char*, long, long, long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR2PCHAR4LONG1PCHAR)(char*, char*, long, long, long, long, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR3PCHAR)(char*, char*, char*, ERRSTRUCT*);
typedef int        (CALLBACK* LPFN3PCHAR)(char*, char*, char*);
typedef void       (CALLBACK* LPPR3PCHAR1LONG)(char*, char*, char*, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR3PCHAR1LONG1BOOL)(char*, char*, char*, long, BOOL, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR3PCHAR1LONG1INT)(char*, char*, char*, long, int, ERRSTRUCT*);
typedef ERRSTRUCT(CALLBACK* LPFN3PCHAR1LONG1PCHAR)(char*, char*, char*, long, char*);
typedef void       (CALLBACK* LPPR5PCHAR)(char*, char*, char*, char*, char*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR5PCHAR1LONG)(char*, char*, char*, char*, char*, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR5PCHAR2LONG)(char*, char*, char*, char*, char*, long, long, ERRSTRUCT*);
typedef void       (CALLBACK* LPPRR6PCHAR1PLONG)(char*, char*, char*, char*, char*, char*, long*, ERRSTRUCT*);
typedef void       (CALLBACK* LPPR8PCHAR1PLONG)(char*, char*, char*, char*, char*, char*, char*, char*, long*, ERRSTRUCT*);

typedef double     (CALLBACK* LPFN1DOUBLE3PCHAR1LONGINT)(double, char*, char*, char*, long, int);
typedef void       (CALLBACK* LPPR5PDBL5PCHAR1LONG)(double*, double*, double*, double*, double*, char*, char*, char*, char*, char*, long, ERRSTRUCT*);

typedef int        (CALLBACK* LPFNVOID)();
typedef double     (CALLBACK* LPFN2VOID)();
typedef void       (CALLBACK* LPPRVOID)();

// date functions
typedef int        (CALLBACK* LPFNRMDYJUL)(short[], long*);
typedef int        (CALLBACK* LPFNRJULMDY)(long, short[]);
typedef int        (CALLBACK* LPFNNEWDATE)(long, BOOL, int, int, int, long*);
typedef int        (CALLBACK* LPFNISITAMARKETHOLIDAY)(long, char*);

// errstruct function
typedef void       (CALLBACK* LPPRERRSTRUCT)(ERRSTRUCT*);

// printerror function
typedef ERRSTRUCT(CALLBACK* LPFNPRINTERROR)(char*, int, long, char*, int, int, int, char*, BOOL);

// roll function
typedef ERRSTRUCT(CALLBACK* LPFNROLL)(int, char*, char*, long, long, char*, char*, BOOL, int, long, BOOL);

// yield helpers
typedef double     (CALLBACK* LPFNYIELD)(double, char*, char*, char*, long, int);
typedef void       (CALLBACK* LPFNSELECTNEXTCALLDATEPRICE)(char*, char*, double*, long*, ERRSTRUCT*);

// -----------------------------------------------------------------------------
// Shared function prototypes (implemented in OLEDBIOCommon.cpp)
// -----------------------------------------------------------------------------
DLLAPI void      STDCALL InitializeErrStruct(ERRSTRUCT* pzErr);
DLLAPI ERRSTRUCT STDCALL PrintError(
    const char* sErrMsg,
    int iID,
    long lRecNo,
    const char* sRecType,
    int iBusinessErr,
    int iSqlErr,
    int iIsamCode,
    const char* sMsgIdentifier,
    BOOL bWarning);

// Legacy common function prototypes (if used elsewhere)
BOOL   IsValueZero(double fValue, unsigned short iPrecision);
double TruncateDouble(double fNumber, int iPrecision);
double RoundDouble(double fNumber, int iPrecision);

