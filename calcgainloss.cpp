/*H*
*
* FILENAME: CalcGainLoss.c
*
* DESCRIPTION:
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Shobhit Barman (EFFRON ENTERPRISES, INC.)
*
// 2018-08-30 J#PER-8997 Restored logic w/future types, but kept exchange rate
-mk.
// 2018-08-03 VI# 61746 Fixed issue w/those future types -mk
// 2018-07-31 VI# 61746 Added ability to calculate cost in transengine and G\L
in calcgainloss for Futures, with secondary type N -mk
// 2018-04-26 VI# 61309 Fixed G/L for short positions - mk
// 2018-03-06 VI# 60490: added code to adjust logic for total cost and G\L on
futures -mk. *H*/

#include "calcgainloss.h"
#include "commonheader.h"

/*F*
** Main function for the dll
*F*/
BOOL APIENTRY DllMain(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved) {

  switch (dwReason) {
  case DLL_PROCESS_ATTACH:
    InitializeCalcGainLossLibrary();
    break;

  case DLL_PROCESS_DETACH:
    FreeCalcGainLossLibrary();
    break;

  default:
    break;
  }

  return TRUE;
} // DllMain

/*F*
** Function to calculate security, currency, short term and long term
** gain & loss for a security
*F*/
extern "C" {
int STDCALL WINAPI CalcGainLoss(TRANS zTR, char *sTranCode, long lSecImpact,
                                char *sPrimaryType, char *sSecondaryType,
                                char *sBaseCurrency, double *pzCGL,
                                double *pzSGL, double *pzSTGL, double *pzMTGL,
                                double *pzLTGL, double *pzTotGL) {
  short mdy[3];
  long lDec311990, lJan11994;

  *pzCGL = *pzSGL = *pzSTGL = *pzLTGL = 0;
  *pzMTGL = *pzTotGL = 0;

  /*
  ** if not a closing transaction and it is not a "LD" transaction, or an
  * "FD"/"FX" transaction,
  ** there is no gain/loss.
  ** SB   3/27/2001 - According to Rella, Now Gain/Loss will be calculated even
  * for UP.
  ** vay 11/15/2001 - by SB's request GL will be calculated on PM/ST/LT trans
  * also
  */
  if (((strcmp(sTranCode, "C") != 0 && strcmp(zTR.sTranType, "LD") == 0) ||
       (strcmp(zTR.sTranType, "FD") == 0) ||
       (strcmp(zTR.sTranType, "FX") ==
        0)) // || strcmp(zTR.sSecXtend, "UP") == 0))
      &&
      !(strcmp(zTR.sTranType, "PM") == 0 || strcmp(zTR.sTranType, "LT") == 0 ||
        strcmp(zTR.sTranType, "ST") == 0))
    return 0;

  /* if the base xrate or the open base xrate is zero make it 1 */
  if (zTR.fBaseXrate == 0)
    zTR.fBaseXrate = 1;

  if (zTR.fSecBaseXrate == 0)
    zTR.fSecBaseXrate = 1;

  if (zTR.fBaseOpenXrate == 0)
    zTR.fBaseOpenXrate = 1;

  // All the gain/loss from forward contract is currency gain/loss
  if (*sPrimaryType == PTYPE_FUTURE && *sSecondaryType == STYPE_CONTRACTS) {
    if (strcmp(zTR.sDrCr, "DR") == 0)
      *pzTotGL = *pzCGL = zTR.fPcplAmt * -1;
    else
      *pzTotGL = *pzCGL = zTR.fPcplAmt;

    return 0;
  } // Forward contract
  else if ((*sPrimaryType == PTYPE_FUTURE) &&
           (*sSecondaryType == STYPE_INDEX ||
            *sSecondaryType == STYPE_FOREIGN)) {
    *pzCGL = 0;
    //*pzTotGL = *pzSGL = zTR.fPcplAmt * lSecImpact * -1;
    // if (strcmp(zTR.sDrCr, "DR") == 0)
    *pzTotGL = *pzSGL =
        -1 * lSecImpact *
        (zTR.fPcplAmt / zTR.fBaseXrate); // * zTR.fSecBaseXrate/
                                         // zTR.fBaseOpenXrate; else
    //	*pzTotGL = *pzSGL = (zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate/
    // zTR.fBaseOpenXrate;
  } // Forward contract
  else if (lSecImpact == 1) {
    // Total Gain Loss = totcost/basexrate - (pcplamt/basexrate)*secbasexrate -
    // optprem/secbasexrate
    *pzTotGL = (zTR.fTotCost / zTR.fSecBaseXrate) -
               (((zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate) /
                zTR.fBaseOpenXrate) -
               (zTR.fOptPrem / zTR.fSecBaseXrate);

    // Currency GainLoss = pcplamt/basexrate -
    // ((pcplamt/basexrate)*secbasexrate)/baseopenxrate
    *pzCGL = (zTR.fPcplAmt / zTR.fBaseXrate) -
             (((zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate) /
              zTR.fBaseOpenXrate);
  } // short
  else {
    // Total Gain Loss = pcplamt /basexrate - totcost/baseopenxrate -
    // optprem/secbasexrate
    *pzTotGL = (zTR.fPcplAmt / zTR.fBaseXrate) -
               (zTR.fTotCost / zTR.fBaseOpenXrate) -
               (zTR.fOptPrem / zTR.fSecBaseXrate);
    /*
    ** Currency Gain Loss = totcost / basexrate - totcost / baseopenxrate. For a
    * loan, it's
    ** the other way
    */
    if (*sPrimaryType == PTYPE_LOAN) /* closing of a loan */
      *pzCGL = (zTR.fTotCost / zTR.fBaseOpenXrate) -
               (zTR.fTotCost / zTR.fSecBaseXrate);
    else
      *pzCGL = (zTR.fTotCost / zTR.fSecBaseXrate) -
               (zTR.fTotCost / zTR.fBaseOpenXrate);

  } // long

  // Security Gain Loss = Total Gain Loss - Currency Gain Loss
  *pzSGL = *pzTotGL - *pzCGL;

  /* if portfolio's base currency != currency */
  if (strcmp(sBaseCurrency, zTR.sCurrId) != 0) {
    if (*sPrimaryType == PTYPE_FUTURE && *sSecondaryType != STYPE_INDEX &&
        *sSecondaryType != STYPE_FOREIGN) /* future */
    {
      *pzCGL = 0;
      *pzSGL = zTR.fPcplAmt;
      *pzTotGL = zTR.fPcplAmt;
    } else if (strcmp(zTR.sTranType, "CS") == 0) /* cover of a short position */
    {
      *pzCGL = 0;
      *pzSGL = zTR.fPcplAmt;
    }
  } /* if portfolio's Base Currency != currency */

  mdy[0] = 12;
  mdy[1] = 31;
  mdy[2] = 1990;
  lpfnrmdyjul(mdy, &lDec311990); /* start date is 12/31/1990 */

  mdy[0] = 1;
  mdy[1] = 1;
  mdy[2] = 1994;
  lpfnrmdyjul(mdy, &lJan11994); /* end date is 1/1/1994 */

  /* discounted security between 12/31/90 and 1/1/94 */
  if (*sPrimaryType == PTYPE_DISCOUNT && zTR.lEffDate > lDec311990 &&
      zTR.lEffDate < lJan11994) {
    *pzSGL = 0;
    *pzCGL = (zTR.fTotCost / zTR.fSecBaseXrate) -
             (zTR.fTotCost / zTR.fBaseOpenXrate);
    *pzTotGL = *pzCGL;
  }

  /* discounted security and accr int is not zero */
  if (*sPrimaryType == PTYPE_DISCOUNT && zTR.fAccrInt != 0.0) {
    *pzSGL -= zTR.fAccrInt;
    *pzTotGL = *pzSGL + *pzCGL;
  }

  /* Redistribute the gain as per Section 988 of the tax code */
  if (strcmp(sBaseCurrency, zTR.sCurrId) != 0)
    ApplySection988(pzCGL, pzSGL, pzTotGL);

  /* Calculate whether the security gain loss is short-term or long-term */
  STLTGainLoss(zTR, pzSGL, pzSTGL, pzLTGL, lSecImpact, sPrimaryType,
               sSecondaryType);

  // double *pzCGL, double *pzSGL,
  /*double *pzSTGL, double *pzMTGL, double *pzLTGL, double *pzTotGL
  if (zTR.fUnits < 0) {
          *pzCGL = *pzCGL * -1;
          *pzSGL = *pzSGL * -1;
          *pzSTGL = *pzSTGL * -1;
          *pzMTGL = *pzMTGL * -1;
          *pzLTGL = *pzLTGL * -1;
          *pzTotGL = *pzTotGL * -1;
  }*/

  return 0;
}
}

/*F*
** For tax purposes, redistribute gain as per PRICE WATERHOUSE interpretation
** of Section 988. Function has three arguments, currency gain/loss, security
** gain/loss and total gain/loss
*F*/
void ApplySection988(double *pzCGL, double *pzSGL, double *pzTotGL) {
  // if total currency GL = 0, nothing to do
  if (*pzCGL == 0)
    return;

  // CASE A - If CGL > 0 and SGL > 0, nothing to do
  if (*pzCGL > 0 && *pzSGL > 0)
    return;
  // CASE B - If CGL < 0 and SGL < 0, nothing to do
  else if (*pzCGL < 0 && *pzSGL < 0)
    return;
  // CASE C - If CGL > TotGL and TotGL >= 0, All the gain loss is currency GL
  else if (*pzCGL > *pzTotGL && *pzTotGL >= 0) {
    *pzSGL = 0;
    *pzCGL = *pzTotGL;
  }
  // CASE D - If CGL > 0 and TotGL < 0, all the gain loss is Security GL
  else if (*pzCGL > 0 && *pzTotGL < 0) {
    *pzCGL = 0;
    *pzSGL = *pzTotGL;
  }
  // CASE E - If CGL < 0 and TotGL > 0, all the gain loss is Security GL
  else if (*pzCGL < 0 && *pzTotGL > 0) {
    *pzCGL = 0;
    *pzSGL = *pzTotGL;
    return;
  }
  // CASE F - If CGL < TotGL, all the gain loss is currency gain loss
  else if (*pzCGL < *pzTotGL) {
    *pzCGL = *pzTotGL;
    *pzSGL = 0;
  }

  return;
}

/*F*
** Function to find out how much of the security gain-loss is long-term,
** and short-term gain-loss.
*F*/
void STLTGainLoss(TRANS zTR, double *pzSGL, double *pzSTGL, double *pzLTGL,
                  long lSecImpact, char *sPrimaryType, char *sSecondaryType) {
  long lJun221984, lJan11988;                  //, lMay71997, lJul281997;
  long l6MonthsDate, l12MonthsDate, lTempDate; // l18MonthsDate,
  short mdy[3];

  *pzLTGL = *pzSTGL = 0;
  /*
   ** If the gain/loss column on the trans contains an 'L', then
   ** force a long term capital gain
   */
  if (strcmp(zTR.sGlFlag, "L") == 0) {
    *pzLTGL = *pzSGL;
    return;
  }

  // Future or an index option
  if (*sPrimaryType == PTYPE_FUTURE ||
      (*sPrimaryType == PTYPE_OPTION && *sSecondaryType == STYPE_INDEX)) {
    *pzSTGL = *pzSGL * 0.4; // 40 % of security gain loss is short term and
    *pzLTGL = *pzSGL * 0.6; // 60 % of security gain loss is long term
    return;
  }

  /* close of a future or a short, treated as short term gain */
  if ((lSecImpact == 1 && zTR.lRevTransNo == 0) ||
      (lSecImpact == -1 && zTR.lRevTransNo != 0)) {
    *pzSTGL = *pzSGL;
    return;
  }

  /*
   ** if the trade date of the transaction is between 6/22/84 and
   ** 1/1/88, the trade is subject to long term gain/loss rates if the
   ** security was held for 6 months or more. Otherwise, the trade is
   ** subject to long term gain/loss rates if the security is held for
   ** more than 365 days or one year
   */
  mdy[0] = 6;
  mdy[1] = 22;
  mdy[2] = 1984;
  lpfnrmdyjul(mdy, &lJun221984); /* start date is 6/22/84 */

  mdy[0] = 1;
  mdy[1] = 1;
  mdy[2] = 1988;
  lpfnrmdyjul(mdy, &lJan11988); /* end date is 1/1/88 */

  /*mdy[0] = 5;
  mdy[1] = 7;
  mdy[2] = 1997;
        lpfnrmdyjul(mdy, &lMay71997); / * date is 5/7/97 * /


  mdy[0] = 7;
  mdy[1] = 28;
  mdy[2] = 1997;
        lpfnrmdyjul(mdy, &lJul281997); / * date is 7/28/97 */
  if (strcmp(zTR.sTranType, "LD") == 0)
    zTR.lTrdDate = zTR.lStlDate;

  /*
  ** There are two ways to check the requiremnet for long term(or medium term).
  ** First method is add required number of months to the trade date and
  ** any security which was sold after the resulting date is long term.
  ** Second method is add 1 day to the trade date then add the required
  ** number of months to the new date and any security which was sold
  ** on or after the resulting date is long term.
  ** First method gives wrong result in some cases, CORRECT method is
  ** the Second Method(add 1 day and then add required number of months).
  **
  ** Let's say the requirement of Long Term is that any security held for
  ** longer than 18 months is long term. Let's also say that a security
  ** was purchased on 4/30/97 and sold on 10/31/98.
  ** If we choose	method 1 then adding 18 months will give us 10/30/98
  ** and any security sold after 10/30/98 will become long term, but that
  ** is wrong, the security will be considered long term only if it is
  ** sold on or after 11/1/98.
  ** If we choose second method, advancing trade date by 1 day gives us
  ** 5/1/97 and advancing that by 18 moths gives us 11/1/98 and then the
  ** given trade will be considered short term which is right.
  */
  if (zTR.lTrdDate > lJun221984 && zTR.lTrdDate < lJan11988) {
    // First advance the date by 1 day
    lpfnNewDateFromCurrent(zTR.lOpenTrdDate, TRUE, 0, 0, 1, &lTempDate);

    // Add 6 months
    lpfnNewDateFromCurrent(lTempDate, TRUE, 0, 6, 0, &l6MonthsDate);

    if (zTR.lTrdDate >= l6MonthsDate)
      *pzLTGL = *pzSGL;
    else
      *pzSTGL = *pzSGL;
  } else {
    // First advance the date by 1 day
    lpfnNewDateFromCurrent(zTR.lOpenTrdDate, TRUE, 0, 0, 1, &lTempDate);

    // Add 12 months
    lpfnNewDateFromCurrent(lTempDate, TRUE, 0, 12, 0, &l12MonthsDate);

    // Add 18 months
    // lpfnNewDateFromCurrent(lTempDate, TRUE, 0, 18, 0, &l18MonthsDate);

    // Trade date less than 12 months is a short-term gain, else Long Term
    if (zTR.lTrdDate < l12MonthsDate)
      *pzSTGL = *pzSGL; // short-term Gain-Loss
    else
      *pzLTGL = *pzSGL; // long-term Gain-Loss

    /* OLD LAW
    ** Trade date less than 12 months is a short-term gain. If the trade
    ** date is greater than or equal to 12 months but less than 18 months
    ** and it is prior to May 7, 1997 or later than July 28 1997 it is
    ** medium-term. If the trade date is greater than or equal to 12 months
    ** but less than 18 months and it is between May 7, 1997 and July 28 1997,
    ** it is Long-term. If trade date is greater or equal to 18 months,
    ** gain-loss is long-term.
    */
    /*if (zTR.lTrdDate < l12MonthsDate)
       *pzSTGL = *pzSGL; // short-term Gain-Loss
    else if (zTR.lTrdDate >= l12MonthsDate && zTR.lTrdDate < l18MonthsDate  &&
             zTR.lTrdDate < lMay71997)
       *pzMTGL = *pzSGL; //medium-term
    else if (zTR.lTrdDate >= l12MonthsDate && zTR.lTrdDate < l18MonthsDate &&
             zTR.lTrdDate > lJul281997)
       *pzMTGL = *pzSGL; // medium-term
    else if (zTR.lTrdDate >= l12MonthsDate && zTR.lTrdDate < l18MonthsDate &&
             zTR.lTrdDate >= lMay71997 && zTR.lTrdDate <= lJul281997)
        *pzLTGL = *pzSGL; // long-term
    else // greater than 18 months
       *pzLTGL = *pzSGL; //long-term*/
  } // if trade date is not between June 22, 1984 and Jan. 1, 1988

  return;
} // STMTLTGainLoss

/*F*
** Function to calculate market discount
*F*/
extern "C" {
int STDCALL WINAPI CalcMarketDiscount(TRANS zTR, long lIssueDate,
                                      long lMaturityDate,
                                      double fRedemptionPrice, double fTradUnit,
                                      double fGainLoss, char *sPrimaryType,
                                      char *sSecondaryType,
                                      double *pfMarketDiscount) {
  long lDaysHeld, lBondLife, lYearsToMaturity, lTempDate1, lTempDate2;
  double fDiscount, fDailyAccretion, fUnitDiscount;
  short mdy[3];

  *pfMarketDiscount = 0;

  // No market discount, if no gain
  if (fGainLoss <= 0)
    return 0;

  // If invalid issue date, default it to purchase date
  if (lIssueDate <= 0)
    lIssueDate = zTR.lOpenTrdDate;

  // No market discount for equity mutual funds
  if (*sPrimaryType == PTYPE_BOND && *sSecondaryType == STYPE_MFUND)
    return 0;

  // Can not do anything if any of the dates are wrong
  if (lMaturityDate <= 0 || lMaturityDate - lIssueDate <= 0)
    return 0;

  mdy[0] = 7;
  mdy[1] = 19;
  mdy[2] = 1984;
  lpfnrmdyjul(mdy, &lTempDate1);

  mdy[0] = 5;
  mdy[1] = 1;
  mdy[2] = 1993;
  lpfnrmdyjul(mdy, &lTempDate2);

  // No market discount for bonds issued before 7/19/1984 and purchased before
  // 5/1/1993
  if (lIssueDate < lTempDate1 && zTR.lOpenTrdDate < lTempDate2)
    return 0;

  // No market discount for Municipal Bond purchased before 5/1/1999
  if (*sPrimaryType == PTYPE_BOND && *sSecondaryType == STYPE_MUNICIPALS &&
      zTR.lOpenTrdDate < lTempDate2)
    return 0;

  // Calculate number of days this security was held
  lDaysHeld = zTR.lTrdDate - zTR.lOpenTrdDate;
  if (lDaysHeld <= 0)
    return 0;

  // Calculate number of days in the life of the bond
  lBondLife = lMaturityDate - zTR.lOpenTrdDate;
  if (lBondLife <= 0)
    return 0;

  // Calculate discount = redemption price - unit cost
  if (zTR.fUnits == 0 || fTradUnit == 0)
    return 0;
  else
    fUnitDiscount =
        fRedemptionPrice - (zTR.fTotCost / (zTR.fUnits * fTradUnit));

  // Calculate daily accretion amount
  fDailyAccretion = fUnitDiscount / lBondLife;

  // Calculate discount
  fDiscount = fDailyAccretion * lDaysHeld;

  // Market discount applies only if unit discount is greater than 1/4 of 1% of
  // the redemption value * complete years to maturity
  lYearsToMaturity = (int)((lMaturityDate - lIssueDate) / 365);
  if (fUnitDiscount >= fRedemptionPrice * lYearsToMaturity * 0.0025) {
    *pfMarketDiscount = fDiscount * zTR.fUnits * fTradUnit;
    // Market Discount can not be more that the total gain
    if (*pfMarketDiscount > fGainLoss)
      *pfMarketDiscount = fGainLoss;
  }

  return 0;
} // CalcMarketDiscount
}

/* CalcMarketDiscountConstantYield = _CalcMarketDiscountConstantYield@604
DLLAPI int STDCALL WINAPI CalcMarketDiscountConstantYield(TRANS zTR, double
fYieldAtCost, long lIssueDate, long lMaturityDate, double fRedemptionPrice,
                                                                                                                                                                                                                 double fTradUnit, double fGainLoss, char *sPrimaryType, char *sSecondaryType, double *pfMarketDiscount)
{
  long		lDaysHeld, lBondLife, lYearsToMaturity, lTempDate1, lTempDate2;
        double	fDiscount, fPrice;
        short		mdy[3];

        *pfMarketDiscount = 0;

        // No market discount, if no gain
        if (fGainLoss <= 0)
                return 0;

        // Cannot calculate market discount, if no valid yield
        if (fYieldAtCost <= 0)
                return 0;

        // If invalid issue date, default it to purchase date
        if (lIssueDate <= 0)
                lIssueDate = zTR.lOpenTrdDate;

        // No market discount for equity mutual funds
        if (*sPrimaryType == PTYPE_BOND && *sSecondaryType == STYPE_MFUND)
                return 0;

        // Can not do anything if any of the dates are wrong
        if (lMaturityDate <= 0 || lMaturityDate - lIssueDate <= 0)
                return 0;

  mdy[0] = 7;
  mdy[1] = 19;
  mdy[2] = 1984;
        lpfnrmdyjul(mdy, &lTempDate1);

  mdy[0] = 5;
  mdy[1] = 1;
  mdy[2] = 1993;
        lpfnrmdyjul(mdy, &lTempDate2);

        // No market discount for bonds issued before 7/19/1984 and purchased
before 5/1/1993 if (lIssueDate < lTempDate1 && zTR.lOpenTrdDate < lTempDate2)
                return 0;

        // No market discount for Municipal Bond purchased before 5/1/1999
        if (*sPrimaryType == PTYPE_BOND && *sSecondaryType == STYPE_MUNICIPALS
&& zTR.lOpenTrdDate < lTempDate2) return 0;

        // Calculate number of days this security was held
        lDaysHeld = zTR.lTrdDate - zTR.lOpenTrdDate;
        if (lDaysHeld <= 0)
                return 0;

        // Calculate number of days in the life of the bond
        lBondLife = lMaturityDate - zTR.lOpenTrdDate;
        if (lBondLife <= 0)
                return 0;

        // Calculate what the price should be based on Yield at Cost
  fPrice = lpfnCalculatePrice(fYieldAtCost, zTR.sSecNo, zTR.sWi, "C",
zTR.lEffDate, 2); if (fPrice <= 0) // error calculating correct cost return 0;

        // Calculate discount
        fDiscount = (zTR.fPcplAmt / (zTR.fUnits * fTradUnit)) - fPrice;

        // Market discount applies only if unit discount is greater than 1/4 of
1% of the
        // redemption value * complete years to maturity
        lYearsToMaturity = (int)((lMaturityDate - lIssueDate) / 365);
        //if (fDiscount >= fRedemptionPrice * lYearsToMaturity * 0.0025)
        //{
                *pfMarketDiscount = fGainLoss - (fDiscount * zTR.fUnits *
fTradUnit);
                // Market Discount can not be more that the total gain
                if (*pfMarketDiscount < 0 || *pfMarketDiscount > fGainLoss)
                        *pfMarketDiscount = 0;
        //}

        return 0;
} // CalcMarketDiscountConstantYield*/

BOOL InitializeCalcGainLossLibrary() {
  /* StarsUtils.dll is 32-bit Delphi. We now use OLEDBIO.dll which contains
   * 64-bit versions of date functions. */
  hStarsUtilsDll = NULL;

  // Load OLEDBIO.dll for date functions
  HINSTANCE hOledbIODll = LoadLibrarySafe("OLEDBIO.dll");
  if (hOledbIODll == NULL)
    return FALSE;

  lpfnrmdyjul = (LPFNRMDYJUL)GetProcAddress(hOledbIODll, "rmdyjul");
  if (!lpfnrmdyjul)
    return FALSE;

  lpfnrjulmdy = (LPFNRJULMDY)GetProcAddress(hOledbIODll, "rjulmdy");
  if (!lpfnrjulmdy)
    return FALSE;

  lpfnNewDateFromCurrent =
      (LPFNNEWDATE)GetProcAddress(hOledbIODll, "NewDateFromCurrent");
  if (!lpfnNewDateFromCurrent)
    return FALSE;

  return TRUE;
} // InitializeCalcGainLossLibrary

void FreeCalcGainLossLibrary() {
  FreeLibrary(hStarsUtilsDll);
} // FreeCalcGainLossLibrary
