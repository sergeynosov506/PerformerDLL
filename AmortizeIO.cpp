/**
 *
 * SUB-SYSTEM: Database Input/Output for Payments
 *
 * FILENAME: AmortizeIO.cpp
 *
 * DESCRIPTION: Amortization and TIPS phantom income processing functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 *
 * NOTES: Multi-row cursor pattern using static state
 *        Mode-based query routing (None/Account/Security)
 *        Table name substitution for holdings table
 *        8 date field conversions for AMORTSTRUCT
 *
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#include "AmortizeIO.h"
#include "ODBCErrorChecking.h"

#include "PaymentsIO.h"

#include "commonheader.h"
#include "dateutils.h"
#include "holdings.h"
#include <cstring>
#include <optional>
#include <string>

#define NANODBC_USE_UNICODE
#include "nanodbc/nanodbc.h"

// extern thread_local nanodbc::connection gConn; // Removed: Use
// GetDBConnection()

extern char sHoldings[]; // Global holdings table name

// ============================================================================
// SQL Queries
// ============================================================================

const char *SQL_UPDATE_AMORTIZE_HOLDINGS =
    "UPDATE %HOLDINGS_TABLE% "
    "SET orig_yield = ?, cost_eff_mat_yld = ?, eff_mat_date = ?, eff_mat_price "
    "= ?, tot_cost = ? "
    "WHERE id = ? AND sec_no = ? AND wi = ? AND sec_xtend = ? AND acct_type = "
    "? AND trans_no = ?";

const char *SQL_AMORTIZE_UNLOAD_BASE =
    "SELECT h.id, h.sec_no, h.wi, h.sec_xtend, sec_symbol, acct_type, "
    "trans_no, h.secid, h.units, trd_date, stl_date, eff_date, elig_date, "
    "orig_face, orig_yield, "
    "tot_cost, orig_cost, cost_eff_mat_yld, eff_mat_date, "
    "eff_mat_price, orig_trans_type, "
    "sec_type, trad_unit, curr_id, inc_curr_id, accrual_sched, "
    "maturity_date, redemption_price, pay_type, defeased, "
    "cur_exrate, cur_inc_exrate, first_cpn_date, last_cpn_date, amort_flag, "
    "accretion_flag "
    "FROM %HOLDINGS_TABLE% h, assets a, fixedinc f "
    "WHERE h.sec_no = a.sec_no AND h.wi = a.when_issue AND "
    "h.sec_no = f.sec_no AND h.wi = f.wi AND (amort_flag = 'Y' OR "
    "accretion_flag = 'Y')";

const char *SQL_AMORTIZE_MODE_ACCOUNT = " AND h.id = ?";
const char *SQL_AMORTIZE_MODE_SECURITY =
    " AND h.id = ? AND h.sec_no = ? AND h.wi = ? AND h.sec_xtend = ? AND "
    "h.acct_type = ? AND h.trans_no = ?";

const char *SQL_PITIPS_UNLOAD_BASE =
    "SELECT h.id, h.sec_no, h.wi, h.sec_xtend, h.sec_symbol, h.acct_type, "
    "h.trans_no, h.secid, h.units, h.trd_date, h.stl_date, h.eff_date, "
    "h.tot_cost, h.orig_cost, "
    "a.sec_type, a.trad_unit, a.curr_id, a.inc_curr_id, "
    "f.maturity_date "
    "FROM dbo.%HOLDINGS_TABLE% h, dbo.assets a, dbo.fixedinc f, dbo.sectype s "
    "WHERE h.sec_no = a.sec_no AND h.wi = a.when_issue AND "
    "a.sec_no = f.sec_no AND a.when_issue = f.wi AND "
    "h.sec_no = f.sec_no AND h.wi = f.wi AND "
    "s.sec_type = a.sec_type AND "
    "s.primary_type = 'B' AND s.secondary_type = 'T' AND s.pay_type = 'I' AND "
    "EXISTS (SELECT * FROM dbo.portmain p WHERE p.id=h.id AND p.deletedate IS "
    "NULL AND p.portfoliotype IN (3,4,5)) AND "
    "h.eff_date < ? AND f.maturity_date > ? AND DATEPART(dd, f.maturity_date) "
    "= DATEPART(dd, ?)";

const char *SQL_PITIPS_MODE_ACCOUNT = " AND h.id = ?";
const char *SQL_PITIPS_MODE_SECURITY =
    " AND h.id = ? AND h.sec_no = ? AND h.wi = ? AND h.sec_xtend = ? AND "
    "h.acct_type = ? AND h.trans_no = ?";

// ============================================================================
// Static State for Multi-Row Cursors
// ============================================================================

struct AmortizeState {
  std::optional<nanodbc::result> result;
  char sMode[2] = {0};
  int iID = 0;
  char sSecNo[20] = {0};
  char sWi[2] = {0};
  char sSecXtend[3] = {0};
  char sAcctType[2] = {0};
  long lTransNo = 0;
  char sHoldingsTable[40] = {0};
  int cRows = 0;
};

struct PITIPSState {
  std::optional<nanodbc::result> result;
  long lValDate = 0;
  char sMode[2] = {0};
  int iID = 0;
  char sSecNo[20] = {0};
  char sWi[2] = {0};
  char sSecXtend[3] = {0};
  char sAcctType[2] = {0};
  long lTransNo = 0;
  char sHoldingsTable[40] = {0};
  int cRows = 0;
};

thread_local AmortizeState g_amortizeState;
thread_local PITIPSState g_pitipsState;

static void set_default_timeout(nanodbc::statement &stmt, int seconds = 90) {
  stmt.timeout(seconds);
};
// ============================================================================
// Helper Functions
// ============================================================================

static void FillAmortStruct(nanodbc::result &result, AMORTSTRUCT *pzAM) {
  memset(pzAM, 0, sizeof(AMORTSTRUCT));

  pzAM->iID = result.get<int>("id", 0);
  read_string(result, "sec_no", pzAM->sSecNo, sizeof(pzAM->sSecNo));
  read_string(result, "wi", pzAM->sWi, sizeof(pzAM->sWi));
  read_string(result, "sec_xtend", pzAM->sSecXtend, sizeof(pzAM->sSecXtend));
  read_string(result, "sec_symbol", pzAM->sSecSymbol, sizeof(pzAM->sSecSymbol));
  read_string(result, "acct_type", pzAM->sAcctType, sizeof(pzAM->sAcctType));
  pzAM->fCostEffMatYld = result.get<double>("cost_eff_mat_yld", 0.0);
  pzAM->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);

  read_string(result, "orig_trans_type", pzAM->sOrigTransType,
              sizeof(pzAM->sOrigTransType));

  pzAM->iSecType = result.get<int>("sec_type", 0);
  pzAM->fTradUnit = result.get<double>("trad_unit", 0.0);
  read_string(result, "curr_id", pzAM->sCurrId, sizeof(pzAM->sCurrId));
  read_string(result, "inc_curr_id", pzAM->sIncCurrId,
              sizeof(pzAM->sIncCurrId));
  pzAM->iAccrualSched = result.get<int>("accrual_sched", 0);

  pzAM->fRedemptionPrice = result.get<double>("redemption_price", 0.0);
  read_string(result, "pay_type", pzAM->sPayType, sizeof(pzAM->sPayType));
  read_string(result, "defeased", pzAM->sDefeased, sizeof(pzAM->sDefeased));

  pzAM->fCurExrate = result.get<double>("cur_exrate", 0.0);
  pzAM->fCurIncExrate = result.get<double>("cur_inc_exrate", 0.0);

  read_string(result, "amort_flag", pzAM->sAmortFlag, sizeof(pzAM->sAmortFlag));
  read_string(result, "accretion_flag", pzAM->sAccretFlag,
              sizeof(pzAM->sAccretFlag));
}

static void FillPITIPSStruct(nanodbc::result &result, PITIPSSTRUCT *pzPI) {
  memset(pzPI, 0, sizeof(PITIPSSTRUCT));

  pzPI->iID = result.get<int>("id", 0);
  read_string(result, "sec_no", pzPI->sSecNo, sizeof(pzPI->sSecNo));
  read_string(result, "wi", pzPI->sWi, sizeof(pzPI->sWi));
  read_string(result, "sec_xtend", pzPI->sSecXtend, sizeof(pzPI->sSecXtend));
  read_string(result, "sec_symbol", pzPI->sSecSymbol, sizeof(pzPI->sSecSymbol));
  pzPI->fTradUnit = result.get<double>("trad_unit", 0.0);
  read_string(result, "curr_id", pzPI->sCurrId, sizeof(pzPI->sCurrId));
  read_string(result, "inc_curr_id", pzPI->sIncCurrId,
              sizeof(pzPI->sIncCurrId));
}

// ============================================================================
// UpdateAmortizeHoldings
// ============================================================================
DLLAPI void STDCALL UpdateAmortizeHoldings(HOLDINGS zHD, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, sHoldings, 0, 0, 0, "UpdateAmortizeHoldings",
             FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!GetDBConnection().connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"UpdateAmortizeHoldings", FALSE);
    return;
  }

  try {
    // Build SQL with table name substitution
    std::string sql = SQL_UPDATE_AMORTIZE_HOLDINGS;
    size_t pos = sql.find("%HOLDINGS_TABLE%");
    if (pos != std::string::npos)
      sql.replace(pos, 16, sHoldings);

    nanodbc::statement stmt(GetDBConnection());
    nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
    set_default_timeout(stmt);

    // Bind parameters
    double fOrigYield = RoundDouble(zHD.fOrigYield, 6);
    double fCostEffMatYld = RoundDouble(zHD.fCostEffMatYld, 6);
    nanodbc::timestamp tsEffMatDate;
    long_to_timestamp(zHD.lEffMatDate, tsEffMatDate);
    double fEffMatPrice = RoundDouble(zHD.fEffMatPrice, 6);
    double fTotCost = RoundDouble(zHD.fTotCost, 3);

    int paramIndex = 0;
    stmt.bind(paramIndex++, &fOrigYield);
    stmt.bind(paramIndex++, &fCostEffMatYld);
    stmt.bind(paramIndex++, &tsEffMatDate);
    stmt.bind(paramIndex++, &fEffMatPrice);
    stmt.bind(paramIndex++, &fTotCost);
    stmt.bind(paramIndex++, &zHD.iID);
    safe_bind_string(stmt, paramIndex, zHD.sSecNo);
    safe_bind_string(stmt, paramIndex, zHD.sWi);
    safe_bind_string(stmt, paramIndex, zHD.sSecXtend);
    safe_bind_string(stmt, paramIndex, zHD.sAcctType);
    stmt.bind(paramIndex++, &zHD.lTransNo);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in UpdateAmortizeHoldings: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"UpdateAmortizeHoldings", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in UpdateAmortizeHoldings", 0,
                        0, (char *)"", 0, -1, 0,
                        (char *)"UpdateAmortizeHoldings", FALSE);
  }
  static_assert(sizeof(g_amortizeState.sHoldingsTable) >= 40,
                "Table name buffer too small");
}

// ============================================================================
// AmortizeUnload
// ============================================================================

DLLAPI void STDCALL AmortizeUnload(char *sMode, int iID, char *sSecNo,
                                   char *sWi, char *sSecXtend, char *sAcctType,
                                   long lTransNo, AMORTSTRUCT *pzAM,
                                   ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "AmortizeUnload", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!GetDBConnection().connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"AmortizeUnload", FALSE);
    return;
  }

  try {
    // Build SQL based on mode
    std::string sql = SQL_AMORTIZE_UNLOAD_BASE;

    // Replace %HOLDINGS_TABLE% with actual table name
    size_t pos = sql.find("%HOLDINGS_TABLE%");
    if (pos != std::string::npos)
      sql.replace(pos, 16, sHoldings);

    if (sMode[0] == 'A') {
      sql += SQL_AMORTIZE_MODE_ACCOUNT;
    } else if (sMode[0] == 'S') {
      sql += SQL_AMORTIZE_MODE_SECURITY;
    }

    // Check if parameters changed
    bool needNewQuery =
        !(g_amortizeState.sMode[0] == sMode[0] && g_amortizeState.iID == iID &&
          strcmp(g_amortizeState.sHoldingsTable, sHoldings) == 0 &&
          (sMode[0] != 'S' ||
           (strcmp(g_amortizeState.sSecNo, sSecNo) == 0 &&
            strcmp(g_amortizeState.sWi, sWi) == 0 &&
            strcmp(g_amortizeState.sSecXtend, sSecXtend) == 0 &&
            strcmp(g_amortizeState.sAcctType, sAcctType) == 0 &&
            g_amortizeState.lTransNo == lTransNo)) &&
          g_amortizeState.cRows > 0);

    if (needNewQuery) {
      g_amortizeState.result.reset();
      g_amortizeState.cRows = 0;

      nanodbc::statement stmt(GetDBConnection());
      nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

      int paramIndex = 0;
      if (sMode[0] == 'A') {
        stmt.bind(paramIndex++, &iID);
      } else if (sMode[0] == 'S') {
        stmt.bind(paramIndex++, &iID);
        safe_bind_string(stmt, paramIndex, sSecNo);
        safe_bind_string(stmt, paramIndex, sWi);
        safe_bind_string(stmt, paramIndex, sSecXtend);
        safe_bind_string(stmt, paramIndex, sAcctType);
        stmt.bind(paramIndex++, &lTransNo);
      }

      g_amortizeState.result = nanodbc::execute(stmt);
      g_amortizeState.sMode[0] = sMode[0];
      g_amortizeState.iID = iID;
      strcpy_s(g_amortizeState.sHoldingsTable, sHoldings);
      if (sMode[0] == 'S') {
        strcpy_s(g_amortizeState.sSecNo, sSecNo);
        strcpy_s(g_amortizeState.sWi, sWi);
        strcpy_s(g_amortizeState.sSecXtend, sSecXtend);
        strcpy_s(g_amortizeState.sAcctType, sAcctType);
        g_amortizeState.lTransNo = lTransNo;
      }
    }

    if (g_amortizeState.result && g_amortizeState.result->next()) {
      g_amortizeState.cRows++;
      FillAmortStruct(*g_amortizeState.result, pzAM);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
      g_amortizeState.cRows = 0;
      g_amortizeState.result.reset();
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in AmortizeUnload: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"AmortizeUnload", FALSE);
    g_amortizeState.result.reset();
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in AmortizeUnload", 0, 0,
                        (char *)"", 0, -1, 0, (char *)"AmortizeUnload", FALSE);
    g_amortizeState.result.reset();
  }
}

// ============================================================================
// PITIPSUnload
// ============================================================================

DLLAPI void STDCALL PITIPSUnload(char *sMode, long lValDate, int iID,
                                 char *sSecNo, char *sWi, char *sSecXtend,
                                 char *sAcctType, long lTransNo,
                                 PITIPSSTRUCT *pzPI, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "PITIPSUnload", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!GetDBConnection().connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"PITIPSUnload", FALSE);
    return;
  }

  try {
    // Build SQL based on mode
    std::string sql = SQL_PITIPS_UNLOAD_BASE;

    // Replace %HOLDINGS_TABLE% with actual table name
    size_t pos = sql.find("%HOLDINGS_TABLE%");
    if (pos != std::string::npos)
      sql.replace(pos, 16, sHoldings);

    if (sMode[0] == 'A') {
      sql += SQL_PITIPS_MODE_ACCOUNT;
    } else if (sMode[0] == 'S') {
      sql += SQL_PITIPS_MODE_SECURITY;
    }

    // Check if parameters changed
    bool needNewQuery = !(
        g_pitipsState.lValDate == lValDate &&
        g_pitipsState.sMode[0] == sMode[0] && g_pitipsState.iID == iID &&
        strcmp(g_pitipsState.sHoldingsTable, sHoldings) == 0 &&
        (sMode[0] != 'S' || (strcmp(g_pitipsState.sSecNo, sSecNo) == 0 &&
                             strcmp(g_pitipsState.sWi, sWi) == 0 &&
                             strcmp(g_pitipsState.sSecXtend, sSecXtend) == 0 &&
                             strcmp(g_pitipsState.sAcctType, sAcctType) == 0 &&
                             g_pitipsState.lTransNo == lTransNo)) &&
        g_pitipsState.cRows > 0);

    if (needNewQuery) {
      g_pitipsState.result.reset();
      g_pitipsState.cRows = 0;

      nanodbc::statement stmt(GetDBConnection());
      nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

      nanodbc::timestamp tsValDate;
      long_to_timestamp(lValDate, tsValDate);

      int paramIndex = 0;
      // Bind 3 valuation date parameters (used in WHERE clause)
      stmt.bind(paramIndex++, &tsValDate);
      stmt.bind(paramIndex++, &tsValDate);
      stmt.bind(paramIndex++, &tsValDate);

      if (sMode[0] == 'A') {
        stmt.bind(paramIndex++, &iID);
      } else if (sMode[0] == 'S') {
        stmt.bind(paramIndex++, &iID);
        safe_bind_string(stmt, paramIndex, sSecNo);
        safe_bind_string(stmt, paramIndex, sWi);
        safe_bind_string(stmt, paramIndex, sSecXtend);
        safe_bind_string(stmt, paramIndex, sAcctType);
        stmt.bind(paramIndex++, &lTransNo);
      }

      g_pitipsState.result = nanodbc::execute(stmt);
      g_pitipsState.lValDate = lValDate;
      g_pitipsState.sMode[0] = sMode[0];
      g_pitipsState.iID = iID;
      strcpy_s(g_pitipsState.sHoldingsTable, sHoldings);
      if (sMode[0] == 'S') {
        strcpy_s(g_pitipsState.sSecNo, sSecNo);
        strcpy_s(g_pitipsState.sWi, sWi);
        strcpy_s(g_pitipsState.sSecXtend, sSecXtend);
        strcpy_s(g_pitipsState.sAcctType, sAcctType);
        g_pitipsState.lTransNo = lTransNo;
      }
    }

    if (g_pitipsState.result && g_pitipsState.result->next()) {
      g_pitipsState.cRows++;
      FillPITIPSStruct(*g_pitipsState.result, pzPI);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
      g_pitipsState.cRows = 0;
      g_pitipsState.result.reset();
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in PITIPSUnload: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"PITIPSUnload", FALSE);
    g_pitipsState.result.reset();
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in PITIPSUnload", 0, 0,
                        (char *)"", 0, -1, 0, (char *)"PITIPSUnload", FALSE);
    g_pitipsState.result.reset();
  }
}

// ============================================================================
// Cleanup (RAII no-ops)
// ============================================================================

DLLAPI void STDCALL FreeAmortizeIO(void) {
  // Optional: Explicitly reset static states
  g_amortizeState.result.reset();
  g_amortizeState.cRows = 0;

  g_pitipsState.result.reset();
  g_pitipsState.cRows = 0;

#ifdef DEBUG
  PrintError("FreeAmortizeIO called (RAII cleanup)", 0, 0, "", 0, 0, 0,
             "FreeAmortizeIO", FALSE);
#endif
}

void CloseAmortizeIO(void) {
  // Internal cleanup - delegate to FreeAmortizeIO
  FreeAmortizeIO();
}