#include "TransIO_Subaccount.h"
#include "ODBCErrorChecking.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include "subacct.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;
extern SUBACCTTABLE1 zSATable;

// Function to find given sub acct in the table in memory
int FindSubAcct(char *sAcctType) {
  int i, Res = -1;

  for (i = 0; i <= zSATable.iNumSAcct - 1; i++)
    if (strcmp(zSATable.zSAcct[i].sAcctType, sAcctType) == 0) {
      Res = i;
      break;
    }

  return Res;
} // FindSubAcct

// Function to add passed SubAcct to the memory table
void AddSubAcct(SUBACCT zSAcct) {
  if (zSATable.iNumSAcct == NUMSUBACCT)
    PrintError((char *)"SubAcct Table is Too Small", 0, 0, (char *)"", 999, 0,
               0, (char *)"OLEDBIO ADDSUBACCT", FALSE);
  else {
    memcpy(&zSATable.zSAcct[zSATable.iNumSAcct], &zSAcct, sizeof(zSAcct));
    zSATable.iNumSAcct++;
  }
} // AddSubAcct

DLLAPI void STDCALL SelectSellAcctType(char *sSellAcctType, char *sAcctType,
                                       ERRSTRUCT *pzErr) {
  InitializeErrStruct(pzErr);

  SUBACCT zSAcct;
  int i = FindSubAcct(sAcctType);
  if (i >= 0) {
    strcpy_s(sSellAcctType, 2, zSATable.zSAcct[i].sSellAcctType);
    return;
  }

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"", 0,
                        -1, 0, (char *)"SelectSellAcctType", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt, NANODBC_TEXT(
                  "SELECT sell_acct_type FROM subacct WHERE acct_type = ? "));

    int idx = 0;
    safe_bind_string(stmt, idx, sAcctType);

    nanodbc::result result = nanodbc::execute(stmt);

    if (result.next()) {
      read_string(result, "sell_acct_type", sSellAcctType, 2); // 1+1

      strcpy_s(zSAcct.sAcctType, sAcctType);
      strcpy_s(zSAcct.sSellAcctType, sSellAcctType);
      AddSubAcct(zSAcct);
    } else {
      pzErr->iSqlError = SQLNOTFOUND;
    }
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in SelectSellAcctType: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"", 0, -1, 0,
                        (char *)"SelectSellAcctType", FALSE);
  } catch (...) {
    *pzErr =
        PrintError((char *)"Unexpected error in SelectSellAcctType", 0, 0,
                   (char *)"", 0, -1, 0, (char *)"SelectSellAcctType", FALSE);
  }
}
