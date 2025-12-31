using System.Runtime.InteropServices;

namespace PerformerDLL.Interop.Common;

/// <summary>
/// Error structure returned by most native DLL functions.
/// Maps to ERRSTRUCT in C++ code.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct NativeERRSTRUCT
{
    public int iID;
    public int lRecNo;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sRecType;
    public int iBusinessError;
    public int iSqlError;
    public int iIsamCode;

    public readonly bool IsSuccess => iSqlError == 0 && iBusinessError == 0;
    public readonly string FormatError() => $"Native Error (ID={iID}, SQL={iSqlError}, Business={iBusinessError})";

    public ERRSTRUCT ToLegacy()
    {
        return new ERRSTRUCT
        {
            iSqlError = iSqlError,
            iErrorCode = iBusinessError,
            sErrorMessage = $"Native Error: ID={iID}, RecNo={lRecNo}, Type={sRecType}, Isam={iIsamCode}",
            sFunctionName = "NativeCall",
            iLineNumber = 0
        };
    }
}

/// <summary>
/// Error structure returned by most native DLL functions.
/// Maps to ERRSTRUCT in C++ code.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct ERRSTRUCT
{
    public int iSqlError;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
    public string sErrorMessage;
    
    public int iErrorCode;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
    public string sFunctionName;
    
    public int iLineNumber;

    /// <summary>
    /// Checks if the operation was successful (no SQL error and no business error).
    /// </summary>
    public readonly bool IsSuccess => iSqlError == 0 && iErrorCode == 0;

    /// <summary>
    /// Gets a formatted error message.
    /// </summary>
    public readonly string FormatError()
    {
        if (IsSuccess)
            return "Success";
        
        return $"Error in {sFunctionName} (Line {iLineNumber}): [{iSqlError}] {sErrorMessage} (Code: {iErrorCode})";
    }
}

/// <summary>
/// Transaction record structure. Maps to TRANS struct in trans.h.
/// Contains all fields for financial transactions.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct TRANS
{
    public int iID;
    public int lTransNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sTranType;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sSecNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sWi;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sSecXtend;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sAcctType;
    
    public int iSecID;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sSecSymbol;
    
    public double fUnits;
    public double fOrigFace;
    public double fUnitCost;
    public double fTotCost;
    public double fOrigCost;
    public double fPcplAmt;
    public double fOptPrem;
    public double fAmortVal;
    public double fBasisAdj;
    public double fCommGcr;
    public double fNetComm;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 6)]
    public string sCommCode;
    
    public double fSecFees;
    public double fMiscFee1;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 6)]
    public string sFeeCode1;
    
    public double fMiscFee2;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 6)]
    public string sFeeCode2;
    
    public double fAccrInt;
    public double fIncomeAmt;
    public double fNetFlow;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sBrokerCode;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sBrokerCode2;
    
    public int lTrdDate;
    public int lStlDate;
    public int lEffDate;
    public int lEntryDate;
    public int lTaxlotNo;
    public int lXrefTransNo;
    public int lPendDivNo;
    public int lRevTransNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sRevType;
    
    public int lNewTransNo;
    public int lOrigTransNo;
    public int lBlockTransNo;
    public int iXID;
    public int lXTransNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sXSecNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sXWi;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sXSecXtend;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sXAcctType;
    
    public int iXSecID;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sCurrId;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sCurrAcctType;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sIncCurrId;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sIncAcctType;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sXCurrId;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sXCurrAcctType;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sSecCurrId;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sAccrCurrId;
    
    public double fBaseXrate;
    public double fIncBaseXrate;
    public double fSecBaseXrate;
    public double fAccrBaseXrate;
    public double fSysXrate;
    public double fIncSysXrate;
    public double fBaseOpenXrate;
    public double fSysOpenXrate;
    public int lOpenTrdDate;
    public int lOpenStlDate;
    public double fOpenUnitCost;
    public double fOrigYld;
    public int lEffMatDate;
    public double fEffMatPrice;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sAcctMthd;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 9)]
    public string sTransSrce;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 9)]
    public string sAdpTag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sDivType;
    
    public double fDivFactor;
    public int lDivintNo;
    public int lRollDate;
    public int lPerfDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sMiscDescInd;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sDrCr;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sBalToAdjust;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sCapTrans;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sSafekInd;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sDtcInclusion;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sDtcResolve;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sReconFlag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sReconSrce;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sIncomeFlag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sLetterFlag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sLedgerFlag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sGlFlag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 31)]
    public string sCreatedBy;
    
    public int lCreateDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 9)]
    public string sCreateTime;
    
    public int lPostDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sBkofFrmt;
    
    public int lBkofSeqNo;
    public int lDtransNo;
    public double fPrice;
    public int iRestrictionCode;
}

/// <summary>
/// Payment transaction structure. Maps to PAYTRAN struct in trans.h.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct NativePAYTRAN
{
    public int iID;
    public int lTransNo;
    public int lPayeeID;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
    public string sDsc;
}

/// <summary>
/// Portfolio main record structure.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct PORTMAIN
{
    public int iID;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 21)]
    public string sUniqueName;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 21)]
    public string sAbbrev;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 61)]
    public string sDescription;
    public int lDateHired;
    public double fIndividualMinAnnualFee;
    public double fIndividualMinAcctSize;
    public double fTotalAssetsManaged;
    public int iInvestmentStyle;
    public int iScope;
    public int iDecisionMaking;
    public int iDefaultReturnType;
    public int iProductType;
    public double fExpenseRatio;
    public int iMarketCap;
    public int iMaturity;
    public int lAsofDate;
    public short iFiscalYearEndMonth;
    public short iFiscalYearEndDay;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 21)]
    public string sPeriodType;
    public int lInceptionDate;
    public bool bUserInceptionDate;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sPortfolioType;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sAdministrator;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sManager;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 41)]
    public string sAddress1;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 41)]
    public string sAddress2;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 41)]
    public string sAddress3;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sAcctMethod;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sTax;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 5)]
    public string sBaseCurrId;
    public bool bIncome;
    public bool bActions;
    public bool bMature;
    public bool bCAvail;
    public bool bFAvail;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 9)]
    public string sAlloc;
    public double fMaxEqPct;
    public double fMaxFiPct;
    public double fMinCashPct;
    public int iEqLotSize;
    public int iFiLotSize;
    public int lValDate;
    public int lDeleteDate;
    public bool bIsInactive;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sCurrHandler;
    public bool bAmortMuni;
    public bool bAmortOther;
    public bool bAccreteDisc;
    public int lAmortStartDate; // Note: lAmortStartDate is long (int in C#)
    public bool bAccretMuni;
    public bool bAccretOther;
    public bool bIncByLot;
    public bool bDiscretionaryAuthority;
    public bool bVotingAuthority;
    public bool bSpecialArrangements;
    public int iIncomeMoneyMarketFund;
    public int iPrincipalMoneyMarketFund;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sIncomeProcessing;
    public int lPricingEffectiveDate;
    public int lLastTransNo;
    public int lPurgeDate;
    public int lLastActivity;
    public int lRollDate;
    public int iVendorID;
    public bool bIsMarketIndex;
}

/// <summary>
/// Holdings record structure.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct HOLDINGS
{
    public int iID;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sSecNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sWi;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sSecXtend;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sAcctType;
    
    public int lTransNo;
    public int iSecID;
    public int lAsofDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sSecSymbol;
    
    public double fUnits;
    public double fOrigFace;
    public double fTotCost;
    public double fUnitCost;
    public double fOrigCost;
    public double fOpenLiability;
    public double fBaseCostXrate;
    public double fSysCostXrate;
    public int lTrdDate;
    public int lEffDate;
    public int lEligDate;
    public int lStlDate;
    public double fOrigYield;
    public int lEffMatDate;
    public double fEffMatPrice;
    public double fCostEffMatYld;
    public int lAmortStartDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sOrigTransType;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sOrigTransSrce;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sLastTransType;
    
    public int lLastTransNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sLastTransSrce;
    
    public int lLastPmtDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sLastPmtType;
    
    public int lLastPmtTrNo;
    public int lNextPmtDate;
    public double fNextPmtAmt;
    public int lLastPdnDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sLtStInd;
    
    public double fMktVal;
    public double fCurLiability;
    public double fMvBaseXrate;
    public double fMvSysXrate;
    public double fAccrInt;
    public double fAiBaseXrate;
    public double fAiSysXrate;
    public double fAnnualIncome;
    public double fAccrualGl;
    public double fCurrencyGl;
    public double fSecurityGl;
    public double fMktEffMatYld;
    public double fMktCurYld;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sSafekInd;
    
    public double fCollateralUnits;
    public double fHedgeValue;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sBenchmarkSecNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sPermLtFlag;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sValuationSrce;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sPrimaryType;
    
    public int iRestrictionCode;
}
