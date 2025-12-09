/**
 *
 * SUB-SYSTEM: Database Input/Output for Asset Pricing
 *
 * FILENAME: AssetPricingIO.cpp
 *
 * DESCRIPTION:	Defines functions
 *				used for DB IO operations in Delphi's written
 *				AssetPricing.exe.
 *
 *
 * PUBLIC FUNCTIONS(S):
 *
 * NOTES:
 *
 * USAGE:	Part of OLEDB.DLL project.
 *
 * AUTHOR:	Valeriy Yegorov. (C) 2002 Effron Enterprises, Inc.
 *
 *
 **/

// History.
// 2025/11/25 Modernized to C++20 + nanodbc - Sergey
// 3/31/04 Changed precision of AccrInt/DivRate/DivFactor fields - vay
// 08/27/2002  Started.

#include "AssetPricingIO.h"

#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <string>

/**
 *
 * SUB-SYSTEM: Database Input/Output for Asset Pricing
 *
 * FILENAME: AssetPricingIO.cpp
 *
 * DESCRIPTION:	Defines functions
 *				used for DB IO operations in Delphi's written
 *				AssetPricing.exe.
 *
 *
 * PUBLIC FUNCTIONS(S):
 *
 * NOTES:
 *
 * USAGE:	Part of OLEDB.DLL project.
 *
 * AUTHOR:	Valeriy Yegorov. (C) 2002 Effron Enterprises, Inc.
 *
 *
 **/

// History.
// 2025/11/25 Modernized to C++20 + nanodbc - Sergey
// 3/31/04 Changed precision of AccrInt/DivRate/DivFactor fields - vay
// 08/27/2002  Started.

#include "AssetPricingIO.h"
#include "OLEDBIOCommon.h"
#include "dateutils.h"
#include "nanodbc/nanodbc.h"
#include <string>

extern thread_local nanodbc::connection gConn;

//****************************************************************
// InsertPriclist - Modern nanodbc implementation
//****************************************************************

DLLAPI void STDCALL InsertPriclist(PRICLIST zPriclist, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  // trace all calls in debug version
  PrintError("Entering", 0, 0, "", 0, 0, 0, "InsertPriclist", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"InsertPriclist", FALSE);
    return;
  }

  // Before inserting new record, always remove existing one
  // This is done here in order not to touch all calling apps
  DeletePriclist(zPriclist, pzErr);
  if (pzErr->iSqlError != 0)
    return;

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(
        stmt,
        NANODBC_TEXT(
            "INSERT INTO priclist ( "
            "Cusip, Price_Exchange, Price_Date, Price_Source, "
            "date_price_updated, date_exrate_updated, "
            "close_price, bid_price, ask_price, high_price, low_price, "
            "Exrate, inc_exrate, ann_div_cpn, ann_div_fnote, "
            "Volume, sp_rating, Moody_rating, Internal_rating, "
            "Accr_int, Cur_yld, Cur_ytm, Bond_eq_yld, "
            "Yld_to_worst, Ytw_type, Yld_to_best, Ytb_type, Yld_to_earliest, "
            "Yte_type, "
            "Cur_dur, Cur_mod_dur, Maturity, Convexity, Variablerate, "
            "Eps, Pe_ratio, Beta, Debtpershare, Equitypershare, Earning_five, "
            "Nav_factor, shar_outstand, Cumulativesplitfactor, book_value, "
            "Priority, Processed, Create_date, NextYearProjectedPE, Sales "
            ") VALUES ( "
            "?, ?, ?, ?, "
            "?, ?, "
            "?, ?, ?, ?, ?, "
            "?, ?, ?, ?, "
            "?, ?, ?, ?, "
            "?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, "
            "?, ?, ?, ?, ?, ?, "
            "?, ?, ?, ?, "
            "?, ?, ?, ?, ? "
            ")"));

    // Convert dates from long (OLE Automation dates) to nanodbc::timestamp
    nanodbc::timestamp tsPriceDate, tsDatePriceUpdated, tsDateExrateUpdated,
        tsCreateDate;
    long_to_timestamp(zPriclist.lPriceDate, tsPriceDate);
    long_to_timestamp(zPriclist.lDatePriceUpdated, tsDatePriceUpdated);
    long_to_timestamp(zPriclist.lDateExrateUpdated, tsDateExrateUpdated);
    long_to_timestamp(zPriclist.lCreateDate, tsCreateDate);

    // Round all double values to appropriate precision
    auto fClosePrice = RoundDouble(zPriclist.fClosePrice, 6);
    auto fBidPrice = RoundDouble(zPriclist.fBidPrice, 6);
    auto fAskPrice = RoundDouble(zPriclist.fAskPrice, 6);
    auto fHighPrice = RoundDouble(zPriclist.fHighPrice, 6);
    auto fLowPrice = RoundDouble(zPriclist.fLowPrice, 6);

    auto fExrate = RoundDouble(zPriclist.fExrate, 12);
    auto fIncExrate = RoundDouble(zPriclist.fIncExrate, 12);
    auto fAnnDivCpn = RoundDouble(zPriclist.fAnnDivCpn, 5);

    auto fAccrInt = RoundDouble(zPriclist.fAccrInt, 7);
    auto fCurYld = RoundDouble(zPriclist.fCurYld, 5);
    auto fCurYtm = RoundDouble(zPriclist.fCurYtm, 5);
    auto fBondEqYld = RoundDouble(zPriclist.fBondEqYld, 5);

    auto fYldToWorst = RoundDouble(zPriclist.fYldToWorst, 5);
    auto fYldToBest = RoundDouble(zPriclist.fYldToBest, 5);
    auto fYldToEarliest = RoundDouble(zPriclist.fYldToEarliest, 5);

    auto fCurDur = RoundDouble(zPriclist.fCurDur, 3);
    auto fCurModDur = RoundDouble(zPriclist.fCurModDur, 3);
    auto fMaturity = RoundDouble(zPriclist.fMaturity, 3);
    auto fConvexity = RoundDouble(zPriclist.fConvexity, 5);
    auto fVariableRate = RoundDouble(zPriclist.fVariableRate, 3);

    auto fEps = RoundDouble(zPriclist.fEps, 2);
    auto fPeRatio = RoundDouble(zPriclist.fPeRatio, 2);
    auto fBeta = RoundDouble(zPriclist.fBeta, 2);
    auto fDebtPerShare = RoundDouble(zPriclist.fDebtPerShare, 2);
    auto fEquityPerShare = RoundDouble(zPriclist.fEquityPerShare, 2);
    auto fEarningFive = RoundDouble(zPriclist.fEarningFive, 2);

    auto fNavFactor = RoundDouble(zPriclist.fNavFactor, 8);
    auto fSharOutstand = RoundDouble(zPriclist.fSharOutstand, 0);
    auto fCumulativeSplitFactor =
        RoundDouble(zPriclist.fCumulativeSplitFactor, 2);
    auto fBookValue = RoundDouble(zPriclist.fBookValue, 2);

    auto fNextYearProjectedPE = RoundDouble(zPriclist.fNextYearProjectedPE, 2);
    auto fSales = RoundDouble(zPriclist.fSales, 2);

    // Bind all 49 parameters
    int idx = 0;
    safe_bind_string(stmt, idx, zPriclist.sCusip);
    safe_bind_string(stmt, idx, zPriclist.sPriceExchange);
    stmt.bind(idx++, &tsPriceDate);
    safe_bind_string(stmt, idx, zPriclist.sPriceSource);

    stmt.bind(idx++, &tsDatePriceUpdated);
    stmt.bind(idx++, &tsDateExrateUpdated);

    stmt.bind(idx++, &fClosePrice);
    stmt.bind(idx++, &fBidPrice);
    stmt.bind(idx++, &fAskPrice);
    stmt.bind(idx++, &fHighPrice);
    stmt.bind(idx++, &fLowPrice);

    stmt.bind(idx++, &fExrate);
    stmt.bind(idx++, &fIncExrate);
    stmt.bind(idx++, &fAnnDivCpn);
    safe_bind_string(stmt, idx, zPriclist.sAnnDivFnote);

    stmt.bind(idx++, &zPriclist.iVolume);
    safe_bind_string(stmt, idx, zPriclist.sSpRating);
    safe_bind_string(stmt, idx, zPriclist.sMoodyRating);
    safe_bind_string(stmt, idx, zPriclist.sInternalRating);

    stmt.bind(idx++, &fAccrInt);
    stmt.bind(idx++, &fCurYld);
    stmt.bind(idx++, &fCurYtm);
    stmt.bind(idx++, &fBondEqYld);

    stmt.bind(idx++, &fYldToWorst);
    safe_bind_string(stmt, idx, zPriclist.sYtwType);
    stmt.bind(idx++, &fYldToBest);
    safe_bind_string(stmt, idx, zPriclist.sYtbType);
    stmt.bind(idx++, &fYldToEarliest);
    safe_bind_string(stmt, idx, zPriclist.sYteType);

    stmt.bind(idx++, &fCurDur);
    stmt.bind(idx++, &fCurModDur);
    stmt.bind(idx++, &fMaturity);
    stmt.bind(idx++, &fConvexity);
    stmt.bind(idx++, &fVariableRate);

    stmt.bind(idx++, &fEps);
    stmt.bind(idx++, &fPeRatio);
    stmt.bind(idx++, &fBeta);
    stmt.bind(idx++, &fDebtPerShare);
    stmt.bind(idx++, &fEquityPerShare);
    stmt.bind(idx++, &fEarningFive);

    stmt.bind(idx++, &fNavFactor);
    stmt.bind(idx++, &fSharOutstand);
    stmt.bind(idx++, &fCumulativeSplitFactor);
    stmt.bind(idx++, &fBookValue);

    stmt.bind(idx++, &zPriclist.iPriority);
    safe_bind_string(stmt, idx, zPriclist.sProcessed);
    stmt.bind(idx++, &tsCreateDate);
    stmt.bind(idx++, &fNextYearProjectedPE);
    stmt.bind(idx++, &fSales);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in InsertPriclist: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"InsertPriclist", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in InsertPriclist", 0, 0,
                        (char *)"D", 0, -1, 0, (char *)"InsertPriclist", FALSE);
  }
}

//****************************************************************
// DeletePriclist - Modern nanodbc implementation
//****************************************************************

DLLAPI void STDCALL DeletePriclist(PRICLIST zPriclist, ERRSTRUCT *pzErr) {
#ifdef DEBUG
  // trace all calls in debug version
  PrintError("Entering", 0, 0, "", 0, 0, 0, "DeletePriclist", FALSE);
#endif

  InitializeErrStruct(pzErr);

  if (!gConn.connected()) {
    *pzErr = PrintError((char *)"Database not connected", 0, 0, (char *)"D", 0,
                        -1, 0, (char *)"DeletePriclist", FALSE);
    return;
  }

  try {
    nanodbc::statement stmt(gConn);
    nanodbc::prepare(stmt,
                     NANODBC_TEXT("DELETE FROM priclist "
                                  "WHERE cusip = ? AND price_exchange = ? AND "
                                  "price_date = ? AND price_source = ?"));

    // Convert date from long (OLE Automation date) to nanodbc::timestamp
    nanodbc::timestamp tsPriceDate;
    long_to_timestamp(zPriclist.lPriceDate, tsPriceDate);

    int idx = 0;
    safe_bind_string(stmt, idx, zPriclist.sCusip);
    safe_bind_string(stmt, idx, zPriclist.sPriceExchange);
    stmt.bind(idx++, &tsPriceDate);
    safe_bind_string(stmt, idx, zPriclist.sPriceSource);

    nanodbc::execute(stmt);
  } catch (const nanodbc::database_error &e) {
    std::string msg = "Database error in DeletePriclist: ";
    msg += e.what();
    *pzErr = PrintError((char *)msg.c_str(), 0, 0, (char *)"D", 0, -1, 0,
                        (char *)"DeletePriclist", FALSE);
  } catch (...) {
    *pzErr = PrintError((char *)"Unexpected error in DeletePriclist", 0, 0,
                        (char *)"D", 0, -1, 0, (char *)"DeletePriclist", FALSE);
  }
}

//****************************************************************
// Initialization and cleanup functions
//****************************************************************

DLLAPI ERRSTRUCT STDCALL InitializeAssetPricingIO() {
  ERRSTRUCT zErr;
  InitializeErrStruct(&zErr);
  // With nanodbc, no preparation needed - statements are prepared on-demand
  return zErr;
}

void CloseAssetPricingIO(void) {
  // With nanodbc RAII, no explicit close needed
}

DLLAPI void STDCALL FreeAssetPricingIO(void) {
  // With nanodbc RAII, no explicit cleanup needed
  CloseAssetPricingIO();
}
