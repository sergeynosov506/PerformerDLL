/**
 * SUB-SYSTEM: Database Input/Output for Valuation
 * FILENAME: ValuationIO_Holdings.cpp
 * DESCRIPTION: Holdings, Holdcash, and Hedgxref implementations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * NOTES: SelectAllHoldingsForAnAccount has 59 columns and 10 date conversions!
 *        Table name substitution for holdings and holdcash tables
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "ValuationIO_Holdings.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "holdings.h"
#include "hedgexref.h" 
#include <optional>
#include <cstring>
#include <string>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SQL Queries (with table name substitution)
// ============================================================================

const char* SQL_SELECT_ALL_HOLDINGS =
    "SELECT ID, sec_no, wi, sec_xtend, acct_type, trans_no, "
    "secid, asof_date, "
    "sec_symbol, units, orig_face, tot_cost, "
    "unit_cost, orig_cost, open_liability, base_cost_xrate, "
    "sys_cost_xrate, trd_date, eff_date, elig_date, "
    "stl_date, orig_yield, eff_mat_date, eff_mat_price, "
    "cost_eff_mat_yld, amort_start_date, orig_trans_type, "
    "orig_trans_srce, last_trans_type, last_trans_no, "
    "last_trans_srce, last_pmt_date, last_pmt_type, "
    "last_pmt_tr_no, next_pmt_date, next_pmt_amt, "
    "last_pdn_date, lt_st_ind, mkt_val, cur_liability, "
    "mv_base_xrate, mv_sys_xrate, accr_int, "
    "ai_base_xrate, ai_sys_xrate, annual_income, "
    "accrual_gl, currency_gl, security_gl, mkt_eff_mat_yld, "
    "mkt_cur_yld, safek_ind, collateral_units, hedge_value, "
    "benchmark_sec_no, perm_lt_flag, valuation_srce, primary_type, restriction_code "
    "FROM %HOLDINGS_TABLE_NAME% "
    "WHERE ID = ? "
    "ORDER BY sec_no, wi";

const char* SQL_SELECT_UNITS_HELD =
    "SELECT SUM(units) "
    "FROM %HOLDINGS_TABLE_NAME% "
    "WHERE ID = ? AND sec_no = ? AND wi = ? "
    "AND sec_xtend = ? AND acct_type = ? AND elig_date < ? AND elig_date <= ?";

const char* SQL_SELECT_ALL_HOLDCASH =
    "SELECT ID, sec_no, wi, sec_xtend, acct_type, secid, "
    "asof_date, sec_symbol, units, tot_cost, unit_cost, "
    "base_cost_xrate, sys_cost_xrate, trd_date, eff_date, "
    "stl_date, last_trans_no, mkt_val, mv_base_xrate, "
    "mv_sys_xrate, currency_gl, collateral_units, hedge_value "
    "FROM %HOLDCASH_TABLE_NAME% "
    "WHERE ID = ? "
    "ORDER BY sec_no, wi";

// Original SQL from ValuationIO.cpp (CSelectAllHedgxrefForAnAccount)
const char* SQL_SELECT_ALL_HEDGXREF =
    "SELECT ID, sec_no, wi, sec_xtend, acct_type, trans_no, secid, asof_date, "
    "sec_no2, wi2, sec_xtend2, acct_type2, trans_no2, secid2, "
    "hedge_units, valuation_srce "
    "FROM %HEDGXREF_TABLE_NAME% "
    "WHERE ID = ?";

// ============================================================================
// Static State for Multi-Row Cursors
// ============================================================================

struct HoldingsState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    char sTableName[40] = {0};
    int cRows = 0;
};

struct HoldcashState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    char sTableName[40] = {0};
    int cRows = 0;
};

struct HedgxrefState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    char sTableName[40] = {0};
    int cRows = 0;
};

static HoldingsState g_holdingsState;
static HoldcashState g_holdcashState;
static HedgxrefState g_hedgxrefState;

// ============================================================================
// SelectAllHoldingsForAnAccount (Most Complex - 59 columns, 10 dates!)
// ============================================================================

DLLAPI void STDCALL SelectAllHoldingsForAnAccount(int iID, HOLDINGS *pzHoldings, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllHoldingsForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHoldingsForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed (including table name!)
        bool needNewQuery = !(
            g_holdingsState.iID == iID &&
            strcmp(g_holdingsState.sTableName, sHoldings) == 0 &&
            g_holdingsState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_holdingsState.result.reset();
            g_holdingsState.cRows = 0;

            // Build SQL with table name substitution
            std::string sql = SQL_SELECT_ALL_HOLDINGS;
            size_t pos = sql.find("%HOLDINGS_TABLE_NAME%");
            if (pos != std::string::npos)
                sql.replace(pos, 21, sHoldings);

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
            stmt.bind(0, &iID);

            g_holdingsState.result = nanodbc::execute(stmt);
            g_holdingsState.iID = iID;
            strcpy_s(g_holdingsState.sTableName, sHoldings);
        }

        // Try to get next record
        if (g_holdingsState.result && g_holdingsState.result->next())
        {
            g_holdingsState.cRows++;
            memset(pzHoldings, 0, sizeof(*pzHoldings));

            // Fill HOLDINGS struct (59 columns!)
            pzHoldings->iID = g_holdingsState.result->get<int>("ID", 0);
            read_string(*g_holdingsState.result, "sec_no", pzHoldings->sSecNo, sizeof(pzHoldings->sSecNo));
            read_string(*g_holdingsState.result, "wi", pzHoldings->sWi, sizeof(pzHoldings->sWi));
            read_string(*g_holdingsState.result, "sec_xtend", pzHoldings->sSecXtend, sizeof(pzHoldings->sSecXtend));
            read_string(*g_holdingsState.result, "acct_type", pzHoldings->sAcctType, sizeof(pzHoldings->sAcctType));
            pzHoldings->lTransNo = g_holdingsState.result->get<long>("trans_no", 0);            
            pzHoldings->iSecID = g_holdingsState.result->get<int>("secid", 0);
            
            // Date 1: asof_date
            read_date(*g_holdingsState.result, "asof_date", &pzHoldings->lAsofDate);
            
            read_string(*g_holdingsState.result, "sec_symbol", pzHoldings->sSecSymbol, sizeof(pzHoldings->sSecSymbol));
            pzHoldings->fUnits = g_holdingsState.result->get<double>("units", 0.0);
            pzHoldings->fOrigFace = g_holdingsState.result->get<double>("orig_face", 0.0);
            pzHoldings->fTotCost = g_holdingsState.result->get<double>("tot_cost", 0.0);
            pzHoldings->fUnitCost = g_holdingsState.result->get<double>("unit_cost", 0.0);
            pzHoldings->fOrigCost = g_holdingsState.result->get<double>("orig_cost", 0.0);
            pzHoldings->fOpenLiability = g_holdingsState.result->get<double>("open_liability", 0.0);
            pzHoldings->fBaseCostXrate = g_holdingsState.result->get<double>("base_cost_xrate", 0.0);
            pzHoldings->fSysCostXrate = g_holdingsState.result->get<double>("sys_cost_xrate", 0.0);
            
            // Dates 2-5: trd_date, eff_date, elig_date, stl_date
            read_date(*g_holdingsState.result, "trd_date", &pzHoldings->lTrdDate);
            read_date(*g_holdingsState.result, "eff_date", &pzHoldings->lEffDate);
            read_date(*g_holdingsState.result, "elig_date", &pzHoldings->lEligDate);
            read_date(*g_holdingsState.result, "stl_date", &pzHoldings->lStlDate);
            
            pzHoldings->fOrigYield = g_holdingsState.result->get<double>("orig_yield", 0.0);
            
            // Date 6: eff_mat_date
            read_date(*g_holdingsState.result, "eff_mat_date", &pzHoldings->lEffMatDate);
            
            pzHoldings->fEffMatPrice = g_holdingsState.result->get<double>("eff_mat_price", 0.0);
            pzHoldings->fCostEffMatYld = g_holdingsState.result->get<double>("cost_eff_mat_yld", 0.0);
            
            // Date 7: amort_start_date
            read_date(*g_holdingsState.result, "amort_start_date", &pzHoldings->lAmortStartDate);
            
            read_string(*g_holdingsState.result, "orig_trans_type", pzHoldings->sOrigTransType, sizeof(pzHoldings->sOrigTransType));
            read_string(*g_holdingsState.result, "orig_trans_srce", pzHoldings->sOrigTransSrce, sizeof(pzHoldings->sOrigTransSrce));
            read_string(*g_holdingsState.result, "last_trans_type", pzHoldings->sLastTransType, sizeof(pzHoldings->sLastTransType));
            pzHoldings->lLastTransNo = g_holdingsState.result->get<long>("last_trans_no", 0);
            read_string(*g_holdingsState.result, "last_trans_srce", pzHoldings->sLastTransSrce, sizeof(pzHoldings->sLastTransSrce));
            
            // Date 8: last_pmt_date
            read_date(*g_holdingsState.result, "last_pmt_date", &pzHoldings->lLastPmtDate);
            
            read_string(*g_holdingsState.result, "last_pmt_type", pzHoldings->sLastPmtType, sizeof(pzHoldings->sLastPmtType));
            pzHoldings->lLastPmtTrNo = g_holdingsState.result->get<long>("last_pmt_tr_no", 0);
            
            // Date 9: next_pmt_date
            read_date(*g_holdingsState.result, "next_pmt_date", &pzHoldings->lNextPmtDate);
            
            pzHoldings->fNextPmtAmt = g_holdingsState.result->get<double>("next_pmt_amt", 0.0);
            
            // Date 10: last_pdn_date
            read_date(*g_holdingsState.result, "last_pdn_date", &pzHoldings->lLastPdnDate);
            
            read_string(*g_holdingsState.result, "lt_st_ind", pzHoldings->sLtStInd, sizeof(pzHoldings->sLtStInd));
            pzHoldings->fMktVal = g_holdingsState.result->get<double>("mkt_val", 0.0);
            pzHoldings->fCurLiability = g_holdingsState.result->get<double>("cur_liability", 0.0);
            pzHoldings->fMvBaseXrate = g_holdingsState.result->get<double>("mv_base_xrate", 0.0);
            pzHoldings->fMvSysXrate = g_holdingsState.result->get<double>("mv_sys_xrate", 0.0);
            pzHoldings->fAccrInt = g_holdingsState.result->get<double>("accr_int", 0.0);
            pzHoldings->fAiBaseXrate = g_holdingsState.result->get<double>("ai_base_xrate", 0.0);
            pzHoldings->fAiSysXrate = g_holdingsState.result->get<double>("ai_sys_xrate", 0.0);
            pzHoldings->fAnnualIncome = g_holdingsState.result->get<double>("annual_income", 0.0);
            pzHoldings->fAccrualGl = g_holdingsState.result->get<double>("accrual_gl", 0.0);
            pzHoldings->fCurrencyGl = g_holdingsState.result->get<double>("currency_gl", 0.0);
            pzHoldings->fSecurityGl = g_holdingsState.result->get<double>("security_gl", 0.0);
            pzHoldings->fMktEffMatYld = g_holdingsState.result->get<double>("mkt_eff_mat_yld", 0.0);
            pzHoldings->fMktCurYld = g_holdingsState.result->get<double>("mkt_cur_yld", 0.0);
            read_string(*g_holdingsState.result, "safek_ind", pzHoldings->sSafekInd, sizeof(pzHoldings->sSafekInd));
            pzHoldings->fCollateralUnits = g_holdingsState.result->get<double>("collateral_units", 0.0);
            pzHoldings->fHedgeValue = g_holdingsState.result->get<double>("hedge_value", 0.0);
            read_string(*g_holdingsState.result, "benchmark_sec_no", pzHoldings->sBenchmarkSecNo, sizeof(pzHoldings->sBenchmarkSecNo));
            read_string(*g_holdingsState.result, "perm_lt_flag", pzHoldings->sPermLtFlag, sizeof(pzHoldings->sPermLtFlag));
            read_string(*g_holdingsState.result, "valuation_srce", pzHoldings->sValuationSrce, sizeof(pzHoldings->sValuationSrce));
            read_string(*g_holdingsState.result, "primary_type", pzHoldings->sPrimaryType, sizeof(pzHoldings->sPrimaryType));
            pzHoldings->iRestrictionCode = g_holdingsState.result->get<int>("restriction_code", 0);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_holdingsState.cRows = 0;
            g_holdingsState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllHoldingsForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHoldingsForAnAccount", FALSE);
        g_holdingsState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllHoldingsForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHoldingsForAnAccount", FALSE);
        g_holdingsState.result.reset();
    }
}

// ============================================================================
// SelectUnitsHeldForASecurity (Aggregate SUM)
// ============================================================================

DLLAPI void STDCALL SelectUnitsHeldForASecurity(int iID, char *sSecNo, char *sWi, 
    char *sSecXtend, char *sAcctType, long lRecDate, long lPayDate, 
    double *pfUnits, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectUnitsHeldForASecurity", FALSE);
#endif

    InitializeErrStruct(pzErr);
    *pfUnits = 0.0;

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnitsHeldForASecurity", FALSE);
        return;
    }

    try
    {
        // Build SQL with table name substitution
        std::string sql = SQL_SELECT_UNITS_HELD;
        size_t pos = sql.find("%HOLDINGS_TABLE_NAME%");
        if (pos != std::string::npos)
            sql.replace(pos, 21, sHoldings);

        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

        nanodbc::timestamp tsRecDate, tsPayDate;
        long_to_timestamp(lRecDate, tsRecDate);
        long_to_timestamp(lPayDate, tsPayDate);

        int paramIndex = 0;
        stmt.bind(paramIndex++, &iID);
        stmt.bind(paramIndex++, sSecNo);
        stmt.bind(paramIndex++, sWi);
        stmt.bind(paramIndex++, sSecXtend);
        stmt.bind(paramIndex++, sAcctType);
        stmt.bind(paramIndex++, &tsRecDate);
        stmt.bind(paramIndex++, &tsPayDate);

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
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
        std::string msg = "Database error in SelectUnitsHeldForASecurity: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnitsHeldForASecurity", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectUnitsHeldForASecurity", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnitsHeldForASecurity", FALSE);
    }
}

// ============================================================================
// SelectAllHoldcashForAnAccount (Multi-Row with 4 dates)
// ============================================================================

DLLAPI void STDCALL SelectAllHoldcashForAnAccount(int iID, HOLDCASH *pzHoldcash, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllHoldcashForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHoldcashForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed (including table name!)
        bool needNewQuery = !(
            g_holdcashState.iID == iID &&
            strcmp(g_holdcashState.sTableName, sHoldcash) == 0 &&
            g_holdcashState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_holdcashState.result.reset();
            g_holdcashState.cRows = 0;

            // Build SQL with table name substitution
            std::string sql =SQL_SELECT_ALL_HOLDCASH;
            size_t pos = sql.find("%HOLDCASH_TABLE_NAME%");
            if (pos != std::string::npos)
                sql.replace(pos, 21, sHoldcash);

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
            stmt.bind(0, &iID);

            g_holdcashState.result = nanodbc::execute(stmt);
            g_holdcashState.iID = iID;
            strcpy_s(g_holdcashState.sTableName, sHoldcash);
        }

        // Try to get next record
        if (g_holdcashState.result && g_holdcashState.result->next())
        {
            g_holdcashState.cRows++;
            memset(pzHoldcash, 0, sizeof(*pzHoldcash));

            // Fill HOLDCASH struct
            pzHoldcash->iID = g_holdcashState.result->get<int>("ID", 0);
            read_string(*g_holdcashState.result, "sec_no", pzHoldcash->sSecNo, sizeof(pzHoldcash->sSecNo));
            read_string(*g_holdcashState.result, "wi", pzHoldcash->sWi, sizeof(pzHoldcash->sWi));
            read_string(*g_holdcashState.result, "sec_xtend", pzHoldcash->sSecXtend, sizeof(pzHoldcash->sSecXtend));
            read_string(*g_holdcashState.result, "acct_type", pzHoldcash->sAcctType, sizeof(pzHoldcash->sAcctType));
            pzHoldcash->iSecID = g_holdcashState.result->get<int>("secid", 0);
            
            // Date conversions (4 dates)
            read_date(*g_holdcashState.result, "asof_date", &pzHoldcash->lAsofDate);
            
            read_string(*g_holdcashState.result, "sec_symbol", pzHoldcash->sSecSymbol, sizeof(pzHoldcash->sSecSymbol));
            pzHoldcash->fUnits = g_holdcashState.result->get<double>("units", 0.0);
            pzHoldcash->fTotCost = g_holdcashState.result->get<double>("tot_cost", 0.0);
            pzHoldcash->fUnitCost = g_holdcashState.result->get<double>("unit_cost", 0.0);
            pzHoldcash->fBaseCostXrate = g_holdcashState.result->get<double>("base_cost_xrate", 0.0);
            pzHoldcash->fSysCostXrate = g_holdcashState.result->get<double>("sys_cost_xrate", 0.0);
            
            read_date(*g_holdcashState.result, "trd_date", &pzHoldcash->lTrdDate);
            read_date(*g_holdcashState.result, "eff_date", &pzHoldcash->lEffDate);
            read_date(*g_holdcashState.result, "stl_date", &pzHoldcash->lStlDate);
            
            pzHoldcash->lLastTransNo = g_holdcashState.result->get<long>("last_trans_no", 0);
            pzHoldcash->fMktVal = g_holdcashState.result->get<double>("mkt_val", 0.0);
            pzHoldcash->fMvBaseXrate = g_holdcashState.result->get<double>("mv_base_xrate", 0.0);
            pzHoldcash->fMvSysXrate = g_holdcashState.result->get<double>("mv_sys_xrate", 0.0);
            pzHoldcash->fCurrencyGl = g_holdcashState.result->get<double>("currency_gl", 0.0);
            pzHoldcash->fCollateralUnits = g_holdcashState.result->get<double>("collateral_units", 0.0);
            pzHoldcash->fHedgeValue = g_holdcashState.result->get<double>("hedge_value", 0.0);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_holdcashState.cRows = 0;
            g_holdcashState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllHoldcashForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHoldcashForAnAccount", FALSE);
        g_holdcashState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllHoldcashForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHoldcashForAnAccount", FALSE);
        g_holdcashState.result.reset();
    }
}

// ============================================================================
// SelectAllHedgxrefForAnAccount (Multi-Row with 3 dates)
// ============================================================================

DLLAPI void STDCALL SelectAllHedgxrefForAnAccount(int iID, HEDGEXREF *pzHedgxref, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllHedgxrefForAnAccount", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHedgxrefForAnAccount", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed (including table name!)
        bool needNewQuery = !(
            g_hedgxrefState.iID == iID &&
            strcmp(g_hedgxrefState.sTableName, sHedgxref) == 0 &&
            g_hedgxrefState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_hedgxrefState.result.reset();
            g_hedgxrefState.cRows = 0;

            // Build SQL with table name substitution
            std::string sql = SQL_SELECT_ALL_HEDGXREF;
            size_t pos = sql.find("%HEDGXREF_TABLE_NAME%");
            if (pos != std::string::npos)
                sql.replace(pos, 21, sHedgxref);

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
            stmt.bind(0, &iID);

            g_hedgxrefState.result = nanodbc::execute(stmt);
            g_hedgxrefState.iID = iID;
            strcpy_s(g_hedgxrefState.sTableName, sHedgxref);
        }

        // Try to get next record
        if (g_hedgxrefState.result && g_hedgxrefState.result->next())
        {
            g_hedgxrefState.cRows++;
            memset(pzHedgxref, 0, sizeof(*pzHedgxref));

            // Fill HEDGEXREF struct - Mapping matches ValuationIO.cpp CSelectAllHedgxrefForAnAccount
            pzHedgxref->iID = g_hedgxrefState.result->get<int>("ID", 0);
            read_string(*g_hedgxrefState.result, "sec_no", pzHedgxref->sSecNo, sizeof(pzHedgxref->sSecNo));
            read_string(*g_hedgxrefState.result, "wi", pzHedgxref->sWi, sizeof(pzHedgxref->sWi));
            read_string(*g_hedgxrefState.result, "sec_xtend", pzHedgxref->sSecXtend, sizeof(pzHedgxref->sSecXtend));
            read_string(*g_hedgxrefState.result, "acct_type", pzHedgxref->sAcctType, sizeof(pzHedgxref->sAcctType));
            pzHedgxref->lTransNo = g_hedgxrefState.result->get<long>("trans_no", 0);
            pzHedgxref->iSecID = g_hedgxrefState.result->get<int>("secid", 0);
            
            // Date conversion (1 date)
            read_date(*g_hedgxrefState.result, "asof_date", &pzHedgxref->lAsofDate);
            
            // Mapped fields to HEDGEXREF struct
            read_string(*g_hedgxrefState.result, "sec_no2", pzHedgxref->sSecNo2, sizeof(pzHedgxref->sSecNo2));
            read_string(*g_hedgxrefState.result, "wi2", pzHedgxref->sWi2, sizeof(pzHedgxref->sWi2));
            read_string(*g_hedgxrefState.result, "sec_xtend2", pzHedgxref->sSecXtend2, sizeof(pzHedgxref->sSecXtend2));
            read_string(*g_hedgxrefState.result, "acct_type2", pzHedgxref->sAcctType2, sizeof(pzHedgxref->sAcctType2));
            pzHedgxref->lTransNo2 = g_hedgxrefState.result->get<long>("trans_no2", 0);
            pzHedgxref->iSecID2 = g_hedgxrefState.result->get<int>("secid2", 0);
            pzHedgxref->fHedgeUnits = g_hedgxrefState.result->get<double>("hedge_units", 0.0);
            read_string(*g_hedgxrefState.result, "valuation_srce", pzHedgxref->sValuationSrce, sizeof(pzHedgxref->sValuationSrce));
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_hedgxrefState.cRows = 0;
            g_hedgxrefState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllHedgxrefForAnAccount: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHedgxrefForAnAccount", FALSE);
        g_hedgxrefState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllHedgxrefForAnAccount", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllHedgxrefForAnAccount", FALSE);
        g_hedgxrefState.result.reset();
    }
}
