/*H*
*
*
* FILENAME: CalcFlow.c
*
* DESCRIPTION: This is a library program to calculate the net flow of a
*              transaction. This program is called either by TRANPROC or
*              CALCPERF programs. Tranproc is supposed to call this routine
*              to figure out netflow and fill net flow and performance impact
*              fields on transaction record. If these two fields are filled on
*              transaction record, calcperf need not call this routine but if
*              for some reason, these fields are not filled out, calcperf call
*              this routine to figure them out.
*
* PUBLIC FUNCTION(S): CalculateNetFlow
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Shobhit Barman (Effron Enterprises, Inc.)
*
* 2020-04-17 J# PER-10655 Added CN transactions -mk.
*
*H*/
 
#include "commonheader.h"
#include "assets.h"
#include "trans.h"


DLLAPI ERRSTRUCT STDCALL WINAPI CalculateNetFlow(TRANS zTR, char *sPerfImpact, FLOWCALCSTRUCT *pzNFCS);
DLLAPI ERRSTRUCT STDCALL WINAPI CalculateNetFlow(TRANS zTR, char *sPerfImpact, FLOWCALCSTRUCT *pzNFCS,bool bPerf);
void			FreeCalcFlow();

LPFNPRINTERROR	lpfnPrintError;
LPPRERRSTRUCT		lpprInitializeErrStruct;
LPFN1PCHAR      lpfnSetErrorFileName;

HINSTANCE	TransEngineDll;
BOOL			bCalcFlowInitialized = FALSE;


/*F*
** Main function for the dll
*F*/
BOOL APIENTRY DllMain (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
							break;

		case DLL_PROCESS_DETACH:
							FreeCalcFlow();
							break;

		default : break;
	}

  return TRUE;
} //DllMain

DLLAPI ERRSTRUCT STDCALL WINAPI CalculateNetFlow(TRANS zTR, char *sPerfImpact, FLOWCALCSTRUCT *pzNFCS)
{
	return CalculateNetFlow(zTR, sPerfImpact, pzNFCS, false);
}

 
DLLAPI ERRSTRUCT STDCALL WINAPI CalculateNetFlow(TRANS zTR, char *sPerfImpact, FLOWCALCSTRUCT *pzNFCS,
																						bool bPerf)
{
	ERRSTRUCT zErr;
 
  lpprInitializeErrStruct(&zErr);
 
  /* if perf_impact is not filled-in, return with an error */
  if (sPerfImpact[0] == ' ')
		return(lpfnPrintError("Invalid Performance Impact", zTR.iID, zTR.lTransNo,
												  "T", 49, 0, 0, "CALC NFLOW1", FALSE));
 
  /* Initialize Return Variables */
  pzNFCS->fBPcplSecFlow = pzNFCS->fLPcplSecFlow = pzNFCS->fBIncomeSecFlow = 0;
	pzNFCS->fLIncomeSecFlow = pzNFCS->fBPcplCashFlow = pzNFCS->fLPcplCashFlow = 0;
	pzNFCS->fBAccrualFlow = pzNFCS->fLAccrualFlow = pzNFCS->fBIncomeCashFlow = 0;
	pzNFCS->fLIncomeCashFlow = pzNFCS->fBSecFees = pzNFCS->fLSecFees = 0;
	pzNFCS->fBSecIncome = pzNFCS->fLSecIncome = 0;
 
  /* if perf_impact is "X", there is no performance effect */
  if (strcmp(sPerfImpact, "X") == 0)
		return zErr;
 
  /* if any of the exchange rates is zero, return with an error */
  if (IsValueZero(zTR.fBaseXrate, 12))
		return(lpfnPrintError("Invalid BaseXrate", zTR.iID, zTR.lTransNo, "T", 67, 0, 0, "CALC NFLOW2", FALSE));
 
  if (IsValueZero(zTR.fIncBaseXrate, 12))
		return(lpfnPrintError("Invalid Income BaseXrate", zTR.iID, zTR.lTransNo, "T", 121, 0, 0, "CALC NFLOW3", FALSE));
 
  if (IsValueZero(zTR.fSecBaseXrate, 12))
		return(lpfnPrintError("Invalid Security BaseXrate", zTR.iID, zTR.lTransNo, "T", 122, 0, 0, "CALC NFLOW2", FALSE));
 
  if (IsValueZero(zTR.fAccrBaseXrate, 12))
		return(lpfnPrintError("Invalid Accrual BaseXrate", zTR.iID, zTR.lTransNo, "T", 123, 0, 0, "CALC NFLOW2", FALSE));


  /* contribution */
  if (strcmp(sPerfImpact, "C") == 0)
  {
		if (strcmp(zTR.sSecXtend, "UP") == 0)
			return zErr;
 
    /* calculate the principal security impact */
		pzNFCS->fLPcplSecFlow = zTR.fPcplAmt;

    /* calculate the income security impact */
		if (IsValueZero(zTR.fAccrInt, 6)) // don't want to double count the flow (as incomesecflow and accrual flow)
		  pzNFCS->fLIncomeSecFlow = zTR.fIncomeAmt;

    /* calculate the accrual impact */
    pzNFCS->fLAccrualFlow = zTR.fAccrInt;
 
    /* calculate the income amount */
    pzNFCS->fLSecIncome = -zTR.fAccrInt;
  } // perfimpact - "C"
  /* withdrawls */
  else if (strcmp(sPerfImpact, "W") == 0)
  {
		if (strcmp(zTR.sSecXtend, "UP") == 0)
			return zErr;
 
    /* calculate the principal security impact */
		pzNFCS->fLPcplSecFlow = -zTR.fPcplAmt;

    /* calculate the principal security impact */
		if (IsValueZero(zTR.fAccrInt, 6)) // don't want to double count the flow (as incomesecflow and accrual flow)
			pzNFCS->fLIncomeSecFlow = -zTR.fIncomeAmt;
 
    /* calculate the accrual impact */
    pzNFCS->fLAccrualFlow = -zTR.fAccrInt;
 
    /* calculate the income amount */
    pzNFCS->fLSecIncome = zTR.fAccrInt;
  } // perfimpact - "W"
  /* fees */
  else if (strcmp(sPerfImpact, "F") == 0 || strcmp(sPerfImpact, "N") == 0)
  {
		/* calculate the principal security impact */
    pzNFCS->fLPcplSecFlow = -zTR.fPcplAmt;

    /* calculate the fee amount */
    pzNFCS->fLSecFees = zTR.fPcplAmt;

		// if this transaction is a credit transaction, reverse the effect of all the flows
		if (strcmp(zTR.sDrCr, "CR") == 0)
		{
      pzNFCS->fLPcplSecFlow *= -1.0;
      pzNFCS->fLSecFees *= -1.0;
		}
	} // perfimpact - "F"
  /* purchases(opens) */
  else if (strcmp(sPerfImpact, "O") == 0)
  {
		/* calculate the principal cash impact */
    pzNFCS->fLPcplCashFlow = -zTR.fPcplAmt;
 
    /* calculate the income cash impact */
    pzNFCS->fLIncomeCashFlow = -zTR.fIncomeAmt;
 
    /* calculate the income amount */
    pzNFCS->fLSecIncome = -zTR.fAccrInt;
 
    if (strcmp(zTR.sSecXtend, "UP") != 0)
    {
			// calculate the principal security impact 
      pzNFCS->fLPcplSecFlow = zTR.fPcplAmt;
 
			// calculate the income security impact 
//      pzNFCS->fLIncomeSecFlow = zTR.fIncomeAmt;
 
      /* calculate the accrual impact */
      pzNFCS->fLAccrualFlow = zTR.fAccrInt;
    }

		// if this transaction is a credit transaction, reverse the effect of all the flows
		if (strcmp(zTR.sDrCr, "CR") == 0)
		{
      pzNFCS->fLPcplCashFlow *= -1.0;
			pzNFCS->fLIncomeCashFlow *= -1.0;
      pzNFCS->fLPcplSecFlow *= -1.0;
      pzNFCS->fLIncomeSecFlow *= -1.0;
      pzNFCS->fLAccrualFlow *= -1.0;
      pzNFCS->fLSecIncome *= -1.0;
		}
  } // perfimpact - "O"
  /* sales */
  else if (strcmp(sPerfImpact, "S") == 0)
  {
		/* calculate the principal cash impact */
    pzNFCS->fLPcplCashFlow = zTR.fPcplAmt;
 
    /* calculate the income cash impact */
    pzNFCS->fLIncomeCashFlow = zTR.fIncomeAmt;
 
    /* calculate the income amount */
    pzNFCS->fLSecIncome = zTR.fAccrInt;
 
    if (strcmp(zTR.sSecXtend, "UP") != 0)
    {
			// calculate the security impact 
      pzNFCS->fLPcplSecFlow = -zTR.fPcplAmt;
 
			// calculate the income security impact 
 //     pzNFCS->fLIncomeSecFlow = -zTR.fIncomeAmt;
 
      /* calculate the accrual impact */
      pzNFCS->fLAccrualFlow = -zTR.fAccrInt;
    } /* if not unsupervised */

		// if this transaction is a debit transaction, reverse the effect of all the flows
		if (strcmp(zTR.sDrCr, "DR") == 0)
		{
      pzNFCS->fLPcplCashFlow *= -1.0;
			pzNFCS->fLIncomeCashFlow *= -1.0;
      pzNFCS->fLPcplSecFlow *= -1.0;
      pzNFCS->fLIncomeSecFlow *= -1.0;
      pzNFCS->fLAccrualFlow *= -1.0;
      pzNFCS->fLSecIncome *= -1.0;
		}
  } // perfimpact - "S"
  /* income */
  else if (strcmp(sPerfImpact, "I") == 0)
  {
		/* calculate the principal cash impact */
    pzNFCS->fLPcplCashFlow = zTR.fPcplAmt;
 
    /* calculate the income cash impact */
    pzNFCS->fLIncomeCashFlow = zTR.fIncomeAmt;
 
    if (strcmp(zTR.sSecXtend, "UP") != 0)
    {
			/* calculate the principal security impact */
      pzNFCS->fLPcplSecFlow = -zTR.fPcplAmt;
			
			/* calculate the income security impact */
      pzNFCS->fLIncomeSecFlow = -zTR.fIncomeAmt;
 
      pzNFCS->fLAccrualFlow = zTR.fAccrInt;
 
			if (zTR.fIncomeAmt != 0.0)
				pzNFCS->fLSecIncome = zTR.fIncomeAmt;
			else
        pzNFCS->fLSecIncome = zTR.fPcplAmt;
    } /* if not unsupervised */

		// if this transaction is a debit transaction, reverse the effect of all the flows
		if (strcmp(zTR.sDrCr, "DR") == 0)
		{
      pzNFCS->fLPcplCashFlow *= -1.0;
			pzNFCS->fLIncomeCashFlow *= -1.0;
      pzNFCS->fLPcplSecFlow *= -1.0;
      pzNFCS->fLIncomeSecFlow *= -1.0;
      pzNFCS->fLAccrualFlow *= -1.0;
      pzNFCS->fLSecIncome *= -1.0;
		}
  } // perfimpact - "I"
  /* special impact for withholding transaction */
  else if (strcmp(sPerfImpact, "T") == 0)
  {
		if (strcmp(zTR.sSecXtend, "UP") == 0)
			return zErr;
 
    /* calculate the principal security impact */
    pzNFCS->fLPcplSecFlow = zTR.fPcplAmt;

    /* calculate the income security impact */
    pzNFCS->fLIncomeSecFlow = zTR.fIncomeAmt;

		// if this transaction is a debit transaction, reverse the effect of all the flows
		if (strcmp(zTR.sDrCr, "DR") == 0)
		{
      pzNFCS->fLPcplSecFlow *= -1.0;
      pzNFCS->fLIncomeSecFlow *= -1.0;
		}
  } // perfimpact - "T"
  /* special impact for amortization/accretion transaction */
  else if (strcmp(sPerfImpact, "A") == 0) 
  {
		// if call is not made from Performance DLL (after-tax amort)
		// then no flow should be calculate (i.e. act like PerfImpact = X)
		if (!bPerf)
			return zErr;

		if (strcmp(zTR.sSecXtend, "UP") == 0)
			return zErr;
 
    /* calculate the principal security impact */
    pzNFCS->fLPcplSecFlow = -zTR.fPcplAmt;

    /* calculate the income security impact */
    pzNFCS->fLIncomeSecFlow = -zTR.fIncomeAmt;

		// if this transaction is a debit transaction, reverse the effect of all the flows
		if (strcmp(zTR.sDrCr, "DR") == 0)
		{
      pzNFCS->fLPcplSecFlow *= -1.0;
      pzNFCS->fLIncomeSecFlow *= -1.0;
		}
  } // perfimpact - "A"
  else /* unknown performance impact */
		return(lpfnPrintError("Invalid Performance Impact", zTR.iID, zTR.lTransNo,
		                      "T", 49, 0, 0, "CALC NFLOW3", FALSE));

	pzNFCS->fBPcplSecFlow = pzNFCS->fLPcplSecFlow / zTR.fBaseXrate;
	pzNFCS->fBIncomeSecFlow = pzNFCS->fLIncomeSecFlow / zTR.fBaseXrate;
  pzNFCS->fBAccrualFlow = pzNFCS->fLAccrualFlow / zTR.fAccrBaseXrate;;
  pzNFCS->fBPcplCashFlow = pzNFCS->fLPcplCashFlow / zTR.fBaseXrate;
  pzNFCS->fBIncomeCashFlow = pzNFCS->fLIncomeCashFlow / zTR.fIncBaseXrate;
  pzNFCS->fBSecIncome = pzNFCS->fLSecIncome / zTR.fIncBaseXrate;
  pzNFCS->fBSecFees = pzNFCS->fLSecFees / zTR.fBaseXrate;	
   
  return zErr;
} // CalculateNetFlow
 
 

DLLAPI int STDCALL WINAPI InitCalcFlow(char *sErrFile)
{

	if (!bCalcFlowInitialized)
	{
		TransEngineDll = LoadLibrarySafe("TransEngine.dll");
		if (TransEngineDll == NULL)
			return (GetLastError());

		lpfnSetErrorFileName = (LPFN1PCHAR)GetProcAddress(TransEngineDll,"SetErrorFileName");
		if (!lpfnSetErrorFileName)
			return (GetLastError());
		lpfnSetErrorFileName(sErrFile);
   
		lpfnPrintError = (LPFNPRINTERROR)GetProcAddress(TransEngineDll, "PrintError");
		if (!lpfnPrintError)
			return (GetLastError());

		lpprInitializeErrStruct =	(LPPRERRSTRUCT)GetProcAddress(TransEngineDll, "InitializeErrStruct");
		if (!lpprInitializeErrStruct)
			return (GetLastError());

		bCalcFlowInitialized = TRUE;
	}

	return 0;
} // InitCalcFlow

void FreeCalcFlow()
{
	if (bCalcFlowInitialized)
	{
		FreeLibrary(TransEngineDll);
		bCalcFlowInitialized = FALSE;
	}
} //FreeCalcFlow