/**
 *
 * SUB-SYSTEM: TranProc
 *
 * FILENAME: Commonfuctions.cpp
 *
 * DESCRIPTION:	This is a copy of Commonfunctionc.c file with C++ extension
 * (cpp) No other changes
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 * AUTHOR:
 *
 *
 **/
#include "assets.h"
#include "commonheader.h"
#include "holdings.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <string>

long GetStlDate(HOLDINGS zHold) {
  // If the lot was created using FR and the cost has been adjusted,
  // use effective date else use original settlement date
  if (strcmp(zHold.sOrigTransType, "FR") == 0 &&
      zHold.fTotCost < zHold.fOrigCost)
    return zHold.lEffDate;
  else
    return zHold.lStlDate;
}
BOOL IsValueZero(double fValue, unsigned short iPrecision) {
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

double TruncateDouble(double fNumber, int iPrecision) {
  double fTemp1, fTemp2, fResult;
  LONGLONG lNumber, lTemp;

  if (iPrecision < 0 || iPrecision > 15 || fabs(fNumber) > 1e18)
    return fNumber;

  // get the absolute fractional part of the number
  lNumber = fNumber;
  fTemp1 = fabs(fNumber - lNumber);

  // First multiply the fractional part by 10 ^ Precision and then divide the
  // integer part of the result by 10 ^ Precision. This will give the fractional
  // part of the number upto the required precision
  fTemp2 = fTemp1 * pow(10.0, iPrecision);
  // add a small fraction to the result, this is necessary to avoid wrong result
  // in many cases, e.g. if the number is 100.80 and the required precision is
  // 2, fTemp1 will be 0.79999999998836 and fTemp2 will become 79.999999998836
  // which will be wrong. So after adding 0.0001, the number will
  // become 80.100099998836 which is what we want.
  fTemp2 += 1 / pow(10.0, iPrecision + 1);

  lTemp = fTemp2;
  fTemp1 = (lTemp) / pow(10.0, iPrecision);

  // Add the fractional part to the integer part to get the result
  if (fNumber >= 0)
    fResult = lNumber + fTemp1;
  else
    fResult = lNumber - fTemp1;

  return fResult;
} // TruncateDouble

/*

double TruncateDouble(double fNumber, int iPrecision)
{
        double fTemp1, fTemp2, fResult;

        if (iPrecision < 0 || iPrecision > 15)
                return fNumber;

        // get the absolute fractional part of the number
        fTemp1 = fabs(fNumber - (int)fNumber);

        // First multiply the fractional part by 10 ^ Precision and then divide
the integer part
        // of the result by 10 ^ Precision. This will give the fractional part
of the number upto
        // the required precision
        fTemp2 = fTemp1 * pow(10, iPrecision);
        // add a small fraction to the result, this is necessary to avoid wrong
result in many cases,
        // e.g. if the number is 100.80 and the required precision is 2, fTemp1
will be 0.79999999998836
        // and fTemp2 will become 79.999999998836 which will be wrong. So after
adding 0.0001, the number
        // will become 80.100099998836 which is what we want.
        fTemp2 += 1 / pow(10, iPrecision + 1);

        fTemp1 = ((int)fTemp2) / pow(10, iPrecision);

        // Add the fractional part to the integer part to get the result
        if (fNumber >= 0)
          fResult = (int)fNumber + fTemp1;
        else
                fResult = (int)fNumber - fTemp1;

        return fResult;
} // TruncateDouble
*/
double RoundDouble(double fNumber, int iPrecision) {
  double fTemp1, fResult;

  if (iPrecision < 0 || iPrecision > 15 || fabs(fNumber) > 1e18)
    return fNumber;

  // if the required precision is 0, add(or subtract if number < 0) 0.5 to the
  // number, if precision is 1, add 0.05, if it is 2 add 0.005 and so on. Then
  // truncate the resulting number to the required precision to get the result
  fTemp1 = 5.0 / pow(10.0, iPrecision + 1);

  if (fNumber < 0)
    fNumber -= fTemp1;
  else
    fNumber += fTemp1;

  fResult = TruncateDouble(fNumber, iPrecision);

  return fResult;
}

using std::string;
string ExtractFilePath(const string &aFileName) {
  char sep = '\\';
  size_t i = aFileName.rfind(sep, aFileName.length());
  if (i != string::npos) {
    return aFileName.substr(0, i + 1);
  }
  return "";
}

string ChangeFileExt(LPCSTR appFullName, LPCSTR ext) {
  string fName = appFullName;

  size_t pos = fName.rfind('.', fName.length());
  if (pos != 0)
    fName = fName.replace(pos, 4, ext);
  return fName;
}

HINSTANCE LoadLibrarySafe(LPCTSTR LibFileName) {
  TCHAR appFullName[MAX_PATH] = "";
  HINSTANCE result = 0;
  string appPath;
  TCHAR libPath[MAX_PATH] = "";

  GetModuleFileName(NULL, appFullName, MAX_PATH);
  GetPrivateProfileString("GENERAL", "CommonLibPath", "", libPath, MAX_PATH,
                          ChangeFileExt(appFullName, ".ini").c_str());
  if (libPath[0] == '\0')
    appPath = ExtractFilePath(appFullName);
  else
    appPath = libPath;
  if (appPath.length() > 0) {
    if (appPath.back() != '\\')
      appPath += '\\';
    appPath += LibFileName;
    result = LoadLibrary(appPath.c_str());
  }
  return result;
}

// 11/14/2001 vay - new way - still has rounding errors
/*
double RoundDouble(double fNumber, int iPrecision)
{
        char sTemp[50];
        char sInt[50];
        char sFmt[8];
        char *sStop;
        int pos;
        LONGLONG iTemp;
        bool bForcedLeading0 = false;

        double fResult;

        fResult = fabs(fNumber);

        if (iPrecision<5)
          strcpy_s(sFmt, "%.7f");
        else {
          strcpy_s(sFmt, "%.");
          itoa(iPrecision+7, sTemp,10);
          strcat_s(sFmt, sTemp);
          strcat_s(sFmt, "f\0");
        }

        memset(sInt,0,sizeof(sInt));

        sprintf(sTemp, sFmt, fResult);
        if (sTemp[0]=='0') {
                sTemp[0]='1';
                bForcedLeading0 = true;
        }

        sStop = strchr(sTemp, '.');
        pos = sStop - sTemp;

        strncat(sInt, sTemp, pos);
        sStop++;
        strncat(sInt, sStop, iPrecision);
        sStop += iPrecision;

        iTemp = _atoi64(sInt);
        if (*sStop>='5') iTemp++;

        _i64toa(iTemp, sInt, 10);

        sStop = sInt+strlen(sInt);
        sStop -= iPrecision;

        memset(sTemp,0,sizeof(sTemp));
        strncat(sTemp, sInt, pos);
        sTemp[pos] ='.';
        strncat(sTemp, sStop, iPrecision);

        if (bForcedLeading0) {
          sTemp[0] = sTemp[0]-1;
        }

    fResult = strtod(sTemp, &sStop);
        fResult = _copysign(fResult, fNumber);

        return fResult;
} // RoundDouble
*/
