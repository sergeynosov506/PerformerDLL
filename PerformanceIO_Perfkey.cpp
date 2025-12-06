/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Perfkey.cpp
 * DESCRIPTION: Performance Key implementations (5 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "PerformanceIO_Perfkey.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "perfkey.h"
#include "dateutils.h"
#include <optional>
#include <cstring>

extern thread_local nanodbc::connection gConn;

// Static state for multi-row cursor
struct SelectPerfkeysState {
    std::optional<nanodbc::result> result;
    int iPortfolioID = 0;
    int cRows = 0;
};

static SelectPerfkeysState g_selectPerfkeysState;

// ============================================================================
// DeletePerfkey
// ============================================================================
DLLAPI void STDCALL DeletePerfkey(long lPerfKeyNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeletePerfkey", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("DELETE FROM PERFKEY where perfkeyno = ?"));
        stmt.bind(0, &lPerfKeyNo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), lPerfKeyNo, 0, (char*)"", 0, -1, 0, (char*)"DeletePerfkey", FALSE);
    }
}

// ============================================================================
// InsertPerfkey
// ============================================================================
DLLAPI void STDCALL InsertPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertPerfkey", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "INSERT INTO PERFKEY "
            "(portfolioid, id, ruleno, scrhdrno, currencyprocessing, "
            "totalrecordindicator, parentchildindicator, parentkeyno, initperfdate, "
            "nondailyperfdate, monthendperfdate, lastperfdate, description, "
            "deletedate, permanentdeleteflag) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        nanodbc::timestamp tsInit, tsNonDaily, tsMonthEnd, tsLast, tsDelete;
        long_to_timestamp(zPK.lInitPerfDate, tsInit);
        long_to_timestamp(zPK.lLndPerfDate, tsNonDaily);
        long_to_timestamp(zPK.lMePerfDate, tsMonthEnd);
        long_to_timestamp(zPK.lLastPerfDate, tsLast);
        long_to_timestamp(zPK.lDeleteDate, tsDelete);

        // Legacy code hardcoded "N" for permanentdeleteflag
        char sPermDelFlg[] = "N";

        int idx = 0;
        stmt.bind(idx++, &zPK.iPortfolioID);
        stmt.bind(idx++, &zPK.iID);
        stmt.bind(idx++, &zPK.lRuleNo);
        stmt.bind(idx++, &zPK.lScrhdrNo);
        stmt.bind(idx++, zPK.sCurrProc);

        stmt.bind(idx++, zPK.sTotalRecInd);
        stmt.bind(idx++, zPK.sParentChildInd);
        stmt.bind(idx++, &zPK.lParentPerfkeyNo);
        stmt.bind(idx++, &tsInit);

        stmt.bind(idx++, &tsNonDaily);
        stmt.bind(idx++, &tsMonthEnd);
        stmt.bind(idx++, &tsLast);
        stmt.bind(idx++, zPK.sDescription);

        stmt.bind(idx++, &tsDelete);
        stmt.bind(idx++, sPermDelFlg);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zPK.lPerfkeyNo, 0, (char*)"", 0, -1, 0, (char*)"InsertPerfkey", FALSE);
    }
}

// ============================================================================
// SelectPerfkeys
// ============================================================================
DLLAPI void STDCALL SelectPerfkeys(PERFKEY *pzPK, int iPortfolioID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPerfkeys", FALSE);
        return;
    }

    try
    {
        // Check if we need to reopen the query
        if (g_selectPerfkeysState.iPortfolioID != iPortfolioID ||
            g_selectPerfkeysState.cRows == 0 ||
            !g_selectPerfkeysState.result)
        {
            g_selectPerfkeysState.result.reset();
            g_selectPerfkeysState.iPortfolioID = iPortfolioID;
            g_selectPerfkeysState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(
                "SELECT perfkeyno, a.portfolioid, id, a.ruleno, ScrhdrNo, "
                "CurrencyProcessing, a.TotalRecordIndicator, ParentChildIndicator, "
                "parentkeyno, InitPerfDate, NonDailyPerfDate, MonthEndPerfDate, "
                "LastPerfDate, a.description, a.DeleteDate "
                "FROM perfkey a join perfrule b on a.portfolioid = b.portfolioid and a.ruleno = b.ruleno "
                "WHERE a.portfolioid = ? AND PermanentDeleteFlag <> 'Y' "
                "ORDER BY a.RuleNo, Scrhdrno, InitPerfDate"));

            stmt.bind(0, &iPortfolioID);
            g_selectPerfkeysState.result = nanodbc::execute(stmt);
        }

        if (g_selectPerfkeysState.result && g_selectPerfkeysState.result->next())
        {
            g_selectPerfkeysState.cRows++;
            memset(pzPK, 0, sizeof(*pzPK));

            pzPK->lPerfkeyNo = g_selectPerfkeysState.result->get<long>("perfkeyno", 0);
            pzPK->iPortfolioID = g_selectPerfkeysState.result->get<int>("portfolioid", 0);
            pzPK->iID = g_selectPerfkeysState.result->get<int>("id", 0);
            pzPK->lRuleNo = g_selectPerfkeysState.result->get<long>("ruleno", 0);
            pzPK->lScrhdrNo = g_selectPerfkeysState.result->get<long>("ScrhdrNo", 0);

            read_string(*g_selectPerfkeysState.result, "CurrencyProcessing", pzPK->sCurrProc, sizeof(pzPK->sCurrProc));
            read_string(*g_selectPerfkeysState.result, "TotalRecordIndicator", pzPK->sTotalRecInd, sizeof(pzPK->sTotalRecInd));
            read_string(*g_selectPerfkeysState.result, "ParentChildIndicator", pzPK->sParentChildInd, sizeof(pzPK->sParentChildInd));

            pzPK->lParentPerfkeyNo = g_selectPerfkeysState.result->get<long>("parentkeyno", 0);

            nanodbc::timestamp ts;
            
            if (!g_selectPerfkeysState.result->is_null("InitPerfDate"))
            {
                ts = g_selectPerfkeysState.result->get<nanodbc::timestamp>("InitPerfDate");
                pzPK->lInitPerfDate = timestamp_to_long(ts);
            }

            if (!g_selectPerfkeysState.result->is_null("NonDailyPerfDate"))
            {
                ts = g_selectPerfkeysState.result->get<nanodbc::timestamp>("NonDailyPerfDate");
                pzPK->lLndPerfDate = timestamp_to_long(ts);
            }

            if (!g_selectPerfkeysState.result->is_null("MonthEndPerfDate"))
            {
                ts = g_selectPerfkeysState.result->get<nanodbc::timestamp>("MonthEndPerfDate");
                pzPK->lMePerfDate = timestamp_to_long(ts);
            }

            if (!g_selectPerfkeysState.result->is_null("LastPerfDate"))
            {
                ts = g_selectPerfkeysState.result->get<nanodbc::timestamp>("LastPerfDate");
                pzPK->lLastPerfDate = timestamp_to_long(ts);
            }

            read_string(*g_selectPerfkeysState.result, "description", pzPK->sDescription, sizeof(pzPK->sDescription));

            if (!g_selectPerfkeysState.result->is_null("DeleteDate"))
            {
                ts = g_selectPerfkeysState.result->get<nanodbc::timestamp>("DeleteDate");
                pzPK->lDeleteDate = timestamp_to_long(ts);
            }
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_selectPerfkeysState.cRows = 0;
            g_selectPerfkeysState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPerfkeys", FALSE);
        g_selectPerfkeysState.result.reset();
    }
}

// ============================================================================
// UpdateNewPerfkey
// ============================================================================
DLLAPI void STDCALL UpdateNewPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateNewPerfkey", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE PERFKEY SET "
            "Parentkeyno = ?, InitPerfDate = ?, NonDailyPerfDate = ?, "
            "MonthEndPerfDate = ?, LastPerfDate = ?, DeleteDate = ? "
            "WHERE perfkeyno = ?"));

        nanodbc::timestamp tsInit, tsNonDaily, tsMonthEnd, tsLast, tsDelete;
        long_to_timestamp(zPK.lInitPerfDate, tsInit);
        long_to_timestamp(zPK.lLndPerfDate, tsNonDaily);
        long_to_timestamp(zPK.lMePerfDate, tsMonthEnd);
        long_to_timestamp(zPK.lLastPerfDate, tsLast);
        long_to_timestamp(zPK.lDeleteDate, tsDelete);

        int idx = 0;
        stmt.bind(idx++, &zPK.lParentPerfkeyNo);
        stmt.bind(idx++, &tsInit);
        stmt.bind(idx++, &tsNonDaily);
        stmt.bind(idx++, &tsMonthEnd);
        stmt.bind(idx++, &tsLast);
        stmt.bind(idx++, &tsDelete);
        stmt.bind(idx++, &zPK.lPerfkeyNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zPK.lPerfkeyNo, 0, (char*)"", 0, -1, 0, (char*)"UpdateNewPerfkey", FALSE);
    }
}

// ============================================================================
// UpdateOldPerfkey
// ============================================================================
DLLAPI void STDCALL UpdateOldPerfkey(PERFKEY zPK, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateOldPerfkey", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE PERFKEY SET "
            "NonDailyPerfDate = ?, "
            "MonthEndPerfDate = ?, LastPerfDate = ?, DeleteDate = ? "
            "WHERE perfkeyno = ?"));

        nanodbc::timestamp tsNonDaily, tsMonthEnd, tsLast, tsDelete;
        long_to_timestamp(zPK.lLndPerfDate, tsNonDaily);
        long_to_timestamp(zPK.lMePerfDate, tsMonthEnd);
        long_to_timestamp(zPK.lLastPerfDate, tsLast);
        long_to_timestamp(zPK.lDeleteDate, tsDelete);

        int idx = 0;
        stmt.bind(idx++, &tsNonDaily);
        stmt.bind(idx++, &tsMonthEnd);
        stmt.bind(idx++, &tsLast);
        stmt.bind(idx++, &tsDelete);
        stmt.bind(idx++, &zPK.lPerfkeyNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zPK.lPerfkeyNo, 0, (char*)"", 0, -1, 0, (char*)"UpdateOldPerfkey", FALSE);
    }
}
