/*
*
* SUB-SYSTEM:
*
* FILENAME: CreatePITIPS.h
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR:
*
*/

#include "payments.h"

#define NUMPITIPS        10


DllExport ERRSTRUCT WINAPI GenereatePhantomIncome(long lValDate, char *sMode, 
																									int iID, char *sSecNo, char *sWi, 
																									char *sSecXtend, 
																									char *sAcctType, long lTransNo);

ERRSTRUCT PITIPSGeneration(PORTTABLE zPMainTable, PINFOTABLE zPInfoTable, 
													 SECTYPETABLE zSTable, CURRTABLE zCTable, 
													 long lValDate, char *sMode,
													 char *sSecNo, char *sWi,
													 char *sSecXtend, char *sAcctType, long lTransNo);

ERRSTRUCT PITIPSUnloadAndSort(long lValDate);

ERRSTRUCT BuildPITable(char *sMode, char *sFileName, long lValDate,
											 int iID, char *sSecNo, char *sWi, char *sSecXtend,
                       char *sAcctType, long lTransNo, PITIPSTABLE *pzPITable);

ERRSTRUCT AddRecordToPITable(PITIPSTABLE *pzPITab, PITIPSSTRUCT zPI);

ERRSTRUCT PITIPSCallTranAlloc(PITIPSSTRUCT zPI, char *sAcctMthd, 
															SECTYPE zSType, long lValDate);

void InitializePITIPSTable(PITIPSTABLE *pzPITable);
void InitializePITIPSStruct(PITIPSSTRUCT *pzPI);

BOOL IsThisAnYearEnd(long lDate);
int DayOfMonth(long lDate);

