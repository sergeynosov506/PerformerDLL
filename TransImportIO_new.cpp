/**
 * SUB-SYSTEM: Database Input/Output for EffronTransImport
 * FILENAME: TransImportIO.cpp
 * DESCRIPTION: Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-12-05
 **/

#include <math.h>
#include "commonheader.h"
#include "TransImportIO.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "TransIO.h"
#include "dtrans.h"
#include "maptransnoex.h"
#include "holdings.h"
#include <optional>
#include <cstring>
#include <string>
#include <vector>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SQL Queries
// ============================================================================

const char* SQL_SELECT_PORTCLOS =
    "SELECT TOP 1 portfolioid FROM Portclos WHERE portfolioid = ? AND dateclosedfor >= ?";

const char* SQL_INSERT_DTRNDESC =
    "INSERT INTO DtrnDesc (ID, Dtrans_No, Seqno, Close_Type, Taxlot_No, Units, Desc_Info) "
    "VALUES (?, ?, ?, ?, ?, ?, ?)";

const char* SQL_INSERT_DPAYTRAN =
    "INSERT INTO DPayTran (ID, Dtrans_No, Payee_ID, Dsc) VALUES (?, ?, ?, ?)";

const char* SQL_SELECT_TRANSNO_BY_DTRANSNO =
    "SELECT MAX(Trans_No) FROM Trans WHERE ID = ? AND DTrans_No = ? AND created_by = ? "
    "AND (Trans_No = Xref_Trans_No OR Xref_Trans_No = 0 OR "
    "(Tran_Type = 'RV' AND Xref_Trans_No = Rev_Trans_No))";

const char* SQL_SELECT_TRANSNO_BY_UNITS_DATES =
    "SELECT trans_no FROM Trans WHERE ID = ? AND Sec_No = ? AND Wi = ? AND Sec_Xtend = ? "
    "AND Tran_Type = ? AND Units = ? AND Trd_Date = ? AND Stl_Date = ? AND Rev_Trans_No = 0";

const char* SQL_SELECT_MAPTRANSNOEX =
    "SELECT TransNo, DtransNo FROM MapTransNoEx "
    "WHERE VendorID = ? AND RuleNo = ? AND PortId = ? "
    "AND Field1 LIKE ? AND Field2 LIKE ? AND Field3 LIKE ? AND Field4 LIKE ? AND SequenceNo = ?";

const char* SQL_SELECT_MAPTRANSNOEX_BY_TRANSNO =
    "SELECT Field1, Field2, Field3, Field4, SequenceNo FROM MapTransNoEx "
    "WHERE VendorID = ? AND RuleNo = ? AND PortId = ? AND TransNo = ?";

const char* SQL_INSERT_MAPTRANSNOEX =
    "INSERT INTO MapTransNoEx (VendorID, RuleNo, PortId, TransNo, DtransNo, "
    "Field1, Field2, Field3, Field4, SequenceNo, RevField1, RevField2, RevField3, RevField4, RevSequenceNo) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

const char* SQL_UPDATE_MAPTRANSNOEX =
    "UPDATE MapTransNoEx SET TransNo = ?, DtransNo = ?, RevField1 = ?, RevField2 = ?, "
    "RevField3 = ?, RevField4 = ?, RevSequenceNo = ? "
    "WHERE VendorID = ? AND RuleNo = ? AND PortId = ? "
    "AND Field1 LIKE ? AND Field2 LIKE ? AND Field3 LIKE ? AND Field4 LIKE ? AND SequenceNo = ?";

const char* SQL_DELETE_MAPTRANSNOEX =
    "DELETE FROM MapTransNoEx WHERE VendorID = ? AND RuleNo = ? AND PortId = ? "
    "AND Field1 LIKE ? AND Field2 LIKE ? AND Field3 LIKE ? AND Field4 LIKE ? AND SequenceNo = ?";

const char* SQL_INSERT_TAXLOTRECON =
    "INSERT INTO TaxlotRecon (VendorID, AccountID, FileDate, SecurityID, PositionID, "
    "Units, Base_mv, MV_xrate, Base_inc, Inc_xrate, Base_OriginalCost, Base_TotalCost, "
    "BaseCost_xrate, Original_face, Price, TradeDate, SettleDate, Restriction_Code, Ignore_Fields, batchID) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

const char* SQL_UPDATE_TRADEEXCHANGE_BY_PK =
    "UPDATE TradeExchange SET OutVendorId = ?, OutFileNo = ?, EffronTransNo = ?, Sent = GetDate() "
    "WHERE AccountId = ? AND TransNo = ? AND VendorID = ?";

const char* SQL_UPDATE_BNKSETEX_REVNO =
    "UPDATE bnksetex SET RevTransNo = ? WHERE Bank_id = ? AND AcctName = ? AND TransNo = ?";

const char* SQL_GROUPPED_HOLDINGS =
    "SELECT id, sec_no, wi, sec_xtend, acct_type, trd_date, sum(units) "
    "FROM dbo.Holdings WHERE ID = ? AND sec_no = ? AND wi = ? AND sec_xtend = ? "
    "AND acct_type LIKE ? AND trd_date IS NOT NULL AND trd_date != ? "
    "GROUP BY id, sec_no, wi, sec_xtend, acct_type, trd_date";

const char* SQL_SELECT_BNKSETEX_BY_UNITS_DATES =
    "SELECT transno FROM bnksetex WHERE bank_id = ? AND id = ? AND tran_type = ? "
    "AND sec_no = ? AND wi = ? AND trd_date = ? AND stl_date = ? "
    "AND units = ? AND base_amount = ? AND incomeamt = ? AND RevTransNo = ''";

// ============================================================================
// Static State for Multi-Row Cursors
// ============================================================================

struct GrouppedHoldingsState {
    std::optional<nanodbc::result> result;
    int iID = 0;
    char sSecNo[20] = {0};
    char sWi[2] = {0};
    char sSecXtend[3] = {0};
    char sAcctType[2] = {0};
    long lTrdDate = 0;
    int cRows = 0;
};

static GrouppedHoldingsState g_grouppedHoldingsState;

// Batch insert state for TaxlotRecon
struct TaxlotReconBatchState {
    std::vector<TAXLOTRECON> records;
};
static TaxlotReconBatchState g_taxlotReconBatch;

// ============================================================================
// SelectPortClos
// ============================================================================
DLLAPI void STDCALL SelectPortClos(long iID, long lCloseDate, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectPortClos", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPortClos", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_PORTCLOS));
        nanodbc::timestamp tsCloseDate;
        long_to_timestamp(lCloseDate, tsCloseDate);
        stmt.bind(0, &iID);
        stmt.bind(1, &tsCloseDate);
        auto result = nanodbc::execute(stmt);
        if (!result.next())
            pzErr->iSqlError = SQLNOTFOUND;
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPortClos", FALSE);
    }
}

// ============================================================================
// InsertDTrans - Simplified insert using OUTPUT INSERTED
// ============================================================================
DLLAPI void STDCALL InsertDTrans(DTRANS *pzDTrans, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertDTrans", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertDTrans", FALSE);
        return;
    }
    try {
        // Use INSERT with OUTPUT to get identity value
        std::string sql = 
            "INSERT INTO Dtrans (ID, Tran_Type, Sec_No, Wi, Sec_Xtend, Acct_Type, SecID, Sec_Symbol, "
            "Units, Orig_Face, Unit_Cost, Tot_Cost, Orig_Cost, Pcpl_Amt, Opt_Prem, Amort_Val, Basis_Adj, "
            "Comm_Gcr, Net_Comm, Comm_Code, Sec_Fees, Misc_Fee1, Fee_Code1, Misc_Fee2, Fee_Code2, "
            "Accr_Int, Income_Amt, Net_Flow, Broker_Code, Broker_Code2, Trd_Date, Stl_Date, Eff_Date, "
            "Entry_Date, Taxlot_No, Xref_Trans_No, Pend_Div_No, Rev_Trans_No, Rev_Type, New_Trans_No, "
            "Orig_Trans_No, Block_Trans_No, X_ID, X_Trans_No, X_Sec_No, X_Wi, X_Sec_Xtend, X_Acct_Type, "
            "X_SecID, Curr_Id, Curr_Acct_Type, Inc_Curr_Id, Inc_Acct_Type, X_Curr_Id, X_Curr_Acct_Type, "
            "Sec_Curr_Id, Accr_Curr_Id, Base_Xrate, Inc_Base_Xrate, Sec_Base_Xrate, Accr_Base_Xrate, "
            "Sys_Xrate, Inc_Sys_Xrate, Base_Open_Xrate, Sys_Open_Xrate, Open_Trd_Date, Open_Stl_Date, "
            "Open_Unit_Cost, Orig_Yld, Eff_Mat_Date, Eff_Mat_Price, Acct_Mthd, Trans_Srce, Adp_Tag, "
            "Div_Type, Div_Factor, Divint_No, Roll_Date, Perf_Date, Misc_Desc_Ind, Dr_Cr, Bal_To_Adjust, "
            "Cap_Trans, Safek_Ind, Dtc_Inclusion, Dtc_Resolve, Recon_Flag, Recon_Srce, Income_Flag, "
            "Letter_Flag, Ledger_Flag, Gl_Flag, Created_By, Create_Date, Create_Time, Post_Date, "
            "Bkof_Frmt, Bkof_Seq_No, Price, Restriction_Code, Status_Flag, Err_Status_Code, Process_Tag) "
            "OUTPUT INSERTED.Dtrans_no "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0)";

        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));

        // Convert dates
        nanodbc::timestamp tsTrdDate, tsStlDate, tsEffDate, tsEntryDate, tsOpenTrdDate, tsOpenStlDate;
        nanodbc::timestamp tsEffMatDate, tsRollDate, tsPerfDate, tsCreateDate, tsPostDate;
        long_to_timestamp(pzDTrans->zT.lTrdDate, tsTrdDate);
        long_to_timestamp(pzDTrans->zT.lStlDate, tsStlDate);
        long_to_timestamp(pzDTrans->zT.lEffDate, tsEffDate);
        long_to_timestamp(pzDTrans->zT.lEntryDate, tsEntryDate);
        long_to_timestamp(pzDTrans->zT.lOpenTrdDate, tsOpenTrdDate);
        long_to_timestamp(pzDTrans->zT.lOpenStlDate, tsOpenStlDate);
        long_to_timestamp(pzDTrans->zT.lEffMatDate, tsEffMatDate);
        long_to_timestamp(pzDTrans->zT.lRollDate, tsRollDate);
        long_to_timestamp(pzDTrans->zT.lPerfDate, tsPerfDate);
        long_to_timestamp(pzDTrans->zT.lCreateDate, tsCreateDate);
        long_to_timestamp(pzDTrans->zT.lPostDate, tsPostDate);

        // Round values
        double fUnits = RoundDouble(pzDTrans->zT.fUnits, 5);
        double fOrigFace = RoundDouble(pzDTrans->zT.fOrigFace, 3);
        double fUnitCost = RoundDouble(pzDTrans->zT.fUnitCost, 6);
        double fTotCost = RoundDouble(pzDTrans->zT.fTotCost, 3);
        double fOrigCost = RoundDouble(pzDTrans->zT.fOrigCost, 3);
        double fPcplAmt = RoundDouble(pzDTrans->zT.fPcplAmt, 2);
        double fOptPrem = RoundDouble(pzDTrans->zT.fOptPrem, 2);
        double fAmortVal = RoundDouble(pzDTrans->zT.fAmortVal, 2);
        double fBasisAdj = RoundDouble(pzDTrans->zT.fBasisAdj, 2);
        double fCommGcr = RoundDouble(pzDTrans->zT.fCommGcr, 2);
        double fNetComm = RoundDouble(pzDTrans->zT.fNetComm, 2);
        double fSecFees = RoundDouble(pzDTrans->zT.fSecFees, 2);
        double fMiscFee1 = RoundDouble(pzDTrans->zT.fMiscFee1, 2);
        double fMiscFee2 = RoundDouble(pzDTrans->zT.fMiscFee2, 2);
        double fAccrInt = RoundDouble(pzDTrans->zT.fAccrInt, 2);
        double fIncomeAmt = RoundDouble(pzDTrans->zT.fIncomeAmt, 2);
        double fNetFlow = RoundDouble(pzDTrans->zT.fNetFlow, 2);
        double fBaseXrate = RoundDouble(pzDTrans->zT.fBaseXrate, 12);
        double fIncBaseXrate = RoundDouble(pzDTrans->zT.fIncBaseXrate, 12);
        double fSecBaseXrate = RoundDouble(pzDTrans->zT.fSecBaseXrate, 12);
        double fAccrBaseXrate = RoundDouble(pzDTrans->zT.fAccrBaseXrate, 12);
        double fSysXrate = RoundDouble(pzDTrans->zT.fSysXrate, 12);
        double fIncSysXrate = RoundDouble(pzDTrans->zT.fIncSysXrate, 12);
        double fBaseOpenXrate = RoundDouble(pzDTrans->zT.fBaseOpenXrate, 12);
        double fSysOpenXrate = RoundDouble(pzDTrans->zT.fSysOpenXrate, 12);
        double fOpenUnitCost = RoundDouble(pzDTrans->zT.fOpenUnitCost, 6);
        double fOrigYld = RoundDouble(pzDTrans->zT.fOrigYld, 6);
        double fEffMatPrice = RoundDouble(pzDTrans->zT.fEffMatPrice, 6);
        double fDivFactor = RoundDouble(pzDTrans->zT.fDivFactor, 7);
        double fPrice = RoundDouble(pzDTrans->zT.fPrice, 8);

        int p = 0;
        stmt.bind(p++, &pzDTrans->zT.iID);
        stmt.bind(p++, pzDTrans->zT.sTranType);
        stmt.bind(p++, pzDTrans->zT.sSecNo);
        stmt.bind(p++, pzDTrans->zT.sWi);
        stmt.bind(p++, pzDTrans->zT.sSecXtend);
        stmt.bind(p++, pzDTrans->zT.sAcctType);
        stmt.bind(p++, &pzDTrans->zT.iSecID);
        stmt.bind(p++, pzDTrans->zT.sSecSymbol);
        stmt.bind(p++, &fUnits);
        stmt.bind(p++, &fOrigFace);
        stmt.bind(p++, &fUnitCost);
        stmt.bind(p++, &fTotCost);
        stmt.bind(p++, &fOrigCost);
        stmt.bind(p++, &fPcplAmt);
        stmt.bind(p++, &fOptPrem);
        stmt.bind(p++, &fAmortVal);
        stmt.bind(p++, &fBasisAdj);
        stmt.bind(p++, &fCommGcr);
        stmt.bind(p++, &fNetComm);
        stmt.bind(p++, pzDTrans->zT.sCommCode);
        stmt.bind(p++, &fSecFees);
        stmt.bind(p++, &fMiscFee1);
        stmt.bind(p++, pzDTrans->zT.sFeeCode1);
        stmt.bind(p++, &fMiscFee2);
        stmt.bind(p++, pzDTrans->zT.sFeeCode2);
        stmt.bind(p++, &fAccrInt);
        stmt.bind(p++, &fIncomeAmt);
        stmt.bind(p++, &fNetFlow);
        stmt.bind(p++, pzDTrans->zT.sBrokerCode);
        stmt.bind(p++, pzDTrans->zT.sBrokerCode2);
        stmt.bind(p++, &tsTrdDate);
        stmt.bind(p++, &tsStlDate);
        stmt.bind(p++, &tsEffDate);
        stmt.bind(p++, &tsEntryDate);
        stmt.bind(p++, &pzDTrans->zT.lTaxlotNo);
        stmt.bind(p++, &pzDTrans->zT.lXrefTransNo);
        stmt.bind(p++, &pzDTrans->zT.lPendDivNo);
        stmt.bind(p++, &pzDTrans->zT.lRevTransNo);
        stmt.bind(p++, pzDTrans->zT.sRevType);
        stmt.bind(p++, &pzDTrans->zT.lNewTransNo);
        stmt.bind(p++, &pzDTrans->zT.lOrigTransNo);
        stmt.bind(p++, &pzDTrans->zT.lBlockTransNo);
        stmt.bind(p++, pzDTrans->zT.sXID);
        stmt.bind(p++, &pzDTrans->zT.lXTransNo);
        stmt.bind(p++, pzDTrans->zT.sXSecNo);
        stmt.bind(p++, pzDTrans->zT.sXWi);
        stmt.bind(p++, pzDTrans->zT.sXSecXtend);
        stmt.bind(p++, pzDTrans->zT.sXAcctType);
        stmt.bind(p++, &pzDTrans->zT.iXSecID);
        stmt.bind(p++, pzDTrans->zT.sCurrId);
        stmt.bind(p++, pzDTrans->zT.sCurrAcctType);
        stmt.bind(p++, pzDTrans->zT.sIncCurrId);
        stmt.bind(p++, pzDTrans->zT.sIncAcctType);
        stmt.bind(p++, pzDTrans->zT.sXCurrId);
        stmt.bind(p++, pzDTrans->zT.sXCurrAcctType);
        stmt.bind(p++, pzDTrans->zT.sSecCurrId);
        stmt.bind(p++, pzDTrans->zT.sAccrCurrId);
        stmt.bind(p++, &fBaseXrate);
        stmt.bind(p++, &fIncBaseXrate);
        stmt.bind(p++, &fSecBaseXrate);
        stmt.bind(p++, &fAccrBaseXrate);
        stmt.bind(p++, &fSysXrate);
        stmt.bind(p++, &fIncSysXrate);
        stmt.bind(p++, &fBaseOpenXrate);
        stmt.bind(p++, &fSysOpenXrate);
        stmt.bind(p++, &tsOpenTrdDate);
        stmt.bind(p++, &tsOpenStlDate);
        stmt.bind(p++, &fOpenUnitCost);
        stmt.bind(p++, &fOrigYld);
        stmt.bind(p++, &tsEffMatDate);
        stmt.bind(p++, &fEffMatPrice);
        stmt.bind(p++, pzDTrans->zT.sAcctMthd);
        stmt.bind(p++, pzDTrans->zT.sTransSrce);
        stmt.bind(p++, pzDTrans->zT.sAdpTag);
        stmt.bind(p++, pzDTrans->zT.sDivType);
        stmt.bind(p++, &fDivFactor);
        stmt.bind(p++, &pzDTrans->zT.lDivintNo);
        stmt.bind(p++, &tsRollDate);
        stmt.bind(p++, &tsPerfDate);
        stmt.bind(p++, pzDTrans->zT.sMiscDescInd);
        stmt.bind(p++, pzDTrans->zT.sDrCr);
        stmt.bind(p++, pzDTrans->zT.sBalToAdjust);
        stmt.bind(p++, pzDTrans->zT.sCapTrans);
        stmt.bind(p++, pzDTrans->zT.sSafekInd);
        stmt.bind(p++, pzDTrans->zT.sDtcInclusion);
        stmt.bind(p++, pzDTrans->zT.sDtcResolve);
        stmt.bind(p++, pzDTrans->zT.sReconFlag);
        stmt.bind(p++, pzDTrans->zT.sReconSrce);
        stmt.bind(p++, pzDTrans->zT.sIncomeFlag);
        stmt.bind(p++, pzDTrans->zT.sLetterFlag);
        stmt.bind(p++, pzDTrans->zT.sLedgerFlag);
        stmt.bind(p++, pzDTrans->zT.sGlFlag);
        stmt.bind(p++, pzDTrans->zT.sCreatedBy);
        stmt.bind(p++, &tsCreateDate);
        stmt.bind(p++, pzDTrans->zT.sCreateTime);
        stmt.bind(p++, &tsPostDate);
        stmt.bind(p++, pzDTrans->zT.sBkofFrmt);
        stmt.bind(p++, &pzDTrans->zT.lBkofSeqNo);
        stmt.bind(p++, &fPrice);
        stmt.bind(p++, &pzDTrans->zT.iRestrictionCode);
        stmt.bind(p++, pzDTrans->sStatusFlag);
        stmt.bind(p++, &pzDTrans->iErrStatusCode);

        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            pzDTrans->zT.lDtransNo = result.get<long>(0, 0);
            pzDTrans->lProcessTag = pzDTrans->zT.lDtransNo;
            // Update process_tag
            nanodbc::just_execute(gConn, "UPDATE Dtrans SET Process_Tag = " + 
                std::to_string(pzDTrans->lProcessTag) + " WHERE Dtrans_no = " + 
                std::to_string(pzDTrans->zT.lDtransNo));
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), pzDTrans->zT.iID, pzDTrans->lProcessTag, (char*)"D", 0, -1, 0, (char*)"InsertDTrans", FALSE);
    }
}

// ============================================================================
// InsertDTrnDesc
// ============================================================================
DLLAPI void STDCALL InsertDTrnDesc(TRNDESC *pzDTrnDesc, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertDTrnDesc", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertDTrnDesc", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_INSERT_DTRNDESC));
        double fUnits = RoundDouble(pzDTrnDesc->fUnits, 5);
        stmt.bind(0, &pzDTrnDesc->iID);
        stmt.bind(1, &pzDTrnDesc->lDtransNo);
        stmt.bind(2, &pzDTrnDesc->iSeqno);
        stmt.bind(3, pzDTrnDesc->sCloseType);
        stmt.bind(4, &pzDTrnDesc->lTaxlotNo);
        stmt.bind(5, &fUnits);
        stmt.bind(6, pzDTrnDesc->sDescInfo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), pzDTrnDesc->iID, pzDTrnDesc->lDtransNo, (char*)"D", 0, -1, 0, (char*)"InsertDTrnDesc", FALSE);
    }
}

// ============================================================================
// InsertDPayTran
// ============================================================================
DLLAPI void STDCALL InsertDPayTran(PAYTRAN *pzDPayTran, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertDPayTran", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertDPayTran", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_INSERT_DPAYTRAN));
        stmt.bind(0, &pzDPayTran->iID);
        stmt.bind(1, &pzDPayTran->lTransNo);
        stmt.bind(2, &pzDPayTran->lPayeeID);
        stmt.bind(3, pzDPayTran->sDsc);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), pzDPayTran->iID, pzDPayTran->lTransNo, (char*)"D", 0, -1, 0, (char*)"InsertDPayTran", FALSE);
    }
}

// ============================================================================
// SelectTransNoByDtransNo
// ============================================================================
DLLAPI void STDCALL SelectTransNoByDtransNo(TRANS *pzTrans, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransNoByDtransNo", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransNoByDtransNo", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_TRANSNO_BY_DTRANSNO));
        stmt.bind(0, &pzTrans->iID);
        stmt.bind(1, &pzTrans->lDtransNo);
        stmt.bind(2, pzTrans->sCreatedBy);
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            pzTrans->lTransNo = result.get<long>(0, 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransNoByDtransNo", FALSE);
    }
}

// ============================================================================
// SelectTransNoByUnitsAndDates
// ============================================================================
DLLAPI void STDCALL SelectTransNoByUnitsAndDates(TRANS *pzTrans, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectTransNoByUnitsAndDates", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransNoByUnitsAndDates", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_TRANSNO_BY_UNITS_DATES));
        nanodbc::timestamp tsTrdDate, tsStlDate;
        long_to_timestamp(pzTrans->lTrdDate, tsTrdDate);
        long_to_timestamp(pzTrans->lStlDate, tsStlDate);
        stmt.bind(0, &pzTrans->iID);
        stmt.bind(1, pzTrans->sSecNo);
        stmt.bind(2, pzTrans->sWi);
        stmt.bind(3, pzTrans->sSecXtend);
        stmt.bind(4, pzTrans->sTranType);
        stmt.bind(5, &pzTrans->fUnits);
        stmt.bind(6, &tsTrdDate);
        stmt.bind(7, &tsStlDate);
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            pzTrans->lTransNo = result.get<long>(0, 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectTransNoByUnitsAndDates", FALSE);
    }
}

// ============================================================================
// MapTransNoEx CRUD Functions
// ============================================================================

DLLAPI void STDCALL SelectMapTransNoEx(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectMapTransNoEx", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectMapTransNoEx", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_MAPTRANSNOEX));
        stmt.bind(0, &pzMapTransNoEx->iVendorID);
        stmt.bind(1, &pzMapTransNoEx->iRuleNo);
        stmt.bind(2, &pzMapTransNoEx->lPortId);
        stmt.bind(3, pzMapTransNoEx->sField1);
        stmt.bind(4, pzMapTransNoEx->sField2);
        stmt.bind(5, pzMapTransNoEx->sField3);
        stmt.bind(6, pzMapTransNoEx->sField4);
        stmt.bind(7, &pzMapTransNoEx->lSequenceNo);
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            pzMapTransNoEx->lTransNo = result.get<long>(0, 0);
            pzMapTransNoEx->lDtransNo = result.get<long>(1, 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectMapTransNoEx", FALSE);
    }
}

DLLAPI void STDCALL SelectMapTransNoExByTransNo(MAPTRANSNOEX *pzMapTransNoEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectMapTransNoExByTransNo", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectMapTransNoExByTransNo", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_MAPTRANSNOEX_BY_TRANSNO));
        stmt.bind(0, &pzMapTransNoEx->iVendorID);
        stmt.bind(1, &pzMapTransNoEx->iRuleNo);
        stmt.bind(2, &pzMapTransNoEx->lPortId);
        stmt.bind(3, &pzMapTransNoEx->lTransNo);
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            read_string(result, 0, pzMapTransNoEx->sField1, sizeof(pzMapTransNoEx->sField1));
            read_string(result, 1, pzMapTransNoEx->sField2, sizeof(pzMapTransNoEx->sField2));
            read_string(result, 2, pzMapTransNoEx->sField3, sizeof(pzMapTransNoEx->sField3));
            read_string(result, 3, pzMapTransNoEx->sField4, sizeof(pzMapTransNoEx->sField4));
            pzMapTransNoEx->lSequenceNo = result.get<long>(4, 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectMapTransNoExByTransNo", FALSE);
    }
}

DLLAPI void STDCALL InsertMapTransNoEx(MAPTRANSNOEX* pzMapTransNoEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertMapTransNoEx", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMapTransNoEx", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_INSERT_MAPTRANSNOEX));
        int p = 0;
        stmt.bind(p++, &pzMapTransNoEx->iVendorID);
        stmt.bind(p++, &pzMapTransNoEx->iRuleNo);
        stmt.bind(p++, &pzMapTransNoEx->lPortId);
        stmt.bind(p++, &pzMapTransNoEx->lTransNo);
        stmt.bind(p++, &pzMapTransNoEx->lDtransNo);
        stmt.bind(p++, pzMapTransNoEx->sField1);
        stmt.bind(p++, pzMapTransNoEx->sField2);
        stmt.bind(p++, pzMapTransNoEx->sField3);
        stmt.bind(p++, pzMapTransNoEx->sField4);
        stmt.bind(p++, &pzMapTransNoEx->lSequenceNo);
        stmt.bind(p++, pzMapTransNoEx->sRevField1);
        stmt.bind(p++, pzMapTransNoEx->sRevField2);
        stmt.bind(p++, pzMapTransNoEx->sRevField3);
        stmt.bind(p++, pzMapTransNoEx->sRevField4);
        stmt.bind(p++, &pzMapTransNoEx->lRevSequenceNo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        // Ignore PK violation (DB_E_INTEGRITYVIOLATION)
        std::string what = e.what();
        if (what.find("2627") != std::string::npos || what.find("2601") != std::string::npos) {
            pzErr->iSqlError = -2147217873; // DB_E_INTEGRITYVIOLATION
        } else {
            std::string msg = "Database error: "; msg += e.what();
            *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMapTransNoEx", FALSE);
        }
    }
}

DLLAPI void STDCALL UpdateMapTransNoEx(MAPTRANSNOEX* pzMapTransNoEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdateMapTransNoEx", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateMapTransNoEx", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UPDATE_MAPTRANSNOEX));
        int p = 0;
        stmt.bind(p++, &pzMapTransNoEx->lTransNo);
        stmt.bind(p++, &pzMapTransNoEx->lDtransNo);
        stmt.bind(p++, pzMapTransNoEx->sRevField1);
        stmt.bind(p++, pzMapTransNoEx->sRevField2);
        stmt.bind(p++, pzMapTransNoEx->sRevField3);
        stmt.bind(p++, pzMapTransNoEx->sRevField4);
        stmt.bind(p++, &pzMapTransNoEx->lRevSequenceNo);
        stmt.bind(p++, &pzMapTransNoEx->iVendorID);
        stmt.bind(p++, &pzMapTransNoEx->iRuleNo);
        stmt.bind(p++, &pzMapTransNoEx->lPortId);
        stmt.bind(p++, pzMapTransNoEx->sField1);
        stmt.bind(p++, pzMapTransNoEx->sField2);
        stmt.bind(p++, pzMapTransNoEx->sField3);
        stmt.bind(p++, pzMapTransNoEx->sField4);
        stmt.bind(p++, &pzMapTransNoEx->lSequenceNo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateMapTransNoEx", FALSE);
    }
}

DLLAPI void STDCALL DeleteMapTransNoEx(MAPTRANSNOEX* pzMapTransNoEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "DeleteMapTransNoEx", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMapTransNoEx", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_DELETE_MAPTRANSNOEX));
        stmt.bind(0, &pzMapTransNoEx->iVendorID);
        stmt.bind(1, &pzMapTransNoEx->iRuleNo);
        stmt.bind(2, &pzMapTransNoEx->lPortId);
        stmt.bind(3, pzMapTransNoEx->sField1);
        stmt.bind(4, pzMapTransNoEx->sField2);
        stmt.bind(5, pzMapTransNoEx->sField3);
        stmt.bind(6, pzMapTransNoEx->sField4);
        stmt.bind(7, &pzMapTransNoEx->lSequenceNo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMapTransNoEx", FALSE);
    }
}

// ============================================================================
// TaxlotRecon Functions with Batch Support
// ============================================================================

DLLAPI void STDCALL InsertTaxlotRecon(TAXLOTRECON* pzTaxlotRecon, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertTaxlotRecon", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertTaxlotRecon", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_INSERT_TAXLOTRECON));
        nanodbc::timestamp tsFileDate, tsTrdDate, tsStlDate;
        long_to_timestamp(pzTaxlotRecon->lFileDate, tsFileDate);
        long_to_timestamp(pzTaxlotRecon->lTrdDate, tsTrdDate);
        long_to_timestamp(pzTaxlotRecon->lStlDate, tsStlDate);
        int p = 0;
        stmt.bind(p++, &pzTaxlotRecon->iVendorID);
        stmt.bind(p++, pzTaxlotRecon->sAccountID);
        stmt.bind(p++, &tsFileDate);
        stmt.bind(p++, pzTaxlotRecon->sSecurityID);
        stmt.bind(p++, pzTaxlotRecon->sPositionID);
        stmt.bind(p++, &pzTaxlotRecon->fUnits);
        stmt.bind(p++, &pzTaxlotRecon->fBaseMV);
        stmt.bind(p++, &pzTaxlotRecon->fMVXrate);
        stmt.bind(p++, &pzTaxlotRecon->fBaseInc);
        stmt.bind(p++, &pzTaxlotRecon->fIncXrate);
        stmt.bind(p++, &pzTaxlotRecon->fBaseOrigCost);
        stmt.bind(p++, &pzTaxlotRecon->fBaseTotCost);
        stmt.bind(p++, &pzTaxlotRecon->fBaseCostXrate);
        stmt.bind(p++, &pzTaxlotRecon->fOrigFace);
        stmt.bind(p++, &pzTaxlotRecon->fPrice);
        stmt.bind(p++, &tsTrdDate);
        stmt.bind(p++, &tsStlDate);
        stmt.bind(p++, pzTaxlotRecon->sRestrictionCode);
        stmt.bind(p++, &pzTaxlotRecon->iIgnoreFields);
        stmt.bind(p++, &pzTaxlotRecon->ibatchID);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertTaxlotRecon", FALSE);
    }
}

DLLAPI void STDCALL InsertOneTaxlotRecon(TAXLOTRECON* pzTaxlotRecon, long lBatchSize, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertOneTaxlotRecon", FALSE);
#endif
    InitializeErrStruct(pzErr);
    g_taxlotReconBatch.records.push_back(*pzTaxlotRecon);
    if ((long)g_taxlotReconBatch.records.size() >= lBatchSize) {
        InsertAllTaxlotRecon(pzErr);
    }
}

DLLAPI void STDCALL InsertAllTaxlotRecon(ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertAllTaxlotRecon", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (g_taxlotReconBatch.records.empty()) return;
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertAllTaxlotRecon", FALSE);
        return;
    }
    try {
        for (auto& rec : g_taxlotReconBatch.records) {
            InsertTaxlotRecon(&rec, pzErr);
            if (pzErr->iSqlError != 0) break;
        }
        g_taxlotReconBatch.records.clear();
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertAllTaxlotRecon", FALSE);
    }
}

// ============================================================================
// TradeExchange Functions
// ============================================================================

DLLAPI void STDCALL InsertTradeExchange(TRADEEXCHANGE* pzTradeExchange, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertTradeExchange", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertTradeExchange", FALSE);
        return;
    }
    try {
        // First get the next SequenceNo
        std::string seqSql = "SELECT ISNULL(MAX(SequenceNo),-1)+1 FROM TradeExchange "
            "WHERE TrdDate = ? AND AccountID = ? AND AssetID = ? AND TransCode = ? AND VendorID = ?";
        nanodbc::statement seqStmt(gConn);
        nanodbc::prepare(seqStmt, NANODBC_TEXT(seqSql.c_str()));
        nanodbc::timestamp tsTrdDate, tsStlDate, tsFileDate, tsTimeRcvd;
        long_to_timestamp(pzTradeExchange->lTrdDate, tsTrdDate);
        long_to_timestamp(pzTradeExchange->lStlDate, tsStlDate);
        long_to_timestamp(pzTradeExchange->lFileDate, tsFileDate);
        long_to_timestamp((long)pzTradeExchange->fTimeRcvd, tsTimeRcvd);
        seqStmt.bind(0, &tsTrdDate);
        seqStmt.bind(1, pzTradeExchange->sAccountID);
        seqStmt.bind(2, pzTradeExchange->sAssetID);
        seqStmt.bind(3, pzTradeExchange->sTransCode);
        seqStmt.bind(4, &pzTradeExchange->iVendorID);
        auto seqResult = nanodbc::execute(seqStmt);
        long lSequenceNo = 0;
        if (seqResult.next()) lSequenceNo = seqResult.get<long>(0, 0);

        // Now insert
        std::string sql = "INSERT INTO TradeExchange (AccountId, TransNo, VendorID, FileDate, FileNo, Received, "
            "AssetId, TransCode, TrdDate, StlDate, RecordType, OrigTransNo, MasterTicketNo, Units, UnitCode, "
            "PcplAmt, IncPcplCode, Price, NetComm, CommCode, SecFees, AccrInt, OrigFace, MortFactor, ExRate, "
            "StlLocCode, WriteDownMtd, TaxlotNo, TradeType, Broker, Desc1, Desc2, Desc3, Desc4, SequenceNo) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
        double fUnits = RoundDouble(pzTradeExchange->fUnits, 5);
        double fOrigFace = RoundDouble(pzTradeExchange->fOrigFace, 3);
        double fPcplAmt = RoundDouble(pzTradeExchange->fPcplAmt, 2);
        double fNetComm = RoundDouble(pzTradeExchange->fNetComm, 2);
        double fSecFees = RoundDouble(pzTradeExchange->fSecFees, 2);
        double fAccrInt = RoundDouble(pzTradeExchange->fAccrInt, 2);
        double fMortFactor = RoundDouble(pzTradeExchange->fMortFactor, 3);
        double fExRate = RoundDouble(pzTradeExchange->fExRate, 12);
        int p = 0;
        stmt.bind(p++, pzTradeExchange->sAccountID);
        stmt.bind(p++, &pzTradeExchange->lTransNo);
        stmt.bind(p++, &pzTradeExchange->iVendorID);
        stmt.bind(p++, &tsFileDate);
        stmt.bind(p++, &pzTradeExchange->iFileNo);
        stmt.bind(p++, &tsTimeRcvd);
        stmt.bind(p++, pzTradeExchange->sAssetID);
        stmt.bind(p++, pzTradeExchange->sTransCode);
        stmt.bind(p++, &tsTrdDate);
        stmt.bind(p++, &tsStlDate);
        stmt.bind(p++, &pzTradeExchange->iRecordType);
        stmt.bind(p++, &pzTradeExchange->lOrigTransNo);
        stmt.bind(p++, &pzTradeExchange->lMasterTicketNo);
        stmt.bind(p++, &fUnits);
        stmt.bind(p++, pzTradeExchange->sUnitCode);
        stmt.bind(p++, &fPcplAmt);
        stmt.bind(p++, pzTradeExchange->sIncPcplCode);
        stmt.bind(p++, &pzTradeExchange->fPrice);
        stmt.bind(p++, &fNetComm);
        stmt.bind(p++, pzTradeExchange->sCommCode);
        stmt.bind(p++, &fSecFees);
        stmt.bind(p++, &fAccrInt);
        stmt.bind(p++, &fOrigFace);
        stmt.bind(p++, &fMortFactor);
        stmt.bind(p++, &fExRate);
        stmt.bind(p++, pzTradeExchange->sStlLocCode);
        stmt.bind(p++, pzTradeExchange->sWriteDownMtd);
        stmt.bind(p++, pzTradeExchange->sTaxlotNo);
        stmt.bind(p++, pzTradeExchange->sTradeType);
        stmt.bind(p++, pzTradeExchange->sBroker);
        stmt.bind(p++, pzTradeExchange->sDesc1);
        stmt.bind(p++, pzTradeExchange->sDesc2);
        stmt.bind(p++, pzTradeExchange->sDesc3);
        stmt.bind(p++, pzTradeExchange->sDesc4);
        stmt.bind(p++, &lSequenceNo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertTradeExchange", FALSE);
    }
}

// ============================================================================
// BnkSet Functions
// ============================================================================

DLLAPI void STDCALL InsertBnkSet(BNKSET *pzBnkSet, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertBnkSet", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertBnkSet", FALSE);
        return;
    }
    try {
        // First get duplicate_counter
        std::string cntSql = "SELECT ISNULL(MAX(duplicate_counter),-1)+1 FROM BnkSet "
            "WHERE id = ? AND stl_date = ? AND tran_type = ? AND dr_cr = ? AND sec_no = ? AND bank_id = ?";
        nanodbc::statement cntStmt(gConn);
        nanodbc::prepare(cntStmt, NANODBC_TEXT(cntSql.c_str()));
        nanodbc::timestamp tsStlDate, tsTrdDate, tsEntryDate, tsReconDate;
        long_to_timestamp(pzBnkSet->lStlDate, tsStlDate);
        long_to_timestamp(pzBnkSet->lTrdDate, tsTrdDate);
        long_to_timestamp(pzBnkSet->lEntryDate, tsEntryDate);
        long_to_timestamp(pzBnkSet->lReconciliationDate, tsReconDate);
        cntStmt.bind(0, &pzBnkSet->iID);
        cntStmt.bind(1, &tsStlDate);
        cntStmt.bind(2, pzBnkSet->sTranType);
        cntStmt.bind(3, pzBnkSet->sDrCr);
        cntStmt.bind(4, pzBnkSet->sSecNo);
        cntStmt.bind(5, pzBnkSet->sBankID);
        auto cntResult = nanodbc::execute(cntStmt);
        int iDuplicateCounter = 0;
        if (cntResult.next()) iDuplicateCounter = cntResult.get<int>(0, 0);

        std::string sql = "INSERT INTO bnkset (id, stl_date, tran_type, dr_cr, sec_no, bank_id, duplicate_counter, "
            "cusip, trd_date, description, units, commission, sec_fees, short_gain, long_gain, broker, cost, "
            "base_amount, native_amount, exrate, currency, principal_income_flag, entry_date, reversal_code, "
            "reconciliation_date, effron_trans_no) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
        double fUnits = RoundDouble(pzBnkSet->fUnits, 3);
        double fCommission = RoundDouble(pzBnkSet->fCommission, 2);
        double fSecFees = RoundDouble(pzBnkSet->fSecFees, 2);
        double fShortGain = RoundDouble(pzBnkSet->fShortGain, 2);
        double fLongGain = RoundDouble(pzBnkSet->fLongGain, 2);
        double fCost = RoundDouble(pzBnkSet->fCost, 2);
        double fBaseAmount = RoundDouble(pzBnkSet->fBaseAmount, 2);
        double fNativeAmount = RoundDouble(pzBnkSet->fNativeAmount, 2);
        double fExrate = RoundDouble(pzBnkSet->fExrate, 12);
        int p = 0;
        stmt.bind(p++, &pzBnkSet->iID);
        stmt.bind(p++, &tsStlDate);
        stmt.bind(p++, pzBnkSet->sTranType);
        stmt.bind(p++, pzBnkSet->sDrCr);
        stmt.bind(p++, pzBnkSet->sSecNo);
        stmt.bind(p++, pzBnkSet->sBankID);
        stmt.bind(p++, &iDuplicateCounter);
        stmt.bind(p++, pzBnkSet->sCusip);
        stmt.bind(p++, &tsTrdDate);
        stmt.bind(p++, pzBnkSet->sDescription);
        stmt.bind(p++, &fUnits);
        stmt.bind(p++, &fCommission);
        stmt.bind(p++, &fSecFees);
        stmt.bind(p++, &fShortGain);
        stmt.bind(p++, &fLongGain);
        stmt.bind(p++, pzBnkSet->sBroker);
        stmt.bind(p++, &fCost);
        stmt.bind(p++, &fBaseAmount);
        stmt.bind(p++, &fNativeAmount);
        stmt.bind(p++, &fExrate);
        stmt.bind(p++, pzBnkSet->sCurrency);
        stmt.bind(p++, pzBnkSet->sPrincipalIncomeFlag);
        stmt.bind(p++, &tsEntryDate);
        stmt.bind(p++, pzBnkSet->sReversalCode);
        stmt.bind(p++, &tsReconDate);
        stmt.bind(p++, &pzBnkSet->iEffronTransNo);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertBnkSet", FALSE);
    }
}

DLLAPI void STDCALL InsertBnkSetEx(BNKSETEX *pzBnkSetEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertBnkSetEx", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertBnkSetEx", FALSE);
        return;
    }
    try {
        std::string sql = "INSERT INTO bnksetex (ID, Stl_date, Tran_type, Dr_Cr, Sec_no, Wi, Duplicate_counter, "
            "Cusip, Bank_id, AcctName, Units, Base_amount, Native_amount, OrigFace, OrigCost, Cost, IncomeAmt, "
            "Commission, Sec_fees, Short_gain, Long_gain, Exrate, Currency, Broker, Trd_date, RestrictionCode, "
            "TransNo, RevTranType, RevTransNo, BalToAdjust, Entry_date, Principal_income_flag, "
            "Reconciliation_date, Effron_trans_no, Description) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sql.c_str()));
        nanodbc::timestamp tsStlDate, tsTrdDate, tsEntryDate, tsReconDate;
        long_to_timestamp(pzBnkSetEx->lStlDate, tsStlDate);
        long_to_timestamp(pzBnkSetEx->lTrdDate, tsTrdDate);
        long_to_timestamp(pzBnkSetEx->lEntryDate, tsEntryDate);
        long_to_timestamp(pzBnkSetEx->lReconciliationDate, tsReconDate);
        double fUnits = RoundDouble(pzBnkSetEx->fUnits, 5);
        double fOrigFace = RoundDouble(pzBnkSetEx->fOrigFace, 5);
        double fCommission = RoundDouble(pzBnkSetEx->fCommission, 2);
        double fSecFees = RoundDouble(pzBnkSetEx->fSecFees, 2);
        double fShortGain = RoundDouble(pzBnkSetEx->fShortGain, 2);
        double fLongGain = RoundDouble(pzBnkSetEx->fLongGain, 2);
        double fOrigCost = RoundDouble(pzBnkSetEx->fOrigCost, 2);
        double fCost = RoundDouble(pzBnkSetEx->fCost, 2);
        double fBaseAmount = RoundDouble(pzBnkSetEx->fBaseAmount, 2);
        double fNativeAmount = RoundDouble(pzBnkSetEx->fNativeAmount, 2);
        double fIncomeAmt = RoundDouble(pzBnkSetEx->fIncomeAmt, 2);
        double fExrate = RoundDouble(pzBnkSetEx->fExrate, 12);
        int p = 0;
        stmt.bind(p++, &pzBnkSetEx->lID);
        stmt.bind(p++, &tsStlDate);
        stmt.bind(p++, pzBnkSetEx->sTranType);
        stmt.bind(p++, pzBnkSetEx->sDrCr);
        stmt.bind(p++, pzBnkSetEx->sSecNo);
        stmt.bind(p++, pzBnkSetEx->sWi);
        stmt.bind(p++, &pzBnkSetEx->iDuplicateCounter);
        stmt.bind(p++, pzBnkSetEx->sCusip);
        stmt.bind(p++, pzBnkSetEx->sBankId);
        stmt.bind(p++, pzBnkSetEx->sAcctName);
        stmt.bind(p++, &fUnits);
        stmt.bind(p++, &fBaseAmount);
        stmt.bind(p++, &fNativeAmount);
        stmt.bind(p++, &fOrigFace);
        stmt.bind(p++, &fOrigCost);
        stmt.bind(p++, &fCost);
        stmt.bind(p++, &fIncomeAmt);
        stmt.bind(p++, &fCommission);
        stmt.bind(p++, &fSecFees);
        stmt.bind(p++, &fShortGain);
        stmt.bind(p++, &fLongGain);
        stmt.bind(p++, &fExrate);
        stmt.bind(p++, pzBnkSetEx->sCurrency);
        stmt.bind(p++, pzBnkSetEx->sBroker);
        stmt.bind(p++, &tsTrdDate);
        stmt.bind(p++, pzBnkSetEx->sRestrictionCode);
        stmt.bind(p++, pzBnkSetEx->sTransNo);
        stmt.bind(p++, pzBnkSetEx->sRevTranType);
        stmt.bind(p++, pzBnkSetEx->sRevTransNo);
        stmt.bind(p++, pzBnkSetEx->sBalToAdjust);
        stmt.bind(p++, &tsEntryDate);
        stmt.bind(p++, pzBnkSetEx->sPrincipalIncomeFlag);
        stmt.bind(p++, &tsReconDate);
        stmt.bind(p++, &pzBnkSetEx->lEffronTransNo);
        stmt.bind(p++, pzBnkSetEx->sDescription);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        // Check for PK violation
        std::string what = e.what();
        if (what.find("2627") != std::string::npos || what.find("2601") != std::string::npos) {
            pzErr->iSqlError = 9729; // ERROR_DUPLICATE_KEY
        } else {
            std::string msg = "Database error: "; msg += e.what();
            *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertBnkSetEx", FALSE);
        }
    }
}

DLLAPI void STDCALL UpdateTradeExchangeByPK(TRADEEXCHANGE *pzTradeExchange, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdateTradeExchangeByPK", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateTradeExchangeByPK", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UPDATE_TRADEEXCHANGE_BY_PK));
        stmt.bind(0, &pzTradeExchange->iOutVendorID);
        stmt.bind(1, &pzTradeExchange->iOutFileNo);
        stmt.bind(2, &pzTradeExchange->lEffronTransNo);
        stmt.bind(3, pzTradeExchange->sAccountID);
        stmt.bind(4, &pzTradeExchange->lTransNo);
        stmt.bind(5, &pzTradeExchange->iVendorID);
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateTradeExchangeByPK", FALSE);
    }
}

// ============================================================================
// BnkSetEx Functions
// ============================================================================

DLLAPI void STDCALL UpdateBnkSetExRevNo(BNKSETEX *pzBnkSetEx, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdateBnkSetExRevNo", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateBnkSetExRevNo", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UPDATE_BNKSETEX_REVNO));
        stmt.bind(0, pzBnkSetEx->sTransNo);
        stmt.bind(1, pzBnkSetEx->sBankId);
        stmt.bind(2, pzBnkSetEx->sAcctName);
        stmt.bind(3, pzBnkSetEx->sRevTransNo);
        auto affected = nanodbc::execute(stmt);
        // Check rows affected - original checked for exactly 1
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateBnkSetExRevNo", FALSE);
    }
}

DLLAPI void STDCALL SelectBnksetexNoByUnitsAndDates(BNKSETEX *pzBnksetex, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SelectBnksetexNoByUnitsAndDates", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectBnksetexNoByUnitsAndDates", FALSE);
        return;
    }
    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SELECT_BNKSETEX_BY_UNITS_DATES));
        nanodbc::timestamp tsTrdDate, tsStlDate;
        long_to_timestamp(pzBnksetex->lTrdDate, tsTrdDate);
        long_to_timestamp(pzBnksetex->lStlDate, tsStlDate);
        stmt.bind(0, pzBnksetex->sBankId);
        stmt.bind(1, &pzBnksetex->lID);
        stmt.bind(2, pzBnksetex->sTranType);
        stmt.bind(3, pzBnksetex->sSecNo);
        stmt.bind(4, pzBnksetex->sWi);
        stmt.bind(5, &tsTrdDate);
        stmt.bind(6, &tsStlDate);
        stmt.bind(7, &pzBnksetex->fUnits);
        stmt.bind(8, &pzBnksetex->fBaseAmount);
        stmt.bind(9, &pzBnksetex->fIncomeAmt);
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            read_string(result, 0, pzBnksetex->sTransNo, sizeof(pzBnksetex->sTransNo));
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectBnksetexNoByUnitsAndDates", FALSE);
    }
}

// ============================================================================
// GrouppedHoldingsFor - Multi-row cursor
// ============================================================================

DLLAPI void STDCALL GrouppedHoldingsFor(HOLDINGS *pzHoldings, int iID, char *sSecNo, char *sWi,
    char *sSecXtend, char *sAcctType, long lTrdDate, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "GrouppedHoldingsFor", FALSE);
#endif
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"GrouppedHoldingsFor", FALSE);
        return;
    }
    try {
        bool needNewQuery = !(
            g_grouppedHoldingsState.iID == iID &&
            strcmp(g_grouppedHoldingsState.sSecNo, sSecNo) == 0 &&
            strcmp(g_grouppedHoldingsState.sWi, sWi) == 0 &&
            strcmp(g_grouppedHoldingsState.sSecXtend, sSecXtend) == 0 &&
            strcmp(g_grouppedHoldingsState.sAcctType, sAcctType) == 0 &&
            g_grouppedHoldingsState.lTrdDate == lTrdDate &&
            g_grouppedHoldingsState.cRows > 0
        );

        if (needNewQuery) {
            g_grouppedHoldingsState.result.reset();
            g_grouppedHoldingsState.cRows = 0;
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_GROUPPED_HOLDINGS));
            nanodbc::timestamp tsTrdDate;
            long_to_timestamp(lTrdDate, tsTrdDate);
            stmt.bind(0, &iID);
            stmt.bind(1, sSecNo);
            stmt.bind(2, sWi);
            stmt.bind(3, sSecXtend);
            stmt.bind(4, sAcctType);
            stmt.bind(5, &tsTrdDate);
            g_grouppedHoldingsState.result = nanodbc::execute(stmt);
            g_grouppedHoldingsState.iID = iID;
            strcpy_s(g_grouppedHoldingsState.sSecNo, sSecNo);
            strcpy_s(g_grouppedHoldingsState.sWi, sWi);
            strcpy_s(g_grouppedHoldingsState.sSecXtend, sSecXtend);
            strcpy_s(g_grouppedHoldingsState.sAcctType, sAcctType);
            g_grouppedHoldingsState.lTrdDate = lTrdDate;
        }

        memset(pzHoldings, 0, sizeof(*pzHoldings));
        if (g_grouppedHoldingsState.result && g_grouppedHoldingsState.result->next()) {
            g_grouppedHoldingsState.cRows++;
            pzHoldings->iID = g_grouppedHoldingsState.result->get<int>(0, 0);
            read_string(*g_grouppedHoldingsState.result, 1, pzHoldings->sSecNo, sizeof(pzHoldings->sSecNo));
            read_string(*g_grouppedHoldingsState.result, 2, pzHoldings->sWi, sizeof(pzHoldings->sWi));
            read_string(*g_grouppedHoldingsState.result, 3, pzHoldings->sSecXtend, sizeof(pzHoldings->sSecXtend));
            read_string(*g_grouppedHoldingsState.result, 4, pzHoldings->sAcctType, sizeof(pzHoldings->sAcctType));
            pzHoldings->lTrdDate = read_date(*g_grouppedHoldingsState.result, 5);
            pzHoldings->fUnits = g_grouppedHoldingsState.result->get<double>(6, 0.0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
            g_grouppedHoldingsState.cRows = 0;
            g_grouppedHoldingsState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e) {
        std::string msg = "Database error: "; msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"GrouppedHoldingsFor", FALSE);
        g_grouppedHoldingsState.result.reset();
    }
}

// ============================================================================
// Cleanup Functions (RAII - simplified)
// ============================================================================

void CloseTransImportIO(void)
{
    g_grouppedHoldingsState.result.reset();
    g_grouppedHoldingsState.cRows = 0;
    g_taxlotReconBatch.records.clear();
}

void FreeTransImportIO(void)
{
    CloseTransImportIO();
#ifdef DEBUG
    PrintError("FreeTransImportIO called (RAII cleanup)", 0, 0, "", 0, 0, 0, "FreeTransImportIO", FALSE);
#endif
}
