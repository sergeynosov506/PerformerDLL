#include "RollIO_Holdmap.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"

extern thread_local nanodbc::connection gConn;

DLLAPI void STDCALL ReadAllHoldmap(char *sHoldingsName, char *sHoldcashName, char *sPortbalName, char *sPortmainName, 
							  char *sHedgxrefName, char *sPayrecName, char *sHoldtotName, char *sDataType, 
							  long *plAsofDate, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, "", 0, 0, 0, "ReadAllHoldmap", FALSE);
#endif

	InitializeErrStruct(pzErr);

	try
	{
		// SELECT asof_date, holdings_name, holdcash_name, portbal_name, Portdir_name, hedge_xref_name, payrec_name, holdtot_name, data_type FROM Holdmap Order by asof_date DESC
		nanodbc::result result = nanodbc::execute(gConn, NANODBC_TEXT("SELECT asof_date, holdings_name, holdcash_name, portbal_name, Portdir_name, hedge_xref_name, payrec_name, holdtot_name, data_type FROM Holdmap Order by asof_date DESC"));

		if (result.next())
		{
			// asof_date is column 0 (1-based in legacy, 0-based in nanodbc)
			// But wait, legacy map had:
			// COLUMN_ENTRY(1, m_vAsofDate) -> asof_date
			// COLUMN_ENTRY(2, m_sHoldings) -> holdings_name
			// ...
			
			// nanodbc result.get<type>(col_index)
			
			// 0: asof_date
			nanodbc::timestamp asof_ts = result.get<nanodbc::timestamp>(0);
			*plAsofDate = timestamp_to_long(asof_ts);

			// 1: holdings_name
			std::string holdings = result.get<std::string>(1, "");
			strcpy_s(sHoldingsName, STR80LEN, holdings.c_str());

			// 2: holdcash_name
			std::string holdcash = result.get<std::string>(2, "");
			strcpy_s(sHoldcashName, STR80LEN, holdcash.c_str());

			// 3: portbal_name
			std::string portbal = result.get<std::string>(3, "");
			strcpy_s(sPortbalName, STR80LEN, portbal.c_str());

			// 4: Portdir_name (mapped to m_sPortmain in legacy)
			std::string portmain = result.get<std::string>(4, "");
			strcpy_s(sPortmainName, STR80LEN, portmain.c_str());

			// 5: hedge_xref_name
			std::string hedgxref = result.get<std::string>(5, "");
			strcpy_s(sHedgxrefName, STR80LEN, hedgxref.c_str());

			// 6: payrec_name
			std::string payrec = result.get<std::string>(6, "");
			strcpy_s(sPayrecName, STR80LEN, payrec.c_str());

			// 7: holdtot_name
			std::string holdtot = result.get<std::string>(7, "");
			strcpy_s(sHoldtotName, STR80LEN, holdtot.c_str());

			// 8: data_type
			std::string datatype = result.get<std::string>(8, "");
			strcpy_s(sDataType, STR1LEN, datatype.c_str());
		}
		else
		{
			pzErr->iSqlError = SQLNOTFOUND;
		}
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("ReadAllHoldmap", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}
