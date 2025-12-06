#include "TransIO_TransDesc.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "trans.h"
#include <iostream>
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

DLLAPI void STDCALL SelectTransDesc(TRNDESC *pzTransDesc, int iID, long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"SelectTransDesc", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT ID, trans_No, Seqno, Close_Type, Taxlot_No, Units, Desc_Info FROM trnDesc WHERE ID = ? AND trans_no = ? "));

        stmt.bind(0, &iID);
        stmt.bind(1, &lTransNo);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            pzTransDesc->iID = result.get<int>("ID");
            pzTransDesc->lDtransNo = result.get<long>("trans_No");
            pzTransDesc->iSeqno = result.get<int>("Seqno");
            read_string(result, "Close_Type", pzTransDesc->sCloseType, sizeof(pzTransDesc->sCloseType));
            pzTransDesc->lTaxlotNo = result.get<long>("Taxlot_No");
            pzTransDesc->fUnits = result.get<double>("Units");
            read_string(result, "Desc_Info", pzTransDesc->sDescInfo, sizeof(pzTransDesc->sDescInfo));
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectTransDesc: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"SelectTransDesc", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectTransDesc", iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"SelectTransDesc", FALSE);
    }
}

DLLAPI void STDCALL InsertTransDesc(TRNDESC zTransDesc, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", zTransDesc.iID, zTransDesc.lDtransNo, (char*)"D", 0, -1, 0, (char*)"InsertTransDesc", FALSE);
        return;
    }

    try
    {
        // Rounding logic from original code
        zTransDesc.fUnits = RoundDouble(zTransDesc.fUnits, 5);

        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("INSERT INTO trnDesc (ID, trans_No, Seqno, Close_Type, Taxlot_No, Units, Desc_Info) VALUES (?, ?, ?, ?, ?, ?, ?)"));

        stmt.bind(0, &zTransDesc.iID);
        stmt.bind(1, &zTransDesc.lDtransNo);
        stmt.bind(2, &zTransDesc.iSeqno);
        stmt.bind(3, zTransDesc.sCloseType);
        stmt.bind(4, &zTransDesc.lTaxlotNo);
        stmt.bind(5, &zTransDesc.fUnits);
        stmt.bind(6, zTransDesc.sDescInfo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertTransDesc: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), zTransDesc.iID, zTransDesc.lDtransNo, (char*)"D", 0, -1, 0, (char*)"InsertTransDesc", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertTransDesc", zTransDesc.iID, zTransDesc.lDtransNo, (char*)"D", 0, -1, 0, (char*)"InsertTransDesc", FALSE);
    }
}
