/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Payrec.cpp
 * 
 * DESCRIPTION: Payment record query and update implementations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: SelectAllPayrecForAnAccount: Multi-row cursor with table name substitution
 *        UpdatePayrec: UPDATE operation with table name substitution
 *        UpdatePortmainValDate: Simple UPDATE on portmain table
 *        
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#include "commonheader.h"
#include "ValuationIO_Payrec.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "PaymentsIO.h"
#include <optional>
#include <cstring>
#include <string>

extern thread_local nanodbc::connection gConn;
extern char sPayrec[];  // Global payrec table name

// ============================================================================
// SQL Query Templates (with table name substitution)
// ============================================================================

const char* SQL_SELECT_ALL_PAYREC =
    "SELECT ID, sec_no, wi, sec_xtend, acct_type, "
    "trans_no, secid, asof_date, tran_type, "
    "divint_no, units, base_cost_xrate, "
    "sys_cost_xrate, cur_val, eff_date, "
    "mv_base_xrate, mv_sys_xrate, valuation_srce "
    "FROM %PAYREC_TABLE_NAME% "
    "WHERE ID = ?";

const char* SQL_UPDATE_PAYREC =
    "UPDATE %PAYREC_TABLE_NAME% "
    "SET units = ?, base_cost_xrate = ?, sys_cost_xrate = ?, "
    "cur_val = ?, mv_base_xrate = ?, mv_sys_xrate = ?, valuation_srce = ? "
    "WHERE ID = ? AND sec_no = ? AND wi = ? AND sec_xtend = ? AND acct_type = ? AND trans_no = ?";

const char* SQL_UPDATE_PORTMAIN_VALDATE =
    "UPDATE portmain "
    "SET val_date = ? "
    "WHERE id = ?";

// ============================================================================
// Static State for Multi-Row Cursor
// ============================================================================

struct PayrecState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    char sTableName[40] = {0};
    int cRows = 0;
};

static PayrecState g_payrecState;

// ============================================================================
// SelectAllPayrecForAnAccount (Multi-Row Cursor with Table Substitution)
// ============================================================================

DLLAPI void STDCALL SelectAllPayrecForAnAccount(int iID, PAYREC *pzPayrec, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllPayrecForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPayrecForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed (including table name!)
        bool needNewQuery = !(
            g_payrecState.iID == iID &&
            strcmp(g_payrecState.sTableName, sPayrec) == 0 &&
            g_payrecState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_payrecState.result.reset();
            g_payrecState.cRows = 0;

            // Build SQL with table name substitution
            std::string sql = SQL_SELECT_ALL_PAYREC;
            size_t pos = sql.find("%PAYREC_TABLE_NAME%");
            if (pos != std::string::npos)
                sql.replace(pos, 19, sPayrec);

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
            stmt.bind(0, &iID);

            g_payrecState.result = nanodbc::execute(stmt);
            g_payrecState.iID = iID;
            strcpy_s(g_payrecState.sTableName, sPayrec);
        }

        // Try to get next record
        if (g_payrecState.result && g_payrecState.result->next())
        {
            g_payrecState.cRows++;
            memset(pzPayrec, 0, sizeof(*pzPayrec));

            // Fill PAYREC struct
            pzPayrec->iID = g_payrecState.result->get<int>("ID", 0);
            read_string(*g_payrecState.result, "sec_no", pzPayrec->sSecNo, sizeof(pzPayrec->sSecNo));
            read_string(*g_payrecState.result, "wi", pzPayrec->sWi, sizeof(pzPayrec->sWi));
            read_string(*g_payrecState.result, "sec_xtend", pzPayrec->sSecXtend, sizeof(pzPayrec->sSecXtend));
            read_string(*g_payrecState.result, "acct_type", pzPayrec->sAcctType, sizeof(pzPayrec->sAcctType));
            
            pzPayrec->lTransNo = g_payrecState.result->get<long>("trans_no", 0);
            pzPayrec->iSecID = g_payrecState.result->get<int>("secid", 0);
            
            // Convert dates
            read_date(*g_payrecState.result, "asof_date", &pzPayrec->lAsofDate);
            read_date(*g_payrecState.result, "eff_date", &pzPayrec->lEffDate);
            
            read_string(*g_payrecState.result, "tran_type", pzPayrec->sTranType, sizeof(pzPayrec->sTranType));
            
            pzPayrec->lDivintNo = g_payrecState.result->get<long>("divint_no", 0);
            pzPayrec->fUnits = g_payrecState.result->get<double>("units", 0.0);
            pzPayrec->fBaseCostXrate = g_payrecState.result->get<double>("base_cost_xrate", 0.0);
            pzPayrec->fSysCostXrate = g_payrecState.result->get<double>("sys_cost_xrate", 0.0);
            pzPayrec->fCurVal = g_payrecState.result->get<double>("cur_val", 0.0);
            pzPayrec->fMvBaseXrate = g_payrecState.result->get<double>("mv_base_xrate", 0.0);
            pzPayrec->fMvSysXrate = g_payrecState.result->get<double>("mv_sys_xrate", 0.0);
            
            read_string(*g_payrecState.result, "valuation_srce", pzPayrec->sValuationSrce, sizeof(pzPayrec->sValuationSrce));
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_payrecState.cRows = 0;
            g_payrecState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllPayrecForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPayrecForAnAccount", FALSE);
        g_payrecState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllPayrecForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPayrecForAnAccount", FALSE);
        g_payrecState.result.reset();
    }
}

// ============================================================================
// UpdatePayrec (UPDATE with Table Substitution)
// ============================================================================

DLLAPI void STDCALL UpdatePayrec(PAYREC zPayrec, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdatePayrec", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePayrec", FALSE);
        return;
    }

    try
    {
        // Build SQL with table name substitution
        std::string sql = SQL_UPDATE_PAYREC;
        size_t pos = sql.find("%PAYREC_TABLE_NAME%");
        if (pos != std::string::npos)
            sql.replace(pos, 19, sPayrec);

        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

        // Bind parameters (SET clause)
        int paramIndex = 0;
        stmt.bind(paramIndex++, &zPayrec.fUnits);
        stmt.bind(paramIndex++, &zPayrec.fBaseCostXrate);
        stmt.bind(paramIndex++, &zPayrec.fSysCostXrate);
        stmt.bind(paramIndex++, &zPayrec.fCurVal);
        stmt.bind(paramIndex++, &zPayrec.fMvBaseXrate);
        stmt.bind(paramIndex++, &zPayrec.fMvSysXrate);
        stmt.bind(paramIndex++, zPayrec.sValuationSrce);
        
        // WHERE clause
        stmt.bind(paramIndex++, &zPayrec.iID);
        stmt.bind(paramIndex++, zPayrec.sSecNo);
        stmt.bind(paramIndex++, zPayrec.sWi);
        stmt.bind(paramIndex++, zPayrec.sSecXtend);
        stmt.bind(paramIndex++, zPayrec.sAcctType);
        stmt.bind(paramIndex++, &zPayrec.lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdatePayrec: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePayrec", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdatePayrec", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePayrec", FALSE);
    }
}

// ============================================================================
// UpdatePortmainValDate (Simple UPDATE)
// ============================================================================

DLLAPI void STDCALL UpdatePortmainValDate(int iID, long lValDate, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdatePortmainValDate", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePortmainValDate", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UPDATE_PORTMAIN_VALDATE));

        nanodbc::timestamp tsValDate;
        long_to_timestamp(lValDate, tsValDate);

        stmt.bind(0, &tsValDate);
        stmt.bind(1, &iID);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdatePortmainValDate: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePortmainValDate", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdatePortmainValDate", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePortmainValDate", FALSE);
    }
}
