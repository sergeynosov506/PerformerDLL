#include "RollIO_Trans.h"
#include "ODBCErrorChecking.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"

extern thread_local nanodbc::connection gConn;

static void FillTransStruct(nanodbc::result &result, TRANS *pzTR) {
  // 1: id
  pzTR->iID = result.get<int>(0, 0);
  // 2: trans_no
  pzTR->lTransNo = result.get<int>(1, 0);
  // 3: tran_type
  strcpy_s(pzTR->sTranType, STR3LEN, result.get<std::string>(2, "").c_str());
  // 4: sec_no
  strcpy_s(pzTR->sSecNo, STR12LEN, result.get<std::string>(3, "").c_str());
  // 5: wi
  strcpy_s(pzTR->sWi, STR1LEN, result.get<std::string>(4, "").c_str());

  // 6: sec_xtend
  strcpy_s(pzTR->sSecXtend, STR1LEN, result.get<std::string>(5, "").c_str());
  // 7: acct_type
  strcpy_s(pzTR->sAcctType, STR3LEN, result.get<std::string>(6, "").c_str());
  // 8: secid
  pzTR->iSecID = result.get<int>(7, 0);
  // 9: sec_symbol
  strcpy_s(pzTR->sSecSymbol, STR20LEN, result.get<std::string>(8, "").c_str());
  // 10: units
  pzTR->fUnits = result.get<double>(9, 0.0);

  // 11: orig_face
  pzTR->fOrigFace = result.get<double>(10, 0.0);
  // 12: unit_cost
  pzTR->fUnitCost = result.get<double>(11, 0.0);
  // 13: tot_cost
  pzTR->fTotCost = result.get<double>(12, 0.0);
  // 14: orig_cost
  pzTR->fOrigCost = result.get<double>(13, 0.0);

  // 15: pcpl_amt
  pzTR->fPcplAmt = result.get<double>(14, 0.0);
  // 16: opt_prem
  pzTR->fOptPrem = result.get<double>(15, 0.0);
  // 17: amort_val
  pzTR->fAmortVal = result.get<double>(16, 0.0);
  // 18: basis_adj
  pzTR->fBasisAdj = result.get<double>(17, 0.0);

  // 19: comm_gcr
  pzTR->fCommGcr = result.get<double>(18, 0.0);
  // 20: net_comm
  pzTR->fNetComm = result.get<double>(19, 0.0);
  // 21: comm_code
  strcpy_s(pzTR->sCommCode, STR5LEN, result.get<std::string>(20, "").c_str());
  // 22: sec_fees
  pzTR->fSecFees = result.get<double>(21, 0.0);

  // 23: misc_fee1
  pzTR->fMiscFee1 = result.get<double>(22, 0.0);
  // 24: fee_code1
  strcpy_s(pzTR->sFeeCode1, STR5LEN, result.get<std::string>(23, "").c_str());
  // 25: misc_fee2
  pzTR->fMiscFee2 = result.get<double>(24, 0.0);
  // 26: fee_code2
  strcpy_s(pzTR->sFeeCode2, STR5LEN, result.get<std::string>(25, "").c_str());

  // 27: accr_int
  pzTR->fAccrInt = result.get<double>(26, 0.0);
  // 28: income_amt
  pzTR->fIncomeAmt = result.get<double>(27, 0.0);
  // 29: net_flow
  pzTR->fNetFlow = result.get<double>(28, 0.0);
  // 30: broker_code
  strcpy_s(pzTR->sBrokerCode, STR12LEN,
           result.get<std::string>(29, "").c_str());

  // 31: broker_code2
  strcpy_s(pzTR->sBrokerCode2, STR12LEN,
           result.get<std::string>(30, "").c_str());
  // 32: trd_date
  pzTR->lTrdDate = timestamp_to_long(result.get<nanodbc::timestamp>(31));
  // 33: stl_date
  pzTR->lStlDate = timestamp_to_long(result.get<nanodbc::timestamp>(32));
  // 34: eff_date
  pzTR->lEffDate = timestamp_to_long(result.get<nanodbc::timestamp>(33));

  // 35: entry_date
  pzTR->lEntryDate = timestamp_to_long(result.get<nanodbc::timestamp>(34));
  // 36: taxlot_no
  pzTR->lTaxlotNo = result.get<int>(35, 0);
  // 37: xref_trans_no
  pzTR->lXrefTransNo = result.get<int>(36, 0);

  // 38: pend_div_no
  pzTR->lPendDivNo = result.get<int>(37, 0);
  // 39: rev_trans_no
  pzTR->lRevTransNo = result.get<int>(38, 0);
  // 40: rev_type
  strcpy_s(pzTR->sRevType, STR1LEN, result.get<std::string>(39, "").c_str());

  // 41: new_trans_no
  pzTR->lNewTransNo = result.get<int>(40, 0);
  // 42: orig_trans_no
  pzTR->lOrigTransNo = result.get<int>(41, 0);
  // 43: block_trans_no
  pzTR->lBlockTransNo = result.get<int>(42, 0);

  // 44: x_id
  pzTR->iXID = result.get<int>(43, 0);
  // 45: x_trans_no
  pzTR->lXTransNo = result.get<int>(44, 0);
  // 46: x_sec_no
  strcpy_s(pzTR->sXSecNo, STR12LEN, result.get<std::string>(45, "").c_str());
  // 47: x_wi
  strcpy_s(pzTR->sXWi, STR1LEN, result.get<std::string>(46, "").c_str());

  // 48: x_sec_xtend
  strcpy_s(pzTR->sXSecXtend, STR1LEN, result.get<std::string>(47, "").c_str());
  // 49: x_acct_type
  strcpy_s(pzTR->sXAcctType, STR3LEN, result.get<std::string>(48, "").c_str());
  // 50: x_secid
  pzTR->iXSecID = result.get<int>(49, 0);
  // 51: curr_id
  strcpy_s(pzTR->sCurrId, STR3LEN, result.get<std::string>(50, "").c_str());

  // 52: curr_acct_type
  strcpy_s(pzTR->sCurrAcctType, STR3LEN,
           result.get<std::string>(51, "").c_str());
  // 53: inc_curr_id
  strcpy_s(pzTR->sIncCurrId, STR3LEN, result.get<std::string>(52, "").c_str());
  // 54: inc_acct_type
  strcpy_s(pzTR->sIncAcctType, STR3LEN,
           result.get<std::string>(53, "").c_str());

  // 55: x_curr_id
  strcpy_s(pzTR->sXCurrId, STR3LEN, result.get<std::string>(54, "").c_str());
  // 56: x_curr_acct_type
  strcpy_s(pzTR->sXCurrAcctType, STR3LEN,
           result.get<std::string>(55, "").c_str());
  // 57: sec_curr_id
  strcpy_s(pzTR->sSecCurrId, STR3LEN, result.get<std::string>(56, "").c_str());

  // 58: accr_curr_id
  strcpy_s(pzTR->sAccrCurrId, STR3LEN, result.get<std::string>(57, "").c_str());
  // 59: base_xrate
  pzTR->fBaseXrate = result.get<double>(58, 0.0);
  // 60: inc_base_xrate
  pzTR->fIncBaseXrate = result.get<double>(59, 0.0);

  // 61: sec_base_xrate
  pzTR->fSecBaseXrate = result.get<double>(60, 0.0);
  // 62: accr_base_xrate
  pzTR->fAccrBaseXrate = result.get<double>(61, 0.0);
  // 63: sys_xrate
  pzTR->fSysXrate = result.get<double>(62, 0.0);

  // 64: inc_sys_xrate
  pzTR->fIncSysXrate = result.get<double>(63, 0.0);
  // 65: base_open_xrate
  pzTR->fBaseOpenXrate = result.get<double>(64, 0.0);
  // 66: sys_open_xrate
  pzTR->fSysOpenXrate = result.get<double>(65, 0.0);

  // 67: open_trd_date
  pzTR->lOpenTrdDate = timestamp_to_long(result.get<nanodbc::timestamp>(66));
  // 68: open_stl_date
  pzTR->lOpenStlDate = timestamp_to_long(result.get<nanodbc::timestamp>(67));
  // 69: open_unit_cost
  pzTR->fOpenUnitCost = result.get<double>(68, 0.0);

  // 70: orig_yld
  pzTR->fOrigYld = result.get<double>(69, 0.0);
  // 71: eff_mat_date
  pzTR->lEffMatDate = timestamp_to_long(result.get<nanodbc::timestamp>(70));
  // 72: eff_mat_price
  pzTR->fEffMatPrice = result.get<double>(71, 0.0);

  // 73: acct_mthd
  strcpy_s(pzTR->sAcctMthd, STR1LEN, result.get<std::string>(72, "").c_str());
  // 74: trans_srce
  strcpy_s(pzTR->sTransSrce, STR8LEN, result.get<std::string>(73, "").c_str());
  // 75: adp_tag
  strcpy_s(pzTR->sAdpTag, STR8LEN, result.get<std::string>(74, "").c_str());
  // 76: div_type
  strcpy_s(pzTR->sDivType, STR1LEN, result.get<std::string>(75, "").c_str());

  // 77: div_factor
  pzTR->fDivFactor = result.get<double>(76, 0.0);
  // 78: divint_no
  pzTR->lDivintNo = result.get<int>(77, 0);
  // 79: roll_date
  pzTR->lRollDate = timestamp_to_long(result.get<nanodbc::timestamp>(78));
  // 80: perf_date
  pzTR->lPerfDate = timestamp_to_long(result.get<nanodbc::timestamp>(79));

  // 81: misc_desc_ind
  strcpy_s(pzTR->sMiscDescInd, STR1LEN,
           result.get<std::string>(80, "").c_str());
  // 82: dr_cr
  strcpy_s(pzTR->sDrCr, STR1LEN, result.get<std::string>(81, "").c_str());
  // 83: bal_to_adjust
  strcpy_s(pzTR->sBalToAdjust, STR1LEN,
           result.get<std::string>(82, "").c_str());
  // 84: cap_trans
  strcpy_s(pzTR->sCapTrans, STR1LEN, result.get<std::string>(83, "").c_str());

  // 85: safek_ind
  strcpy_s(pzTR->sSafekInd, STR1LEN, result.get<std::string>(84, "").c_str());
  // 86: dtc_inclusion
  strcpy_s(pzTR->sDtcInclusion, STR1LEN,
           result.get<std::string>(85, "").c_str());
  // 87: dtc_resolve
  strcpy_s(pzTR->sDtcResolve, STR1LEN, result.get<std::string>(86, "").c_str());

  // 88: recon_flag
  strcpy_s(pzTR->sReconFlag, STR1LEN, result.get<std::string>(87, "").c_str());
  // 89: recon_srce
  strcpy_s(pzTR->sReconSrce, STR1LEN, result.get<std::string>(88, "").c_str());
  // 90: income_flag
  strcpy_s(pzTR->sIncomeFlag, STR1LEN, result.get<std::string>(89, "").c_str());
  // 91: letter_flag
  strcpy_s(pzTR->sLetterFlag, STR1LEN, result.get<std::string>(90, "").c_str());

  // 92: ledger_flag
  strcpy_s(pzTR->sLedgerFlag, STR1LEN, result.get<std::string>(91, "").c_str());
  // 93: gl_flag
  strcpy_s(pzTR->sGlFlag, STR1LEN, result.get<std::string>(92, "").c_str());
  // 94: created_by
  strcpy_s(pzTR->sCreatedBy, STR30LEN, result.get<std::string>(93, "").c_str());
  // 95: create_date
  pzTR->lCreateDate = timestamp_to_long(result.get<nanodbc::timestamp>(94));

  // 96: create_time
  strcpy_s(pzTR->sCreateTime, STR8LEN, result.get<std::string>(95, "").c_str());
  // 97: post_date
  pzTR->lPostDate = timestamp_to_long(result.get<nanodbc::timestamp>(96));
  // 98: bkof_frmt
  strcpy_s(pzTR->sBkofFrmt, STR1LEN, result.get<std::string>(97, "").c_str());
  // 99: bkof_seq_no
  pzTR->lBkofSeqNo = result.get<int>(98, 0);

  // 100: dtrans_no
  pzTR->lDtransNo = result.get<int>(99, 0);
  // 101: price
  pzTR->fPrice = result.get<double>(100, 0.0);
  // 102: restriction_code
  pzTR->iRestrictionCode = result.get<int>(101, 0);
}

DLLAPI void STDCALL SelectForwardTrans(long iID, long lEffDate1, long lEffDate2,
                                       char *sSecNo, char *sWi,
                                       BOOL bSpecificSecNo, TRANS *pzTR,
                                       ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectForwardTrans", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND rev_trans_no = 0 AND "
            "eff_date > ? AND eff_date <= ? ORDER BY trans_no"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts1;
    long_to_timestamp(lEffDate1, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lEffDate2, ts2);
    stmt.bind(2, &ts2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectForwardTrans", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectBackwardTrans(long iID, long lEffDate1,
                                        long lEffDate2, char *sSecNo, char *sWi,
                                        BOOL bSpecificSecNo, TRANS *pzTR,
                                        ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectBackwardTrans", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND rev_trans_no = 0 AND "
            "eff_date > ? AND eff_date <= ? ORDER BY trans_no DESC"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts1;
    long_to_timestamp(lEffDate1, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lEffDate2, ts2);
    stmt.bind(2, &ts2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectBackwardTrans", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectAsofTrans(long iID, long lEffDate, long lTransNo,
                                    char *sSecNo, char *sWi,
                                    BOOL bSpecificSecNo, TRANS *pzTR,
                                    ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAsofTrans", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND eff_date <= ? AND "
            "trans_no > ? order by trans_no"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts;
    long_to_timestamp(lEffDate, ts);
    stmt.bind(1, &ts);
    stmt.bind(2, &lTransNo);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectAsofTrans", 0, 0, "", 0, 0, 0, (char *)e.what(),
                        FALSE);
  }
}

DLLAPI void STDCALL SelectTransByTransNo(long iID, long lTransNo1,
                                         long lTransNo2, char *sSecNo,
                                         char *sWi, BOOL bSpecificSecNo,
                                         TRANS *pzTR, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransByTransNo", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND trans_no >= ? AND "
            "trans_no <= ?"));

    stmt.bind(0, &iID);
    stmt.bind(1, &lTransNo1);
    stmt.bind(2, &lTransNo2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectTransByTransNo", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectTransByTradeDate(long iID, long lTrdDate1,
                                           long lTrdDate2, char *sSecNo,
                                           char *sWi, BOOL bSpecificSecNo,
                                           TRANS *pzTR, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransByTradeDate", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND trd_date >= ? AND "
            "trd_date <= ?"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts1;
    long_to_timestamp(lTrdDate1, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lTrdDate2, ts2);
    stmt.bind(2, &ts2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectTransByTradeDate", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectTransByEffectiveDate(long iID, long lEffDate1,
                                               long lEffDate2, char *sSecNo,
                                               char *sWi, BOOL bSpecificSecNo,
                                               TRANS *pzTR, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransByEffectiveDate",
             FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND eff_date >= ? AND "
            "eff_date <= ?"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts1;
    long_to_timestamp(lEffDate1, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lEffDate2, ts2);
    stmt.bind(2, &ts2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectTransByEffectiveDate", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectTransByEntryDate(long iID, long lEntryDate1,
                                           long lEntryDate2, char *sSecNo,
                                           char *sWi, BOOL bSpecificSecNo,
                                           TRANS *pzTR, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransByEntryDate", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND entry_date >= ? AND "
            "entry_date <= ?"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts1;
    long_to_timestamp(lEntryDate1, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lEntryDate2, ts2);
    stmt.bind(2, &ts2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectTransByEntryDate", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectAsofTransCount(long iID, long lEffDate, long lTransNo,
                                         char *sSecNo, char *sWi,
                                         BOOL bSpecificSecNo, long *piCount,
                                         ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAsofTransCount", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt,
                     NANODBC_TEXT("SELECT count(*) FROM trans WHERE id = ? AND "
                                  "eff_date <= ? AND trans_no > ?"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts;
    long_to_timestamp(lEffDate, ts);
    stmt.bind(1, &ts);
    stmt.bind(2, &lTransNo);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      *piCount = result.get<int>(0);
    } else {
      *piCount = 0;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectAsofTransCount", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectRegularTransCount(long iID, long lEffDate1,
                                            long lEffDate2, char *sSecNo,
                                            char *sWi, BOOL bSpecificSecNo,
                                            long *piCount, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectRegularTransCount", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT("SELECT count(*) FROM trans WHERE id = ? AND rev_trans_no "
                     "= 0 AND eff_date > ? AND eff_date <= ?"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts1;
    long_to_timestamp(lEffDate1, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lEffDate2, ts2);
    stmt.bind(2, &ts2);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      *piCount = result.get<int>(0);
    } else {
      *piCount = 0;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectRegularTransCount", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL CheckIfTransExists(long iID, long *piCount,
                                       ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "CheckIfTransExists", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt,
                     NANODBC_TEXT("SELECT count(*) FROM trans WHERE id = ?"));

    stmt.bind(0, &iID);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      *piCount = result.get<int>(0);
    } else {
      *piCount = 0;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("CheckIfTransExists", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL UpdatePerfDate(long iId, long lStartDate, long lEndDate,
                                   ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdatePerfDate", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT(
                  "Update trans set perf_date = eff_date where id = ? and "
                  "eff_date > ? and eff_date <= ? and eff_date <> perf_date"));

    stmt.bind(0, &iId);
    nanodbc::timestamp ts1;
    long_to_timestamp(lStartDate, ts1);
    stmt.bind(1, &ts1);
    nanodbc::timestamp ts2;
    long_to_timestamp(lEndDate, ts2);
    stmt.bind(2, &ts2);

    stmt.bind(2, &ts2);

    stmt.execute();
  } catch (const std::exception &e) {
    *pzErr = PrintError("UpdatePerfDate", 0, 0, "", 0, 0, 0, (char *)e.what(),
                        FALSE);
  }
}

DLLAPI void STDCALL SelectTransBySettlementDate(long iID, long lStlDate,
                                                TRANS *pzTR, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransBySettlementDate",
             FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT id, trans_no, tran_type, sec_no, wi, sec_xtend, acct_type, "
            "secid, sec_symbol, units, orig_face, unit_cost, tot_cost, "
            "orig_cost, pcpl_amt, opt_prem, amort_val, basis_adj, comm_gcr, "
            "net_comm, comm_code, sec_fees, misc_fee1, fee_code1, misc_fee2, "
            "fee_code2, accr_int, income_amt, net_flow, broker_code, "
            "broker_code2, trd_date, stl_date, eff_date, entry_date, "
            "taxlot_no, xref_trans_no, pend_div_no, rev_trans_no, rev_type, "
            "new_trans_no, orig_trans_no, block_trans_no, x_id, x_trans_no, "
            "x_sec_no, x_wi, x_sec_xtend, x_acct_type, x_secid, curr_id, "
            "curr_acct_type, inc_curr_id, inc_acct_type, x_curr_id, "
            "x_curr_acct_type, sec_curr_id, accr_curr_id, base_xrate, "
            "inc_base_xrate, sec_base_xrate, accr_base_xrate, sys_xrate, "
            "inc_sys_xrate, base_open_xrate, sys_open_xrate, open_trd_date, "
            "open_stl_date, open_unit_cost, orig_yld, eff_mat_date, "
            "eff_mat_price, acct_mthd, trans_srce, adp_tag, div_type, "
            "div_factor, divint_no, roll_date, perf_date, misc_desc_ind, "
            "dr_cr, bal_to_adjust, cap_trans, safek_ind, dtc_inclusion, "
            "dtc_resolve, recon_flag, recon_srce, income_flag, letter_flag, "
            "ledger_flag, gl_flag, created_by, create_date, create_time, "
            "post_date, bkof_frmt, bkof_seq_no, dtrans_no, price, "
            "restriction_code FROM trans WHERE id = ? AND stl_date = ?"));

    stmt.bind(0, &iID);
    nanodbc::timestamp ts;
    long_to_timestamp(lStlDate, ts);
    stmt.bind(1, &ts);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      FillTransStruct(result, pzTR);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectTransBySettlementDate", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}
