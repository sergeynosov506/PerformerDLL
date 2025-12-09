#include "TransIO_Holddel.h"
#include "ODBCErrorChecking.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;

// Helper to fill HOLDDEL struct from result
void FillHolddelStruct(nanodbc::result &result, HOLDDEL *pzHD) {
  pzHD->lCreateTransNo = result.get<long>("create_trans_no", 0);
  pzHD->lCreateDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("create_date"));
  pzHD->lRevTransNo = result.get<long>("rev_trans_no", 0);
  pzHD->iID = result.get<int>("ID", 0);
  read_string(result, "sec_no", pzHD->sSecNo, sizeof(pzHD->sSecNo));
  read_string(result, "wi", pzHD->sWi, sizeof(pzHD->sWi));
  read_string(result, "sec_xtend", pzHD->sSecXtend, sizeof(pzHD->sSecXtend));
  read_string(result, "acct_type", pzHD->sAcctType, sizeof(pzHD->sAcctType));
  pzHD->lTransNo = result.get<long>("trans_no", 0);
  pzHD->iSecID = result.get<int>("secid", 0);
  pzHD->lAsofDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("asof_date"));
  read_string(result, "sec_symbol", pzHD->sSecSymbol, sizeof(pzHD->sSecSymbol));
  pzHD->fUnits = result.get<double>("units", 0.0);
  pzHD->fOrigFace = result.get<double>("orig_face", 0.0);
  pzHD->fTotCost = result.get<double>("tot_cost", 0.0);
  pzHD->fUnitCost = result.get<double>("unit_cost", 0.0);
  pzHD->fOrigCost = result.get<double>("orig_cost", 0.0);
  pzHD->fOpenLiability = result.get<double>("open_liability", 0.0);
  pzHD->fBaseCostXrate = result.get<double>("base_cost_xrate", 0.0);
  pzHD->fSysCostXrate = result.get<double>("sys_cost_xrate", 0.0);
  pzHD->lTrdDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("trd_date"));
  pzHD->lEffDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("eff_date"));
  pzHD->lEligDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("elig_date"));
  pzHD->lStlDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("stl_date"));
  pzHD->fOrigYield = result.get<double>("orig_yield", 0.0);
  pzHD->lEffMatDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("eff_mat_date"));
  pzHD->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);
  pzHD->fCostEffMatYld = result.get<double>("cost_eff_mat_yld", 0.0);
  pzHD->lAmortStartDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("amort_start_date"));
  read_string(result, "orig_trans_type", pzHD->sOrigTransType,
              sizeof(pzHD->sOrigTransType));
  read_string(result, "orig_trans_srce", pzHD->sOrigTransSrce,
              sizeof(pzHD->sOrigTransSrce));
  read_string(result, "last_trans_type", pzHD->sLastTransType,
              sizeof(pzHD->sLastTransType));
  pzHD->lLastTransNo = result.get<long>("last_trans_no", 0);
  read_string(result, "last_trans_srce", pzHD->sLastTransSrce,
              sizeof(pzHD->sLastTransSrce));
  pzHD->lLastPmtDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("last_pmt_date"));
  read_string(result, "last_pmt_type", pzHD->sLastPmtType,
              sizeof(pzHD->sLastPmtType));
  pzHD->lLastPmtTrNo = result.get<long>("last_pmt_tr_no", 0);
  pzHD->lNextPmtDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("next_pmt_date"));
  pzHD->fNextPmtAmt = result.get<double>("next_pmt_amt", 0.0);
  pzHD->lLastPdnDate =
      timestamp_to_long(result.get<nanodbc::timestamp>("last_pdn_date"));
  read_string(result, "lt_st_ind", pzHD->sLtStInd, sizeof(pzHD->sLtStInd));
  pzHD->fMktVal = result.get<double>("mkt_val", 0.0);
  pzHD->fCurLiability = result.get<double>("cur_liability", 0.0);
  pzHD->fMvBaseXrate = result.get<double>("mv_base_xrate", 0.0);
  pzHD->fMvSysXrate = result.get<double>("mv_sys_xrate", 0.0);
  pzHD->fAccrInt = result.get<double>("accr_int", 0.0);
  pzHD->fAiBaseXrate = result.get<double>("ai_base_xrate", 0.0);
  pzHD->fAiSysXrate = result.get<double>("ai_sys_xrate", 0.0);
  pzHD->fAnnualIncome = result.get<double>("annual_income", 0.0);
  pzHD->fAccrualGl = result.get<double>("accrual_gl", 0.0);
  pzHD->fCurrencyGl = result.get<double>("currency_gl", 0.0);
  pzHD->fSecurityGl = result.get<double>("security_gl", 0.0);
  pzHD->fMktEffMatYld = result.get<double>("mkt_eff_mat_yld", 0.0);
  pzHD->fMktCurYld = result.get<double>("mkt_cur_yld", 0.0);
  read_string(result, "safek_ind", pzHD->sSafekInd, sizeof(pzHD->sSafekInd));
  pzHD->fCollateralUnits = result.get<double>("collateral_units", 0.0);
  pzHD->fHedgeValue = result.get<double>("hedge_value", 0.0);
  read_string(result, "benchmark_sec_no", pzHD->sBenchmarkSecNo,
              sizeof(pzHD->sBenchmarkSecNo));
  read_string(result, "perm_lt_flag", pzHD->sPermLtFlag,
              sizeof(pzHD->sPermLtFlag));
  read_string(result, "valuation_srce", pzHD->sValuationSrce,
              sizeof(pzHD->sValuationSrce));
  read_string(result, "primary_type", pzHD->sPrimaryType,
              sizeof(pzHD->sPrimaryType));
  pzHD->iRestrictionCode = result.get<int>("restriction_code", 0);
}

DLLAPI void STDCALL InsertHolddel(HOLDDEL zHD, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"T", 0,
                        -1, 0, (char *)"InsertHolddel", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "insert into holddel "
            "(create_trans_no, create_date, rev_trans_no, "
            "ID, sec_no,wi, sec_xtend, acct_type, trans_no, "
            "secid, asof_date, "
            "sec_symbol, units, orig_face, tot_cost,  "
            "unit_cost, orig_cost, open_liability, base_cost_xrate,  "
            "sys_cost_xrate, trd_date, eff_date, elig_date, "
            "stl_date, orig_yield, eff_mat_date, eff_mat_price, "
            "cost_eff_mat_yld, amort_start_date, orig_trans_type, "
            "orig_trans_srce, last_trans_type, last_trans_no, "
            "last_trans_srce, last_pmt_date, last_pmt_type, "
            "last_pmt_tr_no, next_pmt_date, next_pmt_amt, "
            "last_pdn_date, lt_st_ind, mkt_val, cur_liability, "
            "mv_base_xrate, mv_sys_xrate, accr_int, "
            "ai_base_xrate, ai_sys_xrate, annual_income,  "
            "accrual_gl, currency_gl, security_gl, mkt_eff_mat_yld, "
            "mkt_cur_yld, safek_ind, collateral_units, hedge_value, "
            "benchmark_sec_no, perm_lt_flag, valuation_srce, primary_type, "
            "restriction_code) "
            "values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));

    nanodbc::timestamp tsCreateDate, tsAsofDate, tsTrdDate, tsEffDate,
        tsEligDate, tsStlDate;
    nanodbc::timestamp tsEffMatDate, tsAmortStartDate, tsLastPmtDate,
        tsNextPmtDate, tsLastPdnDate;

    long_to_timestamp(zHD.lCreateDate, tsCreateDate);
    long_to_timestamp(zHD.lAsofDate, tsAsofDate);
    long_to_timestamp(zHD.lTrdDate, tsTrdDate);
    long_to_timestamp(zHD.lEffDate, tsEffDate);
    long_to_timestamp(zHD.lEligDate, tsEligDate);
    long_to_timestamp(zHD.lStlDate, tsStlDate);
    long_to_timestamp(zHD.lEffMatDate, tsEffMatDate);
    long_to_timestamp(zHD.lAmortStartDate, tsAmortStartDate);
    long_to_timestamp(zHD.lLastPmtDate, tsLastPmtDate);
    long_to_timestamp(zHD.lNextPmtDate, tsNextPmtDate);
    long_to_timestamp(zHD.lLastPdnDate, tsLastPdnDate);

    // Rounding
    zHD.fUnits = RoundDouble(zHD.fUnits, 5);
    zHD.fOrigFace = RoundDouble(zHD.fOrigFace, 3);
    zHD.fTotCost = RoundDouble(zHD.fTotCost, 3);
    zHD.fUnitCost = RoundDouble(zHD.fUnitCost, 6);
    zHD.fOrigCost = RoundDouble(zHD.fOrigCost, 3);
    zHD.fOpenLiability = RoundDouble(zHD.fOpenLiability, 2);
    zHD.fBaseCostXrate = RoundDouble(zHD.fBaseCostXrate, 12);
    zHD.fSysCostXrate = RoundDouble(zHD.fSysCostXrate, 12);
    zHD.fOrigYield = RoundDouble(zHD.fOrigYield, 6);
    zHD.fEffMatPrice = RoundDouble(zHD.fEffMatPrice, 6);
    zHD.fCostEffMatYld = RoundDouble(zHD.fCostEffMatYld, 6);
    zHD.fNextPmtAmt = RoundDouble(zHD.fNextPmtAmt, 2);
    zHD.fMktVal = RoundDouble(zHD.fMktVal, 2);
    zHD.fCurLiability = RoundDouble(zHD.fCurLiability, 2);
    zHD.fMvBaseXrate = RoundDouble(zHD.fMvBaseXrate, 12);
    zHD.fMvSysXrate = RoundDouble(zHD.fMvSysXrate, 12);
    zHD.fAccrInt = RoundDouble(zHD.fAccrInt, 2);
    zHD.fAiBaseXrate = RoundDouble(zHD.fAiBaseXrate, 12);
    zHD.fAiSysXrate = RoundDouble(zHD.fAiSysXrate, 12);
    zHD.fAnnualIncome = RoundDouble(zHD.fAnnualIncome, 2);
    zHD.fAccrualGl = RoundDouble(zHD.fAccrualGl, 2);
    zHD.fCurrencyGl = RoundDouble(zHD.fCurrencyGl, 2);
    zHD.fSecurityGl = RoundDouble(zHD.fSecurityGl, 2);
    zHD.fMktEffMatYld = RoundDouble(zHD.fMktEffMatYld, 6);
    zHD.fMktCurYld = RoundDouble(zHD.fMktCurYld, 6);
    zHD.fCollateralUnits = RoundDouble(zHD.fCollateralUnits, 5);
    zHD.fHedgeValue = RoundDouble(zHD.fHedgeValue, 2);

    int idx = 0;
    stmt.bind(idx++, &zHD.lCreateTransNo);
    stmt.bind(idx++, &tsCreateDate);
    stmt.bind(idx++, &zHD.lRevTransNo);
    stmt.bind(idx++, &zHD.iID);
    safe_bind_string(stmt, idx, zHD.sSecNo);
    safe_bind_string(stmt, idx, zHD.sWi);
    safe_bind_string(stmt, idx, zHD.sSecXtend);
    safe_bind_string(stmt, idx, zHD.sAcctType);
    stmt.bind(idx++, &zHD.lTransNo);
    stmt.bind(idx++, &zHD.iSecID);
    stmt.bind(idx++, &tsAsofDate);
    safe_bind_string(stmt, idx, zHD.sSecSymbol);
    stmt.bind(idx++, &zHD.fUnits);
    stmt.bind(idx++, &zHD.fOrigFace);
    stmt.bind(idx++, &zHD.fTotCost);
    stmt.bind(idx++, &zHD.fUnitCost);
    stmt.bind(idx++, &zHD.fOrigCost);
    stmt.bind(idx++, &zHD.fOpenLiability);
    stmt.bind(idx++, &zHD.fBaseCostXrate);
    stmt.bind(idx++, &zHD.fSysCostXrate);
    stmt.bind(idx++, &tsTrdDate);
    stmt.bind(idx++, &tsEffDate);
    stmt.bind(idx++, &tsEligDate);
    stmt.bind(idx++, &tsStlDate);
    stmt.bind(idx++, &zHD.fOrigYield);
    stmt.bind(idx++, &tsEffMatDate);
    stmt.bind(idx++, &zHD.fEffMatPrice);
    stmt.bind(idx++, &zHD.fCostEffMatYld);
    stmt.bind(idx++, &tsAmortStartDate);
    safe_bind_string(stmt, idx, zHD.sOrigTransType);
    safe_bind_string(stmt, idx, zHD.sOrigTransSrce);
    safe_bind_string(stmt, idx, zHD.sLastTransType);
    stmt.bind(idx++, &zHD.lLastTransNo);
    safe_bind_string(stmt, idx, zHD.sLastTransSrce);
    stmt.bind(idx++, &tsLastPmtDate);
    safe_bind_string(stmt, idx, zHD.sLastPmtType);
    stmt.bind(idx++, &zHD.lLastPmtTrNo);
    stmt.bind(idx++, &tsNextPmtDate);
    stmt.bind(idx++, &zHD.fNextPmtAmt);
    stmt.bind(idx++, &tsLastPdnDate);
    safe_bind_string(stmt, idx, zHD.sLtStInd);
    stmt.bind(idx++, &zHD.fMktVal);
    stmt.bind(idx++, &zHD.fCurLiability);
    stmt.bind(idx++, &zHD.fMvBaseXrate);
    stmt.bind(idx++, &zHD.fMvSysXrate);
    stmt.bind(idx++, &zHD.fAccrInt);
    stmt.bind(idx++, &zHD.fAiBaseXrate);
    stmt.bind(idx++, &zHD.fAiSysXrate);
    stmt.bind(idx++, &zHD.fAnnualIncome);
    stmt.bind(idx++, &zHD.fAccrualGl);
    stmt.bind(idx++, &zHD.fCurrencyGl);
    stmt.bind(idx++, &zHD.fSecurityGl);
    stmt.bind(idx++, &zHD.fMktEffMatYld);
    stmt.bind(idx++, &zHD.fMktCurYld);
    safe_bind_string(stmt, idx, zHD.sSafekInd);
    stmt.bind(idx++, &zHD.fCollateralUnits);
    stmt.bind(idx++, &zHD.fHedgeValue);
    safe_bind_string(stmt, idx, zHD.sBenchmarkSecNo);
    safe_bind_string(stmt, idx, zHD.sPermLtFlag);
    safe_bind_string(stmt, idx, zHD.sValuationSrce);
    safe_bind_string(stmt, idx, zHD.sPrimaryType);
    stmt.bind(idx++, &zHD.iRestrictionCode);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in InsertHolddel: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), zHD.iID, zHD.lTransNo, (char *)"T",
                        0, -1, 0, (char *)"InsertHolddel", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in InsertHolddel", zHD.iID,
                        zHD.lTransNo, (char *)"T", 0, -1, 0,
                        (char *)"InsertHolddel", FALSE);
  }
}

DLLAPI void STDCALL SelectHolddel(HOLDDEL *pzHD, long lCreateTransNo,
                                  long lCreateDate, int iID, char *sSecNo,
                                  char *sWi, char *sSecXtend, char *sAcctType,
                                  long lTransNo, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"T", 0,
                        -1, 0, (char *)"SelectHolddel", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "select "
            "create_trans_no, create_date, rev_trans_no, "
            "ID, sec_no,wi, sec_xtend, acct_type, trans_no, "
            "secid, asof_date, "
            "sec_symbol, units, orig_face, tot_cost,  "
            "unit_cost, orig_cost, open_liability, base_cost_xrate,  "
            "sys_cost_xrate, trd_date, eff_date, elig_date, "
            "stl_date, orig_yield, eff_mat_date, eff_mat_price, "
            "cost_eff_mat_yld, amort_start_date, orig_trans_type, "
            "orig_trans_srce, last_trans_type, last_trans_no, "
            "last_trans_srce, last_pmt_date, last_pmt_type, "
            "last_pmt_tr_no, next_pmt_date, next_pmt_amt, "
            "last_pdn_date, lt_st_ind, mkt_val, cur_liability, "
            "mv_base_xrate, mv_sys_xrate, accr_int, "
            "ai_base_xrate, ai_sys_xrate, annual_income,  "
            "accrual_gl, currency_gl, security_gl, mkt_eff_mat_yld, "
            "mkt_cur_yld, safek_ind, collateral_units, hedge_value, "
            "benchmark_sec_no, perm_lt_flag, valuation_srce, primary_type, "
            "restriction_code "
            "from holddel "
            "where create_trans_no=? and create_date=? and "
            "ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=? and trans_no=?"));

    nanodbc::timestamp tsCreateDate;
    long_to_timestamp(lCreateDate, tsCreateDate);

    int idx = 0;
    stmt.bind(idx++, &lCreateTransNo);
    stmt.bind(idx++, &tsCreateDate);
    stmt.bind(idx++, &iID);
    safe_bind_string(stmt, idx, sSecNo);
    safe_bind_string(stmt, idx, sWi);
    safe_bind_string(stmt, idx, sSecXtend);
    safe_bind_string(stmt, idx, sAcctType);
    stmt.bind(idx++, &lTransNo);

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      FillHolddelStruct(result, pzHD);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectHolddel: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), iID, lTransNo, (char *)"T", 0, -1,
                        0, (char *)"SelectHolddel", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in SelectHolddel", iID, lTransNo,
                   (char *)"T", 0, -1, 0, (char *)"SelectHolddel", FALSE);
  }
}

DLLAPI void STDCALL HolddelUpdate(int iID, long lRevTransNo,
                                  long lCreateTransNo, long lCreateDate,
                                  ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"T", 0,
                        -1, 0, (char *)"HolddelUpdate", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "UPDATE holddel SET rev_trans_no = ? "
            "WHERE id = ? and create_trans_no = ?  and create_date = ? "));

    nanodbc::timestamp tsCreateDate;
    long_to_timestamp(lCreateDate, tsCreateDate);

    stmt.bind(0, &lRevTransNo);
    stmt.bind(1, &iID);
    stmt.bind(2, &lCreateTransNo);
    stmt.bind(3, &tsCreateDate);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in HolddelUpdate: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), iID, 0, (char *)"T", 0, -1, 0,
                        (char *)"HolddelUpdate", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in HolddelUpdate", iID, 0,
                        (char *)"T", 0, -1, 0, (char *)"HolddelUpdate", FALSE);
  }
}
