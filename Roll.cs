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
        [MarshalAs(UnmanagedType.LPStr)] string sDBPath,
        [MarshalAs(UnmanagedType.LPStr)] string sType,
        [MarshalAs(UnmanagedType.LPStr)] string sMode,
        int lAsofDate,
        [MarshalAs(UnmanagedType.LPStr)] string sErrFile);

    #endregion

    #region Roll Operations

    /// <summary>
    /// Full Roll function matching DLL signature.
    /// </summary>
    /// <param name="iID">Account ID</param>
    /// <param name="alphaId1">Alpha identifier 1 (e.g. sec_no)</param>
    /// <param name="alphaId2">Alpha identifier 2 (e.g. wi)</param>
    /// <param name="numericId1">Numeric identifier 1 (e.g. date range start)</param>
    /// <param name="numericId2">Numeric identifier 2 (e.g. date range end)</param>
    /// <param name="alphaFlag">Alpha flag (F=firm, B=branch, S=security, C=composite, M=manager)</param>
    /// <param name="numericFlag">Numeric flag (T=trans, TD=trade date, ED=eff date, ND=entry date)</param>
    /// <param name="initDataSet">Whether to initialize the dataset</param>
    /// <param name="whichDataSet">Target dataset (0=auto, 1=roll, 2=perform, 3=settlement)</param>
    /// <param name="rollDate">Target roll date (YYYYMMDD)</param>
    /// <param name="resetPerfDate">Performance reset flag (0=no, 1=reset to monthly, 2=reset to holdmap)</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall, EntryPoint = "Roll")]
    public static extern ERRSTRUCT PerformRoll(
        int iID,
        [MarshalAs(UnmanagedType.LPStr)] string alphaId1,
        [MarshalAs(UnmanagedType.LPStr)] string alphaId2,
        int numericId1,
        int numericId2,
        [MarshalAs(UnmanagedType.LPStr)] string alphaFlag,
        [MarshalAs(UnmanagedType.LPStr)] string numericFlag,
        [MarshalAs(UnmanagedType.Bool)] bool initDataSet,
        int whichDataSet,
        int rollDate,
        int resetPerfDate);

    /// <summary>
    /// RollFromCurrent function matching DLL signature.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall, EntryPoint = "RollFromCurrent")]
    public static extern NativeERRSTRUCT RollFromCurrent(
        int iID,
        [MarshalAs(UnmanagedType.LPStr)] string alphaFlag,
        [MarshalAs(UnmanagedType.Bool)] bool initDataSet,
        int whichDataSet,
        int rollDate,
        int resetPerfDate);

    /// <summary>
    /// RollFromInception function matching DLL signature.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall, EntryPoint = "RollFromInception")]
    public static extern NativeERRSTRUCT RollFromInception(
        int iID,
        int whichDataSet,
        int rollDate,
        int resetPerfDate);

    /// <summary>
    /// SettlementRoll function matching DLL signature.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall, EntryPoint = "SettlementRoll")]
    public static extern NativeERRSTRUCT SettlementRoll(
        int iID,
        [MarshalAs(UnmanagedType.LPStr)] string alphaFlag,
        int rollDate,
        [MarshalAs(UnmanagedType.Bool)] bool doTransaction);

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
                throw new NativeInteropException(
                    $"Roll.{operationName} failed: {result.FormatError()}",
                    result);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "Roll.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function '{operationName}' not found in Roll.dll.",
                new NativeERRSTRUCT(),
                ex);
        }
    }

    /// <summary>
    /// Rolls multiple portfolios in a batch operation.
    /// </summary>
    public static List<(int AccountId, NativeERRSTRUCT Result)> BatchRoll(
        IEnumerable<int> accountIds,
        int toDate,
        bool verbose = false)
    {
        var results = new List<(int, NativeERRSTRUCT)>();

        foreach (var accountId in accountIds)
        {
            try
            {
                var result = RollFromCurrent(
                    accountId,
                    "B", // Alpha flag for Branch account
                    true, // Initialize dataset
                    1, // Which dataset (Roll)
                    toDate,
                    1); // Reset performance date
                
                results.Add((accountId, result));
            }
            catch (Exception ex)
            {
                var errorStruct = new NativeERRSTRUCT
                {
                    iSqlError = -1,
                    iBusinessError = -1
                };
                results.Add((accountId, errorStruct));
            }
        }

        return results;
    }

    #endregion
}
