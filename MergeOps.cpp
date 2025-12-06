/**
 * 
 * SUB-SYSTEM: Composite Merge Operations
 * 
 * FILENAME: MergeOps.cpp
 * 
 * DESCRIPTION: Operation functions for composite merge operations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * NOTES: GUID parameters converted to const char* (varchar(36))
 *        All operations use nanodbc::execute for non-query operations
 *        Stored procedures called via nanodbc::statement
 *        
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#include "MergeOps.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include "MergeQueries.h"
#include <string>
#include <cstring>

extern thread_local nanodbc::connection gConn;

// ============================================================================
// Helper: Convert GUID string to proper format if needed
// SQL Server GUIDs are stored as varchar(36) in format:
// XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
// ============================================================================

static std::string GuidToString(const char* sGuid)
{
    // If already a valid GUID string, return as-is
    // Otherwise, could add validation/formatting here
    return std::string(sGuid);
}

// ============================================================================
// InsertContacts
// ============================================================================

DLLAPI void STDCALL InsertContacts(CONTACTS *pzContacts, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertContacts", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertContacts", FALSE);
        return;
    }

    try
    {
        // First, try to SELECT existing contact
        nanodbc::statement stmtSelect(gConn);
        nanodbc::prepare(stmtSelect, NANODBC_TEXT(SQL_InsertContacts));
        
        stmtSelect.bind(0, pzContacts->sUniqueName);
        stmtSelect.bind(1, pzContacts->sContactType);
        
        nanodbc::result result = nanodbc::execute(stmtSelect);
        
        if (result.next())
        {
            // Contact exists, return its ID
            pzContacts->lID = result.get<long>("id", 0);
        }
        else
        {
            // Contact doesn't exist, insert it
            // Note: Original code would generate ID, assuming database has auto-increment
            // If ID needs to be generated manually, add that logic here
            nanodbc::statement stmtInsert(gConn);
            nanodbc::prepare(stmtInsert, NANODBC_TEXT(
                "INSERT INTO CONTACTS ("
                "contacttype, uniquename, abbrev, description, "
                "address1, address2, address3, city_id, state_id, zip, country_id, "
                "phone1, phone2, fax1, fax2, webaddress, domicile_id"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
            
            int idx = 0;
            stmtInsert.bind(idx++, pzContacts->sContactType);
            stmtInsert.bind(idx++, pzContacts->sUniqueName);
            stmtInsert.bind(idx++, pzContacts->sAbbrev);
            stmtInsert.bind(idx++, pzContacts->sDescription);
            stmtInsert.bind(idx++, pzContacts->sAddress1);
            stmtInsert.bind(idx++, pzContacts->sAddress2);
            stmtInsert.bind(idx++, pzContacts->sAddress3);
            stmtInsert.bind(idx++, &pzContacts->lCityID);
            stmtInsert.bind(idx++, &pzContacts->lStateID);
            stmtInsert.bind(idx++, pzContacts->sZip);
            stmtInsert.bind(idx++, &pzContacts->lCountryID);
            stmtInsert.bind(idx++, pzContacts->sPhone1);
            stmtInsert.bind(idx++, pzContacts->sPhone2);
            stmtInsert.bind(idx++, pzContacts->sFax1);
            stmtInsert.bind(idx++, pzContacts->sFax2);
            stmtInsert.bind(idx++, pzContacts->sWebAddress);
            stmtInsert.bind(idx++, &pzContacts->lDomicileID);
            
            nanodbc::execute(stmtInsert);
            
            // Get generated ID if using IDENTITY column
            nanodbc::result resultID = nanodbc::execute(gConn, NANODBC_TEXT("SELECT @@IDENTITY AS newID"));
            if (resultID.next())
            {
                pzContacts->lID = resultID.get<long>("newID", 0);
            }
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertContacts: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertContacts", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertContacts", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertContacts", FALSE);
    }
}

// ============================================================================
// DeleteMapCompMemTransEx
// ============================================================================

DLLAPI void STDCALL DeleteMapCompMemTransEx(long lCompDate, int iCompID, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "DeleteMapCompMemTransEx", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMapCompMemTransEx", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_DeleteCompMemTransEx));
        
        nanodbc::timestamp tsCompDate;
        long_to_timestamp(lCompDate, tsCompDate);
        
        stmt.bind(0, &tsCompDate);
        stmt.bind(1, &iCompID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteMapCompMemTransEx: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMapCompMemTransEx", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteMapCompMemTransEx", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMapCompMemTransEx", FALSE);
    }
}

// ============================================================================
// InsertMapCompMemTransEx
// ============================================================================

DLLAPI void STDCALL InsertMapCompMemTransEx(MAPCOMPMEMTRANSEX *pzMap, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertMapCompMemTransEx", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMapCompMemTransEx", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_InsertMapCompMemTransEx));
        
        nanodbc::timestamp tsCompDate;
        long_to_timestamp(pzMap->lCompDate, tsCompDate);
        
        stmt.bind(0, &tsCompDate);
        stmt.bind(1, &pzMap->iCompID);
        stmt.bind(2, &pzMap->lCompTrans);
        stmt.bind(3, &pzMap->iCompMem);
        stmt.bind(4, &pzMap->lCompMemTrans);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertMapCompMemTransEx: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMapCompMemTransEx", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertMapCompMemTransEx", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMapCompMemTransEx", FALSE);
    }
}

// ============================================================================
// InsertMergeCompSegMap
// ============================================================================

DLLAPI void STDCALL InsertMergeCompSegMap(const char* sSessionID, int iOwnerID, int iID, 
    int iMemberPortID, int iMemberSegID, int iSegmentTypeID, int iParentRuleID,
    int iMemberSegType, int iLevelNumber, const char* sCatValue, 
    double fTaxRate, const char* sName, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertMergeCompSegMap", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMergeCompSegMap", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_InsertMergeCompSegMap));
        
        int idx = 0;
        stmt.bind(idx++, sSessionID);
        stmt.bind(idx++, &iOwnerID);
        stmt.bind(idx++, &iID);
        stmt.bind(idx++, &iMemberPortID);
        stmt.bind(idx++, &iMemberSegID);
        stmt.bind(idx++, &iSegmentTypeID);
        stmt.bind(idx++, &iParentRuleID);
        stmt.bind(idx++, &iMemberSegType);
        stmt.bind(idx++, &iLevelNumber);
        stmt.bind(idx++, sCatValue);
        stmt.bind(idx++, &fTaxRate);
        stmt.bind(idx++, sName);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in InsertMergeCompSegMap: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMergeCompSegMap", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in InsertMergeCompSegMap", 0, 0, (char*)"", 0, -1, 0, (char*)"InsertMergeCompSegMap", FALSE);
    }
}

// ============================================================================
// DeleteMergeSessionData
// ============================================================================

DLLAPI void STDCALL DeleteMergeSessionData(const char* sSessionID, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "DeleteMergeSessionData", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMergeSessionData", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_DeleteMergeSessionData));
        
        // SQL has 4 identical SessionID parameters (one for each table)
        stmt.bind(0, sSessionID);
        stmt.bind(1, sSessionID);
        stmt.bind(2, sSessionID);
        stmt.bind(3, sSessionID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteMergeSessionData: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMergeSessionData", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteMergeSessionData", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMergeSessionData", FALSE);
    }
}

// ============================================================================
// UpdateUnitvalueMonthlyIPV
// ============================================================================

DLLAPI void STDCALL UpdateUnitvalueMonthlyIPV(const char* sSessionID, long lDateFrom, 
    long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "UpdateUnitvalueMonthlyIPV", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateUnitvalueMonthlyIPV", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_UpdateUnitvalueMonthlyIPV));
        
        nanodbc::timestamp tsDateFrom, tsDateTo;
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, sSessionID);
        stmt.bind(1, &tsDateFrom);
        stmt.bind(2, &tsDateTo);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdateUnitvalueMonthlyIPV: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateUnitvalueMonthlyIPV", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdateUnitvalueMonthlyIPV", 0, 0, (char*)"", 0, -1, 0, (char*)"UpdateUnitvalueMonthlyIPV", FALSE);
    }
}

// ============================================================================
// DeleteMergeUVGracePeriod
// ============================================================================

DLLAPI void STDCALL DeleteMergeUVGracePeriod(int iPortID, int iOwnerID, 
    const char* sSessionID, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "DeleteMergeUVGracePeriod", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMergeUVGracePeriod", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(SQL_DeleteMergeUVGracePeriod));
        
        stmt.bind(0, &iPortID);
        stmt.bind(1, &iOwnerID);
        stmt.bind(2, sSessionID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in DeleteMergeUVGracePeriod: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMergeUVGracePeriod", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in DeleteMergeUVGracePeriod", 0, 0, (char*)"", 0, -1, 0, (char*)"DeleteMergeUVGracePeriod", FALSE);
    }
}

// ============================================================================
// Complex Build Operations - These call stored procedures and build SQL
// ============================================================================

// BuildMergeCompSegMap
DLLAPI void STDCALL BuildMergeCompSegMap(char* sSessionIDOut, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "BuildMergeCompSegMap", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeCompSegMap", FALSE);
        return;
    }

    try
    {
        // Build SQL using helper from MergeQueries
        char sSQL[MAXSQLSIZE];
        BuildMergeCompSegMap_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsDateFrom, tsDateTo;
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        // Binding pattern: OwnerID, DateFrom, DateTo for stored proc calls
        stmt.bind(0, &iOwnerID);
        stmt.bind(1, &tsDateFrom);
        stmt.bind(2, &tsDateTo);
        stmt.bind(3, &iOwnerID);
        stmt.bind(4, &tsDateFrom);
        stmt.bind(5, &tsDateTo);
        
        nanodbc::result result = nanodbc::execute(stmt);
        
        // First result should contain the generated SessionID
        if (result.next())
        {
            std::string sessionID = result.get<std::string>(0, "");
            strcpy_s(sSessionIDOut, 37, sessionID.c_str());
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in BuildMergeCompSegMap: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeCompSegMap", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in BuildMergeCompSegMap", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeCompSegMap", FALSE);
    }
}

// Build MergeCompport
DLLAPI void STDCALL BuildMergeCompport(const char* sSessionID, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "BuildMergeCompport", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeCompport", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildMergeCompport_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsDateFrom, tsDateTo;
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, sSessionID);
        stmt.bind(1, &iOwnerID);
        stmt.bind(2, &tsDateFrom);
        stmt.bind(3, &tsDateTo);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in BuildMergeCompport: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeCompport", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in BuildMergeCompport", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeCompport", FALSE);
    }
}

// BuildMergeUV
DLLAPI void STDCALL BuildMergeUV(const char* sSessionID, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "BuildMergeUV", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeUV", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildMergeUV_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsDateFrom, tsDateTo;
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, sSessionID);
        stmt.bind(1, sSessionID);
        stmt.bind(2, &iOwnerID);
        stmt.bind(3, &tsDateFrom);
        stmt.bind(4, &tsDateTo);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in BuildMergeUV: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeUV", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in BuildMergeUV", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeUV", FALSE);
    }
}

// GetSummarizedDataForCompositeEx
DLLAPI void STDCALL GetSummarizedDataForCompositeEx(const char* sSessionID, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "GetSummarizedDataForCompositeEx", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"GetSummarizedDataForCompositeEx", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildSQLForCompositeEx(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsDateFrom, tsDateTo;
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        // Complex query has many parameters - bind all required ones
        // This varies based on the SQL structure from BuildSQLForCompositeEx
        // For now, bind common parameters
        stmt.bind(0, sSessionID);
        stmt.bind(1, &iOwnerID);
        stmt.bind(2, &tsDateFrom);
        stmt.bind(3, &tsDateTo);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in GetSummarizedDataForCompositeEx: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"GetSummarizedDataForCompositeEx", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in GetSummarizedDataForCompositeEx", 0, 0, (char*)"", 0, -1, 0, (char*)"GetSummarizedDataForCompositeEx", FALSE);
    }
}

// BuildMergeSData
DLLAPI void STDCALL BuildMergeSData(const char* sSessionID, int iOwnerID, 
    long lDateFrom, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "BuildMergeSData", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeSData", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildMergeSData_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsDateFrom;
        long_to_timestamp(lDateFrom, tsDateFrom);
        
        stmt.bind(0, sSessionID);
        stmt.bind(1, sSessionID);
        stmt.bind(2, &iOwnerID);
        stmt.bind(3, &tsDateFrom);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in BuildMergeSData: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeSData", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in BuildMergeSData", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildMergeSData", FALSE);
    }
}

// BuildUpdateMergeSData
DLLAPI void STDCALL BuildUpdateMergeSData(const char* sSessionID, long lDateFrom, 
    long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "BuildUpdateMergeSData", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildUpdateMergeSData", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildUpdateMergeSData_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsDateFrom, tsDateTo;
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, sSessionID);
        stmt.bind(1, &tsDateFrom);
        stmt.bind(2, &tsDateTo);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in BuildUpdateMergeSData: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"BuildUpdateMergeSData", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in BuildUpdateMergeSData", 0, 0, (char*)"", 0, -1, 0, (char*)"BuildUpdateMergeSData", FALSE);
    }
}

// CopySummaryData
DLLAPI void STDCALL CopySummaryData(const char* sDestTable, const char* sSrcTable, 
    int iPortID, long lPerformDate, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "CopySummaryData", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"CopySummaryData", FALSE);
        return;
    }

    try
    {
        // Build SQL with table name replacements
        char sSQL[MAXSQLSIZE];
        strcpy_s(sSQL, MAXSQLSIZE, SQL_CopySummaryData);
        
        // Replace %DEST_TABLE_NAME% and %SRC_TABLE_NAME% placeholders
        std::string sql(sSQL);
        size_t pos;
        while ((pos = sql.find("%DEST_TABLE_NAME%")) != std::string::npos)
            sql.replace(pos, 17, sDestTable);
        while ((pos = sql.find("%SRC_TABLE_NAME%")) != std::string::npos)
            sql.replace(pos, 16, sSrcTable);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, sql);
        
        nanodbc::timestamp tsPerformDate;
        long_to_timestamp(lPerformDate, tsPerformDate);
        
        stmt.bind(0, &iPortID);
        stmt.bind(1, &tsPerformDate);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in CopySummaryData: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"CopySummaryData", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in CopySummaryData", 0, 0, (char*)"", 0, -1, 0, (char*)"CopySummaryData", FALSE);
    }
}

// ============================================================================
// Summarization Operations
// ============================================================================

// SummarizeTaxPerf
DLLAPI void STDCALL SummarizeTaxPerf(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SummarizeTaxPerf", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeTaxPerf", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildSummarizeTaxPerf_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsPerformDate, tsDateFrom, tsDateTo;
        long_to_timestamp(lPerformDate, tsPerformDate);
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, &tsPerformDate);
        stmt.bind(1, sSessionID);
        stmt.bind(2, &tsDateFrom);
        stmt.bind(3, &tsDateTo);
        stmt.bind(4, sSessionID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SummarizeTaxPerf: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeTaxPerf", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SummarizeTaxPerf", 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeTaxPerf", FALSE);
    }
}

// SummarizeMonthsum
DLLAPI void STDCALL SummarizeMonthsum(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SummarizeMonthsum", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeMonthsum", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildSummarizeMonthsum_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsPerformDate, tsDateFrom, tsDateTo;
        long_to_timestamp(lPerformDate, tsPerformDate);
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, &tsPerformDate);
        stmt.bind(1, &tsPerformDate);
        stmt.bind(2, sSessionID);
        stmt.bind(3, &tsDateFrom);
        stmt.bind(4, &tsDateTo);
        stmt.bind(5, sSessionID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SummarizeMonthsum: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeMonthsum", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SummarizeMonthsum", 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeMonthsum", FALSE);
    }
}

// SummarizeInceptionSummdata
DLLAPI void STDCALL SummarizeInceptionSummdata(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SummarizeInceptionSummdata", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeInceptionSummdata", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildSummarizeInceptionSummdata_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsPerformDate, tsDateFrom, tsDateTo;
        long_to_timestamp(lPerformDate, tsPerformDate);
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, &tsPerformDate);
        stmt.bind(1, &tsPerformDate);
        stmt.bind(2, sSessionID);
        stmt.bind(3, &tsDateFrom);
        stmt.bind(4, &tsDateTo);
        stmt.bind(5, sSessionID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SummarizeInceptionSummdata: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeInceptionSummdata", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SummarizeInceptionSummdata", 0, 0, (char*)"", 0, -1, 0, (char*)"SummarizeInceptionSummdata", FALSE);
    }
}

// SubtractInceptionSummdata
DLLAPI void STDCALL SubtractInceptionSummdata(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr)
{
#ifdef DEBUG
    PrintError("Entering", 0, 0, "", 0, 0, 0, "SubtractInceptionSummdata", FALSE);
#endif

    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SubtractInceptionSummdata", FALSE);
        return;
    }

    try
    {
        char sSQL[MAXSQLSIZE];
        BuildSubtractInceptionSummdata_SQL(sSQL);
        
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(sSQL));
        
        nanodbc::timestamp tsPerformDate, tsDateFrom, tsDateTo;
        long_to_timestamp(lPerformDate, tsPerformDate);
        long_to_timestamp(lDateFrom, tsDateFrom);
        long_to_timestamp(lDateTo, tsDateTo);
        
        stmt.bind(0, &tsPerformDate);
        stmt.bind(1, &tsPerformDate);
        stmt.bind(2, sSessionID);
        stmt.bind(3, &tsDateFrom);
        stmt.bind(4, &tsDateTo);
        stmt.bind(5, sSessionID);
        
        nanodbc::execute(stmt);
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SubtractInceptionSummdata: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), 0, 0, (char*)"", 0, -1, 0, (char*)"SubtractInceptionSummdata", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SubtractInceptionSummdata", 0, 0, (char*)"", 0, -1, 0, (char*)"SubtractInceptionSummdata", FALSE);
    }
}
