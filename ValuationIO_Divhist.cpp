/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Divhist.cpp
 * 
 * DESCRIPTION: Dividend history and units calculation implementations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: SelectAllDivhistForAnAccount: Multi-row cursor with static state
 *        SelectUnitsForASoldSecurity: Aggregate SUM query
 *        
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#include "commonheader.h"
#include "ValuationIO_Divhist.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "divhist.h"
#include <optional>
#include <cstring>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SQL Queries
// ============================================================================

const char* SQL_SELECT_ALL_DIVHIST =
    "SELECT ID, TransNo, DivintNo, TranType, "
    "SecNo, Wi, SecID, SecXtend, AcctType, "
    "Units, DivTransNo, "
    "TranLocation, ExDate, PayDate "
    "FROM DivHist "
    "WHERE ID = ?";

const char* SQL_SELECT_UNITS_SOLD =
    "SELECT SUM(a.units) "
    "FROM trans a, trantype b, holddel c "
    "WHERE a.tran_type = b.tran_type AND a.dr_cr = b.dr_cr AND b.tran_code = 'C' AND "
    "a.id = c.id AND a.trans_no = c.create_trans_no AND a.rev_trans_no = 0 AND "
    "c.rev_trans_no = 0 AND a.id = ? AND a.sec_no = ? AND a.wi = ? AND "
    "a.sec_xtend = ? AND a.acct_type = ? AND "
    "a.trd_date <= ? AND c.elig_date < ? AND c.elig_date <= ? AND a.trd_date > ?";

// ============================================================================
// Static State for Multi-Row Cursor
// ============================================================================

struct DivhistState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    int cRows = 0;
};

static DivhistState g_divhistState;

// ============================================================================
// SelectAllDivhistForAnAccount (Multi-Row Cursor)
// ============================================================================

DLLAPI void STDCALL SelectAllDivhistForAnAccount(int iID, DIVHIST *pzDivhist, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllDivhistForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllDivhistForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed
        bool needNewQuery = !(g_divhistState.iID == iID && g_divhistState.cRows > 0);

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_divhistState.result.reset();
            g_divhistState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_ALL_DIVHIST));
            stmt.bind(0, &iID);

            g_divhistState.result = nanodbc::execute(stmt);
            g_divhistState.iID = iID;
        }

        // Try to get next record
        if (g_divhistState.result && g_divhistState.result->next())
        {
            g_divhistState.cRows++;
            memset(pzDivhist, 0, sizeof(*pzDivhist));

            // Fill DIVHIST struct
            pzDivhist->iID = g_divhistState.result->get<int>("ID", 0);
            pzDivhist->lTransNo = g_divhistState.result->get<long>("TransNo", 0);
            pzDivhist->lDivintNo = g_divhistState.result->get<long>("DivintNo", 0);
            
            read_string(*g_divhistState.result, "TranType", pzDivhist->sTranType, sizeof(pzDivhist->sTranType));
            read_string(*g_divhistState.result, "SecNo", pzDivhist->sSecNo, sizeof(pzDivhist->sSecNo));
            read_string(*g_divhistState.result, "Wi", pzDivhist->sWi, sizeof(pzDivhist->sWi));
            
            pzDivhist->iSecID = g_divhistState.result->get<int>("SecID", 0);
            
            read_string(*g_divhistState.result, "SecXtend", pzDivhist->sSecXtend, sizeof(pzDivhist->sSecXtend));
            read_string(*g_divhistState.result, "AcctType", pzDivhist->sAcctType, sizeof(pzDivhist->sAcctType));
            
            pzDivhist->fUnits = g_divhistState.result->get<double>("Units", 0.0);
            pzDivhist->lDivTransNo = g_divhistState.result->get<long>("DivTransNo", 0);
            
            read_string(*g_divhistState.result, "TranLocation", pzDivhist->sTranLocation, sizeof(pzDivhist->sTranLocation));
            
            // Convert dates
            pzDivhist->lExDate = timestamp_to_long(g_divhistState.result->get<nanodbc::timestamp>("ExDate"));
            pzDivhist->lPayDate = timestamp_to_long(g_divhistState.result->get<nanodbc::timestamp>("PayDate"));
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_divhistState.cRows = 0;
            g_divhistState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllDivhistForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllDivhistForAnAccount", FALSE);
        g_divhistState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllDivhistForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllDivhistForAnAccount", FALSE);
        g_divhistState.result.reset();
    }
}

// ============================================================================
// SelectUnitsForASoldSecurity (Aggregate Query)
// ============================================================================

DLLAPI void STDCALL SelectUnitsForASoldSecurity(int iID, char *sSecNo, char *sWi, 
    char *sSecXtend, char *sAcctType, long lValDate, long lRecDate, long lPayDate, 
    double *pfUnits, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectUnitsForASoldSecurity", FALSE);
#endif

    InitializeErrStruct(pzErr);
    *pfUnits = 0.0;

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnitsForASoldSecurity", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_UNITS_SOLD));

        // Convert dates
        nanodbc::timestamp tsValDate, tsRecDate, tsPayDate;
        long_to_timestamp(lValDate, tsValDate);
        long_to_timestamp(lRecDate, tsRecDate);
        long_to_timestamp(lPayDate, tsPayDate);

        // Bind parameters
        int paramIndex = 0;
        stmt.bind(paramIndex++, &iID);
        stmt.bind(paramIndex++, sSecNo);
        stmt.bind(paramIndex++, sWi);
        stmt.bind(paramIndex++, sSecXtend);
        stmt.bind(paramIndex++, sAcctType);
        stmt.bind(paramIndex++, &tsValDate);
        stmt.bind(paramIndex++, &tsRecDate);
        stmt.bind(paramIndex++, &tsPayDate);
        stmt.bind(paramIndex++, &tsRecDate);  // Note: lRecDate used for trd_date comparison

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
            // SUM returns NULL if no rows, so handle that
            if (!result.is_null(0))
                *pfUnits = result.get<double>(0, 0.0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectUnitsForASoldSecurity: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnitsForASoldSecurity", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectUnitsForASoldSecurity", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnitsForASoldSecurity", FALSE);
    }
}
