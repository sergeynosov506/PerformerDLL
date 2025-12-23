using System;
using System.Runtime.InteropServices;
using System.Text;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for TransEngine.dll - Transaction processing and conversion engine.
/// Contains 18 exported functions for processing financial transactions.
/// </summary>
/// <remarks>
/// TransEngine.dll handles:
/// - Transaction allocation (TranAlloc)
/// - Transaction processing (TranProc)
/// - Phantom income calculations
/// - Holdings/portfolio updates
/// 
/// All functions use Cdecl calling convention (C++ standard).
/// </remarks>
public static class TransEngine
{
    private const string DLL_NAME = "TransEngine.dll";
    
    #region Initialization

    /// <summary>
    /// Initializes the transaction processing engine with connection information.
    /// </summary>
    /// <param name="server">Database server name</param>
    /// <param name="database">Database name</param>
    /// <param name="username">Database username (optional for Windows auth)</param>
    /// <param name="password">Database password (optional for Windows auth)</param>
    /// <param name="useWindowsAuth">Use Windows authentication</param>
    /// <returns>0 if successful, non-zero error code otherwise</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int InitTranProc(
        [MarshalAs(UnmanagedType.LPStr)] string server,
        [MarshalAs(UnmanagedType.LPStr)] string database,
        [MarshalAs(UnmanagedType.LPStr)] string? username,
        [MarshalAs(UnmanagedType.LPStr)] string? password,
        [MarshalAs(UnmanagedType.Bool)] bool useWindowsAuth);

    /// <summary>
    /// Initializes an error structure.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void InitializeErrStruct(ref ERRSTRUCT err);

    /// <summary>
    /// Initializes a transaction structure with default values.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void InitializeTransStruct(ref TRANS trans);

    /// <summary>
    /// Initializes a holdings structure with default values.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void InitializeHoldingsStruct(ref HOLDINGS holdings);

    /// <summary>
    /// Initializes a portfolio main structure with default values.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void InitializePortmainStruct(ref PORTMAIN portMain);

    /// <summary>
    /// Initializes DTrans description structure.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void InitializeDtransDesc();

    #endregion

    #region Transaction Processing

    /// <summary>
    /// Main transaction processing function. Processes a transaction and updates holdings.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="trans">Transaction to process</param>
    /// <param name="updateHoldings">Whether to update holdings table</param>
    /// <param name="err">Error structure for reporting</param>
    /// <returns>Error structure with results</returns>
    /// <remarks>
    /// This is the core transaction processing engine. It:
    /// 1. Validates the transaction
    /// 2. Updates holdings based on transaction type
    /// 3. Calculates cost basis and gains/losses
    /// 4. Updates portfolio balances
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ERRSTRUCT TranProc(
        int iID,
        ref TRANS trans,
        [MarshalAs(UnmanagedType.Bool)] bool updateHoldings,
        ref ERRSTRUCT err);

    /// <summary>
    /// Gets information about the last TranProc operation.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void TranProcInfo();

    /// <summary>
    /// Allocates a transaction against specific tax lots or holdings.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="trans">Transaction to allocate</param>
    /// <param name="allocationMethod">Method: FIFO, LIFO, MaxGain, MinGain, etc.</param>
    /// <param name="err">Error structure</param>
    /// <returns>Error structure with results</returns>
    /// <remarks>
    /// Used for sells/dispositions to determine which lots are being sold.
    /// Supports multiple allocation methods based on tax optimization.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ERRSTRUCT TranAlloc(
        int iID,
        ref TRANS trans,
        [MarshalAs(UnmanagedType.LPStr)] string allocationMethod,
        ref ERRSTRUCT err);

    #endregion

    #region Holdings Updates

    /// <summary>
    /// Updates holdings after transaction processing.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="trans">Transaction that was processed</param>
    /// <param name="err">Error structure</param>
    /// <returns>Error structure with results</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ERRSTRUCT UpdateHold(
        int iID,
        ref TRANS trans,
        ref ERRSTRUCT err);

    #endregion

    #region Transaction Table Operations

    /// <summary>
    /// Initializes the transaction table for batch operations.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void InitializeTransTable2();

    /// <summary>
    /// Adds a transaction to the transaction table for batch processing.
    /// </summary>
    /// <param name="trans">Transaction to add</param>
    /// <param name="index">Index in the table</param>
    /// <returns>Success indicator</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AddTransToTransTable2(ref TRANS trans, int index);

    #endregion

    #region Income and Inflation Calculations

    /// <summary>
    /// Calculates phantom income for a security (e.g., original issue discount bonds).
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="iSecID">Security ID</param>
    /// <param name="startDate">Calculation start date</param>
    /// <param name="endDate">Calculation end date</param>
    /// <param name="err">Error structure</param>
    /// <returns>Error structure with phantom income amount</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ERRSTRUCT CalculatePhantomIncome(
        int iID,
        int iSecID,
        int startDate,
        int endDate,
        ref ERRSTRUCT err);

    /// <summary>
    /// Calculates inflation rate for TIPS or inflation-indexed securities.
    /// </summary>
    /// <param name="iSecID">Security ID</param>
    /// <param name="date">Calculation date</param>
    /// <param name="inflationRate">Output: calculated rate</param>
    /// <param name="err">Error structure</param>
    /// <returns>Error structure with results</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern ERRSTRUCT CalculateInflationRate(
        int iSecID,
        int date,
        ref double inflationRate,
        ref ERRSTRUCT err);

    #endregion

    #region Error and Logging

    /// <summary>
    /// Prints an error message to the log.
    /// </summary>
    /// <param name="message">Error message</param>
    /// <param name="err">Error structure</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void PrintError(
        [MarshalAs(UnmanagedType.LPStr)] string message,
        ref ERRSTRUCT err);

    /// <summary>
    /// Sets the error log file name.
    /// </summary>
    /// <param name="fileName">Full path to error log file</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void SetErrorFileName(
        [MarshalAs(UnmanagedType.LPStr)] string fileName);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Safely executes a TransEngine operation with error handling.
    /// </summary>
    public static void SafeCall(Func<ERRSTRUCT, ERRSTRUCT> operation, string operationName)
    {
        var err = new ERRSTRUCT();
        InitializeErrStruct(ref err);

        try
        {
            var result = operation(err);
            
            if (!result.IsSuccess)
            {
                throw new NativeInteropException(
                    $"TransEngine.{operationName} failed: {result.FormatError()}",
                    result);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "TransEngine.dll not found. Ensure it's in the application directory.",
                err);
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function '{operationName}' not found in TransEngine.dll.",
                err,
                ex);
        }
    }

    #endregion
}
