#include "PerformanceIO_Common.h"
#include "OLEDBIOCommon.h" // For PrintError if needed, or just use commonheader
#include <iostream>

// Defined in OLEDBIO.cpp
extern thread_local nanodbc::connection gConn;

nanodbc::connection* GetDbConnection()
{
    if (gConn.connected())
        return &gConn;
    return nullptr;
}

void HandleDbError(ERRSTRUCT* pzErr, const char* msg, const char* context)
{
    if (pzErr)
    {
        pzErr->iSqlError = -1;
        PrintError(msg ? msg : "Unknown Error", 0, 0, "E", 0, -1, 0, context ? context : "DBERR", FALSE);
    }
}
