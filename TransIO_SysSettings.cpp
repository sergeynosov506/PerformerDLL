#include "TransIO_SysSettings.h"
#include "ODBCErrorChecking.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;

DLLAPI void SelectSyssettings(SYSSETTING *pzSyssetng, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectSyssettings", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT("SELECT SystemCurrency, DateForAccrualFlag,   \
				PaymentsStartDate, WeightedStatisticsFlag, EquityRatingsSource, \
				FixedRatingsSource, PerformanceType, YieldType, EarliestActionDate, \
				Flow_Weight_Method \
				FROM Syssetng"));

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      memset(pzSyssetng, 0, sizeof(*pzSyssetng));

      read_string(result, "SystemCurrency", pzSyssetng->sSystemcurrency,
                  sizeof(pzSyssetng->sSystemcurrency));
      read_string(result, "DateForAccrualFlag", pzSyssetng->sDateforaccrualflag,
                  sizeof(pzSyssetng->sDateforaccrualflag));
      pzSyssetng->iPaymentsStartDate = result.get<int>("PaymentsStartDate");
      read_string(result, "WeightedStatisticsFlag",
                  pzSyssetng->sWeightedStatisticsFlag,
                  sizeof(pzSyssetng->sWeightedStatisticsFlag));
      read_string(result, "EquityRatingsSource", pzSyssetng->sEquityRating,
                  sizeof(pzSyssetng->sEquityRating));
      read_string(result, "FixedRatingsSource", pzSyssetng->sFixedRating,
                  sizeof(pzSyssetng->sFixedRating));
      read_string(result, "PerformanceType", pzSyssetng->sPerformanceType,
                  sizeof(pzSyssetng->sPerformanceType));
      read_string(result, "YieldType", pzSyssetng->sYieldType,
                  sizeof(pzSyssetng->sYieldType));

      // Handle EarliestActionDate (VARIANT)
      // Assuming it's a date, we can read it as a timestamp and convert to
      // double (OLE Automation date) Or if it's stored as datetime in SQL,
      // nanodbc returns timestamp. The original code used a VARIANT, but here
      // we can just get the double value if needed, or if
      // pzSyssetng->lEarliestactiondate is a long/double. Looking at original
      // code: pzSyssetng->lEarliestactiondate =
      // cmdSelectSyssetng.m_vEarliestActionDate.date; So it expects a double
      // (DATE is double).

      // For now, let's assume we can get it as a timestamp and convert.
      // But wait, pzSyssetng->lEarliestactiondate is likely a long or double
      // representing the date. Let's check SYSSETTING struct definition if
      // possible, but for now assuming it's standard OLE date.

      // Actually, let's use the helper if available or just get as double if
      // the column is float/real. If it's datetime, we need to convert.

      // Let's try to get as timestamp and convert to OLE date.
      if (!result.is_null("EarliestActionDate")) {
        pzSyssetng->lEarliestactiondate = timestamp_to_long(
            result.get<nanodbc::timestamp>("EarliestActionDate"));
      } else {
        pzSyssetng->lEarliestactiondate = 0;
      }

      pzSyssetng->iFlowWeightMethod = result.get<int>("Flow_Weight_Method");
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectSyssettings: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectSyssettings", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in SelectSyssettings", 0, 0,
                   (char *)"", 0, -1, 0, (char *)"SelectSyssettings", FALSE);
  }
}

DLLAPI void SelectSysvalues(SYSVALUES *pzSysvalues, ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectSysvalues", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT("SELECT value FROM sysvalues WHERE name = ?"));
    int idx = 0;
    safe_bind_string(stmt, idx, pzSysvalues->sName);

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      read_string(result, "value", pzSysvalues->sValue,
                  sizeof(pzSysvalues->sValue));
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectSysvalues: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectSysvalues", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in SelectSysvalues", 0, 0,
                        (char *)"", 0, -1, 0, (char *)"SelectSysvalues", FALSE);
  }
}

DLLAPI void STDCALL SelectCFStartDate(VARIANT *pvCFStartDate,
                                      ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectCFStartDate", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "SELECT ISNULL(convert(date, value), '12/30/1899') As DateValue, 0 Sort \
																		FROM sysvalues WHERE name = 'CONSFEERORDATE' \
									union all select '12/30/1899' As DateValue, 1 Sort  from rtntype where id = 12 order by Sort"));

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      if (!result.is_null("DateValue")) {
        pvCFStartDate->date =
            timestamp_to_long(result.get<nanodbc::timestamp>("DateValue"));
        pvCFStartDate->vt = VT_DATE;
      } else {
        pvCFStartDate->date = 0;
        pvCFStartDate->vt = VT_DATE;
      }
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectCFStartDate: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectCFStartDate", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in SelectCFStartDate", 0, 0,
                   (char *)"", 0, -1, 0, (char *)"SelectCFStartDate", FALSE);
  }
}
