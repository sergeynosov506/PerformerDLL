using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for OLEDBIO.dll - Data access layer for PerformerDLL system.
/// Contains 252+ exported functions for database operations using nanodbc/ODBC.
/// </summary>
/// <remarks>
/// IMPORTANT: OLEDBIO.dll must be:
/// - x64 architecture (matches this wrapper)
/// - Located in same directory as calling executable OR in PATH
/// - Requires ODBC Driver 17/18 for SQL Server installed
/// 
/// Calling Convention: StdCall (standard for Windows DLLs)
/// String Encoding: ANSI (CharSet.Ansi)
/// </remarks>
public static class OLEDBIO
{
    private const string DLL_NAME = "OLEDBIO.dll";
    private const CallingConvention CALL_CONV = CallingConvention.StdCall;

    #region Initialization and Cleanup

    /// <summary>
    /// Initializes the OLEDBIO library. Must be called before any other OLEDBIO functions.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void InitializeOLEDBIO();

    /// <summary>
    /// Frees resources used by the OLEDBIO library. Call before application exit.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void FreeOLEDBIO();

    /// <summary>
    /// Initializes error structure to default values.
    /// </summary>
    /// <param name="err">Pointer to ERRSTRUCT to initialize</param>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void InitializeErrStruct(ref ERRSTRUCT err);

    #endregion

    #region Database Transaction Control

    /// <summary>
    /// Starts a database transaction.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void StartDBTransaction();

    /// <summary>
    /// Commits the current database transaction.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void CommitDBTransaction();

    /// <summary>
    /// Rolls back the current database transaction.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void RollbackDBTransaction();

    #endregion

    #region Date/Time Functions

    /// <summary>
    /// Gets the current date and time.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void CurrentDateAndTime();

    /// <summary>
    /// Gets the current month end date.
    /// </summary>
    /// <param name="currentDate">Current date as integer YYYYMMDD</param>
    /// <returns>Month end date as integer</returns>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "CurrentMonthEnd")]
    public static extern int CurrentMonthEnd(int currentDate);

    /// <summary>
    /// Gets the last business day before or on the given date.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "LastBusinessDay")]
    public static extern int LastBusinessDay(int date, ref ERRSTRUCT err);

    /// <summary>
    /// Checks if a date is a bank holiday.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "IsItABankHoliday")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsItABankHoliday(int date);

    /// <summary>
    /// Checks if a date is a market holiday.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "IsItAMarketHoliday")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsItAMarketHoliday(int date);

    /// <summary>
    /// Converts month/day/year to Julian date.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "rmdyjul")]
    public static extern int RmdyJul(int mdyDate);

    /// <summary>
    /// Checks if a year is a leap year.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "rleapyear")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool RLeapYear(int year);

    #endregion

    #region Portfolio (Portmain) Operations

    /// <summary>
    /// Selects a portfolio by ID.
    /// </summary>
    /// <param name="iID">Portfolio ID</param>
    /// <param name="portMain">Portfolio record to populate</param>
    /// <param name="err">Error structure</param>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectPortmain")]
    public static extern void SelectPortmain(int iID, ref PORTMAIN portMain, ref ERRSTRUCT err);

    /// <summary>
    /// Deletes a portfolio account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "AccountDeletePortmain")]
    public static extern void AccountDeletePortmain(int iID, ref ERRSTRUCT err);

    /// <summary>
    /// Inserts a new portfolio account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void AccountInsertPortmain();

    /// <summary>
    /// Updates the valuation date for a portfolio.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "UpdatePortmainValDate")]
    public static extern void UpdatePortmainValDate(int iID, int valuationDate, ref ERRSTRUCT err);

    #endregion

    #region Transaction Operations

    /// <summary>
    /// Selects a transaction record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectTrans")]
    public static extern void SelectTrans(
        int iID,
        int lEffDate1,
        int lEffDate2,
        StringBuilder sSecNo,
        StringBuilder sWi,
        [MarshalAs(UnmanagedType.Bool)] bool bSpecificSecNo,
        ref TRANS trans,
        ref ERRSTRUCT err);

    /// <summary>
    /// Selects transactions by effective date range.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectTransByEffectiveDate")]
    public static extern void SelectTransByEffectiveDate(
        int iID,
        int lEffDate1,
        int lEffDate2,
        StringBuilder sSecNo,
        StringBuilder sWi,
        [MarshalAs(UnmanagedType.Bool)] bool bSpecificSecNo,
        ref TRANS trans,
        ref ERRSTRUCT err);

    /// <summary>
    /// Selects transactions by trade date range.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectTransByTradeDate")]
    public static extern void SelectTransByTradeDate(
        int iID,
        int lTrdDate1,
        int lTrdDate2,
        StringBuilder sSecNo,
        StringBuilder sWi,
        [MarshalAs(UnmanagedType.Bool)] bool bSpecificSecNo,
        ref TRANS trans,
        ref ERRSTRUCT err);

    /// <summary>
    /// Selects transactions by transaction number range.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectTransByTransNo")]
    public static extern void SelectTransByTransNo(
        int iID,
        int lTransNo1,
        int lTransNo2,
        StringBuilder sSecNo,
        StringBuilder sWi,
        [MarshalAs(UnmanagedType.Bool)] bool bSpecificSecNo,
        ref TRANS trans,
        ref ERRSTRUCT err);

    /// <summary>
    /// Checks if any transactions exist for an account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "CheckIfTransExists")]
    public static extern void CheckIfTransExists(int iID, ref int count, ref ERRSTRUCT err);

    /// <summary>
    /// Inserts a new transaction.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void InsertTrans();

    /// <summary>
    /// Inserts a transaction description.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void InsertTransDesc();

    #endregion

    #region DControl (Daily Control) Operations

    /// <summary>
    /// Selects daily control record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectDcontrol")]
    public static extern void SelectDcontrol(int iID, int lEffDate, ref ERRSTRUCT err);

    #endregion

    #region DTrans (Daily Transaction) Operations

    /// <summary>
    /// Selects daily transaction record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectDtrans")]
    public static extern void SelectDtrans(int iID, int lEffDate, ref TRANS trans, ref ERRSTRUCT err);

    /// <summary>
    /// Selects daily transaction description.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectDtransDesc")]
    public static extern void SelectDtransDesc(int lDtransNo, StringBuilder desc, ref ERRSTRUCT err);

    /// <summary>
    /// Updates a daily transaction record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "UpdateDtrans")]
    public static extern void UpdateDtrans(
        int iID,
        int lStlDate,
        int lEffDate,
        int lEntryDate,
        double fUnits,
        double fUnitCost,
        ref ERRSTRUCT err);

    #endregion

    #region Holdings Operations

    /// <summary>
    /// Selects holdings for an account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void SelectHoldings();

    /// <summary>
    /// Inserts a holdings record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "InsertHoldings")]
    public static extern void InsertHoldings(/* parameters omitted for brevity */);

    /// <summary>
    /// Updates a holdings record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "UpdateHoldings")]
    public static extern void UpdateHoldings(/* parameters omitted for brevity */);

    /// <summary>
    /// Deletes holdings for an account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void DeleteHoldings();

    /// <summary>
    /// Selects all holdings for a specific account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void SelectAllHoldingsForAnAccount();

    /// <summary>
    /// Deletes holdings for an account.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "AccountDeleteHoldings")]
    public static extern void AccountDeleteHoldings(
        int iID,
        StringBuilder sSecNo,
        StringBuilder sWi,
        ref ERRSTRUCT err);

    #endregion

    #region Security Price Operations

    /// <summary>
    /// Gets the security price for a given date.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "GetSecurityPrice")]
    public static extern double GetSecurityPrice(
        int iSecID,
        int lDate,
        ref double price,
        ref ERRSTRUCT err);

    /// <summary>
    /// Checks if closing price is manual.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "IsManualClosingPrice")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsManualClosingPrice(int iSecID, int lDate);

    #endregion

    #region Performance Operations

    /// <summary>
    /// Selects performance control record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectPerfctrl")]
    public static extern void SelectPerfctrl(int iID, ref ERRSTRUCT err);

    /// <summary>
    /// Updates performance control.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "UpdatePerfctrl")]
    public static extern void UpdatePerfctrl(int iID, int lStartDate, int lEndDate, ref ERRSTRUCT err);

    /// <summary>
    /// Inserts a performance key record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "InsertPerfkey")]
    public static extern void InsertPerfkey(/* parameters */);

    /// <summary>
    /// Updates performance date.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "UpdatePerfDate")]
    public static extern void UpdatePerfDate(int iID, int lPerfDate, ref ERRSTRUCT err);

    #endregion

    #region Unit Value Operations

    /// <summary>
    /// Selects unit value for a portfolio/date.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectUnitValue")]
    public static extern void SelectUnitValue(int iID, int lDate, ref ERRSTRUCT err);

    /// <summary>
    /// Inserts a unit value record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "InsertUnitValue")]
    public static extern void InsertUnitValue(
        int iID,
        int lDate,
        double fUnitValue,
        double fUnits,
        double fMarketValue,
        ref ERRSTRUCT err);

    /// <summary>
    /// Updates a unit value record.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "UpdateUnitValue")]
    public static extern void UpdateUnitValue(
        int iID,
        int lDate,
        double fUnitValue,
        double fUnits,
        double fMarketValue,
        ref ERRSTRUCT err);

    #endregion

    #region System Settings

    /// <summary>
    /// Selects system values/settings.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "SelectSysValues")]
    public static extern void SelectSysValues(StringBuilder sysKey);

    /// <summary>
    /// Selects system settings.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV)]
    public static extern void SelectSyssettings();

    #endregion

    #region Error Handling

    /// <summary>
    /// Prints/logs an error message.
    /// </summary>
    /// <param name="functionName">Name of function where error occurred</param>
    /// <param name="iID">Account ID (0 if not applicable)</param>
    /// <param name="lTransNo">Transaction number (0 if not applicable)</param>
    /// <param name="sSecNo">Security number (empty if not applicable)</param>
   /// <param name="lTaxlotNo">Taxlot number (0 if not applicable)</param>
    /// <param name="iSecID">Security ID (0 if not applicable)</param>
    /// <param name="iLineNumber">Line number in source</param>
    /// <param name="errorMessage">Error message text</param>
    /// <param name="bShowMessage">Whether to display message to user</param>
    /// <returns>ERRSTRUCT with error details</returns>
    [DllImport(DLL_NAME, CallingConvention = CALL_CONV, EntryPoint = "PrintError")]
    public static extern ERRSTRUCT PrintError(
        [MarshalAs(UnmanagedType.LPStr)] string functionName,
        int iID,
        int lTransNo,
        [MarshalAs(UnmanagedType.LPStr)] string sSecNo,
        int lTaxlotNo,
        int iSecID,
        int iLineNumber,
        [MarshalAs(UnmanagedType.LPStr)] string errorMessage,
        [MarshalAs(UnmanagedType.Bool)] bool bShowMessage);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Safely calls an OLEDBIO function and checks for errors.
    /// </summary>
    /// <param name="action">Action to execute</param>
    /// <param name="functionName">Name of the function (for error reporting)</param>
    /// <exception cref="NativeInteropException">Thrown if operation fails</exception>
    public static void SafeCall(Action<ERRSTRUCT> action, string functionName)
    {
        var err = new ERRSTRUCT();
        InitializeErrStruct(ref err);
        
        try
        {
            action(err);
            
            if (!err.IsSuccess)
            {
                throw new NativeInteropException(
                    $"OLEDBIO operation failed: {err.FormatError()}",
                    err);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                $"OLEDBIO.dll not found. Ensure it's in the same directory as the executable or in the system PATH.",
                err);
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function '{functionName}' not found in OLEDBIO.dll. Version mismatch?",
                err,
                ex);
        }
    }

    #endregion
}

/// <summary>
/// Exception thrown when a native interop call fails.
/// </summary>
public class NativeInteropException : Exception
{
    public ERRSTRUCT ErrorStruct { get; }

    public NativeInteropException(string message, ERRSTRUCT errStruct)
        : base(message)
    {
        ErrorStruct = errStruct;
    }

    public NativeInteropException(string message, ERRSTRUCT errStruct, Exception innerException)
        : base(message, innerException)
    {
        ErrorStruct = errStruct;
    }
}
