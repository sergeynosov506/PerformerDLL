/**
 * SUB-SYSTEM: Database Input/Output for Valuation
 * FILENAME: ValuationIO_Segments.cpp
 * DESCRIPTION: Segment hierarchy and mapping implementations (11 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * NOTES: Complex module with boolean checks, multi-row iterators, and ID lookups
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "ValuationIO_Segments.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "segmain.h"
#include "portmain.h"
#include <optional>
#include <cstring>
#include <string>

extern thread_local nanodbc::connection gConn;
extern char sPortmain[STR80LEN];

// Static state for multi-row cursors
struct SegmentsState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

struct SegmapState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

struct SegmentState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

static SegmentsState g_segmentsState;
static SegmapState g_segmapState;
static SegmentState g_segmentState;

// ===== MULTI-ROW ITERATORS =====

DLLAPI void STDCALL SelectAllSegments(SEGMENTS *pzSegments, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSegments", FALSE);
        return;
    }

    try {
        if (g_segmentsState.cRows == 0) {
            g_segmentsState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT("SELECT id, level_id, abbrev, name, code, sequencenbr, groupid, international FROM segments"));
            g_segmentsState.result = nanodbc::execute(stmt);
        }

        if (g_segmentsState.result && g_segmentsState.result->next()) {
            g_segmentsState.cRows++;
            memset(pzSegments, 0, sizeof(*pzSegments));
            pzSegments->iID = g_segmentsState.result->get<int>("id", 0);
            pzSegments->iLevelID = g_segmentsState.result->get<int>("level_id", 0);
            read_string(*g_segmentsState.result, "abbrev", pzSegments->sAbbrev, sizeof(pzSegments->sAbbrev));
            read_string(*g_segmentsState.result, "name", pzSegments->sName, sizeof(pzSegments->sName));
            read_string(*g_segmentsState.result, "code", pzSegments->sCode, sizeof(pzSegments->sCode));
            pzSegments->iSequenceNbr = g_segmentsState.result->get<int>("sequencenbr", 0);
            pzSegments->iGroupID = g_segmentsState.result->get<int>("groupid", 0);
            read_int(*g_segmentsState.result, "international", &pzSegments->iInternational);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
            g_segmentsState.cRows = 0;
            g_segmentsState.result.reset();
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSegments", FALSE);
        g_segmentsState.result.reset();
    }
}

DLLAPI void STDCALL SelectAllSegmap(SEGMAP *pzSegmap, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSegmap", FALSE);
        return;
    }

    try {
        if (g_segmapState.cRows == 0) {
            g_segmapState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT("SELECT SegmentID, SegmentLevel1ID, SegmentLevel2ID, SegmentLevel3ID FROM segmap"));
            g_segmapState.result = nanodbc::execute(stmt);
        }

        if (g_segmapState.result && g_segmapState.result->next()) {
            g_segmapState.cRows++;
            memset(pzSegmap, 0, sizeof(*pzSegmap));
            pzSegmap->iSegmentID = g_segmapState.result->get<int>("SegmentID", 0);
            pzSegmap->iSegmentLevel1ID = g_segmapState.result->get<int>("SegmentLevel1ID", 0);
            pzSegmap->iSegmentLevel2ID = g_segmapState.result->get<int>("SegmentLevel2ID", 0);
            pzSegmap->iSegmentLevel3ID = g_segmapState.result->get<int>("SegmentLevel3ID", 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
            g_segmapState.cRows = 0;
            g_segmapState.result.reset();
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllSegmap", FALSE);
        g_segmapState.result.reset();
    }
}

DLLAPI void STDCALL SelectSegment(SEGMENTS *pzSegment, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegment", FALSE);
        return;
    }

    try {
        if (g_segmentState.cRows == 0) {
            g_segmentState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT("SELECT id, level_id, abbrev, name FROM segments"));
            g_segmentState.result = nanodbc::execute(stmt);
        }

        if (g_segmentState.result && g_segmentState.result->next()) {
            g_segmentState.cRows++;
            memset(pzSegment, 0, sizeof(*pzSegment));
            pzSegment->iID = g_segmentState.result->get<int>("id", 0);
            pzSegment->iLevelID = g_segmentState.result->get<int>("level_id", 0);
            read_string(*g_segmentState.result, "abbrev", pzSegment->sAbbrev, sizeof(pzSegment->sAbbrev));
            read_string(*g_segmentState.result, "name", pzSegment->sName, sizeof(pzSegment->sName));
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
            g_segmentState.cRows = 0;
            g_segmentState.result.reset();
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegment", FALSE);
        g_segmentState.result.reset();
    }
}

// ===== BOOLEAN / INTERNATIONAL CHECKS =====

DLLAPI BOOL STDCALL IsAssetInternational(int iID, char *sSecNo, char *sWi, int iLevel1, int *piLevel2, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    BOOL bRes = FALSE;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"IsAssetInternational", FALSE);
        return FALSE;
    }

    try {
        // Step 1: Check currency match
        nanodbc::statement stmt1(gConn);
        nanodbc::prepare(stmt1, NANODBC_TEXT(
            "SELECT 'P' as Curr, id, basecurrid, '' as inc_curr_id, '' as Primary_type, '' as Secondary_Type "
            "FROM portmain WHERE id = ? "
            "UNION "
            "SELECT 'A' as Curr, 0 as id, curr_id as basecurrid, inc_curr_id, b.Primary_type, b.Secondary_Type "
            "FROM assets a, sectype b "
            "WHERE sec_no = ? AND when_issue = ? AND a.sec_type = b.sec_type ORDER BY ID DESC"));
        
        stmt1.bind(0, &iID);
        stmt1.bind(1, sSecNo);
        stmt1.bind(2, sWi);
        
        auto result1 = nanodbc::execute(stmt1);
        
        char sPortMainCurrID[5] = {0}, sAssetCurrID[5] = {0};
        while (result1.next()) {
            char sCurr[2] = {0};
            read_string(result1, "Curr", sCurr, sizeof(sCurr));
            char sBaseCurrId[5] = {0};
            read_string(result1, "basecurrid", sBaseCurrId, sizeof(sBaseCurrId));
            
            if (sCurr[0] == 'P')
                strcpy_s(sPortMainCurrID, sBaseCurrId);
            else
                strcpy_s(sAssetCurrID, sBaseCurrId);
        }
        
        if (_stricmp(sPortMainCurrID, sAssetCurrID) == 0)
            bRes = TRUE;
        
        // Step 2: If currencies match, check international segment
        if (bRes) {
            bRes = FALSE;
            nanodbc::statement stmt2(gConn);
            nanodbc::prepare(stmt2, NANODBC_TEXT("SELECT international, level_id FROM segments WHERE id = ?"));
            stmt2.bind(0, &iLevel1);
            
            auto result2 = nanodbc::execute(stmt2);
            if (result2.next()) {
                int iInternational = result2.get<int>("international", 0);
                if (iInternational != 0) {
                    *piLevel2 = iInternational;
                    bRes = TRUE;
                }
            }
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"IsAssetInternational", FALSE);
    }
    
    return bRes;
}

// ===== ID LOOKUP FUNCTIONS (return int) =====

DLLAPI int STDCALL GetInterSegID(int iSegTypeID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    int iRes = 0;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"GetInterSegID", FALSE);
        return 0;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT international FROM segments WHERE id = ?"));
        stmt.bind(0, &iSegTypeID);
        
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            iRes = result.get<int>("international", 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"GetInterSegID", FALSE);
    }
    
    return iRes;
}

DLLAPI int STDCALL SelectSegmentIdFromSegMap(int IndustLevel1, int IndustLevel2, int IndustLevel3, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    int iRes = 0;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmentIdFromSegMap", FALSE);
        return 0;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT SegmentID FROM segmap "
            "WHERE SegmentLevel1ID = ? AND SegmentLevel2ID = ? AND SegmentLevel3ID = ?"));
        
        stmt.bind(0, &IndustLevel1);
        stmt.bind(1, &IndustLevel2);
        stmt.bind(2, &IndustLevel3);
        
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            iRes = result.get<int>("SegmentID", 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmentIdFromSegMap", FALSE);
    }
    
    return iRes;
}

DLLAPI int STDCALL SelectSegmentLevelId(int SegmentTypeId, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    int iRes = 0;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmentLevelId", FALSE);
        return 0;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT Level_id FROM segments WHERE id = ?"));
        stmt.bind(0, &SegmentTypeId);
        
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            iRes = result.get<int>("Level_id", 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmentLevelId", FALSE);
    }
    
    return iRes;
}

DLLAPI int STDCALL SelectUnsupervisedSegmentId(int iLevelId, int iSqnNbr, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    int iRes = 0;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnsupervisedSegmentId", FALSE);
        return 0;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT id FROM segments WHERE level_id = ? AND sequencenbr = ?"));
        
        stmt.bind(0, &iLevelId);
        stmt.bind(1, &iSqnNbr);
        
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            iRes = result.get<int>("id", 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnsupervisedSegmentId", FALSE);
    }
    
    return iRes;
}

DLLAPI int STDCALL SelectPledgedSegment(int Level1, int Level3, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    int iRes = 0;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPledgedSegment", FALSE);
        return 0;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT SegmentID FROM segmap "
            "WHERE SegmentLevel1ID = ? AND SegmentLevel3ID = ?"));
        
        stmt.bind(0, &Level1);
        stmt.bind(1, &Level3);
        
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            iRes = result.get<int>("SegmentID", 0);
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectPledgedSegment", FALSE);
    }
    
    return iRes;
}

// ===== AGGREGATE / SPECIAL QUERIES =====

DLLAPI void STDCALL SelectUnsupervised(int iPortfolioId, char *sSecXtend, double *pfTotalMktValue, double *pfTotalCost, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);
    *pfTotalMktValue = 0.0;
    *pfTotalCost = 0.0;
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnsupervised", FALSE);
        return;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT 'H' TableName, SUM(mkt_val) AS MarketValue, SUM(tot_cost) AS TotalCost "
            "FROM holdings WHERE id = ? AND sec_xtend = ? "
            "UNION ALL "
            "SELECT 'C' TableName, SUM(mkt_val) AS MarketValue, SUM(tot_cost) AS TotalCost "
            "FROM holdcash WHERE id = ? AND sec_xtend = ?"));
        
        stmt.bind(0, &iPortfolioId);
        stmt.bind(1, sSecXtend);
        stmt.bind(2, &iPortfolioId);
        stmt.bind(3, sSecXtend);
        
        auto result = nanodbc::execute(stmt);
        while (result.next()) {
            if (!result.is_null("MarketValue"))
                *pfTotalMktValue += result.get<double>("MarketValue", 0.0);
            if (!result.is_null("TotalCost"))
                *pfTotalCost += result.get<double>("TotalCost", 0.0);
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectUnsupervised", FALSE);
    }
}

DLLAPI void STDCALL SelectAdhocPortmain(PORTMAIN *pzPR, int iID, ERRSTRUCT *pzErr)
{
    // Note: This is typically same as SelectPortmain but from 'pmadhoc' table
    // For simplicity, using standard portmain query pattern
    InitializeErrStruct(pzErr);
    
    if (!gConn.connected()) {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAdhocPortmain", FALSE);
        return;
    }

    try {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT abbrev, basecurrid FROM pmadhoc WHERE id = ?"));
        stmt.bind(0, &iID);
        
        auto result = nanodbc::execute(stmt);
        if (result.next()) {
            memset(pzPR, 0, sizeof(*pzPR));
            pzPR->iID = iID;
            read_string(result, "abbrev", pzPR->sAbbrev, sizeof(pzPR->sAbbrev));
            read_string(result, "basecurrid", pzPR->sBaseCurrId, sizeof(pzPR->sBaseCurrId));
        } else {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    } catch (const nanodbc::database_error& e) {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAdhocPortmain", FALSE);
    }
}
