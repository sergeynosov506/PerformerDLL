/**
 * SUB-SYSTEM: Database Input/Output for Performance
 * FILENAME: PerformanceIO_Lookups.cpp
 * DESCRIPTION: Lookup and reference table implementations (5 functions)
 *              Modernized to C++20 + nanodbc from legacy ATL OLE DB Templates
 * AUTHOR: Modernized 2025-11-30
 */

#include "commonheader.h"
#include "PerformanceIO_Lookups.h"
#include "OLEDBIOCommon.h"
#include "nanodbc/nanodbc.h"
#include "currency.h"
#include "sectype.h"
#include "segmain.h"
#include <optional>
#include <cstring>
#include <string>
#include "dateutils.h"
 // Thread-local database connection
extern thread_local nanodbc::connection gConn;

// Static state for multi-row cursors
struct PartCurrenciesState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

struct CountriesState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

struct PartSectypeState {
    std::optional<nanodbc::result> result;
    int cRows = 0;
};

static PartCurrenciesState g_partCurrenciesState;
static CountriesState g_countriesState;
static PartSectypeState g_partSectypeState;

// ============================================================================
// SelectAllPartCurrencies
// ============================================================================
DLLAPI void STDCALL SelectAllPartCurrencies(PARTCURR *pzCurrency, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPartCurrencies", FALSE);
        return;
    }

    try
    {
        if (g_partCurrenciesState.cRows == 0)
        {
            g_partCurrenciesState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT("SELECT curr_id, sec_no, wi FROM currency ORDER BY curr_id"));
            g_partCurrenciesState.result = nanodbc::execute(stmt);
        }

        if (g_partCurrenciesState.result && g_partCurrenciesState.result->next())
        {
            g_partCurrenciesState.cRows++;
            memset(pzCurrency, 0, sizeof(*pzCurrency));
            
            read_string(*g_partCurrenciesState.result, "curr_id", pzCurrency->sCurrId, sizeof(pzCurrency->sCurrId));
            read_string(*g_partCurrenciesState.result, "sec_no", pzCurrency->sSecNo, sizeof(pzCurrency->sSecNo));
            read_string(*g_partCurrenciesState.result, "wi", pzCurrency->sWi, sizeof(pzCurrency->sWi));
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_partCurrenciesState.cRows = 0;
            g_partCurrenciesState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPartCurrencies", FALSE);
        g_partCurrenciesState.result.reset();
    }
}

// ============================================================================
// SelectAllCountries
// ============================================================================
DLLAPI void STDCALL SelectAllCountries(COUNTRY *pzCountry, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllCountries", FALSE);
        return;
    }

    try
    {
        if (g_countriesState.cRows == 0)
        {
            g_countriesState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT("SELECT id, code, description FROM country ORDER BY code"));
            g_countriesState.result = nanodbc::execute(stmt);
        }

        if (g_countriesState.result && g_countriesState.result->next())
        {
            g_countriesState.cRows++;
            memset(pzCountry, 0, sizeof(*pzCountry));
            
            pzCountry->iId = g_countriesState.result->get<int>("id", 0);
            read_string(*g_countriesState.result, "code", pzCountry->sCode, sizeof(pzCountry->sCode));
            read_string(*g_countriesState.result, "description", pzCountry->sDescription, sizeof(pzCountry->sDescription));
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_countriesState.cRows = 0;
            g_countriesState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllCountries", FALSE);
        g_countriesState.result.reset();
    }
}

// ============================================================================
// SelectAllPartSectype
// ============================================================================
DLLAPI void STDCALL SelectAllPartSectype(PARTSTYPE *pzST, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPartSectype", FALSE);
        return;
    }

    try
    {
        if (g_partSectypeState.cRows == 0)
        {
            g_partSectypeState.result.reset();
            nanodbc::statement stmt(gConn);
            nanodbc::prepare(stmt, NANODBC_TEXT("SELECT sec_type, primary_type, secondary_type, mktval_ind FROM sectype ORDER BY sec_type"));
            g_partSectypeState.result = nanodbc::execute(stmt);
        }

        if (g_partSectypeState.result && g_partSectypeState.result->next())
        {
            g_partSectypeState.cRows++;
            memset(pzST, 0, sizeof(*pzST));
            
            pzST->iSecType = g_partSectypeState.result->get<int>("sec_type", 0);
            read_string(*g_partSectypeState.result, "primary_type", pzST->sPrimaryType, sizeof(pzST->sPrimaryType));
            read_string(*g_partSectypeState.result, "secondary_type", pzST->sSecondaryType, sizeof(pzST->sSecondaryType));
            read_string(*g_partSectypeState.result, "mktval_ind", pzST->sMktValInd, sizeof(pzST->sMktValInd));
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
            g_partSectypeState.cRows = 0;
            g_partSectypeState.result.reset();
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectAllPartSectype", FALSE);
        g_partSectypeState.result.reset();
    }
}

// ============================================================================
// SelectOneSegment
// ============================================================================
DLLAPI void STDCALL SelectOneSegment(char *sSegment, int *piGroupID, int *piLevelID, int iSegmentID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneSegment", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT name, groupid, level_id FROM segments WHERE id = ?"));
        stmt.bind(0, &iSegmentID);

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "name", sSegment, STR64LEN);
            *piGroupID = result.get<int>("groupid", 0);
            *piLevelID = result.get<int>("level_id", 0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectOneSegment", FALSE);
    }
}

// ============================================================================
// SelectSegmain
// ============================================================================
DLLAPI void STDCALL SelectSegmain(int iPortID, int iSegmentType, long *lSegmentID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmain", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT("SELECT id FROM Segmain WHERE Owner_ID = ? AND SegmentTypeID = ?"));
        stmt.bind(0, &iPortID);
        stmt.bind(1, &iSegmentType);

        auto result = nanodbc::execute(stmt);

        if (result.next())
        {
            *lSegmentID = result.get<long>("id", 0);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        *pzErr = PrintError((char*)e.what(), 0, 0, (char*)"", 0, -1, 0, (char*)"SelectSegmain", FALSE);
    }
}
