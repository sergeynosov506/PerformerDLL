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
    /// Initializes the transaction processing engine.
    /// </summary>
    /// <param name="lAsofDate">As of date (YYYYMMDD)</param>
    /// <param name="sDBAlias">Database alias or connection string</param>
    /// <param name="sMode">Mode string</param>
    /// <param name="sType">Type string</param>
    /// <param name="bPrepareQueries">Whether to prepare queries</param>
    /// <param name="sErrFile">Path to error log file</param>
    /// <returns>Error structure</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT InitTranProc(
        int lAsofDate,
        [MarshalAs(UnmanagedType.LPStr)] string sDBAlias,
        [MarshalAs(UnmanagedType.LPStr)] string sMode,
        [MarshalAs(UnmanagedType.LPStr)] string sType,
        [MarshalAs(UnmanagedType.Bool)] bool bPrepareQueries,
        [MarshalAs(UnmanagedType.LPStr)] string sErrFile);

    /// <summary>
    /// Initializes an error structure.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void InitializeErrStruct(ref NativeERRSTRUCT err);

    /// <summary>
    /// Initializes a transaction structure with default values.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void InitializeTransStruct(ref TRANS trans);

    /// <summary>
    /// Initializes a holdings structure with default values.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void InitializeHoldingsStruct(ref HOLDINGS holdings);

    /// <summary>
    /// Initializes a portfolio main structure with default values.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void InitializePortmainStruct(ref PORTMAIN portMain);

    /// <summary>
    /// Initializes DTrans description structure.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
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
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT TranProc(
        int iID,
        ref TRANS trans,
        [MarshalAs(UnmanagedType.Bool)] bool updateHoldings,
        ref NativeERRSTRUCT err);

    /// <summary>
    /// Gets information about the last TranProc operation.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
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
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT TranAlloc(
        int iID,
        ref TRANS trans,
        [MarshalAs(UnmanagedType.LPStr)] string allocationMethod,
        ref NativeERRSTRUCT err);

    #endregion

    #region Holdings Updates

    /// <summary>
    /// Updates holdings after transaction processing.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="trans">Transaction that was processed</param>
    /// <param name="err">Error structure</param>
    /// <returns>Error structure with results</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT UpdateHold(
        int iID,
        ref TRANS trans,
        ref NativeERRSTRUCT err);

    #endregion

    #region Transaction Table Operations

    /// <summary>
    /// Initializes the transaction table for batch operations.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void InitializeTransTable2();

    /// <summary>
    /// Adds a transaction to the transaction table for batch processing.
    /// </summary>
    /// <param name="trans">Transaction to add</param>
    /// <param name="index">Index in the table</param>
    /// <returns>Success indicator</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
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
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT CalculatePhantomIncome(
        int iID,
        int iSecID,
        int startDate,
        int endDate,
        ref NativeERRSTRUCT err);

    /// <summary>
    /// Calculates inflation rate for TIPS or inflation-indexed securities.
    /// </summary>
    /// <param name="iSecID">Security ID</param>
    /// <param name="date">Calculation date</param>
    /// <param name="inflationRate">Output: calculated rate</param>
    /// <param name="err">Error structure</param>
    /// <returns>Error structure with results</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT CalculateInflationRate(
        int iSecID,
        int date,
        ref double inflationRate,
        ref NativeERRSTRUCT err);

    #endregion

    #region Error and Logging

    /// <summary>
    /// Prints an error message to the log.
    /// </summary>
    /// <param name="message">Error message</param>
    /// <param name="err">Error structure</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void PrintError(
        [MarshalAs(UnmanagedType.LPStr)] string message,
        ref NativeERRSTRUCT err);

    /// <summary>
    /// Sets the error log file name.
    /// </summary>
    /// <param name="fileName">Full path to error log file</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void SetErrorFileName(
        [MarshalAs(UnmanagedType.LPStr)] string fileName);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Safely executes a TransEngine operation with error handling.
    /// </summary>
    public static void SafeCall(Func<NativeERRSTRUCT, NativeERRSTRUCT> operation, string operationName)
    {
        var err = new NativeERRSTRUCT();
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
