/*H*
* 
* FILENAME: portbal.h
* 
* DESCRIPTION: 
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE:
* 
* AUTHOR: 
*
*H*/

#ifndef NT
  #define NT 1
#endif

typedef struct
{
  int		 iID;
  long	 lAsofDate;
  double fYtdTaxInc;
  double fYtdNtaxInc;
  double fYtdGovtInc;
  double fYtdMortInc;
  double fYtdMuniInc;
  double fYtdOtherInc;
  double fYtdDivInc;
  double fYtdDivNtaxInc;
  double fYtdDiscInc;
  double fYtdOrgdiscInc;
  double fYtdShortInc;
  double fYtdShortNtaxInc;
  double fWithldInc;
  double fReclaims;
  double fAccrInc;
  double fIhDiv;
  double fOhDiv;
  double fMarginInt;
  double fCreditInt;
  double fAnnualInc;
  double fYield;
  double fShortGl;
  double fPyShortGl;
  double fMedGl;
  double fPyMedGl;
  double fLongGl;
  double fPyLongGl;
  double fCurrGl;
  double fPyCurrGl;
  double fSecurityGl;
  double fPySecurityGl;
  double fAccrualGl;
  double fPyAccrualGl;
  double fFiscalEq;
  double fDlyCapChg;
  double fFyCapChg;
  double fCurEquity;
  double fCashAvail;
  double fFundsAvail;
  double fMktValLong;
  double fMktValShort;
  double fCashVal;
  double fMoneymktLval;
  double fBondLval;
  double fEquityLval;
  double fOptionLval;
  double fMfundEqtyLval;
  double fMfundBondLval;
  double fPreferredLval;
  double fCvtBondLval;
  double fTbillLval;
  double fStInstrLval;
  double fMoneymktSval;
  double fBondSval;
  double fEquitySval;
  double fOptionSval;
  double fMfundEqtySval;
  double fMfundBondSval;
  double fPreferredSval;
  double fCvtBondSval;
  double fTbillSval;
  double fStInstrSval;
  double fWiCalc;
  double fMarkToMarket;
  double fLiabilityVal;
  double fNonSupVal;
  double fYtdBasisAdj;
  double fYtdAmortVal;
  double fYtdDivCharge;
  double fYtdIncCharge;
  double fAdvFees;
  double fOpenBalFwd;
} PORTBAL;


typedef void	(CALLBACK* LPPRSELECTPORTBAL)(PORTBAL *, int, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRUPDATEPORTBAL)(PORTBAL, ERRSTRUCT *);

