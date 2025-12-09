/**
 * SUB-SYSTEM: Login.DLL Integration
 * FILENAME: TransIO_Login.h
 * DESCRIPTION: Function declarations for Login.DLL integration
 * AUTHOR: Modernized 2025-11-29
 */

#ifndef TRANSIO_LOGIN_H
#define TRANSIO_LOGIN_H

#include "OLEDBIOCommon.h"

// Function pointer type for GetUserAndPassword from Login.DLL
typedef int(CALLBACK *LPFNGetUserAndPassword)(char *sAlias, char *sUser,
                                              char *sPassword, int *piUID,
                                              BOOL bShowDialog);

// Global function pointer (initialized in OLEDBIO.cpp via LoadLibrary)
extern LPFNGetUserAndPassword lpfnGetUserAndPassword;

// Helper functions for Login.DLL integration
void GetConnectionString(char *sAlias, char *sConnectStr, ERRSTRUCT *pzErr);
void GetODBCInfo(char *sODBCDSN, char *sSQLDBName, ERRSTRUCT *pzErr);

#endif // TRANSIO_LOGIN_H
