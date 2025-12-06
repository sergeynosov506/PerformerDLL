/**
* FILENAME: OLEDBIOCommon.cpp
* DESCRIPTION:
*   Implements shared utilities declared in commonheader.h
*   Modernized for ODBC (no COM/ATL dependencies).
* AUTHOR: Valeriy Yegorov (C) 2003; Refactor: Grok/xAI (2025)
*/

#include "commonheader.h"
#include "OLEDBIOCommon.h"
#include <ctime>
#include <string>
#include <iostream>
#include <math.h>
#include <float.h>

int iVendorID = 0;

#pragma managed(push, off)

void __stdcall InitializeErrStruct(ERRSTRUCT* pzErr)
{
    if (!pzErr) return;
    pzErr->iID = 0;
    pzErr->lRecNo = 0;
    pzErr->sRecType[0] = ' ';
    pzErr->sRecType[1] = '\0';
    pzErr->iBusinessError = 0;
    pzErr->iSqlError = 0;
    pzErr->iIsamCode = 0;
}

// ----------------------------------------------------------
// Legacy-style logging to file in %TEMP%\OLEDBio.err
// ----------------------------------------------------------
ERRSTRUCT __stdcall PrintError(
    const char* sErrMsg,
    int iID,
    long lRecNo,
    const char* sRecType,
    int iBusinessErr,
    int iSqlErr,
    int iIsamCode,
    const char* sMsgIdentifier,
    BOOL bWarning)
{
    ERRSTRUCT zErr;
    InitializeErrStruct(&zErr);
    zErr.iID = iID;
    zErr.lRecNo = lRecNo;
    strncpy_s(zErr.sRecType, sRecType ? sRecType : " ", _TRUNCATE);
    zErr.iBusinessError = iBusinessErr;
    zErr.iSqlError = iSqlErr;
    zErr.iIsamCode = iIsamCode;

    char sErrWarn[10];
    strcpy_s(sErrWarn, bWarning ? "WARNING" : "ERROR");

    // Build temporary log path
    static char sErrorFile[MAX_PATH] = "";
    if (sErrorFile[0] == '\0') {
        char tempPath[MAX_PATH];
        DWORD dwRetVal = GetTempPathA(MAX_PATH, tempPath);
        if (dwRetVal && dwRetVal < MAX_PATH)
            sprintf_s(sErrorFile, "%sOLEDBio.err", tempPath);
        else
            strcpy_s(sErrorFile, "c:\\temp\\OLEDBio.err");
    }

    // Date/time strings
    char sDate[12], sTime[12];
    _strdate_s(sDate);
    _strtime_s(sTime);

    // Write entry
    FILE* fp = nullptr;
    if (fopen_s(&fp, sErrorFile, "a") == 0 && fp) {
        fprintf(fp,
            "%s %s %s | ID=%d RecType=%s RecNo=%ld "
            "| BErr=%d SqlErr=%d IsamErr=%d | Msg=%s | Loc=%s\n",
            sDate, sTime, sErrWarn, iID,
            sRecType ? sRecType : "?", lRecNo,
            iBusinessErr, iSqlErr, iIsamCode,
            sErrMsg ? sErrMsg : "(null)",
            sMsgIdentifier ? sMsgIdentifier : "(null)");
        fclose(fp);
    }

#ifdef DEBUG
    std::cerr << "[OLEDBIO] " << sErrWarn << ": "
        << (sErrMsg ? sErrMsg : "(null)") << std::endl;
#endif

    return zErr;
}

// ----------------------------------------------------------
// Utility functions (RoundDouble, etc.)
// ----------------------------------------------------------
BOOL IsValueZero(double fValue, unsigned short iPrecision)
{
    double fMinVal, fMaxVal;

    if (iPrecision == 0 || iPrecision > 13)
        iPrecision = 3;

    fMinVal = pow(10.0, -iPrecision);
    fMaxVal = fMinVal * -1;

    if (fValue < fMinVal && fValue > fMaxVal)
        return TRUE;
    else
        return FALSE;
}

double TruncateDouble(double fNumber, int iPrecision)
{
    double      fTemp1, fTemp2, fResult;
    LONGLONG    lNumber, lTemp;

    if (iPrecision < 0 || iPrecision > 15 || fabs(fNumber) > 1e18)
        return fNumber;

    // get the absolute fractional part of the number
    lNumber = (LONGLONG)fNumber;
    fTemp1 = fabs(fNumber - lNumber);

    // First multiply the fractional part by 10 ^ Precision and then divide the integer part
    // of the result by 10 ^ Precision. This will give the fractional part of the number upto
    // the required precision
    fTemp2 = fTemp1 * pow(10.0, iPrecision);
    // add a small fraction to the result, this is necessary to avoid wrong result in many cases
    fTemp2 += 1 / pow(10.0, iPrecision + 1);

    lTemp = (LONGLONG)fTemp2;
    fTemp1 = (double)(lTemp) / pow(10.0, iPrecision);

    // Add the fractional part to the integer part to get the result
    if (fNumber >= 0)
        fResult = lNumber + fTemp1;
    else
        fResult = lNumber - fTemp1;

    return fResult;
}

double RoundDouble(double fNumber, int iPrecision)
{
    double fTemp1, fResult;

    if (iPrecision < 0 || iPrecision > 15 || fabs(fNumber) > 1e18)
        return fNumber;

    // if the required precision is 0, add(or subtract if number < 0) 0.5 to the number, if
    // precision is 1, add 0.05, if it is 2 add 0.005 and so on. Then truncate the resulting
    // number to the required precision to get the result
    fTemp1 = 5.0 / pow(10.0, iPrecision + 1);

    if (fNumber < 0)
        fNumber -= fTemp1;
    else
        fNumber += fTemp1;

    fResult = TruncateDouble(fNumber, iPrecision);

    return fResult;
}

BOOL Char2BOOL(char* c)
{
    if (c && (*c == 'Y' || *c == 'y'))
        return TRUE;
    return FALSE;
}

// Converts a wide-character string (wchar_t*) to a multibyte string (char*).
void wtoc(char* Dest, const wchar_t* Source)
{
    if (!Dest || !Source) return;
    // Use wcstombs_s for safe conversion, matching usage in header
    size_t converted = 0;
    wcstombs_s(&converted, Dest, MAXSQLSIZE, Source, wcslen(Source));
}

// Converts a multibyte string (char*) to a wide-character string (wchar_t*).
void ctow(wchar_t* Dest, const char* Source)
{
    if (!Dest || !Source) return;
    // Use mbstowcs_s for safe conversion
    size_t converted = 0;
    mbstowcs_s(&converted, Dest, MAXSQLSIZE, Source, strlen(Source));
}
#pragma managed(pop)
