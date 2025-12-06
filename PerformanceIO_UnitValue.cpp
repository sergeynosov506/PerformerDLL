/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_UnitValue.cpp
 * DESCRIPTION: Unit Value functions implementation
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-12-01
 */

#include "PerformanceIO_UnitValue.h"
#include "PerformanceIO_Common.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <iostream>
#include <vector>

// Static buffer for batch inserts
static std::vector<UNITVALUE> g_BatchUV;

DLLAPI void STDCALL DeleteDailyUnitValueForADate(long iPortfolioID, long iID, long lPerformDate, long iRorType, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteDailyUnitValueForADate: No connection", 0, 0, "E", 0, -1, 0, "DeleteDailyUnitValueForADate", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM UNITVALUE 
            WHERE ror_type >= ? AND 
            portfolio_id = ? AND id = ? AND UVDate = ?
        )");

        int idx = 0;
        stmt.bind(idx++, &iRorType);
        stmt.bind(idx++, &iPortfolioID);
        stmt.bind(idx++, &iID);
        
        nanodbc::date dPerform = LongToDateStruct(lPerformDate);
        stmt.bind(idx++, &dPerform);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteDailyUnitValueForADate");
    }
}

DLLAPI void STDCALL MarkPeriodUVForADateRangeAsDeleted(long iPortfolioID, long iID, long lPerformDate, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("MarkPeriodUVForADateRangeAsDeleted: No connection", 0, 0, "E", 0, -1, 0, "MarkPeriodUVForADateRangeAsDeleted", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            update unitvalue set ror_type = -1 * ror_type where ror_type > 0 and portfolio_id = ? and id = ? and uvdate >= ? and (ror_source <> ? or uvdate <= dbo.fn_PriorMonthEnd((select pricing_date from STARSDAT))) and  
            ( (uvdate <> (select pricing_date from starsdat)) or (not exists(select * from dsumdata d where d.id = unitvalue.id and d.perform_date = (select pricing_date from starsdat))))
        )");

        int idx = 0;
        stmt.bind(idx++, &iPortfolioID);
        stmt.bind(idx++, &iID);
        
        nanodbc::date dPerform = LongToDateStruct(lPerformDate);
        stmt.bind(idx++, &dPerform);
        
        int iRorSource = rsMonthToDate; // Fixed value from legacy code
        stmt.bind(idx++, &iRorSource);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "MarkPeriodUVForADateRangeAsDeleted");
    }
}

DLLAPI void STDCALL DeleteMarkedUnitValue(long iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteMarkedUnitValue: No connection", 0, 0, "E", 0, -1, 0, "DeleteMarkedUnitValue", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM UNITVALUE WHERE Ror_Type<0 AND UVDate > ? 
            AND id IN (SELECT id FROM SEGMAIN WHERE Owner_ID = ?)
        )");

        int idx = 0;
        nanodbc::date dPerform = LongToDateStruct(lPerformDate);
        stmt.bind(idx++, &dPerform);
        
        stmt.bind(idx++, &iPortfolioID);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteMarkedUnitValue");
    }
}

DLLAPI void STDCALL DeleteUnitValueSince(long iPortfolioID, long iID, long lBeginDate, int iRorType, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteUnitValueSince: No connection", 0, 0, "E", 0, -1, 0, "DeleteUnitValueSince", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM UNITVALUE WHERE Portfolio_ID = ? 
            AND id = ? AND UVDate >= ? AND ROR_type = ?
        )");

        int idx = 0;
        stmt.bind(idx++, &iPortfolioID);
        stmt.bind(idx++, &iID);
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(idx++, &dBegin);
        
        stmt.bind(idx++, &iRorType);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteUnitValueSince");
    }
}

DLLAPI void STDCALL DeleteUnitValueSince2(long iPortfolioID, long iID, long lBeginDate, int iRorType, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteUnitValueSince2: No connection", 0, 0, "E", 0, -1, 0, "DeleteUnitValueSince2", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM UNITVALUE WHERE Portfolio_ID = ? 
            AND id = ? AND UVDate >= ? AND ROR_type >= ?
        )");

        int idx = 0;
        stmt.bind(idx++, &iPortfolioID);
        stmt.bind(idx++, &iID);
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(idx++, &dBegin);
        
        stmt.bind(idx++, &iRorType);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteUnitValueSince2");
    }
}

DLLAPI void STDCALL DeleteUnitValueForPortfolioSince(long iPortfolioID, long lBeginDate, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteUnitValueForPortfolioSince: No connection", 0, 0, "E", 0, -1, 0, "DeleteUnitValueForPortfolioSince", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM UNITVALUE 
            WHERE ID in (SELECT id FROM Segmain WHERE Owner_ID = ?) 
            AND UVDate >= ?
        )");

        int idx = 0;
        stmt.bind(idx++, &iPortfolioID);
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(idx++, &dBegin);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteUnitValueForPortfolioSince");
    }
}

DLLAPI void STDCALL DeleteUnitValueForPortfolioSince2(long iPortfolioID, long lBeginDate, long iBeginRorType, long iEndRorType, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("DeleteUnitValueForPortfolioSince2: No connection", 0, 0, "E", 0, -1, 0, "DeleteUnitValueForPortfolioSince2", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            DELETE FROM UNITVALUE 
            WHERE ID in (SELECT id FROM Segmain WHERE Owner_ID = ?) 
            AND UVDate >= ? AND Ror_Type BETWEEN ? AND ?
        )");

        int idx = 0;
        stmt.bind(idx++, &iPortfolioID);
        
        nanodbc::date dBegin = LongToDateStruct(lBeginDate);
        stmt.bind(idx++, &dBegin);
        
        stmt.bind(idx++, &iBeginRorType);
        stmt.bind(idx++, &iEndRorType);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "DeleteUnitValueForPortfolioSince2");
    }
}

DLLAPI void STDCALL RecalcDailyUV(long iPortfolioID, long lStartDate, long lEndDate, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("RecalcDailyUV: No connection", 0, 0, "E", 0, -1, 0, "RecalcDailyUV", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            UPDATE UNITVALUE SET UnitValue = cast(UnitValue as float) * 
            IsNull((SELECT UnitValue 
            FROM UNITVALUE UV_New 
            WHERE UV_New.ID=UnitValue.ID AND UV_New.Stream_Begin_Date=UnitValue.Stream_Begin_Date 
            AND UV_New.Ror_type = UnitValue.Ror_type 
            AND UV_New.UVDate = ? ) / 
            (SELECT case when abs(UnitValue)<=0.00000001 then NULL else UnitValue end 
            FROM UNITVALUE UV_Old 
            WHERE UV_Old.ID=UnitValue.ID AND UV_Old.Stream_Begin_Date=UnitValue.Stream_Begin_Date 
            AND UV_Old.Ror_type = -1*UnitValue.Ror_type 
            AND UV_Old.UVDate = ? ), 1) 
            WHERE  UVDate > ? AND UVDate < ? 
            AND id IN (SELECT id FROM SEGMAIN WHERE Owner_ID = ?) 
            
            DELETE FROM Unitvalue 
            WHERE  UVDate > ? AND UVDate <= ? 
            AND id IN (SELECT id FROM SEGMAIN WHERE Owner_ID = ?) 
            AND EXISTS (SELECT * FROM perfreturntype AS exclude 
            WHERE exclude.id = Unitvalue.Portfolio_ID AND exclude.itemtype = 9 
            AND exclude.returntype = Unitvalue.Ror_Type)
        )");

        int idx = 0;
        nanodbc::date dStart = LongToDateStruct(lStartDate);
        nanodbc::date dEnd = LongToDateStruct(lEndDate);

        stmt.bind(idx++, &dStart); // UV_New.UVDate
        stmt.bind(idx++, &dStart); // UV_Old.UVDate
        stmt.bind(idx++, &dStart); // UVDate > ?
        stmt.bind(idx++, &dEnd);   // UVDate < ?
        stmt.bind(idx++, &iPortfolioID); // Owner_ID
        
        stmt.bind(idx++, &dStart); // UVDate > ?
        stmt.bind(idx++, &dEnd);   // UVDate <= ?
        stmt.bind(idx++, &iPortfolioID); // Owner_ID

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "RecalcDailyUV");
    }
}

DLLAPI void STDCALL InsertUnitValue(UNITVALUE zUV, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("InsertUnitValue: No connection", 0, 0, "E", 0, -1, 0, "InsertUnitValue", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            INSERT INTO UNITVALUE (Portfolio_ID, ID, Ror_Type, 
                                    UVDate, UnitValue, Fudge_factor, 
                                    Stream_Begin_Date, Ror_Source ) 
                                    VALUES 
                                    (?, ?, ?, ?, ?, ?, ?, ?)
        )");

        int idx = 0;
        stmt.bind(idx++, &zUV.iPortfolioID);
        stmt.bind(idx++, &zUV.iID);
        stmt.bind(idx++, &zUV.iRorType);
        
        nanodbc::date dUV = LongToDateStruct(zUV.lUVDate);
        stmt.bind(idx++, &dUV);
        
        double fVal = RoundDouble(zUV.fUnitValue, 8);
        stmt.bind(idx++, &fVal);
        
        stmt.bind(idx++, &zUV.fFudgeFactor);
        
        nanodbc::date dStream = LongToDateStruct(zUV.lStreamBeginDate);
        stmt.bind(idx++, &dStream);
        
        stmt.bind(idx++, &zUV.iRorSource);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "InsertUnitValue");
    }
}

DLLAPI void STDCALL InsertUnitValueBatch(UNITVALUE* pzUV, long lBatchSize, ERRSTRUCT *pzErr)
{
    *pzErr = {};
    try
    {
        if (lBatchSize < 0)
        {
            g_BatchUV.clear();
            return;
        }

        if (pzUV)
        {
            g_BatchUV.push_back(*pzUV);
        }

        if (g_BatchUV.size() >= (size_t)lBatchSize && !g_BatchUV.empty())
        {
            nanodbc::connection* conn = GetDbConnection();
            if (!conn) throw std::runtime_error("No connection");

            nanodbc::transaction trans(*conn);
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, R"(
                INSERT INTO UNITVALUE (Portfolio_ID, ID, Ror_Type, 
                                        UVDate, UnitValue, Fudge_factor, 
                                        Stream_Begin_Date, Ror_Source ) 
                                        VALUES 
                                        (?, ?, ?, ?, ?, ?, ?, ?)
            )");

            for (const auto& uv : g_BatchUV)
            {
                int idx = 0;
                stmt.bind(idx++, &uv.iPortfolioID);
                stmt.bind(idx++, &uv.iID);
                stmt.bind(idx++, &uv.iRorType);
                
                nanodbc::date dUV = LongToDateStruct(uv.lUVDate);
                stmt.bind(idx++, &dUV);
                
                double fVal = RoundDouble(uv.fUnitValue, 8);
                stmt.bind(idx++, &fVal);
                
                stmt.bind(idx++, &uv.fFudgeFactor);
                
                nanodbc::date dStream = LongToDateStruct(uv.lStreamBeginDate);
                stmt.bind(idx++, &dStream);
                
                stmt.bind(idx++, &uv.iRorSource);
                
                nanodbc::execute(stmt);
            }
            
            trans.commit();
            g_BatchUV.clear();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "InsertUnitValueBatch");
    }
}

DLLAPI void STDCALL SelectUnitValue(UNITVALUE *pzUV, int iPortfolioID, long lPerformDate, ERRSTRUCT *pzErr)
{
    static std::optional<nanodbc::result> s_result;
    static int s_iPortfolioID = 0;
    static long s_lPerformDate = 0;

    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("SelectUnitValue: No connection", 0, 0, "E", 0, -1, 0, "SelectUnitValue", FALSE);
            return;
        }

        bool bNewQuery = true;
        if (s_result && s_iPortfolioID == iPortfolioID && s_lPerformDate == lPerformDate)
        {
            bNewQuery = false;
        }

        if (bNewQuery)
        {
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, R"(
                SELECT Portfolio_ID, U.ID, Ror_Type, 
                    UVDate, UnitValue, Fudge_factor, 
                    Stream_Begin_Date, Ror_Source 
                FROM UNITVALUE U, SEGMAIN S 
                WHERE S.ID=U.ID and S.owner_id=? and U.uvdate=?
            )");

            stmt.bind(0, &iPortfolioID);
            nanodbc::date dPerform = LongToDateStruct(lPerformDate);
            stmt.bind(1, &dPerform);

            s_result = stmt.execute();
            s_iPortfolioID = iPortfolioID;
            s_lPerformDate = lPerformDate;
        }

        if (s_result && s_result->next())
        {
            int col = 0;
            pzUV->iPortfolioID = s_result->get<int>(col++);
            pzUV->iID = s_result->get<int>(col++);
            pzUV->iRorType = s_result->get<int>(col++);
            
            read_date(*s_result, col++, &pzUV->lUVDate);
            read_double(*s_result, col++, &pzUV->fUnitValue);
            read_double(*s_result, col++, &pzUV->fFudgeFactor);
            read_date(*s_result, col++, &pzUV->lStreamBeginDate);
            pzUV->iRorSource = s_result->get<int>(col++);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            s_result.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectUnitValue");
        s_result.reset();
    }
}

DLLAPI void STDCALL SelectOneUnitValue(UNITVALUE *pzUV, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("SelectOneUnitValue: No connection", 0, 0, "E", 0, -1, 0, "SelectOneUnitValue", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            SELECT Portfolio_ID, ID, Ror_Type, 
                    UVDate, UnitValue, Fudge_factor, 
                    Stream_Begin_Date, Ror_Source  
            FROM UnitValue 
            WHERE Portfolio_ID = ? AND ID = ? AND Ror_Type = ? 
                    AND UVDate = ?
        )");

        int idx = 0;
        stmt.bind(idx++, &pzUV->iPortfolioID);
        stmt.bind(idx++, &pzUV->iID);
        stmt.bind(idx++, &pzUV->iRorType);
        
        nanodbc::date dUV = LongToDateStruct(pzUV->lUVDate);
        stmt.bind(idx++, &dUV);

        nanodbc::result result = stmt.execute();

        if (result.next())
        {
            int col = 0;
            pzUV->iPortfolioID = result.get<int>(col++);
            pzUV->iID = result.get<int>(col++);
            pzUV->iRorType = result.get<int>(col++);
            
            read_date(result, col++, &pzUV->lUVDate);
            read_double(result, col++, &pzUV->fUnitValue);
            read_double(result, col++, &pzUV->fFudgeFactor);
            read_date(result, col++, &pzUV->lStreamBeginDate);
            pzUV->iRorSource = result.get<int>(col++);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectOneUnitValue");
    }
}

DLLAPI void STDCALL SelectUnitValueRange(UNITVALUE *pzUV, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    static std::optional<nanodbc::result> s_result;
    static UNITVALUE s_zUV = {};
    static long s_lBeginDate = 0;
    static long s_lEndDate = 0;

    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("SelectUnitValueRange: No connection", 0, 0, "E", 0, -1, 0, "SelectUnitValueRange", FALSE);
            return;
        }

        bool bNewQuery = true;
        if (s_result && 
            s_zUV.iPortfolioID == pzUV->iPortfolioID &&
            s_zUV.iID == pzUV->iID &&
            s_zUV.iRorType == pzUV->iRorType &&
            s_lBeginDate == lBeginDate &&
            s_lEndDate == lEndDate)
        {
            bNewQuery = false;
        }

        if (bNewQuery)
        {
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, R"(
                SELECT Portfolio_ID, ID, Ror_Type, 
                        UVDate, UnitValue, Fudge_factor, 
                        Stream_Begin_Date, Ror_Source  
                FROM UnitValue 
                WHERE Portfolio_ID = ? AND ID = ? AND Ror_Type = ? 
                        AND UVDate >= ? AND UVDate <= ? 
                ORDER BY UVDate
            )");

            int idx = 0;
            stmt.bind(idx++, &pzUV->iPortfolioID);
            stmt.bind(idx++, &pzUV->iID);
            stmt.bind(idx++, &pzUV->iRorType);
            
            nanodbc::date dBegin = LongToDateStruct(lBeginDate);
            stmt.bind(idx++, &dBegin);
            
            nanodbc::date dEnd = LongToDateStruct(lEndDate);
            stmt.bind(idx++, &dEnd);

            s_result = stmt.execute();
            s_zUV = *pzUV;
            s_lBeginDate = lBeginDate;
            s_lEndDate = lEndDate;
        }

        if (s_result && s_result->next())
        {
            int col = 0;
            pzUV->iPortfolioID = s_result->get<int>(col++);
            pzUV->iID = s_result->get<int>(col++);
            pzUV->iRorType = s_result->get<int>(col++);
            
            read_date(*s_result, col++, &pzUV->lUVDate);
            read_double(*s_result, col++, &pzUV->fUnitValue);
            read_double(*s_result, col++, &pzUV->fFudgeFactor);
            read_date(*s_result, col++, &pzUV->lStreamBeginDate);
            pzUV->iRorSource = s_result->get<int>(col++);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            s_result.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectUnitValueRange");
        s_result.reset();
    }
}

DLLAPI void STDCALL SelectUnitValueRange2(UNITVALUE *pzUV, int iPortfolioID, long lBeginDate, long lEndDate, ERRSTRUCT *pzErr)
{
    static std::optional<nanodbc::result> s_result;
    static int s_iPortfolioID = 0;
    static long s_lBeginDate = 0;
    static long s_lEndDate = 0;

    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("SelectUnitValueRange2: No connection", 0, 0, "E", 0, -1, 0, "SelectUnitValueRange2", FALSE);
            return;
        }

        bool bNewQuery = true;
        if (s_result && 
            s_iPortfolioID == iPortfolioID &&
            s_lBeginDate == lBeginDate &&
            s_lEndDate == lEndDate)
        {
            bNewQuery = false;
        }

        if (bNewQuery)
        {
            nanodbc::statement stmt(*conn);
            nanodbc::prepare(stmt, R"(
                SELECT Portfolio_ID, u.ID, Ror_Type, 
                        UVDate, UnitValue, Fudge_factor, 
                        Stream_Begin_Date, Ror_Source  
                FROM portmain p join segmain sm on sm.owner_id = p.id join UnitValue u on u.id = sm.id 
                WHERE p.id = ? AND Ror_Type = p.defaultreturntype 
                        AND UVDate >= ? AND UVDate <= ? and ror_source = 4 
                ORDER BY UVDate
            )");

            int idx = 0;
            stmt.bind(idx++, &iPortfolioID);
            
            nanodbc::date dBegin = LongToDateStruct(lBeginDate);
            stmt.bind(idx++, &dBegin);
            
            nanodbc::date dEnd = LongToDateStruct(lEndDate);
            stmt.bind(idx++, &dEnd);

            s_result = stmt.execute();
            s_iPortfolioID = iPortfolioID;
            s_lBeginDate = lBeginDate;
            s_lEndDate = lEndDate;
        }

        if (s_result && s_result->next())
        {
            int col = 0;
            pzUV->iPortfolioID = s_result->get<int>(col++);
            pzUV->iID = s_result->get<int>(col++);
            pzUV->iRorType = s_result->get<int>(col++);
            
            read_date(*s_result, col++, &pzUV->lUVDate);
            read_double(*s_result, col++, &pzUV->fUnitValue);
            read_double(*s_result, col++, &pzUV->fFudgeFactor);
            read_date(*s_result, col++, &pzUV->lStreamBeginDate);
            pzUV->iRorSource = s_result->get<int>(col++);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            s_result.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectUnitValueRange2");
        s_result.reset();
    }
}

DLLAPI void STDCALL UpdateUnitValue(UNITVALUE zUV, long lOldDate, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("UpdateUnitValue: No connection", 0, 0, "E", 0, -1, 0, "UpdateUnitValue", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            UPDATE UNITVALUE SET Stream_Begin_Date = ?, 
            UVDate = ?, UnitValue = ? 
            WHERE Portfolio_ID = ? AND ID = ? AND Ror_Type = ? 
            AND UVDate = ?
        )");

        int idx = 0;
        nanodbc::date dStream = LongToDateStruct(zUV.lStreamBeginDate);
        stmt.bind(idx++, &dStream);
        
        nanodbc::date dUV = LongToDateStruct(zUV.lUVDate);
        stmt.bind(idx++, &dUV);
        
        double fVal = RoundDouble(zUV.fUnitValue, 8);
        stmt.bind(idx++, &fVal);
        
        stmt.bind(idx++, &zUV.iPortfolioID);
        stmt.bind(idx++, &zUV.iID);
        stmt.bind(idx++, &zUV.iRorType);
        
        nanodbc::date dOld = LongToDateStruct(lOldDate);
        stmt.bind(idx++, &dOld);

        nanodbc::result res = stmt.execute();
        if (res.affected_rows() == 0)
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "UpdateUnitValue");
    }
}
