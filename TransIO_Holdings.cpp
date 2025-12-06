#include "TransIO_Holdings.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <iostream>
#include <vector>

extern thread_local nanodbc::connection gConn;

// Helper to fill HOLDINGS struct from result
void FillHoldingsStruct(nanodbc::result& result, HOLDINGS* pzHR)
{
    pzHR->iID = result.get<int>("ID", 0);
    read_string(result, "sec_no", pzHR->sSecNo, sizeof(pzHR->sSecNo));
    read_string(result, "wi", pzHR->sWi, sizeof(pzHR->sWi));
    read_string(result, "sec_xtend", pzHR->sSecXtend, sizeof(pzHR->sSecXtend));
    read_string(result, "acct_type", pzHR->sAcctType, sizeof(pzHR->sAcctType));
    pzHR->lTransNo = result.get<long>("trans_no", 0);
    pzHR->iSecID = result.get<int>("secid", 0);
    pzHR->lAsofDate = timestamp_to_long(result.get<nanodbc::timestamp>("asof_date"));
    read_string(result, "sec_symbol", pzHR->sSecSymbol, sizeof(pzHR->sSecSymbol));
    pzHR->fUnits = result.get<double>("units", 0.0);
    pzHR->fOrigFace = result.get<double>("orig_face", 0.0);
    pzHR->fTotCost = result.get<double>("tot_cost", 0.0);
    pzHR->fUnitCost = result.get<double>("unit_cost", 0.0);
    pzHR->fOrigCost = result.get<double>("orig_cost", 0.0);
    pzHR->fOpenLiability = result.get<double>("open_liability", 0.0);
    pzHR->fBaseCostXrate = result.get<double>("base_cost_xrate", 0.0);
    pzHR->fSysCostXrate = result.get<double>("sys_cost_xrate", 0.0);
    pzHR->lTrdDate = timestamp_to_long(result.get<nanodbc::timestamp>("trd_date"));
    pzHR->lEffDate = timestamp_to_long(result.get<nanodbc::timestamp>("eff_date"));
    pzHR->lEligDate = timestamp_to_long(result.get<nanodbc::timestamp>("elig_date"));
    pzHR->lStlDate = timestamp_to_long(result.get<nanodbc::timestamp>("stl_date"));
    pzHR->fOrigYield = result.get<double>("orig_yield", 0.0);
    pzHR->lEffMatDate = timestamp_to_long(result.get<nanodbc::timestamp>("eff_mat_date"));
    pzHR->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);
    pzHR->fCostEffMatYld = result.get<double>("cost_eff_mat_yld", 0.0);
    pzHR->lAmortStartDate = timestamp_to_long(result.get<nanodbc::timestamp>("amort_start_date"));
    read_string(result, "orig_trans_type", pzHR->sOrigTransType, sizeof(pzHR->sOrigTransType));
    read_string(result, "orig_trans_srce", pzHR->sOrigTransSrce, sizeof(pzHR->sOrigTransSrce));
    read_string(result, "last_trans_type", pzHR->sLastTransType, sizeof(pzHR->sLastTransType));
    pzHR->lLastTransNo = result.get<long>("last_trans_no", 0);
    read_string(result, "last_trans_srce", pzHR->sLastTransSrce, sizeof(pzHR->sLastTransSrce));
    pzHR->lLastPmtDate = timestamp_to_long(result.get<nanodbc::timestamp>("last_pmt_date"));
    read_string(result, "last_pmt_type", pzHR->sLastPmtType, sizeof(pzHR->sLastPmtType));
    pzHR->lLastPmtTrNo = result.get<long>("last_pmt_tr_no", 0);
    pzHR->lNextPmtDate = timestamp_to_long(result.get<nanodbc::timestamp>("next_pmt_date"));
    pzHR->fNextPmtAmt = result.get<double>("next_pmt_amt", 0.0);
    pzHR->lLastPdnDate = timestamp_to_long(result.get<nanodbc::timestamp>("last_pdn_date"));
    read_string(result, "lt_st_ind", pzHR->sLtStInd, sizeof(pzHR->sLtStInd));
    pzHR->fMktVal = result.get<double>("mkt_val", 0.0);
    pzHR->fCurLiability = result.get<double>("cur_liability", 0.0);
    pzHR->fMvBaseXrate = result.get<double>("mv_base_xrate", 0.0);
    pzHR->fMvSysXrate = result.get<double>("mv_sys_xrate", 0.0);
    pzHR->fAccrInt = result.get<double>("accr_int", 0.0);
    pzHR->fAiBaseXrate = result.get<double>("ai_base_xrate", 0.0);
    pzHR->fAiSysXrate = result.get<double>("ai_sys_xrate", 0.0);
    pzHR->fAnnualIncome = result.get<double>("annual_income", 0.0);
    pzHR->fAccrualGl = result.get<double>("accrual_gl", 0.0);
    pzHR->fCurrencyGl = result.get<double>("currency_gl", 0.0);
    pzHR->fSecurityGl = result.get<double>("security_gl", 0.0);
    pzHR->fMktEffMatYld = result.get<double>("mkt_eff_mat_yld", 0.0);
    pzHR->fMktCurYld = result.get<double>("mkt_cur_yld", 0.0);
    read_string(result, "safek_ind", pzHR->sSafekInd, sizeof(pzHR->sSafekInd));
    pzHR->fCollateralUnits = result.get<double>("collateral_units", 0.0);
    pzHR->fHedgeValue = result.get<double>("hedge_value", 0.0);
    read_string(result, "benchmark_sec_no", pzHR->sBenchmarkSecNo, sizeof(pzHR->sBenchmarkSecNo));
    read_string(result, "perm_lt_flag", pzHR->sPermLtFlag, sizeof(pzHR->sPermLtFlag));
    read_string(result, "valuation_srce", pzHR->sValuationSrce, sizeof(pzHR->sValuationSrce));
    read_string(result, "primary_type", pzHR->sPrimaryType, sizeof(pzHR->sPrimaryType));
    pzHR->iRestrictionCode = result.get<int>("restriction_code", 0);
}

DLLAPI void STDCALL DeleteHoldings(int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType, 
							  long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"DeleteHoldings", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "Delete from holdings "
            "Where ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=? and trans_no=?"));

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteHoldings: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"T", 0, -1, 0, (char*)"DeleteHoldings", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteHoldings", iID, lTransNo, (char*)"T", 0, -1, 0, (char*)"DeleteHoldings", FALSE);
    }
}

DLLAPI void STDCALL InsertHoldings(HOLDINGS zHR, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"InsertHoldings", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "insert into holdings "
            "(ID, sec_no, wi, sec_xtend, acct_type, trans_no, "
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
            "values(?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?)"));

        nanodbc::timestamp tsAsofDate, tsTrdDate, tsEffDate, tsEligDate, tsStlDate;
        nanodbc::timestamp tsEffMatDate, tsAmortStartDate, tsLastPmtDate, tsNextPmtDate, tsLastPdnDate;

        long_to_timestamp(zHR.lAsofDate, tsAsofDate);
        long_to_timestamp(zHR.lTrdDate, tsTrdDate);
        long_to_timestamp(zHR.lEffDate, tsEffDate);
        long_to_timestamp(zHR.lEligDate, tsEligDate);
        long_to_timestamp(zHR.lStlDate, tsStlDate);
        long_to_timestamp(zHR.lEffMatDate, tsEffMatDate);
        long_to_timestamp(zHR.lAmortStartDate, tsAmortStartDate);
        long_to_timestamp(zHR.lLastPmtDate, tsLastPmtDate);
        long_to_timestamp(zHR.lNextPmtDate, tsNextPmtDate);
        long_to_timestamp(zHR.lLastPdnDate, tsLastPdnDate);

        // Rounding
        zHR.fUnits = RoundDouble(zHR.fUnits, 5);
        zHR.fOrigFace = RoundDouble(zHR.fOrigFace, 3);
        zHR.fTotCost = RoundDouble(zHR.fTotCost, 3);
        zHR.fUnitCost = RoundDouble(zHR.fUnitCost, 6);
        zHR.fOrigCost = RoundDouble(zHR.fOrigCost, 3);
        zHR.fOpenLiability = RoundDouble(zHR.fOpenLiability, 2);
        zHR.fBaseCostXrate = RoundDouble(zHR.fBaseCostXrate, 12);
        zHR.fSysCostXrate = RoundDouble(zHR.fSysCostXrate, 12);
        zHR.fOrigYield = RoundDouble(zHR.fOrigYield, 6);
        zHR.fEffMatPrice = RoundDouble(zHR.fEffMatPrice, 6);
        zHR.fCostEffMatYld = RoundDouble(zHR.fCostEffMatYld, 6);
        zHR.fNextPmtAmt = RoundDouble(zHR.fNextPmtAmt, 2);
        zHR.fMktVal = RoundDouble(zHR.fMktVal, 2);
        zHR.fCurLiability = RoundDouble(zHR.fCurLiability, 2);
        zHR.fMvBaseXrate = RoundDouble(zHR.fMvBaseXrate, 12);
        zHR.fMvSysXrate = RoundDouble(zHR.fMvSysXrate, 12);
        zHR.fAccrInt = RoundDouble(zHR.fAccrInt, 2);
        zHR.fAiBaseXrate = RoundDouble(zHR.fAiBaseXrate, 12);
        zHR.fAiSysXrate = RoundDouble(zHR.fAiSysXrate, 12);
        zHR.fAnnualIncome = RoundDouble(zHR.fAnnualIncome, 2);
        zHR.fAccrualGl = RoundDouble(zHR.fAccrualGl, 2);
        zHR.fCurrencyGl = RoundDouble(zHR.fCurrencyGl, 2);
        zHR.fSecurityGl = RoundDouble(zHR.fSecurityGl, 2);
        zHR.fMktEffMatYld = RoundDouble(zHR.fMktEffMatYld, 6);
        zHR.fMktCurYld = RoundDouble(zHR.fMktCurYld, 6);
        zHR.fCollateralUnits = RoundDouble(zHR.fCollateralUnits, 5);
        zHR.fHedgeValue = RoundDouble(zHR.fHedgeValue, 2);

        int idx = 0;
        stmt.bind(idx++, &zHR.iID);
        stmt.bind(idx++, zHR.sSecNo);
        stmt.bind(idx++, zHR.sWi);
        stmt.bind(idx++, zHR.sSecXtend);
        stmt.bind(idx++, zHR.sAcctType);
        stmt.bind(idx++, &zHR.lTransNo);
        stmt.bind(idx++, &zHR.iSecID);
        stmt.bind(idx++, &tsAsofDate);
        stmt.bind(idx++, zHR.sSecSymbol);
        stmt.bind(idx++, &zHR.fUnits);
        stmt.bind(idx++, &zHR.fOrigFace);
        stmt.bind(idx++, &zHR.fTotCost);
        stmt.bind(idx++, &zHR.fUnitCost);
        stmt.bind(idx++, &zHR.fOrigCost);
        stmt.bind(idx++, &zHR.fOpenLiability);
        stmt.bind(idx++, &zHR.fBaseCostXrate);
        stmt.bind(idx++, &zHR.fSysCostXrate);
        stmt.bind(idx++, &tsTrdDate);
        stmt.bind(idx++, &tsEffDate);
        stmt.bind(idx++, &tsEligDate);
        stmt.bind(idx++, &tsStlDate);
        stmt.bind(idx++, &zHR.fOrigYield);
        stmt.bind(idx++, &tsEffMatDate);
        stmt.bind(idx++, &zHR.fEffMatPrice);
        stmt.bind(idx++, &zHR.fCostEffMatYld);
        stmt.bind(idx++, &tsAmortStartDate);
        stmt.bind(idx++, zHR.sOrigTransType);
        stmt.bind(idx++, zHR.sOrigTransSrce);
        stmt.bind(idx++, zHR.sLastTransType);
        stmt.bind(idx++, &zHR.lLastTransNo);
        stmt.bind(idx++, zHR.sLastTransSrce);
        stmt.bind(idx++, &tsLastPmtDate);
        stmt.bind(idx++, zHR.sLastPmtType);
        stmt.bind(idx++, &zHR.lLastPmtTrNo);
        stmt.bind(idx++, &tsNextPmtDate);
        stmt.bind(idx++, &zHR.fNextPmtAmt);
        stmt.bind(idx++, &tsLastPdnDate);
        stmt.bind(idx++, zHR.sLtStInd);
        stmt.bind(idx++, &zHR.fMktVal);
        stmt.bind(idx++, &zHR.fCurLiability);
        stmt.bind(idx++, &zHR.fMvBaseXrate);
        stmt.bind(idx++, &zHR.fMvSysXrate);
        stmt.bind(idx++, &zHR.fAccrInt);
        stmt.bind(idx++, &zHR.fAiBaseXrate);
        stmt.bind(idx++, &zHR.fAiSysXrate);
        stmt.bind(idx++, &zHR.fAnnualIncome);
        stmt.bind(idx++, &zHR.fAccrualGl);
        stmt.bind(idx++, &zHR.fCurrencyGl);
        stmt.bind(idx++, &zHR.fSecurityGl);
        stmt.bind(idx++, &zHR.fMktEffMatYld);
        stmt.bind(idx++, &zHR.fMktCurYld);
        stmt.bind(idx++, zHR.sSafekInd);
        stmt.bind(idx++, &zHR.fCollateralUnits);
        stmt.bind(idx++, &zHR.fHedgeValue);
        stmt.bind(idx++, zHR.sBenchmarkSecNo);
        stmt.bind(idx++, zHR.sPermLtFlag);
        stmt.bind(idx++, zHR.sValuationSrce);
        stmt.bind(idx++, zHR.sPrimaryType);
        stmt.bind(idx++, &zHR.iRestrictionCode);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertHoldings: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), zHR.iID, zHR.lTransNo, (char*)"T", 0, -1, 0, (char*)"InsertHoldings", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertHoldings", zHR.iID, zHR.lTransNo, (char*)"T", 0, -1, 0, (char*)"InsertHoldings", FALSE);
    }
}

static std::vector<HOLDINGS> g_HoldingsBatch;

DLLAPI void STDCALL InsertHoldingsBatch(HOLDINGS* pzHR, long lBatchSize, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"InsertHoldingsBatch", FALSE);
        return;
    }

    try
    {
        if (lBatchSize < 0)
        {
            g_HoldingsBatch.clear();
            return;
        }

        if (pzHR)
        {
            g_HoldingsBatch.push_back(*pzHR);
        }

        if (g_HoldingsBatch.size() >= (size_t)lBatchSize && g_HoldingsBatch.size() > 0)
        {
            nanodbc::transaction trans(gConn);
            
            for (const auto& zHR : g_HoldingsBatch)
            {
                InsertHoldings(zHR, pzErr);
                if (pzErr->iSqlError != 0)
                {
                    // If any insert fails, we should probably stop and rollback?
                    // The original code says "any failure will cause entire trans to rollback"
                    // But InsertHoldings catches exceptions.
                    // If InsertHoldings returns error, we should probably throw to trigger rollback?
                    // But InsertHoldings already handles error reporting.
                    // Let's just return.
                    return;
                }
            }
            
            trans.commit();
            g_HoldingsBatch.clear();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertHoldingsBatch: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"T", 0, -1, 0, (char*)"InsertHoldingsBatch", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertHoldingsBatch", 0, 0, (char*)"T", 0, -1, 0, (char*)"InsertHoldingsBatch", FALSE);
    }
}

DLLAPI void STDCALL SelectHoldings(HOLDINGS *pzHR, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
                              long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectHoldings", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "select ID, sec_no, wi, sec_xtend, acct_type, trans_no, "
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
            "from holdings "
            "where ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=? and trans_no=? "));

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &lTransNo);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHR);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectHoldings: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, lTransNo, (char*)"T", 0, -1, 0, (char*)"SelectHoldings", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectHoldings", iID, lTransNo, (char*)"T", 0, -1, 0, (char*)"SelectHoldings", FALSE);
    }
}

DLLAPI void STDCALL UpdateHoldings(HOLDINGS zHR, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"UpdateHoldings", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "update holdings set "
            "secid=?,  asof_date=?,  "
            "sec_symbol=?,  units=?,  orig_face=?,  tot_cost=?,   "
            "unit_cost=?,  orig_cost=?,  open_liability=?,  base_cost_xrate=?,   "
            "sys_cost_xrate=?,  trd_date=?,  eff_date=?,  elig_date=?,  "
            "stl_date=?,  orig_yield=?,  eff_mat_date=?,  eff_mat_price=?,  "
            "cost_eff_mat_yld=?,  amort_start_date=?,  orig_trans_type=?,  "
            "orig_trans_srce=?,  last_trans_type=?,  last_trans_no=?,  "
            "last_trans_srce=?,  last_pmt_date=?,  last_pmt_type=?,  "
            "last_pmt_tr_no=?,  next_pmt_date=?,  next_pmt_amt=?,  "
            "last_pdn_date=?,  lt_st_ind=?,  mkt_val=?,  cur_liability=?,  "
            "mv_base_xrate=?,  mv_sys_xrate=?,  accr_int=?,  "
            "ai_base_xrate=?,  ai_sys_xrate=?,  annual_income=?,   "
            "accrual_gl=?,  currency_gl=?,  security_gl=?,  mkt_eff_mat_yld=?,  "
            "mkt_cur_yld=?,  safek_ind=?,  collateral_units=?,  hedge_value=?,  "
            "benchmark_sec_no=?,  perm_lt_flag=?,  valuation_srce=?,  primary_type=?,  "
            "restriction_code=? "
            "where ID=? and sec_no=? and wi=? and sec_xtend=? and "
            "acct_type=? and trans_no=? "));

        nanodbc::timestamp tsAsofDate, tsTrdDate, tsEffDate, tsEligDate, tsStlDate;
        nanodbc::timestamp tsEffMatDate, tsAmortStartDate, tsLastPmtDate, tsNextPmtDate, tsLastPdnDate;

        long_to_timestamp(zHR.lAsofDate, tsAsofDate);
        long_to_timestamp(zHR.lTrdDate, tsTrdDate);
        long_to_timestamp(zHR.lEffDate, tsEffDate);
        long_to_timestamp(zHR.lEligDate, tsEligDate);
        long_to_timestamp(zHR.lStlDate, tsStlDate);
        long_to_timestamp(zHR.lEffMatDate, tsEffMatDate);
        long_to_timestamp(zHR.lAmortStartDate, tsAmortStartDate);
        long_to_timestamp(zHR.lLastPmtDate, tsLastPmtDate);
        long_to_timestamp(zHR.lNextPmtDate, tsNextPmtDate);
        long_to_timestamp(zHR.lLastPdnDate, tsLastPdnDate);

        // Rounding
        zHR.fUnits = RoundDouble(zHR.fUnits, 5);
        zHR.fOrigFace = RoundDouble(zHR.fOrigFace, 3);
        zHR.fTotCost = RoundDouble(zHR.fTotCost, 3);
        zHR.fUnitCost = RoundDouble(zHR.fUnitCost, 6);
        zHR.fOrigCost = RoundDouble(zHR.fOrigCost, 3);
        zHR.fOpenLiability = RoundDouble(zHR.fOpenLiability, 2);
        zHR.fBaseCostXrate = RoundDouble(zHR.fBaseCostXrate, 12);
        zHR.fSysCostXrate = RoundDouble(zHR.fSysCostXrate, 12);
        zHR.fOrigYield = RoundDouble(zHR.fOrigYield, 6);
        zHR.fEffMatPrice = RoundDouble(zHR.fEffMatPrice, 6);
        zHR.fCostEffMatYld = RoundDouble(zHR.fCostEffMatYld, 6);
        zHR.fNextPmtAmt = RoundDouble(zHR.fNextPmtAmt, 2);
        zHR.fMktVal = RoundDouble(zHR.fMktVal, 2);
        zHR.fCurLiability = RoundDouble(zHR.fCurLiability, 2);
        zHR.fMvBaseXrate = RoundDouble(zHR.fMvBaseXrate, 12);
        zHR.fMvSysXrate = RoundDouble(zHR.fMvSysXrate, 12);
        zHR.fAccrInt = RoundDouble(zHR.fAccrInt, 2);
        zHR.fAiBaseXrate = RoundDouble(zHR.fAiBaseXrate, 12);
        zHR.fAiSysXrate = RoundDouble(zHR.fAiSysXrate, 12);
        zHR.fAnnualIncome = RoundDouble(zHR.fAnnualIncome, 2);
        zHR.fAccrualGl = RoundDouble(zHR.fAccrualGl, 2);
        zHR.fCurrencyGl = RoundDouble(zHR.fCurrencyGl, 2);
        zHR.fSecurityGl = RoundDouble(zHR.fSecurityGl, 2);
        zHR.fMktEffMatYld = RoundDouble(zHR.fMktEffMatYld, 6);
        zHR.fMktCurYld = RoundDouble(zHR.fMktCurYld, 6);
        zHR.fCollateralUnits = RoundDouble(zHR.fCollateralUnits, 5);
        zHR.fHedgeValue = RoundDouble(zHR.fHedgeValue, 2);

        int idx = 0;
        stmt.bind(idx++, &zHR.iSecID);
        stmt.bind(idx++, &tsAsofDate);
        stmt.bind(idx++, zHR.sSecSymbol);
        stmt.bind(idx++, &zHR.fUnits);
        stmt.bind(idx++, &zHR.fOrigFace);
        stmt.bind(idx++, &zHR.fTotCost);
        stmt.bind(idx++, &zHR.fUnitCost);
        stmt.bind(idx++, &zHR.fOrigCost);
        stmt.bind(idx++, &zHR.fOpenLiability);
        stmt.bind(idx++, &zHR.fBaseCostXrate);
        stmt.bind(idx++, &zHR.fSysCostXrate);
        stmt.bind(idx++, &tsTrdDate);
        stmt.bind(idx++, &tsEffDate);
        stmt.bind(idx++, &tsEligDate);
        stmt.bind(idx++, &tsStlDate);
        stmt.bind(idx++, &zHR.fOrigYield);
        stmt.bind(idx++, &tsEffMatDate);
        stmt.bind(idx++, &zHR.fEffMatPrice);
        stmt.bind(idx++, &zHR.fCostEffMatYld);
        stmt.bind(idx++, &tsAmortStartDate);
        stmt.bind(idx++, zHR.sOrigTransType);
        stmt.bind(idx++, zHR.sOrigTransSrce);
        stmt.bind(idx++, zHR.sLastTransType);
        stmt.bind(idx++, &zHR.lLastTransNo);
        stmt.bind(idx++, zHR.sLastTransSrce);
        stmt.bind(idx++, &tsLastPmtDate);
        stmt.bind(idx++, zHR.sLastPmtType);
        stmt.bind(idx++, &zHR.lLastPmtTrNo);
        stmt.bind(idx++, &tsNextPmtDate);
        stmt.bind(idx++, &zHR.fNextPmtAmt);
        stmt.bind(idx++, &tsLastPdnDate);
        stmt.bind(idx++, zHR.sLtStInd);
        stmt.bind(idx++, &zHR.fMktVal);
        stmt.bind(idx++, &zHR.fCurLiability);
        stmt.bind(idx++, &zHR.fMvBaseXrate);
        stmt.bind(idx++, &zHR.fMvSysXrate);
        stmt.bind(idx++, &zHR.fAccrInt);
        stmt.bind(idx++, &zHR.fAiBaseXrate);
        stmt.bind(idx++, &zHR.fAiSysXrate);
        stmt.bind(idx++, &zHR.fAnnualIncome);
        stmt.bind(idx++, &zHR.fAccrualGl);
        stmt.bind(idx++, &zHR.fCurrencyGl);
        stmt.bind(idx++, &zHR.fSecurityGl);
        stmt.bind(idx++, &zHR.fMktEffMatYld);
        stmt.bind(idx++, &zHR.fMktCurYld);
        stmt.bind(idx++, zHR.sSafekInd);
        stmt.bind(idx++, &zHR.fCollateralUnits);
        stmt.bind(idx++, &zHR.fHedgeValue);
        stmt.bind(idx++, zHR.sBenchmarkSecNo);
        stmt.bind(idx++, zHR.sPermLtFlag);
        stmt.bind(idx++, zHR.sValuationSrce);
        stmt.bind(idx++, zHR.sPrimaryType);
        stmt.bind(idx++, &zHR.iRestrictionCode);

        stmt.bind(idx++, &zHR.iID);
        stmt.bind(idx++, zHR.sSecNo);
        stmt.bind(idx++, zHR.sWi);
        stmt.bind(idx++, zHR.sSecXtend);
        stmt.bind(idx++, zHR.sAcctType);
        stmt.bind(idx++, &zHR.lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdateHoldings: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), zHR.iID, zHR.lTransNo, (char*)"T", 0, -1, 0, (char*)"UpdateHoldings", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdateHoldings", zHR.iID, zHR.lTransNo, (char*)"T", 0, -1, 0, (char*)"UpdateHoldings", FALSE);
    }
}

DLLAPI void STDCALL HoldingsForFifoAndAvgAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                          char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsForFifoAndAvgAccts", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT id, sec_no, wi, sec_xtend, "
            "trans_no, acct_type, units, orig_face, tot_cost, unit_cost, "
            "orig_cost, base_cost_xrate, sys_cost_xrate, trd_date, eff_date, stl_date, "
            "orig_yield, eff_mat_date, eff_mat_price, cost_eff_mat_yld, safek_ind, "
            "collateral_units, perm_lt_flag,cur_liability,open_liability, restriction_code  "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type LIKE ? "
            "AND trd_date is NOT NULL AND trd_date != ? "
            "ORDER BY trd_date, trans_no "));

        nanodbc::timestamp tsTrdDate;
        long_to_timestamp(lTrdDate, tsTrdDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsTrdDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHoldings);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsForFifoAndAvgAccts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForFifoAndAvgAccts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsForFifoAndAvgAccts", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForFifoAndAvgAccts", FALSE);
    }
}

DLLAPI void STDCALL HoldingsForLifoAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                    char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsForLifoAccts", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT  id, sec_no, wi, sec_xtend, "
            "trans_no, acct_type, units, orig_face, tot_cost, unit_cost, "
            "orig_cost, base_cost_xrate, sys_cost_xrate, trd_date, eff_date, stl_date, "
            "orig_yield, eff_mat_date, eff_mat_price, cost_eff_mat_yld, safek_ind, "
            "collateral_units, perm_lt_flag,cur_liability,open_liability, restriction_code  "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type LIKE ? "
            "AND trd_date is NOT NULL AND trd_date != ? "
            "ORDER BY trd_date DESC, trans_no ASC  "));

        nanodbc::timestamp tsTrdDate;
        long_to_timestamp(lTrdDate, tsTrdDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsTrdDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHoldings);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsForLifoAccts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForLifoAccts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsForLifoAccts", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForLifoAccts", FALSE);
    }
}

DLLAPI void STDCALL HoldingsForHighAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                    char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsForHighAccts", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT  id, sec_no, wi, sec_xtend, "
            "trans_no, acct_type, units, orig_face, tot_cost, unit_cost, "
            "orig_cost, base_cost_xrate, sys_cost_xrate, trd_date, eff_date, stl_date, "
            "orig_yield, eff_mat_date, eff_mat_price, cost_eff_mat_yld, safek_ind, "
            "collateral_units, perm_lt_flag,cur_liability,open_liability, restriction_code  "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type LIKE ? "
            "AND trd_date is NOT NULL AND trd_date != ?  and units <> '0' "
            "ORDER BY tot_cost / units DESC, trd_date ASC "));

        nanodbc::timestamp tsTrdDate;
        long_to_timestamp(lTrdDate, tsTrdDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsTrdDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHoldings);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsForHighAccts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForHighAccts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsForHighAccts", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForHighAccts", FALSE);
    }
}

DLLAPI void STDCALL HoldingsForLowAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend,
                                    char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsForLowAccts", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT  id, sec_no, wi, sec_xtend, "
            "trans_no, acct_type, units, orig_face, tot_cost, unit_cost, "
            "orig_cost, base_cost_xrate, sys_cost_xrate, trd_date, eff_date, stl_date, "
            "orig_yield, eff_mat_date, eff_mat_price, cost_eff_mat_yld, safek_ind, "
            "collateral_units, perm_lt_flag,cur_liability,open_liability, restriction_code  "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type LIKE ? "
            "AND trd_date is NOT NULL AND trd_date != ?  and units <> '0' "
            "ORDER BY tot_cost / units, trd_date "));

        nanodbc::timestamp tsTrdDate;
        long_to_timestamp(lTrdDate, tsTrdDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsTrdDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHoldings);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsForLowAccts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForLowAccts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsForLowAccts", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForLowAccts", FALSE);
    }
}

DLLAPI void STDCALL HoldingsForMinimumGainAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
										   char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate,
										   ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsForMinimumGainAccts", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT  id, sec_no, wi, sec_xtend, "
            "trans_no, acct_type, units, orig_face, tot_cost, unit_cost, "
            "orig_cost, base_cost_xrate, sys_cost_xrate, trd_date, eff_date, stl_date, "
            "orig_yield, eff_mat_date, eff_mat_price, cost_eff_mat_yld, safek_ind, "
            "collateral_units, perm_lt_flag,cur_liability,open_liability, restriction_code  "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type LIKE ? "
            "AND trd_date is NOT NULL AND trd_date != ?  and units <> '0' "
            "AND trd_date >= ? AND trd_date <= ? "
            "ORDER BY tot_cost / units DESC, trd_date ASC "));

        nanodbc::timestamp tsTrdDate, tsMinTrdDate, tsMaxTrdDate;
        long_to_timestamp(lTrdDate, tsTrdDate);
        long_to_timestamp(lMinTrdDate, tsMinTrdDate);
        long_to_timestamp(lMaxTrdDate, tsMaxTrdDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsTrdDate);
        stmt.bind(6, &tsMinTrdDate);
        stmt.bind(7, &tsMaxTrdDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHoldings);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsForMinimumGainAccts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForMinimumGainAccts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsForMinimumGainAccts", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForMinimumGainAccts", FALSE);
    }
}

DLLAPI void STDCALL HoldingsForMaximumGainAccts(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi, char *sSecXtend, 
										   char *sAcctType, long lTrdDate, long lMinTrdDate, long lMaxTrdDate,
										   ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsForMaximumGainAccts", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT  id, sec_no, wi, sec_xtend, "
            "trans_no, acct_type, units, orig_face, tot_cost, unit_cost, "
            "orig_cost, base_cost_xrate, sys_cost_xrate, trd_date, eff_date, stl_date, "
            "orig_yield, eff_mat_date, eff_mat_price, cost_eff_mat_yld, safek_ind, "
            "collateral_units, perm_lt_flag,cur_liability,open_liability, restriction_code  "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type LIKE ? "
            "AND trd_date is NOT NULL AND trd_date != ?  and units <> '0' "
            "AND trd_date >= ? AND trd_date <= ? "
            "ORDER BY tot_cost / units, trd_date "));

        nanodbc::timestamp tsTrdDate, tsMinTrdDate, tsMaxTrdDate;
        long_to_timestamp(lTrdDate, tsTrdDate);
        long_to_timestamp(lMinTrdDate, tsMinTrdDate);
        long_to_timestamp(lMaxTrdDate, tsMaxTrdDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsTrdDate);
        stmt.bind(6, &tsMinTrdDate);
        stmt.bind(7, &tsMaxTrdDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillHoldingsStruct(result, pzHoldings);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsForMaximumGainAccts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForMaximumGainAccts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsForMaximumGainAccts", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsForMaximumGainAccts", FALSE);
    }
}

DLLAPI void STDCALL HoldingsSumSelect(double *pfTotUnits, double *pfTotTotCost, double *pfTotOrigCost, double *pfTotBaseTCost, 
								 double *pfTotSysTCost, double *pfTotOpenLiability, double *pfTotBaseOLiability, double *pfTotSysOLiability,
								 int iID, char *sSecNo, char *sWi, char *sSecXtend, 
								 char *sAcctType, long lInvalidDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsSumSelect", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT sum(units) un, sum(tot_cost) tc, sum(orig_cost) oc, "
            "sum(tot_cost / base_cost_xrate) tcb, "
            "sum(tot_cost / sys_cost_xrate) tcs, "
            "sum(open_liability) ol, "
            "sum(open_liability / base_cost_xrate) tlb, "
            "sum(open_liability / sys_cost_xrate) tls "
            "FROM Holdings "
            "WHERE ID = ? AND sec_no = ? AND wi = ? AND "
            "sec_xtend = ? AND acct_type = ? AND "
            "trd_date IS NOT NULL AND trd_date != ?"));

        nanodbc::timestamp tsInvalidDate;
        long_to_timestamp(lInvalidDate, tsInvalidDate);

        stmt.bind(0, &iID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWi);
        stmt.bind(3, sSecXtend);
        stmt.bind(4, sAcctType);
        stmt.bind(5, &tsInvalidDate);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            *pfTotUnits = result.get<double>(0, 0.0);
            *pfTotTotCost = result.get<double>(1, 0.0);
            *pfTotOrigCost = result.get<double>(2, 0.0);
            *pfTotBaseTCost = result.get<double>(3, 0.0);
            *pfTotSysTCost = result.get<double>(4, 0.0);
            *pfTotOpenLiability = result.get<double>(5, 0.0);
            *pfTotBaseOLiability = result.get<double>(6, 0.0);
            *pfTotSysOLiability = result.get<double>(7, 0.0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsSumSelect: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsSumSelect", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsSumSelect", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsSumSelect", FALSE);
    }
}

DLLAPI void STDCALL HoldingsSumUpdate(double fTotUnitCost, double fTradingUnits, double fTotOrigUnit, double fUnitLiability,
								 double fNewBaseXrate, double fNewSysXrate, char *sTranType, long lTaxlotNo, 
								 char *sTransSrce, int iID, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
								 long lInvalidDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"HoldingsSumUpdate", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE Holdings "
            "SET tot_cost = (units * ?  * ?),  "
            "unit_cost = ?, "
            "orig_cost = (units * ? * ?), "
            "open_liability = (units * ?), "
            "base_cost_xrate = ?, sys_cost_xrate = ?, "
            "last_trans_type = ?, "
            "last_trans_no = ?, last_trans_srce = ?  "
            "WHERE ID = ? AND sec_no = ? AND  wi = ? AND "
            "sec_xtend = ? AND acct_type = ? AND "
            "trd_date IS NOT NULL AND trd_date != ? "));

        nanodbc::timestamp tsInvalidDate;
        long_to_timestamp(lInvalidDate, tsInvalidDate);

        // Rounding
        fTotUnitCost = RoundDouble(fTotUnitCost, 10);
        fTotOrigUnit = RoundDouble(fTotOrigUnit, 10);
        fUnitLiability = RoundDouble(fUnitLiability, 10);
        fNewBaseXrate = RoundDouble(fNewBaseXrate, 12);
        fNewSysXrate = RoundDouble(fNewSysXrate, 12);

        stmt.bind(0, &fTradingUnits);
        stmt.bind(1, &fTotUnitCost);
        stmt.bind(2, &fTotUnitCost);
        stmt.bind(3, &fTradingUnits);
        stmt.bind(4, &fTotOrigUnit);
        stmt.bind(5, &fUnitLiability);
        stmt.bind(6, &fNewBaseXrate);
        stmt.bind(7, &fNewSysXrate);
        stmt.bind(8, sTranType);
        stmt.bind(9, &lTaxlotNo);
        stmt.bind(10, sTransSrce);
        
        stmt.bind(11, &iID);
        stmt.bind(12, sSecNo);
        stmt.bind(13, sWi);
        stmt.bind(14, sSecXtend);
        stmt.bind(15, sAcctType);
        stmt.bind(16, &tsInvalidDate);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in HoldingsSumUpdate: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsSumUpdate", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in HoldingsSumUpdate", iID, 0, (char*)"", 0, -1, 0, (char*)"HoldingsSumUpdate", FALSE);
    }
}
