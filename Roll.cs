using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for Roll.dll - Portfolio rollup and aggregation operations.
/// Contains 5 exported functions for end-of-month and end-of-period processing.
/// </summary>
/// <remarks>
/// Roll.dll handles:
/// - End-of-month portfolio rollups
/// - End-of-period aggregations
/// - Settlement date rolls
/// - Cumulative performance calculations
/// 
/// These operations are typically run monthly or quarterly to aggregate holdings,
/// calculate performance, and prepare summary data.
/// </remarks>
public static class Roll
{
    private const string DLL_NAME = "Roll.dll";

    #region Initialization

    /// <summary>
    /// Initializes the Roll engine with database connection information.
    /// </summary>
    /// <param name="server">Database server</param>
    /// <param name="database">Database name</param>
    /// <param name="username">Username (null for Windows auth)</param>
    /// <param name="password">Password (null for Windows auth)</param>
    /// <param name="logFile">Path to log file</param>
    /// <returns>Error structure</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT InitRoll(
        int lAsOfDate,
        [MarshalAs(UnmanagedType.LPStr)] string server,
        [MarshalAs(UnmanagedType.LPStr)] string database,
        [MarshalAs(UnmanagedType.LPStr)] string? username,
        [MarshalAs(UnmanagedType.LPStr)] string? password,
        [MarshalAs(UnmanagedType.LPStr)] string logFile);

    #endregion

    #region Roll Operations

    /// <summary>
    /// Performs a portfolio roll from one date to another.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="acctNo">Account number</param>
    /// <param name="acctType">Account type</param>
    /// <param name="fromDate">Starting date (YYYYMMDD)</param>
    /// <param name="toDate">Ending date (YYYYMMDD)</param>
    /// <param name="description">Roll description</param>
    /// <param name="userName">User performing the roll</param>
    /// <param name="calculatePerformance">Whether to calculate performance</param>
    /// <param name="updateMarketValues">Whether to update market values</param>
    /// <param name="createSnapshots">Whether to create holdings snapshots</param>
    /// <param name="verbose">Verbose logging</param>
    /// <returns>Error structure with results</returns>
    /// <remarks>
    /// This is the main portfolio roll function. It:
    /// 1. Processes all transactions between fromDate and toDate
    /// 2. Updates holdings and balances
    /// 3. Calculates performance metrics if requested
    /// 4. Creates end-of-period snapshots
    /// 5. Updates rollup tables
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT RollPortfolio(
        int iID,
        [MarshalAs(UnmanagedType.LPStr)] string acctNo,
        [MarshalAs(UnmanagedType.LPStr)] string acctType,
        int fromDate,
        int toDate,
        [MarshalAs(UnmanagedType.LPStr)] string description,
        [MarshalAs(UnmanagedType.LPStr)] string userName,
        int calculatePerformance,
        int updateMarketValues,
        int createSnapshots,
        int verbose);

    /// <summary>
    /// Rolls a portfolio from its current roll date to the specified date.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="acctNo">Account number</param>
    /// <param name="toDate">Target roll date</param>
    /// <param name="updateMarketValues">Update market values flag</param>
    /// <param name="createSnapshots">Create snapshots flag</param>
    /// <param name="verbose">Verbose logging flag</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// This function automatically determines the starting date from the portfolio's
    /// last roll date and rolls forward to the specified date.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT RollFromCurrent(
        int iID,
        [MarshalAs(UnmanagedType.LPStr)] string acctNo,
        int toDate,
        int updateMarketValues,
        int createSnapshots,
        int verbose);

    /// <summary>
    /// Rolls a portfolio from inception to the specified date.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="toDate">Target roll date</param>
    /// <param name="createSnapshots">Create snapshots flag</param>
    /// <param name="verbose">Verbose logging flag</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// WARNING: This operation can be time-consuming for portfolios with long histories.
    /// It processes all transactions from the portfolio inception date.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT RollFromInception(
        int iID,
        int toDate,
        int createSnapshots,
        int verbose);

    /// <summary>
    /// Performs a settlement roll to process unsettled trades.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="acctNo">Account number</param>
    /// <param name="settlementDate">Settlement date to process</param>
    /// <param name="verbose">Verbose logging flag</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// Settlement rolls are used to:
    /// - Mark trades as settled on their settlement date
    /// - Update cash balances
    /// - Finalize cost basis calculations
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT SettlementRoll(
        int iID,
        [MarshalAs(UnmanagedType.LPStr)] string acctNo,
        int settlementDate,
        int verbose);

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern void FreeRoll();

    #endregion

    #region Helper Methods

    /// <summary>
    /// Safely executes a roll operation with comprehensive error handling.
    /// </summary>
    public static void SafeRoll(Func<NativeERRSTRUCT> rollOperation, string operationName)
    {
        try
        {
            var result = rollOperation();
            
            if (!result.IsSuccess)
            {
                var legacyErr = result.ToLegacy();
                throw new NativeInteropException(
                    $"Roll.{operationName} failed: {legacyErr.FormatError()}",
                    legacyErr);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "Roll.dll not found. Ensure it's in the application directory.",
                new ERRSTRUCT());
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function '{operationName}' not found in Roll.dll.",
                new ERRSTRUCT(),
                ex);
        }
    }

    /// <summary>
    /// Rolls multiple portfolios in a batch operation.
    /// </summary>
    public static List<(int AccountId, ERRSTRUCT Result)> BatchRoll(
        IEnumerable<int> accountIds,
        int toDate,
        bool verbose = false)
    {
        var results = new List<(int, ERRSTRUCT)>();

        foreach (var accountId in accountIds)
        {
            try
            {
                var result = RollFromCurrent(
                    accountId,
                    "", // Account number will be looked up
                    toDate,
                    1, // Update market values
                    1, // Create snapshots
                    verbose ? 1 : 0);
                
                results.Add((accountId, result.ToLegacy()));
            }
            catch (Exception ex)
            {
                var errorStruct = new ERRSTRUCT
                {
                    iSqlError = -1,
                    sErrorMessage = ex.Message,
                    sFunctionName = nameof(BatchRoll)
                };
                results.Add((accountId, errorStruct));
            }
        }

        return results;
    }

    #endregion
}
