/**
 * FILENAME: OLEDBIO.cpp
 * DESCRIPTION:
 *   Main entry for OLEDBIO.DLL – modernized ODBC/nanodbc backend.
 *   Replaces legacy OLE DB with ODBC (Driver 18+).
 * AUTHOR: Valeriy Yegorov (C) 2003; Refactor: Grok/xAI (2025)
 */
#include "OLEDBIO.h"
#include "ODBCErrorChecking.h"
#include "SysSettings.h"
#include "TransIO_Holdmap.h"
#include "TransIO_Login.h"
#include "TransIO_SysSettings.h"
#include "assets.h"
#include "commonheader.h"
#include "nanodbc\nanodbc.h"
#include "portmain.h"
#include "sectype.h"
#include "subacct.h"
#include "trantype.h"
#include <iostream>
#include <string>
#include <windows.h>

#pragma managed(push, off)
#pragma comment(lib, "odbc32.lib")

// ----------------------------------------------------------
// Internal globals
// ----------------------------------------------------------
static bool gInitialized = false;
static long gTransCount = 0;
thread_local nanodbc::connection gConn;
static CErrorChecking gErr;

// ----------------------------------------------------------
// Helper: maps nanodbc exception to ERRSTRUCT
// ----------------------------------------------------------
static int MapDbError(const nanodbc::database_error &e, ERRSTRUCT *perr,
                      const char *where) {
  if (perr)
    InitializeErrStruct(perr);

  std::string msg = "[ODBC] Exception: ";
  msg += e.what();

  if (perr) {
    perr->iSqlError = -1;
    perr->iBusinessError = 0;
    perr->iIsamCode = 0;
    if (where && *where)
      strncpy_s(perr->sRecType, where, _TRUNCATE);
    PrintError(const_cast<char *>(msg.c_str()), 0, 0, const_cast<char *>("E"),
               perr->iBusinessError, perr->iSqlError, perr->iIsamCode,
               const_cast<char *>(where ? where : "DBERR"), FALSE);
  } else {
    PrintError(const_cast<char *>(msg.c_str()), 0, 0, const_cast<char *>("E"),
               0, -1, 0, const_cast<char *>(where ? where : "DBERR"), FALSE);
  }
  return -1;
}

HINSTANCE LoadLibrarySafe(LPCTSTR libFileName) {
  HINSTANCE hInst = LoadLibrary(libFileName);
  if (!hInst) {
    char msg[260];
    sprintf_s(msg, "Failed to load library: %ls", libFileName);
    PrintError(msg, 0, 0, (char *)"", 0, GetLastError(), 0,
               (char *)"LoadLibrarySafe", FALSE);
  }
  return hInst;
}

// Helper to detect best installed ODBC driver
static std::string GetBestSQLDriver() {
  const char *drivers[] = {"ODBC Driver 18 for SQL Server",
                           "ODBC Driver 17 for SQL Server",
                           "ODBC Driver 13 for SQL Server"};

  HKEY hKey = NULL;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers", 0, KEY_READ,
                    &hKey) == ERROR_SUCCESS) {
    for (const char *drv : drivers) {
      if (RegQueryValueExA(hKey, drv, nullptr, nullptr, nullptr, nullptr) ==
          ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return std::string("{") + drv + "}";
      }
    }
    RegCloseKey(hKey);
  }
  return "{ODBC Driver 18 for SQL Server}";
}

// ----------------------------------------------------------
// DLL Export: Initialize ODBC connection
// ----------------------------------------------------------
// ---- helper: adapt legacy OLE DB conn string to ODBC Driver 18/17 ----
static std::string AdaptToODBC(std::string oleStr) {
  // Strip any "Provider=..." segment (SQLOLEDB / MSOLEDBSQL.19 / etc.)
  size_t p = oleStr.find("Provider=");
  if (p != std::string::npos) {
    size_t endProv = oleStr.find(';', p);
    oleStr.erase(
        p, (endProv == std::string::npos ? oleStr.size() : endProv + 1) - p);
  }

  auto pick = [&](const char *key) -> std::string {
    std::string k(key);
    size_t pos = oleStr.find(k);
    if (pos == std::string::npos)
      return {};
    pos += k.size();
    size_t end = oleStr.find(';', pos);
    return oleStr.substr(pos, (end == std::string::npos ? oleStr.size() : end) -
                                  pos);
  };

  // Try common OLE DB keys
  std::string server = pick("Data Source=");
  if (server.empty())
    server = pick("Server="); // some apps already pass ODBC-ish
  std::string db = pick("Initial Catalog=");
  if (db.empty())
    db = pick("Database=");
  std::string uid = pick("User ID=");
  if (uid.empty())
    uid = pick("UID=");
  std::string pwd = pick("Password=");
  if (pwd.empty())
    pwd = pick("PWD=");
  std::string integratedSec = pick("Integrated Security=");

  // If a DSN was passed in legacy path (rare), keep it
  std::string dsn = pick("DSN=");

  // Build ODBC Driver 18 connection string
  std::string odbc;
  if (!dsn.empty()) {
    odbc = "DSN=" + dsn + ";";
    if (!db.empty())
      odbc += "Database=" + db + ";";
    if (!uid.empty())
      odbc += "UID=" + uid + ";";
    if (!pwd.empty())
      odbc += "PWD=" + pwd + ";";
  } else {
    // Dynamic driver detection
    std::string driver = "Driver=" + GetBestSQLDriver() + ";";
    odbc = driver;
    if (!server.empty())
      odbc += "Server=" + server + ";";
    if (!db.empty())
      odbc += "Database=" + db + ";";

    // Handle Trusted Connection / Integrated Security
    if (_stricmp(integratedSec.c_str(), "SSPI") == 0) {
      odbc += "Trusted_Connection=yes;";
    } else {
      if (!uid.empty())
        odbc += "UID=" + uid + ";";
      if (!pwd.empty())
        odbc += "PWD=" + pwd + ";";
    }
  }

  // Safe defaults for common lab/dev installs. Adjust as needed.
  // If you use trusted certs in prod, switch to
  // Encrypt=yes;TrustServerCertificate=no;
  if (odbc.find("Encrypt=") == std::string::npos)
    odbc += "Encrypt=no;";
  if (odbc.find("TrustServerCertificate=") == std::string::npos)
    odbc += "TrustServerCertificate=yes;";

  // Keep a reasonable timeout
  if (odbc.find("Connection Timeout=") == std::string::npos)
    odbc += "Connection Timeout=30;";

  return odbc;
}

// If your nanodbc connection is defined in another translation unit, keep this
// extern. Otherwise, if it's here, remove extern and make it a real definition.
// ---- Global holdmap table name buffers ----
char sHoldings[STR80LEN] = {0};
char sHoldcash[STR80LEN] = {0};
char sPortmain[STR80LEN] = {0};
char sPortbal[STR80LEN] = {0};
char sPayrec[STR80LEN] = {0};
char sHedgxref[STR80LEN] = {0};
char sHoldtot[STR80LEN] = {0};

char sDBAlias[256] = "";
char sUser[64 + NT] = {0};

SYSSETTING zSyssetng;
PORTMAIN zSavedPMain;
ASSETS zSavedAsset;
SECTYPETABLE zSTable;

bool bPMainIsValid = false;
bool bAssetIsValid = false;

SUBACCTTABLE1 zSATable;
TRANTYPETABLE1 zTTable;

// ----------------------------------------------------------
// AppRole Logic (Ported from Login.dll / LoginCOMUnit.pas)
// ----------------------------------------------------------
static bool NeedToInvokeAppRole(nanodbc::connection &conn) {
  try {
    // Check if current user is 'SYS' (System User) in usersdef table
    // Delphi: Select * from usersdef where user_id = 'UserID'
    // We use SYSTEM_USER (or USER_NAME()) to get current ID
    nanodbc::result results = nanodbc::execute(
        conn, "SELECT InternalUser FROM usersdef WHERE user_id = USER_NAME()");

    if (results.next()) {
      std::string internalUser = results.get<std::string>(0);
      // If InternalUser is 'SYS', we do NOT invoke AppRole
      if (_stricmp(internalUser.c_str(), "SYS") == 0) {
        return false;
      }
    }
    // Default to true if user not found or not 'SYS'
    return true;
  } catch (...) {
    // If table doesn't exist or error, fail safe (invoke role?) or skip?
    // Delphi code defaults to TRUE on exception.
    return true;
  }
}

static void InvokeAppRole(nanodbc::connection &conn) {
  fprintf(stderr, "!!! AppRole: Checking if needed !!!\n");
  if (!NeedToInvokeAppRole(conn)) {
    fprintf(stderr, "!!! AppRole: Not needed (InternalUser = SYS) !!!\n");
    return;
  }

  try {
    fprintf(stderr, "!!! AppRole: Querying appRole table !!!\n");
    // Get Role and Password
    // Delphi: Select Role, Password from appRole
    nanodbc::result results =
        nanodbc::execute(conn, "SELECT Role, Password FROM appRole");

    if (results.next()) {
      std::string role = results.get<std::string>(0);
      std::string pwd = results.get<std::string>(1);

      if (!role.empty() && role != "NO") {
        fprintf(stderr, "!!! AppRole: Invoking role %s !!!\n", role.c_str());
        char sql[1024];
        sprintf_s(
            sql,
            "IF (SELECT USER_NAME()) <> '%s' EXEC sp_setapprole '%s', '%s'",
            role.c_str(), role.c_str(), pwd.c_str());

        nanodbc::execute(conn, sql);
        fprintf(stderr, "!!! AppRole: Successfully invoked role %s !!!\n",
                role.c_str());
      }
    } else {
      fprintf(stderr,
              "!!! AppRole: No role found in appRole table for OLEDBIO !!!\n");
    }
  } catch (const std::exception &e) {
    fprintf(stderr, "!!! AppRole Error: %s !!!\n", e.what());
#ifdef DEBUG
    PrintError((char *)e.what(), 0, 0, (char *)"W", 0, -1, 0,
               (char *)"InvokeAppRole", FALSE);
#endif
  }
}

DLLAPI void STDCALL InitializeOLEDBIO(char *sAlias, char *sMode, char *sType,
                                      long lAsofDate, int iPrepareWhat,
                                      ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError(const_cast<char *>("Entering"), 0, 0, const_cast<char *>(""), 0, 0,
             0, const_cast<char *>("InitializeOLEDBIO"), FALSE);
#endif

  InitializeErrStruct(pzErr);

  // If reinitializing with a new alias, free first
  if (gInitialized && (_stricmp(sAlias, sDBAlias) != 0))
    FreeOLEDBIO();

  // Only perform load/init once per process
  if (!gInitialized) {
    // --- Build connection string via Login.DLL (legacy path), then adapt to
    // ODBC ---
    char sConnectStr[512] = {0};
    GetConnectionString(sAlias, sConnectStr, pzErr);

    try {
      if (sConnectStr[0]) {
        // Convert legacy OLE DB string (or DSN) to ODBC Driver 18
        std::string odbcStr;
        if (strchr(sConnectStr, '=') == nullptr) {
          // No key-value pairs? Assume it is a DSN.
          odbcStr = std::string("DSN=") + sConnectStr + ";";
        } else {
          odbcStr = AdaptToODBC(sConnectStr);
        }
        fprintf(stderr, "!!! Connecting to: %s !!!\n", odbcStr.c_str());
        fflush(stderr);
        gConn = nanodbc::connection(odbcStr);
        // Verify connection with a simple query (throws if invalid)
        nanodbc::execute(gConn, "SELECT 1");
      } else {
        // Strict Mode: If alias was provided but no connection string found ->
        // ERROR. Do NOT fallback to default DSN.
        pzErr->iSqlError = -1;
        char sMsg[256];
        sprintf_s(sMsg, "No connection string found for alias: %s",
                  sAlias ? sAlias : "(null)");
        fprintf(stderr, "!!! FAILED: %s !!!\n", sMsg);
        fflush(stderr);
        PrintError(sMsg, 0, 0, const_cast<char *>(""), 0, pzErr->iSqlError, 0,
                   const_cast<char *>("INITOLEDBIO2c"), FALSE);
        return;
      }

      // Login.dll dependency removed. User/Password must be in DSN or
      // ConnectionString. Legacy 'GetUserAndPassword' call removed.
    } catch (const nanodbc::database_error &e) {
      pzErr->iSqlError = -1;
      fprintf(stderr, "!!! DB ERROR: %s !!!\n", e.what());
      fflush(stderr);
      PrintError(const_cast<char *>(e.what()), 0, 0, const_cast<char *>("E"), 0,
                 pzErr->iSqlError, 0, const_cast<char *>("INITOLEDBIO_CONNECT"),
                 FALSE);
      return;
    }

    // Mark initialized and cache alias
    gInitialized = true;
    strcpy_s(sDBAlias, sAlias);

    // --- Invoke AppRole (Legacy Security) ---
    InvokeAppRole(gConn);
  }

  // --- Holdmap init (unchanged behavior) ---
  *pzErr = InitHoldmap(lAsofDate);
  if (pzErr->iSqlError || pzErr->iBusinessError) {
    FreeOLEDBIO();
    return;
  }

  bAssetIsValid = false;
  bPMainIsValid = false;
}

DLLAPI void STDCALL SetTransCount(long lCnt) { return; }

DLLAPI long STDCALL GetTransCount(void) { return 0; }

// ----------------------------------------------------------
// DLL Export: transaction management
// ----------------------------------------------------------
DLLAPI int STDCALL StartDBTransaction(void) {
  if (!gInitialized)
    return -1;
  try {
    nanodbc::execute(gConn, "BEGIN TRANSACTION;");
    ++gTransCount;
    return 0;
  } catch (const nanodbc::database_error &e) {
    MapDbError(e, nullptr, "StartDBTransaction");
    return -1;
  }
}

DLLAPI int STDCALL CommitDBTransaction(void) {
  if (!gInitialized)
    return -1;
  try {
    nanodbc::execute(gConn, "COMMIT TRANSACTION;");
    if (gTransCount > 0)
      --gTransCount;
    return 0;
  } catch (const nanodbc::database_error &e) {
    MapDbError(e, nullptr, "CommitDBTransaction");
    return -1;
  }
}

DLLAPI int STDCALL RollbackDBTransaction(void) {
  if (!gInitialized)
    return -1;
  try {
    nanodbc::execute(gConn, "ROLLBACK TRANSACTION;");
    if (gTransCount > 0)
      --gTransCount;
    return 0;
  } catch (const nanodbc::database_error &e) {
    MapDbError(e, nullptr, "RollbackDBTransaction");
    return -1;
  }
}

DLLAPI int STDCALL AbortDBTransaction(BOOL bInTrans) { return 0; }

// ----------------------------------------------------------
// DLL Export: cleanup
// ----------------------------------------------------------
DLLAPI void STDCALL FreeOLEDBIO(void) {
  if (gInitialized) {
    try {
      gConn.disconnect();
    } catch (...) {
    }
    gInitialized = false;
    gTransCount = 0;
  }
}
DLLAPI void STDCALL InitHoldmapCS(long lAsofDate, ERRSTRUCT *pzErr) {
  *pzErr = InitHoldmap(lAsofDate);
  return;
}

DLLAPI ERRSTRUCT STDCALL InitHoldmap(long lAsofDate) {
  ERRSTRUCT zErr;
  InitializeErrStruct(&zErr);
  zTTable.iNumTType = 0;

  try {
    // --- Step 1: Load system settings ---
    SelectSyssettings(&zSyssetng, &zErr);
    if (zErr.iSqlError != 0)
      return zErr;

    // --- Step 2: Determine holdmap table names based on as-of-date ---
    if (lAsofDate == DEFAULT_DATE) {
      strcpy_s(sHoldings, "holdings");
      strcpy_s(sHoldcash, "holdcash");
      strcpy_s(sPortmain, "portmain");
      strcpy_s(sPortbal, "portbal");
      strcpy_s(sPayrec, "payrec");
      strcpy_s(sHedgxref, "hedgxref");
      strcpy_s(sHoldtot, "holdtot");
    } else if (lAsofDate == ADHOC_DATE) {
      strcpy_s(sHoldings, "hdadhoc");
      strcpy_s(sHoldcash, "hcadhoc");
      strcpy_s(sPortmain, "pmadhoc");
      strcpy_s(sPortbal, "pbadhoc");
      strcpy_s(sPayrec, "pradhoc");
      strcpy_s(sHedgxref, "hxadhoc");
      strcpy_s(sHoldtot, "htadhoc");
    } else if (lAsofDate == PERFORM_DATE) {
      strcpy_s(sHoldings, "hdperf");
      strcpy_s(sHoldcash, "hcperf");
      strcpy_s(sPortmain, "pmperf");
      strcpy_s(sPortbal, "pbperf");
      strcpy_s(sPayrec, "prperf");
      strcpy_s(sHedgxref, "hxperf");
      strcpy_s(sHoldtot, "htperf");
    } else if (lAsofDate == SETTLEMENT_DATE) {
      strcpy_s(sHoldings, "hdstlmnt");
      strcpy_s(sHoldcash, "hcstlmnt");
      strcpy_s(sPortmain, "pmstlmnt");
      strcpy_s(sPortbal, "pbstlmnt");
      strcpy_s(sPayrec, "prstlmnt");
      strcpy_s(sHedgxref, "hxstlmnt");
      strcpy_s(sHoldtot, "htstlmnt");
    } else {
      // --- Step 3: Load from DB holdmap table using ODBC ---
      SelectHoldmap(lAsofDate, sHoldings, sHoldcash, sPortmain, sPortbal,
                    sPayrec, sHedgxref, sHoldtot, &zErr);

      if (zErr.iSqlError != 0) {
        char buf[20];
        _itoa_s(lAsofDate, buf, 10);
        char msg[128];
        sprintf_s(msg, "Error Reading Holdmap for %s", buf);
        zErr = PrintError(msg, 0, 0, (char *)"", 0, zErr.iSqlError, 0,
                          (char *)"INITOLEDB4", FALSE);
      }
    }
  } catch (const nanodbc::database_error &e) {
    // Log ODBC failure
    zErr.iSqlError = -1;
    PrintError((char *)e.what(), 0, 0, (char *)"H", 0, zErr.iSqlError, 0,
               (char *)"InitHoldmap", FALSE);
  } catch (...) {
    zErr.iSqlError = -1;
    PrintError((char *)"Unexpected exception in InitHoldmap", 0, 0, (char *)"",
               0, zErr.iSqlError, 0, (char *)"InitHoldmap", FALSE);
  }

  return zErr;
}
#pragma managed(pop)
nanodbc::connection &GetDBConnection() { return gConn; }
