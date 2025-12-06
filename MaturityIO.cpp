/**
 * 
 * SUB-SYSTEM: Database Input/Output for Payments  
 * 
 * FILENAME: MaturityIO.cpp
 * 
 * DESCRIPTION: Maturity and forward contract processing functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: Multi-row cursor pattern using static state
 *        Mode-based query routing (None/Account/Security)
 *        
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#include "commonheader.h"
#include "MaturityIO.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "PaymentsIO.h"
#include "assets.h"
#include "portmain.h"
#include "sectype.h"
#include <optional>
#include <cstring>
#include <string>

extern thread_local nanodbc::connection gConn;
extern char sHoldings[];  // Global holdings table name

// ============================================================================
// SQL Queries
// ============================================================================

const char* SQL_MATURITY_UNLOAD_BASE = 
    "SELECT h.id hid, h.sec_no, h.wi, h.sec_xtend, h.acct_type, h.trans_no, h.units, "
    "h.orig_face, h.sec_symbol, h.tot_cost, "
    "a.id aid, a.sec_type, a.trad_unit, a.curr_id, a.inc_curr_id, a.cur_exrate, a.cur_inc_exrate, "
    "f.maturity_date, f.redemption_price, "
    "hp1.exrate secexrate, hp1.inc_exrate secincexrate, "
    "hp2.exrate currexrate, hp2.inc_exrate currincexrate, "
    "CASE WHEN (hf.VariableRate IS NULL) OR (hf.VariableRate = 0) THEN 1 ELSE hf.VariableRate END VariableRate "
    "FROM Holdings h, assets a "
    "JOIN fixedinc f ON a.sec_no = f.sec_no AND a.when_issue = f.wi "
    "LEFT OUTER JOIN histpric hp1 ON a.sec_no = hp1.sec_no AND a.when_issue = hp1.wi AND f.maturity_date = hp1.price_date "
    "LEFT OUTER JOIN histpric hp2 ON a.curr_id = hp2.sec_no AND f.maturity_date = hp2.price_date "
    "LEFT OUTER JOIN histfinc hf ON a.sec_no = hf.sec_no AND a.when_issue = hf.wi AND f.maturity_date = hf.price_date "
    "WHERE h.sec_no = a.sec_no AND h.wi = a.when_issue AND "
    "a.when_issue <> 'W' AND a.auto_mature = 'Y' AND "
    "h.trd_date IS NOT NULL AND h.sec_no = f.sec_no AND h.wi = f.wi AND "
    "h.sec_xtend != 'TS' AND h.sec_xtend != 'TL' AND "
    "f.maturity_date >= ? AND f.maturity_date <= ?";

const char* SQL_MATURITY_MODE_ACCOUNT = " AND h.id = ?";

const char* SQL_MATURITY_MODE_SECURITY = " AND h.id = ? AND h.sec_no = ? AND h.wi = ? AND h.sec_xtend = ? AND h.acct_type = ?";

const char* SQL_FORWARD_UNLOAD_BASE =
    "SELECT h.id hid, h.sec_no, h.wi, h.sec_xtend, h.acct_type, "
    "h.trans_no, h.units, h.orig_face, "
    "a.id aid, a.sec_type, a.trad_unit, a.curr_id, a.inc_curr_id, a.cur_exrate, a.cur_inc_exrate, "
    "d.exp_date, h.open_liability, h.cur_liability "
    "FROM %HOLDINGS_TABLE% h, assets a, deriv d "
    "WHERE h.sec_no = a.sec_no AND h.wi = a.when_issue AND "
    "a.when_issue <> 'Y' AND a.auto_mature = 'Y' AND "
    "h.trd_date IS NOT NULL AND "
    "h.sec_xtend != 'TS' AND h.sec_xtend != 'TL' AND "
    "d.exp_date BETWEEN ? AND ? AND "
    "h.sec_no = d.sec_no AND h.wi = d.when_issue";

const char* SQL_SELECT_ALL_SECTYPES =
    "SELECT sec_type, sec_type_desc, primary_type, secondary_type, "
    "third_type, stl_days, sec_fee_flag, pay_type, "
    "accrual_sched, commission_flag, position_ind, "
    "lot_ind, cost_ind, lot_exists_ind, avg_ind, mktval_ind, intcalc "
    "FROM sectype ORDER BY sec_type";

// ============================================================================
// Static State for Multi-Row Cursors
// ============================================================================

struct MaturityState {
    std::optional<nanodbc::result> result;
    long lStartDate = 0;
    long lEndDate = 0;
    char sMode[2] = {0};
    int iID = 0;
    char sSecNo[20] = {0};
    char sWi[2] = {0};
    char sSecXtend[3] = {0};
    char sAcctType[2] = {0};
    int cRows = 0;
};

struct ForwardState {
    std::optional<nanodbc::result> result;
    long lStartDate = 0;
    long lEndDate = 0;
    char sMode[2] = {0};
    int iID = 0;
    char sSecNo[20] = {0};
    char sWi[2] = {0};
    char sSecXtend[3] = {0};
    char sAcctType[2] = {0};
    char sHoldingsTable[40] = {0};
    int cRows = 0;
};

struct SectypeState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

static MaturityState g_maturityState;
static ForwardState g_forwardState;
static SectypeState g_sectypeState;

// ============================================================================
// Helper Functions
// ============================================================================

static void FillMatStruct(nanodbc::result& result, MATSTRUCT* pzMS)
{
    memset(pzMS, 0, sizeof(MATSTRUCT));
    
    pzMS->iID = result.get<int>("hid", 0);
    read_string(result, "sec_no", pzMS->sSecNo, sizeof(pzMS->sSecNo));
    read_string(result, "wi", pzMS->sWi, sizeof(pzMS->sWi));
    read_string(result, "sec_xtend", pzMS->sSecXtend, sizeof(pzMS->sSecXtend));
    read_string(result, "acct_type", pzMS->sAcctType, sizeof(pzMS->sAcctType));
    pzMS->lTransNo = result.get<long>("trans_no", 0);
    pzMS->fUnits = result.get<double>("units", 0.0);
    pzMS->fOrigFace = result.get<double>("orig_face", 0.0);
    read_string(result, "sec_symbol", pzMS->sSecSymbol, sizeof(pzMS->sSecSymbol));
    pzMS->fTotCost = result.get<double>("tot_cost", 0.0);
    
    pzMS->iSecID = result.get<int>("aid", 0);
    pzMS->iSecType = result.get<int>("sec_type", 0);
    pzMS->fTrdUnit = result.get<double>("trad_unit", 0.0);
    read_string(result, "curr_id", pzMS->sCurrId, sizeof(pzMS->sCurrId));
    read_string(result, "inc_curr_id", pzMS->sIncCurrId, sizeof(pzMS->sIncCurrId));
    pzMS->fCurExrate = result.get<double>("cur_exrate", 0.0);
    pzMS->fCurIncExrate = result.get<double>("cur_inc_exrate", 0.0);
    
    // Date conversion
    read_date(result, "maturity_date", &pzMS->lMaturityDate);
    
    pzMS->fRedemptionPrice = result.get<double>("redemption_price", 0.0);
    pzMS->fVariableRate = result.get<double>("VariableRate", 1.0);
    
    // Exchange rate priority: security rate > currency rate > asset rate
    double fSecExrate = result.get<double>("secexrate", 0.0);
    double fSecIncExrate = result.get<double>("secincexrate", 0.0);
    double fCurrExrate = result.get<double>("currexrate", 0.0);
    double fCurrIncExrate = result.get<double>("currincexrate", 0.0);
    
    if (fSecExrate > 0) {
        pzMS->fCurExrate = fSecExrate;
        pzMS->fCurIncExrate = fSecIncExrate;
    } else if (fCurrExrate > 0) {
        pzMS->fCurExrate = fCurrExrate;
        pzMS->fCurIncExrate = fCurrIncExrate;
    }
}

static void FillFMatStruct(nanodbc::result& result, FMATSTRUCT* pzMS)
{
    memset(pzMS, 0, sizeof(FMATSTRUCT));
    
    pzMS->iID = result.get<int>("hid", 0);
    read_string(result, "sec_no", pzMS->sSecNo, sizeof(pzMS->sSecNo));
    read_string(result, "wi", pzMS->sWi, sizeof(pzMS->sWi));
    read_string(result, "sec_xtend", pzMS->sSecXtend, sizeof(pzMS->sSecXtend));
    read_string(result, "acct_type", pzMS->sAcctType, sizeof(pzMS->sAcctType));
    
    pzMS->lTransNo = result.get<long>("trans_no", 0);
    pzMS->fUnits = result.get<double>("units", 0.0);
    pzMS->fOrigFace = result.get<double>("orig_face", 0.0);
    
    pzMS->iSecID = result.get<int>("aid", 0);
    pzMS->iSecType = result.get<int>("sec_type", 0);
    pzMS->fTrdUnit = result.get<double>("trad_unit", 0.0);
    read_string(result, "curr_id", pzMS->sCurrId, sizeof(pzMS->sCurrId));
    read_string(result, "inc_curr_id", pzMS->sIncCurrId, sizeof(pzMS->sIncCurrId));
    pzMS->fCurExrate = result.get<double>("cur_exrate", 0.0);
    pzMS->fCurIncExrate = result.get<double>("cur_inc_exrate", 0.0);
    
    // Date conversion
    read_date(result, "exp_date", &pzMS->lExpDate);
    
    pzMS->fOpenliability = result.get<double>("open_liability", 0.0);
    pzMS->fCurliability = result.get<double>("cur_liability", 0.0);
}

static void FillSectypeStruct(nanodbc::result& result, SECTYPE* pzST)
{
    memset(pzST, 0, sizeof(SECTYPE));
    
    pzST->iSecType = result.get<int>("sec_type", 0);
    read_string(result, "sec_type_desc", pzST->sSecTypeDesc, sizeof(pzST->sSecTypeDesc));
    read_string(result, "primary_type", pzST->sPrimaryType, sizeof(pzST->sPrimaryType));
    read_string(result, "secondary_type", pzST->sSecondaryType, sizeof(pzST->sSecondaryType));
    read_string(result, "third_type", pzST->sThirdType, sizeof(pzST->sThirdType));
    
    pzST->iStlDays = result.get<int>("stl_days", 0);
    read_string(result, "sec_fee_flag", pzST->sSecFeeFlag, sizeof(pzST->sSecFeeFlag));
    read_string(result, "pay_type", pzST->sPayType, sizeof(pzST->sPayType));
    
    pzST->iAccrualSched = result.get<int>("accrual_sched", 0);
    read_string(result, "commission_flag", pzST->sCommissionFlag, sizeof(pzST->sCommissionFlag));
    read_string(result, "position_ind", pzST->sPositionInd, sizeof(pzST->sPositionInd));
    
    read_string(result, "lot_ind", pzST->sLotInd, sizeof(pzST->sLotInd));
    read_string(result, "cost_ind", pzST->sCostInd, sizeof(pzST->sCostInd));
    read_string(result, "lot_exists_ind", pzST->sLotExistsInd, sizeof(pzST->sLotExistsInd));
    read_string(result, "avg_ind", pzST->sAvgInd, sizeof(pzST->sAvgInd));
    read_string(result, "mktval_ind", pzST->sMktValInd, sizeof(pzST->sMktValInd));
    pzST->iIntcalc = result.get<int>("intcalc", 0);
}

// ============================================================================
// MaturityUnload
// ============================================================================

DLLAPI void STDCALL MaturityUnload(MATSTRUCT *pzMS, long lStartDate, long lEndDate, 
    char *sMode, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
    char *sAcctType, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "MaturityUnload", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"MaturityUnload", FALSE);
        return;
    }

    try
    {
        // Build SQL based on mode
        std::string sql = SQL_MATURITY_UNLOAD_BASE;
        if (sMode[0] == 'A') {
            sql += SQL_MATURITY_MODE_ACCOUNT;
        } else if (sMode[0] == 'S') {
            sql += SQL_MATURITY_MODE_SECURITY;
        }
        
        // Check if parameters changed
        bool needNewQuery = !(
            g_maturityState.lStartDate == lStartDate &&
            g_maturityState.lEndDate == lEndDate &&
            g_maturityState.sMode[0] == sMode[0] &&
            g_maturityState.iID == iID &&
            (sMode[0] != 'S' || (
                strcmp(g_maturityState.sSecNo, sSecNo) == 0 &&
                strcmp(g_maturityState.sWi, sWi) == 0 &&
                strcmp(g_maturityState.sSecXtend, sSecXtend) == 0 &&
                strcmp(g_maturityState.sAcctType, sAcctType) == 0
            )) &&
            g_maturityState.cRows > 0
        );

        if (needNewQuery)
        {
            g_maturityState.result.reset();
            g_maturityState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

            nanodbc::timestamp tsStart, tsEnd;
            long_to_timestamp(lStartDate, tsStart);
            long_to_timestamp(lEndDate, tsEnd);

            int paramIndex = 0;
            stmt.bind(paramIndex++, &tsStart);
            stmt.bind(paramIndex++, &tsEnd);
            
            if (sMode[0] == 'A') {
                stmt.bind(paramIndex++, &iID);
            } else if (sMode[0] == 'S') {
                stmt.bind(paramIndex++, &iID);
                stmt.bind(paramIndex++, sSecNo);
                stmt.bind(paramIndex++, sWi);
                stmt.bind(paramIndex++, sSecXtend);
                stmt.bind(paramIndex++, sAcctType);
            }

            g_maturityState.result = nanodbc::execute(stmt);
            g_maturityState.lStartDate = lStartDate;
            g_maturityState.lEndDate = lEndDate;
            g_maturityState.sMode[0] = sMode[0];
            g_maturityState.iID = iID;
            if (sMode[0] == 'S') {
                strcpy_s(g_maturityState.sSecNo, sSecNo);
                strcpy_s(g_maturityState.sWi, sWi);
                strcpy_s(g_maturityState.sSecXtend, sSecXtend);
                strcpy_s(g_maturityState.sAcctType, sAcctType);
            }
        }

        if (g_maturityState.result && g_maturityState.result->next())
        {
            g_maturityState.cRows++;
            FillMatStruct(*g_maturityState.result, pzMS);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_maturityState.cRows = 0;
            g_maturityState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in MaturityUnload: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"MaturityUnload", FALSE);
        g_maturityState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in MaturityUnload", 0, 0, (char*)"", 0, -1, 0, (char*)"MaturityUnload", FALSE);
        g_maturityState.result.reset();
    }
}

// ============================================================================
// ForwardMaturityUnload
// ============================================================================

DLLAPI void STDCALL ForwardMaturityUnload(FMATSTRUCT *pzMS, long lStartDate, long lEndDate, 
    char *sMode, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
    char *sAcctType, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "ForwardMaturityUnload", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"ForwardMaturityUnload", FALSE);
        return;
    }

    try
    {
        // Build SQL with table name substitution
        std::string sql = SQL_FORWARD_UNLOAD_BASE;
        
        // Replace %HOLDINGS_TABLE% with actual table name
        size_t pos = sql.find("%HOLDINGS_TABLE%");
        if (pos != std::string::npos)
            sql.replace(pos, 16, sHoldings);
        
        // Add mode-specific filters
        if (sMode[0] == 'A') {
            sql += SQL_MATURITY_MODE_ACCOUNT;
        } else if (sMode[0] == 'S') {
            sql += SQL_MATURITY_MODE_SECURITY;
        }
        
        // Check if parameters changed
        bool needNewQuery = !(
            g_forwardState.lStartDate == lStartDate &&
            g_forwardState.lEndDate == lEndDate &&
            g_forwardState.sMode[0] == sMode[0] &&
            g_forwardState.iID == iID &&
            strcmp(g_forwardState.sHoldingsTable, sHoldings) == 0 &&
            (sMode[0] != 'S' || (
                strcmp(g_forwardState.sSecNo, sSecNo) == 0 &&
                strcmp(g_forwardState.sWi, sWi) == 0 &&
                strcmp(g_forwardState.sSecXtend, sSecXtend) == 0 &&
                strcmp(g_forwardState.sAcctType, sAcctType) == 0
            )) &&
            g_forwardState.cRows > 0
        );

        if (needNewQuery)
        {
            g_forwardState.result.reset();
            g_forwardState.cRows = 0;

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

            nanodbc::timestamp tsStart, tsEnd;
            long_to_timestamp(lStartDate, tsStart);
            long_to_timestamp(lEndDate, tsEnd);

            int paramIndex = 0;
            stmt.bind(paramIndex++, &tsStart);
            stmt.bind(paramIndex++, &tsEnd);
            
            if (sMode[0] == 'A') {
                stmt.bind(paramIndex++, &iID);
            } else if (sMode[0] == 'S') {
                stmt.bind(paramIndex++, &iID);
                stmt.bind(paramIndex++, sSecNo);
                stmt.bind(paramIndex++, sWi);
                stmt.bind(paramIndex++, sSecXtend);
                stmt.bind(paramIndex++, sAcctType);
            }

            g_forwardState.result = nanodbc::execute(stmt);
            g_forwardState.lStartDate = lStartDate;
            g_forwardState.lEndDate = lEndDate;
            g_forwardState.sMode[0] = sMode[0];
            g_forwardState.iID = iID;
            strcpy_s(g_forwardState.sHoldingsTable, sHoldings);
            if (sMode[0] == 'S') {
                strcpy_s(g_forwardState.sSecNo, sSecNo);
                strcpy_s(g_forwardState.sWi, sWi);
                strcpy_s(g_forwardState.sSecXtend, sSecXtend);
                strcpy_s(g_forwardState.sAcctType, sAcctType);
            }
        }

        if (g_forwardState.result && g_forwardState.result->next())
        {
            g_forwardState.cRows++;
            FillFMatStruct(*g_forwardState.result, pzMS);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_forwardState.cRows = 0;
            g_forwardState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in ForwardMaturityUnload: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"ForwardMaturityUnload", FALSE);
        g_forwardState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in ForwardMaturityUnload", 0, 0, (char*)"", 0, -1, 0, (char*)"ForwardMaturityUnload", FALSE);
        g_forwardState.result.reset();
    }
}

// ============================================================================
// SelectAllSectypes
// ============================================================================

DLLAPI void STDCALL SelectAllSectypes(SECTYPE *pzST, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllSectypes", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSectypes", FALSE);
        return;
    }

    try
    {
        if (!g_sectypeState.result)
        {
            g_sectypeState.cRows = 0;
            g_sectypeState.result = nanodbc::execute(gConn, NANODBC_TEXT(SQL_SELECT_ALL_SECTYPES));
        }

        if (g_sectypeState.result->next())
        {
            g_sectypeState.cRows++;
            FillSectypeStruct(*g_sectypeState.result, pzST);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_sectypeState.cRows = 0;
            g_sectypeState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllSectypes: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSectypes", FALSE);
        g_sectypeState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllSectypes", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSectypes", FALSE);
        g_sectypeState.result.reset();
    }
}

// ============================================================================
// Initialization and Cleanup (RAII no-ops)
// ============================================================================

DLLAPI ERRSTRUCT STDCALL InitializeMaturityIO(char *sMode)
{
    ERRSTRUCT zErr;
    InitializeErrStruct(&zErr);
    
    // No-op with nanodbc RAII
    // Mode parameter maintained for compatibility but not used
    
#ifdef DEBUG
    PrintError("InitializeMaturityIO called (no-op with nanodbc RAII)", 
        0, 0, "", 0, 0, 0, "InitializeMaturityIO", FALSE);
#endif

    return zErr;
}

DLLAPI void STDCALL FreeMaturityIO(void)
{
    // Optional: Explicitly reset static states
    // (automatic cleanup occurs when states go out of scope at program exit)
    g_maturityState.result.reset();
    g_maturityState.cRows = 0;
    
    g_forwardState.result.reset();
    g_forwardState.cRows = 0;
    
    g_sectypeState.result.reset();
    g_sectypeState.cRows = 0;
    
#ifdef DEBUG
    PrintError("FreeMaturityIO called (RAII cleanup)", 
        0, 0, "", 0, 0, 0, "FreeMaturityIO", FALSE);
#endif
}

void CloseMaturityIO(void)
{
    // Internal cleanup - delegate to FreeMaturityIO
    FreeMaturityIO();
}