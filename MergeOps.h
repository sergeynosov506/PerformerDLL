/**
 * 
 * SUB-SYSTEM: Composite Merge Operations
 * 
 * FILENAME: MergeOps.h
 * 
 * DESCRIPTION: Operation function prototypes for composite merge operations
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * 
 * PUBLIC FUNCTIONS: Insert, Delete, Update, and Build operations for composite merging
 * 
 * NOTES: All DLLAPI signatures preserved for binary compatibility
 *        GUID parameters converted to const char* (varchar(36) in SQL Server)
 *        
 * USAGE: Part of OLEDB.DLL project
 *
 * AUTHOR: Modernized 2025-11-26
 *
 **/

#ifndef MERGEOPS_H
#define MERGEOPS_H

#include "OLEDBIOCommon.h"
#include "contacts.h"
#include "MapCompMemTransEx.h"

// ============================================================================
// Simple Operations - INSERT/DELETE
// ============================================================================

// Insert contact (with auto-ID generation if not exists)
DLLAPI void STDCALL InsertContacts(CONTACTS *pzContacts, ERRSTRUCT *pzErr);

// Delete composite member transaction mapping by date and ID
DLLAPI void STDCALL DeleteMapCompMemTransEx(long lCompDate, int iCompID, ERRSTRUCT *pzErr);

// Insert composite member transaction mapping
DLLAPI void STDCALL InsertMapCompMemTransEx(MAPCOMPMEMTRANSEX *pzMap, ERRSTRUCT *pzErr);

// Insert merge composite segment map entry
DLLAPI void STDCALL InsertMergeCompSegMap(const char* sSessionID, int iOwnerID, int iID, 
    int iMemberPortID, int iMemberSegID, int iSegmentTypeID, int iParentRuleID,
    int iMemberSegType, int iLevelNumber, const char* sCatValue, 
    double fTaxRate, const char* sName, ERRSTRUCT *pzErr);

// Delete all merge session data by SessionID
DLLAPI void STDCALL DeleteMergeSessionData(const char* sSessionID, ERRSTRUCT *pzErr);

// Update unit value monthly IPV records
DLLAPI void STDCALL UpdateUnitvalueMonthlyIPV(const char* sSessionID, long lDateFrom, 
    long lDateTo, ERRSTRUCT *pzErr);

// Delete merge UV grace period records
DLLAPI void STDCALL DeleteMergeUVGracePeriod(int iPortID, int iOwnerID, 
    const char* sSessionID, ERRSTRUCT *pzErr);

// ============================================================================
// Complex Build Operations - Stored Procedures
// ============================================================================

// Build merge composite segment map (calls stored procedures)
// Returns SessionID in sSessionIDOut (must be 37+ chars: 36 for GUID + null terminator)
DLLAPI void STDCALL BuildMergeCompSegMap(char* sSessionIDOut, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Build merge composite port (calls stored procedures with existing SessionID)
DLLAPI void STDCALL BuildMergeCompport(const char* sSessionID, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Build merge unit values
DLLAPI void STDCALL BuildMergeUV(const char* sSessionID, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Get summarized data for composite (complex query with calculations)
DLLAPI void STDCALL GetSummarizedDataForCompositeEx(const char* sSessionID, int iOwnerID, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Build merge summary data
DLLAPI void STDCALL BuildMergeSData(const char* sSessionID, int iOwnerID, 
    long lDateFrom, ERRSTRUCT *pzErr);

// Build/update merge summary data
DLLAPI void STDCALL BuildUpdateMergeSData(const char* sSessionID, long lDateFrom, 
    long lDateTo, ERRSTRUCT *pzErr);

// Copy summary data from one table to another
DLLAPI void STDCALL CopySummaryData(const char* sDestTable, const char* sSrcTable, 
    int iPortID, long lPerformDate, ERRSTRUCT *pzErr);

// ============================================================================
// Summarization Operations
// ============================================================================

// Summarize tax performance data
DLLAPI void STDCALL SummarizeTaxPerf(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Summarize monthly summary data
DLLAPI void STDCALL SummarizeMonthsum(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Summarize inception summary data
DLLAPI void STDCALL SummarizeInceptionSummdata(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

// Subtract inception summary data
DLLAPI void STDCALL SubtractInceptionSummdata(const char* sSessionID, long lPerformDate, 
    long lDateFrom, long lDateTo, ERRSTRUCT *pzErr);

#endif // MERGEOPS_H
