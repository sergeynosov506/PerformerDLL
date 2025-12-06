#include "TransIO_Holdmap.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;

DLLAPI void STDCALL SelectHoldmap(long lAsofDate, char *sHoldingsName, char *sHoldcashName, char *sPortmainName, 
							 char *sPortbalName, char *sPayrecName, char *sHXrefName, char *sHoldtotName,
							 ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectHoldmap", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT holdings_name, holdcash_name, portdir_name, "
            "portbal_name, payrec_name, hedge_xref_name, holdtot_name "
            "FROM Holdmap WHERE asof_date = ?"));

        nanodbc::timestamp tsAsofDate;
        long_to_timestamp(lAsofDate, tsAsofDate);

        stmt.bind(0, &tsAsofDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "holdings_name", sHoldingsName, 20);
            read_string(result, "holdcash_name", sHoldcashName, 20);
            read_string(result, "portdir_name", sPortmainName, 20);
            read_string(result, "portbal_name", sPortbalName, 20);
            read_string(result, "payrec_name", sPayrecName, 20);
            read_string(result, "hedge_xref_name", sHXrefName, 20);
            read_string(result, "holdtot_name", sHoldtotName, 20);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectHoldmap: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectHoldmap", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectHoldmap", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectHoldmap", FALSE);
    }
}
