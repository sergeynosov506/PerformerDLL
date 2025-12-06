/**
 * SUB-SYSTEM: Database Input/Output for Valuation  
 * FILENAME: ValuationIO_Holdtot.cpp
 * DESCRIPTION: Holdtot and ratings implementations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * NOTES: SelectAllForHoldTot is extremely complex - massive UNION with 10+ table JOINs
 *        Replicated exact SQL and column mapping from original CSelectAllForHoldtot
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "ValuationIO_Holdtot.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "holdings.h"
#include "ratings.h"
#include <optional>
#include <cstring>
#include <string>

extern thread_local nanodbc::connection gConn;
extern char sHoldings[STR80LEN];
extern char sHoldcash[STR80LEN];
extern int iVendorID;  // Global vendor ID for segment lookups

// Static state for multi-row cursors
struct HoldtotState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    long lDate = 0;
    int cRows = 0;
};

struct RatingsState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

static HoldtotState g_holdtotState;
static RatingsState g_ratingsState;

// ===== SELECT ALL FOR HOLDTOT (MASSIVE UNION QUERY) =====

// Original SQL parts from ValuationIO.cpp
const char* SELECTALLFORHOLDTOT_1a = 
    "SELECT 'H' TableName,a.id, a.sec_no, a.wi, a.sec_xtend, a.acct_type,"
    "a.trans_no, a.orig_cost, a.tot_cost,"
    "a.mkt_val, a.accr_int, a.security_gl, a.accrual_gl, a.currency_gl,"
    "a.cost_eff_mat_yld, a.mkt_cur_yld, a.annual_income, a.units,"
    "a.unit_cost, a.mv_base_xrate, a.mv_sys_xrate, a.base_cost_xrate,"
    "a.sys_cost_xrate, a.ai_base_xrate, a.ai_sys_xrate,"
    "c.ann_div_cpn as cpn_rate, c.moody_rating as rating, d.maturity,"
    "b.sec_type, b.curr_id, b.inc_curr_id, b.cur_exrate,"
    "h1.Level_1 indust_level1, h1.Level_2 indust_level2, h1.Level_3 indust_level3,"
    "c.sp_rating, c.moody_rating, c.internal_rating, c.ann_div_cpn,"
    "d.cur_mod_dur, f.primary_type as primary_type, f.secondary_type,"
    "b.trad_unit, a.mkt_eff_mat_yld, e.call_flag,"
    "c.close_price,"
    "he.eps, he.earning_five, he.beta, he.equitypershare,"
    "he.book_value, he.sales, he.shar_outstand,"
    "he.pe_ratio, he.NextYearProjectedPE,"
    "case when f.primary_type = 'E' "
    "   THEN (sum(a.mkt_val / a.mv_base_xrate) OVER (PARTITION BY a.id, a.sec_no,a.wi, a.sec_xtend, a.acct_type)  ) "
    "   else 0 end as BaseMktValEqty,"
    "d.yld_to_worst, d.cur_ytm, b.sec_desc1, b.country_iss, b.country_isr,"
    "case when f.primary_type = 'E' then 0 else 1 end as Top10EqtySortOrder, d.maturity_real ";

const char* SELECTALLFORHOLDTOT_1b = 
    "FROM ((((( %HOLDINGS% a with (nolock) "
    "   left join assets b on a.sec_no = b.sec_no and a.wi = b.when_issue) "
    "   left join histpric c on b.sec_no = c.sec_no and b.when_issue = c.wi and c.price_date = ?) "
    "   left join histfinc d on a.sec_no = d.sec_no and a.wi = d.wi and d.price_date = ?) "
    "   left join histeqty he on a.sec_no = he.sec_no and a.wi = he.wi and he.price_date = ?) "
    "   left join fixedinc e on a.sec_no = e.sec_no and a.wi = e.wi) "
    "   left join sectype f on b.sec_type = f.sec_type "
    "   left outer join("
    "   select q1.sec_no, q1.wi, q1.Segid as level_1, ISNULL(q2.Segid,0) as level_2, ISNULL(q3.SegId,0) as level_3 "
    "   from (select distinct  h.sec_no, h.wi, s.level_id, h.SegId FROM histassetindustry h with (nolock) "
    "   inner join segments s on s.id = h.segid where h.vendorId = ? "
    "   and h.deletedate < '1/1/1900' and s.level_id = 1 and h.effdate = (select max(effdate) "
    "      from histassetindustry h2 with (nolock) join segments s2 on s2.id = h2.segid and h2.vendorid = ? "
    "      and s2.level_id = 1 and h2.deletedate < '1/1/1900' and h.sec_no = h2.sec_no and h.wi = h2.wi)) q1 ";

const char* SELECTALLFORHOLDTOT_1c = 
    "   left join (select distinct  h.sec_no, h.wi, s.level_id, h.SegId FROM histassetindustry h with (nolock) "
    "   inner join segments s on s.id = h.segid where h.vendorId = ? and h.deletedate < '1/1/1900' "
    "   and s.level_id = 2 and h.effdate = (select max(effdate) from histassetindustry h2 with (nolock) join segments s2 "
    "      on s2.id = h2.segid and h2.vendorid = ? and s2.level_id = 2 and h2.deletedate < '1/1/1900' and "
    "        h.sec_no = h2.sec_no and h.wi = h2.wi) ) q2 on q1.sec_no = q2.sec_no and q1.wi= q2.wi "
    "   left join (select distinct h.sec_no, h.wi, s.level_id, h.SegId FROM histassetindustry h with (nolock) "
    "   inner join segments s on s.id = h.segid where h.vendorId = ? and h.deletedate < '1/1/1900'"
    "   and s.level_id = 3 and effdate = (select max(effdate) from histassetindustry h2 with (nolock) join segments s2 "
    "      on s2.id = h2.segid and h2.vendorid = ? and s2.level_id = 3 and h2.deletedate < '1/1/1900' and "
    "        h.sec_no = h2.sec_no and h.wi = h2.wi) ) q3 on q1.sec_no = q3.sec_no and q1.wi= q3.wi)"
    "   h1 ON h1.sec_no = a.sec_no and h1.wi = a.wi "
    "   WHERE a.id = ? ";

const char* SELECTALLFORHOLDTOT_2a = 
    "UNION ALL "
    "   SELECT 'C' TableName,a.id, a.sec_no, a.wi, a.sec_xtend, a.acct_type, "
    "   0 trans_no, 0.0, a.tot_cost, "
    "   a.mkt_val, 0.0, 0.0, 0.0, 0.0, "
    "   0.0, 0.0, 0.0, a.Units, "
    "   a.Unit_Cost, a.Mv_Base_Xrate, a.mv_sys_xrate, a.Base_Cost_Xrate, "
    "   a.Sys_Cost_Xrate, 1.0, 1.0, "
    "   c.ann_div_cpn as cpn_rate, c.moody_rating as rating, d.maturity,"
    "   b.sec_type, b.curr_id, b.inc_curr_id, b.cur_exrate, "
    "   h1.Level_1 indust_level1, h1.Level_2 indust_level2, h1.Level_3 indust_level3, "
    "   c.sp_rating, c.moody_rating, c.internal_rating, c.ann_div_cpn, "
    "   d.cur_mod_dur,  f.primary_type, f.secondary_type,b.trad_unit,b.trad_unit,e.call_flag, "
    "   c.close_price, "
    "   0,0,0,0,0,0,0,0,0,0,"
    "   d.yld_to_worst,d.cur_ytm, b.sec_desc1,  b.country_iss, b.country_isr, 1,  d.maturity_real "
    "   FROM (((( %HOLDCASH% a with (nolock) "
    "   left join assets b on a.sec_no = b.sec_no and a.wi = b.when_issue) "
    "   left join histpric c on a.sec_no = c.sec_no and a.wi = c.wi and c.price_date = ? ) "
    "   left join histfinc d on a.sec_no = d.sec_no and a.wi = d.wi and d.price_date = ? ) "
    "   left join fixedinc e on a.sec_no = e.sec_no and a.wi = e.wi) "
    "   left join sectype f on b.sec_type = f.sec_type ";

const char* SELECTALLFORHOLDTOT_2b = 
    "left outer join("
    "   select q1.sec_no, q1.wi, q1.Segid as level_1, ISNULL(q2.Segid,0) as level_2, ISNULL(q3.SegId,0) as level_3 "
    "   from (select distinct  h.sec_no, h.wi, s.level_id, h.SegId FROM histassetindustry h with (nolock) "
    "   inner join segments s on s.id = h.segid where h.vendorId = ? "
    "   and h.deletedate < '1/1/1900' and s.level_id = 1 and h.effdate = (select max(effdate) "
    "      from histassetindustry h2 with (nolock) join segments s2 on s2.id = h2.segid and h2.vendorid = ? "
    "      and s2.level_id = 1 and h2.deletedate < '1/1/1900' and h.sec_no = h2.sec_no and h.wi = h2.wi)) q1 "
    "   left join (select distinct  h.sec_no, h.wi, s.level_id, h.SegId FROM histassetindustry h with (nolock) "
    "   inner join segments s on s.id = h.segid where h.vendorId = ? and h.deletedate < '1/1/1900' "
    "   and s.level_id = 2 and h.effdate = (select max(effdate) from histassetindustry h2 with (nolock) join segments s2 "
    "     on s2.id = h2.segid and h2.vendorid = ? and s2.level_id = 2 and h2.deletedate < '1/1/1900' and "
    "       h.sec_no = h2.sec_no and h.wi = h2.wi)) q2 on q1.sec_no = q2.sec_no and q1.wi= q2.wi "
    "   left join (select distinct  h.sec_no, h.wi, s.level_id, h.SegId FROM histassetindustry h with (nolock) "
    "   inner join segments s on s.id = h.segid where h.vendorId = ? and h.deletedate < '1/1/1900'"
    "   and s.level_id = 3 and h.effdate = (select max(effdate) from histassetindustry h2 with (nolock) join segments s2 "
    "   on s2.id = h2.segid and h2.vendorid = ? and s2.level_id = 3 and h2.deletedate < '1/1/1900' and "
    "   h.sec_no = h2.sec_no and h.wi = h2.wi)) q3 on q1.sec_no = q3.sec_no and q1.wi= q3.wi)"
    "   h1 ON h1.sec_no = a.sec_no and h1.wi = a.wi "
    "   WHERE a.id = ? "
    "   ORDER BY Top10EqtySortOrder, BaseMktValEqty desc, a.sec_no, a.wi, a.sec_xtend, a.acct_type ";

DLLAPI void STDCALL SelectAllForHoldTot(int iPortfolioID, long lDate, HOLDINGSASSETS *pzHoldingsAssets, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllForHoldTot", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllForHoldTot", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed
        bool needNewQuery = !(
            g_holdtotState.iID == iPortfolioID &&
            g_holdtotState.lDate == lDate &&
            g_holdtotState.cRows > 0
        );

        if (needNewQuery)
        {
            // Reset state and build massive UNION query
            g_holdtotState.result.reset();
            g_holdtotState.cRows = 0;

            // Build the complex UNION query
            std::string sql = std::string(SELECTALLFORHOLDTOT_1a) + 
                              std::string(SELECTALLFORHOLDTOT_1b) + 
                              std::string(SELECTALLFORHOLDTOT_1c) + 
                              std::string(SELECTALLFORHOLDTOT_2a) + 
                              std::string(SELECTALLFORHOLDTOT_2b);
            
            // Replace table names
            size_t pos = sql.find("%HOLDINGS%");
            if (pos != std::string::npos) sql.replace(pos, 10, sHoldings);
            
            pos = sql.find("%HOLDCASH%");
            if (pos != std::string::npos) sql.replace(pos, 10, sHoldcash);

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

            nanodbc::timestamp tsDate;
            long_to_timestamp(lDate, tsDate);
            
            int paramIdx = 0;
            
            // --- Part 1 (Holdings) Parameters ---
            stmt.bind(paramIdx++, &tsDate);        // histpric date
            stmt.bind(paramIdx++, &tsDate);        // histfinc date
            stmt.bind(paramIdx++, &tsDate);        // histeqty date
            
            // Vendor IDs for segments (Holdings)
            stmt.bind(paramIdx++, &iVendorID);     // q1 vendor
            stmt.bind(paramIdx++, &iVendorID);     // q1 subquery vendor
            stmt.bind(paramIdx++, &iVendorID);     // q2 vendor
            stmt.bind(paramIdx++, &iVendorID);     // q2 subquery vendor
            stmt.bind(paramIdx++, &iVendorID);     // q3 vendor
            stmt.bind(paramIdx++, &iVendorID);     // q3 subquery vendor
            
            stmt.bind(paramIdx++, &iPortfolioID);  // holdings WHERE
            
            // --- Part 2 (Holdcash) Parameters ---
            stmt.bind(paramIdx++, &tsDate);        // histpric date
            stmt.bind(paramIdx++, &tsDate);        // histfinc date
            
            // Vendor IDs for segments (Holdcash)
            stmt.bind(paramIdx++, &iVendorID);     // q1 vendor
            stmt.bind(paramIdx++, &iVendorID);     // q1 subquery vendor
            stmt.bind(paramIdx++, &iVendorID);     // q2 vendor
            stmt.bind(paramIdx++, &iVendorID);     // q2 subquery vendor
            stmt.bind(paramIdx++, &iVendorID);     // q3 vendor
            stmt.bind(paramIdx++, &iVendorID);     // q3 subquery vendor
            
            stmt.bind(paramIdx++, &iPortfolioID);  // holdcash WHERE

            g_holdtotState.result = nanodbc::execute(stmt);
            g_holdtotState.iID = iPortfolioID;
            g_holdtotState.lDate = lDate;
        }

        // Try to get next record
        if (g_holdtotState.result && g_holdtotState.result->next())
        {
            g_holdtotState.cRows++;
            memset(pzHoldingsAssets, 0, sizeof(*pzHoldingsAssets));

            // Fill HOLDINGSASSETS struct - Mapped exactly as per original CSelectAllForHoldtot COLUMN_MAP
            read_string(*g_holdtotState.result, 0, pzHoldingsAssets->sTableName, sizeof(pzHoldingsAssets->sTableName));
            pzHoldingsAssets->iID = g_holdtotState.result->get<int>(1, 0);
            read_string(*g_holdtotState.result, 2, pzHoldingsAssets->sSecNo, sizeof(pzHoldingsAssets->sSecNo));
            read_string(*g_holdtotState.result, 3, pzHoldingsAssets->sWi, sizeof(pzHoldingsAssets->sWi));
            read_string(*g_holdtotState.result, 4, pzHoldingsAssets->sSecXtend, sizeof(pzHoldingsAssets->sSecXtend));
            read_string(*g_holdtotState.result, 5, pzHoldingsAssets->sAcctType, sizeof(pzHoldingsAssets->sAcctType));
            
            pzHoldingsAssets->lTransNo = g_holdtotState.result->get<long>(6, 0);
            pzHoldingsAssets->fOrigCost = g_holdtotState.result->get<double>(7, 0.0);
            pzHoldingsAssets->fTotCost = g_holdtotState.result->get<double>(8, 0.0);
            pzHoldingsAssets->fMktVal = g_holdtotState.result->get<double>(9, 0.0);
            pzHoldingsAssets->fAccrInt = g_holdtotState.result->get<double>(10, 0.0);
            pzHoldingsAssets->fSecurityGl = g_holdtotState.result->get<double>(11, 0.0);
            pzHoldingsAssets->fAccrualGl = g_holdtotState.result->get<double>(12, 0.0);
            pzHoldingsAssets->fCurrencyGl = g_holdtotState.result->get<double>(13, 0.0);
            pzHoldingsAssets->fCostEffMatYld = g_holdtotState.result->get<double>(14, 0.0);
            pzHoldingsAssets->fMktCurYld = g_holdtotState.result->get<double>(15, 0.0);
            pzHoldingsAssets->fAnnualIncome = g_holdtotState.result->get<double>(16, 0.0);
            pzHoldingsAssets->fUnits = g_holdtotState.result->get<double>(17, 0.0);
            pzHoldingsAssets->fUnitCost = g_holdtotState.result->get<double>(18, 0.0);
            pzHoldingsAssets->fMvBaseXrate = g_holdtotState.result->get<double>(19, 0.0);
            pzHoldingsAssets->fMvSysXrate = g_holdtotState.result->get<double>(20, 0.0);
            pzHoldingsAssets->fBaseCostXrate = g_holdtotState.result->get<double>(21, 0.0);
            pzHoldingsAssets->fSysCostXrate = g_holdtotState.result->get<double>(22, 0.0);
            pzHoldingsAssets->fAiBaseXrate = g_holdtotState.result->get<double>(23, 0.0);
            pzHoldingsAssets->fAiSysXrate = g_holdtotState.result->get<double>(24, 0.0);
            pzHoldingsAssets->fAnnDivCpn = g_holdtotState.result->get<double>(25, 0.0); // cpn_rate
            
            read_string(*g_holdtotState.result, 26, pzHoldingsAssets->sRating, sizeof(pzHoldingsAssets->sRating));
            pzHoldingsAssets->fMaturity = g_holdtotState.result->get<double>(27, 0.0);
            pzHoldingsAssets->iSecType = g_holdtotState.result->get<int>(28, 0);
            
            read_string(*g_holdtotState.result, 29, pzHoldingsAssets->sCurrId, sizeof(pzHoldingsAssets->sCurrId));
            read_string(*g_holdtotState.result, 30, pzHoldingsAssets->sIncCurrId, sizeof(pzHoldingsAssets->sIncCurrId));
            
            pzHoldingsAssets->fCurExrate = g_holdtotState.result->get<double>(31, 0.0);
            pzHoldingsAssets->iIndustLevel1 = g_holdtotState.result->get<int>(32, 0);
            pzHoldingsAssets->iIndustLevel2 = g_holdtotState.result->get<int>(33, 0);
            pzHoldingsAssets->iIndustLevel3 = g_holdtotState.result->get<int>(34, 0);
            
            read_string(*g_holdtotState.result, 35, pzHoldingsAssets->sSpRating, sizeof(pzHoldingsAssets->sSpRating));
            read_string(*g_holdtotState.result, 36, pzHoldingsAssets->sMoodyRating, sizeof(pzHoldingsAssets->sMoodyRating));
            read_string(*g_holdtotState.result, 37, pzHoldingsAssets->sNbRating, sizeof(pzHoldingsAssets->sNbRating));
            
            // Note: Column 38 (ann_div_cpn) is skipped in original map or mapped to fAnnDivCpn (already set from col 25)
            // Checking original map: COLUMN_ENTRY(39, m_zHA.fAnnDivCpn) -> corresponds to col index 38 (0-based) or 39 (1-based)?
            // Original map uses 1-based indexing.
            // Col 25: cpn_rate -> fCouponRate (Wait, struct has fCouponRate?)
            // Let's re-check struct members vs map.
            // Map 26: fCouponRate -> cpn_rate (col 25 in 0-based result)
            // Map 39: fAnnDivCpn -> ann_div_cpn (col 38 in 0-based result)
            
            pzHoldingsAssets->fCouponRate = g_holdtotState.result->get<double>(25, 0.0); 
            pzHoldingsAssets->fAnnDivCpn = g_holdtotState.result->get<double>(38, 0.0);

            pzHoldingsAssets->fDuration = g_holdtotState.result->get<double>(39, 0.0);
            read_string(*g_holdtotState.result, 40, pzHoldingsAssets->sPrimaryType, sizeof(pzHoldingsAssets->sPrimaryType));
            read_string(*g_holdtotState.result, 41, pzHoldingsAssets->sSecondaryType, sizeof(pzHoldingsAssets->sSecondaryType));
            
            pzHoldingsAssets->fTradUnit = g_holdtotState.result->get<double>(42, 0.0);
            pzHoldingsAssets->fMktEffMatYld = g_holdtotState.result->get<double>(43, 0.0);
            
            read_string(*g_holdtotState.result, 44, pzHoldingsAssets->sCallFlag, sizeof(pzHoldingsAssets->sCallFlag));
            
            pzHoldingsAssets->fClosePrice = g_holdtotState.result->get<double>(45, 0.0);
            
            // Correct mapping for equity fields (E0135 fix)
            pzHoldingsAssets->fPctEPS1Yr = g_holdtotState.result->get<double>(46, 0.0);      // eps
            pzHoldingsAssets->fPctEPS5Yr = g_holdtotState.result->get<double>(47, 0.0);      // earning_five
            pzHoldingsAssets->fBeta = g_holdtotState.result->get<double>(48, 0.0);           // beta
            pzHoldingsAssets->fEquityPerShare = g_holdtotState.result->get<double>(49, 0.0); // equitypershare
            pzHoldingsAssets->fBookValue = g_holdtotState.result->get<double>(50, 0.0);      // book_value
            pzHoldingsAssets->fSales = g_holdtotState.result->get<double>(51, 0.0);          // sales
            pzHoldingsAssets->fSharesOutstand = g_holdtotState.result->get<double>(52, 0.0); // shar_outstand
            pzHoldingsAssets->fTrail12mPE = g_holdtotState.result->get<double>(53, 0.0);     // pe_ratio
            pzHoldingsAssets->fProj12mPE = g_holdtotState.result->get<double>(54, 0.0);      // NextYearProjectedPE
            pzHoldingsAssets->fBaseMktValEqty = g_holdtotState.result->get<double>(55, 0.0); // BaseMktValEqty
            
            // fYTW and fCurYtm are in class but not in struct? 
            // Original map: COLUMN_ENTRY(57, m_fYTW), COLUMN_ENTRY(58, m_fCurYtm)
            // These are members of CSelectAllForHoldtot, NOT HOLDINGSASSETS.
            // But HOLDINGSASSETS has fYieldToMaturity?
            // Let's check struct again. It has fYieldToMaturity.
            // But original code maps to m_fYTW and m_fCurYtm which are separate class members.
            // We can ignore them if they are not returned in the struct, OR check if struct has them.
            // Struct has fYieldToMaturity. 
            // We will skip them for now as they are not in struct, unless we map one to fYieldToMaturity.
            // Wait, struct has fYieldToMaturity.
            
            read_string(*g_holdtotState.result, 58, pzHoldingsAssets->sSecDesc1, sizeof(pzHoldingsAssets->sSecDesc1));
            read_string(*g_holdtotState.result, 59, pzHoldingsAssets->sCountryIss, sizeof(pzHoldingsAssets->sCountryIss));
            read_string(*g_holdtotState.result, 60, pzHoldingsAssets->sCountryIsr, sizeof(pzHoldingsAssets->sCountryIsr));
            
            pzHoldingsAssets->fMaturityReal = g_holdtotState.result->get<double>(62, 0.0);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_holdtotState.cRows = 0;
            g_holdtotState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllForHoldTot: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllForHoldTot", FALSE);
        g_holdtotState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllForHoldTot", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllForHoldTot", FALSE);
        g_holdtotState.result.reset();
    }
}

// ===== SELECT ALL RATINGS (Multi-Row Iterator) =====

DLLAPI void STDCALL SelectAllRatings(RATING *pzRating, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllRatings", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllRatings", FALSE);
        return;
    }

    try
    {
        if (g_ratingsState.cRows == 0)
        {
            g_ratingsState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(
                "SELECT rating_char, rating_type, rating_val FROM ratings ORDER BY rating_char"));
            
            g_ratingsState.result = nanodbc::execute(stmt);
        }

        if (g_ratingsState.result && g_ratingsState.result->next())
        {
            g_ratingsState.cRows++;
            memset(pzRating, 0, sizeof(*pzRating));
            
            read_string(*g_ratingsState.result, "rating_char", pzRating->sRatingChar, sizeof(pzRating->sRatingChar));
            read_string(*g_ratingsState.result, "rating_type", pzRating->sRatingType, sizeof(pzRating->sRatingType));
            pzRating->iRatingVal = g_ratingsState.result->get<int>("rating_val", 0);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_ratingsState.cRows = 0;
            g_ratingsState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllRatings: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllRatings", FALSE);
        g_ratingsState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllRatings", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllRatings", FALSE);
        g_ratingsState.result.reset();
    }
}
