/**
 *
 * SUB-SYSTEM: Database Input/Output for Dividend Payment
 *
 * FILENAME: DipayIO.cpp
 *
 * DESCRIPTION: Dividend and interest payment processing functions
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 *
 * NOTES: Multi-row cursor pattern using static state
 *        Mode-based query routing (None/Account/Security)
 *        Type-based filtering (Stock/Income)
 *        Multiple date field conversions for ACCDIV
 *
 * AUTHOR: Modernized 2025-12-02
 *
 **/

#include "dipayio.h"
#include "ODBCErrorChecking.h"

#include "accdiv.h"

#include "commonheader.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include "porttax.h"
#include "withhold_rclm.h"
#include <cstring>

#include <string>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SQL Queries
// ============================================================================

const char *SQL_SELECTACCDIV_BEGIN =
    "SELECT ac.id, trans_no, divint_no, tran_type, ac.sec_no, ac.wi, ac.secid, "
    "ac.sec_xtend, "
    "acct_type, elig_date, sec_symbol, div_type, div_factor, ac.units, "
    "orig_face, "
    "pcpl_amt, income_amt, trd_date, stl_date, eff_date, entry_date, "
    "ac.curr_id, "
    "curr_acct_type, ac.inc_curr_id, inc_acct_type, sec_curr_id, accr_curr_id, "
    "base_xrate, inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
    "inc_sys_xrate, orig_yld, eff_mat_date, eff_mat_price, acct_mthd, "
    "trans_srce, dr_cr, "
    "dtc_inclusion, dtc_resolve, income_flag, letter_flag, ledger_flag, "
    "ac.created_by, "
    "ac.create_date, create_time, suspend_flag, delete_flag, "
    "sec_type, trad_unit, cur_exrate, cur_inc_exrate, "
    "hp1.exrate secexrate, hp1.inc_exrate secincexrate, "
    "hp2.exrate currexrate, hp2.inc_exrate currincexrate "
    "FROM assets ax "
    "JOIN accdiv ac on ac.sec_no = ax.sec_no AND ac.wi = ax.when_issue "
    "LEFT OUTER JOIN histpric hp1 on ac.sec_no = hp1.sec_no and "
    "ac.wi = hp1.wi and ac.eff_date = hp1.price_date "
    "LEFT OUTER JOIN histpric hp2 on ac.curr_id = hp2.sec_no "
    "and ac.wi = hp2.wi and ac.eff_date = hp2.price_date "
    "WHERE ac.eff_date <= ? AND ac.suspend_flag <> 'Y' AND ac.delete_flag <> "
    "'Y'";

const char *SQL_SELECTACCDIV_MODE_A = " AND ac.id = ?";
const char *SQL_SELECTACCDIV_MODE_S =
    " AND ac.id = ? AND ac.sec_no = ? AND ac.wi = ? AND ac.sec_xtend = ? AND "
    "acct_type = ? AND trans_no = ?";
const char *SQL_SELECTACCDIV_TYPE_S =
    " AND (tran_type in ('SD', 'SP', 'SB', 'SX'))";
const char *SQL_SELECTACCDIV_TYPE_I =
    " AND (tran_type not in ('SD', 'SP', 'SB', 'SX'))";
const char *SQL_SELECTACCDIV_END =
    " ORDER BY ac.id, ac.sec_no, ac.wi, ac.sec_xtend, acct_type, divint_no, "
    "tran_type, trans_no, dr_cr, trd_date";

const char *SQL_UPDATE_ACCDIV_DELETE_FLAG =
    "UPDATE accdiv SET delete_flag = ? "
    "WHERE id = ? AND sec_no = ? AND wi = ? AND "
    "sec_xtend = ? AND acct_type = ? AND "
    "trans_no = ? AND divint_no = ?";

const char *SQL_SELECT_PORTTAX =
    "SELECT Taxdate, FedIncomeRate, FedStGLRate, "
    "FedLtGLRate, StIncomeRate, StStGLRate, "
    "StLtGLRate, StockWithholdRate, BondWithholdRate "
    "FROM porttax a "
    "WHERE id = ? and TaxDate = (select max(taxdate) from porttax where id = "
    "a.id and taxdate <= ?)";

const char *SQL_SELECT_WITHRCLM =
    "SELECT w.curr_id, w.fixedinc_withhold, "
    "w.fixedinc_rclm, w.equity_withhold, w.equity_rclm "
    "FROM withrclm w "
    "JOIN currency c on w.curr_id = c.curr_id "
    "order by w.curr_id";

// ============================================================================
// Static State for Multi-Row Cursors
// ============================================================================

struct SelectAllAccdivState {
  std::optional<nanodbc::result> result;
  char sMode[2] = {0};
  char sType[2] = {0};
  int iID = 0;
  char sSecNo[20] = {0};
  char sWi[2] = {0};
  char sSecXtend[3] = {0};
  char sAcctType[2] = {0};
  long lTransNo = 0;
  long lEffDate = 0;
  int cRows = 0;
};

struct SelectWithrclmState {
  std::optional<nanodbc::result> result;
  int cRows = 0;
};

static SelectAllAccdivState g_accdivState;
static SelectWithrclmState g_withrclmState;

// ============================================================================
// Helper Functions
// ============================================================================

static void FillAccdivStruct(nanodbc::result &result, ACCDIV *pzAccdiv,
                             int *iSecType, double *fTradingUnit,
                             double *fCurExrate, double *fCurIncExrate) {
  memset(pzAccdiv, 0, sizeof(ACCDIV));

  pzAccdiv->iID = result.get<int>("id", 0);
  pzAccdiv->lTransNo = result.get<long>("trans_no", 0);
  pzAccdiv->lDivintNo = result.get<long>("divint_no", 0);
  read_string(result, "tran_type", pzAccdiv->sTranType,
              sizeof(pzAccdiv->sTranType));
  read_string(result, "sec_no", pzAccdiv->sSecNo, sizeof(pzAccdiv->sSecNo));
  read_string(result, "wi", pzAccdiv->sWi, sizeof(pzAccdiv->sWi));
  pzAccdiv->iSecID = result.get<int>("secid", 0);
  read_string(result, "sec_xtend", pzAccdiv->sSecXtend,
              sizeof(pzAccdiv->sSecXtend));

  read_string(result, "acct_type", pzAccdiv->sAcctType,
              sizeof(pzAccdiv->sAcctType));
  pzAccdiv->lEligDate = read_date(result, "elig_date");
  read_string(result, "sec_symbol", pzAccdiv->sSecSymbol,
              sizeof(pzAccdiv->sSecSymbol));
  read_string(result, "div_type", pzAccdiv->sDivType,
              sizeof(pzAccdiv->sDivType));
  pzAccdiv->fDivFactor = result.get<double>("div_factor", 0.0);
  pzAccdiv->fUnits = result.get<double>("units", 0.0);
  pzAccdiv->fOrigFace = result.get<double>("orig_face", 0.0);

  pzAccdiv->fPcplAmt = result.get<double>("pcpl_amt", 0.0);
  pzAccdiv->fIncomeAmt = result.get<double>("income_amt", 0.0);
  pzAccdiv->lTrdDate = read_date(result, "trd_date");
  pzAccdiv->lStlDate = read_date(result, "stl_date");
  pzAccdiv->lEffDate = read_date(result, "eff_date");
  pzAccdiv->lEntryDate = read_date(result, "entry_date");
  read_string(result, "curr_id", pzAccdiv->sCurrId, sizeof(pzAccdiv->sCurrId));

  read_string(result, "curr_acct_type", pzAccdiv->sCurrAcctType,
              sizeof(pzAccdiv->sCurrAcctType));
  read_string(result, "inc_curr_id", pzAccdiv->sIncCurrId,
              sizeof(pzAccdiv->sIncCurrId));
  read_string(result, "inc_acct_type", pzAccdiv->sIncAcctType,
              sizeof(pzAccdiv->sIncAcctType));
  read_string(result, "sec_curr_id", pzAccdiv->sSecCurrId,
              sizeof(pzAccdiv->sSecCurrId));
  read_string(result, "accr_curr_id", pzAccdiv->sAccrCurrId,
              sizeof(pzAccdiv->sAccrCurrId));

  pzAccdiv->fBaseXrate = result.get<double>("base_xrate", 0.0);
  pzAccdiv->fIncBaseXrate = result.get<double>("inc_base_xrate", 0.0);
  pzAccdiv->fSecBaseXrate = result.get<double>("sec_base_xrate", 0.0);
  pzAccdiv->fAccrBaseXrate = result.get<double>("accr_base_xrate", 0.0);
  pzAccdiv->fSysXrate = result.get<double>("sys_xrate", 0.0);

  pzAccdiv->fIncSysXrate = result.get<double>("inc_sys_xrate", 0.0);
  pzAccdiv->fOrigYld = result.get<double>("orig_yld", 0.0);
  pzAccdiv->lEffMatDate = read_date(result, "eff_mat_date");
  pzAccdiv->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);
  read_string(result, "acct_mthd", pzAccdiv->sAcctMthd,
              sizeof(pzAccdiv->sAcctMthd));
  read_string(result, "trans_srce", pzAccdiv->sTransSrce,
              sizeof(pzAccdiv->sTransSrce));
  read_string(result, "dr_cr", pzAccdiv->sDrCr, sizeof(pzAccdiv->sDrCr));

  read_string(result, "dtc_inclusion", pzAccdiv->sDtcInclusion,
              sizeof(pzAccdiv->sDtcInclusion));
  read_string(result, "dtc_resolve", pzAccdiv->sDtcResolve,
              sizeof(pzAccdiv->sDtcResolve));
  read_string(result, "income_flag", pzAccdiv->sIncomeFlag,
              sizeof(pzAccdiv->sIncomeFlag));
  read_string(result, "letter_flag", pzAccdiv->sLetterFlag,
              sizeof(pzAccdiv->sLetterFlag));
  read_string(result, "ledger_flag", pzAccdiv->sLedgerFlag,
              sizeof(pzAccdiv->sLedgerFlag));
  read_string(result, "created_by", pzAccdiv->sCreatedBy,
              sizeof(pzAccdiv->sCreatedBy));

  pzAccdiv->lCreateDate = read_date(result, "create_date");
  read_string(result, "create_time", pzAccdiv->sCreateTime,
              sizeof(pzAccdiv->sCreateTime));
  read_string(result, "suspend_flag", pzAccdiv->sSuspendFlag,
              sizeof(pzAccdiv->sSuspendFlag));
  read_string(result, "delete_flag", pzAccdiv->sDeleteFlag,
              sizeof(pzAccdiv->sDeleteFlag));

  // Additional fields from assets
  *iSecType = result.get<int>("sec_type", 0);
  *fTradingUnit = result.get<double>("trad_unit", 0.0);
  double fCurExrateAsset = result.get<double>("cur_exrate", 0.0);
  double fCurIncExrateAsset = result.get<double>("cur_inc_exrate", 0.0);

  // Exchange rates from histpric
  double fSecExrate = result.get<double>("secexrate", 0.0);
  double fSecIncExrate = result.get<double>("secincexrate", 0.0);
  double fCurrExrate = result.get<double>("currexrate", 0.0);
  double fCurrIncExrate = result.get<double>("currincexrate", 0.0);

  // Exchange rate priority logic
  // If security itself has an exchange rate for the effective date, use that,
  // else if there is an exchange rate for the income currency, use that, if
  // even that does not exists, use the current exchange rates from asset
  // record.
  if (fSecExrate > 0) {
    *fCurExrate = fSecExrate;
    *fCurIncExrate = fSecIncExrate;
  } else if (fCurrExrate > 0) {
    *fCurExrate = fCurrExrate;
    *fCurIncExrate = fCurrIncExrate;
  } else {
    *fCurExrate = fCurExrateAsset;
    *fCurIncExrate = fCurIncExrateAsset;
  }

  // If same currency but different exrate, synchronize them
  if (strcmp(pzAccdiv->sCurrId, pzAccdiv->sIncCurrId) == 0 &&
      *fCurExrate != *fCurIncExrate) {
    if (*fCurExrate > 0)
      *fCurIncExrate = *fCurExrate;
    else
      *fCurExrate = *fCurIncExrate;
  }
}

// ============================================================================
// SelectAllAccdiv
// ============================================================================

void STDCALL SelectAllAccdiv(ACCDIV *pzAccdiv, int *iSecType,
                             double *fTradingUnit, double *fCurExrate,
                             double *fCurIncExrate, char *sMode, char *sType,
                             int iID, char *sSecNo, char *sWi, char *sSecXtend,
                             char *sAcctType, long lTransNo, long lEffDate,
                             ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllAccdiv", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectAllAccdiv", FALSE);
    return;
  }

  try {
    // Build SQL based on mode and type
    std::string sql = SQL_SELECTACCDIV_BEGIN;

    if (sMode[0] == 'A') {
      sql += SQL_SELECTACCDIV_MODE_A;
    } else if (sMode[0] == 'S') {
      sql += SQL_SELECTACCDIV_MODE_S;
    }

    if (sType[0] == 'S') {
      sql += SQL_SELECTACCDIV_TYPE_S;
    } else if (sType[0] == 'I') {
      sql += SQL_SELECTACCDIV_TYPE_I;
    }

    sql += SQL_SELECTACCDIV_END;

    // Check if parameters changed
    bool needNewQuery = !(
        g_accdivState.sMode[0] == sMode[0] &&
        g_accdivState.sType[0] == sType[0] &&
        g_accdivState.lEffDate == lEffDate && g_accdivState.iID == iID &&
        (sMode[0] != 'S' || (strcmp(g_accdivState.sSecNo, sSecNo) == 0 &&
                             strcmp(g_accdivState.sWi, sWi) == 0 &&
                             strcmp(g_accdivState.sSecXtend, sSecXtend) == 0 &&
                             strcmp(g_accdivState.sAcctType, sAcctType) == 0 &&
                             g_accdivState.lTransNo == lTransNo)) &&
        g_accdivState.cRows > 0);

    if (needNewQuery) {
      g_accdivState.result.reset();
      g_accdivState.cRows = 0;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

      nanodbc::timestamp tsEffDate;
      long_to_timestamp(lEffDate, tsEffDate);

      int paramIndex = 0;
      stmt.bind(paramIndex++, &tsEffDate);

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

      g_accdivState.result = nanodbc::execute(stmt);
      g_accdivState.sMode[0] = sMode[0];
      g_accdivState.sType[0] = sType[0];
      g_accdivState.lEffDate = lEffDate;
      g_accdivState.iID = iID;
      if (sMode[0] == 'S') {
        strcpy_s(g_accdivState.sSecNo, sSecNo);
        strcpy_s(g_accdivState.sWi, sWi);
        strcpy_s(g_accdivState.sSecXtend, sSecXtend);
        strcpy_s(g_accdivState.sAcctType, sAcctType);
        g_accdivState.lTransNo = lTransNo;
      }
    }

    if (g_accdivState.result && g_accdivState.result->next()) {
      g_accdivState.cRows++;
      FillAccdivStruct(*g_accdivState.result, pzAccdiv, iSecType, fTradingUnit,
                       fCurExrate, fCurIncExrate);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
      g_accdivState.cRows = 0;
      g_accdivState.result.reset();
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectAllAccdiv: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectAllAccdiv", FALSE);
    g_accdivState.result.reset();
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in SelectAllAccdiv", 0, 0,
                        (char *)"", 0, -1, 0, (char *)"SelectAllAccdiv", FALSE);
    g_accdivState.result.reset();
  }
}

// ============================================================================
// UpdateAccdivDeleteFlag
// ============================================================================

void STDCALL UpdateAccdivDeleteFlag(char *sDeleteFlag, int iID, char *sSecNo,
                                    char *sWi, char *sSecXtend, char *sAcctType,
                                    long lTransNo, long lDivintNo,
                                    ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdateAccdivDeleteFlag", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"UpdateAccdivDeleteFlag", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UPDATE_ACCDIV_DELETE_FLAG));

    int paramIndex = 0;
    safe_bind_string(stmt, paramIndex, sDeleteFlag);
    stmt.bind(paramIndex++, &iID);
    safe_bind_string(stmt, paramIndex, sSecNo);
    safe_bind_string(stmt, paramIndex, sWi);
    safe_bind_string(stmt, paramIndex, sSecXtend);
    safe_bind_string(stmt, paramIndex, sAcctType);
    stmt.bind(paramIndex++, &lTransNo);
    stmt.bind(paramIndex++, &lDivintNo);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in UpdateAccdivDeleteFlag: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"UpdateAccdivDeleteFlag", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in UpdateAccdivDeleteFlag", 0,
                        0, (char *)"", 0, -1, 0,
                        (char *)"UpdateAccdivDeleteFlag", FALSE);
  }
}

// ============================================================================
// SelectPorttax
// ============================================================================

void STDCALL SelectPorttax(int iID, long lTaxDate, PORTTAX *pzPTax,
                           ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectPorttax", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectPorttax", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_PORTTAX));

    nanodbc::timestamp tsTaxDate;
    long_to_timestamp(lTaxDate, tsTaxDate);

    int paramIndex = 0;
    stmt.bind(paramIndex++, &iID);
    stmt.bind(paramIndex++, &tsTaxDate);

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      memset(pzPTax, 0, sizeof(PORTTAX));

      pzPTax->lTaxDate = read_date(result, "Taxdate");
      pzPTax->fFedIncomeRate = result.get<double>("FedIncomeRate", 0.0);
      pzPTax->fFedStGLRate = result.get<double>("FedStGLRate", 0.0);
      pzPTax->fFedLtGLRate = result.get<double>("FedLtGLRate", 0.0);
      pzPTax->fStIncomeRate = result.get<double>("StIncomeRate", 0.0);
      pzPTax->fStStGLRate = result.get<double>("StStGLRate", 0.0);
      pzPTax->fStLtGLRate = result.get<double>("StLtGLRate", 0.0);
      pzPTax->fStockWithholdRate = result.get<double>("StockWithholdRate", 0.0);
      pzPTax->fBondWithholdRate = result.get<double>("BondWithholdRate", 0.0);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectPorttax: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectPorttax", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in SelectPorttax", 0, 0,
                        (char *)"", 0, -1, 0, (char *)"SelectPorttax", FALSE);
  }
}

// ============================================================================
// SelectWithrclm
// ============================================================================

void STDCALL SelectWithrclm(WITHHOLDRCLM *pzWH, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectWithrclm", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectWithrclm", FALSE);
    return;
  }

  try {
    // If first call, execute query
    if (!g_withrclmState.cRows) {
      g_withrclmState.result.reset();
      g_withrclmState.cRows = 0;

      nanodbc::statement stmt(gConn);
      nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_WITHRCLM));

      g_withrclmState.result = nanodbc::execute(stmt);
    }

    if (g_withrclmState.result && g_withrclmState.result->next()) {
      g_withrclmState.cRows++;

      memset(pzWH, 0, sizeof(WITHHOLDRCLM));

      read_string(*g_withrclmState.result, "curr_id", pzWH->sCurrId,
                  sizeof(pzWH->sCurrId));
      pzWH->fFixedincWithhold =
          g_withrclmState.result->get<double>("fixedinc_withhold", 0.0);
      pzWH->fFixedincRclm =
          g_withrclmState.result->get<double>("fixedinc_rclm", 0.0);
      pzWH->fEquityWithhold =
          g_withrclmState.result->get<double>("equity_withhold", 0.0);
      pzWH->fEquityRclm =
          g_withrclmState.result->get<double>("equity_rclm", 0.0);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
      g_withrclmState.cRows = 0;
      g_withrclmState.result.reset();
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectWithrclm: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectWithrclm", FALSE);
    g_withrclmState.result.reset();
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in SelectWithrclm", 0, 0,
                        (char *)"", 0, -1, 0, (char *)"SelectWithrclm", FALSE);
    g_withrclmState.result.reset();
  }
}

// ============================================================================
// Initialization and Cleanup
// ============================================================================

ERRSTRUCT STDCALL InitializeDivIntPayIO(char *sMode, char *sType) {
  ERRSTRUCT zErr;
  InitializeErrStruct(&zErr);

  // No preparation needed with nanodbc - queries are prepared on first use

#ifdef DEBUG
  PrintError("InitializeDivIntPayIO called", 0, 0, "", 0, 0, 0,
             "InitializeDivIntPayIO", FALSE);
#endif

  return zErr;
}

void CloseDivIntPayIO(void) {
  // Reset all static states
  g_accdivState.result.reset();
  g_accdivState.cRows = 0;

  g_withrclmState.result.reset();
  g_withrclmState.cRows = 0;

#ifdef DEBUG
  PrintError("CloseDivIntPayIO called", 0, 0, "", 0, 0, 0, "CloseDivIntPayIO",
             FALSE);
#endif
}

void STDCALL FreeDivIntPayIO(void) {
  // Delegate to CloseDivIntPayIO
  CloseDivIntPayIO();

#ifdef DEBUG
  PrintError("FreeDivIntPayIO called (RAII cleanup)", 0, 0, "", 0, 0, 0,
             "FreeDivIntPayIO", FALSE);
#endif
}
