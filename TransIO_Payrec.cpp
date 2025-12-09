#include "TransIO_Payrec.h"
#include "ODBCErrorChecking.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;

DLLAPI void STDCALL DeletePayrec(int iID, char *sSecNo, char *sWi,
                                 char *sSecXtend, char *sAcctType,
                                 long lTransNo, long lDivintNo,
                                 ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"T", 0,
                        -1, 0, (char *)"DeletePayrec", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT("Delete from payrec "
                     "Where ID=? and sec_no=? and wi=? and sec_xtend=? and "
                     "acct_type=? and trans_no=? and divint_no=?"));

    int idx = 0;
    stmt.bind(idx++, &iID);
    safe_bind_string(stmt, idx, sSecNo);
    safe_bind_string(stmt, idx, sWi);
    safe_bind_string(stmt, idx, sSecXtend);
    safe_bind_string(stmt, idx, sAcctType);
    stmt.bind(idx++, &lTransNo);
    stmt.bind(idx++, &lDivintNo);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in DeletePayrec: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), iID, lTransNo, (char *)"T", 0, -1,
                        0, (char *)"DeletePayrec", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in DeletePayrec", iID, lTransNo,
                   (char *)"T", 0, -1, 0, (char *)"DeletePayrec", FALSE);
  }
}

DLLAPI void STDCALL InsertPayrec(PAYREC zPYR, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"T", 0,
                        -1, 0, (char *)"InsertPayrec", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT("insert into payrec "
                           "(ID, sec_no, wi, sec_xtend, acct_type, "
                           "trans_no, secid, asof_date, tran_type, "
                           "divint_no, units, base_cost_xrate, "
                           "sys_cost_xrate, cur_val, eff_date, "
                           "mv_base_xrate, mv_sys_xrate, valuation_srce) "
                           "values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                           "?, ?, ?, ?, ?, ?)"));

    nanodbc::timestamp tsAsofDate, tsEffDate;
    long_to_timestamp(zPYR.lAsofDate, tsAsofDate);
    long_to_timestamp(zPYR.lEffDate, tsEffDate);

    // Rounding
    zPYR.fUnits = RoundDouble(zPYR.fUnits, 5);
    zPYR.fBaseCostXrate = RoundDouble(zPYR.fBaseCostXrate, 12);
    zPYR.fSysCostXrate = RoundDouble(zPYR.fSysCostXrate, 12);
    zPYR.fCurVal = RoundDouble(zPYR.fCurVal, 2);
    zPYR.fMvBaseXrate = RoundDouble(zPYR.fMvBaseXrate, 12);
    zPYR.fMvSysXrate = RoundDouble(zPYR.fMvSysXrate, 12);

    int idx = 0;
    stmt.bind(idx++, &zPYR.iID);
    safe_bind_string(stmt, idx, zPYR.sSecNo);
    safe_bind_string(stmt, idx, zPYR.sWi);
    safe_bind_string(stmt, idx, zPYR.sSecXtend);
    safe_bind_string(stmt, idx, zPYR.sAcctType);
    stmt.bind(idx++, &zPYR.lTransNo);
    stmt.bind(idx++, &zPYR.iSecID);
    stmt.bind(idx++, &tsAsofDate);
    safe_bind_string(stmt, idx, zPYR.sTranType);
    stmt.bind(idx++, &zPYR.lDivintNo);
    stmt.bind(idx++, &zPYR.fUnits);
    stmt.bind(idx++, &zPYR.fBaseCostXrate);
    stmt.bind(idx++, &zPYR.fSysCostXrate);
    stmt.bind(idx++, &zPYR.fCurVal);
    stmt.bind(idx++, &tsEffDate);
    stmt.bind(idx++, &zPYR.fMvBaseXrate);
    stmt.bind(idx++, &zPYR.fMvSysXrate);
    safe_bind_string(stmt, idx, zPYR.sValuationSrce);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in InsertPayrec: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), zPYR.iID, zPYR.lTransNo,
                        (char *)"T", 0, -1, 0, (char *)"InsertPayrec", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in InsertPayrec", zPYR.iID,
                        zPYR.lTransNo, (char *)"T", 0, -1, 0,
                        (char *)"InsertPayrec", FALSE);
  }
}

DLLAPI void STDCALL SelectPayrec(PAYREC *pzPYR, int iID, char *sSecNo,
                                 char *sWi, char *sSecXtend, char *sAcctType,
                                 long lTransNo, long lDivintNo,
                                 ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"T", 0,
                        -1, 0, (char *)"SelectPayrec", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT("select "
                     "ID, sec_no, wi, sec_xtend, acct_type, "
                     "trans_no, secid, asof_date, tran_type, "
                     "divint_no, units, base_cost_xrate, "
                     "sys_cost_xrate, cur_val, eff_date, "
                     "mv_base_xrate, mv_sys_xrate, valuation_srce "
                     "from payrec "
                     "where ID=? and sec_no=? and wi=? and sec_xtend=? and "
                     "acct_type=? and trans_no=? and divint_no=?"));

    int idx = 0;
    stmt.bind(idx++, &iID);
    safe_bind_string(stmt, idx, sSecNo);
    safe_bind_string(stmt, idx, sWi);
    safe_bind_string(stmt, idx, sSecXtend);
    safe_bind_string(stmt, idx, sAcctType);
    stmt.bind(idx++, &lTransNo);
    stmt.bind(idx++, &lDivintNo);

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      pzPYR->iID = result.get<int>("ID", 0);
      read_string(result, "sec_no", pzPYR->sSecNo, sizeof(pzPYR->sSecNo));
      read_string(result, "wi", pzPYR->sWi, sizeof(pzPYR->sWi));
      read_string(result, "sec_xtend", pzPYR->sSecXtend,
                  sizeof(pzPYR->sSecXtend));
      read_string(result, "acct_type", pzPYR->sAcctType,
                  sizeof(pzPYR->sAcctType));
      pzPYR->lTransNo = result.get<long>("trans_no", 0);
      pzPYR->iSecID = result.get<int>("secid", 0);
      pzPYR->lAsofDate =
          timestamp_to_long(result.get<nanodbc::timestamp>("asof_date"));
      read_string(result, "tran_type", pzPYR->sTranType,
                  sizeof(pzPYR->sTranType));
      pzPYR->lDivintNo = result.get<long>("divint_no", 0);
      pzPYR->fUnits = result.get<double>("units", 0.0);
      pzPYR->fBaseCostXrate = result.get<double>("base_cost_xrate", 0.0);
      pzPYR->fSysCostXrate = result.get<double>("sys_cost_xrate", 0.0);
      pzPYR->fCurVal = result.get<double>("cur_val", 0.0);
      pzPYR->lEffDate =
          timestamp_to_long(result.get<nanodbc::timestamp>("eff_date"));
      pzPYR->fMvBaseXrate = result.get<double>("mv_base_xrate", 0.0);
      pzPYR->fMvSysXrate = result.get<double>("mv_sys_xrate", 0.0);
      read_string(result, "valuation_srce", pzPYR->sValuationSrce,
                  sizeof(pzPYR->sValuationSrce));
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectPayrec: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), iID, lTransNo, (char *)"T", 0, -1,
                        0, (char *)"SelectPayrec", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in SelectPayrec", iID, lTransNo,
                   (char *)"T", 0, -1, 0, (char *)"SelectPayrec", FALSE);
  }
}
