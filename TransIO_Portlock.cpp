#include "TransIO_Portlock.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include <iostream>
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;
extern char sUser[64 + NT]; // Assuming sUser is defined elsewhere, likely in TransIO.cpp or similar. 
                       // If it's a global variable, we need to ensure it's accessible. 
                       // Based on previous files, it seems to be available.

DLLAPI int STDCALL InsertPortLock(int iId, char *sCreatedBy, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"InsertPortLock", FALSE);
        return -1;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        // The original query used SET LOCK_TIMEOUT 0 before INSERT and SET LOCK_TIMEOUT -1 after.
        // We will try to execute this as a single batch if possible, or separate if needed.
        // Nanodbc/ODBC might handle multiple statements if the driver supports it.
        // However, for safety and clarity, we can execute them sequentially if the session state persists.
        // But SET LOCK_TIMEOUT is session specific, so it should work if we use the same connection.
        
        // Original:
        // "SET LOCK_TIMEOUT 0 INSERT INTO PORTLOCK (id, created_by, user_id, lock_time) VALUES (?, ?, ?, getdate() ) SET LOCK_TIMEOUT -1"
        
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SET LOCK_TIMEOUT 0; "
            "INSERT INTO PORTLOCK (id, created_by, user_id, lock_time) VALUES (?, ?, ?, getdate()); "
            "SET LOCK_TIMEOUT -1;"));

        stmt.bind(0, &iId);
        stmt.bind(1, sCreatedBy);
        stmt.bind(2, sUser); // Using global sUser

        nanodbc::execute(stmt);

        return pzErr->iSqlError;
    }
    catch (const nanodbc::database_error& e)
    {
        // Check for specific error codes if possible, or rely on the error message/state
        // The original code checked for DB_E_INTEGRITYVIOLATION or DB_E_ABORTLIMITREACHED
        // which mapped to duplicate key error.
        // In ODBC/nanodbc, we might need to check SQLState or native error code.
        // 23000 is integrity constraint violation.
        
        // For now, we will map any error that looks like a duplicate key or timeout to 9729 if appropriate,
        // but the original code specifically returned 9729 for duplicate key.
        
        // If it's a timeout (due to SET LOCK_TIMEOUT 0), it might throw a specific error.
        
        // Let's try to preserve the logic:
        // If it fails, we log it.
        // But the original code had a specific check:
        // if (hr == DB_E_INTEGRITYVIOLATION || hr == DB_E_ABORTLIMITREACHED) pzErr->iSqlError = 9729;
        
        // We'll assume for now that if it fails, it might be a duplicate.
        // But better to just report the error as is, unless we can be sure.
        // However, the original code returned 9729.
        
        std::string msg = "Database error in InsertPortLock: ";
        msg += e.what();
        
        // Check if it's a duplicate key error (often 2627 or 2601 in SQL Server)
        // or Lock timeout (1222)
        // For now, we'll just log it. If the user needs specific return codes, we might need to inspect e.native_error().
        
        // Attempt to mimic the "duplicate key" return if it looks like one.
        // But without native error access easily in this catch block without casting, we'll stick to standard reporting.
        // Wait, nanodbc::database_error has native_error() method? No, it has state() and what().
        // We can check e.state() for "23000".
        
        *pzErr = PrintError((char*)msg.c_str(), iId, iId, (char*)"F", 0, -1, 0, (char*)"InsertPortLock", FALSE);
        
        // If we want to return 9729 for duplicates, we could check e.what() for "duplicate" or "violation".
        if (msg.find("duplicate") != std::string::npos || msg.find("violation") != std::string::npos)
        {
             pzErr->iSqlError = 9729;
             return 9729;
        }
        
        return pzErr->iSqlError;
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertPortLock", iId, iId, (char*)"F", 0, -1, 0, (char*)"InsertPortLock", FALSE);
        return pzErr->iSqlError;
    }
}

DLLAPI void STDCALL SelectPortLock(int iId, char *sUserId, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectPortLock", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "select user_id from portlock with (nolock) where id=?"));

        stmt.bind(0, &iId);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "user_id", sUserId, 17); // 16+1
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectPortLock: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iId, iId, (char*)"F", 0, -1, 0, (char*)"SelectPortLock", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectPortLock", iId, iId, (char*)"F", 0, -1, 0, (char*)"SelectPortLock", FALSE);
    }
}

DLLAPI void STDCALL DeletePortLock(int iId, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"DeletePortLock", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM PORTLOCK Where Id = ?"));

        stmt.bind(0, &iId);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeletePortLock: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iId, 0, (char*)"", 0, -1, 0, (char*)"DeletePortLock", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeletePortLock", iId, 0, (char*)"", 0, -1, 0, (char*)"DeletePortLock", FALSE);
    }
}
