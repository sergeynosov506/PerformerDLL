#include "TransIO_DControl.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL SelectDcontrol(DCONTROL *pzDC, long lRecDate, char *sCountry, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectDcontrol", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "Select marketclosed, bankclosed, recorddescription "
            "From dcontrol "
            "Where recorddate = ? and country = ? "));

        nanodbc::timestamp ts;
        long_to_timestamp(lRecDate, ts);
        
        stmt.bind(0, &ts);
        stmt.bind(1, sCountry);

        nanodbc::result result = nanodbc::execute(stmt);
        
        if (result.next())
        {
            read_string(result, "marketclosed", pzDC->sMarketClosed, sizeof(pzDC->sMarketClosed));
            read_string(result, "bankclosed", pzDC->sBAnkClosed, sizeof(pzDC->sBAnkClosed));
            read_string(result, "recorddescription", pzDC->sRecordDescription, sizeof(pzDC->sRecordDescription));
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectDcontrol: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, lRecDate, (char*)"D", 0, -1, 0, (char*)"SelectDcontrol", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectDcontrol", 0, lRecDate, (char*)"D", 0, -1, 0, (char*)"SelectDcontrol", FALSE);
    }
}
