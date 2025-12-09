/**
 * SUB-SYSTEM: Database Input/Output
 * FILENAME: OLEDBIOCommon.h
 * DESCRIPTION: Defines common variables, constants, etc. used in OLEDBIO.DLL
 *              for re-use in dependent modules. Refactored for ODBC (2025):
 * Removed OLE DB/ATL deps; RAII focus, std::string for SQL, preserved AdjustSQL
 * for dynamic tables. PUBLIC FUNCTION(S): Helpers for ERRSTRUCT, AdjustSQL,
 * etc. NOTES: No COM/ATL; errors via exceptions or ERRSTRUCT. USAGE: Part of
 * ODBCIO.DLL project (renamed from OLEDB.DLL). AUTHOR: Original: Valeriy
 * Yegorov. (C) 2001 Effron Enterprises, Inc.; Refactored: 2025
 **/

// History (updated):
// 2025-11-09 ODBC Migration: Remove CDataSource/CSession/CDBPropSet;
// std::string sSQL; nanodbc compat 2008-08-20 Updated for approles - yb
// 2007-12-04 Change to adjustsql() to replace all occurrences of %table%, not
// just first - yb. 09/04/2001 Overloaded AdjustSQL function - vay.

#pragma once                    // Modern: replaces #ifndef OLEDBIOCOMMON
#pragma warning(disable : 4995) // Secure funcs
#pragma warning(disable : 4244) // Type conversions

#include <cstdint> // For types
#include <memory>
#include <string>
#include <vector> // For potential param lists


// Includes (adapt paths; assume commonheader.h has NT, STR80LEN etc.)
#include "ODBCErrorChecking.h" // Refactor to ODBCErrorChecking.h? Keep for now, remove OLE DB specifics
#include "SysSettings.h"
#include "commonheader.h"


// Typedefs for callbacks (preserved; extern "C" for DLL)
extern "C" {
//    typedef int (CALLBACK* LPFNGETUSERANDPASSWORD)(char*, char*, char*, int*,
//    bool);
// typedef void (CALLBACK* LPPRGETODBCDSNANDDBNAME)(char*, char*);
typedef void(CALLBACK *LPPRGETCONNECTSTR)(char *, char *);
//    typedef void (CALLBACK* LPFNGETAPPROLENAMEANDPASSWORD)(char*, char*,
//    char*, char*, char*); typedef int (CALLBACK* LPFN1INT)(int);  // lpfnTimer
//    typedef void (CALLBACK* LP2PR1PCHAR)(char*, char*);  // lpprTimerResult
}

// Constants (preserved)
#define HOLDMAPNAMESIZE 20
#define MAXSQLSIZE 24576 // Max SQL buffer

// Common globals (thread_local for multi-thread; adjust if single-threaded)
extern std::string sErrMsg;
extern char sUser[64 + NT];
extern std::string sSQL; // Changed: std::string vs WCHAR* (UTF-8 friendly)

// Table names (preserved; for historical/adhoc tables)
extern char sHoldings[STR80LEN];
extern char sHoldcash[STR80LEN];
extern char sPayrec[STR80LEN];
extern char sHedgxref[STR80LEN];
extern char sPortmain[STR80LEN];
extern char sPortbal[STR80LEN];
extern char sHoldtot[STR80LEN];

// Forward declaration
namespace nanodbc {
class connection;
}
nanodbc::connection &GetDBConnection();

extern CErrorChecking dbErr; // Or static func

extern SYSSETTING zSyssetng; // From syssettings.h
extern bool bPMainIsValid;
extern bool bAssetIsValid;

// Callback externs (preserved)
extern LPFN1INT lpfnTimer;
extern LP2PR1PCHAR lpprTimerResult;

// Helpers (preserved/adapted)
BOOL Char2BOOL(char *c); // 'Y'/'N' -> true/false

// String conversions (preserved; for legacy WCHAR <-> CHAR)
void wtoc(char *Dest, const wchar_t *Source);
void ctow(wchar_t *Dest, const char *Source);

// Basic query class (simplified: No OLE DB; for AdjustSQL only)
class CQuery {
public:
  bool m_bPrepared;
  int m_cRows;
  char m_sAdjSQL[MAXSQLSIZE]; // Buffer for adjusted SQL

  CQuery() : m_bPrepared(false), m_cRows(0) { m_sAdjSQL[0] = '\0'; }

  // Replace %table% placeholders (overloads preserved; now handles multiple)
  void AdjustSQL(const char *sOldSQL, char *sTblName);
  void AdjustSQL(const wchar_t *sOldSQL, char *sTblName);
  void AdjustSQL(const wchar_t *sOldSQL, char *sDstTblName, char *sSrcTblName);
  void AdjustSQL(const char *sOldSQL, char *sDstTblName, char *sSrcTblName);

  // New: std::string version for modern use
  void AdjustSQL(const std::string &sOldSQL, const std::string &sTblName,
                 std::string &outSQL);
};

// BLOB macros removed: Not needed for ODBC/nanodbc (use bind for blobs/streams)
// #define _BLOB_ENTRY_CODE_EX ...  // Use nanodbc::bind for IUnknown/blobs

// Inline impls for AdjustSQL (move to .cpp if large; here for header-only)
inline void CQuery::AdjustSQL(const char *sOldSQL, char *sTblName) {
  std::string sql(sOldSQL);
  size_t pos = 0;
  while ((pos = sql.find("%", pos)) != std::string::npos) {
    size_t end = sql.find("%", pos + 1);
    if (end != std::string::npos) {
      sql.replace(pos, end - pos + 1, sTblName);
      pos += strlen(sTblName);
    } else {
      break;
    }
  }
  strncpy_s(m_sAdjSQL, sizeof(m_sAdjSQL), sql.c_str(), sql.length());
}

inline void CQuery::AdjustSQL(const std::string &sOldSQL,
                              const std::string &sTblName,
                              std::string &outSQL) {
  outSQL = sOldSQL;
  size_t pos = 0;
  while ((pos = outSQL.find("%", pos)) != std::string::npos) {
    size_t end = outSQL.find("%", pos + 1);
    if (end != std::string::npos) {
      outSQL.replace(pos, end - pos + 1, sTblName);
      pos += sTblName.length();
    } else {
      break;
    }
  }
}

// Other overloads: Implement similarly (strtok_s -> string replace for all
// occurrences)
inline void CQuery::AdjustSQL(const char *sOldSQL, char *sDstTblName,
                              char *sSrcTblName) {
  // Similar to single, but replace two % pairs
  std::string sql(sOldSQL);
  // Assume pattern %dst%...%src% - adjust logic
  size_t pos1 = sql.find("%");
  if (pos1 != std::string::npos) {
    size_t end1 = sql.find("%", pos1 + 1);
    if (end1 != std::string::npos)
      sql.replace(pos1, end1 - pos1 + 1, sDstTblName);
  }
  size_t pos2 = sql.find("%", pos1); // After first
  if (pos2 != std::string::npos) {
    size_t end2 = sql.find("%", pos2 + 1);
    if (end2 != std::string::npos)
      sql.replace(pos2, end2 - pos2 + 1, sSrcTblName);
  }
  strncpy_s(m_sAdjSQL, sizeof(m_sAdjSQL), sql.c_str(), sql.length());
}

// WCHAR overloads: Convert to char first
inline void CQuery::AdjustSQL(const wchar_t *sOldSQL, char *sTblName) {
  std::unique_ptr<char[]> temp(new char[MAXSQLSIZE]());
  size_t converted = 0;
  wcstombs_s(&converted, temp.get(), MAXSQLSIZE, sOldSQL, wcslen(sOldSQL));
  AdjustSQL(temp.get(), sTblName);
}

inline void CQuery::AdjustSQL(const wchar_t *sOldSQL, char *sDstTblName,
                              char *sSrcTblName) {
  std::unique_ptr<char[]> temp(new char[MAXSQLSIZE]());
  size_t converted = 0;
  wcstombs_s(&converted, temp.get(), MAXSQLSIZE, sOldSQL, wcslen(sOldSQL));
  AdjustSQL(temp.get(), sDstTblName, sSrcTblName);
}
