#include "DigenerateOps.h"
#include "DigenerateQueries.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <optional>
#include <memory>
#include <cstring>
#include <string>
#include <stdio.h>
#include "PaymentsIO.h"
#include "accdiv.h"
#include "divhist.h"
#include "fwtrans.h"

extern thread_local nanodbc::connection gConn;
extern char sHoldings[];

// ============================================================================
// Helper Functions
// ============================================================================

static void FillDivintUnloadStruct(nanodbc::result& result, DILIBSTRUCT* pzDL)
{
    memset(pzDL, 0, sizeof(DILIBSTRUCT));
    
    read_string(result, "table_name", pzDL->sTableName, sizeof(pzDL->sTableName));
    pzDL->iID = result.get<int>("p1", 0);
    read_string(result, "p2", pzDL->sSecNo, sizeof(pzDL->sSecNo));
    read_string(result, "p3", pzDL->sWi, sizeof(pzDL->sWi));
    read_string(result, "p4", pzDL->sSecXtend, sizeof(pzDL->sSecXtend));
    read_string(result, "sec_symbol", pzDL->sSecSymbol, sizeof(pzDL->sSecSymbol));
    read_string(result, "p5", pzDL->sAcctType, sizeof(pzDL->sAcctType));
    pzDL->lTransNo = result.get<long>("p6", 0);
    pzDL->fUnits = result.get<double>("units", 0.0);
    
    pzDL->lEffDate = timestamp_to_long(result.get<nanodbc::timestamp>("eff_date"));
    pzDL->lEligDate = timestamp_to_long(result.get<nanodbc::timestamp>("elig_date"));
    pzDL->lStlDate = timestamp_to_long(result.get<nanodbc::timestamp>("stl_date"));
    
    pzDL->fOrigFace = result.get<double>("orig_face", 0.0);
    pzDL->fOrigYield = result.get<double>("orig_yield", 0.0);
    
    pzDL->lEffMatDate = timestamp_to_long(result.get<nanodbc::timestamp>("eff_mat_date"));
    pzDL->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);
    
    read_string(result, "orig_trans_type", pzDL->sOrigTransType, sizeof(pzDL->sOrigTransType));
    pzDL->lCreateDate = timestamp_to_long(result.get<nanodbc::timestamp>("p9")); // create_date/trd_date alias
    read_string(result, "safek_ind", pzDL->sSafekInd, sizeof(pzDL->sSafekInd));
    // p8 is create_trans_no/trans_no, not used in struct? Original code had comment "it is dummy field"
    
    pzDL->iSecID = result.get<int>("secid", 0);
    pzDL->fTrdUnit = result.get<double>("trad_unit", 0.0);
    pzDL->iSecType = result.get<int>("sec_type", 0);
    read_string(result, "auto_action", pzDL->sAutoAction, sizeof(pzDL->sAutoAction));
    read_string(result, "auto_divint", pzDL->sAutoDivint, sizeof(pzDL->sAutoDivint));
    
    read_string(result, "curr_id", pzDL->sCurrId, sizeof(pzDL->sCurrId));
    read_string(result, "inc_curr_id", pzDL->sIncCurrId, sizeof(pzDL->sIncCurrId));
    pzDL->fCurExrate = result.get<double>("cur_exrate", 0.0);
    pzDL->fCurIncExrate = result.get<double>("cur_inc_exrate", 0.0);
    
    pzDL->lDivintNo = result.get<long>("divint_no", 0);
    read_string(result, "div_type", pzDL->sDivType, sizeof(pzDL->sDivType));
    
    pzDL->lExDate = timestamp_to_long(result.get<nanodbc::timestamp>("ex_date"));
    pzDL->lRecDate = timestamp_to_long(result.get<nanodbc::timestamp>("rec_date"));
    pzDL->lPayDate = timestamp_to_long(result.get<nanodbc::timestamp>("pay_date"));
    
    pzDL->fDivRate = result.get<double>("div_rate", 0.0);
    read_string(result, "post_status", pzDL->sPostStatus, sizeof(pzDL->sPostStatus));
    pzDL->lDivCreateDate = timestamp_to_long(result.get<nanodbc::timestamp>("create_date"));
    pzDL->lModifyDate = timestamp_to_long(result.get<nanodbc::timestamp>("modify_date"));
    
    pzDL->lFwdDivintNo = result.get<long>("fwd_divint_no", 0);
    pzDL->lPrevDivintNo = result.get<long>("prev_divint_no", 0);
    read_string(result, "delete_flag", pzDL->sDeleteFlag, sizeof(pzDL->sDeleteFlag));
    
    read_string(result, "process_flag", pzDL->sProcessFlag, sizeof(pzDL->sProcessFlag));
    read_string(result, "process_type", pzDL->sProcessType, sizeof(pzDL->sProcessType));
    read_string(result, "short_settle", pzDL->sShortSettle, sizeof(pzDL->sShortSettle));
    read_string(result, "incl_units", pzDL->sInclUnits, sizeof(pzDL->sInclUnits));
    read_string(result, "split_ind", pzDL->sSplitInd, sizeof(pzDL->sSplitInd));
    
    read_string(result, "tran_type", pzDL->sTranType, sizeof(pzDL->sTranType));
    pzDL->lDivTransNo = result.get<long>("div_trans_no", 0);
    read_string(result, "tran_location", pzDL->sTranLocation, sizeof(pzDL->sTranLocation));
    read_string(result, "tran_type", pzDL->sDTTranType, sizeof(pzDL->sDTTranType)); // Alias conflict? Last column is dt.tran_type
    
    if (strlen(pzDL->sTranType) == 0 && pzDL->lDivTransNo == 0 && strlen(pzDL->sTranLocation) == 0)
        pzDL->bNullDivhist = TRUE; 
    else
        pzDL->bNullDivhist = FALSE; 
}

// ============================================================================
// InsertAccdiv
// ============================================================================
DLLAPI void InsertAccdiv(ACCDIV *zAccdiv, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
		int p = 0;
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_InsertAccdiv));
        stmt.bind(p++, &zAccdiv->iID);
        stmt.bind(p++, &zAccdiv->lTransNo);
        stmt.bind(p++, &zAccdiv->lDivintNo);
        stmt.bind(p++, (const char*)zAccdiv->sTranType);
        
        stmt.bind(p++, (const char*)zAccdiv->sSecNo);
        stmt.bind(p++, (const char*)zAccdiv->sWi);
        stmt.bind(p++, &zAccdiv->iSecID);
        stmt.bind(p++, (const char*)zAccdiv->sSecXtend);
        stmt.bind(p++, (const char*)zAccdiv->sAcctType);
        
        nanodbc::timestamp tsEligDate; long_to_timestamp(zAccdiv->lEligDate, tsEligDate);
        stmt.bind(p++, &tsEligDate);
        stmt.bind(p++, (const char*)zAccdiv->sSecSymbol);
        
        stmt.bind(p++, (const char*)zAccdiv->sDivType);
        double fDivFactor = RoundDouble(zAccdiv->fDivFactor, 7); stmt.bind(p++, &fDivFactor);
        double fUnits = RoundDouble(zAccdiv->fUnits, 5); stmt.bind(p++, &fUnits);
        double fOrigFace = RoundDouble(zAccdiv->fOrigFace, 3); stmt.bind(p++, &fOrigFace);
        double fPcplAmt = RoundDouble(zAccdiv->fPcplAmt, 2); stmt.bind(p++, &fPcplAmt);
        
        double fIncomeAmt = RoundDouble(zAccdiv->fIncomeAmt, 2); stmt.bind(p++, &fIncomeAmt);
        nanodbc::timestamp tsTrdDate; long_to_timestamp(zAccdiv->lTrdDate, tsTrdDate); stmt.bind(p++, &tsTrdDate);
        nanodbc::timestamp tsStlDate; long_to_timestamp(zAccdiv->lStlDate, tsStlDate); stmt.bind(p++, &tsStlDate);
        nanodbc::timestamp tsEffDate; long_to_timestamp(zAccdiv->lEffDate, tsEffDate); stmt.bind(p++, &tsEffDate);
        nanodbc::timestamp tsEntryDate; long_to_timestamp(zAccdiv->lEntryDate, tsEntryDate); stmt.bind(p++, &tsEntryDate);
        
        stmt.bind(p++, (const char*)zAccdiv->sCurrId);
        stmt.bind(p++, (const char*)zAccdiv->sCurrAcctType);
        stmt.bind(p++, (const char*)zAccdiv->sIncCurrId);
        stmt.bind(p++, (const char*)zAccdiv->sIncAcctType);
        stmt.bind(p++, (const char*)zAccdiv->sSecCurrId);
        stmt.bind(p++, (const char*)zAccdiv->sAccrCurrId);
        
        double fBaseXrate = RoundDouble(zAccdiv->fBaseXrate, 12); stmt.bind(p++, &fBaseXrate);
        double fIncBaseXrate = RoundDouble(zAccdiv->fIncBaseXrate, 12); stmt.bind(p++, &fIncBaseXrate);
        double fSecBaseXrate = RoundDouble(zAccdiv->fSecBaseXrate, 12); stmt.bind(p++, &fSecBaseXrate);
        double fAccrBaseXrate = RoundDouble(zAccdiv->fAccrBaseXrate, 12); stmt.bind(p++, &fAccrBaseXrate);
        double fSysXrate = RoundDouble(zAccdiv->fSysXrate, 12); stmt.bind(p++, &fSysXrate);
        
        double fIncSysXrate = RoundDouble(zAccdiv->fIncSysXrate, 12); stmt.bind(p++, &fIncSysXrate);
        double fOrigYld = RoundDouble(zAccdiv->fOrigYld, 6); stmt.bind(p++, &fOrigYld);
        nanodbc::timestamp tsEffMatDate; long_to_timestamp(zAccdiv->lEffMatDate, tsEffMatDate); stmt.bind(p++, &tsEffMatDate);
        double fEffMatPrice = RoundDouble(zAccdiv->fEffMatPrice, 6); stmt.bind(p++, &fEffMatPrice);
        stmt.bind(p++, (const char*)zAccdiv->sAcctMthd);
        
        stmt.bind(p++, (const char*)zAccdiv->sTransSrce);
        stmt.bind(p++, (const char*)zAccdiv->sDrCr);
        stmt.bind(p++, (const char*)zAccdiv->sDtcInclusion);
        stmt.bind(p++, (const char*)zAccdiv->sDtcResolve);
        stmt.bind(p++, (const char*)zAccdiv->sIncomeFlag);
        
        stmt.bind(p++, (const char*)zAccdiv->sLetterFlag);
        stmt.bind(p++, (const char*)zAccdiv->sLedgerFlag);
        stmt.bind(p++, (const char*)zAccdiv->sCreatedBy);
        nanodbc::timestamp tsCreateDate; long_to_timestamp(zAccdiv->lCreateDate, tsCreateDate); stmt.bind(p++, &tsCreateDate);
        stmt.bind(p++, (const char*)zAccdiv->sCreateTime);
        
        stmt.bind(p++, (const char*)zAccdiv->sSuspendFlag);
        stmt.bind(p++, (const char*)zAccdiv->sDeleteFlag);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "InsertAccdiv", FALSE);
    }
}

// ============================================================================
// UpdateAccdiv
// ============================================================================
DLLAPI void UpdateAccdiv(ACCDIV *zAccdiv, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UpdateAccdiv));

        int p = 0;
        stmt.bind(p++, (const char*)zAccdiv->sDivType);
        double fDivFactor = RoundDouble(zAccdiv->fDivFactor, 7); stmt.bind(p++, &fDivFactor);
        double fPcplAmt = RoundDouble(zAccdiv->fPcplAmt, 2); stmt.bind(p++, &fPcplAmt);
        double fIncomeAmt = RoundDouble(zAccdiv->fIncomeAmt, 2); stmt.bind(p++, &fIncomeAmt);
        nanodbc::timestamp tsTrdDate; long_to_timestamp(zAccdiv->lTrdDate, tsTrdDate); stmt.bind(p++, &tsTrdDate);
        nanodbc::timestamp tsStlDate; long_to_timestamp(zAccdiv->lStlDate, tsStlDate); stmt.bind(p++, &tsStlDate);
        nanodbc::timestamp tsEffDate; long_to_timestamp(zAccdiv->lEffDate, tsEffDate); stmt.bind(p++, &tsEffDate);
        
        stmt.bind(p++, &zAccdiv->iID);
        stmt.bind(p++, &zAccdiv->lDivintNo);
        stmt.bind(p++, &zAccdiv->lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "UpdateAccdiv", FALSE);
    }
}

// ============================================================================
// InsertDivhist
// ============================================================================
DLLAPI void InsertDivhist(DIVHIST *zDH, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_InsertDivhist));

        int p = 0;
        stmt.bind(p++, &zDH->iID);
        stmt.bind(p++, &zDH->lTransNo);
        stmt.bind(p++, &zDH->lDivintNo);
        stmt.bind(p++, (const char*)zDH->sTranType);
        
        stmt.bind(p++, (const char*)zDH->sSecNo);
        stmt.bind(p++, (const char*)zDH->sWi);
        stmt.bind(p++, &zDH->iSecID);
        stmt.bind(p++, (const char*)zDH->sSecXtend);
        stmt.bind(p++, (const char*)zDH->sAcctType);
        
        double fUnits = RoundDouble(zDH->fUnits, 5); stmt.bind(p++, &fUnits);
        stmt.bind(p++, &zDH->lDivTransNo);
        
        stmt.bind(p++, (const char*)zDH->sTranLocation);
        nanodbc::timestamp tsExDate; long_to_timestamp(zDH->lExDate, tsExDate); stmt.bind(p++, &tsExDate);
        nanodbc::timestamp tsPayDate; long_to_timestamp(zDH->lPayDate, tsPayDate); stmt.bind(p++, &tsPayDate);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "InsertDivhist", FALSE);
    }
}

// ============================================================================
// UpdateDivhist
// ============================================================================
DLLAPI void UpdateDivhist(int iID, long lDivintNo, long lTransNo, double fUnits, long lExDate, long lPayDate, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UpdateDivhist));

        int p = 0;
        double fUnitsRounded = RoundDouble(fUnits, 5); stmt.bind(p++, &fUnitsRounded);
        nanodbc::timestamp tsExDate; long_to_timestamp(lExDate, tsExDate); stmt.bind(p++, &tsExDate);
        nanodbc::timestamp tsPayDate; long_to_timestamp(lPayDate, tsPayDate); stmt.bind(p++, &tsPayDate);
        
        stmt.bind(p++, &iID);
        stmt.bind(p++, &lDivintNo);
        stmt.bind(p++, &lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "UpdateDivhist", FALSE);
    }
}

// ============================================================================
// DeleteDivhist
// ============================================================================
DLLAPI void DeleteDivhist(int iID, long lDivintNo, long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_DeleteDivhist));

        int p = 0;
        stmt.bind(p++, &iID);
        stmt.bind(p++, &lDivintNo);
        stmt.bind(p++, &lTransNo);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "DeleteDivhist", FALSE);
    }
}

// ============================================================================
// DeleteFWTrans
// ============================================================================
DLLAPI void DeleteFWTrans(int iID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_DeleteFWTrans));

        int p = 0;
        stmt.bind(p++, &iID);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "DeleteFWTrans", FALSE);
    }
}

// ============================================================================
// InsertFWTrans
// ============================================================================
DLLAPI void InsertFWTrans(FWTRANS *zFWTrans, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_InsertFWTrans));

        int p = 0;
        stmt.bind(p++, &zFWTrans->iID);
        stmt.bind(p++, &zFWTrans->lTransNo);
        stmt.bind(p++, &zFWTrans->lDivintNo);
        stmt.bind(p++, (const char*)zFWTrans->sTranType);
        
        stmt.bind(p++, (const char*)zFWTrans->sSecNo);
        stmt.bind(p++, (const char*)zFWTrans->sWi);
        stmt.bind(p++, &zFWTrans->iSecID);
        stmt.bind(p++, (const char*)zFWTrans->sSecXtend);
        stmt.bind(p++, (const char*)zFWTrans->sAcctType);
        
        stmt.bind(p++, (const char*)zFWTrans->sDivType);
        double fDivFactor = RoundDouble(zFWTrans->fDivFactor, 7); stmt.bind(p++, &fDivFactor);
        double fUnits = RoundDouble(zFWTrans->fUnits, 5); stmt.bind(p++, &fUnits);
        double fPcplAmt = RoundDouble(zFWTrans->fPcplAmt, 2); stmt.bind(p++, &fPcplAmt);
        
        double fIncomeAmt = RoundDouble(zFWTrans->fIncomeAmt, 2); stmt.bind(p++, &fIncomeAmt);
        nanodbc::timestamp tsTrdDate; long_to_timestamp(zFWTrans->lTrdDate, tsTrdDate); stmt.bind(p++, &tsTrdDate);
        nanodbc::timestamp tsStlDate; long_to_timestamp(zFWTrans->lStlDate, tsStlDate); stmt.bind(p++, &tsStlDate);
        nanodbc::timestamp tsEffDate; long_to_timestamp(zFWTrans->lEffDate, tsEffDate); stmt.bind(p++, &tsEffDate);
        
        stmt.bind(p++, (const char*)zFWTrans->sCurrId);
        stmt.bind(p++, (const char*)zFWTrans->sIncCurrId);
        stmt.bind(p++, (const char*)zFWTrans->sIncAcctType);
        stmt.bind(p++, (const char*)zFWTrans->sSecCurrId);
        stmt.bind(p++, (const char*)zFWTrans->sAccrCurrId);
        
        double fBaseXrate = RoundDouble(zFWTrans->fBaseXrate, 12); stmt.bind(p++, &fBaseXrate);
        double fIncBaseXrate = RoundDouble(zFWTrans->fIncBaseXrate, 12); stmt.bind(p++, &fIncBaseXrate);
        double fSecBaseXrate = RoundDouble(zFWTrans->fSecBaseXrate, 12); stmt.bind(p++, &fSecBaseXrate);
        double fAccrBaseXrate = RoundDouble(zFWTrans->fAccrBaseXrate, 12); stmt.bind(p++, &fAccrBaseXrate);
        double fSysXrate = RoundDouble(zFWTrans->fSysXrate, 12); stmt.bind(p++, &fSysXrate);
        
        double fIncSysXrate = RoundDouble(zFWTrans->fIncSysXrate, 12); stmt.bind(p++, &fIncSysXrate);
        stmt.bind(p++, (const char*)zFWTrans->sDrCr);
        nanodbc::timestamp tsCreateDate; long_to_timestamp(zFWTrans->lCreateDate, tsCreateDate); stmt.bind(p++, &tsCreateDate);
        stmt.bind(p++, (const char*)zFWTrans->sCreateTime);
        stmt.bind(p++, (const char*)zFWTrans->sDescription);

        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "InsertFWTrans", FALSE);
    }
}

// ============================================================================
// DivintUnload
// ============================================================================

struct DivintUnloadState {
    std::optional<nanodbc::result> result;
    char sMode[2] = {0};
    char sType[2] = {0};
    long lStartExDate = 0;
    long lEndExDate = 0;
    int iID = 0;
    int iEndId = 0;
    char sSecNo[20] = {0};
    char sWi[2] = {0};
    char sSecXtend[3] = {0};
    char sAcctType[2] = {0};
    char sHoldingsTable[40] = {0};
    int cRows = 0;
};

static DivintUnloadState g_divintUnloadState;

DLLAPI void DivintUnload(DILIBSTRUCT *pzDL, long lStartExDate, long lEndExDate, char *sMode, char *sType,
                            int iID, int iEndId, char *sSecNo, char *sWi, char *sSecXtend, char *sAcctType,
                            long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        bool needNewQuery = !(
            g_divintUnloadState.sMode[0] == sMode[0] &&
            g_divintUnloadState.sType[0] == sType[0] &&
            g_divintUnloadState.lStartExDate == lStartExDate &&
            g_divintUnloadState.lEndExDate == lEndExDate &&
            g_divintUnloadState.iID == iID &&
            strcmp(g_divintUnloadState.sHoldingsTable, sHoldings) == 0 &&
            (sMode[0] != 'B' || g_divintUnloadState.iEndId == iEndId) &&
            (sMode[0] != 'S' || (
                strcmp(g_divintUnloadState.sSecNo, sSecNo) == 0 &&
                strcmp(g_divintUnloadState.sWi, sWi) == 0 &&
                strcmp(g_divintUnloadState.sSecXtend, sSecXtend) == 0 &&
                strcmp(g_divintUnloadState.sAcctType, sAcctType) == 0
            )) &&
            g_divintUnloadState.cRows > 0
        );

        if (needNewQuery)
        {
            g_divintUnloadState.result.reset();
            g_divintUnloadState.cRows = 0;

            // Allocate sSQL on the heap to reduce stack usage
            std::unique_ptr<char[]> sSQL(new char[MAXSQLSIZE]);
            BuildDivintUnloadSQL(sSQL.get(), sHoldings, sType, sMode[0]);

            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(sSQL.get()));

            int p = 0;
            nanodbc::timestamp tsStartExDate; long_to_timestamp(lStartExDate, tsStartExDate);
            nanodbc::timestamp tsEndExDate; long_to_timestamp(lEndExDate, tsEndExDate);

            // Bind parameters based on mode and query structure (3 subqueries)
            // The logic in BuildDivintUnloadSQL constructs 3 subqueries (UNION ALL)
            // Each subquery needs parameters.
            
            // Subquery 1: PAID
            stmt.bind(p++, &tsStartExDate);
            stmt.bind(p++, &tsEndExDate);
            if (sMode[0] == 'B') { stmt.bind(p++, &iID); stmt.bind(p++, &iEndId); }
            else if (sMode[0] == 'A') { stmt.bind(p++, &iID); }
            else if (sMode[0] == 'S') { 
                stmt.bind(p++, &iID); stmt.bind(p++, sSecNo); stmt.bind(p++, sWi); 
                stmt.bind(p++, sSecXtend); stmt.bind(p++, sAcctType); 
            }

            // Subquery 2: TOPAY
            stmt.bind(p++, &tsStartExDate);
            stmt.bind(p++, &tsEndExDate);
            if (sMode[0] == 'B') { stmt.bind(p++, &iID); stmt.bind(p++, &iEndId); }
            else if (sMode[0] == 'A') { stmt.bind(p++, &iID); }
            else if (sMode[0] == 'S') { 
                stmt.bind(p++, &iID); stmt.bind(p++, sSecNo); stmt.bind(p++, sWi); 
                stmt.bind(p++, sSecXtend); stmt.bind(p++, sAcctType); 
            }

            // Subquery 3: DIVINT_UNLOAD_2
            stmt.bind(p++, &tsStartExDate);
            stmt.bind(p++, &tsEndExDate);
            if (sMode[0] == 'B') { stmt.bind(p++, &iID); stmt.bind(p++, &iEndId); }
            else if (sMode[0] == 'A') { stmt.bind(p++, &iID); }
            else if (sMode[0] == 'S') { 
                stmt.bind(p++, &iID); stmt.bind(p++, sSecNo); stmt.bind(p++, sWi); 
                stmt.bind(p++, sSecXtend); stmt.bind(p++, sAcctType); 
            }

            g_divintUnloadState.result = nanodbc::execute(stmt);
            
            // Update state
            g_divintUnloadState.sMode[0] = sMode[0];
            g_divintUnloadState.sType[0] = sType[0];
            g_divintUnloadState.lStartExDate = lStartExDate;
            g_divintUnloadState.lEndExDate = lEndExDate;
            g_divintUnloadState.iID = iID;
            g_divintUnloadState.iEndId = iEndId;
            strcpy_s(g_divintUnloadState.sHoldingsTable, sHoldings);
            if (sMode[0] == 'S') {
                strcpy_s(g_divintUnloadState.sSecNo, sSecNo);
                strcpy_s(g_divintUnloadState.sWi, sWi);
                strcpy_s(g_divintUnloadState.sSecXtend, sSecXtend);
                strcpy_s(g_divintUnloadState.sAcctType, sAcctType);
            }
        }

        if (g_divintUnloadState.result && g_divintUnloadState.result->next())
        {
            g_divintUnloadState.cRows++;
            FillDivintUnloadStruct(*g_divintUnloadState.result, pzDL);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_divintUnloadState.cRows = 0;
            g_divintUnloadState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "DivintUnload", FALSE);
        g_divintUnloadState.result.reset();
    }
}

// Internal function to clear state, called by DigenerateUtils
void FreeDivintUnloadState()
{
    g_divintUnloadState.result.reset();
    g_divintUnloadState.cRows = 0;
}
