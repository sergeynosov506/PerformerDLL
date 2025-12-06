#pragma once
#include "OLEDBIOCommon.h"

// Simple Query Strings
extern const char* SQL_InsertAccdiv;
extern const char* SQL_SelectOneAccdiv;
extern const char* SQL_UpdateAccdiv;
extern const char* SQL_SelectPartCurrency;
extern const char* SQL_InsertDivhist;
extern const char* SQL_UpdateDivhist;
extern const char* SQL_DeleteDivhist;
extern const char* SQL_DeleteFWTrans;
extern const char* SQL_InsertFWTrans;
extern const char* SQL_SelectAllPartPortmain;
extern const char* SQL_SelectOnePartPortmain;
extern const char* SQL_SelectAllSubacct;
extern const char* SQL_SelectPortfolioRange;

// Complex Query Builders
void BuildDivintUnloadSQL(char* sAdjSQL, const char* sHoldings, const char* sType, char cMode);
