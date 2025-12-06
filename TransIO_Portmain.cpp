#include "TransIO_Portmain.h"
#include "OLEDBIOCommon.h"
#include "ODBCErrorChecking.h"
#include "nanodbc/nanodbc.h"
#include "dateutils.h"
#include <iostream>

extern thread_local nanodbc::connection gConn;
extern PORTMAIN zSavedPMain;
extern bool bPMainIsValid;

// Helper to fill PORTMAIN struct from result
void FillPortmainStruct(nanodbc::result& result, PORTMAIN* pzPR)
{
    pzPR->iID = result.get<int>("id", 0);
    read_string(result, "unique_name", pzPR->sUniqueName, sizeof(pzPR->sUniqueName));
    read_string(result, "abbrev", pzPR->sAbbrev, sizeof(pzPR->sAbbrev));
    read_string(result, "description", pzPR->sDescription, sizeof(pzPR->sDescription));
    pzPR->lDateHired = timestamp_to_long(result.get<nanodbc::timestamp>("datehired"));
    pzPR->fIndividualMinAnnualFee = result.get<double>("individual_min_annual_fee", 0.0);
    pzPR->fIndividualMinAcctSize = result.get<double>("individual_min_acct_size", 0.0);
    pzPR->fTotalAssetsManaged = result.get<double>("totalassetsmanaged", 0.0);
    pzPR->iInvestmentStyle = result.get<int>("investmentstyle", 0);
    pzPR->iScope = result.get<int>("scope", 0);
    pzPR->iDecisionMaking = result.get<int>("decisionmaking", 0);
    pzPR->iDefaultReturnType = result.get<int>("defaultreturntype", 0);
    pzPR->iProductType = result.get<int>("producttype", 0);
    pzPR->fExpenseRatio = result.get<double>("expenseratio", 0.0);
    pzPR->iMarketCap = result.get<int>("marketcap", 0);
    pzPR->iMaturity = result.get<int>("maturity", 0);
    pzPR->lAsofDate = timestamp_to_long(result.get<nanodbc::timestamp>("asofdate"));
    pzPR->iFiscalYearEndMonth = result.get<int>("fiscalyearendmonth", 0);
    pzPR->iFiscalYearEndDay = result.get<int>("fiscalyearendday", 0);
    read_string(result, "periodtype", pzPR->sPeriodType, sizeof(pzPR->sPeriodType));
    pzPR->lInceptionDate = timestamp_to_long(result.get<nanodbc::timestamp>("inceptiondate"));
    
    char sUserInceptionDate[2] = {0};
    read_string(result, "userinceptiondate", sUserInceptionDate, sizeof(sUserInceptionDate));
    pzPR->bUserInceptionDate = Char2BOOL(sUserInceptionDate);

    read_string(result, "portfoliotype", pzPR->sPortfolioType, sizeof(pzPR->sPortfolioType));
    read_string(result, "administrator", pzPR->sAdministrator, sizeof(pzPR->sAdministrator));
    read_string(result, "manager", pzPR->sManager, sizeof(pzPR->sManager));
    read_string(result, "address1", pzPR->sAddress1, sizeof(pzPR->sAddress1));
    read_string(result, "address2", pzPR->sAddress2, sizeof(pzPR->sAddress2));
    read_string(result, "address3", pzPR->sAddress3, sizeof(pzPR->sAddress3));
    read_string(result, "acctmethod", pzPR->sAcctMethod, sizeof(pzPR->sAcctMethod));
    read_string(result, "tax", pzPR->sTax, sizeof(pzPR->sTax));
    read_string(result, "basecurrid", pzPR->sBaseCurrId, sizeof(pzPR->sBaseCurrId));

    char sIncome[2] = {0};
    read_string(result, "income", sIncome, sizeof(sIncome));
    pzPR->bIncome = Char2BOOL(sIncome);

    char sActions[2] = {0};
    read_string(result, "actions", sActions, sizeof(sActions));
    pzPR->bActions = Char2BOOL(sActions);

    char sMature[2] = {0};
    read_string(result, "mature", sMature, sizeof(sMature));
    pzPR->bMature = Char2BOOL(sMature);

    char sCAvail[2] = {0};
    read_string(result, "cavail", sCAvail, sizeof(sCAvail));
    pzPR->bCAvail = Char2BOOL(sCAvail);

    char sFAvail[2] = {0};
    read_string(result, "favail", sFAvail, sizeof(sFAvail));
    pzPR->bFAvail = Char2BOOL(sFAvail);

    read_string(result, "alloc", pzPR->sAlloc, sizeof(pzPR->sAlloc));
    pzPR->fMaxEqPct = result.get<double>("maxeqpct", 0.0);
    pzPR->fMaxFiPct = result.get<double>("maxfipct", 0.0);
    pzPR->fMinCashPct = result.get<double>("mincashpct", 0.0);
    pzPR->iEqLotSize = result.get<int>("eqlotsize", 0);
    pzPR->iFiLotSize = result.get<int>("filotsize", 0);
    pzPR->lValDate = timestamp_to_long(result.get<nanodbc::timestamp>("valdate"));
    pzPR->lDeleteDate = timestamp_to_long(result.get<nanodbc::timestamp>("deletedate"));

    char sIsInactive[2] = {0};
    read_string(result, "isinactive", sIsInactive, sizeof(sIsInactive));
    pzPR->bIsInactive = Char2BOOL(sIsInactive);

    read_string(result, "currhandler", pzPR->sCurrHandler, sizeof(pzPR->sCurrHandler));

    char sAmortMuni[2] = {0};
    read_string(result, "amortmuni", sAmortMuni, sizeof(sAmortMuni));
    pzPR->bAmortMuni = Char2BOOL(sAmortMuni);

    char sAmortOther[2] = {0};
    read_string(result, "amortother", sAmortOther, sizeof(sAmortOther));
    pzPR->bAmortOther = Char2BOOL(sAmortOther);

    char sAccreteDisc[2] = {0};
    read_string(result, "accretedisc", sAccreteDisc, sizeof(sAccreteDisc));
    pzPR->bAccreteDisc = Char2BOOL(sAccreteDisc);

    pzPR->lAmortStartDate = timestamp_to_long(result.get<nanodbc::timestamp>("amortstartdate"));

    char sIncByLot[2] = {0};
    read_string(result, "incbylot", sIncByLot, sizeof(sIncByLot));
    pzPR->bIncByLot = Char2BOOL(sIncByLot);

    char sDiscretionaryAuthority[2] = {0};
    read_string(result, "discretionaryauthority", sDiscretionaryAuthority, sizeof(sDiscretionaryAuthority));
    pzPR->bDiscretionaryAuthority = Char2BOOL(sDiscretionaryAuthority);

    char sVotingAuthority[2] = {0};
    read_string(result, "votingauthority", sVotingAuthority, sizeof(sVotingAuthority));
    pzPR->bVotingAuthority = Char2BOOL(sVotingAuthority);

    char sSpecialArrangements[2] = {0};
    read_string(result, "specialarrangements", sSpecialArrangements, sizeof(sSpecialArrangements));
    pzPR->bSpecialArrangements = Char2BOOL(sSpecialArrangements);

    pzPR->iIncomeMoneyMarketFund = result.get<int>("incomemoneymarketfund", 0);
    pzPR->iPrincipalMoneyMarketFund = result.get<int>("principalmoneymarketfund", 0);
    read_string(result, "incomeprocessing", pzPR->sIncomeProcessing, sizeof(pzPR->sIncomeProcessing));
    pzPR->lPricingEffectiveDate = timestamp_to_long(result.get<nanodbc::timestamp>("pricingeffectivedate"));
    pzPR->lLastTransNo = result.get<long>("lasttransno", 0);
    pzPR->lPurgeDate = timestamp_to_long(result.get<nanodbc::timestamp>("purgedate"));
    pzPR->lLastActivity = timestamp_to_long(result.get<nanodbc::timestamp>("lastactivity"));
    pzPR->lRollDate = timestamp_to_long(result.get<nanodbc::timestamp>("rolldate"));
    pzPR->iVendorID = result.get<int>("DefaultVendorID", 0);

    char sAccretMuni[2] = {0};
    read_string(result, "accretmuni", sAccretMuni, sizeof(sAccretMuni));
    pzPR->bAccretMuni = Char2BOOL(sAccretMuni);

    char sAccretOther[2] = {0};
    read_string(result, "accretother", sAccretOther, sizeof(sAccretOther));
    pzPR->bAccretOther = Char2BOOL(sAccretOther);
}

DLLAPI void STDCALL UpdatePortmainLastTransNo(long lLastTransNo, int iID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"UpdatePortmainLastTransNo", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "UPDATE Portmain SET lasttransno = ? WHERE ID = ?"));

        stmt.bind(0, &lLastTransNo);
        stmt.bind(1, &iID);

        nanodbc::execute(stmt);

        if (bPMainIsValid && zSavedPMain.iID == iID) 
			zSavedPMain.lLastTransNo = lLastTransNo;
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in UpdatePortmainLastTransNo: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"UpdatePortmainLastTransNo", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in UpdatePortmainLastTransNo", iID, 0, (char*)"", 0, -1, 0, (char*)"UpdatePortmainLastTransNo", FALSE);
    }
}

DLLAPI void STDCALL SelectPortmain(PORTMAIN *pzPR, int iID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectPortmain", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT p.id, unique_name, abbrev, description, "
            "datehired, individual_min_annual_fee, individual_min_acct_size, "
            "totalassetsmanaged, p.investmentstyle, scope, decisionmaking, "
            "defaultreturntype, producttype, expenseratio, marketcap, "
            "maturity, asofdate, fiscalyearendmonth, fiscalyearendday, "
            "periodtype, inceptiondate, userinceptiondate, "
            "portfoliotype, administrator, manager, "
            "address1, address2, address3, acctmethod, tax, "
            "basecurrid, income, actions, mature, cavail, favail, "
            "alloc, maxeqpct, maxfipct, mincashpct, eqlotsize, filotsize, "
            "valdate, deletedate, isinactive, currhandler, amortmuni, "
            "amortother, accretedisc, amortstartdate, incbylot, "
            "discretionaryauthority, votingauthority, specialarrangements, "
            "incomemoneymarketfund, principalmoneymarketfund, "
            "incomeprocessing, pricingeffectivedate, lasttransno, "
            "purgedate, lastactivity, rolldate, pinfo.DefaultVendorID, pinfo.accretmuni, pinfo.accretother "
            "FROM portmain as p "
            "left join portinfo as pinfo on pinfo.id = p.id WHERE p.id=?"));

        stmt.bind(0, &iID);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillPortmainStruct(result, pzPR);
            memcpy(&zSavedPMain, pzPR, sizeof(zSavedPMain));
			bPMainIsValid = true;
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectPortmain: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"SelectPortmain", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectPortmain", iID, 0, (char*)"", 0, -1, 0, (char*)"SelectPortmain", FALSE);
    }
}

DLLAPI void STDCALL SelectPortmainByUniqueName(PORTMAIN *pzPR, char *sName, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectPortmainByUniqueName", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT p.id, unique_name, abbrev, description, "
            "datehired, individual_min_annual_fee, individual_min_acct_size, "
            "totalassetsmanaged, p.investmentstyle, scope, decisionmaking, "
            "defaultreturntype, producttype, expenseratio, marketcap, "
            "maturity, asofdate, fiscalyearendmonth, fiscalyearendday, "
            "periodtype, inceptiondate, userinceptiondate, "
            "portfoliotype, administrator, manager, "
            "address1, address2, address3, acctmethod, tax, "
            "basecurrid, income, actions, mature, cavail, favail, "
            "alloc, maxeqpct, maxfipct, mincashpct, eqlotsize, filotsize, "
            "valdate, deletedate, isinactive, currhandler, amortmuni, "
            "amortother, accretedisc, amortstartdate, incbylot, "
            "discretionaryauthority, votingauthority, specialarrangements, "
            "incomemoneymarketfund, principalmoneymarketfund, "
            "incomeprocessing, pricingeffectivedate, lasttransno, "
            "purgedate, lastactivity, rolldate, pinfo.DefaultVendorID, pinfo.accretmuni, pinfo.accretother "
            "FROM portmain as p "
            "left join portinfo as pinfo on pinfo.id = p.id WHERE p.unique_name=?"));

        stmt.bind(0, sName);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            FillPortmainStruct(result, pzPR);
            memcpy(&zSavedPMain, pzPR, sizeof(zSavedPMain));
			bPMainIsValid = true;
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectPortmainByUniqueName: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), pzPR->iID, 0, sName, 0, -1, 0, (char*)"SelectPortmainByUniqueName", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectPortmainByUniqueName", pzPR->iID, 0, sName, 0, -1, 0, (char*)"SelectPortmainByUniqueName", FALSE);
    }
}

DLLAPI void STDCALL SelectAcctMethod(char *sAcctMethod, int iID, ERRSTRUCT *pzErr)
{
    InitializeErrStruct(pzErr);

    if (bPMainIsValid && zSavedPMain.iID == iID)
	{
		strcpy_s(sAcctMethod, 2, zSavedPMain.sAcctMethod);
		return;
	}

    if (!gConn.connected())
    {
        *pzErr = PrintError((char*)"Database not connected", 0, 0, (char*)"T", 0, -1, 0, (char*)"SelectAcctMethod", FALSE);
        return;
    }

    try
    {
        nanodbc::statement stmt(gConn);
        nanodbc::prepare(stmt, NANODBC_TEXT(
            "SELECT acctmethod FROM Portmain WHERE ID = ?"));

        stmt.bind(0, &iID);

        nanodbc::result result = nanodbc::execute(stmt);

        if (result.next())
        {
            read_string(result, "acctmethod", sAcctMethod, 2);
        }
        else
        {
            pzErr->iSqlError = SQLNOTFOUND;
        }
    }
    catch (const nanodbc::database_error& e)
    {
        std::string msg = "Database error in SelectAcctMethod: ";
        msg += e.what();
        *pzErr = PrintError((char*)msg.c_str(), iID, 0, (char*)"", 0, -1, 0, (char*)"SelectAcctMethod", FALSE);
    }
    catch (...)
    {
        *pzErr = PrintError((char*)"Unexpected error in SelectAcctMethod", iID, 0, (char*)"", 0, -1, 0, (char*)"SelectAcctMethod", FALSE);
    }
}
