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

  // Login.dll dependency removed.
  // The provided alias is treated as the connection string or DSN.
  if (sAlias) {
    strncpy_s(sConnectStr, 512, sAlias, _TRUNCATE);
  } else {
    sConnectStr[0] = '\0';
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

  // Login.dll dependency removed.
  // We assume the caller handles DSN/DB config or this function is deprecated.
  // For now, return empty strings to indicate no legacy info.
  sODBCDSN[0] = '\0';
  sSQLDBName[0] = '\0';
}
