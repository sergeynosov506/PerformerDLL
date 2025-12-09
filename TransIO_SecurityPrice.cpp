#include "TransIO_SecurityPrice.h"
#include "DateFunctions.h"
#include "ODBCErrorChecking.h"
#include "TransIO.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include "syssettings.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;
extern CErrorChecking dbErr;

DLLAPI BOOL STDCALL IsItManualForSecurity(char *sSecNo, char *sWi,
                                          long lPriceDate, ERRSTRUCT *pzErr) {
  BOOL bRes = FALSE;
  InitializeErrStruct(pzErr);

  try {
    nanodbc::statement statement(gConn);
    string sSQL = "SELECT price_source FROM histpric WHERE sec_no = ? AND wi = "
                  "? AND price_date = ? AND price_source IN ('M', 'W')";
    nanodbc::prepare(statement, sSQL);

    int idx = 0;
    safe_bind_string(statement, idx, sSecNo);
    safe_bind_string(statement, idx, sWi);

    nanodbc::date dPriceDate = LongToDateStruct(lPriceDate);
    statement.bind(idx++, &dPriceDate);

    nanodbc::result result = statement.execute();

    if (result.next()) {
      bRes = TRUE;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("IsItManualForSecurity", 0, 0, sSecNo, 0, 0, 0,
                        (char *)e.what(), FALSE);
  }

  return bRes;
}

DLLAPI BOOL STDCALL IsManualClosingPrice(char *sSecNo, char *sWi,
                                         long lPriceDate, ERRSTRUCT *pzErr) {
  BOOL bRes = FALSE;
  InitializeErrStruct(pzErr);

  try {
    nanodbc::statement statement(gConn);
    string sSQL = "SELECT field_name FROM manprice WHERE sec_no = ? AND wi = ? "
                  "AND price_date = ? AND field_name = 'close_price'";
    nanodbc::prepare(statement, sSQL);

    int idx = 0;
    safe_bind_string(statement, idx, sSecNo);
    safe_bind_string(statement, idx, sWi);

    nanodbc::date dPriceDate = LongToDateStruct(lPriceDate);
    statement.bind(idx++, &dPriceDate);

    nanodbc::result result = statement.execute();

    if (result.next()) {
      bRes = TRUE;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("IsManualClosingPrice", 0, 0, sSecNo, 0, 0, 0,
                        (char *)e.what(), FALSE);
  }

  return bRes;
}

ERRSTRUCT InternalGetSecurityPrice(char *sSecNo, char *sWi, long lPriceDate,
                                   PRICEINFO *pzPInfo, BOOL bFindLastPrice) {
  ERRSTRUCT zErr;
  long lLastBusinessDate;
  ASSETS zAssets;

  InitializeErrStruct(&zErr);

  try {
    nanodbc::statement statement(gConn);
    string sSQL =
        "SELECT hp1.date_price_updated, hp1.price_source, hp1.close_price, "
        "hp1.exrate, "
        "hp1.inc_exrate, hp1.ann_div_cpn, "
        "hp2.date_exrate_updated, hp2.exrate as currexrate, hp2.inc_exrate as "
        "currincexrate, "
        "f.accr_int, f.cur_yld, f.cur_ytm, f.yld_to_worst, f.variablerate, "
        "e.eps, "
        "a.benchmark_secno, a.curr_id, a.inc_curr_id, a.trad_unit, a.sec_type "
        "FROM histpric hp1 "
        "LEFT OUTER JOIN histfinc f ON hp1.sec_no = f.sec_no AND hp1.wi = f.wi "
        "AND hp1.price_date = f.price_date "
        "LEFT OUTER JOIN histeqty e ON hp1.sec_no = e.sec_no AND hp1.wi = e.wi "
        "AND hp1.price_date = e.price_date "
        "LEFT OUTER JOIN assets a ON hp1.sec_no = a.sec_no AND hp1.wi = "
        "a.when_issue "
        "LEFT OUTER JOIN histpric hp2 ON a.curr_id = hp2.sec_no AND "
        "hp2.sec_no='N' AND hp2.price_date = hp1.price_date "
        "WHERE hp1.sec_no = ? AND hp1.wi = ? AND hp1.price_date = ?";

    nanodbc::prepare(statement, sSQL);

    int idx = 0;
    safe_bind_string(statement, idx, sSecNo);
    safe_bind_string(statement, idx, sWi);

    nanodbc::date dPriceDate = LongToDateStruct(lPriceDate);
    statement.bind(idx++, &dPriceDate);

    nanodbc::result result = statement.execute();

    if (result.next()) {
      pzPInfo->bIsPriceValid = TRUE;
      pzPInfo->bIsExrateValid = TRUE;
      pzPInfo->bRecordFound = TRUE;

      strcpy_s(pzPInfo->sSecNo, sSecNo);
      strcpy_s(pzPInfo->sWi, sWi);
      pzPInfo->lPriceDate = lPriceDate;

      pzPInfo->lDatePriceUpdated =
          DateStructToLong(result.get<nanodbc::date>(0));
      strcpy_s(pzPInfo->sPriceSource, result.get<string>(1, "").c_str());
      pzPInfo->fClosePrice = result.get<double>(2, 0.0);

      double fExrate = result.get<double>(3, 0.0);
      double fIncExrate = result.get<double>(4, 0.0);

      pzPInfo->fAnnDivCpn = result.get<double>(5, 0.0);

      pzPInfo->lDateExrateUpdated =
          DateStructToLong(result.get<nanodbc::date>(6));
      double fCurrExrate = result.get<double>(7, 0.0);
      double fCurrIncExrate = result.get<double>(8, 0.0);

      pzPInfo->fAccrInt = result.get<double>(9, 0.0);
      pzPInfo->fCurYld = result.get<double>(10, 0.0);
      double fCurYtm = result.get<double>(11, 0.0);
      double fYldToWorst = result.get<double>(12, 0.0);
      pzPInfo->fVariablerate = result.get<double>(13, 0.0);

      pzPInfo->fEps = result.get<double>(14, 0.0);

      strcpy_s(pzPInfo->sBenchmarkSecNo, result.get<string>(15, "").c_str());
      strcpy_s(pzPInfo->sCurrId, result.get<string>(16, "").c_str());
      strcpy_s(pzPInfo->sIncCurrId, result.get<string>(17, "").c_str());
      pzPInfo->fTradUnit = result.get<double>(18, 0.0);
      pzPInfo->iSecType = result.get<int>(19, 0);

      if (fCurrExrate > 0) {
        pzPInfo->fExrate = fCurrExrate;
        pzPInfo->fIncExrate = fCurrIncExrate;
      } else {
        pzPInfo->fExrate = fExrate;
        pzPInfo->fIncExrate =
            fIncExrate; // Note: Original code used fExrate here too, likely bug
                        // or intentional? "pzPInfo->fIncExrate =
                        // cmdGetSecurityPrice.m_zPInfo.fExrate;" - line 4093. I
                        // will follow original code logic but check if it was a
                        // typo.
        // Original code:
        // pzPInfo->fExrate = cmdGetSecurityPrice.m_zPInfo.fExrate;
        // pzPInfo->fIncExrate = cmdGetSecurityPrice.m_zPInfo.fExrate;
        // Wait, line 4093 in view says: pzPInfo->fIncExrate =
        // cmdGetSecurityPrice.m_zPInfo.fExrate; This looks like a typo in
        // original code, but I must preserve behavior unless I am sure.
        // However, the column map has fIncExrate at index 5 (0-based 4).
        // Let's look at line 4093 again.
        // 4092: pzPInfo->fExrate = cmdGetSecurityPrice.m_zPInfo.fExrate;
        // 4093: pzPInfo->fIncExrate = cmdGetSecurityPrice.m_zPInfo.fExrate;
        // Yes, it assigns fExrate to fIncExrate.
        // But wait, the column map has m_zPInfo.fIncExrate at index 5.
        // So m_zPInfo has fIncExrate.
        // Why would it assign fExrate to fIncExrate?
        // Maybe because if fCurrExrate is 0, it falls back to hp1.exrate?
        // But hp1 has inc_exrate too.
        // I will stick to original code behavior for binary compatibility.
        pzPInfo->fIncExrate = fExrate;
      }

      // Check Yield Type
      // Need access to zSyssetng.sYieldType. zSyssetng is global in
      // TransIO.cpp? I need to check if zSyssetng is available. It is likely
      // externed in TransIO.h or OLEDBIOCommon.h? I will assume it is available
      // or I need to declare it extern. In TransIO.cpp line 4100: if
      // (_stricmp(zSyssetng.sYieldType, "W") == 0)

      // extern SYSSETTINGS zSyssetng; // Removed: Redundant and incorrect type.
      // Using global zSyssetng from OLEDBIOCommon.h

      if (_stricmp(zSyssetng.sYieldType, "W") == 0)
        pzPInfo->fCurYtm = fYldToWorst;
      else
        pzPInfo->fCurYtm = fCurYtm;

      // If same currency id but two different rates, make the rate same
      if (_stricmp(pzPInfo->sCurrId, pzPInfo->sIncCurrId) == 0 &&
          pzPInfo->fExrate != pzPInfo->fIncExrate) {
        if (pzPInfo->fExrate > 0)
          pzPInfo->fIncExrate = pzPInfo->fExrate;
        else
          pzPInfo->fExrate = pzPInfo->fIncExrate;
      }

      if (pzPInfo->fClosePrice > 0)
        pzPInfo->bIsPriceValid = TRUE;
      else
        pzPInfo->bIsPriceValid = FALSE;

      if (pzPInfo->fClosePrice == 0)
        pzPInfo->bIsPriceValid = IsManualClosingPrice(
            pzPInfo->sSecNo, pzPInfo->sWi, pzPInfo->lPriceDate, &zErr);

      if (pzPInfo->fExrate <= 0)
        pzPInfo->bIsExrateValid = FALSE;
    } else {
      if (bFindLastPrice) {
        zErr.iBusinessError = LastBusinessDay(lPriceDate, (char *)"USA",
                                              (char *)"M", &lLastBusinessDate);
        if (zErr.iBusinessError != 0)
          return zErr;

        zErr = InternalGetSecurityPrice(sSecNo, sWi, lLastBusinessDate, pzPInfo,
                                        FALSE);
      } else {
        SelectAsset(&zAssets, sSecNo, sWi, -1, &zErr);
        if (zErr.iSqlError == SQLNOTFOUND) {
          PrintError("SecNo not found in Assets", 0, 0, sSecNo, 0,
                     zErr.iSqlError, 0, "GETSECURITYPRICE", FALSE);
          return zErr;
        } else if (zErr.iSqlError != 0) {
          return zErr;
        }

        strcpy_s(pzPInfo->sSecNo, zAssets.sSecNo);
        strcpy_s(pzPInfo->sWi, zAssets.sWhenIssue);
        strcpy_s(pzPInfo->sCurrId, zAssets.sCurrId);
        strcpy_s(pzPInfo->sIncCurrId, zAssets.sIncCurrId);
        pzPInfo->fTradUnit = zAssets.fTradUnit;
        pzPInfo->iSecType = zAssets.iSecType;
        strcpy_s(pzPInfo->sBenchmarkSecNo, zAssets.sBenchmarkSecNo);
      }
    }
  } catch (const std::exception &e) {
    zErr = PrintError("InternalGetSecurityPrice", 0, 0, "", 0, 0, 0,
                      (char *)e.what(), FALSE);
  }

  return zErr;
}

DLLAPI void STDCALL GetSecurityPrice(char *sSecNo, char *sWi, long lPriceDate,
                                     PRICEINFO *pzPInfo, ERRSTRUCT *pzErr) {
  PrintError("Entering", 0, 0, "", 0, 0, 0, "GetSecurityPrice", FALSE);
  *pzErr = InternalGetSecurityPrice(sSecNo, sWi, lPriceDate, pzPInfo, TRUE);
}
