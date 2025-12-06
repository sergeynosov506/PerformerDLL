#include "TransIO_DTransDesc.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL SelectDtransDesc(DTRNDESC *pzTR, int iID, long lDtransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectDtransDesc", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT ID, dtrans_no, seqno, close_type, "
            "taxlot_no, units, Desc_info "
            "FROM dtrnDesc "
            "WHERE ID = ? AND dtrans_no = ? "
            "ORDER BY seqno "));

        stmt.bind(0, &iID);
        stmt.bind(1, &lDtransNo);

        nanodbc::result result = nanodbc::execute(stmt);
        
        // As discussed, we are fetching the first row. 
        // If the caller expects multiple rows, this interface (single struct pointer) is insufficient.
        // But we are matching the existing signature.
        
        if (result.next())
        {
            read_int(result, "ID", &pzTR->iID);
            read_int(result, "dtrans_no", &pzTR->lDtransNo); // struct has lDtransNo as int? Check trndesc.h
            // trndesc.h: int lDtransNo; (line 25)
            
            read_short(result, "seqno", &pzTR->iSeqno);
            read_string(result, "close_type", pzTR->sCloseType, sizeof(pzTR->sCloseType));
            read_int(result, "taxlot_no", &pzTR->lTaxlotNo);
            read_double(result, "units", &pzTR->fUnits);
            read_string(result, "Desc_info", pzTR->sDescInfo, sizeof(pzTR->sDescInfo));
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectDtransDesc: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lDtransNo, (char*)"D", 0, -1, 0, (char*)"SelectDtransDesc", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectDtransDesc", iID, lDtransNo, (char*)"D", 0, -1, 0, (char*)"SelectDtransDesc", FALSE);
    }
}
