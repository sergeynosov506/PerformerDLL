using System;
using System.Runtime.InteropServices;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for StarsUtils.dll - Date and utility functions.
/// Contains date manipulation, business day calculations, and other utilities.
/// </summary>
/// <remarks>
/// StarsUtils.dll provides:
/// - Month end calculations
/// - Business day calculations
/// - Holiday checking
/// - Date format conversions
/// 
/// Based on Delphi PortfolioActionsUnit.pas signatures.
/// </remarks>
public static class StarsUtils
{
    private const string DLL_NAME = "OLEDBIO.dll";

    #region Month End Functions

    /// <summary>
    /// Gets the last day of the month for a given date.
    /// </summary>
    /// <param name="currentDate">Input date (YYYYMMDD format)</param>
    /// <returns>Last day of the month containing the input date</returns>
    /// <remarks>
    /// From Delphi: TLastMonthEnd = function(lCurrentDate: longint): longint; stdcall;
    /// 
    /// Example: LastMonthEnd(20231215) returns 20231130 (previous month end)
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern int LastMonthEnd(int currentDate);

    /// <summary>
    /// Gets the current month end date for a given date.
    /// </summary>
    /// <param name="currentDate">Input date (YYYYMMDD format)</param>
    /// <returns>Last day of the month containing the input date</returns>
    /// <remarks>
    /// From Delphi: TCurrentMonthEnd = function(lCurrentDate: longint): longint; stdcall;
    /// 
    /// Example: CurrentMonthEnd(20231215) returns 20231231
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern int CurrentMonthEnd(int currentDate);

    /// <summary>
    /// Checks if a date is a month end.
    /// </summary>
    /// <param name="currentDate">Date to check (YYYYMMDD format)</param>
    /// <returns>True if the date is the last day of its month</returns>
    /// <remarks>
    /// From Delphi: TIsItAMonthEnd = function(lCurrentDate: longint): longbool; stdcall;
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsItAMonthEnd(int currentDate);

    #endregion

    #region Business Day Functions

    /// <summary>
    /// Gets the next business day after a given date.
    /// </summary>
    /// <param name="currentDate">Starting date (YYYYMMDD format)</param>
    /// <param name="country">Country code (e.g., "US")</param>
    /// <param name="whichBusiness">Business type (e.g., "B" for bank)</param>
    /// <param name="nextBusinessDate">Output: next business date</param>
    /// <returns>0 if successful, error code otherwise</returns>
    /// <remarks>
    /// From Delphi: TBusinessDay = function(lCurrentDate: longint; sCountry, sWhichBusiness: PChar; 
    ///              var lNextBusinessDate: longint): integer; stdcall;
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern int NextBusinessDay(
        int currentDate,
        [MarshalAs(UnmanagedType.LPStr)] string country,
        [MarshalAs(UnmanagedType.LPStr)] string whichBusiness,
        out int nextBusinessDate);

    /// <summary>
    /// Gets the previous business day before a given date.
    /// </summary>
    /// <param name="currentDate">Starting date (YYYYMMDD format)</param>
    /// <param name="country">Country code (e.g., "US")</param>
    /// <param name="whichBusiness">Business type (e.g., "B" for bank)</param>
    /// <param name="lastBusinessDate">Output: previous business date</param>
    /// <returns>0 if successful, error code otherwise</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern int LastBusinessDay(
        int currentDate,
        [MarshalAs(UnmanagedType.LPStr)] string country,
        [MarshalAs(UnmanagedType.LPStr)] string whichBusiness,
        out int lastBusinessDate);

    #endregion

    #region Date Manipulation

    /// <summary>
    /// Creates a new date by adding years, months, and days to a current date.
    /// </summary>
    /// <param name="currentDate">Starting date (YYYYMMDD format)</param>
    /// <param name="addToCurrent">If true, adds to current; if false, subtracts</param>
    /// <param name="year">Years to add/subtract</param>
    /// <param name="month">Months to add/subtract</param>
    /// <param name="day">Days to add/subtract</param>
    /// <param name="newDate">Output: resulting date</param>
    /// <returns>0 if successful, error code otherwise</returns>
    /// <remarks>
    /// From Delphi: TNewDateFromCurrent = function(lCurrentDate: longint; bAddToCurrent: Longbool;
    ///              iYear, iMonth, iDay: integer; var lNewDate: longint): integer; stdcall;
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern int NewDateFromCurrent(
        int currentDate,
        [MarshalAs(UnmanagedType.Bool)] bool addToCurrent,
        int year,
        int month,
        int day,
        out int newDate);

    #endregion

    #region Date Conversion

    /// <summary>
    /// MDY (Month-Day-Year) structure for date conversions.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct MdyType
    {
        public int Month;
        public int Day;
        public int Year;
    }

    /// <summary>
    /// Converts month/day/year to Julian date (YYYYMMDD format).
    /// </summary>
    /// <param name="mdy">Input: month, day, year structure</param>
    /// <param name="date">Output: date in YYYYMMDD format</param>
    /// <returns>0 if successful, error code otherwise</returns>
    /// <remarks>
    /// From Delphi: Trmdyjul = function(var mdy: mdytype; var lDate: longint): integer; stdcall;
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall, EntryPoint = "rmdyjul")]
    public static extern int RmdyJul(ref MdyType mdy, out int date);

    /// <summary>
    /// Converts Julian date (YYYYMMDD) to month/day/year.
    /// </summary>
    /// <param name="date">Input: date in YYYYMMDD format</param>
    /// <param name="mdy">Output: month, day, year structure</param>
    /// <returns>0 if successful, error code otherwise</returns>
    /// <remarks>
    /// From Delphi: Trjulmdy = function(lDate: longint; var mdy: mdytype): integer; stdcall;
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall, EntryPoint = "rjulmdy")]
    public static extern int RjulMdy(int date, out MdyType mdy);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Default country code for business day calculations.
    /// </summary>
    public const string DefaultCountry = "US";

    /// <summary>
    /// Default business type for business day calculations.
    /// </summary>
    public const string DefaultBusiness = "B";

    /// <summary>
    /// Gets the next business day using default settings.
    /// </summary>
    /// <param name="currentDate">Starting date</param>
    /// <returns>Next business date, or -1 on error</returns>
    public static int GetNextBusinessDay(int currentDate)
    {
        if (NextBusinessDay(currentDate, DefaultCountry, DefaultBusiness, out int nextDate) == 0)
            return nextDate;
        return -1;
    }

    /// <summary>
    /// Gets the previous business day using default settings.
    /// </summary>
    /// <param name="currentDate">Starting date</param>
    /// <returns>Previous business date, or -1 on error</returns>
    public static int GetLastBusinessDay(int currentDate)
    {
        if (LastBusinessDay(currentDate, DefaultCountry, DefaultBusiness, out int lastDate) == 0)
            return lastDate;
        return -1;
    }

    /// <summary>
    /// Converts a DateTime to YYYYMMDD integer format.
    /// </summary>
    public static int DateTimeToYYYYMMDD(DateTime date)
    {
        return date.Year * 10000 + date.Month * 100 + date.Day;
    }

    /// <summary>
    /// Converts a YYYYMMDD integer to DateTime.
    /// </summary>
    public static DateTime YYYYMMDDToDateTime(int date)
    {
        int year = date / 10000;
        int month = (date / 100) % 100;
        int day = date % 100;
        return new DateTime(year, month, day);
    }

    #endregion
}
