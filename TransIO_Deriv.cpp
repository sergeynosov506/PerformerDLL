#include "TransIO_Deriv.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL SelectCallPut(char *sCallPut, char *sSecNo, char *sWhenIssue, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectCallPut", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT call_put FROM deriv WHERE sec_no = ? and when_issue = ?"));

        stmt.bind(0, sSecNo);
        stmt.bind(1, sWhenIssue);

        nanodbc::result result = nanodbc::execute(stmt);
        
        if (result.next())
        {
            read_string(result, "call_put", sCallPut, 1 + NT);
        }
        else
        {
            sCallPut[0] = '\0';
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectCallPut: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectCallPut", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectCallPut", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectCallPut", FALSE);
    }
}
