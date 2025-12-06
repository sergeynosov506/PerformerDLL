#include "TransIO_AccDiv.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL DeleteAccdivOneLot(int iID, long lTransNo, long lDivintNo, ERRSTRUCT* pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteAccdivOneLot", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM AccDiv WHERE ID = ? AND trans_no = ? AND divint_no = ?"));

        stmt.bind(0, &iID);
        stmt.bind(1, &lTransNo);
        stmt.bind(2, &lDivintNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteAccdivOneLot: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccdivOneLot", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteAccdivOneLot", iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccdivOneLot", FALSE);
    }
}

DLLAPI void STDCALL DeleteAccruingAccdivOneLot(ACCDIV* pzAccdiv, ERRSTRUCT* pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteAccruingAccdivOneLot", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM AccDiv WHERE ID = ? "
            "AND sec_no = ? AND wi = ? "
            "AND sec_xtend = ? AND acct_type = ? "
            "AND trans_no = ? AND delete_flag <> 'Y'"));

        stmt.bind(0, &pzAccdiv->iID);
        stmt.bind(1, pzAccdiv->sSecNo);
        stmt.bind(2, pzAccdiv->sWi);
        stmt.bind(3, pzAccdiv->sSecXtend);
        stmt.bind(4, pzAccdiv->sAcctType);
        stmt.bind(5, &pzAccdiv->lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteAccruingAccdivOneLot: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), pzAccdiv->iID, pzAccdiv->lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccruingAccdivOneLot", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteAccruingAccdivOneLot", pzAccdiv->iID, pzAccdiv->lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccruingAccdivOneLot", FALSE);
    }
}

DLLAPI void STDCALL DeleteAccdivAllLots(int iID, long lDivintNo, ERRSTRUCT* pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteAccdivAllLots", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM AccDiv WHERE ID = ? AND divint_no = ?"));

        stmt.bind(0, &iID);
        stmt.bind(1, &lDivintNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteAccdivAllLots: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lDivintNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccdivAllLots", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteAccdivAllLots", iID, lDivintNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccdivAllLots", FALSE);
    }
}
