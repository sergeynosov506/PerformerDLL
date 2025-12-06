#include "TransIO_Assets.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;
extern ASSETS zSavedAsset;
extern bool bAssetIsValid;

DLLAPI void STDCALL SelectAsset(ASSETS *pzAssets, char *sSecNo, char *sWhenIssue, int iVendorID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    // Check cache
    if (bAssetIsValid && 
        strcmp(zSavedAsset.sSecNo, sSecNo) == 0 &&
        strcmp(zSavedAsset.sWhenIssue, sWhenIssue) == 0) 
    {
        memcpy(pzAssets, &zSavedAsset, sizeof(zSavedAsset));
        return;
    }

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectAsset", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT a.sec_no, a.when_issue, "
            "cusip, exchange, exchange_lock,sec_type, "
            "sec_desc1,sec_desc2,sec_desc3,sec_desc4,sec_desc5, "
            "sort_name, short_name, desc_lock,data_source, record_lock,"
            "curr_id, inc_curr_id, margin,sp_rating, moody_rating, internal_rating,"
            "auto_divint, auto_action, auto_mature, trad_unit,units,"
            "ann_div_cpn, ann_div_cpn_lock,ann_div_fnote, cur_prdate,"
            "cur_close, cur_bid, cur_ask, cur_source, cur_pr_exch,cur_exrate,cur_inc_exrate,"
            "prior_prdate, prior_close, prior_bid, prior_ask, prior_source,prior_pr_exch,"
            "prior_exrate, prior_inc_exrate, dtc_flag, cns_flag, market_maker,issue_status,"
            "owner_grp,create_date,created_by,date_priced,modify_date,modified_by,"
            "delete_date, "
            "Min(CASE WHEN s.level_id = 1 THEN h.segid ELSE a.Indust_Level1 END) AS SegId1,"
            "Min(CASE WHEN s.level_id = 2 THEN h.segid ELSE a.Indust_Level2 END) AS SegId2,"
            "Min(CASE WHEN s.level_id = 3 THEN h.segid ELSE a.Indust_Level3 END) AS SegId3,"
            "sic, country_iss,"
            "country_isr,euro_clear,volume,inv_pct,a.id, benchmark_secno "
            "FROM assets as a "
            "LEFT OUTER JOIN histassetindustry AS h "
            " ON h.sec_no = a.sec_no and h.wi = a.when_issue AND h.vendorid = ? "
            " and h.deletedate < '1/1/1900' "
            "LEFT OUTER JOIN segments AS s ON s.id = h.segid "
            "WHERE a.sec_no = ? AND a.when_issue = ? "
            "GROUP BY a.sec_no, a.when_issue, "
            "  cusip, exchange, exchange_lock,sec_type,"
            "sec_desc1,sec_desc2,sec_desc3,sec_desc4,sec_desc5,"
            "sort_name, short_name, desc_lock,data_source, record_lock,"
            "curr_id, inc_curr_id, margin,sp_rating, moody_rating, internal_rating,"
            "auto_divint, auto_action, auto_mature, trad_unit,units,"
            "ann_div_cpn, ann_div_cpn_lock,ann_div_fnote, cur_prdate,"
            "cur_close, cur_bid, cur_ask, cur_source, cur_pr_exch,cur_exrate,cur_inc_exrate,"
            "prior_prdate, prior_close, prior_bid, prior_ask, prior_source,prior_pr_exch,"
            "prior_exrate, prior_inc_exrate, dtc_flag, cns_flag, market_maker,issue_status,"
            "owner_grp,create_date,created_by,date_priced,modify_date,modified_by,"
            "delete_date, sic, country_iss,"
            "country_isr,euro_clear,volume,inv_pct, a.id, benchmark_secno "));

        stmt.bind(0, &iVendorID);
        stmt.bind(1, sSecNo);
        stmt.bind(2, sWhenIssue);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "sec_no", pzAssets->sSecNo, sizeof(pzAssets->sSecNo));
            read_string(result, "when_issue", pzAssets->sWhenIssue, sizeof(pzAssets->sWhenIssue));
            read_string(result, "cusip", pzAssets->sCusip, sizeof(pzAssets->sCusip));
            read_string(result, "exchange", pzAssets->sExchange, sizeof(pzAssets->sExchange));
            read_string(result, "exchange_lock", pzAssets->sExchangeLock, sizeof(pzAssets->sExchangeLock));
            pzAssets->iSecType = result.get<short>("sec_type", 0);
            
            read_string(result, "sec_desc1", pzAssets->sSecDesc1, sizeof(pzAssets->sSecDesc1));
            read_string(result, "sec_desc2", pzAssets->sSecDesc2, sizeof(pzAssets->sSecDesc2));
            read_string(result, "sec_desc3", pzAssets->sSecDesc3, sizeof(pzAssets->sSecDesc3));
            read_string(result, "sec_desc4", pzAssets->sSecDesc4, sizeof(pzAssets->sSecDesc4));
            read_string(result, "sec_desc5", pzAssets->sSecDesc5, sizeof(pzAssets->sSecDesc5));

            read_string(result, "sort_name", pzAssets->sSortName, sizeof(pzAssets->sSortName));
            read_string(result, "short_name", pzAssets->sShortName, sizeof(pzAssets->sShortName));
            read_string(result, "desc_lock", pzAssets->sDescLock, sizeof(pzAssets->sDescLock));
            read_string(result, "data_source", pzAssets->sDataSource, sizeof(pzAssets->sDataSource));
            read_string(result, "record_lock", pzAssets->sRecordLock, sizeof(pzAssets->sRecordLock));

            read_string(result, "curr_id", pzAssets->sCurrId, sizeof(pzAssets->sCurrId));
            read_string(result, "inc_curr_id", pzAssets->sIncCurrId, sizeof(pzAssets->sIncCurrId));
            read_string(result, "margin", pzAssets->sMargin, sizeof(pzAssets->sMargin));
            read_string(result, "sp_rating", pzAssets->sSpRating, sizeof(pzAssets->sSpRating));
            read_string(result, "moody_rating", pzAssets->sMoodyRating, sizeof(pzAssets->sMoodyRating));
            read_string(result, "internal_rating", pzAssets->sInternalRating, sizeof(pzAssets->sInternalRating));

            read_string(result, "auto_divint", pzAssets->sAutoDivint, sizeof(pzAssets->sAutoDivint));
            read_string(result, "auto_action", pzAssets->sAutoAction, sizeof(pzAssets->sAutoAction));
            read_string(result, "auto_mature", pzAssets->sAutoMature, sizeof(pzAssets->sAutoMature));
            pzAssets->fTradUnit = result.get<double>("trad_unit", 0.0);
            pzAssets->fUnits = result.get<double>("units", 0.0);

            pzAssets->fAnnDivCpn = result.get<double>("ann_div_cpn", 0.0);
            read_string(result, "ann_div_cpn_lock", pzAssets->sAnnDivCpnLock, sizeof(pzAssets->sAnnDivCpnLock));
            read_string(result, "ann_div_fnote", pzAssets->sAnnDivFnote, sizeof(pzAssets->sAnnDivFnote));
            pzAssets->lCurPrdate = timestamp_to_long(result.get<nanodbc::timestamp>("cur_prdate"));

            pzAssets->fCurClose = result.get<double>("cur_close", 0.0);
            pzAssets->fCurBid = result.get<double>("cur_bid", 0.0);
            pzAssets->fCurAsk = result.get<double>("cur_ask", 0.0);
            read_string(result, "cur_source", pzAssets->sCurSource, sizeof(pzAssets->sCurSource));
            read_string(result, "cur_pr_exch", pzAssets->sCurPrExch, sizeof(pzAssets->sCurPrExch));
            pzAssets->fCurExrate = result.get<double>("cur_exrate", 0.0);
            pzAssets->fCurIncExrate = result.get<double>("cur_inc_exrate", 0.0);

            pzAssets->lPriorPrdate = timestamp_to_long(result.get<nanodbc::timestamp>("prior_prdate"));
            pzAssets->fPriorClose = result.get<double>("prior_close", 0.0);
            pzAssets->fPriorBid = result.get<double>("prior_bid", 0.0);
            pzAssets->fPriorAsk = result.get<double>("prior_ask", 0.0);
            read_string(result, "prior_source", pzAssets->sPriorSource, sizeof(pzAssets->sPriorSource));
            read_string(result, "prior_pr_exch", pzAssets->sPriorPrExch, sizeof(pzAssets->sPriorPrExch));

            pzAssets->fPriorExrate = result.get<double>("prior_exrate", 0.0);
            pzAssets->fPriorIncExrate = result.get<double>("prior_inc_exrate", 0.0);
            read_string(result, "dtc_flag", pzAssets->sDtcFlag, sizeof(pzAssets->sDtcFlag));
            read_string(result, "cns_flag", pzAssets->sCnsFlag, sizeof(pzAssets->sCnsFlag));
            read_string(result, "market_maker", pzAssets->sMarketMaker, sizeof(pzAssets->sMarketMaker));
            read_string(result, "issue_status", pzAssets->sIssueStatus, sizeof(pzAssets->sIssueStatus));

            read_string(result, "owner_grp", pzAssets->sOwnerGrp, sizeof(pzAssets->sOwnerGrp));
            pzAssets->lCreateDate = timestamp_to_long(result.get<nanodbc::timestamp>("create_date"));
            read_string(result, "created_by", pzAssets->sCreatedBy, sizeof(pzAssets->sCreatedBy));
            pzAssets->lDatePriced = timestamp_to_long(result.get<nanodbc::timestamp>("date_priced"));
            pzAssets->lModifyDate = timestamp_to_long(result.get<nanodbc::timestamp>("modify_date"));
            read_string(result, "modified_by", pzAssets->sModifiedBy, sizeof(pzAssets->sModifiedBy));

            pzAssets->lDeleteDate = timestamp_to_long(result.get<nanodbc::timestamp>("delete_date"));
            pzAssets->iIndustLevel1 = result.get<int>("SegId1", 0);
            pzAssets->iIndustLevel2 = result.get<int>("SegId2", 0);
            pzAssets->iIndustLevel3 = result.get<int>("SegId3", 0);
            read_string(result, "sic", pzAssets->sSic, sizeof(pzAssets->sSic));
            read_string(result, "country_iss", pzAssets->sCountryIss, sizeof(pzAssets->sCountryIss));

            read_string(result, "country_isr", pzAssets->sCountryIsr, sizeof(pzAssets->sCountryIsr));
            read_string(result, "euro_clear", pzAssets->sEuroClear, sizeof(pzAssets->sEuroClear));
            pzAssets->iVolume = result.get<int>("volume", 0);
            pzAssets->fInvPct = result.get<double>("inv_pct", 0.0);
            pzAssets->iID = result.get<int>("id", 0);
            read_string(result, "benchmark_secno", pzAssets->sBenchmarkSecNo, sizeof(pzAssets->sBenchmarkSecNo));

            // Update cache
            memcpy(&zSavedAsset, pzAssets, sizeof(zSavedAsset));
            bAssetIsValid = true;
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAsset: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectAsset", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAsset", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectAsset", FALSE);
    }
}
