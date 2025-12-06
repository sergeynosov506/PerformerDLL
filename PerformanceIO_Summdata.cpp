/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Summdata.cpp
 * DESCRIPTION: Summdata, Monthsum, and Bankstat implementations (9 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "PerformanceIO_Summdata.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "summdata.h"
#include "dateutils.h"
#include <optional>
#include <cstring>

extern thread_local nanodbc::connection gConn;

// Static state for multi-row cursor
struct SelectPeriodSummdataState {
    std::optional<nanodbc::result> result;
    int iPortfolioID = 0;
    long lPerformDate1 = 0;
    long lPerformDate2 = 0;
    int cRows = 0;
};

static SelectPeriodSummdataState g_selectPeriodSummdataState;

// ============================================================================
// DeleteSummdata
// ============================================================================
DLLAPI void STDCALL DeleteSummdata(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteSummdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM SUMMDATA WHERE id = ? AND perform_date >= ? AND perform_date <= ?"));

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
        *pzErr = PrintError((char*)e.what(), iID, 0, (char*)"", 0, -1, 0, (char*)"DeleteSummdata", FALSE);
    }
}

// ============================================================================
// DeleteSummdataForPortfolio
// ============================================================================
DLLAPI void STDCALL DeleteSummdataForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteSummdataForPortfolio", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM Summdata WHERE portfolio_id = ? AND perform_date >= ? AND perform_date <= ?"));

        nanodbc::timestamp tsBegin, tsEnd;
        long_to_timestamp(lBeginDate, tsBegin);
        
        if (lEndDate == -1)
            long_to_timestamp(MAXDATE, tsEnd);
        else
            long_to_timestamp(lEndDate, tsEnd);

        stmt.bind(0, &iPortID);
        stmt.bind(1, &tsBegin);
        stmt.bind(2, &tsEnd);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteSummdataForPortfolio", FALSE);
    }
}

// ============================================================================
// DeleteMonthSum
// ============================================================================
DLLAPI void STDCALL DeleteMonthSum(long iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMonthSum", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM MONTHSUM WHERE id = ? AND perform_date >= ? AND perform_date <= ?"));

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
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMonthSum", FALSE);
    }
}

// ============================================================================
// DeleteMonthSumForPortfolio
// ============================================================================
DLLAPI void STDCALL DeleteMonthSumForPortfolio(long iPortID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMonthSumForPortfolio", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM MONTHSUM WHERE portfolio_id = ? AND perform_date >= ? AND perform_date <= ?"));

        nanodbc::timestamp tsBegin, tsEnd;
        long_to_timestamp(lBeginDate, tsBegin);
        
        if (lEndDate == -1)
            long_to_timestamp(MAXDATE, tsEnd);
        else
            long_to_timestamp(lEndDate, tsEnd);

        stmt.bind(0, &iPortID);
        stmt.bind(1, &tsBegin);
        stmt.bind(2, &tsEnd);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMonthSumForPortfolio", FALSE);
    }
}

// ============================================================================
// DeleteBankstat
// ============================================================================
DLLAPI void STDCALL DeleteBankstat(int iID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteBankstat", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM BANKSTAT WHERE id = ? AND perform_date >= ? AND perform_date <= ?"));

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
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteBankstat", FALSE);
    }
}

// ============================================================================
// InsertPeriodSummdata
// ============================================================================
DLLAPI void STDCALL InsertPeriodSummdata(SUMMDATA zSD, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertPeriodSummdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "INSERT INTO SUMMDATA "
            "(portfolio_id, id, perform_date, mkt_val, accr_inc, accr_div, "
            "net_flow, cum_flow, income, cum_income, fees, cum_fees, "
            "feesout, cum_feesout, days_since_nond, perform_type, NotionalFlow, "
            "Cons_Fee, Cum_Cons, create_date, change_date) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        nanodbc::timestamp tsPerform, tsCreate, tsChange;
        long_to_timestamp(zSD.lPerformDate, tsPerform);
        long_to_timestamp(zSD.fCreateDate, tsCreate);
        long_to_timestamp(zSD.fChangeDate, tsChange);

        int idx = 0;
        stmt.bind(idx++, &zSD.iPortfolioID);
        stmt.bind(idx++, &zSD.iID);
        stmt.bind(idx++, &tsPerform);
        stmt.bind(idx++, &zSD.fMktVal);
        stmt.bind(idx++, &zSD.fAccrInc);
        stmt.bind(idx++, &zSD.fAccrDiv);
        
        stmt.bind(idx++, &zSD.fNetFlow);
        stmt.bind(idx++, &zSD.fCumFlow);
        stmt.bind(idx++, &zSD.fIncome);
        stmt.bind(idx++, &zSD.fCumIncome);
        stmt.bind(idx++, &zSD.fFees);
        stmt.bind(idx++, &zSD.fCumFees);
        
        stmt.bind(idx++, &zSD.fFeesOut);
        stmt.bind(idx++, &zSD.fCumFeesOut);
        stmt.bind(idx++, &zSD.lDaysSinceNond);
        stmt.bind(idx++, zSD.sPerformType);
        stmt.bind(idx++, &zSD.fNotionalFlow);
        
        stmt.bind(idx++, &zSD.fCNFees);
        stmt.bind(idx++, &zSD.fCumCNFees);
        stmt.bind(idx++, &tsCreate);
        stmt.bind(idx++, &tsChange);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zSD.iID, 0, (char*)"", 0, -1, 0, (char*)"InsertPeriodSummdata", FALSE);
    }
}

// ============================================================================
// InsertMonthlySummdata
// ============================================================================
DLLAPI void STDCALL InsertMonthlySummdata(SUMMDATA zSD, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMonthlySummdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "INSERT INTO MONTHSUM "
            "(portfolio_id, id, perform_date, mkt_val, accr_inc, accr_div, "
            "net_flow, cum_flow, income, cum_income, fees, cum_fees, "
            "feesout, cum_feesout, days_since_nond, perform_type, NotionalFlow, "
            "Cons_Fee, Cum_Cons, create_date, change_date) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        nanodbc::timestamp tsPerform, tsCreate, tsChange;
        long_to_timestamp(zSD.lPerformDate, tsPerform);
        long_to_timestamp(zSD.fCreateDate, tsCreate);
        long_to_timestamp(zSD.fChangeDate, tsChange);

        int idx = 0;
        stmt.bind(idx++, &zSD.iPortfolioID);
        stmt.bind(idx++, &zSD.iID);
        stmt.bind(idx++, &tsPerform);
        stmt.bind(idx++, &zSD.fMktVal);
        stmt.bind(idx++, &zSD.fAccrInc);
        stmt.bind(idx++, &zSD.fAccrDiv);
        
        stmt.bind(idx++, &zSD.fNetFlow);
        stmt.bind(idx++, &zSD.fCumFlow);
        stmt.bind(idx++, &zSD.fIncome);
        stmt.bind(idx++, &zSD.fCumIncome);
        stmt.bind(idx++, &zSD.fFees);
        stmt.bind(idx++, &zSD.fCumFees);
        
        stmt.bind(idx++, &zSD.fFeesOut);
        stmt.bind(idx++, &zSD.fCumFeesOut);
        stmt.bind(idx++, &zSD.lDaysSinceNond);
        stmt.bind(idx++, zSD.sPerformType);
        stmt.bind(idx++, &zSD.fNotionalFlow);
        
        stmt.bind(idx++, &zSD.fCNFees);
        stmt.bind(idx++, &zSD.fCumCNFees);
        stmt.bind(idx++, &tsCreate);
        stmt.bind(idx++, &tsChange);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zSD.iID, zSD.lPerformDate, (char*)"", 0, -1, 0, (char*)"InsertMonthlySummdata", FALSE);
    }
}

// ============================================================================
// SelectPeriodSummdata
// ============================================================================
DLLAPI void STDCALL SelectPeriodSummdata(SUMMDATA *pzSD, int iPortfolioID, long lPerformDate1, long lPerformDate2, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPeriodSummdata", FALSE);
        return;
    }

    try
    {
        if (g_selectPeriodSummdataState.iPortfolioID != iPortfolioID ||
            g_selectPeriodSummdataState.lPerformDate1 != lPerformDate1 ||
            g_selectPeriodSummdataState.lPerformDate2 != lPerformDate2 ||
            g_selectPeriodSummdataState.cRows == 0 ||
            !g_selectPeriodSummdataState.result)
        {
            g_selectPeriodSummdataState.result.reset();
            g_selectPeriodSummdataState.iPortfolioID = iPortfolioID;
            g_selectPeriodSummdataState.lPerformDate1 = lPerformDate1;
            g_selectPeriodSummdataState.lPerformDate2 = lPerformDate2;
            g_selectPeriodSummdataState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(
                "SELECT portfolio_id, id, perform_date, "
                "mkt_val, accr_inc, accr_div, net_flow, cum_flow, "
                "income, cum_income, fees, cum_fees, feesout, cum_feesout, "
                "days_since_nond, perform_type, NotionalFlow, Cons_Fee, Cum_Cons, 1 SortOrder "
                "FROM summdata "
                "WHERE portfolio_id = ? AND "
                "perform_date >= ? AND perform_date <= ? "
                "UNION SELECT portfolio_id, id, perform_date, "
                "mkt_val, accr_inc, accr_div, net_flow, cum_flow, "
                "income, cum_income, fees, cum_fees, feesout, cum_feesout, "
                "days_since_nond, perform_type, NotionalFlow, Cons_Fee, Cum_Cons, 0 SortOrder "
                "FROM dsumdata "
                "WHERE portfolio_id = ? AND "
                "perform_date >= ? AND perform_date <= ? "
                "ORDER BY id, perform_date, SortOrder"));

            nanodbc::timestamp ts1, ts2;
            long_to_timestamp(lPerformDate1, ts1);
            long_to_timestamp(lPerformDate2, ts2);

            stmt.bind(0, &iPortfolioID);
            stmt.bind(1, &ts1);
            stmt.bind(2, &ts2);
            stmt.bind(3, &iPortfolioID);
            stmt.bind(4, &ts1);
            stmt.bind(5, &ts2);

            g_selectPeriodSummdataState.result = nanodbc::execute(stmt);
        }

        if (g_selectPeriodSummdataState.result && g_selectPeriodSummdataState.result->next())
        {
            g_selectPeriodSummdataState.cRows++;
            memset(pzSD, 0, sizeof(*pzSD));

            pzSD->iPortfolioID = g_selectPeriodSummdataState.result->get<int>("portfolio_id", 0);
            pzSD->iID = g_selectPeriodSummdataState.result->get<int>("id", 0);
            
            nanodbc::timestamp ts;
            if (!g_selectPeriodSummdataState.result->is_null("perform_date"))
            {
                ts = g_selectPeriodSummdataState.result->get<nanodbc::timestamp>("perform_date");
                pzSD->lPerformDate = timestamp_to_long(ts);
            }

            pzSD->fMktVal = g_selectPeriodSummdataState.result->get<double>("mkt_val", 0.0);
            pzSD->fAccrInc = g_selectPeriodSummdataState.result->get<double>("accr_inc", 0.0);
            pzSD->fAccrDiv = g_selectPeriodSummdataState.result->get<double>("accr_div", 0.0);
            pzSD->fNetFlow = g_selectPeriodSummdataState.result->get<double>("net_flow", 0.0);
            pzSD->fCumFlow = g_selectPeriodSummdataState.result->get<double>("cum_flow", 0.0);

            pzSD->fIncome = g_selectPeriodSummdataState.result->get<double>("income", 0.0);
            pzSD->fCumIncome = g_selectPeriodSummdataState.result->get<double>("cum_income", 0.0);
            pzSD->fFees = g_selectPeriodSummdataState.result->get<double>("fees", 0.0);
            pzSD->fCumFees = g_selectPeriodSummdataState.result->get<double>("cum_fees", 0.0);
            pzSD->fFeesOut = g_selectPeriodSummdataState.result->get<double>("feesout", 0.0);
            pzSD->fCumFeesOut = g_selectPeriodSummdataState.result->get<double>("cum_feesout", 0.0);

            pzSD->lDaysSinceNond = g_selectPeriodSummdataState.result->get<long>("days_since_nond", 0);
            read_string(*g_selectPeriodSummdataState.result, "perform_type", pzSD->sPerformType, sizeof(pzSD->sPerformType));
            pzSD->fNotionalFlow = g_selectPeriodSummdataState.result->get<double>("NotionalFlow", 0.0);
            pzSD->fCNFees = g_selectPeriodSummdataState.result->get<double>("Cons_Fee", 0.0);
            pzSD->fCumCNFees = g_selectPeriodSummdataState.result->get<double>("Cum_Cons", 0.0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_selectPeriodSummdataState.cRows = 0;
            g_selectPeriodSummdataState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), iPortfolioID, 0, (char*)"", 0, -1, 0, (char*)"SelectPeriodSummdata", FALSE);
        g_selectPeriodSummdataState.result.reset();
    }
}

// ============================================================================
// UpdatePeriodSummdata
// ============================================================================
DLLAPI void STDCALL UpdatePeriodSummdata(SUMMDATA zSD, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdatePeriodSummdata", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE SUMMDATA SET "
            "perform_date = ?, mkt_val = ?, accr_inc = ?, accr_div = ?, "
            "net_flow = ?, cum_flow = ?, income = ?, cum_income = ?, "
            "fees = ?, cum_fees = ?, feesout = ?, cum_feesout = ?, "
            "days_since_nond = ?, perform_type = ?, NotionalFlow = ?, "
            "Cons_Fee = ?, Cum_Cons = ?, change_date = ? "
            "WHERE portfolio_id = ? AND id = ? AND perform_date = ?"));

        nanodbc::timestamp tsPerform, tsChange, tsOldPerform;
        long_to_timestamp(zSD.lPerformDate, tsPerform);
        long_to_timestamp(zSD.fChangeDate, tsChange);
        // Note: Legacy code uses m_vOldPerformDate in WHERE clause, but it seems to be initialized from zSD.lPerformDate?
        // Let's check legacy code: SETVARDATE(cmdUpdatePeriodSummdata.m_vOldPerformDate,zSD.lPerformDate);
        // Yes, it updates the record where perform_date matches the input date.
        long_to_timestamp(zSD.lPerformDate, tsOldPerform);

        int idx = 0;
        stmt.bind(idx++, &tsPerform);
        stmt.bind(idx++, &zSD.fMktVal);
        stmt.bind(idx++, &zSD.fAccrInc);
        stmt.bind(idx++, &zSD.fAccrDiv);
        
        stmt.bind(idx++, &zSD.fNetFlow);
        stmt.bind(idx++, &zSD.fCumFlow);
        stmt.bind(idx++, &zSD.fIncome);
        stmt.bind(idx++, &zSD.fCumIncome);
        
        stmt.bind(idx++, &zSD.fFees);
        stmt.bind(idx++, &zSD.fCumFees);
        stmt.bind(idx++, &zSD.fFeesOut);
        stmt.bind(idx++, &zSD.fCumFeesOut);
        
        stmt.bind(idx++, &zSD.lDaysSinceNond);
        stmt.bind(idx++, zSD.sPerformType);
        stmt.bind(idx++, &zSD.fNotionalFlow);
        
        stmt.bind(idx++, &zSD.fCNFees);
        stmt.bind(idx++, &zSD.fCumCNFees);
        stmt.bind(idx++, &tsChange);
        
        stmt.bind(idx++, &zSD.iPortfolioID);
        stmt.bind(idx++, &zSD.iID);
        stmt.bind(idx++, &tsOldPerform);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zSD.iID, 0, (char*)"", 0, -1, 0, (char*)"UpdatePeriodSummdata", FALSE);
    }
}
