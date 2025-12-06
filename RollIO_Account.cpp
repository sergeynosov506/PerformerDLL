#include "RollIO_Account.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include <regex>

extern thread_local nanodbc::connection gConn;

// Helper to replace table names in SQL
static std::string ReplaceTableName(std::string sql, const std::string& placeholder, const std::string& tableName)
{
	return std::regex_replace(sql, std::regex(placeholder), tableName);
}

DLLAPI void STDCALL AccountDeleteHoldings(long iID, char *sSecNo, char *sWi, char *sHoldingsName, 
									 BOOL bSecSpecific, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sHoldingsName, 0, 0, 0, "AccountDeleteHoldings", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSecSpecific)
		{
			sql = "Delete from %Holdings_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "Delete from %Holdings_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%Holdings_TABLE_NAME%", sHoldingsName);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSecSpecific)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountDeleteHoldings", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountDeleteHoldcash(long iID, char *sSecNo, char *sWi, char *sHoldcashName, 
									 BOOL bSecSpecific, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sHoldcashName, 0, 0, 0, "AccountDeleteHoldcash", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSecSpecific)
		{
			sql = "Delete from %Holdcash_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "Delete from %Holdcash_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%Holdcash_TABLE_NAME%", sHoldcashName);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSecSpecific)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountDeleteHoldcash", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountDeletePortmain(long iID, char *sPortmainName, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sPortmainName, 0, 0, 0, "AccountDeletePortmain", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql = "Delete from %Portmain_TABLE_NAME% Where ID=?";
		sql = ReplaceTableName(sql, "%Portmain_TABLE_NAME%", sPortmainName);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountDeletePortmain", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountDeleteHedgxref(long iID, char *sSecNo, char *sWi, char *sHedgxrefName, 
									 BOOL bSecSpecific, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sHedgxrefName, 0, 0, 0, "AccountDeleteHedgxref", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSecSpecific)
		{
			sql = "Delete from %Hedgxref_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "Delete from %Hedgxref_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%Hedgxref_TABLE_NAME%", sHedgxrefName);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSecSpecific)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountDeleteHedgxref", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountDeletePayrec(long iID, char *sSecNo, char *sWi, char *sPayrecName, 
								   BOOL bSecSpecific, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sPayrecName, 0, 0, 0, "AccountDeletePayrec", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSecSpecific)
		{
			sql = "Delete from %Payrec_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "Delete from %Payrec_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%Payrec_TABLE_NAME%", sPayrecName);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSecSpecific)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountDeletePayrec", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountInsertHoldings(long iID, char *sSecNo, char *sWi, char *sDestHoldings, char *sSrceHoldings, 
									 BOOL bSpecificSecNo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sDestHoldings, 0, 0, 0, "AccountInsertHoldings", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSpecificSecNo)
		{
			sql = "INSERT INTO %SRCE_HOLDINGS_TABLE_NAME% SELECT * FROM %DEST_HOLDINGS_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "INSERT INTO %SRCE_HOLDINGS_TABLE_NAME% SELECT * FROM %DEST_HOLDINGS_TABLE_NAME% Where ID=?";
		}

		// Note: Legacy code seems to have swapped Dest/Srce in AdjustSQL or logic?
		// Legacy: AdjustSQL(sSQL, sDestHoldings, sSrceHoldings)
		// And SQL: INSERT INTO %SRCE_HOLDINGS_TABLE_NAME% SELECT * FROM %DEST_HOLDINGS_TABLE_NAME%
		// So sDestHoldings replaces %SRCE_HOLDINGS_TABLE_NAME% ??
		// Let's check legacy AdjustSQL usage.
		// cmdAccountInsertHoldingsSecNo.AdjustSQL(sSQL, sDestHoldings, sSrceHoldings);
		// If AdjustSQL takes (sql, p1, p2), and SQL has %SRCE...% and %DEST...%
		// It likely replaces first param with first placeholder.
		// Wait, usually AdjustSQL replaces %TABLE_NAME% with param.
		// If there are two placeholders, it depends on implementation.
		// Assuming standard behavior: 
		// "INSERT INTO " + sDestHoldings + " SELECT * FROM " + sSrceHoldings + ...
		// The function name is AccountInsertHoldings(..., sDestHoldings, sSrceHoldings, ...)
		// It implies inserting INTO Dest FROM Srce.
		// So %SRCE_HOLDINGS_TABLE_NAME% in SQL string should be replaced by sDestHoldings (the target of INSERT).
		// And %DEST_HOLDINGS_TABLE_NAME% should be replaced by sSrceHoldings (the source of SELECT).
		// This naming in legacy SQL string is confusing (%SRCE...% for INSERT target?).
		// Let's assume standard SQL: INSERT INTO [Target] SELECT ... FROM [Source]
		// So sDestHoldings is Target. sSrceHoldings is Source.
		
		sql = ReplaceTableName(sql, "%SRCE_HOLDINGS_TABLE_NAME%", sDestHoldings);
		sql = ReplaceTableName(sql, "%DEST_HOLDINGS_TABLE_NAME%", sSrceHoldings);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSpecificSecNo)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountInsertHoldings", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountInsertHoldcash(long iID, char *sSecNo, char *sWi, char *sDestHoldcash, char *sSrceHoldcash, 
							         BOOL bSpecificSecNo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sDestHoldcash, 0, 0, 0, "AccountInsertHoldcash", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSpecificSecNo)
		{
			sql = "INSERT INTO %SRCE_HOLDCASH_TABLE_NAME% SELECT * FROM %DEST_HOLDCASH_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "INSERT INTO %SRCE_HOLDCASH_TABLE_NAME% SELECT * FROM %DEST_HOLDCASH_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%SRCE_HOLDCASH_TABLE_NAME%", sDestHoldcash);
		sql = ReplaceTableName(sql, "%DEST_HOLDCASH_TABLE_NAME%", sSrceHoldcash);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSpecificSecNo)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountInsertHoldcash", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountInsertPortmain(long iID, char *sDestPortmain, char *sSrcePortmain, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sDestPortmain, 0, 0, 0, "AccountInsertPortmain", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql = "INSERT INTO %SRCE_PORTMAIN_TABLE_NAME% SELECT * FROM %DEST_PORTMAIN_TABLE_NAME% Where ID=?";
		
		sql = ReplaceTableName(sql, "%SRCE_PORTMAIN_TABLE_NAME%", sDestPortmain);
		sql = ReplaceTableName(sql, "%DEST_PORTMAIN_TABLE_NAME%", sSrcePortmain);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountInsertPortmain", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountInsertHedgxref(long iID, char *sSecNo, char *sWi, char *sDestHedgxref, char *sSrceHedgxref, 
									 BOOL bSpecificSecNo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sDestHedgxref, 0, 0, 0, "AccountInsertHedgxref", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSpecificSecNo)
		{
			sql = "INSERT INTO %SRCE_HEDGXREF_TABLE_NAME% SELECT * FROM %DEST_HEDGXREF_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "INSERT INTO %SRCE_HEDGXREF_TABLE_NAME% SELECT * FROM %DEST_HEDGXREF_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%SRCE_HEDGXREF_TABLE_NAME%", sDestHedgxref);
		sql = ReplaceTableName(sql, "%DEST_HEDGXREF_TABLE_NAME%", sSrceHedgxref);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSpecificSecNo)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountInsertHedgxref", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}

DLLAPI void STDCALL AccountInsertPayrec(long iID, char *sSecNo, char *sWi, char *sDestPayrec, char *sSrcePayrec, 
								   BOOL bSpecificSecNo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
	PrintError("Entering", 0, 0, sDestPayrec, 0, 0, 0, "AccountInsertPayrec", FALSE);
#endif
	InitializeErrStruct(pzErr);
	try
	{
		std::string sql;
		if (bSpecificSecNo)
		{
			sql = "INSERT INTO %SRCE_PAYREC_TABLE_NAME% SELECT * FROM %DEST_PAYREC_TABLE_NAME% Where ID=? and sec_no=? and wi=?";
		}
		else
		{
			sql = "INSERT INTO %SRCE_PAYREC_TABLE_NAME% SELECT * FROM %DEST_PAYREC_TABLE_NAME% Where ID=?";
		}

		sql = ReplaceTableName(sql, "%SRCE_PAYREC_TABLE_NAME%", sDestPayrec);
		sql = ReplaceTableName(sql, "%DEST_PAYREC_TABLE_NAME%", sSrcePayrec);

		nanodbc::statement stmt(gConn);
		nanodbc::prepare(stmt, NANODBC_TEXT(sql));
		
		stmt.bind(0, &iID);
		if (bSpecificSecNo)
		{
			stmt.bind(1, sSecNo);
			stmt.bind(2, sWi);
		}

		stmt.execute();
	}
	catch (const std::exception& e)
	{
		*pzErr = PrintError("AccountInsertPayrec", 0, 0, "", 0, 0, 0, (char*)e.what(), FALSE);
	}
}
