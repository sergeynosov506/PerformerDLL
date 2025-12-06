/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_DSumdata.cpp
 * DESCRIPTION: Daily Summary Data implementations (4 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "PerformanceIO_DSumdata.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "summdata.h"
#include "dateutils.h"
#include <optional>
#include <cstring>

extern thread_local nanodbc::connection gConn;

// Static state for multi-row cursor
struct SelectDailySummdataState {
    std::optional<nanodbc::result> result;
    int iPortfolioID = 0;
    long lPerformDate = 0;
    int cRows = 0;
};

static SelectDailySummdataState g_selectDailySummdataState;

// ============================================================================
// DeleteDSumdata
// ============================================================================
DLLAPI void STDCALL DeleteDSumdata(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteDSumdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM DSUMDATA WHERE id = ? AND perform_date >= ? AND perform_date <= ?"));

        nanodbc::timestamp tsBegin, tsEnd;
        long_to_timestamp(lBeginDate, tsBegin);
        
        if (lEndDate == -1)
            long_to_timestamp(MAXDATE, tsEnd);
        else
            long_to_timestamp(lEndDate, tsEnd);

        stmt.bind(0, &iID);
        stmt.bind(1, &tsBegin);
        stmt.bind(2, &tsEnd);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteDSumdata", FALSE);
    }
}

// ============================================================================
// InsertDailySummdata
// ============================================================================
DLLAPI void STDCALL InsertDailySummdata(SUMMDATA zSD, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertDailySummdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        // Using explicit column names to ensure correct mapping
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "INSERT INTO DSUMDATA "
            "(portfolio_id, id, perform_date, mkt_val, book_value, accr_inc, "
            "accr_div, inc_rclm, div_rclm, net_flow, cum_flow, wtd_flow, purchases, "
            "sales, income, cum_income, wtd_inc, fees, cum_fees, wtd_fees, "
            "exch_rate_base, interval_type, days_since_nond, days_since_last, "
            "create_date, change_date, perform_type, principalpaydown, maturity, "
            "contributions, withdrawals, expenses, receipts, incomecash, principalcash, "
            "feesout, cum_feesout, wtd_feesout, transfers, transferin, transferout, created_by, EstAnnIncome, NotionalFlow, Cons_fee, Cum_Cons, Wtd_Cons) "
            "VALUES (?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, "
            "?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        nanodbc::timestamp tsPerform, tsCreate, tsChange;
        long_to_timestamp(zSD.lPerformDate, tsPerform);
        long_to_timestamp(zSD.fCreateDate, tsCreate);
        long_to_timestamp(zSD.fChangeDate, tsChange);

        int idx = 0;
        stmt.bind(idx++, &zSD.iPortfolioID);
        stmt.bind(idx++, &zSD.iID);
        stmt.bind(idx++, &tsPerform);
        stmt.bind(idx++, &zSD.fMktVal);
        stmt.bind(idx++, &zSD.fBookValue);
        stmt.bind(idx++, &zSD.fAccrInc);

        stmt.bind(idx++, &zSD.fAccrDiv);
        stmt.bind(idx++, &zSD.fIncRclm);
        stmt.bind(idx++, &zSD.fDivRclm);
        stmt.bind(idx++, &zSD.fNetFlow);
        stmt.bind(idx++, &zSD.fCumFlow);
        stmt.bind(idx++, &zSD.fWtdFlow);
        stmt.bind(idx++, &zSD.fPurchases);

        stmt.bind(idx++, &zSD.fSales);
        stmt.bind(idx++, &zSD.fIncome);
        stmt.bind(idx++, &zSD.fCumIncome);
        stmt.bind(idx++, &zSD.fWtdInc);
        stmt.bind(idx++, &zSD.fFees);
        stmt.bind(idx++, &zSD.fCumFees);
        stmt.bind(idx++, &zSD.fWtdFees);

        stmt.bind(idx++, &zSD.fExchRateBase);
        stmt.bind(idx++, zSD.sIntervalType);
        stmt.bind(idx++, &zSD.lDaysSinceNond);
        stmt.bind(idx++, &zSD.lDaysSinceLast);

        stmt.bind(idx++, &tsCreate);
        stmt.bind(idx++, &tsChange);
        stmt.bind(idx++, zSD.sPerformType);
        stmt.bind(idx++, &zSD.fPrincipalPayDown);
        stmt.bind(idx++, &zSD.fMaturity);

        stmt.bind(idx++, &zSD.fContribution);
        stmt.bind(idx++, &zSD.fWithdrawals);
        stmt.bind(idx++, &zSD.fExpenses);
        stmt.bind(idx++, &zSD.fReceipts);
        stmt.bind(idx++, &zSD.fIncomeCash);
        stmt.bind(idx++, &zSD.fPrincipalCash);

        stmt.bind(idx++, &zSD.fFeesOut);
        stmt.bind(idx++, &zSD.fCumFeesOut);
        stmt.bind(idx++, &zSD.fWtdFeesOut);
        stmt.bind(idx++, &zSD.fTransfers);
        stmt.bind(idx++, &zSD.fTransfersIn);
        stmt.bind(idx++, &zSD.fTransfersOut);
        stmt.bind(idx++, &zSD.iCreatedBy);
        stmt.bind(idx++, &zSD.fEstAnnIncome);
        stmt.bind(idx++, &zSD.fNotionalFlow);
        stmt.bind(idx++, &zSD.fCNFees);
        stmt.bind(idx++, &zSD.fCumCNFees);
        stmt.bind(idx++, &zSD.fWtdCNFees);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zSD.iPortfolioID, 0, (char*)"", 0, -1, 0, (char*)"InsertDailySummdata", FALSE);
    }
}

// ============================================================================
// SelectDailySummdata
// ============================================================================
DLLAPI void STDCALL SelectDailySummdata(SUMMDATA *pzSD , int iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectDailySummdata", FALSE);
        return;
    }

    try
    {
        // Check if we need to reopen the query
        if (g_selectDailySummdataState.iPortfolioID != iPortfolioID ||
            g_selectDailySummdataState.lPerformDate != lPerformDate ||
            g_selectDailySummdataState.cRows == 0 ||
            !g_selectDailySummdataState.result)
        {
            g_selectDailySummdataState.result.reset();
            g_selectDailySummdataState.iPortfolioID = iPortfolioID;
            g_selectDailySummdataState.lPerformDate = lPerformDate;
            g_selectDailySummdataState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(
                "SELECT id, portfolio_id, perform_date, "
                "mkt_val, accr_inc, accr_div, net_flow, cum_flow, "
                "income, cum_income, fees, cum_fees, feesout, cum_feesout, "
                "days_since_nond, perform_type, NotionalFlow, Cons_Fee, Cum_Cons "
                "FROM DSUMDATA "
                "WHERE portfolio_id = ? AND perform_date >= ? "
                "ORDER BY id, perform_date DESC"));

            nanodbc::timestamp tsPerform;
            long_to_timestamp(lPerformDate, tsPerform);

            stmt.bind(0, &iPortfolioID);
            stmt.bind(1, &tsPerform);

            g_selectDailySummdataState.result = nanodbc::execute(stmt);
        }

        if (g_selectDailySummdataState.result && g_selectDailySummdataState.result->next())
        {
            g_selectDailySummdataState.cRows++;
            memset(pzSD, 0, sizeof(*pzSD));

            pzSD->iID = g_selectDailySummdataState.result->get<int>("id", 0);
            pzSD->iPortfolioID = g_selectDailySummdataState.result->get<int>("portfolio_id", 0);
            
            nanodbc::timestamp ts;
            if (!g_selectDailySummdataState.result->is_null("perform_date"))
            {
                ts = g_selectDailySummdataState.result->get<nanodbc::timestamp>("perform_date");
                pzSD->lPerformDate = timestamp_to_long(ts);
            }

            pzSD->fMktVal = g_selectDailySummdataState.result->get<double>("mkt_val", 0.0);
            pzSD->fAccrInc = g_selectDailySummdataState.result->get<double>("accr_inc", 0.0);
            pzSD->fAccrDiv = g_selectDailySummdataState.result->get<double>("accr_div", 0.0);
            pzSD->fNetFlow = g_selectDailySummdataState.result->get<double>("net_flow", 0.0);
            pzSD->fCumFlow = g_selectDailySummdataState.result->get<double>("cum_flow", 0.0);

            pzSD->fIncome = g_selectDailySummdataState.result->get<double>("income", 0.0);
            pzSD->fCumIncome = g_selectDailySummdataState.result->get<double>("cum_income", 0.0);
            pzSD->fFees = g_selectDailySummdataState.result->get<double>("fees", 0.0);
            pzSD->fCumFees = g_selectDailySummdataState.result->get<double>("cum_fees", 0.0);
            pzSD->fFeesOut = g_selectDailySummdataState.result->get<double>("feesout", 0.0);
            pzSD->fCumFeesOut = g_selectDailySummdataState.result->get<double>("cum_feesout", 0.0);

            pzSD->lDaysSinceNond = g_selectDailySummdataState.result->get<long>("days_since_nond", 0);
            read_string(*g_selectDailySummdataState.result, "perform_type", pzSD->sPerformType, sizeof(pzSD->sPerformType));
            pzSD->fNotionalFlow = g_selectDailySummdataState.result->get<double>("NotionalFlow", 0.0);
            pzSD->fCNFees = g_selectDailySummdataState.result->get<double>("Cons_Fee", 0.0);
            pzSD->fCumCNFees = g_selectDailySummdataState.result->get<double>("Cum_Cons", 0.0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_selectDailySummdataState.cRows = 0;
            g_selectDailySummdataState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), iPortfolioID, 0, (char*)"", 0, -1, 0, (char*)"SelectDailySummdata", FALSE);
        g_selectDailySummdataState.result.reset();
    }
}

// ============================================================================
// UpdateDailySummdata
// ============================================================================
DLLAPI void STDCALL UpdateDailySummdata(SUMMDATA zSD, long lOldPerformDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateDailySummdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE DSUMDATA SET "
            "perform_date = ?, mkt_val = ?, book_value = ?, "
            "accr_inc = ?, accr_div = ?, net_flow = ?, "
            "cum_flow = ?, wtd_flow = ?, "
            "purchases = ?, sales = ?, income = ?, "
            "cum_income = ?, wtd_inc = ?, fees = ?, "
            "cum_fees = ?, wtd_fees = ?, "
            "feesout = ?, cum_feesOut = ?, wtd_feesOut = ?, "
            "exch_rate_base = ?, interval_type = ?, "
            "days_since_nond = ?, days_since_last = ?, "
            "change_date = ?, perform_type = ?, NotionalFlow = ?, Cons_fee = ?, Cum_Cons = ?, Wtd_Cons = ? "
            "WHERE id = ? and perform_date = ?"));

        nanodbc::timestamp tsPerform, tsChange, tsOldPerform;
        long_to_timestamp(zSD.lPerformDate, tsPerform);
        long_to_timestamp(zSD.fChangeDate, tsChange);
        long_to_timestamp(lOldPerformDate, tsOldPerform);

        int idx = 0;
        stmt.bind(idx++, &tsPerform);
        stmt.bind(idx++, &zSD.fMktVal);
        stmt.bind(idx++, &zSD.fBookValue);
        stmt.bind(idx++, &zSD.fAccrInc);
        stmt.bind(idx++, &zSD.fAccrDiv);
        stmt.bind(idx++, &zSD.fNetFlow);
        stmt.bind(idx++, &zSD.fCumFlow);
        stmt.bind(idx++, &zSD.fWtdFlow);
        stmt.bind(idx++, &zSD.fPurchases);
        stmt.bind(idx++, &zSD.fSales);
        stmt.bind(idx++, &zSD.fIncome);
        stmt.bind(idx++, &zSD.fCumIncome);
        stmt.bind(idx++, &zSD.fWtdInc);
        stmt.bind(idx++, &zSD.fFees);
        stmt.bind(idx++, &zSD.fCumFees);
        stmt.bind(idx++, &zSD.fWtdFees);
        stmt.bind(idx++, &zSD.fFeesOut);
        stmt.bind(idx++, &zSD.fCumFeesOut);
        stmt.bind(idx++, &zSD.fWtdFeesOut);
        stmt.bind(idx++, &zSD.fExchRateBase);
        stmt.bind(idx++, zSD.sIntervalType);
        stmt.bind(idx++, &zSD.lDaysSinceNond);
        stmt.bind(idx++, &zSD.lDaysSinceLast);
        stmt.bind(idx++, &tsChange);
        stmt.bind(idx++, zSD.sPerformType);
        stmt.bind(idx++, &zSD.fNotionalFlow);
        stmt.bind(idx++, &zSD.fCNFees);
        stmt.bind(idx++, &zSD.fCumCNFees);
        stmt.bind(idx++, &zSD.fWtdCNFees);

        stmt.bind(idx++, &zSD.iID);
        stmt.bind(idx++, &tsOldPerform);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zSD.iPortfolioID, 0, (char*)"", 0, -1, 0, (char*)"UpdateDailySummdata", FALSE);
    }
}
