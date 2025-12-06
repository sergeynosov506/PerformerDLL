#include "TransIO_Divhist.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL DeleteDivhistOneLot(int iID, long lTransNo, long lDivintNo, long lDivTransNo, char *sTranType, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteDivhistOneLot", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM divhist WHERE ID=? AND trans_no=? AND divint_no=? AND div_trans_no=? AND tran_type=?"));

        stmt.bind(0, &iID);
        stmt.bind(1, &lTransNo);
        stmt.bind(2, &lDivintNo);
        stmt.bind(3, &lDivTransNo);
        stmt.bind(4, sTranType);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteDivhistOneLot: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteDivhistOneLot", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteDivhistOneLot", iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteDivhistOneLot", FALSE);
    }
}

DLLAPI void STDCALL DeleteAccruingDivhistOneLot(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteAccruingDivhistOneLot", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM divhist WHERE ID=? AND sec_no=? AND wi=? AND sec_xtend=? AND acct_type=? AND trans_no=? AND tran_location='A'"));

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteAccruingDivhistOneLot: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccruingDivhistOneLot", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteAccruingDivhistOneLot", iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"DeleteAccruingDivhistOneLot", FALSE);
    }
}

DLLAPI void STDCALL DeleteDivhistAllLots(int iID, long lDivintNo, long lDivTransNo, char *sTranType, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteDivhistAllLots", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM divhist WHERE ID=? AND divint_no=? AND div_trans_no=? AND tran_type=?"));

        stmt.bind(0, &iID);
        stmt.bind(1, &lDivintNo);
        stmt.bind(2, &lDivTransNo);
        stmt.bind(3, sTranType);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteDivhistAllLots: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lDivintNo, (char*)"D", 0, -1, 0, (char*)"DeleteDivhistAllLots", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteDivhistAllLots", iID, lDivintNo, (char*)"D", 0, -1, 0, (char*)"DeleteDivhistAllLots", FALSE);
    }
}

DLLAPI void STDCALL UpdateDivhistOneLot(char *sNewTType, char *sTranLocation, long lDivTransNo, int iID, long lTransNo, long lDivintNo, char *sOldTType, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"UpdateDivhistOneLot", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE divhist SET tran_type = ?, div_trans_no = ?, tran_location = ? "
            "WHERE ID = ? AND trans_no = ? AND divint_no = ? AND tran_type = ?"));

        stmt.bind(0, sNewTType);
        stmt.bind(1, &lDivTransNo);
        stmt.bind(2, sTranLocation);
        stmt.bind(3, &iID);
        stmt.bind(4, &lTransNo);
        stmt.bind(5, &lDivintNo);
        stmt.bind(6, sOldTType);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdateDivhistOneLot: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"UpdateDivhistOneLot", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdateDivhistOneLot", iID, lTransNo, (char*)"D", 0, -1, 0, (char*)"UpdateDivhistOneLot", FALSE);
    }
}

DLLAPI void STDCALL UpdateDivhistAllLots(char *sNewTType, char *sTranLocation, long lDivTransNo, int iID, long lDivintNo, char *sOldTType, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"UpdateDivhistAllLots", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE divhist SET tran_type = ?, div_trans_no = ?, tran_location = ? "
            "WHERE ID = ? AND divint_no = ? AND tran_type = ?"));

        stmt.bind(0, sNewTType);
        stmt.bind(1, &lDivTransNo);
        stmt.bind(2, sTranLocation);
        stmt.bind(3, &iID);
        stmt.bind(4, &lDivintNo);
        stmt.bind(5, sOldTType);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdateDivhistAllLots: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lDivintNo, (char*)"D", 0, -1, 0, (char*)"UpdateDivhistAllLots", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdateDivhistAllLots", iID, lDivintNo, (char*)"D", 0, -1, 0, (char*)"UpdateDivhistAllLots", FALSE);
    }
}
