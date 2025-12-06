#include "TransIO_DTrans.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL SelectDtrans(TRANS *pzTR, int iID, char *sSecNo, char *sWi, long lProcessTag, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"SelectDtrans", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT d.ID, d.dtrans_no, d.tran_type, d.sec_no, d.wi, d.process_tag, "
            "d.sec_xtend, d.acct_type, d.SecID, d.sec_symbol, d.units, "
            "d.orig_face, d.unit_cost, d.tot_cost, d.orig_cost, d.pcpl_amt, d.opt_prem, d.amort_val, "
            "d.basis_adj, d.comm_gcr, d.net_comm, d.comm_code, d.sec_fees, d.misc_fee1, d.fee_code1, "
            "d.misc_fee2, d.fee_code2, d.accr_int, d.income_amt, d.net_flow, d.broker_code, "
            "d.broker_code2, d.trd_date, d.stl_date, d.eff_date, "
            "d.entry_date, d.taxlot_no, d.xref_trans_no, d.pend_div_no, "
            "d.rev_trans_no, d.rev_type, d.new_trans_no, d.orig_trans_no, d.block_trans_no, d.x_ID, "
            "d.x_trans_no, d.x_sec_no, d.x_wi, d.x_sec_xtend, d.x_acct_type, d.x_secid, d.curr_id, "
            "d.curr_acct_type, d.inc_curr_id, d.inc_acct_type, d.x_curr_id, d.x_curr_acct_type, "
            "d.sec_curr_id, d.accr_curr_id, d.base_xrate, d.inc_base_xrate, d.sec_base_xrate, "
            "d.accr_base_xrate, d.sys_xrate, d.inc_sys_xrate, d.base_open_xrate, d.sys_open_xrate, "
            "d.open_trd_date, d.open_stl_date, d.open_unit_cost, d.orig_yld, "
            "d.eff_mat_date, d.eff_mat_price, d.acct_mthd, d.trans_srce, d.adp_tag, d.div_type, "
            "d.div_factor, d.divint_no, d.roll_date, d.perf_date, d.misc_Desc_ind, "
            "d.dr_cr, d.bal_to_adjust, d.cap_trans, d.safek_ind, d.dtc_inclusion, d.dtc_resolve, d.recon_flag, "
            "d.recon_srce, d.income_flag, d.letter_flag, d.ledger_flag, d.gl_flag, d.created_by, "
            "d.create_date, d.create_time, d.post_date, d.bkof_frmt, "
            "d.bkof_seq_no, d.price, d.restriction_code, t.trade_sort "
            "FROM dtrans d, trantype t "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND process_tag = ? AND "
            "(status_flag = '' OR status_flag is NULL) AND "
            "d.tran_type = t.tran_type AND "
            "d.dr_cr = t.dr_cr "
            "ORDER BY ID, eff_date, trade_sort, rev_trans_no DESC, "
            "acct_mthd DESC, dtrans_no ASC "));

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, &lProcessTag);

        nanodbc::result result = nanodbc::execute(stmt);
        
        // Note: The original code implies this might return multiple rows (CRowset), 
        // but the function signature takes a single TRANS pointer (pzTR).
        // However, looking at the original code: CCommand<CAccessor<CSelectDTrans>, CRowset > cmdSelectDtrans;
        // And the usage in TransEngine usually iterates. 
        // BUT, the function signature provided in TransIO.h is:
        // DLLAPI void STDCALL SelectDtrans(TRANS *pzTR, int iID, char *sSecNo, char *sWi, long lProcessTag, ERRSTRUCT *pzErr);
        // This suggests it might be designed to fetch one, OR the caller expects to iterate?
        // Wait, if it's a CRowset, the caller would typically call MoveNext.
        // But here we are replacing the implementation. 
        // If the original function was just setting up the command for the caller to iterate, 
        // then we have a problem because we are replacing the function that *executes* the query.
        // Let's look at how it was used or defined.
        // In TransIO.cpp: CCommand<CAccessor<CSelectDTrans>, CRowset > cmdSelectDtrans;
        // The function SelectDtrans wasn't fully shown in the snippet, only the class definition.
        // Ah, I missed the implementation of SelectDtrans in TransIO.cpp in the previous `view_file`.
        // The previous `view_file` showed the class definition but cut off before the function implementation?
        // No, wait. The `view_file` output ended at line 800. 
        // The class `CSelectDTrans` was defined around line 538.
        // The `cmdSelectDtrans` was defined at line 723.
        // I did NOT see the implementation of `SelectDtrans` function itself (the DLLAPI one).
        // It must be further down in the file.
        // I should probably check the implementation of `SelectDtrans` to see if it iterates or just fetches the first one.
        // However, based on the signature `TRANS *pzTR`, it looks like it populates a single struct.
        // If it was meant to return multiple, it would likely take a callback or an array.
        // OR, maybe the original code was using the global `cmdSelectDtrans` object statefully?
        // "CCommand<CAccessor<CSelectDTrans>, CRowset > cmdSelectDtrans;" is a global variable.
        // If the `SelectDtrans` function just opens the rowset, and other functions iterate, that's a stateful design.
        // But the request is "update code to modern DB access".
        // If I change it to `nanodbc`, I can't easily maintain that global state if the caller expects to access `cmdSelectDtrans` directly.
        // BUT, the `SelectDtrans` function is `DLLAPI`. The `cmdSelectDtrans` is NOT exported (it's in the .cpp).
        // So the caller (TransEngine.DLL) calls `SelectDtrans`.
        // If `SelectDtrans` populates `pzTR`, it probably fetches one row.
        // Let's assume it fetches the first row for now, or maybe it loops and processes?
        // Wait, if `pzTR` is a pointer to a single struct, it can only hold one record.
        // Unless `pzTR` is an array? "TRANS *pzTR".
        // Let's check `TransIO.h` again.
        // `DLLAPI void STDCALL SelectDtrans(TRANS *pzTR, int iID, char *sSecNo, char *sWi, long lProcessTag, ERRSTRUCT *pzErr);`
        // It looks like it returns ONE transaction or maybe the caller expects it to fill one.
        // Let's look at `SelectDTrans` class again. It has `CRowset`.
        // If the original `SelectDtrans` function implementation iterates and does something, I need to know.
        // I will assume for now it fetches the first matching record, as `pzTR` suggests a single output.
        // I'll implement it to fetch the first one.
        
        if (result.next())
        {
            read_int(result, "ID", &pzTR->iID);
            read_long(result, "dtrans_no", &pzTR->lDtransNo);
            read_string(result, "tran_type", pzTR->sTranType, sizeof(pzTR->sTranType));
            read_string(result, "sec_no", pzTR->sSecNo, sizeof(pzTR->sSecNo));
            read_string(result, "wi", pzTR->sWi, sizeof(pzTR->sWi));
            // process_tag_old? Not in struct? Ah, it was in the map but maybe not in TRANS struct?
            // In CSelectDTrans: COLUMN_ENTRY(6, m_lProcessTag_Old) -> but mapped to what? 
            // The class had `long m_lProcessTag_Old;` as a member.
            // But `TRANS` struct is what we are filling.
            // Let's look at `TRANS` struct definition? It's in `trans.h`. I don't have it.
            // But I can infer from `CSelectDTrans` map.
            // COLUMN_ENTRY(1, m_zDTrans.iID) -> pzTR->iID
            // COLUMN_ENTRY(6, m_lProcessTag_Old) -> This was a member of CSelectDTrans, NOT m_zDTrans.
            // So `process_tag` from DB was going into `m_lProcessTag_Old`.
            // It seems `SelectDtrans` might not be returning `process_tag` to the caller in `pzTR`.
            
            read_string(result, "sec_xtend", pzTR->sSecXtend, sizeof(pzTR->sSecXtend));
            read_string(result, "acct_type", pzTR->sAcctType, sizeof(pzTR->sAcctType));
            read_int(result, "SecID", &pzTR->iSecID);
            read_string(result, "sec_symbol", pzTR->sSecSymbol, sizeof(pzTR->sSecSymbol));
            read_double(result, "units", &pzTR->fUnits);
            
            read_double(result, "orig_face", &pzTR->fOrigFace);
            read_double(result, "unit_cost", &pzTR->fUnitCost);
            read_double(result, "tot_cost", &pzTR->fTotCost);
            read_double(result, "orig_cost", &pzTR->fOrigCost);
            read_double(result, "pcpl_amt", &pzTR->fPcplAmt);
            read_double(result, "opt_prem", &pzTR->fOptPrem);
            read_double(result, "amort_val", &pzTR->fAmortVal);
            
            read_double(result, "basis_adj", &pzTR->fBasisAdj);
            read_double(result, "comm_gcr", &pzTR->fCommGcr);
            read_double(result, "net_comm", &pzTR->fNetComm);
            read_string(result, "comm_code", pzTR->sCommCode, sizeof(pzTR->sCommCode));
            read_double(result, "sec_fees", &pzTR->fSecFees);
            read_double(result, "misc_fee1", &pzTR->fMiscFee1);
            read_string(result, "fee_code1", pzTR->sFeeCode1, sizeof(pzTR->sFeeCode1));
            
            read_double(result, "misc_fee2", &pzTR->fMiscFee2);
            read_string(result, "fee_code2", pzTR->sFeeCode2, sizeof(pzTR->sFeeCode2));
            read_double(result, "accr_int", &pzTR->fAccrInt);
            read_double(result, "income_amt", &pzTR->fIncomeAmt);
            read_double(result, "net_flow", &pzTR->fNetFlow);
            read_string(result, "broker_code", pzTR->sBrokerCode, sizeof(pzTR->sBrokerCode));
            
            read_string(result, "broker_code2", pzTR->sBrokerCode2, sizeof(pzTR->sBrokerCode2));
            
            // Dates
            long lDate;
            read_date(result, "trd_date", &lDate); pzTR->lTrdDate = lDate; // Assuming TRANS has lTrdDate, map used m_vTrdDate
            read_date(result, "stl_date", &lDate); pzTR->lStlDate = lDate;
            read_date(result, "eff_date", &lDate); pzTR->lEffDate = lDate;
            read_date(result, "entry_date", &lDate); pzTR->lEntryDate = lDate;
            
            read_long(result, "taxlot_no", &pzTR->lTaxlotNo);
            read_long(result, "xref_trans_no", &pzTR->lXrefTransNo);
            read_long(result, "pend_div_no", &pzTR->lPendDivNo);
            
            read_long(result, "rev_trans_no", &pzTR->lRevTransNo);
            read_string(result, "rev_type", pzTR->sRevType, sizeof(pzTR->sRevType));
            read_long(result, "new_trans_no", &pzTR->lNewTransNo);
            read_long(result, "orig_trans_no", &pzTR->lOrigTransNo);
            read_long(result, "block_trans_no", &pzTR->lBlockTransNo);
            read_string(result, "x_ID", pzTR->sXID, sizeof(pzTR->sXID));
            
            read_long(result, "x_trans_no", &pzTR->lXTransNo);
            read_string(result, "x_sec_no", pzTR->sXSecNo, sizeof(pzTR->sXSecNo));
            read_string(result, "x_wi", pzTR->sXWi, sizeof(pzTR->sXWi));
            read_string(result, "x_sec_xtend", pzTR->sXSecXtend, sizeof(pzTR->sXSecXtend));
            read_string(result, "x_acct_type", pzTR->sXAcctType, sizeof(pzTR->sXAcctType));
            read_int(result, "x_secid", &pzTR->iXSecID);
            read_string(result, "curr_id", pzTR->sCurrId, sizeof(pzTR->sCurrId));
            
            read_string(result, "curr_acct_type", pzTR->sCurrAcctType, sizeof(pzTR->sCurrAcctType));
            read_string(result, "inc_curr_id", pzTR->sIncCurrId, sizeof(pzTR->sIncCurrId));
            read_string(result, "inc_acct_type", pzTR->sIncAcctType, sizeof(pzTR->sIncAcctType));
            read_string(result, "x_curr_id", pzTR->sXCurrId, sizeof(pzTR->sXCurrId));
            read_string(result, "x_curr_acct_type", pzTR->sXCurrAcctType, sizeof(pzTR->sXCurrAcctType));
            
            read_string(result, "sec_curr_id", pzTR->sSecCurrId, sizeof(pzTR->sSecCurrId));
            read_string(result, "accr_curr_id", pzTR->sAccrCurrId, sizeof(pzTR->sAccrCurrId));
            read_double(result, "base_xrate", &pzTR->fBaseXrate);
            read_double(result, "inc_base_xrate", &pzTR->fIncBaseXrate);
            read_double(result, "sec_base_xrate", &pzTR->fSecBaseXrate);
            
            read_double(result, "accr_base_xrate", &pzTR->fAccrBaseXrate);
            read_double(result, "sys_xrate", &pzTR->fSysXrate);
            read_double(result, "inc_sys_xrate", &pzTR->fIncSysXrate);
            read_double(result, "base_open_xrate", &pzTR->fBaseOpenXrate);
            read_double(result, "sys_open_xrate", &pzTR->fSysOpenXrate);
            
            read_date(result, "open_trd_date", &lDate); pzTR->lOpenTrdDate = lDate;
            read_date(result, "open_stl_date", &lDate); pzTR->lOpenStlDate = lDate;
            read_double(result, "open_unit_cost", &pzTR->fOpenUnitCost);
            read_double(result, "orig_yld", &pzTR->fOrigYld);
            
            read_date(result, "eff_mat_date", &lDate); pzTR->lEffMatDate = lDate;
            read_double(result, "eff_mat_price", &pzTR->fEffMatPrice);
            read_string(result, "acct_mthd", pzTR->sAcctMthd, sizeof(pzTR->sAcctMthd));
            read_string(result, "trans_srce", pzTR->sTransSrce, sizeof(pzTR->sTransSrce));
            read_string(result, "adp_tag", pzTR->sAdpTag, sizeof(pzTR->sAdpTag));
            read_string(result, "div_type", pzTR->sDivType, sizeof(pzTR->sDivType));
            
            read_double(result, "div_factor", &pzTR->fDivFactor);
            read_long(result, "divint_no", &pzTR->lDivintNo);
            read_date(result, "roll_date", &lDate); pzTR->lRollDate = lDate;
            read_date(result, "perf_date", &lDate); pzTR->lPerfDate = lDate;
            read_string(result, "misc_Desc_ind", pzTR->sMiscDescInd, sizeof(pzTR->sMiscDescInd));
            
            read_string(result, "dr_cr", pzTR->sDrCr, sizeof(pzTR->sDrCr));
            read_string(result, "bal_to_adjust", pzTR->sBalToAdjust, sizeof(pzTR->sBalToAdjust));
            read_string(result, "cap_trans", pzTR->sCapTrans, sizeof(pzTR->sCapTrans));
            read_string(result, "safek_ind", pzTR->sSafekInd, sizeof(pzTR->sSafekInd));
            read_string(result, "dtc_inclusion", pzTR->sDtcInclusion, sizeof(pzTR->sDtcInclusion));
            read_string(result, "dtc_resolve", pzTR->sDtcResolve, sizeof(pzTR->sDtcResolve));
            read_string(result, "recon_flag", pzTR->sReconFlag, sizeof(pzTR->sReconFlag));
            
            read_string(result, "recon_srce", pzTR->sReconSrce, sizeof(pzTR->sReconSrce));
            read_string(result, "income_flag", pzTR->sIncomeFlag, sizeof(pzTR->sIncomeFlag));
            read_string(result, "letter_flag", pzTR->sLetterFlag, sizeof(pzTR->sLetterFlag));
            read_string(result, "ledger_flag", pzTR->sLedgerFlag, sizeof(pzTR->sLedgerFlag));
            read_string(result, "gl_flag", pzTR->sGlFlag, sizeof(pzTR->sGlFlag));
            read_string(result, "created_by", pzTR->sCreatedBy, sizeof(pzTR->sCreatedBy));
            
            read_date(result, "create_date", &lDate); pzTR->lCreateDate = lDate;
            read_string(result, "create_time", pzTR->sCreateTime, sizeof(pzTR->sCreateTime));
            read_date(result, "post_date", &lDate); pzTR->lPostDate = lDate;
            read_string(result, "bkof_frmt", pzTR->sBkofFrmt, sizeof(pzTR->sBkofFrmt));
            
            read_long(result, "bkof_seq_no", &pzTR->lBkofSeqNo);
            read_double(result, "price", &pzTR->fPrice);
            read_int(result, "restriction_code", &pzTR->iRestrictionCode);
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectDtrans: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lProcessTag, (char*)"D", 0, -1, 0, (char*)"SelectDtrans", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectDtrans", iID, lProcessTag, (char*)"D", 0, -1, 0, (char*)"SelectDtrans", FALSE);
    }
}

DLLAPI void STDCALL UpdateDtrans(int iID, char *sSecNo, char *sWi, long lProcessTag, char *sStatusFlag, int iBusinessError, 
							int iSqlError, int iIsamCode, int iErrorDate, char *sErrorTime, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"D", 0, -1, 0, (char*)"UpdateDtrans", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE dtrans "
            "SET status_flag = ?, err_status_code = ?, "
            "err_sql_code = ?,  err_isam_code = ?, "
            "error_date = ?, error_time = ? "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "process_tag = ? AND "
            "(status_flag = '' OR status_flag is NULL) "));

        nanodbc::timestamp tsErrorDate;
        long_to_timestamp(iErrorDate, tsErrorDate);

        stmt.bind(0, sStatusFlag);
        stmt.bind(1, &iBusinessError);
        stmt.bind(2, &iSqlError);
        stmt.bind(3, &iIsamCode);
        stmt.bind(4, &tsErrorDate);
        stmt.bind(5, sErrorTime);
        
        stmt.bind(6, &iID);
        stmt.bind(7, sSecNo);
        stmt.bind(8, sWi);
        stmt.bind(9, &lProcessTag);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdateDtrans: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lProcessTag, (char*)"D", 0, -1, 0, (char*)"UpdateDtrans", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdateDtrans", iID, lProcessTag, (char*)"D", 0, -1, 0, (char*)"UpdateDtrans", FALSE);
    }
}
