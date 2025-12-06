#pragma once
#include "nanodbc/nanodbc.h"
#include "commonheader.h"

// Returns the thread-local database connection
nanodbc::connection* GetDbConnection();

// Handles database errors by populating the ERRSTRUCT
void HandleDbError(ERRSTRUCT* pzErr, const char* msg, const char* context);
