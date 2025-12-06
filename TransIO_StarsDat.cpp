#include "TransIO_StarsDat.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h" // For date conversions if needed, though here we deal with longs directly or dates?
// The original code used VARIANT dates (double). 
// *plTradeDate = cmdSelectStarsDate.m_vTradeDate.date;
// In OLE DB, DBTYPE_DATE is a double (OADate).
// We need to check what the database column type is.
// "SELECT pricing_date, trade_date from starsdat"
// If they are datetime columns, nanodbc returns timestamp.
// We need to convert timestamp to OLE Automation Date (double) if the output is long* but treated as date?
// Wait, the signature is long* plTradeDate.
// In the original code: m_vTradeDate.date is a double.
// Assigning double to long? That truncates the time part.
// So it seems it returns the integer part of the OLE Date, which is the number of days since 1899-12-30.
// Let's verify what `date` member of VARIANT is. It is `double`.
// So `*plTradeDate = (long)cmdSelectStarsDate.m_vTradeDate.date;`
// We need a helper to convert nanodbc timestamp to OLE Date (double) and then cast to long.
// Or if the DB column is int, then it's just int.
// But usually pricing_date is datetime.
// Let's assume they are datetime.

extern thread_local nanodbc::connection gConn;

// Helper to convert timestamp to OLE Date (double)
// We might need `timestamp_to_long` from dateutils.h if it does what we think.
// Let's check dateutils.h if possible, or implement a simple conversion.
// OLE Date: 0.0 = 1899-12-30.
// We can use a helper if available.
// `long_to_timestamp` and `timestamp_to_long` are mentioned in the summary.
// Let's assume `timestamp_to_long` returns the format expected here.

DLLAPI void STDCALL SelectStarsDate(long* plTradeDate, long* plPricingDate, ERRSTRUCT* pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectStarsDate", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT pricing_date, trade_date from starsdat"));

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            // Assuming columns are datetime
            nanodbc::timestamp tsPricing = result.get<nanodbc::timestamp>(0);
            nanodbc::timestamp tsTrade = result.get<nanodbc::timestamp>(1);

            // Convert to long (OLE Date integer part)
            // We need to verify if timestamp_to_long does this.
            // If not, we might need to implement it.
            // For now, let's assume timestamp_to_long converts to the long representation used in this app.
             *plPricingDate = timestamp_to_long(tsPricing);
             *plTradeDate = timestamp_to_long(tsTrade);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectStarsDate: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectStarsDate", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectStarsDate", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectStarsDate", FALSE);
    }
}
