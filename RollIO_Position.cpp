#include "RollIO_Position.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"

extern thread_local nanodbc::connection gConn;

DLLAPI void STDCALL SelectPositionIndicator(char *sSecNo, char *sWi, char *sPositionInd, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectPositionIndicator", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT("SELECT position_ind FROM assets a, sectype s WHERE sec_no = ? AND when_issue = ? AND a.sec_type = s.sec_type"));
		
		stmt.bind(0, sSecNo);
		stmt.bind(1, sWi);

		nanodbc::result result = stmt.execute();
		if (result.next())
		{
			strcpy_s(sPositionInd, STR1LEN, result.get<std::string>(0, "").c_str());
		}
		else
		{
			pzErr->iSqlError = SQLNOTFOUND;
		}
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("SelectPositionIndicator", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}
