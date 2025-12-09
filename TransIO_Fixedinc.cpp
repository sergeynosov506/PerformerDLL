#include "TransIO_Fixedinc.h"
#include "ODBCErrorChecking.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"

extern thread_local nanodbc::connection gConn;

// =====================================================
// Implementation
// =====================================================

DLLAPI void STDCALL SelectPartFixedinc(char *sSecNo, char *sWi,
                                       PARTFINC *pzPFinc, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"SelectPartFixedinc", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT maturity_date, redemption_price, flat_code, pay_type, "
            "dated_date, amort_flag, original_face, cur_yld, cur_ytm, "
            "zero_coupon, issue_date, "
            "InflationIndexID, accretion_flag "
            "FROM fixedinc WHERE sec_no = ? and wi = ? "));

    int idx = 0;
    safe_bind_string(stmt, idx, sSecNo);
    safe_bind_string(stmt, idx, sWi);

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      pzPFinc->bRecordFound = TRUE;

      // Map columns
      // 1. maturity_date -> lMaturityDate (datetime -> long)
      read_date(result, "maturity_date", &pzPFinc->lMaturityDate);

      // 2. redemption_price -> fRedemptionPrice (double)
      read_double(result, "redemption_price", &pzPFinc->fRedemptionPrice);

      // 3. flat_code -> sFlatCode (char)
      read_string(result, "flat_code", pzPFinc->sFlatCode,
                  sizeof(pzPFinc->sFlatCode));

      // 4. pay_type -> sPayType (char)
      read_string(result, "pay_type", pzPFinc->sPayType,
                  sizeof(pzPFinc->sPayType));

      // 5. dated_date -> lDatedDate (datetime -> long)
      read_date(result, "dated_date", &pzPFinc->lDatedDate);

      // 6. amort_flag -> sAmortFlag (char)
      read_string(result, "amort_flag", pzPFinc->sAmortFlag,
                  sizeof(pzPFinc->sAmortFlag));

      // 7. original_face -> sOriginalFace (char)
      read_string(result, "original_face", pzPFinc->sOriginalFace,
                  sizeof(pzPFinc->sOriginalFace));

      // 8. cur_yld -> fCurYld (double)
      read_double(result, "cur_yld", &pzPFinc->fCurYld);

      // 9. cur_ytm -> fCurYtm (double)
      read_double(result, "cur_ytm", &pzPFinc->fCurYtm);

      // 10. zero_coupon -> sZeroCoupon (char)
      read_string(result, "zero_coupon", pzPFinc->sZeroCoupon,
                  sizeof(pzPFinc->sZeroCoupon));

      // 11. issue_date -> lIssueDate (datetime -> long)
      read_date(result, "issue_date", &pzPFinc->lIssueDate);

      // 12. InflationIndexID -> lInflationIndexID (long)
      read_long(result, "InflationIndexID", &pzPFinc->lInflationIndexID);

      // 13. accretion_flag -> sAccretFlag (char)
      read_string(result, "accretion_flag", pzPFinc->sAccretFlag,
                  sizeof(pzPFinc->sAccretFlag));

      // Copy keys
      strcpy(pzPFinc->sSecNo, sSecNo);
      strcpy(pzPFinc->sWi, sWi);
    } else {
      pzPFinc->bRecordFound = FALSE;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectPartFixedinc: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"SelectPartFixedinc", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in SelectPartFixedinc", 0, 0,
                   (char *)"D", 0, -1, 0, (char *)"SelectPartFixedinc", FALSE);
  }
}
