/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Holdtot.cpp
 * DESCRIPTION: Holdtot implementations (2 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "PerformanceIO_Holdtot.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "holdtot.h"
#include <string>
#include <regex>

extern thread_local nanodbc::connection gConn;
extern char sHoldtot[]; // Global table name defined in OLEDBIO.cpp

// Helper to replace table name placeholder
static std::string ReplaceHoldtotTableName(const std::string& sql)
{
    // If sHoldtot is empty or default, we might want to fallback or error, 
    // but legacy code assumes it's set.
    std::string tableName = (sHoldtot[0] != '\0') ? sHoldtot : "HoldTot";
    
    std::string result = sql;
    size_t pos = result.find("%HOLDTOT_TABLE_NAME%");
    if (pos != std::string::npos)
    {
        result.replace(pos, 20, tableName);
    }
    return result;
}

// ============================================================================
// DeleteHoldtotForAnAccount
// ============================================================================
DLLAPI void STDCALL DeleteHoldtotForAnAccount(int iID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteHoldtotForAnAccount", FALSE);
        return;
    }

    try
    {
        std::string sql = "Delete from %HOLDTOT_TABLE_NAME% Where ID=?";
        sql = ReplaceHoldtotTableName(sql);

        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql));
        stmt.bind(0, &iID);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteHoldtotForAnAccount", FALSE);
    }
}

// ============================================================================
// InsertHoldtot
// ============================================================================
DLLAPI void STDCALL InsertHoldtot(HOLDTOT zHT, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertHoldtot", FALSE);
        return;
    }

    try
    {
        std::string sql = 
            "INSERT INTO %HOLDTOT_TABLE_NAME% "
            "(id, segmenttype_id, scrhdr_no, native_orig_cost, "
            "native_tot_cost, native_market_value, native_accrual, "
            "native_gainorloss, native_accr_gorl, native_curr_gorl, "
            "native_sec_gorl, native_cost_yield, native_curr_yield, "
            "native_avg_price_change, native_wtdavg_change, native_income, "
            "native_parvalue, base_orig_cost, base_tot_cost, "
            "base_market_value, base_accrual, base_gainorloss, "
            "base_accr_gorl, base_curr_gorl, base_sec_gorl, "
            "base_cost_yield, base_curr_yield, base_avg_price_change, "
            "base_wtdavg_change, base_income, base_parvalue, "
            "system_orig_cost, system_tot_cost, system_market_value, "
            "system_accrual, system_gainorloss, system_accr_gorl, "
            "system_curr_gorl, system_sec_gorl, system_cost_yield, "
            "system_curr_yield, system_avg_price_change, system_wtdavg_change, "
            "system_income, system_parvalue, percent_totmv, "
            "coupon_rate, current_yieldtomaturity, maturity, "
            "duration, rating, numberofsecurities, periodreturn, "
            "PctEPS1Yr, PctEPS5Yr, Beta, PriceBook, PriceSales, "
            "Trail12mPE, Proj12mPE, AvgWtdCap, MedianCap, "
            "PcTopTenHold, ROE, DivYield, maturity_real "
            ") VALUES ( "
            "?, ?, ?, ?,                 ?, ?, ?,             ?, ?, ?, "
            "?, ?, ?,                   ?, ?, ?,                ?, ?, ?, "
            "?, ?, ?,                   ?, ?, ?,                ?, ?, ?, "
            "?, ?, ?,                   ?, ?, ?,                ?, ?, ?, "
            "?, ?, ?,                   ?, ?, ?,                ?, ?, ?, "
            "?, ?, ?,                   ?, ?, ?, ?,             ?, ?, ?, ?, ?, "
            "?, ?, ?, ?,                ?, ?, ?, ?)";

        sql = ReplaceHoldtotTableName(sql);

        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql));

        // Round rating as per legacy code: cmdInsertHoldtot.m_zHoldtot.fRating = RoundDouble(zHT.fRating, 0);
        // We need to include RoundDouble declaration or implementation.
        // It's likely in commonheader.h or similar.
        // Assuming it's available or we can use standard round.
        // Legacy code: RoundDouble(zHT.fRating, 0) -> round to 0 decimals.
        double fRatingRounded = round(zHT.fRating); 

        int idx = 0;
        stmt.bind(idx++, &zHT.iId);
        stmt.bind(idx++, &zHT.iSegmentTypeID);
        stmt.bind(idx++, &zHT.iScrhdrNo);
        stmt.bind(idx++, &zHT.fNativeOrigCost);

        stmt.bind(idx++, &zHT.fNativeTotCost);
        stmt.bind(idx++, &zHT.fNativeMarketValue);
        stmt.bind(idx++, &zHT.fNativeAccrual);

        stmt.bind(idx++, &zHT.fNativeGainorloss);
        stmt.bind(idx++, &zHT.fNativeAccrGorl);
        stmt.bind(idx++, &zHT.fNativeCurrGorl);

        stmt.bind(idx++, &zHT.fNativeSecGorl);
        stmt.bind(idx++, &zHT.fNativeCostYield);
        stmt.bind(idx++, &zHT.fNativeCurrYield);

        stmt.bind(idx++, &zHT.fNativeAvgPriceChange);
        stmt.bind(idx++, &zHT.fNativeWtdavgChange);
        stmt.bind(idx++, &zHT.fNativeIncome);

        stmt.bind(idx++, &zHT.fNativeParvalue);
        stmt.bind(idx++, &zHT.fBaseOrigCost);
        stmt.bind(idx++, &zHT.fBaseTotCost);

        stmt.bind(idx++, &zHT.fBaseMarketValue);
        stmt.bind(idx++, &zHT.fBaseAccrual);
        stmt.bind(idx++, &zHT.fBaseGainorloss);

        stmt.bind(idx++, &zHT.fBaseAccrGorl);
        stmt.bind(idx++, &zHT.fBaseCurrGorl);
        stmt.bind(idx++, &zHT.fBaseSecGorl);

        stmt.bind(idx++, &zHT.fBaseCostYield);
        stmt.bind(idx++, &zHT.fBaseCurrYield);
        stmt.bind(idx++, &zHT.fBaseAvgPriceChange);

        stmt.bind(idx++, &zHT.fBaseWtdavgChange);
        stmt.bind(idx++, &zHT.fBaseIncome);
        stmt.bind(idx++, &zHT.fBaseParvalue);

        stmt.bind(idx++, &zHT.fSystemOrigCost);
        stmt.bind(idx++, &zHT.fSystemTotCost);
        stmt.bind(idx++, &zHT.fSystemMarketValue);

        stmt.bind(idx++, &zHT.fSystemAccrual);
        stmt.bind(idx++, &zHT.fSystemGainorloss);
        stmt.bind(idx++, &zHT.fSystemAccrGorl);

        stmt.bind(idx++, &zHT.fSystemCurrGorl);
        stmt.bind(idx++, &zHT.fSystemSecGorl);
        stmt.bind(idx++, &zHT.fSystemCostYield);

        stmt.bind(idx++, &zHT.fSystemCurrYield);
        stmt.bind(idx++, &zHT.fSystemAvgPriceChange);
        stmt.bind(idx++, &zHT.fSystemWtdavgChange);

        stmt.bind(idx++, &zHT.fSystemIncome);
        stmt.bind(idx++, &zHT.fSystemParvalue);
        stmt.bind(idx++, &zHT.fPercentTotmv);

        stmt.bind(idx++, &zHT.fCouponRate);
        stmt.bind(idx++, &zHT.fCurrentYieldtomaturity);
        stmt.bind(idx++, &zHT.fMaturity);

        stmt.bind(idx++, &zHT.fDuration);
        stmt.bind(idx++, &fRatingRounded); // Use rounded value
        stmt.bind(idx++, &zHT.iNumberofsecurities);
        stmt.bind(idx++, &zHT.fPeriodreturn);

        stmt.bind(idx++, &zHT.fPctEPS1Yr);
        stmt.bind(idx++, &zHT.fPctEPS5Yr);
        stmt.bind(idx++, &zHT.fBeta);
        stmt.bind(idx++, &zHT.fPriceBook);
        stmt.bind(idx++, &zHT.fPriceSales);

        stmt.bind(idx++, &zHT.fTrail12mPE);
        stmt.bind(idx++, &zHT.fProj12mPE);
        stmt.bind(idx++, &zHT.fAvgWtdCap);
        stmt.bind(idx++, &zHT.fMedianCap);

        stmt.bind(idx++, &zHT.fPcTopTenHold);
        stmt.bind(idx++, &zHT.fROE);
        stmt.bind(idx++, &zHT.fDivYield);
        stmt.bind(idx++, &zHT.fMaturityReal);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), zHT.iId, 0, (char*)"", 0, -1, 0, (char*)"InsertHoldtot", FALSE);
    }
}
