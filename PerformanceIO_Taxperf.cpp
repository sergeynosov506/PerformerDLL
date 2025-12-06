/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Taxperf.cpp
 * DESCRIPTION: Tax Performance functions implementation
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-12-01
 */

#include "PerformanceIO_Taxperf.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <iostream>

DLLAPI void STDCALL DeleteTaxperf(int iPortfolioID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteTaxperf: No connection", 0, 0, "E", 0, -1, 0, "DeleteTaxperf", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM Taxperf 
            WHERE ID in (SELECT id FROM Segmain WHERE Owner_ID = ?) 
            AND Perform_Date > ? AND Perform_Date <= ?
        )");

        stmt.bind(0, &iPortfolioID);
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(1, &dBegin);
        
        nanodbc::date dEnd = LongToDateStruct(lEndDate);
        stmt.bind(2, &dEnd);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteTaxperf");
    }
}

DLLAPI void STDCALL InsertTaxperf(TAXPERF zTP, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("InsertTaxperf: No connection", 0, 0, "E", 0, -1, 0, "InsertTaxperf", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            INSERT INTO TAXPERF (
                Portfolio_ID, ID, Perform_Date, 
                Fedinctax_Wthld, Cum_Fedinctax_Wthld, Wtd_Fedinctax_Wthld, 
                Fedtax_Rclm, Cum_Fedtax_Rclm, Wtd_Fedtax_Rclm, 
                Fedetax_Inc, Cum_Fedetax_Inc, Wtd_Fedetax_Inc, 
                Fedatax_Inc, Cum_Fedatax_Inc, Wtd_Fedatax_Inc, 
                Fedetax_Strgl, Cum_Fedetax_Strgl, Wtd_Fedetax_Strgl, 
                Fedetax_Ltrgl, Cum_Fedetax_Ltrgl, Wtd_Fedetax_Ltrgl, 
                Fedetax_Crrgl, Cum_Fedetax_Crrgl, Wtd_Fedetax_Crrgl, 
                Fedatax_Strgl, Cum_Fedatax_Strgl, Wtd_Fedatax_Strgl, 
                Fedatax_Ltrgl, Cum_Fedatax_Ltrgl, Wtd_Fedatax_Ltrgl, 
                Fedatax_Crrgl, Cum_Fedatax_Crrgl, Wtd_Fedatax_Crrgl, 
                Fedatax_Accr_Inc, Fedatax_Accr_Div, Fedatax_Inc_Rclm, Fedatax_Div_Rclm, 
                Fedetax_Accr_Inc, Fedetax_Accr_Div, Fedetax_Inc_Rclm, Fedetax_Div_Rclm, 
                Exch_Rate_Base, Exch_Rate_Sys, 
                Fedatax_Amort, Cum_Fedatax_Amort, Wtd_Fedatax_Amort, 
                Fedetax_Amort, Cum_Fedetax_Amort, Wtd_Fedetax_Amort
            ) VALUES (
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, 
                ?, ?, ?, ?, 
                ?, ?, ?, ?, 
                ?, ?, 
                ?, ?, ?, 
                ?, ?, ?
            )
        )");

        int idx = 0;
        stmt.bind(idx++, &zTP.iPortfolioID);
        stmt.bind(idx++, &zTP.iID);
        
        nanodbc::date dPerform = LongToDateStruct(zTP.lPerformDate);
        stmt.bind(idx++, &dPerform);

        stmt.bind(idx++, &zTP.fFedinctaxWthld);
        stmt.bind(idx++, &zTP.fCumFedinctaxWthld);
        stmt.bind(idx++, &zTP.fWtdFedinctaxWthld);
        
        stmt.bind(idx++, &zTP.fFedtaxRclm);
        stmt.bind(idx++, &zTP.fCumFedtaxRclm);
        stmt.bind(idx++, &zTP.fWtdFedtaxRclm);
        
        stmt.bind(idx++, &zTP.fFedetaxInc);
        stmt.bind(idx++, &zTP.fCumFedetaxInc);
        stmt.bind(idx++, &zTP.fWtdFedetaxInc);
        
        stmt.bind(idx++, &zTP.fFedataxInc);
        stmt.bind(idx++, &zTP.fCumFedataxInc);
        stmt.bind(idx++, &zTP.fWtdFedataxInc);
        
        stmt.bind(idx++, &zTP.fFedetaxStrgl);
        stmt.bind(idx++, &zTP.fCumFedetaxStrgl);
        stmt.bind(idx++, &zTP.fWtdFedetaxStrgl);
        
        stmt.bind(idx++, &zTP.fFedetaxLtrgl);
        stmt.bind(idx++, &zTP.fCumFedetaxLtrgl);
        stmt.bind(idx++, &zTP.fWtdFedetaxLtrgl);
        
        stmt.bind(idx++, &zTP.fFedetaxCrrgl);
        stmt.bind(idx++, &zTP.fCumFedetaxCrrgl);
        stmt.bind(idx++, &zTP.fWtdFedetaxCrrgl);
        
        stmt.bind(idx++, &zTP.fFedataxStrgl);
        stmt.bind(idx++, &zTP.fCumFedataxStrgl);
        stmt.bind(idx++, &zTP.fWtdFedataxStrgl);
        
        stmt.bind(idx++, &zTP.fFedataxLtrgl);
        stmt.bind(idx++, &zTP.fCumFedataxLtrgl);
        stmt.bind(idx++, &zTP.fWtdFedataxLtrgl);
        
        stmt.bind(idx++, &zTP.fFedataxCrrgl);
        stmt.bind(idx++, &zTP.fCumFedataxCrrgl);
        stmt.bind(idx++, &zTP.fWtdFedataxCrrgl);
        
        stmt.bind(idx++, &zTP.fFedataxAccrInc);
        stmt.bind(idx++, &zTP.fFedataxAccrDiv);
        stmt.bind(idx++, &zTP.fFedataxIncRclm);
        stmt.bind(idx++, &zTP.fFedataxDivRclm);
        
        stmt.bind(idx++, &zTP.fFedetaxAccrInc);
        stmt.bind(idx++, &zTP.fFedetaxAccrDiv);
        stmt.bind(idx++, &zTP.fFedetaxIncRclm);
        stmt.bind(idx++, &zTP.fFedetaxDivRclm);
        
        stmt.bind(idx++, &zTP.fExchRateBase);
        stmt.bind(idx++, &zTP.fExchRateSys);
        
        stmt.bind(idx++, &zTP.fFedataxAmort);
        stmt.bind(idx++, &zTP.fCumFedataxAmort);
        stmt.bind(idx++, &zTP.fWtdFedataxAmort);
        
        stmt.bind(idx++, &zTP.fFedetaxAmort);
        stmt.bind(idx++, &zTP.fCumFedetaxAmort);
        stmt.bind(idx++, &zTP.fWtdFedetaxAmort);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "InsertTaxperf");
    }
}

DLLAPI void STDCALL SelectTaxperf(int iPortfolioID, long lPerformDate, TAXPERF *pzTP, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("SelectTaxperf: No connection", 0, 0, "E", 0, -1, 0, "SelectTaxperf", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            SELECT T.Portfolio_ID, T.ID, Perform_Date, 
            Fedinctax_Wthld, Cum_Fedinctax_Wthld, Wtd_Fedinctax_Wthld, 
            Fedtax_Rclm, Cum_Fedtax_Rclm, Wtd_Fedtax_Rclm, 
            Fedetax_Inc, Cum_Fedetax_Inc, Wtd_Fedetax_Inc, 
            Fedatax_Inc, Cum_Fedatax_Inc, Wtd_Fedatax_Inc, 
            Fedetax_Strgl, Cum_Fedetax_Strgl, Wtd_Fedetax_Strgl, 
            Fedetax_Ltrgl, Cum_Fedetax_Ltrgl, Wtd_Fedetax_Ltrgl, 
            Fedetax_Crrgl, Cum_Fedetax_Crrgl, Wtd_Fedetax_Crrgl, 
            Fedatax_Strgl, Cum_Fedatax_Strgl, Wtd_Fedatax_Strgl, 
            Fedatax_Ltrgl, Cum_Fedatax_Ltrgl, Wtd_Fedatax_Ltrgl, 
            Fedatax_Crrgl, Cum_Fedatax_Crrgl, Wtd_Fedatax_Crrgl, 
            Fedatax_Accr_Inc, Fedatax_Accr_Div, Fedatax_Inc_Rclm, Fedatax_Div_Rclm, 
            Fedetax_Accr_Inc, Fedetax_Accr_Div, Fedetax_Inc_Rclm, Fedetax_Div_Rclm, 
            Exch_Rate_Base, Exch_Rate_Sys, 
            Fedatax_Amort, Cum_Fedatax_Amort, Wtd_Fedatax_Amort, 
            Fedetax_Amort, Cum_Fedetax_Amort, Wtd_Fedetax_Amort 
            FROM Taxperf T, Segmain S 
            WHERE T.ID = S.ID AND S.owner_id = ? AND perform_date = ?
        )");

        stmt.bind(0, &iPortfolioID);
        
        nanodbc::date dPerform = LongToDateStruct(lPerformDate);
        stmt.bind(1, &dPerform);

        nanodbc::result result = stmt.execute();

        if (result.next())
        {
            int col = 0;
            pzTP->iPortfolioID = result.get<int>(col++);
            pzTP->iID = result.get<int>(col++);
            
            read_date(result, col++, &pzTP->lPerformDate);
            
            read_double(result, col++, &pzTP->fFedinctaxWthld);
            read_double(result, col++, &pzTP->fCumFedinctaxWthld);
            read_double(result, col++, &pzTP->fWtdFedinctaxWthld);
            
            read_double(result, col++, &pzTP->fFedtaxRclm);
            read_double(result, col++, &pzTP->fCumFedtaxRclm);
            read_double(result, col++, &pzTP->fWtdFedtaxRclm);
            
            read_double(result, col++, &pzTP->fFedetaxInc);
            read_double(result, col++, &pzTP->fCumFedetaxInc);
            read_double(result, col++, &pzTP->fWtdFedetaxInc);
            
            read_double(result, col++, &pzTP->fFedataxInc);
            read_double(result, col++, &pzTP->fCumFedataxInc);
            read_double(result, col++, &pzTP->fWtdFedataxInc);
            
            read_double(result, col++, &pzTP->fFedetaxStrgl);
            read_double(result, col++, &pzTP->fCumFedetaxStrgl);
            read_double(result, col++, &pzTP->fWtdFedetaxStrgl);
            
            read_double(result, col++, &pzTP->fFedetaxLtrgl);
            read_double(result, col++, &pzTP->fCumFedetaxLtrgl);
            read_double(result, col++, &pzTP->fWtdFedetaxLtrgl);
            
            read_double(result, col++, &pzTP->fFedetaxCrrgl);
            read_double(result, col++, &pzTP->fCumFedetaxCrrgl);
            read_double(result, col++, &pzTP->fWtdFedetaxCrrgl);
            
            read_double(result, col++, &pzTP->fFedataxStrgl);
            read_double(result, col++, &pzTP->fCumFedataxStrgl);
            read_double(result, col++, &pzTP->fWtdFedataxStrgl);
            
            read_double(result, col++, &pzTP->fFedataxLtrgl);
            read_double(result, col++, &pzTP->fCumFedataxLtrgl);
            read_double(result, col++, &pzTP->fWtdFedataxLtrgl);
            
            read_double(result, col++, &pzTP->fFedataxCrrgl);
            read_double(result, col++, &pzTP->fCumFedataxCrrgl);
            read_double(result, col++, &pzTP->fWtdFedataxCrrgl);
            
            read_double(result, col++, &pzTP->fFedataxAccrInc);
            read_double(result, col++, &pzTP->fFedataxAccrDiv);
            read_double(result, col++, &pzTP->fFedataxIncRclm);
            read_double(result, col++, &pzTP->fFedataxDivRclm);
            
            read_double(result, col++, &pzTP->fFedetaxAccrInc);
            read_double(result, col++, &pzTP->fFedetaxAccrDiv);
            read_double(result, col++, &pzTP->fFedetaxIncRclm);
            read_double(result, col++, &pzTP->fFedetaxDivRclm);
            
            read_double(result, col++, &pzTP->fExchRateBase);
            read_double(result, col++, &pzTP->fExchRateSys);
            
            read_double(result, col++, &pzTP->fFedataxAmort);
            read_double(result, col++, &pzTP->fCumFedataxAmort);
            read_double(result, col++, &pzTP->fWtdFedataxAmort);
            
            read_double(result, col++, &pzTP->fFedetaxAmort);
            read_double(result, col++, &pzTP->fCumFedetaxAmort);
            read_double(result, col++, &pzTP->fWtdFedetaxAmort);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectTaxperf");
    }
}
