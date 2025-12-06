#include "DigenerateSelectors.h"
#include "DigenerateQueries.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <optional>
#include <cstring>
#include <string>
#include <stdio.h>
#include "accdiv.h"
#include "divhist.h"
#include "fwtrans.h"
#include "portmain.h"
#include "commonheader.h"
#include "currency.h"

extern thread_local nanodbc::connection gConn;

// ============================================================================
// SelectOneAccdiv
// ============================================================================
DLLAPI void SelectOneAccdiv(ACCDIV *pzAccdiv, int iID, long lDivintNo, long lTransNo, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectOneAccdiv));

        int p = 0;
        stmt.bind(p++, &iID);
        stmt.bind(p++, &lDivintNo);
        stmt.bind(p++, &lTransNo);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            memset(pzAccdiv, 0, sizeof(*pzAccdiv));
            
            pzAccdiv->iID = result.get<int>("id", 0);
            pzAccdiv->lTransNo = result.get<long>("trans_no", 0);
            pzAccdiv->lDivintNo = result.get<long>("divint_no", 0);
            read_string(result, "tran_type", pzAccdiv->sTranType, sizeof(pzAccdiv->sTranType));
            read_string(result, "sec_no", pzAccdiv->sSecNo, sizeof(pzAccdiv->sSecNo));
            read_string(result, "wi", pzAccdiv->sWi, sizeof(pzAccdiv->sWi));
            pzAccdiv->iSecID = result.get<int>("secid", 0);
            read_string(result, "sec_xtend", pzAccdiv->sSecXtend, sizeof(pzAccdiv->sSecXtend));
            read_string(result, "acct_type", pzAccdiv->sAcctType, sizeof(pzAccdiv->sAcctType));
            pzAccdiv->lEligDate = timestamp_to_long(result.get<nanodbc::timestamp>("elig_date"));
            read_string(result, "sec_symbol", pzAccdiv->sSecSymbol, sizeof(pzAccdiv->sSecSymbol));
            
            read_string(result, "div_type", pzAccdiv->sDivType, sizeof(pzAccdiv->sDivType));
            pzAccdiv->fDivFactor = result.get<double>("div_factor", 0.0);
            pzAccdiv->fUnits = result.get<double>("units", 0.0);
            pzAccdiv->fOrigFace = result.get<double>("orig_face", 0.0);
            pzAccdiv->fPcplAmt = result.get<double>("pcpl_amt", 0.0);
            pzAccdiv->fIncomeAmt = result.get<double>("income_amt", 0.0);
            
            pzAccdiv->lTrdDate = timestamp_to_long(result.get<nanodbc::timestamp>("trd_date"));
            pzAccdiv->lStlDate = timestamp_to_long(result.get<nanodbc::timestamp>("stl_date"));
            pzAccdiv->lEffDate = timestamp_to_long(result.get<nanodbc::timestamp>("eff_date"));
            pzAccdiv->lEntryDate = timestamp_to_long(result.get<nanodbc::timestamp>("entry_date"));
            
            read_string(result, "curr_id", pzAccdiv->sCurrId, sizeof(pzAccdiv->sCurrId));
            read_string(result, "curr_acct_type", pzAccdiv->sCurrAcctType, sizeof(pzAccdiv->sCurrAcctType));
            read_string(result, "inc_curr_id", pzAccdiv->sIncCurrId, sizeof(pzAccdiv->sIncCurrId));
            read_string(result, "inc_acct_type", pzAccdiv->sIncAcctType, sizeof(pzAccdiv->sIncAcctType));
            read_string(result, "sec_curr_id", pzAccdiv->sSecCurrId, sizeof(pzAccdiv->sSecCurrId));
            read_string(result, "accr_curr_id", pzAccdiv->sAccrCurrId, sizeof(pzAccdiv->sAccrCurrId));
            
            pzAccdiv->fBaseXrate = result.get<double>("base_xrate", 0.0);
            pzAccdiv->fIncBaseXrate = result.get<double>("inc_base_xrate", 0.0);
            pzAccdiv->fSecBaseXrate = result.get<double>("sec_base_xrate", 0.0);
            pzAccdiv->fAccrBaseXrate = result.get<double>("accr_base_xrate", 0.0);
            pzAccdiv->fSysXrate = result.get<double>("sys_xrate", 0.0);
            pzAccdiv->fIncSysXrate = result.get<double>("inc_sys_xrate", 0.0);
            
            pzAccdiv->fOrigYld = result.get<double>("orig_yld", 0.0);
            pzAccdiv->lEffMatDate = timestamp_to_long(result.get<nanodbc::timestamp>("eff_mat_date"));
            pzAccdiv->fEffMatPrice = result.get<double>("eff_mat_price", 0.0);
            read_string(result, "acct_mthd", pzAccdiv->sAcctMthd, sizeof(pzAccdiv->sAcctMthd));
            
            read_string(result, "trans_srce", pzAccdiv->sTransSrce, sizeof(pzAccdiv->sTransSrce));
            read_string(result, "dr_cr", pzAccdiv->sDrCr, sizeof(pzAccdiv->sDrCr));
            read_string(result, "dtc_inclusion", pzAccdiv->sDtcInclusion, sizeof(pzAccdiv->sDtcInclusion));
            read_string(result, "dtc_resolve", pzAccdiv->sDtcResolve, sizeof(pzAccdiv->sDtcResolve));
            read_string(result, "income_flag", pzAccdiv->sIncomeFlag, sizeof(pzAccdiv->sIncomeFlag));
            
            read_string(result, "letter_flag", pzAccdiv->sLetterFlag, sizeof(pzAccdiv->sLetterFlag));
            read_string(result, "ledger_flag", pzAccdiv->sLedgerFlag, sizeof(pzAccdiv->sLedgerFlag));
            read_string(result, "created_by", pzAccdiv->sCreatedBy, sizeof(pzAccdiv->sCreatedBy));
            pzAccdiv->lCreateDate = timestamp_to_long(result.get<nanodbc::timestamp>("create_date"));
            read_string(result, "create_time", pzAccdiv->sCreateTime, sizeof(pzAccdiv->sCreateTime));
            
            read_string(result, "suspend_flag", pzAccdiv->sSuspendFlag, sizeof(pzAccdiv->sSuspendFlag));
            read_string(result, "delete_flag", pzAccdiv->sDeleteFlag, sizeof(pzAccdiv->sDeleteFlag));
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), iID, 0, "", 0, -1, 0, "SelectOneAccdiv", FALSE);
    }
}

// ============================================================================
// SelectPartCurrency
// ============================================================================
struct SelectPartCurrencyState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};
static SelectPartCurrencyState g_selectPartCurrencyState;

DLLAPI void SelectPartCurrency(PARTCURR *pzCurrency, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        if (!g_selectPartCurrencyState.result)
        {
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectPartCurrency));
            g_selectPartCurrencyState.result = nanodbc::execute(stmt);
            g_selectPartCurrencyState.cRows = 0;
        }

        if (g_selectPartCurrencyState.result && g_selectPartCurrencyState.result->next())
        {
            g_selectPartCurrencyState.cRows++;
            read_string(*g_selectPartCurrencyState.result, "curr_id", pzCurrency->sCurrId, sizeof(pzCurrency->sCurrId));
            read_string(*g_selectPartCurrencyState.result, "sec_no", pzCurrency->sSecNo, sizeof(pzCurrency->sSecNo));
            read_string(*g_selectPartCurrencyState.result, "when_issue", pzCurrency->sWi, sizeof(pzCurrency->sWi));
            pzCurrency->fCurrExrate = g_selectPartCurrencyState.result->get<double>("cur_exrate", 0.0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_selectPartCurrencyState.cRows = 0;
            g_selectPartCurrencyState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "SelectPartCurrency", FALSE);
        g_selectPartCurrencyState.result.reset();
    }
}

// ============================================================================
// SelectAllPartPortmain
// ============================================================================
struct SelectAllPartPortmainState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};
static SelectAllPartPortmainState g_selectAllPartPortmainState;

DLLAPI void SelectAllPartPortmain(PARTPMAIN *pzPortmain, ERRSTRUCT *pzErr) 
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        if (!g_selectAllPartPortmainState.result)
        {
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectAllPartPortmain));
            g_selectAllPartPortmainState.result = nanodbc::execute(stmt);
            g_selectAllPartPortmainState.cRows = 0;
        }

        if (g_selectAllPartPortmainState.result && g_selectAllPartPortmainState.result->next())
        {
            g_selectAllPartPortmainState.cRows++;
            memset(pzPortmain, 0, sizeof(*pzPortmain));
            
            pzPortmain->iID = g_selectAllPartPortmainState.result->get<int>("Id", 0);
            read_string(*g_selectAllPartPortmainState.result, "unique_name", pzPortmain->sUniqueName, sizeof(pzPortmain->sUniqueName));
            pzPortmain->iFiscalMonth = g_selectAllPartPortmainState.result->get<int>("FiscalYearEndMonth", 0);
            pzPortmain->iFiscalDay = g_selectAllPartPortmainState.result->get<int>("FiscalYearEndDay", 0);
            
            pzPortmain->lInceptionDate = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("InceptionDate"));
            read_string(*g_selectAllPartPortmainState.result, "AcctMethod", pzPortmain->sAcctMethod, sizeof(pzPortmain->sAcctMethod));
            read_string(*g_selectAllPartPortmainState.result, "BaseCurrId", pzPortmain->sBaseCurrId, sizeof(pzPortmain->sBaseCurrId));
            
            char sTemp[1+NT];
            read_string(*g_selectAllPartPortmainState.result, "Income", sTemp, sizeof(sTemp)); pzPortmain->bIncome = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "Actions", sTemp, sizeof(sTemp)); pzPortmain->bActions = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "Mature", sTemp, sizeof(sTemp)); pzPortmain->bMature = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "CAvail", sTemp, sizeof(sTemp)); pzPortmain->bCAvail = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "FAvail", sTemp, sizeof(sTemp)); pzPortmain->bFAvail = Char2BOOL(sTemp);
            
            pzPortmain->lValDate = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("ValDate"));
            pzPortmain->lDeleteDate = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("DeleteDate"));
            read_string(*g_selectAllPartPortmainState.result, "IsInactive", sTemp, sizeof(sTemp)); pzPortmain->bIsInactive = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "AmortMuni", sTemp, sizeof(sTemp)); pzPortmain->bAmortMuni = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "AmortOther", sTemp, sizeof(sTemp)); pzPortmain->bAmortOther = Char2BOOL(sTemp);
            
            pzPortmain->lAmortStart = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("AmortStartDate"));
            read_string(*g_selectAllPartPortmainState.result, "IncByLot", sTemp, sizeof(sTemp)); pzPortmain->bIncByLot = Char2BOOL(sTemp);
            pzPortmain->lPurgeDate = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("PurgeDate"));
            pzPortmain->lRollDate = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("RollDate"));
            read_string(*g_selectAllPartPortmainState.result, "Tax", pzPortmain->sTax, sizeof(pzPortmain->sTax));
            pzPortmain->iPortfolioType = g_selectAllPartPortmainState.result->get<int>("PortfolioType", 0);
            pzPortmain->lPricingEffectiveDate = timestamp_to_long(g_selectAllPartPortmainState.result->get<nanodbc::timestamp>("PricingEffectiveDate"));
            
            pzPortmain->fMaxEqPct = g_selectAllPartPortmainState.result->get<double>("MaxEqPct", 0.0);
            pzPortmain->fMaxFiPct = g_selectAllPartPortmainState.result->get<double>("MaxFiPct", 0.0);
            pzPortmain->iVendorID = g_selectAllPartPortmainState.result->get<int>("DefaultVendorID", 0);
            pzPortmain->iReturnsToCalculate = g_selectAllPartPortmainState.result->get<int>("ReturnsToCalculate", 0);
            pzPortmain->iRorType = g_selectAllPartPortmainState.result->get<int>("DefaultReturnType", 0);
            
            // Column 30: case when mi.id is null then 'N' else 'Y' end
            // nanodbc access by index (0-based) or name. The query doesn't name this column.
            // Let's assume it's the 30th column (index 29).
            // Wait, nanodbc get by index is safer if name is missing.
            // Indices: Id(0)... DefaultReturnType(28), Case(29), AccretMuni(30), AccretOther(31)
            
            std::string sIsMarketIndex = g_selectAllPartPortmainState.result->get<std::string>(29, "N");
            pzPortmain->bIsMarketIndex = (sIsMarketIndex == "Y");
            
            read_string(*g_selectAllPartPortmainState.result, "AccretMuni", sTemp, sizeof(sTemp)); pzPortmain->bAccretMuni = Char2BOOL(sTemp);
            read_string(*g_selectAllPartPortmainState.result, "AccretOther", sTemp, sizeof(sTemp)); pzPortmain->bAccretOther = Char2BOOL(sTemp);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_selectAllPartPortmainState.cRows = 0;
            g_selectAllPartPortmainState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "SelectAllPartPortmain", FALSE);
        g_selectAllPartPortmainState.result.reset();
    }
}

// ============================================================================
// SelectOnePartPortmain
// ============================================================================
DLLAPI void SelectOnePartPortmain(PARTPMAIN *pzPortmain, int iID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectOnePartPortmain));
        
        stmt.bind(0, &iID);
        
        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            memset(pzPortmain, 0, sizeof(*pzPortmain));
            
            pzPortmain->iID = result.get<int>("Id", 0);
            read_string(result, "unique_name", pzPortmain->sUniqueName, sizeof(pzPortmain->sUniqueName));
            pzPortmain->iFiscalMonth = result.get<int>("FiscalYearEndMonth", 0);
            pzPortmain->iFiscalDay = result.get<int>("FiscalYearEndDay", 0);
            
            pzPortmain->lInceptionDate = timestamp_to_long(result.get<nanodbc::timestamp>("InceptionDate"));
            read_string(result, "AcctMethod", pzPortmain->sAcctMethod, sizeof(pzPortmain->sAcctMethod));
            read_string(result, "BaseCurrId", pzPortmain->sBaseCurrId, sizeof(pzPortmain->sBaseCurrId));
            
            char sTemp[1+NT];
            read_string(result, "Income", sTemp, sizeof(sTemp)); pzPortmain->bIncome = Char2BOOL(sTemp);
            read_string(result, "Actions", sTemp, sizeof(sTemp)); pzPortmain->bActions = Char2BOOL(sTemp);
            read_string(result, "Mature", sTemp, sizeof(sTemp)); pzPortmain->bMature = Char2BOOL(sTemp);
            read_string(result, "CAvail", sTemp, sizeof(sTemp)); pzPortmain->bCAvail = Char2BOOL(sTemp);
            read_string(result, "FAvail", sTemp, sizeof(sTemp)); pzPortmain->bFAvail = Char2BOOL(sTemp);
            
            pzPortmain->lValDate = timestamp_to_long(result.get<nanodbc::timestamp>("ValDate"));
            pzPortmain->lDeleteDate = timestamp_to_long(result.get<nanodbc::timestamp>("DeleteDate"));
            read_string(result, "IsInactive", sTemp, sizeof(sTemp)); pzPortmain->bIsInactive = Char2BOOL(sTemp);
            read_string(result, "AmortMuni", sTemp, sizeof(sTemp)); pzPortmain->bAmortMuni = Char2BOOL(sTemp);
            read_string(result, "AmortOther", sTemp, sizeof(sTemp)); pzPortmain->bAmortOther = Char2BOOL(sTemp);
            
            pzPortmain->lAmortStart = timestamp_to_long(result.get<nanodbc::timestamp>("AmortStartDate"));
            read_string(result, "IncByLot", sTemp, sizeof(sTemp)); pzPortmain->bIncByLot = Char2BOOL(sTemp);
            pzPortmain->lPurgeDate = timestamp_to_long(result.get<nanodbc::timestamp>("PurgeDate"));
            pzPortmain->lRollDate = timestamp_to_long(result.get<nanodbc::timestamp>("RollDate"));
            read_string(result, "Tax", pzPortmain->sTax, sizeof(pzPortmain->sTax));
            pzPortmain->iPortfolioType = result.get<int>("PortfolioType", 0);
            pzPortmain->lPricingEffectiveDate = timestamp_to_long(result.get<nanodbc::timestamp>("PricingEffectiveDate"));
            
            pzPortmain->fMaxEqPct = result.get<double>("MaxEqPct", 0.0);
            pzPortmain->fMaxFiPct = result.get<double>("MaxFiPct", 0.0);
            pzPortmain->iVendorID = result.get<int>("DefaultVendorID", 0);
            pzPortmain->iReturnsToCalculate = result.get<int>("ReturnsToCalculate", 0);
            pzPortmain->iRorType = result.get<int>("DefaultReturnType", 0);
            
            std::string sIsMarketIndex = result.get<std::string>(29, "N");
            pzPortmain->bIsMarketIndex = (sIsMarketIndex == "Y");
            
            read_string(result, "AccretMuni", sTemp, sizeof(sTemp)); pzPortmain->bAccretMuni = Char2BOOL(sTemp);
            read_string(result, "AccretOther", sTemp, sizeof(sTemp)); pzPortmain->bAccretOther = Char2BOOL(sTemp);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), iID, 0, "", 0, -1, 0, "SelectOnePartPortmain", FALSE);
    }
}

// ============================================================================
// SelectAllSubacct
// ============================================================================
struct SelectAllSubacctState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};
static SelectAllSubacctState g_selectAllSubacctState;

DLLAPI void SelectAllSubacct(char *sAcctType, char *sXrefAcctType, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) return;

    try 
    {
        if (!g_selectAllSubacctState.result)
        {
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectAllSubacct));
            g_selectAllSubacctState.result = nanodbc::execute(stmt);
            g_selectAllSubacctState.cRows = 0;
        }

        if (g_selectAllSubacctState.result && g_selectAllSubacctState.result->next())
        {
            g_selectAllSubacctState.cRows++;
            read_string(*g_selectAllSubacctState.result, "acct_type", sAcctType, STR1LEN);
            read_string(*g_selectAllSubacctState.result, "xref_acct_type", sXrefAcctType, STR1LEN);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_selectAllSubacctState.cRows = 0;
            g_selectAllSubacctState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, "", 0, -1, 0, "SelectAllSubacct", FALSE);
        g_selectAllSubacctState.result.reset();
    }
}

// ============================================================================
// SelectPortfolioRange
// ============================================================================
struct SelectPortfolioRangeState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
    int iID = 0;
};
static SelectPortfolioRangeState g_selectPortfolioRangeState;

// Helper to access the current ID from the state, used by GetAllPortfolios
int GetSelectPortfolioRangeID()
{
    return g_selectPortfolioRangeState.iID;
}

// Internal function to iterate, used by GetAllPortfolios
HRESULT MoveNextSelectPortfolioRange()
{
    if (!gConn.connected()) return E_FAIL;
    
    try 
    {
        if (!g_selectPortfolioRangeState.result)
        {
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT(SQL_SelectPortfolioRange));
            g_selectPortfolioRangeState.result = nanodbc::execute(stmt);
            g_selectPortfolioRangeState.cRows = 0;
        }

        if (g_selectPortfolioRangeState.result && g_selectPortfolioRangeState.result->next())
        {
            g_selectPortfolioRangeState.cRows++;
            g_selectPortfolioRangeState.iID = g_selectPortfolioRangeState.result->get<int>("id", 0);
            return S_OK;
        }
        else
        {
            g_selectPortfolioRangeState.cRows = 0;
            g_selectPortfolioRangeState.result.reset();
            return S_FALSE; // EOF
        }
    }
    catch (...)
    {
        g_selectPortfolioRangeState.result.reset();
        return E_FAIL;
    }
}

// Internal function to close/reset, used by GetAllPortfolios
void CloseSelectPortfolioRange()
{
    g_selectPortfolioRangeState.result.reset();
    g_selectPortfolioRangeState.cRows = 0;
}

// Internal function to clear state, called by DigenerateUtils
void FreeSelectPortfolioRangeState()
{
    g_selectPortfolioRangeState.result.reset();
    g_selectPortfolioRangeState.cRows = 0;
}
