#include "TransIO_Holdcash.h"
#include "ODBCErrorChecking.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <cstring>

extern thread_local nanodbc::connection gConn;

// Helper struct for stateful cursors
struct HoldcashCursor {
  int iID = -1;
  char sSecNo[13] = "";
  char sWi[3] = "";
  char sSecXtend[4] = "";
  char sAcctType[3] = "";
  long lTrdDate = 0;
  long lMinTrdDate = 0;
  long lMaxTrdDate = 0;

  nanodbc::result result;
  bool has_result = false;

  void Reset() {
    has_result = false;
    result = nanodbc::result();
    iID = -1;
    sSecNo[0] = '\0';
    sWi[0] = '\0';
    sSecXtend[0] = '\0';
    sAcctType[0] = '\0';
    lTrdDate = 0;
    lMinTrdDate = 0;
    lMaxTrdDate = 0;
  }
};

thread_local HoldcashCursor gFifoCursor;
thread_local HoldcashCursor gLifoCursor;
thread_local HoldcashCursor gHighCursor;
thread_local HoldcashCursor gLowCursor;
thread_local HoldcashCursor gMinGainCursor;
thread_local HoldcashCursor gMaxGainCursor;

// Helper to check if cursor matches params
bool CursorMatches(const HoldcashCursor &cursor, int iID, const char *sSecNo,
                   const char *sWi, const char *sSecXtend,
                   const char *sAcctType, long lTrdDate) {
  return cursor.has_result && cursor.iID == iID &&
         strcmp(cursor.sSecNo, sSecNo) == 0 && strcmp(cursor.sWi, sWi) == 0 &&
         strcmp(cursor.sSecXtend, sSecXtend) == 0 &&
         strcmp(cursor.sAcctType, sAcctType) == 0 &&
         cursor.lTrdDate == lTrdDate;
}

bool CursorMatchesRange(const HoldcashCursor &cursor, int iID,
                        const char *sSecNo, const char *sWi,
                        const char *sSecXtend, const char *sAcctType,
                        long lTrdDate, long lMin, long lMax) {
  return CursorMatches(cursor, iID, sSecNo, sWi, sSecXtend, sAcctType,
                       lTrdDate) &&
         cursor.lMinTrdDate == lMin && cursor.lMaxTrdDate == lMax;
}

void FillHoldingsFromResult(nanodbc::result &result, HOLDINGS *pzHoldings) {
  memset(pzHoldings, 0, sizeof(HOLDINGS));

  read_int(result, "id", &pzHoldings->iID);
  read_string(result, "sec_no", pzHoldings->sSecNo, sizeof(pzHoldings->sSecNo));
  read_string(result, "wi", pzHoldings->sWi, sizeof(pzHoldings->sWi));
  read_string(result, "sec_xtend", pzHoldings->sSecXtend,
              sizeof(pzHoldings->sSecXtend));
  read_string(result, "acct_type", pzHoldings->sAcctType,
              sizeof(pzHoldings->sAcctType));

  read_double(result, "units", &pzHoldings->fUnits);
  read_double(result, "tot_cost", &pzHoldings->fTotCost);
  read_double(result, "unit_cost", &pzHoldings->fUnitCost);
  read_double(result, "base_cost_xrate", &pzHoldings->fBaseCostXrate);
  read_double(result, "sys_cost_xrate", &pzHoldings->fSysCostXrate);

  read_date(result, "trd_date", &pzHoldings->lTrdDate);
  read_date(result, "eff_date", &pzHoldings->lEffDate);
  read_date(result, "stl_date", &pzHoldings->lStlDate);

  read_double(result, "collateral_units", &pzHoldings->fCollateralUnits);
}

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL DeleteHoldcash(int iID, char *sSecNo, char *sWi,
                                   char *sSecXtend, char *sAcctType,
                                   ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"DeleteHoldcash", FALSE);
    return;
  }
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT("DELETE FROM holdcash WHERE ID=? and sec_no=? and "
                           "wi=? and sec_xtend=? and acct_type=?"));
    int idx = 0;
    stmt.bind(idx++, &iID);
    safe_bind_string(stmt, idx, sSecNo);
    safe_bind_string(stmt, idx, sWi);
    safe_bind_string(stmt, idx, sSecXtend);
    safe_bind_string(stmt, idx, sAcctType);
    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"DeleteHoldcash", FALSE);
  }
}

DLLAPI void STDCALL InsertHoldcash(HOLDCASH zHCR, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"InsertHoldcash", FALSE);
    return;
  }
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT(
                  "INSERT INTO holdcash "
                  "(ID, sec_no, wi, sec_xtend, acct_type, secid, "
                  "asof_date, sec_symbol, units, tot_cost, unit_cost, "
                  "base_cost_xrate, sys_cost_xrate, trd_date, eff_date, "
                  "stl_date, last_trans_no, mkt_val, mv_base_xrate, "
                  "mv_sys_xrate, currency_gl, collateral_units, hedge_value) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?)"));

    int idx = 0;
    stmt.bind(idx++, &zHCR.iID);
    safe_bind_string(stmt, idx, zHCR.sSecNo);
    safe_bind_string(stmt, idx, zHCR.sWi);
    safe_bind_string(stmt, idx, zHCR.sSecXtend);
    safe_bind_string(stmt, idx, zHCR.sAcctType);
    stmt.bind(idx++, &zHCR.iSecID);
    stmt.bind(idx++, &zHCR.lAsofDate);
    safe_bind_string(stmt, idx, zHCR.sSecSymbol);
    stmt.bind(idx++, &zHCR.fUnits);
    stmt.bind(idx++, &zHCR.fTotCost);
    stmt.bind(idx++, &zHCR.fUnitCost);
    stmt.bind(idx++, &zHCR.fBaseCostXrate);
    stmt.bind(idx++, &zHCR.fSysCostXrate);
    stmt.bind(idx++, &zHCR.lTrdDate);
    stmt.bind(idx++, &zHCR.lEffDate);
    stmt.bind(idx++, &zHCR.lStlDate);
    stmt.bind(idx++, &zHCR.lLastTransNo);
    stmt.bind(idx++, &zHCR.fMktVal);
    stmt.bind(idx++, &zHCR.fMvBaseXrate);
    stmt.bind(idx++, &zHCR.fMvSysXrate);
    stmt.bind(idx++, &zHCR.fCurrencyGl);
    stmt.bind(idx++, &zHCR.fCollateralUnits);
    stmt.bind(idx++, &zHCR.fHedgeValue);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"InsertHoldcash", FALSE);
  }
}

DLLAPI void STDCALL SelectHoldcash(HOLDCASH *pzHCR, int iID, char *sSecNo,
                                   char *sWi, char *sSecXtend, char *sAcctType,
                                   ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"SelectHoldcash", FALSE);
    return;
  }
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT("SELECT ID, sec_no, wi, sec_xtend, acct_type, secid, "
                     "asof_date, sec_symbol, units, tot_cost, unit_cost, "
                     "base_cost_xrate, sys_cost_xrate, trd_date, eff_date, "
                     "stl_date, last_trans_no, mkt_val, mv_base_xrate, "
                     "mv_sys_xrate, currency_gl, collateral_units, hedge_value "
                     "FROM holdcash "
                     "WHERE ID=? and sec_no=? and wi=? and sec_xtend=? and "
                     "acct_type=?"));

    int idx = 0;
    stmt.bind(idx++, &iID);
    safe_bind_string(stmt, idx, sSecNo);
    safe_bind_string(stmt, idx, sWi);
    safe_bind_string(stmt, idx, sSecXtend);
    safe_bind_string(stmt, idx, sAcctType);

    nanodbc::result result = nanodbc::execute(stmt);
    if (result.next()) {
      read_int(result, "ID", &pzHCR->iID);
      read_string(result, "sec_no", pzHCR->sSecNo, sizeof(pzHCR->sSecNo));
      read_string(result, "wi", pzHCR->sWi, sizeof(pzHCR->sWi));
      read_string(result, "sec_xtend", pzHCR->sSecXtend,
                  sizeof(pzHCR->sSecXtend));
      read_string(result, "acct_type", pzHCR->sAcctType,
                  sizeof(pzHCR->sAcctType));
      read_int(result, "secid", &pzHCR->iSecID);
      read_date(result, "asof_date", &pzHCR->lAsofDate);
      read_string(result, "sec_symbol", pzHCR->sSecSymbol,
                  sizeof(pzHCR->sSecSymbol));
      read_double(result, "units", &pzHCR->fUnits);
      read_double(result, "tot_cost", &pzHCR->fTotCost);
      read_double(result, "unit_cost", &pzHCR->fUnitCost);
      read_double(result, "base_cost_xrate", &pzHCR->fBaseCostXrate);
      read_double(result, "sys_cost_xrate", &pzHCR->fSysCostXrate);
      read_date(result, "trd_date", &pzHCR->lTrdDate);
      read_date(result, "eff_date", &pzHCR->lEffDate);
      read_date(result, "stl_date", &pzHCR->lStlDate);
      read_long(result, "last_trans_no", &pzHCR->lLastTransNo);
      read_double(result, "mkt_val", &pzHCR->fMktVal);
      read_double(result, "mv_base_xrate", &pzHCR->fMvBaseXrate);
      read_double(result, "mv_sys_xrate", &pzHCR->fMvSysXrate);
      read_double(result, "currency_gl", &pzHCR->fCurrencyGl);
      read_double(result, "collateral_units", &pzHCR->fCollateralUnits);
      read_double(result, "hedge_value", &pzHCR->fHedgeValue);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"SelectHoldcash", FALSE);
  }
}

DLLAPI void STDCALL UpdateHoldcash(HOLDCASH zHCR, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"UpdateHoldcash", FALSE);
    return;
  }
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "UPDATE holdcash "
            "SET asof_date=?, sec_symbol=?, units=?, tot_cost=?, unit_cost=?, "
            "base_cost_xrate=?, sys_cost_xrate=?, trd_date=?, eff_date=?, "
            "stl_date=?, last_trans_no=?, mkt_val=?, mv_base_xrate=?, "
            "mv_sys_xrate=?, currency_gl=?, collateral_units=?, hedge_value=? "
            "WHERE ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=?"));

    int idx = 0;
    stmt.bind(idx++, &zHCR.lAsofDate);
    safe_bind_string(stmt, idx, zHCR.sSecSymbol);
    stmt.bind(idx++, &zHCR.fUnits);
    stmt.bind(idx++, &zHCR.fTotCost);
    stmt.bind(idx++, &zHCR.fUnitCost);
    stmt.bind(idx++, &zHCR.fBaseCostXrate);
    stmt.bind(idx++, &zHCR.fSysCostXrate);
    stmt.bind(idx++, &zHCR.lTrdDate);
    stmt.bind(idx++, &zHCR.lEffDate);
    stmt.bind(idx++, &zHCR.lStlDate);
    stmt.bind(idx++, &zHCR.lLastTransNo);
    stmt.bind(idx++, &zHCR.fMktVal);
    stmt.bind(idx++, &zHCR.fMvBaseXrate);
    stmt.bind(idx++, &zHCR.fMvSysXrate);
    stmt.bind(idx++, &zHCR.fCurrencyGl);
    stmt.bind(idx++, &zHCR.fCollateralUnits);
    stmt.bind(idx++, &zHCR.fHedgeValue);

    stmt.bind(idx++, &zHCR.iID);
    safe_bind_string(stmt, idx, zHCR.sSecNo);
    safe_bind_string(stmt, idx, zHCR.sWi);
    safe_bind_string(stmt, idx, zHCR.sSecXtend);
    safe_bind_string(stmt, idx, zHCR.sAcctType);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"UpdateHoldcash", FALSE);
  }
}

// =====================================================
// HoldcashFor... functions
// =====================================================

DLLAPI void STDCALL HoldcashForFifoAndAvgAccts(HOLDINGS *pzHoldings, int iID,
                                               char *sSecNo, char *sWi,
                                               char *sSecXtend, char *sAcctType,
                                               long lTrdDate,
                                               ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"HoldcashForFifoAndAvgAccts", FALSE);
    return;
  }

  try {
    if (!CursorMatches(gFifoCursor, iID, sSecNo, sWi, sSecXtend, sAcctType,
                       lTrdDate)) {
      gFifoCursor.Reset();
      gFifoCursor.iID = iID;
      strcpy(gFifoCursor.sSecNo, sSecNo);
      strcpy(gFifoCursor.sWi, sWi);
      strcpy(gFifoCursor.sSecXtend, sSecXtend);
      strcpy(gFifoCursor.sAcctType, sAcctType);
      gFifoCursor.lTrdDate = lTrdDate;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(
          stmt,
          NANODBC_TEXT(
              "SELECT id, sec_no, wi, sec_xtend, acct_type, units, tot_cost, "
              "unit_cost, base_cost_xrate, "
              "sys_cost_xrate, trd_date, eff_date, stl_date, collateral_units "
              "FROM Holdcash "
              "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
              "sec_xtend = ? AND acct_type LIKE ? "
              "AND trd_date is NOT NULL AND trd_date != ? "
              "ORDER BY trd_date ASC"));

      int idx = 0;
      stmt.bind(idx++, &gFifoCursor.iID);
      safe_bind_string(stmt, idx, gFifoCursor.sSecNo);
      safe_bind_string(stmt, idx, gFifoCursor.sWi);
      safe_bind_string(stmt, idx, gFifoCursor.sSecXtend);
      safe_bind_string(stmt, idx, gFifoCursor.sAcctType);
      stmt.bind(idx++, &gFifoCursor.lTrdDate);

      gFifoCursor.result = nanodbc::execute(stmt);
      gFifoCursor.has_result = true;
    }

    if (gFifoCursor.result.next()) {
      FillHoldingsFromResult(gFifoCursor.result, pzHoldings);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"HoldcashForFifoAndAvgAccts", FALSE);
  }
}

DLLAPI void STDCALL HoldcashForLifoAccts(HOLDINGS *pzHoldings, int iID,
                                         char *sSecNo, char *sWi,
                                         char *sSecXtend, char *sAcctType,
                                         long lTrdDate, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"HoldcashForLifoAccts", FALSE);
    return;
  }

  try {
    if (!CursorMatches(gLifoCursor, iID, sSecNo, sWi, sSecXtend, sAcctType,
                       lTrdDate)) {
      gLifoCursor.Reset();
      gLifoCursor.iID = iID;
      strcpy(gLifoCursor.sSecNo, sSecNo);
      strcpy(gLifoCursor.sWi, sWi);
      strcpy(gLifoCursor.sSecXtend, sSecXtend);
      strcpy(gLifoCursor.sAcctType, sAcctType);
      gLifoCursor.lTrdDate = lTrdDate;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(
          stmt,
          NANODBC_TEXT(
              "SELECT id, sec_no, wi, sec_xtend, acct_type, units, tot_cost, "
              "unit_cost, base_cost_xrate, "
              "sys_cost_xrate, trd_date, eff_date, stl_date, collateral_units "
              "FROM Holdcash "
              "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
              "sec_xtend = ? AND acct_type LIKE ? "
              "AND trd_date is NOT NULL AND trd_date != ? "
              "ORDER BY trd_date DESC"));

      int idx = 0;
      stmt.bind(idx++, &gLifoCursor.iID);
      safe_bind_string(stmt, idx, gLifoCursor.sSecNo);
      safe_bind_string(stmt, idx, gLifoCursor.sWi);
      safe_bind_string(stmt, idx, gLifoCursor.sSecXtend);
      safe_bind_string(stmt, idx, gLifoCursor.sAcctType);
      stmt.bind(idx++, &gLifoCursor.lTrdDate);

      gLifoCursor.result = nanodbc::execute(stmt);
      gLifoCursor.has_result = true;
    }

    if (gLifoCursor.result.next()) {
      FillHoldingsFromResult(gLifoCursor.result, pzHoldings);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"HoldcashForLifoAccts", FALSE);
  }
}

DLLAPI void STDCALL HoldcashForHighAccts(HOLDINGS *pzHoldings, int iID,
                                         char *sSecNo, char *sWi,
                                         char *sSecXtend, char *sAcctType,
                                         long lTrdDate, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"HoldcashForHighAccts", FALSE);
    return;
  }

  try {
    if (!CursorMatches(gHighCursor, iID, sSecNo, sWi, sSecXtend, sAcctType,
                       lTrdDate)) {
      gHighCursor.Reset();
      gHighCursor.iID = iID;
      strcpy(gHighCursor.sSecNo, sSecNo);
      strcpy(gHighCursor.sWi, sWi);
      strcpy(gHighCursor.sSecXtend, sSecXtend);
      strcpy(gHighCursor.sAcctType, sAcctType);
      gHighCursor.lTrdDate = lTrdDate;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(
          stmt,
          NANODBC_TEXT(
              "SELECT id, sec_no, wi, sec_xtend, acct_type, units, tot_cost, "
              "unit_cost, base_cost_xrate, "
              "sys_cost_xrate, trd_date, eff_date, stl_date, collateral_units "
              "FROM Holdcash "
              "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
              "sec_xtend = ? AND acct_type LIKE ? "
              "AND trd_date is NOT NULL AND trd_date != ? and units <> '0' "
              "ORDER BY tot_cost / units DESC, trd_date"));

      int idx = 0;
      stmt.bind(idx++, &gHighCursor.iID);
      safe_bind_string(stmt, idx, gHighCursor.sSecNo);
      safe_bind_string(stmt, idx, gHighCursor.sWi);
      safe_bind_string(stmt, idx, gHighCursor.sSecXtend);
      safe_bind_string(stmt, idx, gHighCursor.sAcctType);
      stmt.bind(idx++, &gHighCursor.lTrdDate);

      gHighCursor.result = nanodbc::execute(stmt);
      gHighCursor.has_result = true;
    }

    if (gHighCursor.result.next()) {
      FillHoldingsFromResult(gHighCursor.result, pzHoldings);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"HoldcashForHighAccts", FALSE);
  }
}

DLLAPI void STDCALL HoldcashForLowAccts(HOLDINGS *pzHoldings, int iID,
                                        char *sSecNo, char *sWi,
                                        char *sSecXtend, char *sAcctType,
                                        long lTrdDate, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"HoldcashForLowAccts", FALSE);
    return;
  }

  try {
    if (!CursorMatches(gLowCursor, iID, sSecNo, sWi, sSecXtend, sAcctType,
                       lTrdDate)) {
      gLowCursor.Reset();
      gLowCursor.iID = iID;
      strcpy(gLowCursor.sSecNo, sSecNo);
      strcpy(gLowCursor.sWi, sWi);
      strcpy(gLowCursor.sSecXtend, sSecXtend);
      strcpy(gLowCursor.sAcctType, sAcctType);
      gLowCursor.lTrdDate = lTrdDate;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(
          stmt,
          NANODBC_TEXT(
              "SELECT id, sec_no, wi, sec_xtend, acct_type, units, tot_cost, "
              "unit_cost, base_cost_xrate, "
              "sys_cost_xrate, trd_date, eff_date, stl_date, collateral_units "
              "FROM Holdcash "
              "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
              "sec_xtend = ? AND acct_type LIKE ? "
              "AND trd_date is NOT NULL AND trd_date != ? and units <> '0' "
              "ORDER BY tot_cost / units ASC, trd_date"));

      int idx = 0;
      stmt.bind(idx++, &gLowCursor.iID);
      safe_bind_string(stmt, idx, gLowCursor.sSecNo);
      safe_bind_string(stmt, idx, gLowCursor.sWi);
      safe_bind_string(stmt, idx, gLowCursor.sSecXtend);
      safe_bind_string(stmt, idx, gLowCursor.sAcctType);
      stmt.bind(idx++, &gLowCursor.lTrdDate);

      gLowCursor.result = nanodbc::execute(stmt);
      gLowCursor.has_result = true;
    }

    if (gLowCursor.result.next()) {
      FillHoldingsFromResult(gLowCursor.result, pzHoldings);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"HoldcashForLowAccts", FALSE);
  }
}

DLLAPI void STDCALL HoldcashForMinimumGainAccts(
    HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
    char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate,
    ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"HoldcashForMinimumGainAccts", FALSE);
    return;
  }

  try {
    if (!CursorMatchesRange(gMinGainCursor, iID, sSecNo, sWi, sSecXtend,
                            sAcctType, lTrdDate, lMinTrdDate, lMaxTrdDate)) {
      gMinGainCursor.Reset();
      gMinGainCursor.iID = iID;
      strcpy(gMinGainCursor.sSecNo, sSecNo);
      strcpy(gMinGainCursor.sWi, sWi);
      strcpy(gMinGainCursor.sSecXtend, sSecXtend);
      strcpy(gMinGainCursor.sAcctType, sAcctType);
      gMinGainCursor.lTrdDate = lTrdDate;
      gMinGainCursor.lMinTrdDate = lMinTrdDate;
      gMinGainCursor.lMaxTrdDate = lMaxTrdDate;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(
          stmt,
          NANODBC_TEXT(
              "SELECT id, sec_no, wi, sec_xtend, acct_type, units, tot_cost, "
              "unit_cost, base_cost_xrate, "
              "sys_cost_xrate, trd_date, eff_date, stl_date, collateral_units "
              "FROM Holdcash "
              "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
              "sec_xtend = ? AND acct_type LIKE ? "
              "AND trd_date is NOT NULL AND trd_date != ? and units <> '0' "
              "AND trd_date >= ? and trd_date <= ? "
              "ORDER BY tot_cost / units DESC, trd_date"));

      int idx = 0;
      stmt.bind(idx++, &gMinGainCursor.iID);
      safe_bind_string(stmt, idx, gMinGainCursor.sSecNo);
      safe_bind_string(stmt, idx, gMinGainCursor.sWi);
      safe_bind_string(stmt, idx, gMinGainCursor.sSecXtend);
      safe_bind_string(stmt, idx, gMinGainCursor.sAcctType);
      stmt.bind(idx++, &gMinGainCursor.lTrdDate);
      stmt.bind(idx++, &gMinGainCursor.lMinTrdDate);
      stmt.bind(idx++, &gMinGainCursor.lMaxTrdDate);

      gMinGainCursor.result = nanodbc::execute(stmt);
      gMinGainCursor.has_result = true;
    }

    if (gMinGainCursor.result.next()) {
      FillHoldingsFromResult(gMinGainCursor.result, pzHoldings);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"HoldcashForMinimumGainAccts", FALSE);
  }
}

DLLAPI void STDCALL HoldcashForMaximumGainAccts(
    HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
    char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate,
    ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);
  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"HoldcashForMaximumGainAccts", FALSE);
    return;
  }

  try {
    if (!CursorMatchesRange(gMaxGainCursor, iID, sSecNo, sWi, sSecXtend,
                            sAcctType, lTrdDate, lMinTrdDate, lMaxTrdDate)) {
      gMaxGainCursor.Reset();
      gMaxGainCursor.iID = iID;
      strcpy(gMaxGainCursor.sSecNo, sSecNo);
      strcpy(gMaxGainCursor.sWi, sWi);
      strcpy(gMaxGainCursor.sSecXtend, sSecXtend);
      strcpy(gMaxGainCursor.sAcctType, sAcctType);
      gMaxGainCursor.lTrdDate = lTrdDate;
      gMaxGainCursor.lMinTrdDate = lMinTrdDate;
      gMaxGainCursor.lMaxTrdDate = lMaxTrdDate;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(
          stmt,
          NANODBC_TEXT(
              "SELECT id, sec_no, wi, sec_xtend, acct_type, units, tot_cost, "
              "unit_cost, base_cost_xrate, "
              "sys_cost_xrate, trd_date, eff_date, stl_date, collateral_units "
              "FROM Holdcash "
              "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
              "sec_xtend = ? AND acct_type LIKE ? "
              "AND trd_date is NOT NULL AND trd_date != ? and units <> '0' "
              "AND trd_date >= ? and trd_date <= ? "
              "ORDER BY tot_cost / units, trd_date"));

      int idx = 0;
      stmt.bind(idx++, &gMaxGainCursor.iID);
      safe_bind_string(stmt, idx, gMaxGainCursor.sSecNo);
      safe_bind_string(stmt, idx, gMaxGainCursor.sWi);
      safe_bind_string(stmt, idx, gMaxGainCursor.sSecXtend);
      safe_bind_string(stmt, idx, gMaxGainCursor.sAcctType);
      stmt.bind(idx++, &gMaxGainCursor.lTrdDate);
      stmt.bind(idx++, &gMaxGainCursor.lMinTrdDate);
      stmt.bind(idx++, &gMaxGainCursor.lMaxTrdDate);

      gMaxGainCursor.result = nanodbc::execute(stmt);
      gMaxGainCursor.has_result = true;
    }

    if (gMaxGainCursor.result.next()) {
      FillHoldingsFromResult(gMaxGainCursor.result, pzHoldings);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    *pzErr = PrintError((char *)e.what(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"HoldcashForMaximumGainAccts", FALSE);
  }
}
