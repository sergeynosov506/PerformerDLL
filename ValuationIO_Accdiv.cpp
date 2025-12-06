/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Accdiv.cpp
 * 
 * DESCRIPTION: Accrued dividend query implementations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: Both functions use multi-row cursor with static state
 *        Complex date range filtering and subquery logic
 *        
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#include "commonheader.h"
#include "ValuationIO_Accdiv.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "accdiv.h"
#include <optional>
#include <cstring>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SQL Queries
// ============================================================================

const char* SQL_SELECT_ALL_ACCDIV =
    "SELECT id, trans_no, divint_no, tran_type, sec_no, wi, "
    "sec_xtend, acct_type, dr_cr, units, pcpl_amt, "
    "income_amt, trd_date, stl_date, eff_date, "
    "curr_id, curr_acct_type, inc_curr_id, inc_acct_type, "
    "base_xrate, inc_base_xrate, "
    "sec_base_xrate, accr_base_xrate, delete_flag "
    "FROM accdiv "
    "WHERE id = ? AND trd_date <= ? AND stl_date > ? "
    "AND EXISTS (SELECT divint_no FROM divint b "
    "WHERE accdiv.divint_no = b.divint_no "
    "AND accdiv.elig_date < b.ex_date AND ISNULL(b.delete_flag, '') <> 'Y')";

const char* SQL_SELECT_PENDING_ACCDIV_TRANS =
    "SELECT id, taxlot_no, divint_no, tran_type, sec_no, wi, "
    "sec_xtend, acct_type, dr_cr, units, pcpl_amt, "
    "income_amt, trd_date, stl_date, eff_date, "
    "curr_id, curr_acct_type, inc_curr_id, inc_acct_type, "
    "base_xrate, inc_base_xrate, "
    "sec_base_xrate, accr_base_xrate "
    "FROM trans "
    "WHERE id = ? AND tran_type = 'RD' "
    "AND rev_trans_no = 0 "
    "AND trd_date <= ? AND stl_date > ?";

// ============================================================================
// Static State for Multi-Row Cursors
// ============================================================================

struct AccdivState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    long lTrdDate = 0;
    long lStlDate = 0;
    int cRows = 0;
};

struct PendingAccdivState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    long lValDate = 0;
    int cRows = 0;
};

static AccdivState g_accdivState;
static PendingAccdivState g_pendingAccdivState;

// ============================================================================
// Helper Function
// ============================================================================

static void FillPartAccdiv(nanodbc::result& result, PARTACCDIV* pzAccdiv)
{
    memset(pzAccdiv, 0, sizeof(*pzAccdiv));
    
    pzAccdiv->iID = result.get<int>("id", 0);
    
    // Handle trans_no vs taxlot_no (different column names in different tables)
    if (!result.is_null("trans_no"))
        pzAccdiv->lTransNo = result.get<long>("trans_no", 0);
    else if (!result.is_null("taxlot_no"))
        pzAccdiv->lTransNo = result.get<long>("taxlot_no", 0);
    
    pzAccdiv->lDivintNo = result.get<long>("divint_no", 0);
    
    read_string(result, "tran_type", pzAccdiv->sTranType, sizeof(pzAccdiv->sTranType));
    read_string(result, "sec_no", pzAccdiv->sSecNo, sizeof(pzAccdiv->sSecNo));
    read_string(result, "wi", pzAccdiv->sWi, sizeof(pzAccdiv->sWi));
    read_string(result, "sec_xtend", pzAccdiv->sSecXtend, sizeof(pzAccdiv->sSecXtend));
    read_string(result, "acct_type", pzAccdiv->sAcctType, sizeof(pzAccdiv->sAcctType));
    read_string(result, "dr_cr", pzAccdiv->sDrCr, sizeof(pzAccdiv->sDrCr));
    
    pzAccdiv->fUnits = result.get<double>("units", 0.0);
    pzAccdiv->fPcplAmt = result.get<double>("pcpl_amt", 0.0);
    pzAccdiv->fIncomeAmt = result.get<double>("income_amt", 0.0);
    
    // Convert dates
    read_date(result, "trd_date", &pzAccdiv->lTrdDate);
    read_date(result, "stl_date", &pzAccdiv->lStlDate);
    read_date(result, "eff_date", &pzAccdiv->lEffDate);
    
    read_string(result, "curr_id", pzAccdiv->sCurrId, sizeof(pzAccdiv->sCurrId));
    read_string(result, "curr_acct_type", pzAccdiv->sCurrAcctType, sizeof(pzAccdiv->sCurrAcctType));
    read_string(result, "inc_curr_id", pzAccdiv->sIncCurrId, sizeof(pzAccdiv->sIncCurrId));
    read_string(result, "inc_acct_type", pzAccdiv->sIncAcctType, sizeof(pzAccdiv->sIncAcctType));
    
    pzAccdiv->fBaseXrate = result.get<double>("base_xrate", 0.0);
    pzAccdiv->fIncBaseXrate = result.get<double>("inc_base_xrate", 0.0);
    pzAccdiv->fSecBaseXrate = result.get<double>("sec_base_xrate", 0.0);
    pzAccdiv->fAccrBaseXrate = result.get<double>("accr_base_xrate", 0.0);
    
    // delete_flag only in accdiv table (not trans)
    if (!result.is_null("delete_flag"))
        read_string(result, "delete_flag", pzAccdiv->sDeleteFlag, sizeof(pzAccdiv->sDeleteFlag));
}

// ============================================================================
// SelectAllAccdivForAnAccount (Multi-Row Cursor)
// ============================================================================

DLLAPI void STDCALL SelectAllAccdivForAnAccount(int iID, long lTrdDate, long lStlDate, 
    PARTACCDIV *pzAccdiv, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllAccdivForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllAccdivForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed
        bool needNewQuery = !(
            g_accdivState.iID == iID &&
            g_accdivState.lTrdDate == lTrdDate &&
            g_accdivState.lStlDate == lStlDate &&
            g_accdivState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_accdivState.result.reset();
            g_accdivState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_ALL_ACCDIV));

            nanodbc::timestamp tsTrdDate, tsStlDate;
            long_to_timestamp(lTrdDate, tsTrdDate);
            long_to_timestamp(lStlDate, tsStlDate);

            stmt.bind(0, &iID);
            stmt.bind(1, &tsTrdDate);
            stmt.bind(2, &tsStlDate);

            g_accdivState.result = nanodbc::execute(stmt);
            g_accdivState.iID = iID;
            g_accdivState.lTrdDate = lTrdDate;
            g_accdivState.lStlDate = lStlDate;
        }

        // Try to get next record
        if (g_accdivState.result && g_accdivState.result->next())
        {
            g_accdivState.cRows++;
            FillPartAccdiv(*g_accdivState.result, pzAccdiv);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_accdivState.cRows = 0;
            g_accdivState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllAccdivForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllAccdivForAnAccount", FALSE);
        g_accdivState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllAccdivForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllAccdivForAnAccount", FALSE);
        g_accdivState.result.reset();
    }
}

// ============================================================================
// SelectPendingAccdivTransForAnAccount (Multi-Row Cursor)
// ============================================================================

DLLAPI void STDCALL SelectPendingAccdivTransForAnAccount(int iID, long lValDate, 
    PARTACCDIV *pzAccdiv, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectPendingAccdivTransForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPendingAccdivTransForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed
        bool needNewQuery = !(
            g_pendingAccdivState.iID == iID &&
            g_pendingAccdivState.lValDate == lValDate &&
            g_pendingAccdivState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_pendingAccdivState.result.reset();
            g_pendingAccdivState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_PENDING_ACCDIV_TRANS));

            nanodbc::timestamp tsValDate;
            long_to_timestamp(lValDate, tsValDate);

            stmt.bind(0, &iID);
            stmt.bind(1, &tsValDate);  // trd_date <= ?
            stmt.bind(2, &tsValDate);  // stl_date > ?

            g_pendingAccdivState.result = nanodbc::execute(stmt);
            g_pendingAccdivState.iID = iID;
            g_pendingAccdivState.lValDate = lValDate;
        }

        // Try to get next record
        if (g_pendingAccdivState.result && g_pendingAccdivState.result->next())
        {
            g_pendingAccdivState.cRows++;
            FillPartAccdiv(*g_pendingAccdivState.result, pzAccdiv);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_pendingAccdivState.cRows = 0;
            g_pendingAccdivState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectPendingAccdivTransForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPendingAccdivTransForAnAccount", FALSE);
        g_pendingAccdivState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectPendingAccdivTransForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPendingAccdivTransForAnAccount", FALSE);
        g_pendingAccdivState.result.reset();
    }
}
