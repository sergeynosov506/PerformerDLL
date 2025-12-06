/**
* 
* SUB-SYSTEM: UpdateHold  
* 
* FILENAME: updhOLD2.C
* 
* DESCRIPTION: An additional file for UpdateHold library function.
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: See UpdateHold.c for more information.
* 
* USAGE:
* 
* AUTHOR: Shobhit Barman (EFFRON ENTERPRISES, INC.)
*
* $Header:   J:/Performer/Data/archives/Master/C/UPDHOLD2.C-arc   1.29   21 Mar 2005 12:28:52   GriffinJ  $
*
**/


#include "transengine.h"
#include <time.h>

/** 
** Function to initialize Error structure defined in holdings.h file 
**/
DLLAPI void STDCALL WINAPI InitializeErrStruct(ERRSTRUCT *pzErr)
{
  pzErr->iID = 0;
  pzErr->lRecNo = 0;
  strcpy_s(pzErr->sRecType, " ");
  pzErr->iBusinessError = 0;
  pzErr->iSqlError = 0;
  pzErr->iIsamCode = 0;
}

/** 
** Function to initialize Holdings structure defined in holdings.h file 
**/
DLLAPI void STDCALL WINAPI InitializeHoldingsStruct(HOLDINGS *pzHoldings)
{
   pzHoldings->iID = 0;
   strcpy_s(pzHoldings->sSecNo, " ");
   strcpy_s(pzHoldings->sWi, " ");
   strcpy_s(pzHoldings->sSecXtend, " ");
   strcpy_s(pzHoldings->sAcctType, " ");
	 pzHoldings->iSecID = pzHoldings->lTransNo = pzHoldings->lAsofDate = 0;
   strcpy_s(pzHoldings->sSecSymbol, " ");
   pzHoldings->fUnits = pzHoldings->fOrigFace = 0;
   pzHoldings->fTotCost = pzHoldings->fUnitCost = 0;
   pzHoldings->fOrigCost = pzHoldings->fOpenLiability = 0;
   pzHoldings->fBaseCostXrate = pzHoldings->fSysCostXrate = 1;
   pzHoldings->lTrdDate = pzHoldings->lEffDate = 0;
   pzHoldings->lEligDate = pzHoldings->lStlDate = 0;
   pzHoldings->fOrigYield = 0;
   pzHoldings->lEffMatDate = 0;
   pzHoldings->fEffMatPrice = pzHoldings->fCostEffMatYld = 0;
   pzHoldings->lAmortStartDate = 0;
   strcpy_s(pzHoldings->sOrigTransType, " ");
   strcpy_s(pzHoldings->sOrigTransSrce, " ");
   strcpy_s(pzHoldings->sLastTransType, " ");
   pzHoldings->lLastTransNo = 0;
   strcpy_s(pzHoldings->sLastTransSrce, " ");
   pzHoldings->lLastPmtDate = 0;
   strcpy_s(pzHoldings->sLastPmtType, " ");
   pzHoldings->lLastPmtTrNo = 0;
   pzHoldings->lNextPmtDate = 0;
   pzHoldings->fNextPmtAmt = 0;
   pzHoldings->lLastPdnDate = 0;
   strcpy_s(pzHoldings->sLtStInd, " ");
   pzHoldings->fMktVal = pzHoldings->fCurLiability = 0;
   pzHoldings->fMvBaseXrate = pzHoldings->fMvSysXrate = 1;
   pzHoldings->fAccrInt = 0;
   pzHoldings->fAiBaseXrate = pzHoldings->fAiSysXrate = 1;
   pzHoldings->fAnnualIncome = pzHoldings->fAccrualGl = 0;
   pzHoldings->fCurrencyGl = pzHoldings->fSecurityGl = 0;
   pzHoldings->fMktEffMatYld = pzHoldings->fMktCurYld = 0;
   strcpy_s(pzHoldings->sSafekInd, " ");
   pzHoldings->fCollateralUnits = pzHoldings->fHedgeValue = 0;
   strcpy_s(pzHoldings->sBenchmarkSecNo, " ");
   strcpy_s(pzHoldings->sPermLtFlag, " ");
   strcpy_s(pzHoldings->sValuationSrce, " ");
   pzHoldings->iRestrictionCode = 0;
} /* initholdings */


/** 
** Function to initialize Holdcash structure defined in holdcash.h file 
**/
DLLAPI void STDCALL WINAPI InitializeHoldcashStruct(HOLDCASH *pzHoldcash)
{
   pzHoldcash->iID = 0;
   strcpy_s(pzHoldcash->sSecNo, " ");
   strcpy_s(pzHoldcash->sWi, " ");
   strcpy_s(pzHoldcash->sSecXtend, " ");
   strcpy_s(pzHoldcash->sAcctType, " ");
   pzHoldcash->lAsofDate = 0;
	 pzHoldcash->iSecID = 0;
   strcpy_s(pzHoldcash->sSecSymbol, " ");
   pzHoldcash->fUnits = pzHoldcash->fTotCost = pzHoldcash->fUnitCost = 0;
   pzHoldcash->fBaseCostXrate = pzHoldcash->fSysCostXrate =1;
   pzHoldcash->lTrdDate = pzHoldcash->lEffDate = 0;
   pzHoldcash->lStlDate = pzHoldcash->lLastTransNo = 0;
   pzHoldcash->fMktVal = 0;
   pzHoldcash->fMvBaseXrate = pzHoldcash->fMvSysXrate = 1;
   pzHoldcash->fCurrencyGl = pzHoldcash->fCollateralUnits = pzHoldcash->fHedgeValue = 0;
} /* initholdcash */


/** 
** Function to initialize Trans structure defined in trans.h file 
**/
DLLAPI void STDCALL WINAPI InitializeTransStruct( TRANS *pzTrans )
{
   pzTrans->iID = 0;
   pzTrans->lTransNo = 0;
   strcpy_s(pzTrans->sTranType, " ");
   strcpy_s(pzTrans->sSecNo, " ");
   strcpy_s(pzTrans->sWi, " ");
   strcpy_s(pzTrans->sSecXtend, " ");
   strcpy_s(pzTrans->sAcctType, " ");
	 pzTrans->iSecID = 0;
   strcpy_s(pzTrans->sSecSymbol, " ");
   pzTrans->fUnits = pzTrans->fOrigFace = pzTrans->fTotCost = 0;
   pzTrans->fUnitCost = pzTrans->fOrigCost = pzTrans->fPcplAmt = 0;
   pzTrans->fOptPrem = pzTrans->fAmortVal = 0;
   pzTrans->fBasisAdj = pzTrans->fCommGcr = pzTrans->fNetComm =0;
   strcpy_s(pzTrans->sCommCode, " ");
   pzTrans->fSecFees = pzTrans->fMiscFee1 = 0;
   strcpy_s(pzTrans->sFeeCode1, " ");
   pzTrans->fMiscFee2 = 0;
   strcpy_s(pzTrans->sFeeCode2, " ");
   pzTrans->fAccrInt = pzTrans->fIncomeAmt = pzTrans->fNetFlow = 0;
   strcpy_s(pzTrans->sBrokerCode, " ");
   strcpy_s(pzTrans->sBrokerCode2, " ");
   pzTrans->lTrdDate = pzTrans->lStlDate = pzTrans->lEffDate = 0;
   pzTrans->lEntryDate = pzTrans->lTaxlotNo = pzTrans->lXrefTransNo = 0;
   pzTrans->lPendDivNo = pzTrans->lRevTransNo = 0;
   strcpy_s(pzTrans->sRevType, " ");
   pzTrans->lNewTransNo = pzTrans->lOrigTransNo = pzTrans->lBlockTransNo = 0;
   pzTrans->sXID = 0;
   pzTrans->lXTransNo = 0;
   strcpy_s(pzTrans->sXSecNo, " ");
   strcpy_s(pzTrans->sXWi, " ");
   strcpy_s(pzTrans->sXSecXtend, " ");
   strcpy_s(pzTrans->sXAcctType, " ");
	 pzTrans->iXSecID = 0;
   strcpy_s(pzTrans->sCurrId, " ");
   strcpy_s(pzTrans->sCurrAcctType, " ");
   strcpy_s(pzTrans->sIncCurrId, " ");
   strcpy_s(pzTrans->sIncAcctType, " ");
   strcpy_s(pzTrans->sXCurrId, " ");
   strcpy_s(pzTrans->sXCurrAcctType," ");
   strcpy_s(pzTrans->sSecCurrId, " ");
   strcpy_s(pzTrans->sAccrCurrId, " ");
   pzTrans->fBaseXrate = pzTrans->fIncBaseXrate = 1;
   pzTrans->fSecBaseXrate = pzTrans->fAccrBaseXrate = 1;
   pzTrans->fSysXrate = pzTrans->fIncSysXrate = 1;
   pzTrans->fBaseOpenXrate = pzTrans->fSysOpenXrate = 1;
   pzTrans->lOpenTrdDate = pzTrans->lOpenStlDate = 0;
   pzTrans->fOpenUnitCost = pzTrans->fOrigYld = 0;
   pzTrans->lEffMatDate = 0;
   pzTrans->fEffMatPrice = 0;
   strcpy_s(pzTrans->sAcctMthd, " ");
   strcpy_s(pzTrans->sTransSrce, " ");
   strcpy_s(pzTrans->sAdpTag, " ");
   strcpy_s(pzTrans->sDivType, " ");
   pzTrans->fDivFactor = 0;
   pzTrans->lDivintNo = pzTrans->lRollDate = pzTrans->lPerfDate = 0;
   strcpy_s(pzTrans->sMiscDescInd, " ");
   strcpy_s(pzTrans->sDrCr, " ");
   strcpy_s(pzTrans->sBalToAdjust, " ");
   strcpy_s(pzTrans->sCapTrans, " ");
   strcpy_s(pzTrans->sSafekInd, " ");
   strcpy_s(pzTrans->sDtcInclusion, " ");
   strcpy_s(pzTrans->sDtcResolve, " ");
   strcpy_s(pzTrans->sReconFlag, " ");
   strcpy_s(pzTrans->sReconSrce, " ");
   strcpy_s(pzTrans->sIncomeFlag, " ");
   strcpy_s(pzTrans->sLetterFlag, " ");
   strcpy_s(pzTrans->sLedgerFlag, " ");
   strcpy_s(pzTrans->sGlFlag, " ");
   strcpy_s(pzTrans->sCreatedBy, " ");
   pzTrans->lCreateDate = 0;
   strcpy_s(pzTrans->sCreateTime, " ");
   pzTrans->lPostDate = 0;
   strcpy_s(pzTrans->sBkofFrmt, " ");
   pzTrans->lBkofSeqNo = pzTrans->lDtransNo = 0;
   pzTrans->fPrice = 0;
   pzTrans->iRestrictionCode = 0;
}/* inittrans */


/** 
** Function to initialize Hedge_Xref structure defined in hedgexref.h file 
**/
void InitializeHedgeXrefStruct( HEDGEXREF *pzHedgeXref)
{
   pzHedgeXref->iID = 0;
   strcpy_s(pzHedgeXref->sSecNo, " ");
   strcpy_s(pzHedgeXref->sWi, " ");
	 pzHedgeXref->iSecID = 0;
   strcpy_s(pzHedgeXref->sSecXtend," ");
   strcpy_s(pzHedgeXref->sAcctType," ");
   strcpy_s(pzHedgeXref->sSecNo2, " ");
   strcpy_s(pzHedgeXref->sWi2, " ");
   strcpy_s(pzHedgeXref->sSecXtend2," ");
   strcpy_s(pzHedgeXref->sAcctType2," ");
	 pzHedgeXref->iSecID2 = 0;
   strcpy_s(pzHedgeXref->sHedgeType," ");
   pzHedgeXref->lAsofDate = pzHedgeXref->lTransNo = pzHedgeXref->lTransNo2 = 0;
   pzHedgeXref->fHedgeUnits = pzHedgeXref->fHedgeUnits2 = pzHedgeXref->fHedgeValBase = 0; 
   pzHedgeXref->fHedgeValNative = pzHedgeXref->fHedgeValSystem =0;
   strcpy_s(pzHedgeXref->sValuationSrce," ");
} /* inithedgexref */


/** 
** Function to initialize Payrec structure defined in payrec.h file 
**/
void InitializePayrecStruct( PAYREC *pzPayrec)
{
   pzPayrec->iID = 0;
   strcpy_s(pzPayrec->sSecNo, " ");
   strcpy_s(pzPayrec->sWi, " ");
   strcpy_s(pzPayrec->sSecXtend, " ");
   strcpy_s(pzPayrec->sAcctType, " ");
	 pzPayrec->iSecID = 0;
   pzPayrec->lAsofDate = pzPayrec->lEffDate = pzPayrec->lTransNo = 0;
   strcpy_s(pzPayrec->sTranType, " ");
   pzPayrec->lDivintNo = 0;
   pzPayrec->fUnits = pzPayrec->fCurVal = 0;
   pzPayrec->fBaseCostXrate = pzPayrec->fSysCostXrate = 1;   
   pzPayrec->fMvBaseXrate = pzPayrec->fMvSysXrate = 1;   
   strcpy_s(pzPayrec->sValuationSrce, " ");
} /* initpayrec */


/** 
** Function to initialize Holddel structure defined in holdings.h file 
**/
void InitializeHolddelStruct( HOLDDEL *pzHolddel )
{
   pzHolddel->iID = 0;
   strcpy_s(pzHolddel->sSecNo, " ");
   strcpy_s(pzHolddel->sWi, " ");
   strcpy_s(pzHolddel->sSecXtend, " ");
   strcpy_s(pzHolddel->sAcctType, " ");
	 pzHolddel->iSecID = 0;
   pzHolddel->lTransNo = pzHolddel->lCreateTransNo = 0;
   pzHolddel->lCreateDate = pzHolddel->lAsofDate = 0;
   strcpy_s(pzHolddel->sSecSymbol, " ");
   pzHolddel->fUnits = pzHolddel->fOrigFace = 0;
   pzHolddel->fTotCost = pzHolddel->fUnitCost = 0;
   pzHolddel->fOrigCost = pzHolddel->fOpenLiability = 0;
   pzHolddel->fBaseCostXrate = pzHolddel->fSysCostXrate = 1;
   pzHolddel->lTrdDate = pzHolddel->lEffDate = 0;
   pzHolddel->lEligDate = pzHolddel->lStlDate = 0;
   pzHolddel->fOrigYield = 0;
   pzHolddel->lEffMatDate = 0;
   pzHolddel->fEffMatPrice = pzHolddel->fCostEffMatYld = 0;
   pzHolddel->lAmortStartDate = 0;
   strcpy_s(pzHolddel->sOrigTransType, " ");
   strcpy_s(pzHolddel->sOrigTransSrce, " ");
   strcpy_s(pzHolddel->sLastTransType, " ");
   pzHolddel->lLastTransNo = 0;
   strcpy_s(pzHolddel->sLastTransSrce, " ");
   pzHolddel->lLastPmtDate = 0;
   strcpy_s(pzHolddel->sLastPmtType, " ");
   pzHolddel->lLastPmtTrNo = pzHolddel->lNextPmtDate = 0;
   pzHolddel->fNextPmtAmt = 0;
   pzHolddel->lLastPdnDate = 0;
   strcpy_s(pzHolddel->sLtStInd, " ");
   pzHolddel->fMktVal = pzHolddel->fCurLiability = 0;
   pzHolddel->fMvBaseXrate = pzHolddel->fMvSysXrate = 1;
   pzHolddel->fAccrInt = 0;
   pzHolddel->fAiBaseXrate = pzHolddel->fAiSysXrate = 1;
   pzHolddel->fAnnualIncome = pzHolddel->fAccrualGl = 0;
   pzHolddel->fCurrencyGl = pzHolddel->fSecurityGl = 0;
   pzHolddel->fMktEffMatYld = pzHolddel->fMktCurYld = 0;
   strcpy_s(pzHolddel->sSafekInd, " ");
   pzHolddel->fCollateralUnits = pzHolddel->fHedgeValue = 0;
   strcpy_s(pzHolddel->sBenchmarkSecNo, " ");
   strcpy_s(pzHolddel->sPermLtFlag, " ");
   strcpy_s(pzHolddel->sValuationSrce, " ");
   pzHolddel->iRestrictionCode = 0;
} /* initholddel */


/**
** This function copies all the common fields from TRANS record to HOLDCASH 
** record. This function is used most of the time to process cash impact of
** the trade.
**/
void CopyFieldsFromTransToHoldcash(HOLDCASH *pzHCR, TRANS zTR)
{
  InitializeHoldcashStruct(pzHCR);

  pzHCR->iID = zTR.iID;
  strcpy_s(pzHCR->sSecNo, zTR.sSecNo);
  strcpy_s(pzHCR->sWi, zTR.sWi);
  strcpy_s(pzHCR->sSecXtend, zTR.sSecXtend);
  strcpy_s(pzHCR->sAcctType, zTR.sAcctType);
  pzHCR->lAsofDate = zTR.lEffDate;
	pzHCR->iSecID = zTR.iSecID;
  strcpy_s(pzHCR->sSecSymbol, zTR.sSecSymbol); 
  pzHCR->fUnits = zTR.fUnits;
  pzHCR->fTotCost = zTR.fTotCost;
  pzHCR->fUnitCost = zTR.fUnitCost;
  pzHCR->fBaseCostXrate = zTR.fSecBaseXrate;
  pzHCR->lTrdDate = zTR.lTrdDate;
  pzHCR->lEffDate = zTR.lEffDate;
  pzHCR->lStlDate = zTR.lStlDate;
  pzHCR->lLastTransNo = zTR.lTransNo;

  /* Calculate the market value of the security       */
  pzHCR->fMktVal = (zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate;
  pzHCR->fMvBaseXrate = zTR.fSecBaseXrate;
 
  /* Calculate the cost and market value system exchange rates */
  if (zTR.fPcplAmt != 0.0)
    pzHCR->fMvSysXrate = zTR.fTotCost / (zTR.fPcplAmt / zTR.fSysXrate);
  else
    pzHCR->fMvSysXrate = 1;

  pzHCR->fSysCostXrate = pzHCR->fMvSysXrate;
} /* copyfromtranstoholdcash */


/**
** This function copies all the common fields from TRANS record to HOLDINGS 
** record. This function is used most of the time when a new record has to be 
** inserted in the HOLDINGS table.
**/
void CopyFieldsFromTransToHoldings(HOLDINGS *pzHR, TRANS zTR)
{
  InitializeHoldingsStruct(pzHR);

  pzHR->iID = zTR.iID;
  strcpy_s(pzHR->sSecNo, zTR.sSecNo);
  strcpy_s(pzHR->sWi, zTR.sWi);
  strcpy_s(pzHR->sSecXtend, zTR.sSecXtend);
  strcpy_s(pzHR->sAcctType, zTR.sAcctType);
  pzHR->lTransNo = zTR.lTaxlotNo;
  pzHR->lAsofDate = zTR.lEffDate;
	pzHR->iSecID = zTR.iSecID;
  strcpy_s(pzHR->sSecSymbol, zTR.sSecSymbol); 
  pzHR->fUnits = zTR.fUnits;
  pzHR->fOrigFace = zTR.fOrigFace;
  pzHR->fTotCost = zTR.fTotCost;
  pzHR->fUnitCost = zTR.fUnitCost;
  pzHR->fOrigCost = zTR.fOrigCost;
  pzHR->fBaseCostXrate = zTR.fSecBaseXrate;
  pzHR->fMvBaseXrate = zTR.fSecBaseXrate;
  pzHR->lTrdDate = zTR.lTrdDate;
  pzHR->lEffDate = zTR.lEffDate;
  pzHR->lStlDate = zTR.lStlDate;
  pzHR->fOrigYield = zTR.fOrigYld;
  pzHR->lEffMatDate = zTR.lEffMatDate;
  pzHR->fEffMatPrice = zTR.fEffMatPrice;
  strcpy_s(pzHR->sOrigTransType, zTR.sTranType);
  strcpy_s(pzHR->sOrigTransSrce, zTR.sTransSrce);
  strcpy_s(pzHR->sLastTransType, zTR.sTranType);
  pzHR->lLastTransNo = zTR.lTransNo;
  strcpy_s(pzHR->sLastTransSrce, zTR.sTransSrce);

  /* Calculate the market value of the security       */
  if (strcmp(zTR.sCurrId, zTR.sSecCurrId) != 0)
  {
		pzHR->fMktVal = (zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate;

    /* Calculate the market value system exchange rate */
    if (!IsValueZero(zTR.fPcplAmt, 2))
      pzHR->fMvSysXrate = zTR.fTotCost / (zTR.fPcplAmt / zTR.fSysXrate);
    else
      pzHR->fMvSysXrate = 1;

    pzHR->fSysCostXrate = pzHR->fMvSysXrate;
  }
  else
  {
    pzHR->fMktVal = zTR.fPcplAmt;
    pzHR->fMvSysXrate = zTR.fSysXrate;
    pzHR->fSysCostXrate = zTR.fSysXrate;
  } 

  pzHR->fAccrInt = zTR.fAccrInt;
  pzHR->fAiBaseXrate = zTR.fAccrBaseXrate;

  /* Calculate the accrued interest system exchange rate */
  if (strcmp(zTR.sIncCurrId, zTR.sAccrCurrId) != 0)
  {
    if (!IsValueZero(zTR.fIncomeAmt, 2))
       pzHR->fAiSysXrate = zTR.fAccrInt / (zTR.fIncomeAmt / zTR.fIncSysXrate);
     else
       pzHR->fAiSysXrate = 1;
  }
  else
    pzHR->fAiSysXrate = zTR.fIncSysXrate;

  strcpy_s(pzHR->sSafekInd, zTR.sSafekInd);

  if (strcmp(zTR.sGlFlag, "L") == 0)
     strcpy_s(pzHR->sPermLtFlag, "Y");
	pzHR->iRestrictionCode = zTR.iRestrictionCode;
} /* copyfromtranstoholdings */


/**
** This function copies all the common fields from HOLDCASH record to HOLDINGS
** record. This function is used for opening and closing transactions against 
** currency positions.            
**/
void CopyFieldsFromHoldcashToHoldings(HOLDINGS *pzHR, HOLDCASH zHCR)
{
  InitializeHoldingsStruct(pzHR);
  pzHR->iID = zHCR.iID;
  strcpy_s(pzHR->sSecNo, zHCR.sSecNo);
  strcpy_s(pzHR->sWi, zHCR.sWi);
  strcpy_s(pzHR->sSecXtend, zHCR.sSecXtend);
  strcpy_s(pzHR->sAcctType, zHCR.sAcctType);
  pzHR->lAsofDate = zHCR.lEffDate;
	pzHR->iSecID = zHCR.iSecID;
  strcpy_s(pzHR->sSecSymbol, zHCR.sSecSymbol); 
  pzHR->fUnits = zHCR.fUnits;
  pzHR->fTotCost = zHCR.fTotCost;
  pzHR->fUnitCost = zHCR.fUnitCost;
  pzHR->fBaseCostXrate = zHCR.fBaseCostXrate;
  pzHR->fSysCostXrate = zHCR.fSysCostXrate;
  pzHR->lTrdDate = zHCR.lTrdDate;
  pzHR->lEffDate = zHCR.lEffDate;
  pzHR->lStlDate = zHCR.lStlDate;
  pzHR->lLastTransNo = zHCR.lLastTransNo;
  pzHR->fMktVal = zHCR.fMktVal;
  pzHR->fMvBaseXrate = zHCR.fMvBaseXrate; 
  pzHR->fMvSysXrate = zHCR.fMvSysXrate;  
  pzHR->fCurrencyGl = zHCR.fCurrencyGl;
  pzHR->fCollateralUnits = zHCR.fCollateralUnits;
  pzHR->fHedgeValue = zHCR.fHedgeValue;
} /* copyfromholdcastoholdings */


/**
** This function copies all the common fields from HOLDINGS record to HOLDCASH 
** record. This function is used most of the time to process cash impact of
** the trade.
**/
void CopyFieldsFromHoldingsToHoldcash(HOLDCASH *pzHCR, HOLDINGS zHR)
{
  InitializeHoldcashStruct(pzHCR);
  pzHCR->iID = zHR.iID;
  strcpy_s(pzHCR->sSecNo, zHR.sSecNo);
  strcpy_s(pzHCR->sWi, zHR.sWi);
  strcpy_s(pzHCR->sSecXtend, zHR.sSecXtend);
  strcpy_s(pzHCR->sAcctType, zHR.sAcctType);
	pzHCR->iSecID = zHR.iSecID;
  pzHCR->lAsofDate = zHR.lEffDate;
  strcpy_s(pzHCR->sSecSymbol, zHR.sSecSymbol); 
  pzHCR->fUnits = zHR.fUnits;
  pzHCR->fTotCost = zHR.fTotCost;
  pzHCR->fUnitCost = zHR.fUnitCost;
  pzHCR->fBaseCostXrate = zHR.fBaseCostXrate;
  pzHCR->fSysCostXrate = zHR.fSysCostXrate;
  pzHCR->lTrdDate = zHR.lTrdDate;
  pzHCR->lEffDate = zHR.lEffDate;
  pzHCR->lStlDate = zHR.lStlDate;
  pzHCR->lLastTransNo = zHR.lLastTransNo;
  pzHCR->fMktVal = zHR.fMktVal; 
  pzHCR->fMvBaseXrate = zHR.fMvBaseXrate;
  pzHCR->fMvSysXrate = zHR.fMvSysXrate;
  pzHCR->fCurrencyGl = zHR.fCurrencyGl;
  pzHCR->fCollateralUnits = zHR.fCollateralUnits;
  pzHCR->fHedgeValue = zHR.fHedgeValue;
} /* copyfromholdingstoholdcash */


/**
** This function is used when calling the gain/loss function to calculate 
** currency and security gain/loss values for the position. This function copies
** only those fields which are required for the calculation and then modifies
** some of the fields, so this function should be used very carefully     
**/
void CopyFieldsFromHoldingsToTrans(TRANS *pzTR, HOLDINGS zHR,char *sPrimaryType)
{
  InitializeTransStruct(pzTR);

  pzTR->iID = zHR.iID;
  strcpy_s(pzTR->sSecNo, zHR.sSecNo);
  strcpy_s(pzTR->sWi, zHR.sWi);
  strcpy_s(pzTR->sSecXtend, zHR.sSecXtend);
  strcpy_s(pzTR->sAcctType, zHR.sAcctType);
	pzTR->iSecID = zHR.iSecID;
  strcpy_s(pzTR->sSecSymbol, zHR.sSecSymbol); 

  pzTR->fUnits = zHR.fUnits;
  pzTR->fOrigFace = zHR.fOrigFace;

  pzTR->fTotCost = zHR.fTotCost;
  pzTR->fUnitCost = zHR.fUnitCost;
  pzTR->fOrigCost = zHR.fOrigCost;

  pzTR->fSecBaseXrate = zHR.fMvBaseXrate;
  pzTR->fBaseXrate = zHR.fMvBaseXrate;
  pzTR->fSysXrate = zHR.fMvSysXrate;
  pzTR->fBaseOpenXrate = zHR.fBaseCostXrate;
  pzTR->fSysOpenXrate = zHR.fSysCostXrate;

	if (strcmp(zHR.sOrigTransType, "FR") == 0 || strcmp(zHR.sOrigTransType, "TS") == 0)
	{
		pzTR->lOpenTrdDate = zHR.lEffDate;
		pzTR->lOpenStlDate = zHR.lEffDate;
	}
	else
	{
		pzTR->lOpenTrdDate = zHR.lTrdDate;
		pzTR->lOpenStlDate = zHR.lStlDate;
	}

  pzTR->fPcplAmt = zHR.fMktVal;
  pzTR->fAccrInt = zHR.fAccrInt;
  pzTR->fIncomeAmt = zHR.fAccrInt;
  pzTR->fIncSysXrate = zHR.fAiSysXrate;

  if (strcmp(zHR.sPermLtFlag, "Y") == 0)
    strcpy_s(pzTR->sGlFlag, "L");

  strcpy_s(pzTR->sTranType, "SL"); /* force trantype to be "SL" */
  strcpy_s(pzTR->sDrCr, "CR"); 

  if (sPrimaryType[0] == 'F')
  {
    pzTR->fPcplAmt = zHR.fCurLiability;
    pzTR->fTotCost = zHR.fOpenLiability;
  }
 
  if (zHR.fUnits < 0)
  {
    strcpy_s(pzTR->sTranType, "CS"); /* force trantype to be "SL" */
    strcpy_s(pzTR->sDrCr, "DR"); 
    pzTR->fUnits *= -1;
    pzTR->fOrigFace *= -1;
    pzTR->fOrigCost *= -1;
    pzTR->fTotCost *= -1;
    pzTR->fPcplAmt *= -1;
    pzTR->fIncomeAmt *= -1;
    pzTR->fAccrInt *= -1;
  }
	pzTR->iRestrictionCode = zHR.iRestrictionCode;
} /* copyfromholdingstotrans */


/**
** This function copies all the common fields from TRANS record to HEDGEXREF
** record. This function is used most of the time to process cash impact of
** the trade.
**/
void CopyFieldsFromTransToHedgeXref(HEDGEXREF *pzHG, TRANS zTR)
{
  InitializeHedgeXrefStruct(pzHG);
  pzHG->iID = zTR.iID;
  strcpy_s(pzHG->sSecNo, zTR.sSecNo);
  strcpy_s(pzHG->sWi, zTR.sWi);
  strcpy_s(pzHG->sSecXtend, zTR.sSecXtend);
  strcpy_s(pzHG->sAcctType, zTR.sAcctType);
  pzHG->iSecID = zTR.iSecID;

  strcpy_s(pzHG->sSecNo2, zTR.sXSecNo);
  strcpy_s(pzHG->sWi2, zTR.sXWi);
  strcpy_s(pzHG->sSecXtend2, zTR.sXSecXtend);
  strcpy_s(pzHG->sAcctType2, zTR.sXAcctType);
	pzHG->iSecID2 = zTR.iXSecID;

  strcpy_s(pzHG->sHedgeType, zTR.sBalToAdjust);
  pzHG->lAsofDate = zTR.lEffDate;
  pzHG->fHedgeValNative = zTR.fPcplAmt;
  /* 
  ** Exchange rates for a transaction can never be zero, therefore
  ** if the program arrives here with a zero exhange rate, it will cause
  ** a memory fault and a programmer must check the function that created
  ** this transaction
  */
  pzHG->fHedgeValBase = zTR.fPcplAmt / zTR.fBaseXrate;
  pzHG->fHedgeValSystem = zTR.fPcplAmt / zTR.fSysXrate;

  pzHG->lTransNo = zTR.lTaxlotNo;
  pzHG->lTransNo2 = zTR.lXTransNo;
} /* copyfromtranstohedgexref */


/**
** This function copies all the common fields from TRANS record to PAYREC   
** record. 
**/
void CopyFieldsFromTransToPayrec(PAYREC *pzPYR, TRANS zTR)
{
  InitializePayrecStruct(pzPYR);
  
  pzPYR->iID = zTR.iID;
  strcpy_s(pzPYR->sSecNo, zTR.sSecNo);
  strcpy_s(pzPYR->sWi, zTR.sWi);
  strcpy_s(pzPYR->sSecXtend, zTR.sSecXtend);
  strcpy_s(pzPYR->sAcctType, zTR.sAcctType);
	pzPYR->iSecID = zTR.iSecID;

  pzPYR->lAsofDate = zTR.lEffDate;
  pzPYR->fBaseCostXrate = zTR.fBaseXrate;
  pzPYR->fMvBaseXrate = zTR.fBaseXrate;
  pzPYR->fSysCostXrate = zTR.fSysXrate;
  pzPYR->fMvSysXrate = zTR.fSysXrate;
  pzPYR->lTransNo = zTR.lTransNo;
  pzPYR->lDivintNo = zTR.lDivintNo;

  if (strcmp(zTR.sTranType, "AR") == 0)
  {
    pzPYR->fUnits = zTR.fPcplAmt;
    pzPYR->fCurVal = zTR.fPcplAmt;
  }
  else
  {
    pzPYR->fUnits = zTR.fUnits;
    pzPYR->fCurVal = (zTR.fPcplAmt / zTR.fBaseXrate) * zTR.fSecBaseXrate;
  }

  if (strcmp(zTR.sTranType, "RR") == 0)
  {
    pzPYR->lEffDate = zTR.lOpenTrdDate;
  /*  strcpy_s(pzPYR->sTranType, "AR");*/
  }
  else
  {
    pzPYR->lEffDate = zTR.lEffDate;
   /* strcpy_s(pzPYR->sTranType, zTR.sTranType);*/
  }
} /* copyfromtranstopayrec */


/**
** This function copies all the columns from the HOLDINGS row to the
** HOLDDEL row.
**/
void CopyFieldsFromHoldingsToHolddel(HOLDDEL *pzHD, HOLDINGS zHR)
{
  InitializeHolddelStruct(pzHD);
  pzHD->iID = zHR.iID;
  strcpy_s(pzHD->sSecNo, zHR.sSecNo);
  strcpy_s(pzHD->sWi, zHR.sWi);
  strcpy_s(pzHD->sSecXtend, zHR.sSecXtend);
  strcpy_s(pzHD->sAcctType, zHR.sAcctType);
  pzHD->lTransNo = zHR.lTransNo;
	pzHD->iSecID = zHR.iSecID;
  pzHD->lAsofDate = zHR.lAsofDate;
  strcpy_s(pzHD->sSecSymbol, zHR.sSecSymbol); 
  pzHD->fUnits = zHR.fUnits;
  pzHD->fOrigFace = zHR.fOrigFace;
  pzHD->fTotCost = zHR.fTotCost;
  pzHD->fUnitCost = zHR.fUnitCost;
  pzHD->fOrigCost = zHR.fOrigCost;
  pzHD->fOpenLiability = zHR.fOpenLiability;
  pzHD->fBaseCostXrate = zHR.fBaseCostXrate;
  pzHD->fSysCostXrate = zHR.fSysCostXrate;
  pzHD->lTrdDate = zHR.lTrdDate;
  pzHD->lEffDate = zHR.lEffDate;
  pzHD->lEligDate = zHR.lEligDate;
  pzHD->lStlDate = zHR.lStlDate;
  pzHD->fOrigYield = zHR.fOrigYield;
  pzHD->lEffMatDate = zHR.lEffMatDate;
  pzHD->fEffMatPrice = zHR.fEffMatPrice;
  pzHD->fCostEffMatYld = zHR.fCostEffMatYld;
  pzHD->lAmortStartDate = zHR.lAmortStartDate;
  strcpy_s(pzHD->sOrigTransType, zHR.sOrigTransType);
  strcpy_s(pzHD->sOrigTransSrce, zHR.sOrigTransSrce);
  strcpy_s(pzHD->sLastTransType, zHR.sLastTransType);
  pzHD->lLastTransNo = zHR.lLastTransNo;
  strcpy_s(pzHD->sLastTransSrce, zHR.sLastTransSrce);
  pzHD->lLastPmtDate = zHR.lLastPmtDate;
  strcpy_s(pzHD->sLastPmtType, zHR.sLastPmtType);
  pzHD->lLastPmtTrNo = zHR.lLastPmtTrNo;
  pzHD->lNextPmtDate = zHR.lNextPmtDate;
  pzHD->fNextPmtAmt = zHR.fNextPmtAmt;
  pzHD->lLastPdnDate =  zHR.lLastPdnDate;
  strcpy_s(pzHD->sLtStInd, zHR.sLtStInd);
  pzHD->fMktVal = zHR.fMktVal;
  pzHD->fCurLiability = zHR.fCurLiability;
  pzHD->fMvBaseXrate = zHR.fMvBaseXrate;
  pzHD->fMvSysXrate = zHR.fMvSysXrate;
  pzHD->fAccrInt = zHR.fAccrInt;
  pzHD->fAiBaseXrate = zHR.fAiBaseXrate;
  pzHD->fAiSysXrate = zHR.fAiSysXrate;
  pzHD->fAnnualIncome = zHR.fAnnualIncome; 
  pzHD->fAccrualGl = zHR.fAccrualGl;
  pzHD->fCurrencyGl = zHR.fCurrencyGl;
  pzHD->fSecurityGl = zHR.fSecurityGl;
  pzHD->fMktEffMatYld = zHR.fMktEffMatYld;
  pzHD->fMktCurYld = zHR.fMktCurYld;
  strcpy_s(pzHD->sSafekInd, zHR.sSafekInd);
  pzHD->fCollateralUnits = zHR.fCollateralUnits;
  pzHD->fHedgeValue = zHR.fHedgeValue;
  strcpy_s(pzHD->sBenchmarkSecNo, zHR.sBenchmarkSecNo);
  strcpy_s(pzHD->sPermLtFlag, zHR.sPermLtFlag);
  strcpy_s(pzHD->sValuationSrce, zHR.sValuationSrce);
  strcpy_s(pzHD->sPrimaryType,zHR.sPrimaryType);
  pzHD->iRestrictionCode = zHR.iRestrictionCode;
} /* copyfromholdingstoholddel */


/**
** This function copies all the columns from the HOLDDEL row to the
** HOLDINGS row.
**/
void CopyFieldsFromHolddelToHoldings(HOLDINGS *pzHR, HOLDDEL zHD)
{
  InitializeHoldingsStruct(pzHR);
  pzHR->iID = zHD.iID;
  strcpy_s(pzHR->sSecNo, zHD.sSecNo);
  strcpy_s(pzHR->sWi, zHD.sWi);
  strcpy_s(pzHR->sSecXtend, zHD.sSecXtend);
  strcpy_s(pzHR->sAcctType, zHD.sAcctType);
  pzHR->lTransNo = zHD.lTransNo;
	pzHR->iSecID = zHD.iSecID;
  pzHR->lAsofDate = zHD.lAsofDate;
  strcpy_s(pzHR->sSecSymbol, zHD.sSecSymbol); 
  pzHR->fUnits = zHD.fUnits;
  pzHR->fOrigFace = zHD.fOrigFace;
  pzHR->fTotCost = zHD.fTotCost;
  pzHR->fUnitCost = zHD.fUnitCost;
  pzHR->fOrigCost = zHD.fOrigCost;
  pzHR->fOpenLiability = zHD.fOpenLiability;
  pzHR->fBaseCostXrate = zHD.fBaseCostXrate;
  pzHR->fSysCostXrate = zHD.fSysCostXrate;
  pzHR->lTrdDate = zHD.lTrdDate;
  pzHR->lEffDate = zHD.lEffDate;
  pzHR->lEligDate = zHD.lEligDate;
  pzHR->lStlDate = zHD.lStlDate;
  pzHR->fOrigYield = zHD.fOrigYield;
  pzHR->lEffMatDate = zHD.lEffMatDate;
  pzHR->fEffMatPrice = zHD.fEffMatPrice;
  pzHR->fCostEffMatYld = zHD.fCostEffMatYld;
  pzHR->lAmortStartDate = zHD.lAmortStartDate;
  strcpy_s(pzHR->sOrigTransType, zHD.sOrigTransType);
  strcpy_s(pzHR->sOrigTransSrce, zHD.sOrigTransSrce);
  strcpy_s(pzHR->sLastTransType, zHD.sLastTransType);
  pzHR->lLastTransNo = zHD.lLastTransNo;
  strcpy_s(pzHR->sLastTransSrce, zHD.sLastTransSrce);
  pzHR->lLastPmtDate = zHD.lLastPmtDate;
  strcpy_s(pzHR->sLastPmtType, zHD.sLastPmtType);
  pzHR->lLastPmtTrNo = zHD.lLastPmtTrNo;
  pzHR->lNextPmtDate = zHD.lNextPmtDate;
  pzHR->fNextPmtAmt = zHD.fNextPmtAmt;
  pzHR->lLastPdnDate =  zHD.lLastPdnDate;
  strcpy_s(pzHR->sLtStInd, zHD.sLtStInd);
  pzHR->fMktVal = zHD.fMktVal;
  pzHR->fCurLiability = zHD.fCurLiability;
  pzHR->fMvBaseXrate = zHD.fMvBaseXrate;
  pzHR->fMvSysXrate = zHD.fMvSysXrate;
  pzHR->fAccrInt = zHD.fAccrInt;
  pzHR->fAiBaseXrate = zHD.fAiBaseXrate;
  pzHR->fAiSysXrate = zHD.fAiSysXrate;
  pzHR->fAnnualIncome = zHD.fAnnualIncome; 
  pzHR->fAccrualGl = zHD.fAccrualGl;
  pzHR->fCurrencyGl = zHD.fCurrencyGl;
  pzHR->fSecurityGl = zHD.fSecurityGl;
  pzHR->fMktEffMatYld = zHD.fMktEffMatYld;
  pzHR->fMktCurYld = zHD.fMktCurYld;
  strcpy_s(pzHR->sSafekInd, zHD.sSafekInd);
  pzHR->fCollateralUnits = zHD.fCollateralUnits;
  pzHR->fHedgeValue = zHD.fHedgeValue;
  strcpy_s(pzHR->sBenchmarkSecNo, zHD.sBenchmarkSecNo);
  strcpy_s(pzHR->sPermLtFlag, zHD.sPermLtFlag);
  strcpy_s(pzHR->sValuationSrce, zHD.sValuationSrce);
  strcpy_s(pzHR->sPrimaryType,zHD.sPrimaryType);
  pzHR->iRestrictionCode = zHD.iRestrictionCode;
  
} /* copyfromholddeltoholdings */


ERRSTRUCT GetSecurityCharacteristics(char *sSecNo, char *sWi, char *sPType, char *sSType, char *sPostnInd, 
																		 char *sLotInd, char *sCostInd, char *sLotExistInd, char *sAvgInd, 
																		 double *pfTrdUnit, char *sCurrId)
{ 
  ERRSTRUCT zErr;
  char      sMsg[80];

  InitializeErrStruct(&zErr);

  /* 
  ** select the primary type(Bond, Equity, etc.), secondary type, position 
  ** indicator(holdcash, holdings, payrec), lot indicator(single lot(cash/money 
  ** market) or multi lot), cost indicator(cost or liability), lot exists 
  ** indicator(whether a lot must exist for trade to be valid or not), average 
  ** indicator(can security be averaged in average cost portfolios, trading 
  ** units and currency id of the security
  */
	lpprSecCharacteristics(sPType, sSType, sPostnInd, sLotInd, sCostInd, sLotExistInd,
												 sAvgInd, pfTrdUnit, sCurrId, sSecNo, sWi, &zErr);
	if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
		return zErr;

  if (IsValueZero(*pfTrdUnit, 3))
  {
    sprintf_s(sMsg, "Invalid Trading Unit For Sec - %s", sSecNo);
    zErr = PrintError(sMsg, 0, 0, "", 21, 0, 0, "UPDH GET SEC", FALSE);
  }

  return zErr;
} /* getsecuritycharacheteristics */


/** 
** This function calculates new CosteXchangeRate, based on the costxrate from
** holdings and the transaction which resulted in the changed costxrate. It 
** needs four inputs which are :
**   TotalHoldingCost(before adding/subtracting the current transaction cost), 
**   TotalTransactionCost, HoldingCostXrate and TransactionCostXrate. 
**   Fifth argument is the output. The Formula is: 
**   NewCostXrate = fNN / fTotV  WHERE
**   NN = HoldCost + TransCost
**  TotV = (HoldCost / CostXrate) + (TransCost / CostXrate)
**  Changed the new cost exchange rate to be a weighted calculation
**/
void CalculateNewCostXRate(double fHoldCost, double fTransCost, double fHoldCXR, 
                           double fTransCXR, double *pzNewXrate)
{
	double fWtdTotal, fTotalCosts; /*fHV, fTV, fTotV, fNN,*/
	
	/* Use the absolute value for the holdcost and the transcost */
	/* SB 10/3/2012 - Idea of using absolute values was wrong. If you use absolute value, you'll get wrong
	**								exchange rate, values should be used as they are. Commented out following code 
	*/   
	/*if (fHoldCost < 0)
		fHoldCost = fHoldCost * -1;

	if (fTransCost < 0)
		fTransCost = fTransCost * -1;*/

	/* if any of the denonimator(HoldCXR,TransCXR,TotV) is zero, make it 1 */
	if (IsValueZero(fHoldCXR, 12))
		fHoldCXR = 1;
    
	//fHV = fHoldCost / fHoldCXR;

	if (IsValueZero(fTransCXR, 12))
		fTransCXR = 1;
    
	//fTV = fTransCost / fTransCXR;

	//fTotV = fHV + fTV;
	//fNN = fHoldCost + fTransCost;
	/* 
	** if numerator or denominator ever become zero, new cost xrate will be 
	** meaningless, in that case just return the average of two exchange rates
	** as the new exchange rate
	*/
	fWtdTotal = ((fTransCXR * fTransCost) + (fHoldCXR * fHoldCost));
	fTotalCosts = (fTransCost + fHoldCost);

	if (IsValueZero(fWtdTotal, 3) || IsValueZero(fTotalCosts, 3))
		*pzNewXrate = (fTransCXR + fHoldCXR) / 2;
	else 
		*pzNewXrate = fWtdTotal / fTotalCosts;
		 
   /* *pzNewXrate = fNN / fTotV; */
	
} /* calculatenewcostxrate */


/**
** This function calculates currency gain/loss as a result of a transaction. 
** It nees four inputs:
** TotalCost, BaseCostXrate, marketValue and MarketValue Base Xrate. 
** The formula for currency gainloss is :
**    currency_gl = (totcost / basecostxrate) - (mktVal / mv basexrate) 
**/
void CalculateCurrencyGainLoss(double fTCost, double fBaseCXR, double fMv, 
                               double fMvBaseCXR, double *pzCurrencyGL)
{
   double  fTempDouble1, fTempDouble2;

/* if any of the denonimator (BaseCXR or MvBaseCXR) is zero, assume, it is 1 */
   if (IsValueZero(fBaseCXR, 12))
     fTempDouble1 = fTCost;
   else
     fTempDouble1 = fTCost / fBaseCXR;

   if (IsValueZero(fMvBaseCXR, 12))
     fTempDouble2 = fMv;
   else
     fTempDouble2 = fMv / fMvBaseCXR;
   
	 *pzCurrencyGL = fTempDouble1 - fTempDouble2;
} /* calculatecurrencygainloss */


DLLAPI int STDCALL WINAPI SetErrorFileName(char *sErrFile)
{
	if (strlen(sErrFile) < 1 || strlen(sErrFile) > 80)
		return(ERR_INVALIDFILENAME);
	else
		strcpy_s(sErrorFile, sErrFile);

	return 0;
} // SetErrorFileName

DLLAPI ERRSTRUCT STDCALL WINAPI PrintError(char *sErrMsg, int iID, long lRecNo, char *sRecType, int iBusinessErr, int iSqlErr, 
																			int iIsamCode, char *sMsgIdentifier, BOOL bWarning)
{
  ERRSTRUCT zErr;
  FILE      *fp;
	char      sRecDesc[50], sTemp[50], sErrWarn[10], sDate[12], sTime[12];
	

  InitializeErrStruct(&zErr);

	zErr.iID = iID;
	zErr.lRecNo = lRecNo;
	if (sRecType) 
		zErr.sRecType[0] =sRecType[0];
	
	if (bWarning == FALSE)
	{
		zErr.iBusinessError = iBusinessErr;
		zErr.iSqlError = iSqlErr;
		zErr.iIsamCode = iIsamCode;
		strcpy_s(sErrWarn, "ERROR");
	}
	else
		strcpy_s(sErrWarn, "WARNING");



   if (iID != 0)
     sprintf_s(sTemp, "Acct - %d|BErr - %d", iID, zErr.iBusinessError);
   else
     sprintf_s(sTemp, "BErr - %d", zErr.iBusinessError);

   if (sRecType[0] == 'D') // DtransNo 
     sprintf_s(sRecDesc, "DtransNo %ld", lRecNo);
   else if (sRecType[0] == 'F') // PerformNo 
     sprintf_s(sRecDesc, "PerformNo %ld", lRecNo);
   else if (sRecType[0] == 'H') // Template HdrNo 
     sprintf_s(sRecDesc, "Template HdrNo %ld", lRecNo);
   else if (sRecType[0] == 'I') // DivintNo 
     sprintf_s(sRecDesc, "DivintNo %ld", lRecNo);
   else if (sRecType[0] == 'L') // TaxlotNo 
     sprintf_s(sRecDesc, "TaxlotNo %ld", lRecNo);
   else if (sRecType[0] == 'P') // perfkey 
     sprintf_s(sRecDesc, "PerfkeyNo %ld", lRecNo);
   else if (sRecType[0] == 'R') // Perfrule 
     sprintf_s(sRecDesc, "PerfruleNo %ld", lRecNo);
   else if (sRecType[0] == 'S') // Script HdrNo 
     sprintf_s(sRecDesc, "Script HdrNo %ld", lRecNo);
   else if (sRecType[0] == 'T') // TransNo 
     sprintf_s(sRecDesc, "TransNo %ld", lRecNo);
   else if (sRecType[0] == 'G') // SegmentID
     sprintf_s(sRecDesc, "SegmentID %ld", lRecNo);
   else
     sprintf_s(sRecDesc, "%s %ld", sRecType, lRecNo);

	// Get current date and time
  _strdate(sDate);
	_strtime(sTime);
  fp = fopen(sErrorFile, "a");
	if (fp != NULL)
	{
		fprintf(fp, "%s  %s  %s|%s|SqlErr - %ld|IsamErr - %ld|%s|ErrMsg - %s|MsgLocation - %s\n",
								sDate, sTime, sErrWarn, sTemp, iSqlErr, iIsamCode, sRecDesc, sErrMsg, sMsgIdentifier);
		fclose(fp);
	}
			     
	return zErr;
} // PrintError
