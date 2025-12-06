/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Scripts.cpp
 * DESCRIPTION: Performance Script and Template functions implementation
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "PerformanceIO_Scripts.h"
#include "PerformanceIO_Common.h" // For DB connection and error handling
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <optional>
#include <iostream>

// Static state for SelectAllScriptHeaderAndDetails
static std::optional<nanodbc::result> g_ScriptResult;
static bool g_ScriptResultOpen = false;

// Static state for SelectAllTemplateHeaderAndDetails
static std::optional<nanodbc::result> g_TemplateResult;
static bool g_TemplateResultOpen = false;

DLLAPI void STDCALL InsertPerfscriptDetail(PSCRDET zPSDetail, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "InsertPerfscriptDetail", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            INSERT INTO pscrdet 
            (scrhdr_no, seq_no, select_type, comparison_rule, 
            begin_point, end_point, and_or_logic, 
            include_exclude, mask_rest, match_rest, mask_wild, 
            match_wild, mask_expand, match_expand, report_dest, 
            start_date, end_date) 
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");

        stmt.bind(0, &zPSDetail.lScrhdrNo);
        stmt.bind(1, &zPSDetail.lSeqNo);
        stmt.bind(2, zPSDetail.sSelectType);
        stmt.bind(3, zPSDetail.sComparisonRule);
        stmt.bind(4, zPSDetail.sBeginPoint);
        stmt.bind(5, zPSDetail.sEndPoint);
        stmt.bind(6, zPSDetail.sAndOrLogic);
        stmt.bind(7, zPSDetail.sIncludeExclude);
        stmt.bind(8, zPSDetail.sMaskRest);
        stmt.bind(9, zPSDetail.sMatchRest);
        stmt.bind(10, zPSDetail.sMaskWild);
        stmt.bind(11, zPSDetail.sMatchWild);
        stmt.bind(12, zPSDetail.sMaskExpand);
        stmt.bind(13, zPSDetail.sMatchExpand);
        stmt.bind(14, zPSDetail.sReportDest);
        
        // Date handling
        nanodbc::date startDate = (zPSDetail.lStartDate > 0) ? LongToDateStruct(zPSDetail.lStartDate) : nanodbc::date();
        nanodbc::date endDate = (zPSDetail.lEndDate > 0) ? LongToDateStruct(zPSDetail.lEndDate) : nanodbc::date();
        
        if (zPSDetail.lStartDate > 0) stmt.bind(15, &startDate); else stmt.bind_null(15);
        if (zPSDetail.lEndDate > 0) stmt.bind(16, &endDate); else stmt.bind_null(16);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "InsertPerfscriptDetail");
    }
}

DLLAPI void STDCALL InsertPerfscriptHeader(PSCRHDR zPSHeader, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "InsertPerfscriptHeader", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            INSERT INTO pscrhdr 
            (scrhdr_no, tmphdr_no, hash_key,  
            segmenttype_id, owner, create_date, created_by, changeable, 
            description, change_date, changed_by, hdr_key) 
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");

        stmt.bind(0, &zPSHeader.lScrhdrNo);
        stmt.bind(1, &zPSHeader.lTmphdrNo);
        stmt.bind(2, &zPSHeader.lHashKey);
        stmt.bind(3, &zPSHeader.iSegmentTypeID);
        stmt.bind(4, zPSHeader.sOwner);
        
        nanodbc::date createDate = (zPSHeader.lCreateDate > 0) ? LongToDateStruct(zPSHeader.lCreateDate) : nanodbc::date();
        if (zPSHeader.lCreateDate > 0) stmt.bind(5, &createDate); else stmt.bind_null(5);
        
        stmt.bind(6, zPSHeader.sCreatedBy);
        stmt.bind(7, zPSHeader.sChangeable);
        stmt.bind(8, zPSHeader.sDescription);
        
        nanodbc::date changeDate = (zPSHeader.lChangeDate > 0) ? LongToDateStruct(zPSHeader.lChangeDate) : nanodbc::date();
        if (zPSHeader.lChangeDate > 0) stmt.bind(9, &changeDate); else stmt.bind_null(9);
        
        stmt.bind(10, zPSHeader.sChangedBy);
        stmt.bind(11, zPSHeader.sHdrKey);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "InsertPerfscriptHeader");
    }
}

DLLAPI void STDCALL UpdatePerfscriptHeader(PSCRHDR zPSHeader, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "UpdatePerfscriptHeader", FALSE);
            return;
        }

        nanodbc::statement stmt(*conn);
        nanodbc::prepare(stmt, R"(
            UPDATE pscrhdr SET hdr_key = ?, change_date = ?, changed_by = ?
            WHERE scrhdr_no = ? AND tmphdr_no = ? AND hash_key = ?
        )");

        stmt.bind(0, zPSHeader.sHdrKey);
        
        nanodbc::date changeDate = (zPSHeader.lChangeDate > 0) ? LongToDateStruct(zPSHeader.lChangeDate) : nanodbc::date();
        if (zPSHeader.lChangeDate > 0) stmt.bind(1, &changeDate); else stmt.bind_null(1);
        
        stmt.bind(2, zPSHeader.sChangedBy);
        stmt.bind(3, &zPSHeader.lScrhdrNo);
        stmt.bind(4, &zPSHeader.lTmphdrNo);
        stmt.bind(5, &zPSHeader.lHashKey);

        nanodbc::execute(stmt);
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "UpdatePerfscriptHeader");
    }
}

DLLAPI void STDCALL SelectAllScriptHeaderAndDetails(PSCRHDR *pzPSHeader, PSCRDET *pzPSDetail, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "SelectAllScriptHeaderAndDetails", FALSE);
            return;
        }

        if (!g_ScriptResultOpen)
        {
            // Using the query from legacy code
            g_ScriptResult = nanodbc::execute(*conn, R"(
                SELECT ph.scrhdr_no, tmphdr_no, hash_key, segmenttype_id, owner, 
                create_date, created_by, changeable, description, 
                change_date, changed_by, hdr_key, 
                s.level_id, s.groupid, 
                seq_no, select_type, comparison_rule, 
                begin_point, end_point, and_or_logic, 
                include_exclude, mask_rest, match_rest, mask_wild, 
                match_wild, mask_expand, match_expand, report_dest, 
                start_date, end_date 
                FROM pscrhdr ph LEFT OUTER JOIN pscrdet pd 
                    ON ph.scrhdr_no = pd.scrhdr_no, segments s 
                WHERE ph.segmenttype_id = s.id 
                ORDER BY ph.scrhdr_no, seq_no DESC
            )");
            
            g_ScriptResultOpen = true;
        }

        if (g_ScriptResult && g_ScriptResult->next())
        {
            // Populate PSCRHDR
            pzPSHeader->lScrhdrNo = g_ScriptResult->get<long>(0);
            pzPSHeader->lTmphdrNo = g_ScriptResult->get<long>(1);
            pzPSHeader->lHashKey = g_ScriptResult->get<long>(2);
            pzPSHeader->iSegmentTypeID = g_ScriptResult->get<int>(3);
            strcpy(pzPSHeader->sOwner, g_ScriptResult->get<nanodbc::string>(4).c_str());
            
            pzPSHeader->lCreateDate = DateStructToLong(g_ScriptResult->get<nanodbc::date>(5));
            strcpy(pzPSHeader->sCreatedBy, g_ScriptResult->get<nanodbc::string>(6).c_str());
            strcpy(pzPSHeader->sChangeable, g_ScriptResult->get<nanodbc::string>(7).c_str());
            strcpy(pzPSHeader->sDescription, g_ScriptResult->get<nanodbc::string>(8).c_str());
            
            pzPSHeader->lChangeDate = DateStructToLong(g_ScriptResult->get<nanodbc::date>(9));
            strcpy(pzPSHeader->sChangedBy, g_ScriptResult->get<nanodbc::string>(10).c_str());
            strcpy(pzPSHeader->sHdrKey, g_ScriptResult->get<nanodbc::string>(11).c_str());
            
            int iLevelID = g_ScriptResult->get<int>(12);
            if (iLevelID == 1)
                pzPSHeader->iGroupID = g_ScriptResult->get<int>(13);
            else
                pzPSHeader->iGroupID = 0;

            // Populate PSCRDET
            if (!g_ScriptResult->is_null(14)) // seq_no is not null
            {
                pzPSDetail->lSeqNo = g_ScriptResult->get<long>(14);
                strcpy(pzPSDetail->sSelectType, g_ScriptResult->get<nanodbc::string>(15).c_str());
                strcpy(pzPSDetail->sComparisonRule, g_ScriptResult->get<nanodbc::string>(16).c_str());
                strcpy(pzPSDetail->sBeginPoint, g_ScriptResult->get<nanodbc::string>(17).c_str());
                strcpy(pzPSDetail->sEndPoint, g_ScriptResult->get<nanodbc::string>(18).c_str());
                strcpy(pzPSDetail->sAndOrLogic, g_ScriptResult->get<nanodbc::string>(19).c_str());
                strcpy(pzPSDetail->sIncludeExclude, g_ScriptResult->get<nanodbc::string>(20).c_str());
                strcpy(pzPSDetail->sMaskRest, g_ScriptResult->get<nanodbc::string>(21).c_str());
                strcpy(pzPSDetail->sMatchRest, g_ScriptResult->get<nanodbc::string>(22).c_str());
                strcpy(pzPSDetail->sMaskWild, g_ScriptResult->get<nanodbc::string>(23).c_str());
                strcpy(pzPSDetail->sMatchWild, g_ScriptResult->get<nanodbc::string>(24).c_str());
                strcpy(pzPSDetail->sMaskExpand, g_ScriptResult->get<nanodbc::string>(25).c_str());
                strcpy(pzPSDetail->sMatchExpand, g_ScriptResult->get<nanodbc::string>(26).c_str());
                strcpy(pzPSDetail->sReportDest, g_ScriptResult->get<nanodbc::string>(27).c_str());
                
                pzPSDetail->lStartDate = DateStructToLong(g_ScriptResult->get<nanodbc::date>(28));
                pzPSDetail->lEndDate = DateStructToLong(g_ScriptResult->get<nanodbc::date>(29));
                
                pzPSDetail->lScrhdrNo = pzPSHeader->lScrhdrNo;
            }
            else
            {
                // No detail record
                memset(pzPSDetail, 0, sizeof(*pzPSDetail));
                pzPSDetail->lScrhdrNo = 0; // Legacy code sets this to 0 (or -1 in comment, but 0 in code logic)
            }
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_ScriptResultOpen = false;
            g_ScriptResult.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectAllScriptHeaderAndDetails");
        g_ScriptResultOpen = false;
        g_ScriptResult.reset();
    }
}

DLLAPI void STDCALL SelectAllTemplateHeaderAndDetails(PTMPHDR *pzPTHeader, PTMPDET *pzPTDetail, ERRSTRUCT *pzErr)
{
    try
    {
        *pzErr = {};
        nanodbc::connection* conn = GetDbConnection();
        if (!conn)
        {
            pzErr->iSqlError = -1;
            PrintError("Database connection not available", 0, 0, "E", 0, -1, 0, "SelectAllTemplateHeaderAndDetails", FALSE);
            return;
        }

        if (!g_TemplateResultOpen)
        {
            g_TemplateResult = nanodbc::execute(*conn, R"(
                SELECT th.tmphdr_no, owner, create_date, created_by, changeable, 
                description, change_date, changed_by, 
                seq_no, select_type, comparison_rule, 
                begin_point, end_point, and_or_logic, 
                include_exclude, mask_rest, match_rest, 
                mask_wild, match_wild, mask_expand, match_expand, 
                report_dest, start_date, end_date 
                FROM ptmphdr th LEFT OUTER JOIN ptmpdet td 
                    ON th.tmphdr_no = td.tmphdr_no 
                ORDER BY th.tmphdr_no, seq_no DESC
            )");
            
            g_TemplateResultOpen = true;
        }

        if (g_TemplateResult && g_TemplateResult->next())
        {
            // Populate PTMPHDR
            pzPTHeader->lTmphdrNo = g_TemplateResult->get<long>(0);
            strcpy(pzPTHeader->sOwner, g_TemplateResult->get<nanodbc::string>(1).c_str());
            pzPTHeader->lCreateDate = DateStructToLong(g_TemplateResult->get<nanodbc::date>(2));
            strcpy(pzPTHeader->sCreatedBy, g_TemplateResult->get<nanodbc::string>(3).c_str());
            strcpy(pzPTHeader->sChangeable, g_TemplateResult->get<nanodbc::string>(4).c_str());
            strcpy(pzPTHeader->sDescription, g_TemplateResult->get<nanodbc::string>(5).c_str());
            pzPTHeader->lChangeDate = DateStructToLong(g_TemplateResult->get<nanodbc::date>(6));
            strcpy(pzPTHeader->sChangedBy, g_TemplateResult->get<nanodbc::string>(7).c_str());

            // Populate PTMPDET
            if (!g_TemplateResult->is_null(8)) // seq_no is not null
            {
                pzPTDetail->lSeqNo = g_TemplateResult->get<long>(8);
                strcpy(pzPTDetail->sSelectType, g_TemplateResult->get<nanodbc::string>(9).c_str());
                strcpy(pzPTDetail->sComparisonRule, g_TemplateResult->get<nanodbc::string>(10).c_str());
                strcpy(pzPTDetail->sBeginPoint, g_TemplateResult->get<nanodbc::string>(11).c_str());
                strcpy(pzPTDetail->sEndPoint, g_TemplateResult->get<nanodbc::string>(12).c_str());
                strcpy(pzPTDetail->sAndOrLogic, g_TemplateResult->get<nanodbc::string>(13).c_str());
                strcpy(pzPTDetail->sIncludeExclude, g_TemplateResult->get<nanodbc::string>(14).c_str());
                strcpy(pzPTDetail->sMaskRest, g_TemplateResult->get<nanodbc::string>(15).c_str());
                strcpy(pzPTDetail->sMatchRest, g_TemplateResult->get<nanodbc::string>(16).c_str());
                strcpy(pzPTDetail->sMaskWild, g_TemplateResult->get<nanodbc::string>(17).c_str());
                strcpy(pzPTDetail->sMatchWild, g_TemplateResult->get<nanodbc::string>(18).c_str());
                strcpy(pzPTDetail->sMaskExpand, g_TemplateResult->get<nanodbc::string>(19).c_str());
                strcpy(pzPTDetail->sMatchExpand, g_TemplateResult->get<nanodbc::string>(20).c_str());
                strcpy(pzPTDetail->sReportDest, g_TemplateResult->get<nanodbc::string>(21).c_str());
                
                pzPTDetail->lStartDate = DateStructToLong(g_TemplateResult->get<nanodbc::date>(22));
                pzPTDetail->lEndDate = DateStructToLong(g_TemplateResult->get<nanodbc::date>(23));
                
                pzPTDetail->lTmphdrNo = pzPTHeader->lTmphdrNo;
            }
            else
            {
                // No detail record
                memset(pzPTDetail, 0, sizeof(*pzPTDetail));
                pzPTDetail->lTmphdrNo = 0;
            }
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_TemplateResultOpen = false;
            g_TemplateResult.reset();
        }
    }
    catch (const std::exception& e)
    {
        HandleDbError(pzErr, e.what(), "SelectAllTemplateHeaderAndDetails");
        g_TemplateResultOpen = false;
        g_TemplateResult.reset();
    }
}
