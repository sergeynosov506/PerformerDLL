/**
 * SUB-SYSTEM: Database Input/Output
 * FILENAME: ODBCErrorChecking.h
 * DESCRIPTION:
 *   Provides diagnostics for ODBC operations and nanodbc exceptions.
 *   Replaces old COM-based OLE DB error system with portable ODBC SQLSTATE
 * logic. AUTHOR: Original: Valeriy Yegorov (2001) Modern Refactor: Grok/xAI
 * (2025)
 */

#pragma once
#pragma warning(disable : 4996)

#include "commonheader.h"
#include "nanodbc/nanodbc.h"
#include <sql.h>
#include <sqlext.h>
#include <sstream>
#include <string>
#include <windows.h>


class CErrorChecking {
public:
  CErrorChecking() = default;
  ~CErrorChecking() = default;

  // ------------------------------------------------------------
  // Display all errors associated with an ODBC handle
  // ------------------------------------------------------------
  void DisplayAllErrors(SQLHANDLE handle, SQLSMALLINT handleType,
                        const char *context = nullptr);

  // ------------------------------------------------------------
  // Converts SQLSTATE/nanodbc errors into a descriptive message
  // ------------------------------------------------------------
  void HandleNanodbcError(const nanodbc::database_error &e,
                          const char *functionName, ERRSTRUCT *perr);

  // ------------------------------------------------------------
  // Helper to print a single SQL error message
  // ------------------------------------------------------------
  static void GetSQLErrorMessage(SQLINTEGER nativeError,
                                 const std::string &sqlState, std::string &msg);

private:
  void LogMessage(const std::string &fullMsg, const char *functionName,
                  ERRSTRUCT *perr);
};

// Global helper for safe string binding (replaces local lambdas)
// Note: We include nanodbc.h to allow inline implementation.
// Usage: safe_bind_string(stmt, idx, sVal);
inline void safe_bind_string(nanodbc::statement &stmt, int &idx,
                             const char *src) {
  // Bind the source string if valid, otherwise bind empty string.
  // nanodbc binds by pointer, so src must remain valid until execute().
  // We do NOT use a local buffer to avoid dangling pointers.
  stmt.bind(idx++, src ? src : "");
}
