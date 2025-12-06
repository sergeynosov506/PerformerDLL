/**
* SUB-SYSTEM: Database Input/Output
* FILENAME: ODBCErrorChecking.h
* DESCRIPTION:
*   Provides diagnostics for ODBC operations and nanodbc exceptions.
*   Replaces old COM-based OLE DB error system with portable ODBC SQLSTATE logic.
* AUTHOR:
*   Original: Valeriy Yegorov (2001)
*   Modern Refactor: Grok/xAI (2025)
*/

#pragma once
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)

#include <string>
#include <sstream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "commonheader.h"

// Forward declaration to avoid including full nanodbc headers
namespace nanodbc {
    class database_error;
    class statement;
    class connection;
}

/**
 * @class CErrorChecking
 * @brief Provides diagnostic and error reporting utilities for ODBC/nanodbc.
 */
class CErrorChecking
{
public:
    CErrorChecking() = default;
    ~CErrorChecking() = default;

    // ------------------------------------------------------------
    // Display all errors associated with an ODBC handle
    // ------------------------------------------------------------
    void DisplayAllErrors(SQLHANDLE handle, SQLSMALLINT handleType, const char* context = nullptr);

    // ------------------------------------------------------------
    // Converts SQLSTATE/nanodbc errors into a descriptive message
    // ------------------------------------------------------------
    void HandleNanodbcError(const nanodbc::database_error& e,
        const char* functionName,
        ERRSTRUCT* perr);

    // ------------------------------------------------------------
    // Helper to print a single SQL error message
    // ------------------------------------------------------------
    static void GetSQLErrorMessage(SQLINTEGER nativeError, const std::string& sqlState, std::string& msg);

private:
    void LogMessage(const std::string& fullMsg, const char* functionName, ERRSTRUCT* perr);
};
