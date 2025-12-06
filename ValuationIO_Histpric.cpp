/**
 * 
 * SUB-SYSTEM: Database Input/Output for Valuation
 * 
 * FILENAME: ValuationIO_Histpric.cpp
 * 
 * DESCRIPTION: Historical price query implementations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: Simple single-record queries (no cursor state needed)
 *        3 separate tables: histpric, histeqty, histfinc
 *        
 * AUTHOR: Modernized 2025-11-29
 *
 **/

#include "commonheader.h"
#include "ValuationIO_Histpric.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "ValuationIO.h"
#include "equities.h"
#include <cstring>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SQL Queries
// ============================================================================

const char* SQL_SELECT_HISTFINC =
    "SELECT sec_no, wi, price_date, id, "
    "accr_int, bond_eq_yld, yld_to_worst, ytw_type, yld_to_best, ytb_type, "
    "yld_to_earliest, yte_type, cur_dur, cur_mod_dur, convexity, variable_rate, cur_yld, cur_ytm "
    "FROM histfinc "
    "WHERE sec_no = ? AND wi = ? AND price_date = ?";

const char* SQL_SELECT_HISTEQTY =
    "SELECT sec_no, wi, price_date, eps "
    "FROM histeqty "
    "WHERE sec_no = ? AND wi = ? AND price_date = ?";

const char* SQL_SELECT_HISTPRIC =
    "SELECT sec_no, wi, price_date, "
    "id, date_price_updated, date_exrate_updated, "
    "price_source, price_exchange, close_price, bid_price, "
    "ask_price, high_price, low_price, exrate, inc_exrate, ann_div_cpn, volume "
    "FROM histpric "
    "WHERE sec_no = ? AND wi = ? AND price_date = ?";

// ============================================================================
// Helper Functions
// ============================================================================

void InitializeHistPric(HISTPRIC *pzHP)
{
    memset(pzHP, 0, sizeof(*pzHP));
    pzHP->fExrate = 1.0;
    pzHP->fIncExrate = 1.0;
}

void InitializePriceInfo(PRICEINFO *pzPInfo)
{
    memset(pzPInfo, 0, sizeof(*pzPInfo));
    pzPInfo->fExrate = 1.0;
    pzPInfo->fIncExrate = 1.0;
}

// ============================================================================
// SelectOneHistfinc
// ============================================================================

DLLAPI void STDCALL SelectOneHistfinc(char *sSecNo, char *sWi, long lPriceDate, 
    HISTFINC *pzHFinc, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectOneHistfinc", FALSE);
#endif

    InitializeErrStruct(pzErr);
    memset(pzHFinc, 0, sizeof(*pzHFinc));

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHistfinc", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_HISTFINC));

        nanodbc::timestamp tsPriceDate;
        long_to_timestamp(lPriceDate, tsPriceDate);

        stmt.bind(0, sSecNo);
        stmt.bind(1, sWi);
        stmt.bind(2, &tsPriceDate);

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "sec_no", pzHFinc->sSecNo, sizeof(pzHFinc->sSecNo));
            read_string(result, "wi", pzHFinc->sWi, sizeof(pzHFinc->sWi));
            read_date(result, "price_date", &pzHFinc->lPriceDate);
            pzHFinc->iId = result.get<int>("id", 0);
            
            pzHFinc->fAccrInt = result.get<double>("accr_int", 0.0);
            pzHFinc->fBondEqYld = result.get<double>("bond_eq_yld", 0.0);
            pzHFinc->fYldToWorst = result.get<double>("yld_to_worst", 0.0);
            read_string(result, "ytw_type", pzHFinc->sYtwType, sizeof(pzHFinc->sYtwType));
            pzHFinc->fYldToBest = result.get<double>("yld_to_best", 0.0);
            read_string(result, "ytb_type", pzHFinc->sYtbType, sizeof(pzHFinc->sYtbType));
            pzHFinc->fYldToEarliest = result.get<double>("yld_to_earliest", 0.0);
            read_string(result, "yte_type", pzHFinc->sYteType, sizeof(pzHFinc->sYteType));
            pzHFinc->fCurDur = result.get<double>("cur_dur", 0.0);
            pzHFinc->fCurModDur = result.get<double>("cur_mod_dur", 0.0);
            pzHFinc->fConvexity = result.get<double>("convexity", 0.0);
            pzHFinc->fVariableRate = result.get<double>("variable_rate", 0.0);
            pzHFinc->fCurYld = result.get<double>("cur_yld", 0.0);
            pzHFinc->fCurYtm = result.get<double>("cur_ytm", 0.0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectOneHistfinc: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHistfinc", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectOneHistfinc", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHistfinc", FALSE);
    }
}

// ============================================================================
// SelectOneHisteqty
// ============================================================================

DLLAPI void STDCALL SelectOneHisteqty(char *sSecNo, char *sWi, long lPriceDate, 
    PRICEINFO *pzPInfo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectOneHisteqty", FALSE);
#endif

    InitializeErrStruct(pzErr);
    InitializePriceInfo(pzPInfo);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHisteqty", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_HISTEQTY));

        nanodbc::timestamp tsPriceDate;
        long_to_timestamp(lPriceDate, tsPriceDate);

        stmt.bind(0, sSecNo);
        stmt.bind(1, sWi);
        stmt.bind(2, &tsPriceDate);

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
            pzPInfo->fEps = result.get<double>("eps", 0.0);
            pzPInfo->lPriceDate = lPriceDate;
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectOneHisteqty: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHisteqty", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectOneHisteqty", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHisteqty", FALSE);
    }
}

// ============================================================================
// SelectOneHistpric
// ============================================================================

DLLAPI void STDCALL SelectOneHistpric(char *sSecNo, char *sWi, long lPriceDate, 
    HISTPRIC *pzHPric, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectOneHistpric", FALSE);
#endif

    InitializeErrStruct(pzErr);
    InitializeHistPric(pzHPric);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHistpric", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_HISTPRIC));

        nanodbc::timestamp tsPriceDate;
        long_to_timestamp(lPriceDate, tsPriceDate);

        stmt.bind(0, sSecNo);
        stmt.bind(1, sWi);
        stmt.bind(2, &tsPriceDate);

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "sec_no", pzHPric->sSecNo, sizeof(pzHPric->sSecNo));
            read_string(result, "wi", pzHPric->sWi, sizeof(pzHPric->sWi));
            read_date(result, "price_date", &pzHPric->lPriceDate);
            
            pzHPric->iId = result.get<int>("id", 0);
            read_date(result, "date_price_updated", &pzHPric->lDatePriceUpdated);
            read_date(result, "date_exrate_updated", &pzHPric->lDateExrateUpdated);
            
            read_string(result, "price_source", pzHPric->sPriceSource, sizeof(pzHPric->sPriceSource));
            read_string(result, "price_exchange", pzHPric->sPriceExchange, sizeof(pzHPric->sPriceExchange));
            
            pzHPric->fClosePrice = result.get<double>("close_price", 0.0);
            pzHPric->fBidPrice = result.get<double>("bid_price", 0.0);
            pzHPric->fAskPrice = result.get<double>("ask_price", 0.0);
            pzHPric->fHighPrice = result.get<double>("high_price", 0.0);
            pzHPric->fLowPrice = result.get<double>("low_price", 0.0);
            pzHPric->fExrate = result.get<double>("exrate", 1.0);
            pzHPric->fIncExrate = result.get<double>("inc_exrate", 1.0);
            pzHPric->fAnnDivCpn = result.get<double>("ann_div_cpn", 0.0);
            pzHPric->iVolume = result.get<int>("volume", 0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectOneHistpric: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHistpric", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectOneHistpric", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneHistpric", FALSE);
    }
}
