#include "RollIO_Portmain.h"
#include "ODBCErrorChecking.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <regex>


extern thread_local nanodbc::connection gConn;

// Helper to replace table names in SQL
static std::string ReplaceTableName(std::string sql,
                                    const std::string &placeholder,
                                    const std::string &tableName) {
  return std::regex_replace(sql, std::regex(placeholder), tableName);
}

DLLAPI void STDCALL SelectLastTransNo(long iID, char *sPortmainName,
                                      long *plLastTransNo, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, sPortmainName, 0, 0, 0, "SelectLastTransNo",
             FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    std::string sql =
        "SELECT lasttransno FROM %PORTMAIN_TABLE_NAME% WHERE id = ?";
    sql = ReplaceTableName(sql, "%PORTMAIN_TABLE_NAME%", sPortmainName);

    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt, NANODBC_TEXT(sql));

    stmt.bind(0, &iID);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      *plLastTransNo = result.get<int>(0);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectLastTransNo", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL SelectRollDate(long iID, char *sPortmainName,
                                   long *plLastRollDate, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, sPortmainName, 0, 0, 0, "SelectRollDate", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    std::string sql = "SELECT rolldate FROM %PORTMAIN_TABLE_NAME% WHERE id = ?";
    sql = ReplaceTableName(sql, "%PORTMAIN_TABLE_NAME%", sPortmainName);

    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt, NANODBC_TEXT(sql));

    stmt.bind(0, &iID);

    nanodbc::result result = stmt.execute();
    if (result.next()) {
      *plLastRollDate = timestamp_to_long(result.get<nanodbc::timestamp>(0));
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const std::exception &e) {
    *pzErr = PrintError("SelectRollDate", 0, 0, "", 0, 0, 0, (char *)e.what(),
                        FALSE);
  }
}

DLLAPI void STDCALL UpdateRollDateAndLastTransNo(long iID, long lRollDate,
                                                 long lLastTransNo,
                                                 char *sPortmainName,
                                                 ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, sPortmainName, 0, 0, 0,
             "UpdateRollDateAndLastTransNo", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    std::string sql = "UPDATE %Portmain_TABLE_NAME% SET RollDate = ?, "
                      "LastTransNo = ?, AcctRolledOn = GetDate() WHERE id = ?";
    sql = ReplaceTableName(sql, "%Portmain_TABLE_NAME%", sPortmainName);

    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt, NANODBC_TEXT(sql));

    nanodbc::timestamp ts;
    long_to_timestamp(lRollDate, ts);
    stmt.bind(0, &ts);
    stmt.bind(1, &lLastTransNo);
    stmt.bind(2, &iID);

    stmt.execute();
  } catch (const std::exception &e) {
    *pzErr = PrintError("UpdateRollDateAndLastTransNo", 0, 0, "", 0, 0, 0,
                        (char *)e.what(), FALSE);
  }
}

DLLAPI void STDCALL UpdateRollDate(long iID, long lRollDate,
                                   char *sPortmainName, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  PrintError("Entering", 0, 0, sPortmainName, 0, 0, 0, "UpdateRollDate", FALSE);
#endif
  InitializeErrStruct(pzErr);
  try {
    std::string sql = "UPDATE %Portmain_TABLE_NAME% SET RollDate = ?, "
                      "AcctRolledOn = GetDate() WHERE id = ?";
    sql = ReplaceTableName(sql, "%Portmain_TABLE_NAME%", sPortmainName);

    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt, NANODBC_TEXT(sql));

    nanodbc::timestamp ts;
    long_to_timestamp(lRollDate, ts);
    stmt.bind(0, &ts);
    stmt.bind(1, &iID);

    stmt.execute();
  } catch (const std::exception &e) {
    *pzErr = PrintError("UpdateRollDate", 0, 0, "", 0, 0, 0, (char *)e.what(),
                        FALSE);
  }
}
