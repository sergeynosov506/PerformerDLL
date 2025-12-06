/**
 * 
 * SUB-SYSTEM: Composite Merge Selectors
 * 
 * FILENAME: MergeSelectors.cpp
 * 
 * DESCRIPTION: Selector functions for composite member and segment selection
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templ ates
 * 
 * NOTES: Maintains iterative calling pattern with static cursor state
 *        All DLLAPI signatures preserved for binary compatibility
 *        
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#include "MergeSelectors.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "MergeQueries.h"
#include "ValuationIO_Holdings.h"
#include <cstring>
#include <optional>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// Static state for multi-row iterative selectors
// These maintain cursor state between successive calls
// ============================================================================

struct MemberSelectorState {
    std::optional<nanodbc::result> result;
    int iOwnerID = 0;
    long lDate = 0;
    int cRows = 0;
};

struct SegmainSelectorState {
    std::optional<nanodbc::result> result;
    int iOwnerID = 0;
    int cRows = 0;
};

struct TransSelectorState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    long lEffDate1 = 0;
    long lEffDate2 = 0;
    int cRows = 0;
};

static MemberSelectorState g_memberState;
static SegmainSelectorState g_segmainState;
static TransSelectorState g_transState;

// ============================================================================
// SelectAllMembersOfAComposite
// ============================================================================

DLLAPI void STDCALL SelectAllMembersOfAComposite(int iOwnerID, long lDate, int *piID, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectAllMembersOfAComposite", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllMembersOfAComposite", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed or if this is a new query
        bool needNewQuery = !(g_memberState.iOwnerID == iOwnerID && 
                             g_memberState.lDate == lDate && 
                             g_memberState.cRows > 0);

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_memberState.cRows = 0;
            g_memberState.result.reset();

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectAllMembersOfAComposite));

            // Convert date
            nanodbc::timestamp tsDate;
            long_to_timestamp(lDate, tsDate);

            // Bind parameters
            stmt.bind(0, &iOwnerID);
            stmt.bind(1, &tsDate);

            // Execute and store result
            g_memberState.result = nanodbc::execute(stmt);
            g_memberState.iOwnerID = iOwnerID;
            g_memberState.lDate = lDate;
        }

        // Try to get next row
        if (g_memberState.result && g_memberState.result->next())
        {
            g_memberState.cRows++;
            *piID = g_memberState.result->get<int>(0, 0);  // id column
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            *piID = 0;
            g_memberState.cRows = 0;
            g_memberState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAllMembersOfAComposite: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllMembersOfAComposite", FALSE);
        g_memberState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAllMembersOfAComposite", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllMembersOfAComposite", FALSE);
        g_memberState.result.reset();
    }
}

// ============================================================================
// SelectSegmainForPortfolio
// ============================================================================

DLLAPI void STDCALL SelectSegmainForPortfolio(SEGMAIN *pzSegmain, int iID, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectSegmainForPortfolio", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmainForPortfolio", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed or if this is a new query
        bool needNewQuery = !(g_segmainState.iOwnerID == iID && g_segmainState.cRows > 0);

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_segmainState.cRows = 0;
            g_segmainState.result.reset();

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectSegmainForPortfolio));

            // Bind parameter
            stmt.bind(0, &iID);

            // Execute and store result
            g_segmainState.result = nanodbc::execute(stmt);
            g_segmainState.iOwnerID = iID;
        }

        // Try to get next row
        if (g_segmainState.result && g_segmainState.result->next())
        {
            g_segmainState.cRows++;

            // Clear output buffer before filling
            memset(pzSegmain, 0, sizeof(SEGMAIN));

            // Fill SEGMAIN struct
            pzSegmain->iID = g_segmainState.result->get<int>("id", 0);
            pzSegmain->iOwnerID = g_segmainState.result->get<int>("owner_id", 0);
            pzSegmain->iSegmentTypeID = g_segmainState.result->get<int>("segmenttype_id", 0);
            read_string(*g_segmainState.result, "segment_name", pzSegmain->sSegmentName, sizeof(pzSegmain->sSegmentName));
            read_string(*g_segmainState.result, "segment_abbrev", pzSegmain->sSegmentAbbrev, sizeof(pzSegmain->sSegmentAbbrev));
            read_string(*g_segmainState.result, "isinactive", pzSegmain->sIsInactive, sizeof(pzSegmain->sIsInactive));
            pzSegmain->iSegLevel = g_segmainState.result->get<short>("seglevel", 0);
            read_string(*g_segmainState.result, "calculated", pzSegmain->sCalculated, sizeof(pzSegmain->sCalculated));
            pzSegmain->iSequenceNo = g_segmainState.result->get<short>("sequence_no", 0);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_segmainState.cRows = 0;
            g_segmainState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectSegmainForPortfolio: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmainForPortfolio", FALSE);
        g_segmainState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectSegmainForPortfolio", iID, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmainForPortfolio", FALSE);
        g_segmainState.result.reset();
    }
}

// ============================================================================
// SelectTransFor - Helper to fill TRANS struct
// ============================================================================

static void FillTransStruct(nanodbc::result& result, TRANS* pzTR)
{
    // Clear output buffer
    memset(pzTR, 0, sizeof(TRANS));

    // Fill all fields from result set
    pzTR->iID = result.get<int>("id", 0);
    pzTR->lTransNo = result.get<long>("trans_no", 0);
    read_string(result, "tran_type", pzTR->sTranType, sizeof(pzTR->sTranType));
    read_string(result, "sec_no", pzTR->sSecNo, sizeof(pzTR->sSecNo));
    read_string(result, "wi", pzTR->sWi, sizeof(pzTR->sWi));

    read_string(result, "sec_xtend", pzTR->sSecXtend, sizeof(pzTR->sSecXtend));
    read_string(result, "acct_type", pzTR->sAcctType, sizeof(pzTR->sAcctType));
    pzTR->iSecID = result.get<int>("secid", 0);
    read_string(result, "sec_symbol", pzTR->sSecSymbol, sizeof(pzTR->sSecSymbol));
    pzTR->fUnits = result.get<double>("units", 0.0);

    pzTR->fOrigFace = result.get<double>("orig_face", 0.0);
    pzTR->fUnitCost = result.get<double>("unit_cost", 0.0);
    pzTR->fTotCost = result.get<double>("tot_cost", 0.0);
    pzTR->fOrigCost = result.get<double>("orig_cost", 0.0);

    pzTR->fPcplAmt = result.get<double>("pcpl_amt", 0.0);
    pzTR->fOptPrem = result.get<double>("opt_prem", 0.0);
    pzTR->fAmortVal = result.get<double>("amort_val", 0.0);
    pzTR->fBasisAdj = result.get<double>("basis_adj", 0.0);

    pzTR->fCommGcr = result.get<double>("comm_gcr", 0.0);
    pzTR->fNetComm = result.get<double>("net_comm", 0.0);
    read_string(result, "comm_code", pzTR->sCommCode, sizeof(pzTR->sCommCode));
    pzTR->fSecFees = result.get<double>("sec_fees", 0.0);

    pzTR->fMiscFee1 = result.get<double>("misc_fee1", 0.0);
    read_string(result, "fee_code1", pzTR->sFeeCode1, sizeof(pzTR->sFeeCode1));
    pzTR->fMiscFee2 = result.get<double>("misc_fee2", 0.0);
    read_string(result, "fee_code2", pzTR->sFeeCode2, sizeof(pzTR->sFeeCode2));

    pzTR->fAccrInt = result.get<double>("accr_int", 0.0);
    pzTR->fIncomeAmt = result.get<double>("income_amt", 0.0);
    pzTR->fNetFlow = result.get<double>("net_flow", 0.0);
    read_string(result, "broker_code", pzTR->sBrokerCode, sizeof(pzTR->sBrokerCode));

    read_string(result, "broker_code2", pzTR->sBrokerCode2, sizeof(pzTR->sBrokerCode2));
    
    // Date fields - convert from timestamp to long
    read_date(result, "trd_date", &pzTR->lTrdDate);
    read_date(result, "stl_date", &pzTR->lStlDate);
    read_date(result, "eff_date", &pzTR->lEffDate);

    read_date(result, "entry_date", &pzTR->lEntryDate);
    pzTR->lTaxlotNo = result.get<long>("taxlot_no", 0);
    pzTR->lXrefTransNo = result.get<long>("xref_trans_no", 0);

    pzTR->lPendDivNo = result.get<long>("pend_div_no", 0);
    pzTR->lRevTransNo = result.get<long>("rev_trans_no", 0);
    read_string(result, "rev_type", pzTR->sRevType, sizeof(pzTR->sRevType));

    pzTR->lNewTransNo = result.get<long>("new_trans_no", 0);
    pzTR->lOrigTransNo = result.get<long>("orig_trans_no", 0);
    pzTR->lBlockTransNo = result.get<long>("block_trans_no", 0);

    read_string(result, "x_id", pzTR->sXID, sizeof(pzTR->sXID));
    pzTR->lXTransNo = result.get<long>("x_trans_no", 0);
    read_string(result, "x_sec_no", pzTR->sXSecNo, sizeof(pzTR->sXSecNo));
    read_string(result, "x_wi", pzTR->sXWi, sizeof(pzTR->sXWi));

    read_string(result, "x_sec_xtend", pzTR->sXSecXtend, sizeof(pzTR->sXSecXtend));
    read_string(result, "x_acct_type", pzTR->sXAcctType, sizeof(pzTR->sXAcctType));
    pzTR->iXSecID = result.get<int>("x_secid", 0);
    read_string(result, "curr_id", pzTR->sCurrId, sizeof(pzTR->sCurrId));

    read_string(result, "curr_acct_type", pzTR->sCurrAcctType, sizeof(pzTR->sCurrAcctType));
    read_string(result, "inc_curr_id", pzTR->sIncCurrId, sizeof(pzTR->sIncCurrId));
    read_string(result, "inc_acct_type", pzTR->sIncAcctType, sizeof(pzTR->sIncAcctType));

    read_string(result, "x_curr_id", pzTR->sXCurrId, sizeof(pzTR->sXCurrId));
    read_string(result, "x_curr_acct_type", pzTR->sXCurrAcctType, sizeof(pzTR->sXCurrAcctType));
    read_string(result, "sec_curr_id", pzTR->sSecCurrId, sizeof(pzTR->sSecCurrId));

    read_string(result, "accr_curr_id", pzTR->sAccrCurrId, sizeof(pzTR->sAccrCurrId));
    pzTR->fBaseXrate = result.get<double>("base_xrate", 0.0);
    pzTR->fIncBaseXrate = result.get<double>("inc_base_xrate", 0.0);

    pzTR->fSecBaseXrate = result.get<double>("sec_base_xrate", 0.0);
    pzTR->fAccrBaseXrate = result.get<double>("accr_base_xrate", 0.0);
    pzTR->fSysXrate = result.get<double>("sys_xrate", 0.0);

    pzTR->fIncSysXrate = result.get<double>("inc_sys_xrate", 0.0);
    pzTR->fBaseOpenXrate = result.get<double>("base_open_xrate", 0.0);
    pzTR->fSysOpenXrate = result.get<double>("sys_open_xrate", 0.0);

    read_date(result, "open_trd_date", &pzTR->lOpenTrdDate);
    read_date(result, "open_stl_date", &pzTR->lOpenStlDate);
    pzTR->fOpenUnitCost = result.get<double>("open_unit_cost", 0.0);

    pzTR->fOrigYld = result.get<double>("orig_yld", 0.0);
    read_date(result, "eff_mat_date", &pzTR->lEffMatDate);
    pzTR->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);

    read_string(result, "acct_mthd", pzTR->sAcctMthd, sizeof(pzTR->sAcctMthd));
    read_string(result, "trans_srce", pzTR->sTransSrce, sizeof(pzTR->sTransSrce));
    read_string(result, "adp_tag", pzTR->sAdpTag, sizeof(pzTR->sAdpTag));
    read_string(result, "div_type", pzTR->sDivType, sizeof(pzTR->sDivType));

    pzTR->fDivFactor = result.get<double>("div_factor", 0.0);
    pzTR->lDivintNo = result.get<long>("divint_no", 0);
    read_date(result, "roll_date", &pzTR->lRollDate);
    read_date(result, "perf_date", &pzTR->lPerfDate);

    read_string(result, "misc_desc_ind", pzTR->sMiscDescInd, sizeof(pzTR->sMiscDescInd));
    read_string(result, "dr_cr", pzTR->sDrCr, sizeof(pzTR->sDrCr));
    read_string(result, "bal_to_adjust", pzTR->sBalToAdjust, sizeof(pzTR->sBalToAdjust));
    read_string(result, "cap_trans", pzTR->sCapTrans, sizeof(pzTR->sCapTrans));

    read_string(result, "safek_ind", pzTR->sSafekInd, sizeof(pzTR->sSafekInd));
    read_string(result, "dtc_inclusion", pzTR->sDtcInclusion, sizeof(pzTR->sDtcInclusion));
    read_string(result, "dtc_resolve", pzTR->sDtcResolve, sizeof(pzTR->sDtcResolve));

    read_string(result, "recon_flag", pzTR->sReconFlag, sizeof(pzTR->sReconFlag));
    read_string(result, "recon_srce", pzTR->sReconSrce, sizeof(pzTR->sReconSrce));
    read_string(result, "income_flag", pzTR->sIncomeFlag, sizeof(pzTR->sIncomeFlag));
    read_string(result, "letter_flag", pzTR->sLetterFlag, sizeof(pzTR->sLetterFlag));

    read_string(result, "ledger_flag", pzTR->sLedgerFlag, sizeof(pzTR->sLedgerFlag));
    read_string(result, "gl_flag", pzTR->sGlFlag, sizeof(pzTR->sGlFlag));
    read_string(result, "created_by", pzTR->sCreatedBy, sizeof(pzTR->sCreatedBy));
    read_date(result, "create_date", &pzTR->lCreateDate);

    read_string(result, "create_time", pzTR->sCreateTime, sizeof(pzTR->sCreateTime));
    read_date(result, "post_date", &pzTR->lPostDate);
    read_string(result, "bkof_frmt", pzTR->sBkofFrmt, sizeof(pzTR->sBkofFrmt));
    pzTR->lBkofSeqNo = result.get<long>("bkof_seq_no", 0);

    pzTR->lDtransNo = result.get<long>("dtrans_no", 0);
    pzTR->fPrice = result.get<double>("price", 0.0);
    pzTR->iRestrictionCode = result.get<int>("restriction_code", 0);
}

// ============================================================================
// SelectTransFor
// ============================================================================

DLLAPI void STDCALL SelectTransFor(TRANS *pzTR, int iID, long lEffDate1, long lEffDate2, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransFor", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransFor", FALSE);
        return;
    }

    try
    {
        // Check if parameters changed or if this is a new query
        bool needNewQuery = !(g_transState.iID == iID && 
                             g_transState.lEffDate1 == lEffDate1 &&
                             g_transState.lEffDate2 == lEffDate2 &&
                             g_transState.cRows > 0);

        if (needNewQuery)
        {
            // Reset state and execute new query
            g_transState.cRows = 0;
            g_transState.result.reset();

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectTransFor));

            // Convert dates
            nanodbc::timestamp tsEffDate1, tsEffDate2;
            long_to_timestamp(lEffDate1, tsEffDate1);
            long_to_timestamp(lEffDate2, tsEffDate2);

            // Bind parameters
            stmt.bind(0, &iID);
            stmt.bind(1, &tsEffDate1);
            stmt.bind(2, &tsEffDate2);

            // Execute and store result
            g_transState.result = nanodbc::execute(stmt);
            g_transState.iID = iID;
            g_transState.lEffDate1 = lEffDate1;
            g_transState.lEffDate2 = lEffDate2;
        }

        // Try to get next row
        if (g_transState.result && g_transState.result->next())
        {
            g_transState.cRows++;
            FillTransStruct(*g_transState.result, pzTR);
        }
        else
        {
            // EOF reached
            pzErr->iSqlError = SQLNOTFOUND;
            g_transState.cRows = 0;
            g_transState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectTransFor: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransFor", FALSE);
        g_transState.result.reset();
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectTransFor", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransFor", FALSE);
        g_transState.result.reset();
    }
}

// ============================================================================
// Helper functions - delegate to ValuationIO
// ============================================================================

extern char sHoldings[];  // From ValuationIO
extern char sHoldcash[];  // From ValuationIO

DLLAPI void STDCALL SelectHoldingsFor(int iID, HOLDINGS *pzHoldings, char *TableName, ERRSTRUCT *pzErr)
{
    char sDLLHoldings[40];

    strcpy_s(sDLLHoldings, sHoldings); // Save Holdings table currently being used in DLL
    strcpy_s(sHoldings, TableName); // set Holdings_table_name internally used in the DLL 
    // call appropriate function from ValuationIO part
    SelectAllHoldingsForAnAccount(iID, pzHoldings, pzErr);
    strcpy_s(sHoldings, sDLLHoldings); // Set current table name back !
}

DLLAPI void STDCALL SelectHoldcashFor(int iID, HOLDCASH *pzHoldcash, char *TableName, ERRSTRUCT *pzErr)
{
    char sDLLHoldcash[40];

    strcpy_s(sDLLHoldcash, sHoldcash); // Save Holdcash table currently being used in DLL
    strcpy_s(sHoldcash, TableName); // set Holdcash_table_name internally used in the DLL 
    // call appropriate function from ValuationIO part
    SelectAllHoldcashForAnAccount(iID, pzHoldcash, pzErr);
    strcpy_s(sHoldcash, sDLLHoldcash); // Set current table name back !
}
