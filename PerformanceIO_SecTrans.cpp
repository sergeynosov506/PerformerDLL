#include "PerformanceIO_SecTrans.h"
#include "PerformanceIO_Common.h"
#include "dateutils.h"
#include <iostream>
#include "nanodbc/nanodbc.h"



// Helper to construct the complex SQL query for SelectPerformanceTransaction
// Preserving the exact structure of the original legacy code's SQL construction
std::string BuildPerformanceTransactionSQL()
{
    std::string sql;
    
    // Part 1: Declare variables and initial SELECT
    sql += R"(
declare		@PID INT 
declare		@VID INT 
declare		@D1 DATETIME 
declare		@d2 DATETIME 
set @PID = ? 
set @VID = ? 
set @D1 = ? 
set @D2 = ? 
SELECT a.id,a.trans_no,a.tran_type,a.sec_no, a.wi,
a.sec_xtend,a.acct_type, a.pcpl_amt, a.accr_int,
a.income_amt,a.net_flow,a.trd_date,a.stl_date,
a.entry_date,a.rev_type,a.perf_date,a.x_sec_no,a.x_wi,
a.x_sec_xtend,a.x_acct_type,a.curr_id,
a.curr_acct_type,a.inc_curr_id,a.x_curr_id,
a.x_curr_acct_type,a.base_xrate,a.inc_base_xrate,
a.sec_base_xrate,a.accr_base_xrate,a.dr_cr,a.taxlot_no,a.divint_no,
b.sec_type,b.curr_id,b.inc_curr_id,
h1.level_1 Indust_Level1, h1.il1_effdate, h1.level_2 Indust_Level2, h1.il2_effdate, 
h1.level_3 Indust_Level3,h1.il3_effdate, 
b.sp_rating,b.moody_rating,b.internal_rating,b.ann_div_cpn,b.cur_exrate,
c.taxable_country,c.tax_country,c.taxable_state,c.tax_state,b.sec_desc1,
b.country_iss,b.country_isr,
a.tot_cost,a.opt_prem,a.base_open_xrate,
a.eff_date,a.open_trd_date,
a.gl_flag,a.rev_trans_no,a.units,
ISNULL((SELECT TOP 1 DATEADD(dd,  id-1, a.perf_date ) FROM rtntype with (nolock) 
 WHERE LEFT(DATENAME(dw, DATEADD(dd,  id-1, a.perf_date )),3) NOT IN ('Sat', 'Sun') 
AND NOT EXISTS (SELECT * FROM dcontrol with (nolock) 
WHERE recorddate = DATEADD(dd, id - 1, a.perf_date) AND MarketClosed='Y' 
AND country=ISNULL((SELECT TOP 1 value FROM sysvalues with (nolock) 
WHERE name='SystemCountry'), 'USA')) 
 AND DATEPART(mm, DATEADD(dd,  id-1, a.perf_date ))=DATEPART(mm, a.perf_date) 
 AND id BETWEEN 1 AND 7 
ORDER BY DATEADD(dd,  id, a.perf_date )), a.perf_date) AS PerfDateAdj, 
CASE WHEN e.DRDElig IS NULL THEN 
CASE WHEN b.sec_type in (1,2) THEN 'Y' ELSE 'N' END 
ELSE e.DRDElig END
)";

    // Part 2: FROM and JOINs
    sql += R"( FROM trans a with (nolock) 
left outer join assets b with (nolock) on a.sec_no = b.sec_no and a.wi = b.when_issue 
left outer join fixedinc c with (nolock) on b.sec_no=c.sec_no and b.when_issue= c.wi 
left outer join equities e with (nolock) on b.sec_no=e.sec_no and b.when_issue= e.wi 
left outer join(
select  q1.sec_no, q1.wi, q1.Segid as level_1, q1.EffDate as il1_effdate, 
ISNULL(q2.Segid,0) as level_2, ISNULL(q2.effdate, '12/30/1899') as il2_effdate, 
ISNULL(q3.SegId,0) as level_3, ISNULL(q3.effdate, '12/30/1899') as il3_effdate 
from(select distinct a.id, a.sec_no, a.wi, s.level_id, h.SegId, h.effdate from trans as a with (nolock) 
inner join histassetindustry h with (nolock) on h.sec_no = a.sec_no and h.wi = a.wi 
inner join segments s on s.id = h.segid where a.id = @PID and h.vendorId = @VID 
AND a.perf_date > @D1 AND a.perf_date <= @D2 AND 
a.sec_xtend <> 'TS' AND a.sec_xtend <> 'TL' AND a.cap_trans <> 'Y' AND 
h.deletedate < '1/1/1900' and s.level_id = 1 ) q1 
)";

    // Part 3: More JOINs and WHERE clause
    sql += R"(left join (select distinct a.id, a.sec_no, a.wi, s.level_id, h.SegId, h.effdate from trans as a with (nolock) 
inner join histassetindustry h with (nolock) on h.sec_no = a.sec_no and h.wi = a.wi 
inner join segments s on s.id = h.segid where a.id = @PID and h.vendorId = @VID 
AND a.perf_date > @D1 AND a.perf_date <= @D2 AND 
a.sec_xtend <> 'TS' AND a.sec_xtend <> 'TL' AND a.cap_trans <> 'Y'
and h.deletedate < '1/1/1900' 
and s.level_id = 2) q2 on q1.sec_no = q2.sec_no and q1.wi= q2.wi and q2.id = q1.id 
left join (select distinct a.id, a.sec_no, a.wi, s.level_id, h.SegId, h.effdate from trans as a with (nolock) 
inner join histassetindustry h with (nolock) on h.sec_no = a.sec_no and h.wi = a.wi 
inner join segments s on s.id = h.segid where a.id = @PID and h.vendorId = @VID 
AND a.perf_date > @D1 AND a.perf_date <= @D2 AND 
a.sec_xtend <> 'TS' AND a.sec_xtend <> 'TL' AND a.cap_trans <> 'Y' 
and h.deletedate < '1/1/1900' and s.level_id = 3) q3 
on q1.sec_no = q3.sec_no and q1.wi= q3.wi and q3.id = q1.id)
h1 ON h1.sec_no = a.sec_no and h1.wi = a.wi 
WHERE a.id = @PID AND 
perf_date > @D1 AND perf_date <= @D2 AND 
sec_xtend <> 'TS' AND sec_xtend <> 'TL' AND cap_trans <> 'Y' AND 
a.sec_no = b.sec_no AND a.wi = b.when_issue 
ORDER BY perf_date, trans_no option (KEEP PLAN) 
)";

    return sql;
}

DLLAPI void STDCALL SelectPerformanceTransaction(PARTTRANS *pzTR, PARTASSET *pzPartAsset, LEVELINFO *pzLevels, int iID, 
											long lStartPerfDate, long lEndPerfDate, int iVendorID, ERRSTRUCT *pzErr)
{
    try
    {
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            HandleDbError(pzErr, "SelectPerformanceTransaction: No connection", "SelectPerformanceTransaction");
            return;
        }

        std::string sSQL = BuildPerformanceTransactionSQL();
        nanodbc::statement stmt(*conn);
        stmt.prepare(sSQL);

        // Bind parameters: @PID, @VID, @D1, @D2
        stmt.bind(0, &iID);
        stmt.bind(1, &iVendorID);
        
        nanodbc::date dStart = LongToDateStruct(lStartPerfDate);
        stmt.bind(2, &dStart);
        
        nanodbc::date dEnd = LongToDateStruct(lEndPerfDate);
        stmt.bind(3, &dEnd);

        nanodbc::result result = stmt.execute();

        // This function seems to be designed to return ONE record at a time, 
        // but the legacy implementation had a loop with MoveNext that would return the *first* record
        // and then subsequent calls would return next records if parameters matched.
        // However, looking at the legacy code:
        // if (!(cmdSelectPerformanceTransaction.m_zTrans.iID == iID && ...)) { Close/Open }
        // It implements stateful iteration.
        // For now, we will implement a simplified version that fetches the first record,
        // or we need to implement the static state if callers rely on iteration.
        // Given the complexity and "SelectPerformanceTransaction" name (singular), 
        // but the legacy code clearly supports iteration.
        // Let's use the static result pattern to support iteration.

        static nanodbc::result g_Result;
        static bool g_ResultOpen = false;
        static int g_LastID = -1;
        static long g_LastStartDate = -1;
        static long g_LastEndDate = -1;
        static int g_LastVendorID = -1;

        if (!g_ResultOpen || g_LastID != iID || g_LastStartDate != lStartPerfDate || 
            g_LastEndDate != lEndPerfDate || g_LastVendorID != iVendorID)
        {
            g_Result = stmt.execute();
            g_ResultOpen = true;
            g_LastID = iID;
            g_LastStartDate = lStartPerfDate;
            g_LastEndDate = lEndPerfDate;
            g_LastVendorID = iVendorID;
        }

        if (g_Result.next())
        {
            // Map result to PARTTRANS
            pzTR->iID = g_Result.get<int>(0);
            pzTR->lTransNo = g_Result.get<long>(1);
            read_string(g_Result, 2, pzTR->sTranType, sizeof(pzTR->sTranType));
            read_string(g_Result, 3, pzTR->sSecNo, sizeof(pzTR->sSecNo));
            read_string(g_Result, 4, pzTR->sWi, sizeof(pzTR->sWi));
            read_string(g_Result, 5, pzTR->sSecXtend, sizeof(pzTR->sSecXtend));
            read_string(g_Result, 6, pzTR->sAcctType, sizeof(pzTR->sAcctType));
            pzTR->fPcplAmt = g_Result.get<double>(7);
            pzTR->fAccrInt = g_Result.get<double>(8);
            pzTR->fIncomeAmt = g_Result.get<double>(9);
            pzTR->fNetFlow = g_Result.get<double>(10);
            
            pzTR->lTrdDate = DateStructToLong(g_Result.get<nanodbc::date>(11));
            pzTR->lStlDate = DateStructToLong(g_Result.get<nanodbc::date>(12));
            pzTR->lEntryDate = DateStructToLong(g_Result.get<nanodbc::date>(13));
            
            read_string(g_Result, 14, pzTR->sRevType, sizeof(pzTR->sRevType));
            
            pzTR->lPerfDate = DateStructToLong(g_Result.get<nanodbc::date>(15));
            
            read_string(g_Result, 16, pzTR->sXSecNo, sizeof(pzTR->sXSecNo));
            read_string(g_Result, 17, pzTR->sXWi, sizeof(pzTR->sXWi));
            read_string(g_Result, 18, pzTR->sXSecXtend, sizeof(pzTR->sXSecXtend));
            read_string(g_Result, 19, pzTR->sXAcctType, sizeof(pzTR->sXAcctType));
            read_string(g_Result, 20, pzTR->sCurrId, sizeof(pzTR->sCurrId));
            read_string(g_Result, 21, pzTR->sCurrAcctType, sizeof(pzTR->sCurrAcctType));
            read_string(g_Result, 22, pzTR->sIncCurrId, sizeof(pzTR->sIncCurrId));
            read_string(g_Result, 23, pzTR->sXCurrId, sizeof(pzTR->sXCurrId));
            read_string(g_Result, 24, pzTR->sXCurrAcctType, sizeof(pzTR->sXCurrAcctType));
            
            pzTR->fBaseXrate = g_Result.get<double>(25);
            pzTR->fIncBaseXrate = g_Result.get<double>(26);
            pzTR->fSecBaseXrate = g_Result.get<double>(27);
            pzTR->fAccrBaseXrate = g_Result.get<double>(28);
            
            read_string(g_Result, 29, pzTR->sDrCr, sizeof(pzTR->sDrCr));
            
            pzTR->lTaxlotNo = g_Result.get<long>(30);
            pzTR->lDivintNo = g_Result.get<long>(31);
            
            // Map result to PARTASSET
            // Note: Legacy code maps PARTASSET2 (internal) to PARTASSET (public)
            // We read into pzPartAsset directly
            // Columns 32-34
            // pzPartAsset->iSecType is int, legacy mapped from column 33 (b.sec_type)
            // Wait, legacy column map:
            // COLUMN_ENTRY(33, m_zAsset.iSecType) -> index 32 (0-based)
            // COLUMN_ENTRY(34, m_zAsset.sCurrId) -> index 33
            // COLUMN_ENTRY(35, m_zAsset.sIncCurrId) -> index 34
            
            pzPartAsset->iSecType = g_Result.get<int>(32);
            read_string(g_Result, 33, pzPartAsset->sCurrId, sizeof(pzPartAsset->sCurrId));
            read_string(g_Result, 34, pzPartAsset->sIncCurrId, sizeof(pzPartAsset->sIncCurrId));
            
            // Map result to LEVELINFO
            pzLevels->iIndustLevel1 = g_Result.get<int>(35);
            pzLevels->lEffDate1 = DateStructToLong(g_Result.get<nanodbc::date>(36));
            pzLevels->iIndustLevel2 = g_Result.get<int>(37);
            pzLevels->lEffDate2 = DateStructToLong(g_Result.get<nanodbc::date>(38));
            pzLevels->iIndustLevel3 = g_Result.get<int>(39);
            pzLevels->lEffDate3 = DateStructToLong(g_Result.get<nanodbc::date>(40));
            
            // Continue mapping PARTASSET
            read_string(g_Result, 41, pzPartAsset->sSpRating, sizeof(pzPartAsset->sSpRating));
            read_string(g_Result, 42, pzPartAsset->sMoodyRating, sizeof(pzPartAsset->sMoodyRating));
            read_string(g_Result, 43, pzPartAsset->sInternalRating, sizeof(pzPartAsset->sInternalRating));
            pzPartAsset->fAnnDivCpn = g_Result.get<double>(44);
            pzPartAsset->fCurExrate = g_Result.get<double>(45);
            
            read_string(g_Result, 46, pzPartAsset->sTaxableCountry, sizeof(pzPartAsset->sTaxableCountry));
            read_string(g_Result, 47, pzPartAsset->sTaxCountry, sizeof(pzPartAsset->sTaxCountry));
            read_string(g_Result, 48, pzPartAsset->sTaxableState, sizeof(pzPartAsset->sTaxableState));
            read_string(g_Result, 49, pzPartAsset->sTaxState, sizeof(pzPartAsset->sTaxState));
            read_string(g_Result, 50, pzPartAsset->sSecDesc1, sizeof(pzPartAsset->sSecDesc1));
            read_string(g_Result, 51, pzPartAsset->sCountryIss, sizeof(pzPartAsset->sCountryIss));
            read_string(g_Result, 52, pzPartAsset->sCountryIsr, sizeof(pzPartAsset->sCountryIsr));
            
            // Continue mapping PARTTRANS
            pzTR->fTotCost = g_Result.get<double>(53);
            pzTR->fOptPrem = g_Result.get<double>(54);
            pzTR->fBaseOpenXrate = g_Result.get<double>(55);
            
            pzTR->lEffDate = DateStructToLong(g_Result.get<nanodbc::date>(56));
            pzTR->lOpenTrdDate = DateStructToLong(g_Result.get<nanodbc::date>(57));
            
            read_string(g_Result, 58, pzTR->sGLFlag, sizeof(pzTR->sGLFlag));
            pzTR->lRevTransNo = g_Result.get<long>(59);
            pzTR->fUnits = g_Result.get<double>(60);
            pzTR->lPerfDateAdj = DateStructToLong(g_Result.get<nanodbc::date>(61));
            
            // Last one for PARTASSET
            read_string(g_Result, 62, pzPartAsset->sDRDElig, sizeof(pzPartAsset->sDRDElig));
            
            // Also copy SecNo and Wi to PARTASSET as legacy code did
            strcpy_s(pzPartAsset->sSecNo, pzTR->sSecNo);
            strcpy_s(pzPartAsset->sWhenIssue, pzTR->sWi);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_ResultOpen = false;
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectPerformanceTransaction");
    }
}

DLLAPI void STDCALL SelectAllPartTrantype(PARTTRANTYPE *pzPTType, ERRSTRUCT *pzErr)
{
    try
    {
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            HandleDbError(pzErr, "SelectAllPartTrantype: No connection", "SelectAllPartTrantype");
            return;
        }

        static nanodbc::result g_Result;
        static bool g_ResultOpen = false;

        if (!g_ResultOpen)
        {
            nanodbc::statement stmt(*conn);
            stmt.prepare("select tran_type, dr_cr, tran_code, sec_impact, perf_impact from TRANTYPE ORDER BY tran_type, dr_cr");
            g_Result = stmt.execute();
            g_ResultOpen = true;
        }

        if (g_Result.next())
        {
            read_string(g_Result, 0, pzPTType->sTranType, sizeof(pzPTType->sTranType));
            read_string(g_Result, 1, pzPTType->sDrCr, sizeof(pzPTType->sDrCr));
            read_string(g_Result, 2, pzPTType->sTranCode, sizeof(pzPTType->sTranCode));
            pzPTType->lSecImpact = g_Result.get<long>(3);
            read_string(g_Result, 4, pzPTType->sPerfImpact, sizeof(pzPTType->sPerfImpact));
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_ResultOpen = false;
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectAllPartTrantype");
    }
}

DLLAPI void STDCALL DeleteDailyFlows(int iPortfolioID, long lBeginDate, ERRSTRUCT *pzErr)
{
    try
    {
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            HandleDbError(pzErr, "DeleteDailyFlows: No connection", "DeleteDailyFlows");
            return;
        }

        nanodbc::statement stmt(*conn);
        stmt.prepare("DELETE FROM dailyflows WHERE perform_date > ? AND segmain_id IN (SELECT id from segmain where owner_id = ?) ");
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(0, &dBegin);
        stmt.bind(1, &iPortfolioID);
        
        stmt.execute();
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteDailyFlows");
    }
}

DLLAPI void STDCALL InsertDailyFlows(DAILYFLOWS zDF, ERRSTRUCT *pzErr)
{
    try
    {
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            HandleDbError(pzErr, "InsertDailyFlows: No connection", "InsertDailyFlows");
            return;
        }

        nanodbc::statement stmt(*conn);
        stmt.prepare("INSERT INTO DAILYFLOWS (segmain_id, perform_date, net_flow, fees, fees_out, create_date, cons_fee) VALUES (?, ?, ?, ?, ?, ?, ?)");
        
        stmt.bind(0, &zDF.iSegmainID);
        
        nanodbc::date dPerform = LongToDateStruct(zDF.lPerformDate);
        stmt.bind(1, &dPerform);
        
        stmt.bind(2, &zDF.fNetFlow);
        stmt.bind(3, &zDF.fFees);
        stmt.bind(4, &zDF.fFeesOut);
        
        nanodbc::date dCreate = LongToDateStruct((long)zDF.fCreateDate); // Assuming fCreateDate is float/double representing date? Legacy used SETVARDATE which handles double/long.
        // Legacy: SETVARDATE(cmdInsertDailyFlows.m_vCreateDate,zDF.fCreateDate);
        // zDF.fCreateDate seems to be float/double.
        stmt.bind(5, &dCreate);
        
        stmt.bind(6, &zDF.fCNFees);
        
        stmt.execute();
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "InsertDailyFlows");
    }
}

DLLAPI void STDCALL DeleteDailyFlowsByID(int iSegmainID, long lBeginDate, ERRSTRUCT *pzErr)
{
    try
    {
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            HandleDbError(pzErr, "DeleteDailyFlowsByID: No connection", "DeleteDailyFlowsByID");
            return;
        }

        nanodbc::statement stmt(*conn);
        stmt.prepare("DELETE FROM dailyflows WHERE perform_date > ? AND segmain_id = ? ");
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(0, &dBegin);
        stmt.bind(1, &iSegmainID);
        
        stmt.execute();
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteDailyFlowsByID");
    }
}
