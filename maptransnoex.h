/*H*
* 
* FILENAME: maptransnoex.h
* 
* DESCRIPTION: Defines structures to access MapTransNoEx,
*				TradeExchange, TaxlotRecon tables
*				in SQL Server Database
* 
* PUBLIC FUNCTION(S): 
* 
* NOTES: 
* 
* USAGE: As a part of OLEDBIO.DLL project
* 
* AUTHOR: Valeriy Yegorov (C) 2002 Effron Enterprises Inc.
*
*H*/
#ifndef MAPTRANSNOEX_H
#define MAPTRANSNOEX_H


#ifndef NT
  #define NT 1
#endif

typedef struct
{
    int		iVendorID;
    int		iRuleNo;
    long	lPortId;
    long	lTransNo;
    long	lDtransNo;
    char	sField1[30+NT];
    char	sField2[30+NT];
    char	sField3[30+NT];
    char	sField4[30+NT];
    long	lSequenceNo;
    char	sRevField1[30+NT];
    char	sRevField2[30+NT];
    char	sRevField3[30+NT];
    char	sRevField4[30+NT];
    long	lRevSequenceNo;

} MAPTRANSNOEX;

typedef struct
{
	char	sAccountID[20+NT];
	long	lTransNo;
	int		iVendorID;
	long	lFileDate;
	int		iFileNo;
	double	fTimeRcvd;
	char	sAssetID[12+NT];
	char	sTransCode[1+NT];
	long	lTrdDate;
	long	lStlDate;
	int		iRecordType;
	long	lOrigTransNo;
	long	lMasterTicketNo;
	double	fUnits;
	char	sUnitCode[1+NT];
	double	fPcplAmt;
	char	sIncPcplCode[1+NT];
	double	fPrice;
	double	fNetComm;
	char	sCommCode[1+NT];
	double	fSecFees;
	double	fAccrInt;
	double	fOrigFace;
	double	fMortFactor;
	double	fExRate;
	char	sStlLocCode[2+NT];
	char	sWriteDownMtd[1+NT];
	char	sTaxlotNo[10+NT];
	char	sTradeType[1+NT];
	char	sBroker[6+NT];
	char	sDesc1[40+NT];
	char	sDesc2[40+NT];
	char	sDesc3[40+NT];
	char	sDesc4[40+NT];
	long	lSequenceNo;
	int		iOutVendorID;
	int		iOutFileNo;
	long	lEffronTransNo;
	double	fTimeSent;

} TRADEEXCHANGE;

typedef struct
{
    int		iVendorID;
	char	sAccountID[60+NT];
	long	lFileDate;
	char	sSecurityID[60+NT];
	char	sPositionID[20+NT];
	double	fUnits;
	double	fBaseMV;
	double	fMVXrate;
	double	fBaseInc;
	double	fIncXrate;
	double	fBaseOrigCost;
	double	fBaseTotCost;
	double	fBaseCostXrate;
	double	fOrigFace;
	double	fPrice;
	long	lTrdDate;
	long	lStlDate;
	char	sRestrictionCode[20+NT];
	int		iIgnoreFields;
	int     ibatchID;

}  TAXLOTRECON;

typedef struct
{
	int		iID;
	long	lStlDate;
	char	sTranType[2+NT];
	char	sDrCr[2+NT];
	char	sSecNo[12+NT];
	int		iDuplicateCounter;
	char	sBankID[20+NT];
	char	sCusip[10+NT];
	long	lTrdDate;
	char	sDescription[50+NT];
	double	fUnits;
	double	fCommission;
	double	fSecFees;
	double	fShortGain;
	double	fLongGain;
	char	sBroker[20+NT];
	double	fCost;
	double	fBaseAmount;
	double	fNativeAmount;
	double	fExrate;
	char	sCurrency[4+NT];
	char	sPrincipalIncomeFlag[1+NT];
	long	lEntryDate;
	char	sReversalCode[1+NT];
	long	lReconciliationDate;
	int		iEffronTransNo;

} BNKSET;

typedef struct
{	
    long	lRowID;
    long	lID;
    long	lStlDate;
    char	sTranType[2+NT];
    char	sDrCr[2+NT];
    char	sSecNo[12+NT];
    char	sWi[2+NT];
    int		iDuplicateCounter;
    char	sCusip[10+NT];
    char	sBankId[20+NT];
    char	sAcctName[20+NT];
    double	fUnits;
    double	fBaseAmount;
    double	fNativeAmount;
    double	fOrigFace;
    double	fOrigCost;
    double	fCost;
    double	fIncomeAmt;
    double	fCommission;
    double	fSecFees;
    double	fShortGain;
    double	fLongGain;
    double	fExrate;
    char	sCurrency[4+NT];
    char	sBroker[20+NT];
    long	lTrdDate;
    char	sRestrictionCode[2+NT];
    char	sTransNo[30+NT];
    char	sRevTranType[2+NT];
    char	sRevTransNo[30+NT];
    char	sBalToAdjust[4+NT];
    long	lEntryDate;
    char	sPrincipalIncomeFlag[1+NT];
    long	lReconciliationDate;
    long	lEffronTransNo;
    char	sDescription[50+NT];

} BNKSETEX;

#endif // !MAPTRANSNOEX_H
