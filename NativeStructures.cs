using System.Runtime.InteropServices;

namespace PerformerDLL.Interop.Common;

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
    /// Checks if the operation was successful (no SQL error).
    /// </summary>
    public readonly bool IsSuccess => iSqlError == 0;

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
/// Portfolio main record structure.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct PORTMAIN
{
    public int iID;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sAcctNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 61)]
    public string sAcctName;
    
    public int lLastTransNo;
    public int lLastRollDate;
    public int lValuationDate;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 4)]
    public string sAcctType;
    
    public int iManagerID;
    public int iCustodianID;
}

/// <summary>
/// Holdings record structure.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct HOLDINGS
{
    public int iID;
    public int lTaxlotNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 13)]
    public string sSecNo;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sWi;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 3)]
    public string sSecXtend;
    
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 2)]
    public string sAcctType;
    
    public int iSecID;
    public double fUnits;
    public double fOrigFace;
    public double fUnitCost;
    public double fTotCost;
    public int lOpenDate;
    public int lCloseDate;
}
