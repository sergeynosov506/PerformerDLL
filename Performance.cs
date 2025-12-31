using System;
using System.Runtime.InteropServices;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for Performance.dll - Performance calculation engine.
/// Contains functions for calculating portfolio performance metrics.
/// </summary>
/// <remarks>
/// Performance.dll handles:
/// - Time-weighted return calculations
/// - Money-weighted return calculations
/// - Performance attribution
/// - Benchmark comparisons
/// - Daily and monthly performance
/// 
/// The typical workflow is:
/// 1. Call InitPerformance() once at startup
/// 2. Call CalculatePerformance() for each portfolio
/// 
/// Based on Delphi PortfolioActionsUnit.pas signatures.
/// </remarks>
public static class Performance
{
    private const string DLL_NAME = "Performance.dll";

    #region Initialization

    /// <summary>
    /// Initializes the Performance engine with database connection.
    /// </summary>
    /// <param name="asofDate">As-of date (YYYYMMDD format, or -1 for default)</param>
    /// <param name="dbPath">Database path/connection string</param>
    /// <param name="mode">Operation mode (empty string for default)</param>
    /// <param name="type">Operation type (empty string for default)</param>
    /// <param name="prepareQueries">Whether to prepare SQL queries</param>
    /// <param name="errFile">Path to error log file</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// From Delphi: TInitPerformance = function(lAsofDate: longint; sDBPath, sMode, sType: PChar; 
    ///              bPrepareQueries: longbool; sErrFile: PChar): NativeERRSTRUCT; stdcall;
    /// 
    /// This must be called before CalculatePerformance().
    /// The asofDate parameter can be -1 for default behavior.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT InitPerformance(
        int asofDate,
        [MarshalAs(UnmanagedType.LPStr)] string dbPath,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        [MarshalAs(UnmanagedType.LPStr)] string type,
        [MarshalAs(UnmanagedType.Bool)] bool prepareQueries,
        [MarshalAs(UnmanagedType.LPStr)] string errFile);

    #endregion

    #region Performance Calculations

    /// <summary>
    /// Calculates performance for a portfolio.
    /// </summary>
    /// <param name="accountId">Account/Portfolio ID</param>
    /// <param name="currentDate">End date for calculation (YYYYMMDD)</param>
    /// <param name="startDate">Start date for calculation (YYYYMMDD)</param>
    /// <param name="anchorDate">Anchor date for calculations (typically startDate+1)</param>
    /// <param name="earliestPerfDate">Earliest performance date (typically startDate+1)</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// From Delphi: TCalculatePerformance = function(iID: integer; 
    ///              lCurrentDate, lStartDate, lAnchorDate, lEarliestPerfDate: longint): NativeERRSTRUCT; stdcall;
    /// 
    /// The calculation covers the period from startDate to currentDate.
    /// For large date ranges, the Delphi code breaks this into 12-month periods.
    /// 
    /// Example from Delphi:
    ///   Result := CalculatePerformance(iID, lEndPerfDate, lStartPerfDate, lStartPerfDate+1, lStartPerfDate+1);
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT CalculatePerformance(
        int accountId,
        int currentDate,
        int startDate,
        int anchorDate,
        int earliestPerfDate);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Initializes the Performance engine with standard settings.
    /// </summary>
    /// <param name="dbPath">Database connection string</param>
    /// <param name="errFile">Error log file path</param>
    /// <returns>True if initialization succeeded</returns>
    public static bool Initialize(string dbPath, string errFile)
    {
        try
        {
            var result = InitPerformance(-1, dbPath, "", "", false, errFile);
            return result.IsSuccess;
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "Performance.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
    }

    /// <summary>
    /// Safely calculates performance with error handling.
    /// </summary>
    public static void SafeCalculate(int accountId, int startDate, int endDate)
    {
        try
        {
            var result = CalculatePerformance(
                accountId,
                endDate,
                startDate,
                startDate + 1,
                startDate + 1);

            if (!result.IsSuccess)
            {
                throw new NativeInteropException(
                    $"Performance calculation failed: {result.FormatError()}",
                    result);
            }
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "Performance.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
        catch (EntryPointNotFoundException ex)
        {
            throw new NativeInteropException(
                $"Function not found in Performance.dll. Version mismatch?",
                new NativeERRSTRUCT(),
                ex);
        }
    }

    /// <summary>
    /// Calculates performance in yearly chunks for large date ranges.
    /// This mirrors the Delphi implementation for handling multi-year periods.
    /// </summary>
    /// <param name="accountId">Account ID</param>
    /// <param name="startDate">Start date (YYYYMMDD)</param>
    /// <param name="endDate">End date (YYYYMMDD)</param>
    /// <returns>Final error structure</returns>
    public static NativeERRSTRUCT CalculatePerformanceInChunks(
        int accountId,
        int startDate,
        int endDate)
    {
        NativeERRSTRUCT result = default;
        int currentStart = startDate;

        // Calculate one year at a time (mirrors Delphi logic)
        while (true)
        {
            int yearEnd = AddYearToDate(currentStart);
            
            if (yearEnd >= endDate)
            {
                // Final chunk
                result = CalculatePerformance(accountId, endDate, currentStart, currentStart + 1, currentStart + 1);
                break;
            }
            else
            {
                // Yearly chunk
                result = CalculatePerformance(accountId, yearEnd, currentStart, currentStart + 1, currentStart + 1);
                
                if (!result.IsSuccess)
                    break;
                
                currentStart = yearEnd;
            }
        }

        return result;
    }

    /// <summary>
    /// Adds one year to a date in YYYYMMDD format.
    /// </summary>
    private static int AddYearToDate(int date)
    {
        int year = date / 10000;
        int monthDay = date % 10000;
        return (year + 1) * 10000 + monthDay;
    }

    #endregion
}
