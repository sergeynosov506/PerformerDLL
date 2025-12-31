using System;
using System.Runtime.InteropServices;
using System.Text;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for Valuation.dll - Portfolio valuation and mark-to-market operations.
/// Contains 2 exported functions for valuing portfolio holdings and cash positions.
/// </summary>
/// <remarks>
/// Valuation.dll handles:
/// - Portfolio holdings valuation (mark-to-market)
/// - Cash position valuation
/// - Accrued interest calculations
/// - Gain/loss calculations
/// - Currency exchange rate conversions
/// 
/// These operations are typically run daily or on-demand to value portfolios
/// at market prices and calculate unrealized gains/losses.
/// </remarks>
public static class Valuation
{
    private const string DLL_NAME = "Valuation.dll";

    #region Initialization

    /// <summary>
    /// Initializes the Valuation engine with database connection information.
    /// </summary>
    /// <param name="asofDate">As-of date for valuation (YYYYMMDD format)</param>
    /// <param name="dbAlias">Database alias/connection string</param>
    /// <param name="errorFile">Path to error log file</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// This function must be called before any valuation operations.
    /// It initializes the database connection and loads required reference data.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT InitializeAccountValuation(
        int asofDate,
        [MarshalAs(UnmanagedType.LPStr)] string dbAlias,
        [MarshalAs(UnmanagedType.LPStr)] string errorFile);

    #endregion

    #region Valuation Operations

    /// <summary>
    /// Performs portfolio valuation for a specific account.
    /// </summary>
    /// <param name="mode">Valuation mode (e.g., "F" for full, "P" for partial)</param>
    /// <param name="accountId">Account ID to value</param>
    /// <param name="priceDate">Price date for valuation (YYYYMMDD format)</param>
    /// <param name="partialValuation">
    /// If true, performs partial valuation (holdings only, no accruals).
    /// If false, performs full valuation (holdings, cash, accruals, gain/loss).
    /// </param>
    /// <returns>Error structure with results</returns>
    /// <remarks>
    /// This is the main valuation function. It:
    /// 1. Retrieves current holdings and cash positions
    /// 2. Gets market prices for securities
    /// 3. Calculates market values using exchange rates
    /// 4. Computes accrued interest (if full valuation)
    /// 5. Calculates unrealized gain/loss
    /// 6. Updates valuation tables
    /// 
    /// Partial valuation is typically used for performance calculations where
    /// only market values are needed without detailed accrual breakdowns.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT AccountValuation(
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        int accountId,
        int priceDate,
        [MarshalAs(UnmanagedType.Bool)] bool partialValuation);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Safely executes a valuation operation with comprehensive error handling.
    /// </summary>
    public static void SafeValuation(Func<NativeERRSTRUCT> valuationOperation, string operationName)
    {
        try
        {
            var result = valuationOperation();
            
            if (!result.IsSuccess)
            {
                throw new NativeInteropException(
                    $"Valuation.{operationName} failed: {result.FormatError()}",
                    result);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "Valuation.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function '{operationName}' not found in Valuation.dll.",
                new NativeERRSTRUCT(),
                ex);
        }
    }

    /// <summary>
    /// Initializes the valuation engine with standard error handling.
    /// </summary>
    public static void Initialize(int asofDate, string dbAlias, string errorFile)
    {
        SafeValuation(
            () => InitializeAccountValuation(asofDate, dbAlias, errorFile),
            nameof(InitializeAccountValuation));
    }

    /// <summary>
    /// Values a single account with standard error handling.
    /// </summary>
    public static void ValueAccount(string mode, int accountId, int priceDate, bool partialValuation = false)
    {
        SafeValuation(
            () => AccountValuation(mode, accountId, priceDate, partialValuation),
            nameof(AccountValuation));
    }

    #endregion
}
