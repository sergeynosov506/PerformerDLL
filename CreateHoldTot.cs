using System;
using System.Runtime.InteropServices;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for CreateHoldTot.dll - Holdings totals creation engine.
/// Contains functions for creating summary holdings records.
/// </summary>
/// <remarks>
/// CreateHoldTot.dll handles:
/// - Creating holdings totals (summaries) for portfolios
/// - Aggregating holdings by various dimensions
/// - End-of-period holdings snapshots
/// 
/// The typical workflow during valuation is:
/// 1. Call InitHoldtot() once with database connection
/// 2. Call CreateHoldtot() for each portfolio after valuation
/// 
/// Based on Delphi PortfolioActionsUnit.pas signatures.
/// </remarks>
public static class CreateHoldTot
{
    private const string DLL_NAME = "CreateHoldTot.dll";

    #region Initialization

    /// <summary>
    /// Initializes the HoldTot engine with database connection.
    /// </summary>
    /// <param name="asofDate">As-of date (YYYYMMDD format)</param>
    /// <param name="dbAlias">Database alias/connection string</param>
    /// <param name="errFile">Path to error log file</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// From Delphi: TInitHoldtot = function(lAsofDate:longint; sDBAlias, sErrFile:PChar): NativeERRSTRUCT; stdcall;
    /// 
    /// This must be called before CreateHoldtot().
    /// The asofDate can be:
    /// - -1 for current/default holdings
    /// - 0 for adhoc holdings
    /// - A specific date (YYYYMMDD) for historical holdings
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT InitHoldtot(
        int asofDate,
        [MarshalAs(UnmanagedType.LPStr)] string dbAlias,
        [MarshalAs(UnmanagedType.LPStr)] string errFile);

    #endregion

    #region Holdings Totals Creation

    /// <summary>
    /// Creates holdings totals (summary records) for a portfolio.
    /// </summary>
    /// <param name="portfolioId">Portfolio/Account ID</param>
    /// <param name="date">Date for the holdings snapshot (YYYYMMDD)</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// From Delphi: TCreateHoldtot = function(iPortfolioId: integer; lDate: longint): NativeERRSTRUCT; stdcall;
    /// 
    /// This creates summary records in the holdtot table, aggregating holdings
    /// by security, account type, and other dimensions.
    /// 
    /// Should be called after AccountValuation() completes successfully.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT CreateHoldtot(
        int portfolioId,
        int date);

    #endregion

    #region Helper Methods

    private static bool _isInitialized = false;

    /// <summary>
    /// Initializes the HoldTot engine with standard error handling.
    /// </summary>
    /// <param name="asofDate">As-of date (YYYYMMDD, -1 for current)</param>
    /// <param name="dbAlias">Database connection string</param>
    /// <param name="errFile">Error log file path</param>
    /// <returns>True if initialization succeeded</returns>
    public static bool Initialize(int asofDate, string dbAlias, string errFile)
    {
        try
        {
            var result = InitHoldtot(asofDate, dbAlias, errFile);
            _isInitialized = result.IsSuccess;
            return _isInitialized;
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "CreateHoldTot.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
    }

    /// <summary>
    /// Creates holdings totals with initialization check and error handling.
    /// </summary>
    /// <param name="portfolioId">Portfolio ID</param>
    /// <param name="date">Date in YYYYMMDD format</param>
    /// <param name="dbAlias">Database connection (for auto-init if needed)</param>
    /// <param name="errFile">Error file path (for auto-init if needed)</param>
    public static void SafeCreateHoldtot(
        int portfolioId, 
        int date,
        string? dbAlias = null,
        string? errFile = null)
    {
        try
        {
            // Auto-initialize if not done yet
            if (!_isInitialized && dbAlias != null && errFile != null)
            {
                var initResult = InitHoldtot(date, dbAlias, errFile);
                if (!initResult.IsSuccess)
                {
                    throw new NativeInteropException(
                        $"HoldTot initialization failed: {initResult.FormatError()}",
                        initResult);
                }
                _isInitialized = true;
            }

            var result = CreateHoldtot(portfolioId, date);
            
            if (!result.IsSuccess)
            {
                throw new NativeInteropException(
                    $"CreateHoldtot failed: {result.FormatError()}",
                    result);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "CreateHoldTot.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function not found in CreateHoldTot.dll. Version mismatch?",
                new NativeERRSTRUCT(),
                ex);
        }
    }

    /// <summary>
    /// Resets the initialization state (for testing or reconnection).
    /// </summary>
    public static void ResetInitialization()
    {
        _isInitialized = false;
    }

    #endregion
}
