#include "TransIO_Sectype.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "assets.h"
#include "TransIO.h" // For SelectAsset and zSTable
#include <iostream>
#include "dateutils.h"


extern thread_local nanodbc::connection gConn;
extern SECTYPETABLE zSTable;

// Function to find given sectype in the table in memory
int FindSecType(int iSecType)
{
  int i, Res = -1;

  for (i=0; i <= zSTable.iNumSType - 1; i++)
    if (zSTable.zSType[i].iSecType == iSecType) 
    {
      Res = i;
      break;
	}

  return Res;	
} // FindSecType

// Function to add passed SecType to the memory table
void AddSecType(SECTYPE zSecType)
{
  if (zSTable.iNumSType == NUMSECTYPES) 
    PrintError((char*)"Sectype Table is Too Small", 0, 0, (char*)"", 999, 0, 0, (char*)"OLEDBIO ADDSECTYPE", FALSE);
  else 
  {
    memcpy(&zSTable.zSType[zSTable.iNumSType], &zSecType, sizeof(zSecType));
    zSTable.iNumSType++;
  }
} //AddSecType

// Helper to fill SECTYPE struct from result
void FillSectypeStruct(nanodbc::result& result, SECTYPE* pzSectype)
{
    pzSectype->iSecType = result.get<int>("sec_type", 0);
    read_string(result, "sec_type_desc", pzSectype->sSecTypeDesc, sizeof(pzSectype->sSecTypeDesc));
    read_string(result, "primary_type", pzSectype->sPrimaryType, sizeof(pzSectype->sPrimaryType));
    read_string(result, "secondary_type", pzSectype->sSecondaryType, sizeof(pzSectype->sSecondaryType));
    read_string(result, "third_type", pzSectype->sThirdType, sizeof(pzSectype->sThirdType));
    
    pzSectype->iStlDays = result.get<int>("stl_days", 0);
    read_string(result, "sec_fee_flag", pzSectype->sSecFeeFlag, sizeof(pzSectype->sSecFeeFlag));
    read_string(result, "pay_type", pzSectype->sPayType, sizeof(pzSectype->sPayType));
    pzSectype->iAccrualSched = result.get<int>("accrual_sched", 0);
    
    read_string(result, "commission_flag", pzSectype->sCommissionFlag, sizeof(pzSectype->sCommissionFlag));
    read_string(result, "position_ind", pzSectype->sPositionInd, sizeof(pzSectype->sPositionInd));
    read_string(result, "lot_ind", pzSectype->sLotInd, sizeof(pzSectype->sLotInd));
    read_string(result, "cost_ind", pzSectype->sCostInd, sizeof(pzSectype->sCostInd));
    
    read_string(result, "lot_exists_ind", pzSectype->sLotExistsInd, sizeof(pzSectype->sLotExistsInd));
    read_string(result, "avg_ind", pzSectype->sAvgInd, sizeof(pzSectype->sAvgInd));
    read_string(result, "mktval_ind", pzSectype->sMktValInd, sizeof(pzSectype->sMktValInd));
    read_string(result, "screen_ind", pzSectype->sScreenInd, sizeof(pzSectype->sScreenInd));
    
    read_string(result, "indust_level", pzSectype->sIndustLevel, sizeof(pzSectype->sIndustLevel));
    read_string(result, "yield", pzSectype->sYield, sizeof(pzSectype->sYield));
    pzSectype->iIntcalc = result.get<int>("intcalc", 0);
}

DLLAPI void STDCALL SelectSectype(SECTYPE *pzSectype, short iSecType, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    int i = FindSecType(iSecType);
    if (i >= 0) 
    {
        memcpy(pzSectype, &zSTable.zSType[i], sizeof(zSTable.zSType[i]));
        return;
    }

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", iSecType, 0, (char*)"", 0, -1, 0, (char*)"SelectSectype", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "Select sec_type, "
            "sec_type_desc, primary_type, secondary_type, third_type, "
            "stl_days, sec_fee_flag, pay_type, accrual_sched, "
            "commission_flag, position_ind, lot_ind, cost_ind, "
            "lot_exists_ind, avg_ind, mktval_ind, screen_ind, "
            "indust_level, yield, intcalc "
            "From Sectype Where sec_type = ? "));

        stmt.bind(0, &iSecType);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            memset(pzSectype, 0, sizeof(*pzSectype));
            FillSectypeStruct(result, pzSectype);
            AddSecType(*pzSectype);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectSectype: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iSecType, 0, (char*)"", 0, -1, 0, (char*)"SelectSectype", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectSectype", iSecType, 0, (char*)"", 0, -1, 0, (char*)"SelectSectype", FALSE);
    }
}

DLLAPI void STDCALL SecurityCharacteristics(char *sPrimaryType, char *sSecondaryType, char *sPositionInd, char *sLotInd,
									   char *sCostInd, char *sLotExistsInd, char *sAvgInd, double *pfTradUnit,
									   char *sCurrId, char *sSecNo, char *sWhenIssue, ERRSTRUCT *pzErr)
{
    ASSETS zAsset;
    SECTYPE zSType;

    InitializeErrStruct(pzErr);

    SelectAsset(&zAsset, sSecNo, sWhenIssue, -1, pzErr);
    if (pzErr->iSqlError != 0 || pzErr->iBusinessError != 0) 
    {
        if (pzErr->iSqlError == SQLNOTFOUND) 
            *pzErr = PrintError((char*)"Security Does Not Exist in Assets", 0, 0, sSecNo, 8, pzErr->iSqlError, pzErr->iIsamCode, (char*)"SecurityCharachteristics", TRUE);
        return;
    }
    else 
    {
        SelectSectype(&zSType, zAsset.iSecType, pzErr);
        if (pzErr->iSqlError != 0 || pzErr->iBusinessError != 0) 
        {
            if (pzErr->iSqlError == SQLNOTFOUND) 
                *pzErr = PrintError((char*)"Type Does Not Exist in SecType", zAsset.iSecType, 0, (char*)"", 18, pzErr->iSqlError, pzErr->iIsamCode, (char*)"SecurityCharachteristics", TRUE);
            return;
        }
    }

    strcpy_s(sPrimaryType, 2, zSType.sPrimaryType);
    strcpy_s(sSecondaryType, 2, zSType.sSecondaryType);
    strcpy_s(sPositionInd, 2, zSType.sPositionInd);
    strcpy_s(sLotInd, 2, zSType.sLotInd);
    strcpy_s(sCostInd, 2, zSType.sCostInd);
    strcpy_s(sLotExistsInd, 2, zSType.sLotExistsInd);
    strcpy_s(sAvgInd, 2, zSType.sAvgInd);
    *pfTradUnit = zAsset.fTradUnit;
    strcpy_s(sCurrId, 5, zAsset.sCurrId);
}
