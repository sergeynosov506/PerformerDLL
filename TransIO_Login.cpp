/**
 * SUB-SYSTEM: Login.DLL Integration
 * FILENAME: TransIO_Login.cpp
 * DESCRIPTION: Login.DLL function implementations and wrappers
 * AUTHOR: Modernized 2025-11-29
 */

#include "TransIO_Login.h"
#include "OLEDBIOCommon.h"
#include "commonheader.h"
#include <Windows.h>

// Global function pointer for GetUserAndPassword (loaded from Login.DLL)
LPFNGetUserAndPassword lpfnGetUserAndPassword = nullptr;

// Module handle for Login.DLL
static HMODULE hLoginDLL = NULL;

/**
 * GetConnectionString - Retrieves connection string from Login.DLL
 * This is a wrapper that calls into Login.DLL to get the database connection
 * string
 */
void GetConnectionString(char *sAlias, char *sConnectStr, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!sConnectStr) {
    pzErr->iSqlError = -1;
    PrintError((char *)"Invalid parameter (sConnectStr is NULL)", 0, 0,
               (char *)"", 0, -1, 0, (char *)"GetConnectionString", FALSE);
    return;
  }

  sConnectStr[0] = '\0';

  // Try to load Login.DLL if not already loaded
  if (!hLoginDLL) {
    hLoginDLL = LoadLibraryA("Login.dll");
    if (!hLoginDLL) {
// Login.DLL not found - this is acceptable; we'll use fallback
#ifdef DEBUG
      PrintError(
          (char *)"Login.dll not found, using fallback connection method", 0, 0,
          (char *)"", 0, 0, 0, (char *)"GetConnectionString", FALSE);
#endif
      return;
    }

    // Get function pointer for GetUserAndPassword
    lpfnGetUserAndPassword =
        (LPFNGetUserAndPassword)GetProcAddress(hLoginDLL, "GetUserAndPassword");
  }

  // Try to get connection string from Login.DLL if available
  if (hLoginDLL) {
    // Try GetConnectionString function from Login.DLL
    typedef void(CALLBACK * LPFNGetConnectionString)(char *, char *);
    LPFNGetConnectionString pfnGetConnStr =
        (LPFNGetConnectionString)GetProcAddress(hLoginDLL,
                                                "GetConnectionStringC");

    if (pfnGetConnStr) {
      pfnGetConnStr(sAlias, sConnectStr);
    }
  }
}

/**
 * GetODBCInfo - Retrieves ODBC DSN and database name
 * This is a wrapper for legacy code that may use DSN-based connections
 */
void GetODBCInfo(char *sODBCDSN, char *sSQLDBName, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!sODBCDSN || !sSQLDBName) {
    pzErr->iSqlError = -1;
    PrintError((char *)"Invalid parameters", 0, 0, (char *)"", 0, -1, 0,
               (char *)"GetODBCInfo", FALSE);
    return;
  }

  sODBCDSN[0] = '\0';
  sSQLDBName[0] = '\0';

  // Try to load Login.DLL if not already loaded
  if (!hLoginDLL) {
    hLoginDLL = LoadLibraryA("Login.dll");
    if (!hLoginDLL) {
#ifdef DEBUG
      PrintError((char *)"Login.dll not found for ODBC info", 0, 0, (char *)"",
                 0, 0, 0, (char *)"GetODBCInfo", FALSE);
#endif
      return;
    }
  }

  // Try to get ODBC info from Login.DLL
  if (hLoginDLL) {
    typedef void(CALLBACK * LPFNGetODBCDSNAndDBName)(char *, char *);
    LPFNGetODBCDSNAndDBName pfnGetODBCInfo =
        (LPFNGetODBCDSNAndDBName)GetProcAddress(hLoginDLL,
                                                "GetODBCDSNAndDBName");

    if (pfnGetODBCInfo) {
      pfnGetODBCInfo(sODBCDSN, sSQLDBName);
    }
  }
}
