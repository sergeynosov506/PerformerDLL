#include "TransIO_Currency.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;

DLLAPI void STDCALL SelectCurrencySecno(char *sSecNo, char *sWi, int *piSecid, char *sCurrId, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectCurrencySecno", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT sec_no, wi, secid "
            "FROM currency WHERE curr_id = ? "));

        stmt.bind(0, sCurrId);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "sec_no", sSecNo, 13);
            read_string(result, "wi", sWi, 2);
            *piSecid = result.get<int>("secid", 0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectCurrencySecno: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectCurrencySecno", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectCurrencySecno", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectCurrencySecno", FALSE);
    }
}
