/*H*
*
* SUB-SYSTEM: performance
*
* FILENAME: taxperf.h
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
*
*H*/
 
typedef struct
{
	long   iPortfolioID;
	int    iID;
	long	 lPerformDate;
  double fFedinctaxWthld;
  double fCumFedinctaxWthld;
  double fWtdFedinctaxWthld;

/* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double fStinctaxWthld;
  double fCumStinctaxWthld;
  double fWtdStinctaxWthld;
*/
  double fFedtaxRclm;
  double fCumFedtaxRclm;
  double fWtdFedtaxRclm;

/* these fields (state tax related) are not in use anymore - 5/11/06 vay
	double fSttaxRclm;
  double fCumSttaxRclm;
  double fWtdSttaxRclm;
*/
  double fFedetaxInc;
  double fCumFedetaxInc;
  double fWtdFedetaxInc;
  double fFedataxInc;
	double fCumFedataxInc;
  double fWtdFedataxInc;

/* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double fStetaxInc;
  double fCumStetaxInc;
  double fWtdStetaxInc;
  double fStataxInc;
  double fCumStataxInc;
  double fWtdStataxInc;
*/
  double fFedetaxStrgl;
  double fCumFedetaxStrgl;
  double fWtdFedetaxStrgl;
  double fFedetaxLtrgl;
  double fCumFedetaxLtrgl;
  double fWtdFedetaxLtrgl;
  double fFedetaxCrrgl;
  double fCumFedetaxCrrgl;
  double fWtdFedetaxCrrgl;
  double fFedataxStrgl;
  double fCumFedataxStrgl;
  double fWtdFedataxStrgl;
  double fFedataxLtrgl;
  double fCumFedataxLtrgl;
  double fWtdFedataxLtrgl;
  double fFedataxCrrgl;
  double fCumFedataxCrrgl;
  double fWtdFedataxCrrgl;

/* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double fStetaxStrgl;
  double fCumStetaxStrgl;
  double fWtdStetaxStrgl;
  double fStetaxLtrgl;
  double fCumStetaxLtrgl;
  double fWtdStetaxLtrgl;
  double fStetaxCrrgl;
  double fCumStetaxCrrgl;
  double fWtdStetaxCrrgl;
  double fStataxStrgl;
  double fCumStataxStrgl;
  double fWtdStataxStrgl;
  double fStataxLtrgl;
  double fCumStataxLtrgl;
  double fWtdStataxLtrgl;
  double fStataxCrrgl;
  double fCumStataxCrrgl;
  double fWtdStataxCrrgl;
*/
  double fFedataxAccrInc;
  double fFedataxAccrDiv;
  double fFedataxIncRclm;
  double fFedataxDivRclm;
  double fFedetaxAccrInc;
  double fFedetaxAccrDiv;
  double fFedetaxIncRclm;
  double fFedetaxDivRclm;

/* these fields (state tax related) are not in use anymore - 5/11/06 vay
  double fStataxAccrInc;
  double fStataxAccrDiv;
  double fStataxIncRclm;
  double fStataxDivRclm;
  double fStetaxAccrInc;
  double fStetaxAccrDiv;
  double fStetaxIncRclm;
  double fStetaxDivRclm;
*/
  double fExchRateBase;
  double fExchRateSys;

  double fFedataxAmort;
  double fCumFedataxAmort;
  double fWtdFedataxAmort;

  double fFedetaxAmort;
  double fCumFedetaxAmort;
  double fWtdFedetaxAmort;
} TAXPERF;


typedef void  (CALLBACK* LPPRTAXPERF)(TAXPERF, ERRSTRUCT *);
typedef void  (CALLBACK* LPPRSELECTTAXPERF)(int, int, TAXPERF *,  ERRSTRUCT *);
