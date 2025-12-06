#include "TransIO_Trantype.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "trantype.h"
#include <iostream>
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;
extern TRANTYPETABLE1 zTTable; // Defined in OLEDBIO.cpp

int FindTranType(char *sTranType, char *sDrCr)
{
  int i, Res = -1;

  for (i=0; i <= zTTable.iNumTType - 1; i++)
    if (strcmp(zTTable.zTType[i].sTranType, sTranType) == 0 && 
		strcmp(zTTable.zTType[i].sDrCr, sDrCr) == 0)
    {
      Res = i;
      break;
	}

  return Res;	
} // FindTranType

void AddTranType(TRANTYPE zTranType)
{
  if (zTTable.iNumTType == NUMTRANTYPE) 
    PrintError((char*)"Trantype Table is Too Small", 0, 0, (char*)"", 999, 0, 0, (char*)"OLEDBIO ADDTRANTYPE", FALSE);
  else 
  {
    memcpy(&zTTable.zTType[zTTable.iNumTType], &zTranType, sizeof(zTranType));
    zTTable.iNumTType++;
  }
} //AddTranType

DLLAPI void STDCALL SelectTrantype(TRANTYPE *pzTrantype, char *sTranType, char *sDrCr, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    int i = FindTranType(sTranType, sDrCr);
    if (i >= 0) 
    {
        memcpy(pzTrantype, &zTTable.zTType[i], sizeof(zTTable.zTType[i]));
        return;
    }

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTrantype", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("select tran_type, dr_cr, \
				abbrv_desc, full_desc, tran_code, rpt_code, \
				cash_impact, sec_impact, perf_impact, portbal_impact, \
				trade_date_ind, settle_date_ind, trade_fees, trade_reverse, \
				journal, trade_screen, trade_sort, auto_gen \
				from TRANTYPE \
				where tran_type = ? and dr_cr = ? "));

        stmt.bind(0, sTranType);
        stmt.bind(1, sDrCr);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            TRANTYPE zTranType;
            memset(&zTranType, 0, sizeof(zTranType));

            read_string(result, "tran_type", zTranType.sTranType, sizeof(zTranType.sTranType));
            read_string(result, "dr_cr", zTranType.sDrCr, sizeof(zTranType.sDrCr));
            read_string(result, "abbrv_desc", zTranType.sAbbrvDesc, sizeof(zTranType.sAbbrvDesc));
            read_string(result, "full_desc", zTranType.sFullDesc, sizeof(zTranType.sFullDesc));
            read_string(result, "tran_code", zTranType.sTranCode, sizeof(zTranType.sTranCode));
            read_string(result, "rpt_code", zTranType.sRptCode, sizeof(zTranType.sRptCode));
            
            zTranType.lCashImpact = result.get<long>("cash_impact");
            zTranType.lSecImpact = result.get<long>("sec_impact");
            read_string(result, "perf_impact", zTranType.sPerfImpact, sizeof(zTranType.sPerfImpact));
            zTranType.lPortbalImpact = result.get<long>("portbal_impact");
            
            zTranType.iTradeDateInd = result.get<short>("trade_date_ind");
            zTranType.iSettleDateInd = result.get<short>("settle_date_ind");
            read_string(result, "trade_fees", zTranType.sTradeFees, sizeof(zTranType.sTradeFees));
            read_string(result, "trade_reverse", zTranType.sTradeReverse, sizeof(zTranType.sTradeReverse));
            read_string(result, "journal", zTranType.sJournal, sizeof(zTranType.sJournal));
            read_string(result, "trade_screen", zTranType.sTradeScreen, sizeof(zTranType.sTradeScreen));
            zTranType.iTradeSort = result.get<short>("trade_sort");
            read_string(result, "auto_gen", zTranType.sAutoGen, sizeof(zTranType.sAutoGen));

            memcpy(pzTrantype, &zTranType, sizeof(*pzTrantype));
            AddTranType(*pzTrantype);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectTrantype: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTrantype", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectTrantype", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTrantype", FALSE);
    }
}

DLLAPI void STDCALL SelectTrancode(char *sTranCode, char *sTranType, char *sDrCr, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    int i = FindTranType(sTranType, sDrCr);
    if (i >= 0) 
    {
        strcpy_s(sTranCode, 2, zTTable.zTType[i].sTranCode);
        return;
    }

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTrancode", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("select tran_code from TRANTYPE where tran_type = ? and dr_cr = ? "));

        stmt.bind(0, sTranType);
        stmt.bind(1, sDrCr);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "tran_code", sTranCode, 2);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectTrancode: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTrancode", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectTrancode", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTrancode", FALSE);
    }
}
