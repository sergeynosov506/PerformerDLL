#include "TransIO_Hedgexref.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL DeleteHedgxref(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteHedgxref", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "DELETE FROM hedgexref "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND sec_xtend = ? AND "
            "acct_type = ? AND trans_no = ?"));

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
        std::string msg = "Database error in DeleteHedgxref: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteHedgxref", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteHedgxref", 0, 0, (char*)"D", 0, -1, 0, (char*)"DeleteHedgxref", FALSE);
    }
}

DLLAPI void STDCALL InsertHedgxref(HEDGEXREF *pzHG, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"InsertHedgxref", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "INSERT INTO hedgexref "
            "(ID, sec_no, wi, sec_xtend, acct_type, trans_no, "
            "secid, asof_date, sec_no2, wi2, sec_xtend2, "
            "acct_type2, trans_no2, secid2, hedge_units, "
            "hedge_val_base, hedge_val_native, hedge_val_system, "
            "hedge_units2, hedge_type, valuation_srce) "
            "VALUES "
            "(?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, "
            "?, ?, ?, ?, "
            "?, ?, ?, "
            "?, ?, ?)"));

        stmt.bind(0, &pzHG->iID);
        stmt.bind(1, pzHG->sSecNo);
        stmt.bind(2, pzHG->sWi);
        stmt.bind(3, pzHG->sSecXtend);
        stmt.bind(4, pzHG->sAcctType);
        stmt.bind(5, &pzHG->lTransNo);
        
        stmt.bind(6, &pzHG->iSecID);
        // asof_date is long in struct, assuming it maps to int/long in DB or needs conversion? 
        // Based on other files, dates are often longs.
        stmt.bind(7, &pzHG->lAsofDate); 
        stmt.bind(8, pzHG->sSecNo2);
        stmt.bind(9, pzHG->sWi2);
        stmt.bind(10, pzHG->sSecXtend2);
        
        stmt.bind(11, pzHG->sAcctType2);
        stmt.bind(12, &pzHG->lTransNo2);
        stmt.bind(13, &pzHG->iSecID2);
        stmt.bind(14, &pzHG->fHedgeUnits);
        
        stmt.bind(15, &pzHG->fHedgeValBase);
        stmt.bind(16, &pzHG->fHedgeValNative);
        stmt.bind(17, &pzHG->fHedgeValSystem);
        
        stmt.bind(18, &pzHG->fHedgeUnits2);
        stmt.bind(19, pzHG->sHedgeType);
        stmt.bind(20, pzHG->sValuationSrce);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertHedgxref: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"D", 0, -1, 0, (char*)"InsertHedgxref", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertHedgxref", 0, 0, (char*)"D", 0, -1, 0, (char*)"InsertHedgxref", FALSE);
    }
}

DLLAPI void STDCALL SelectHedgxref(HEDGEXREF *pzHG, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectHedgxref", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT ID, sec_no, wi, sec_xtend, acct_type, trans_no, secid, asof_date, "
            "sec_no2, wi2, sec_xtend2, acct_type2, trans_no2, secid2, "
            "hedge_units, hedge_val_base, hedge_val_native, hedge_val_system, "
            "hedge_units2, hedge_type, valuation_srce "
            "FROM hedgexref "
            "WHERE ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=? and trans_no=?"));

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &lTransNo);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_int(result, "ID", &pzHG->iID);
            read_string(result, "sec_no", pzHG->sSecNo, sizeof(pzHG->sSecNo));
            read_string(result, "wi", pzHG->sWi, sizeof(pzHG->sWi));
            read_string(result, "sec_xtend", pzHG->sSecXtend, sizeof(pzHG->sSecXtend));
            read_string(result, "acct_type", pzHG->sAcctType, sizeof(pzHG->sAcctType));
            read_long(result, "trans_no", &pzHG->lTransNo);
            read_int(result, "secid", &pzHG->iSecID);
            read_long(result, "asof_date", &pzHG->lAsofDate); // Assuming date is stored as int/long

            read_string(result, "sec_no2", pzHG->sSecNo2, sizeof(pzHG->sSecNo2));
            read_string(result, "wi2", pzHG->sWi2, sizeof(pzHG->sWi2));
            read_string(result, "sec_xtend2", pzHG->sSecXtend2, sizeof(pzHG->sSecXtend2));
            read_string(result, "acct_type2", pzHG->sAcctType2, sizeof(pzHG->sAcctType2));
            read_long(result, "trans_no2", &pzHG->lTransNo2);
            read_int(result, "secid2", &pzHG->iSecID2);

            read_double(result, "hedge_units", &pzHG->fHedgeUnits);
            read_double(result, "hedge_val_base", &pzHG->fHedgeValBase);
            read_double(result, "hedge_val_native", &pzHG->fHedgeValNative);
            read_double(result, "hedge_val_system", &pzHG->fHedgeValSystem);

            read_double(result, "hedge_units2", &pzHG->fHedgeUnits2);
            read_string(result, "hedge_type", pzHG->sHedgeType, sizeof(pzHG->sHedgeType));
            read_string(result, "valuation_srce", pzHG->sValuationSrce, sizeof(pzHG->sValuationSrce));
        }
        else
        {
             // Not found handling if needed, or just leave struct as is?
             // Legacy code didn't seem to return a specific not found error in the signature, 
             // but usually caller checks something. 
             // For now, we just don't fill the struct.
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectHedgxref: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectHedgxref", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectHedgxref", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectHedgxref", FALSE);
    }
}

DLLAPI void STDCALL UpdateHedgxref(HEDGEXREF *pzHG, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"UpdateHedgxref", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE hedgexref "
            "SET asof_date=?, sec_no2=?, wi2=?, sec_xtend2=?, "
            "acct_type2=?, trans_no2=?, secid2=?, hedge_units=?, "
            "hedge_val_base=?, hedge_val_native=?, "
            "hedge_val_system=?, hedge_units2=?, "
            "hedge_type=?, valuation_srce=? "
            "WHERE ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=? and trans_no=?"));

        stmt.bind(0, &pzHG->lAsofDate);
        stmt.bind(1, pzHG->sSecNo2);
        stmt.bind(2, pzHG->sWi2);
        stmt.bind(3, pzHG->sSecXtend2);
        
        stmt.bind(4, pzHG->sAcctType2);
        stmt.bind(5, &pzHG->lTransNo2);
        stmt.bind(6, &pzHG->iSecID2);
        stmt.bind(7, &pzHG->fHedgeUnits);
        
        stmt.bind(8, &pzHG->fHedgeValBase);
        stmt.bind(9, &pzHG->fHedgeValNative);
        
        stmt.bind(10, &pzHG->fHedgeValSystem);
        stmt.bind(11, &pzHG->fHedgeUnits2);
        
        stmt.bind(12, pzHG->sHedgeType);
        stmt.bind(13, pzHG->sValuationSrce);

        // WHERE clause
        stmt.bind(14, &pzHG->iID);
        stmt.bind(15, pzHG->sSecNo);
        stmt.bind(16, pzHG->sWi);
        stmt.bind(17, pzHG->sSecXtend);
        stmt.bind(18, pzHG->sAcctType);
        stmt.bind(19, &pzHG->lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdateHedgxref: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"D", 0, -1, 0, (char*)"UpdateHedgxref", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdateHedgxref", 0, 0, (char*)"D", 0, -1, 0, (char*)"UpdateHedgxref", FALSE);
    }
}
