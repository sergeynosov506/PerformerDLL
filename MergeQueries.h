/**
 * 
 * SUB-SYSTEM: Composite Merge Queries
 * 
 * FILENAME: MergeQueries.h
 * 
 * DESCRIPTION: SQL query string definitions and SQL builder functions
 *              Compatible with both legacy ATL OLE DB and modern nanodbc
 * 
 * NOTES: All SQL strings use ? parameter placeholders (ODBC/nanodbc compatible)
 *        SQL builder functions create complex multi-statement SQL batches
 *        No modernization required - SQL is already database-agnostic
 *        
 * USAGE: Part of OLEDB.DLL project
 *        Used by MergeSelectors.cpp and MergeOps.cpp
 *
 * AUTHOR: Original SQL definitions maintained; Documentation updated 2025-11-26
 *
 **/
#ifndef MERGEQUERIES_H
#define MERGEQUERIES_H

#pragma once
#include "OLEDBIOCommon.h"

// ============================================================================
// Simple Query Strings (nanodbc-compatible with ? placeholders)
// ============================================================================

// SELECT query for composite member IDs
extern const char* SQL_SelectAllMembersOfAComposite;

// SELECT query for portfolio segments
extern const char* SQL_SelectSegmainForPortfolio;

// SELECT query for transactions (102 columns, complex JOIN)
extern const char* SQL_SelectTransFor;

// SELECT query for contact lookup (used by InsertContacts)
extern const char* SQL_InsertContacts;

// DELETE query for composite member transaction mapping
extern const char* SQL_DeleteCompMemTransEx;

// INSERT query for composite member transaction mapping
extern const char* SQL_InsertMapCompMemTransEx;

// INSERT query with table name placeholders (requires runtime substitution)
extern const char* SQL_CopySummaryData;

// Multi-statement DELETE for merge session cleanup
extern const char* SQL_DeleteMergeSessionData;

// UPDATE query for unit value monthly IPV records
extern const char* SQL_UpdateUnitvalueMonthlyIPV;

// Complex conditional UPDATE for grace period handling
extern const char* SQL_DeleteMergeUVGracePeriod;

// INSERT query for merge composite segment map
extern const char* SQL_InsertMergeCompSegMap;

// ============================================================================
// Complex Query Builders
// These functions build multi-statement SQL batches by concatenating
// predefined SQL fragments. All use ? placeholders for nanodbc compatibility.
// ============================================================================

// Build SQL for merge composite segment map (calls stored procedures)
// Generates new SessionID via newid() and returns it
void BuildMergeCompSegMap_SQL(char *sSegMapSQL);

// Build SQL for merge composite port (uses existing SessionID)
void BuildMergeCompport_SQL(char *sCompportSQL);

// Build SQL for merge unit values
void BuildMergeUV_SQL(char *sUVSQL);

// Build extremely complex SQL for composite data summarization
// Contains 20+ SQL fragments with calculations and aggregations
void BuildSQLForCompositeEx(char* sAdjSQL);

// Build SQL for merge summary data creation
void BuildMergeSData_SQL(char *sSQL);

// Build SQL for merge summary data updates
void BuildUpdateMergeSData_SQL(char *sSQL);

// Build SQL for tax performance summarization
void BuildSummarizeTaxPerf_SQL(char *sSQL);

// Build SQL for monthly summary data
void BuildSummarizeMonthsum_SQL(char *sSQL);

// Build SQL for inception summary data
void BuildSummarizeInceptionSummdata_SQL(char *sSQL);

// Build SQL for inception summary data subtraction
void BuildSubtractInceptionSummdata_SQL(char *sSQL);

#endif // MERGEQUER IES_H
