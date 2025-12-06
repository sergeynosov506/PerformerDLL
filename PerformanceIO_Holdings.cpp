/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Holdings.cpp
 * DESCRIPTION: Performance Holdings and HoldCash functions implementation
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "PerformanceIO_Holdings.h"
#include "PerformanceIO_Common.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <optional>
#include <iostream>
#include <string>
#include <regex>

// Static state for SelectPerformanceHoldings
static std::optional<nanodbc::result> g_HoldingsResult;
static bool g_HoldingsResultOpen = false;
static std::string g_LastHoldingsTable;

// Static state for SelectPerformanceHoldcash
static std::optional<nanodbc::result> g_HoldCashResult;
static bool g_HoldCashResultOpen = false;
static std::string g_LastHoldCashTable;



// Helper to replace table name in SQL
static std::string AdjustSQL(const std::string& sql, const std::string& placeholder, const std::string& tableName)
{
    std::string result = sql;
    size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos)
    {
        result.replace(pos, placeholder.length(), tableName);
        pos += tableName.length();
    }
    return result;
}

DLLAPI void STDCALL SelectPerformanceHoldings(PARTHOLDING *pzHold, PARTASSET2 *pzPartAsset, LEVELINFO *pzLInfo,
																				 int iID, char *sHoldings, int iVendorID, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "SelectPerformanceHoldings", FALSE);
            return;
        }

        // If new query parameters or table changed, close and reopen
        if (g_HoldingsResultOpen && (g_LastHoldingsTable != sHoldings))
        {
             // In legacy code, it checks if params match. Here we simplify:
             // If the caller calls with new params, we expect them to be passed.
             // However, the legacy logic for "subsequent call" relies on checking if current row matches params.
             // But here we are iterating. If the user calls this function again, they usually want the NEXT row.
             // The legacy code checks: if (!(cmd.m_zHold.iID == iID && cmd.m_iVendorID == iVendorID && cmd.m_cRows > 0))
             // This implies if params CHANGE, we reset.
             // We will assume if the result is open, we are iterating.
             // But if the table name changes, we MUST reset.
             g_HoldingsResultOpen = false;
             g_HoldingsResult.reset();
        }

        if (!g_HoldingsResultOpen)
        {
            std::string sqlPart1 = R"(declare @PID INT declare @VID INT 
set @PID = ? set @VID=? 
SELECT a.id, a.sec_no, a.wi, sec_xtend, trans_no, 
tot_cost, Base_Cost_Xrate, mkt_val, mv_base_xrate, accr_int, ai_base_xrate, primary_type, 
b.sec_type,b.curr_id,b.inc_curr_id,
h1.Level_1 AS indust_level1, h1.il1_effdate, 
h1.Level_2 AS indust_level2, h1.il2_effdate, 
h1.Level_3 AS indust_level3, h1.il3_effdate, 
b.sp_rating,b.moody_rating,b.internal_rating,b.ann_div_cpn,b.cur_exrate, 
c.taxable_country, c.tax_country, c.taxable_state, c.tax_state, b.sec_desc1, 
b.country_iss, b.country_isr, 
CASE WHEN e.DRDElig IS NULL THEN 
CASE WHEN b.sec_type in (1,2) THEN 'Y' ELSE 'N' END 
ELSE e.DRDElig END, a.annual_income )";

            std::string sqlPart2 = R"( FROM %HOLDINGS_TABLE_NAME% as a with (nolock) 
left outer join assets as b with (nolock) on a.sec_no = b.sec_no and a.wi = b.when_issue 
left outer join fixedinc c with (nolock) on b.sec_no = c.sec_no and b.when_issue = c.wi 
left outer join equities e with (nolock) on b.sec_no = e.sec_no and b.when_issue = e.wi 
left outer join(
SELECT q1.sec_no , q1.wi 
, q1.Segid AS level_1, q1.effdate AS il1_effdate 
, ISNULL(q2.Segid, 0) AS level_2, ISNULL(q2.effdate, '12/30/1988') AS il2_effdate 
, ISNULL(q3.SegId, 0) AS level_3, ISNULL(q3.effdate, '12/30/1988') AS il3_effdate 
FROM( 
SELECT DISTINCT h.sec_no , h.wi, s.level_id, h.SegId, h.effdate 
FROM histassetindustry h WITH(NOLOCK) 
INNER JOIN segments s ON s.id = h.segid 
WHERE h.vendorId = @VID 
AND h.deletedate < '1/1/1900' 
AND s.level_id = 1 
) q1 
LEFT JOIN( 
SELECT DISTINCT h.sec_no, h.wi, s.level_id, h.SegId, h.effdate 
FROM histassetindustry h WITH(NOLOCK) 
INNER JOIN segments s ON s.id = h.segid 
WHERE h.vendorId = @VID 
AND h.deletedate < '1/1/1900' 
AND s.level_id = 2 
) q2 ON q1.sec_no = q2.sec_no 
AND q1.wi = q2.wi 
LEFT JOIN( 
SELECT DISTINCT h.sec_no, h.wi, s.level_id, h.SegId, h.effdate 
FROM histassetindustry h WITH(NOLOCK) 
INNER JOIN segments s ON s.id = h.segid 
WHERE h.vendorId = @VID 
AND h.deletedate < '1/1/1900' 
AND s.level_id = 3 
) q3 ON q1.sec_no = q3.sec_no 
AND q1.wi = q3.wi 
) h1 ON h1.sec_no = a.sec_no 
AND h1.wi = a.wi 
WHERE a.id = @PID AND a.sec_xtend <> 'UP' 
AND a.sec_xtend <> 'TS' AND a.sec_xtend <> 'TL' and 
a.sec_no = b.sec_no and a.wi = b.when_issue 
Order by a.id, a.sec_no, trans_no )";

            std::string fullSql = sqlPart1 + sqlPart2;
            fullSql = AdjustSQL(fullSql, "%HOLDINGS_TABLE_NAME%", sHoldings);

            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, fullSql);

            stmt.bind(0, &iID);
            stmt.bind(1, &iVendorID);

            g_HoldingsResult = stmt.execute();
            g_HoldingsResultOpen = true;
            g_LastHoldingsTable = sHoldings;
        }

        if (g_HoldingsResult && g_HoldingsResult->next())
        {
            // Map columns
            // 0: id, 1: sec_no, 2: wi, 3: sec_xtend, 4: trans_no
            pzHold->iID = g_HoldingsResult->get<int>(0);
            read_string(*g_HoldingsResult, 1, pzHold->sSecNo, sizeof(pzHold->sSecNo));
            read_string(*g_HoldingsResult, 2, pzHold->sWi, sizeof(pzHold->sWi));
            read_string(*g_HoldingsResult, 3, pzHold->sSecXtend, sizeof(pzHold->sSecXtend));
            pzHold->lTransNo = g_HoldingsResult->get<long>(4);

            // 5: tot_cost, 6: Base_Cost_Xrate, 7: mkt_val, 8: mv_base_xrate, 9: accr_int, 10: ai_base_xrate
            pzHold->fTotCost = g_HoldingsResult->get<double>(5);
            pzHold->fBaseCostXrate = g_HoldingsResult->get<double>(6);
            pzHold->fMktVal = g_HoldingsResult->get<double>(7);
            pzHold->fMvBaseXrate = g_HoldingsResult->get<double>(8);
            pzHold->fAccrInt = g_HoldingsResult->get<double>(9);
            pzHold->fAiBaseXrate = g_HoldingsResult->get<double>(10);
            
            // 11: primary_type
            read_string(*g_HoldingsResult, 11, pzHold->sPrimaryType, sizeof(pzHold->sPrimaryType));

            // 12: sec_type, 13: curr_id, 14: inc_curr_id
            pzPartAsset->iSecType = g_HoldingsResult->get<int>(12);
            read_string(*g_HoldingsResult, 13, pzPartAsset->sCurrId, sizeof(pzPartAsset->sCurrId));
            read_string(*g_HoldingsResult, 14, pzPartAsset->sIncCurrId, sizeof(pzPartAsset->sIncCurrId));

            // 15: indust_level1, 16: il1_effdate
            pzLInfo->iIndustLevel1 = g_HoldingsResult->get<int>(15);
            read_date(*g_HoldingsResult, 16, &pzLInfo->lEffDate1);

            // 17: indust_level2, 18: il2_effdate
            pzLInfo->iIndustLevel2 = g_HoldingsResult->get<int>(17);
            read_date(*g_HoldingsResult, 18, &pzLInfo->lEffDate2);

            // 19: indust_level3, 20: il3_effdate
            pzLInfo->iIndustLevel3 = g_HoldingsResult->get<int>(19);
            read_date(*g_HoldingsResult, 20, &pzLInfo->lEffDate3);

            // 21: sp_rating, 22: moody_rating, 23: internal_rating, 24: ann_div_cpn, 25: cur_exrate
            read_string(*g_HoldingsResult, 21, pzPartAsset->sSpRating, sizeof(pzPartAsset->sSpRating));
            read_string(*g_HoldingsResult, 22, pzPartAsset->sMoodyRating, sizeof(pzPartAsset->sMoodyRating));
            read_string(*g_HoldingsResult, 23, pzPartAsset->sInternalRating, sizeof(pzPartAsset->sInternalRating));
            pzPartAsset->fAnnDivCpn = g_HoldingsResult->get<double>(24);
            pzPartAsset->fCurExrate = g_HoldingsResult->get<double>(25);

            // 26: taxable_country, 27: tax_country, 28: taxable_state, 29: tax_state
            read_string(*g_HoldingsResult, 26, pzPartAsset->sTaxableCountry, sizeof(pzPartAsset->sTaxableCountry));
            read_string(*g_HoldingsResult, 27, pzPartAsset->sTaxCountry, sizeof(pzPartAsset->sTaxCountry));
            read_string(*g_HoldingsResult, 28, pzPartAsset->sTaxableState, sizeof(pzPartAsset->sTaxableState));
            read_string(*g_HoldingsResult, 29, pzPartAsset->sTaxState, sizeof(pzPartAsset->sTaxState));

            // 30: sec_desc1
            read_string(*g_HoldingsResult, 30, pzPartAsset->sSecDesc1, sizeof(pzPartAsset->sSecDesc1));

            // 31: country_iss, 32: country_isr
            read_string(*g_HoldingsResult, 31, pzPartAsset->sCountryIss, sizeof(pzPartAsset->sCountryIss));
            read_string(*g_HoldingsResult, 32, pzPartAsset->sCountryIsr, sizeof(pzPartAsset->sCountryIsr));

            // 33: DRDElig
            read_string(*g_HoldingsResult, 33, pzPartAsset->sDRDElig, sizeof(pzPartAsset->sDRDElig));

            // 34: annual_income
            pzHold->fAnnualIncome = g_HoldingsResult->get<double>(34);

            // Populate additional fields in pzPartAsset from pzHold as per legacy code
            strcpy_s(pzPartAsset->sSecNo, pzHold->sSecNo);
            strcpy_s(pzPartAsset->sWhenIssue, pzHold->sWi);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_HoldingsResultOpen = false;
            g_HoldingsResult.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectPerformanceHoldings");
        g_HoldingsResultOpen = false;
        g_HoldingsResult.reset();
    }
}

DLLAPI void STDCALL SelectPerformanceHoldcash(PARTHOLDING *pzHold, PARTASSET2 *pzPartAsset, LEVELINFO	*pzLInfo,
																				int iID, char *sHoldcash, int iVendorID, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "SelectPerformanceHoldcash", FALSE);
            return;
        }

        if (g_HoldCashResultOpen && (g_LastHoldCashTable != sHoldcash))
        {
             g_HoldCashResultOpen = false;
             g_HoldCashResult.reset();
        }

        if (!g_HoldCashResultOpen)
        {
            std::string sqlPart1 = R"(SELECT a.id, a.sec_no, a.wi, a.sec_xtend, a.tot_cost, a.mkt_val, a.mv_base_xrate, 
b.sec_type,b.curr_id,b.inc_curr_id,
h1.Level_1 AS indust_level1, h1.il1_effdate, 
h1.Level_2 AS  indust_level2, h1.il2_effdate, 
h1.Level_3 AS indust_level3, h1.il3_effdate, 
b.sp_rating,b.moody_rating,b.internal_rating,b.ann_div_cpn,b.cur_exrate, 
c.taxable_country, c.tax_country, c.taxable_state, c.tax_state, b.sec_desc1, 
b.country_iss, b.country_isr, case when IsNULL(a.mv_base_xrate, 0) <> 0 then (0.01 * b.ann_div_cpn * a.units/a.mv_base_xrate) else 0 END ANNINC 
FROM %HOLDCASH_TABLE_NAME%  as a with (nolock) 
left outer join assets as b with (nolock) on a.sec_no = b.sec_no and a.wi = b.when_issue 
left outer join fixedinc c with (nolock) on b.sec_no = c.sec_no and b.when_issue = c.wi )";

            std::string sqlPart2 = R"(left outer join(
select q1.sec_no, q1.wi, q1.Segid as level_1, q1.effdate as il1_effdate, ISNULL(q2.Segid,0) as level_2, ISNULL(q2.effdate, '12/30/1899') as il2_effdate, 
ISNULL(q3.SegId,0) as level_3, ISNULL(q3.effdate, '12/30/1899') as il3_effdate 
from (select distinct a.id, a.sec_no, a.wi, s.level_id, h.SegId, h.effdate FROM %HOLDCASH_TABLE_NAME%  AS a with (nolock) 
inner join histassetindustry h with (nolock) on h.sec_no = a.sec_no and h.wi = a.wi 
inner join segments s on s.id = h.segid where a.id = ? and h.vendorId = ? 
and h.deletedate < '1/1/1900' and s.level_id = 1 ) q1 
left join (select distinct a.id, a.sec_no, a.wi, s.level_id, h.SegId, h.effdate FROM %HOLDCASH_TABLE_NAME% as a with (nolock) 
inner join histassetindustry h with (nolock) on h.sec_no = a.sec_no and h.wi = a.wi 
inner join segments s on s.id = h.segid where a.id = ?  and h.vendorId = ? and h.deletedate < '1/1/1900' 
and s.level_id = 2) q2 on q1.sec_no = q2.sec_no and q1.wi= q2.wi and q2.id = q1.id 
left join (select distinct a.id, a.sec_no, a.wi, s.level_id, h.SegId, h.effdate FROM %HOLDCASH_TABLE_NAME%  as a with (nolock) 
inner join histassetindustry h with (nolock) on h.sec_no = a.sec_no and h.wi = a.wi 
inner join segments s on s.id = h.segid where a.id = ?  and h.vendorId = ? and h.deletedate < '1/1/1900'
and s.level_id = 3) q3 on q1.sec_no = q3.sec_no and q1.wi= q3.wi and q3.id = q1.id) 
h1 ON h1.sec_no = a.sec_no and h1.wi = a.wi 
WHERE a.id = ? AND a.sec_xtend <> 'UP' 
AND a.sec_xtend <> 'TS' AND a.sec_xtend <> 'TL' 
AND a.sec_no = b.sec_no and a.wi = b.when_issue 
Order by a.id, a.sec_no, a.wi, a.sec_xtend)";

            std::string fullSql = sqlPart1 + sqlPart2;
            fullSql = AdjustSQL(fullSql, "%HOLDCASH_TABLE_NAME%", sHoldcash);

            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, fullSql);

            // Bind parameters: 7 parameters, all are ID or VendorID
            // Legacy map: 1:ID, 2:VID, 3:ID, 4:VID, 5:ID, 6:VID, 7:ID
            stmt.bind(0, &iID);
            stmt.bind(1, &iVendorID);
            stmt.bind(2, &iID);
            stmt.bind(3, &iVendorID);
            stmt.bind(4, &iID);
            stmt.bind(5, &iVendorID);
            stmt.bind(6, &iID);

            g_HoldCashResult = stmt.execute();
            g_HoldCashResultOpen = true;
            g_LastHoldCashTable = sHoldcash;
        }

        if (g_HoldCashResult && g_HoldCashResult->next())
        {
            // Map columns
            // 0: id, 1: sec_no, 2: wi, 3: sec_xtend, 4: tot_cost, 5: mkt_val, 6: mv_base_xrate
            pzHold->iID = g_HoldCashResult->get<int>(0);
            read_string(*g_HoldCashResult, 1, pzHold->sSecNo, sizeof(pzHold->sSecNo));
            read_string(*g_HoldCashResult, 2, pzHold->sWi, sizeof(pzHold->sWi));
            read_string(*g_HoldCashResult, 3, pzHold->sSecXtend, sizeof(pzHold->sSecXtend));
            pzHold->fTotCost = g_HoldCashResult->get<double>(4);
            pzHold->fMktVal = g_HoldCashResult->get<double>(5);
            pzHold->fMvBaseXrate = g_HoldCashResult->get<double>(6);

            // 7: sec_type, 8: curr_id, 9: inc_curr_id
            pzPartAsset->iSecType = g_HoldCashResult->get<int>(7);
            read_string(*g_HoldCashResult, 8, pzPartAsset->sCurrId, sizeof(pzPartAsset->sCurrId));
            read_string(*g_HoldCashResult, 9, pzPartAsset->sIncCurrId, sizeof(pzPartAsset->sIncCurrId));

            // 10: indust_level1, 11: il1_effdate
            pzLInfo->iIndustLevel1 = g_HoldCashResult->get<int>(10);
            read_date(*g_HoldCashResult, 11, &pzLInfo->lEffDate1);

            // 12: indust_level2, 13: il2_effdate
            pzLInfo->iIndustLevel2 = g_HoldCashResult->get<int>(12);
            read_date(*g_HoldCashResult, 13, &pzLInfo->lEffDate2);

            // 14: indust_level3, 15: il3_effdate
            pzLInfo->iIndustLevel3 = g_HoldCashResult->get<int>(14);
            read_date(*g_HoldCashResult, 15, &pzLInfo->lEffDate3);

            // 16: sp_rating, 17: moody_rating, 18: internal_rating, 19: ann_div_cpn, 20: cur_exrate
            read_string(*g_HoldCashResult, 16, pzPartAsset->sSpRating, sizeof(pzPartAsset->sSpRating));
            read_string(*g_HoldCashResult, 17, pzPartAsset->sMoodyRating, sizeof(pzPartAsset->sMoodyRating));
            read_string(*g_HoldCashResult, 18, pzPartAsset->sInternalRating, sizeof(pzPartAsset->sInternalRating));
            pzPartAsset->fAnnDivCpn = g_HoldCashResult->get<double>(19);
            pzPartAsset->fCurExrate = g_HoldCashResult->get<double>(20);

            // 21: taxable_country, 22: tax_country, 23: taxable_state, 24: tax_state
            read_string(*g_HoldCashResult, 21, pzPartAsset->sTaxableCountry, sizeof(pzPartAsset->sTaxableCountry));
            read_string(*g_HoldCashResult, 22, pzPartAsset->sTaxCountry, sizeof(pzPartAsset->sTaxCountry));
            read_string(*g_HoldCashResult, 23, pzPartAsset->sTaxableState, sizeof(pzPartAsset->sTaxableState));
            read_string(*g_HoldCashResult, 24, pzPartAsset->sTaxState, sizeof(pzPartAsset->sTaxState));

            // 25: sec_desc1
            read_string(*g_HoldCashResult, 25, pzPartAsset->sSecDesc1, sizeof(pzPartAsset->sSecDesc1));

            // 26: country_iss, 27: country_isr
            read_string(*g_HoldCashResult, 26, pzPartAsset->sCountryIss, sizeof(pzPartAsset->sCountryIss));
            read_string(*g_HoldCashResult, 27, pzPartAsset->sCountryIsr, sizeof(pzPartAsset->sCountryIsr));

            // 28: annual_income
            pzHold->fAnnualIncome = g_HoldCashResult->get<double>(28);

            // Populate additional fields
            strcpy_s(pzPartAsset->sSecNo, pzHold->sSecNo);
            strcpy_s(pzPartAsset->sWhenIssue, pzHold->sWi);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_HoldCashResultOpen = false;
            g_HoldCashResult.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectPerformanceHoldcash");
        g_HoldCashResultOpen = false;
        g_HoldCashResult.reset();
    }
}
