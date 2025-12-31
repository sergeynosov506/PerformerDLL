using System;
using System.Runtime.InteropServices;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for Payments.dll - Processing of income, maturities, and amortizations.
/// </summary>
public static class Payments
{
    private const string DLL_NAME = "Payments.dll";

    /// <summary>
    /// Initializes the Payments engine.
    /// </summary>
    /// <param name="dbPath">Database path/connection string</param>
    /// <param name="type">Operation type</param>
    /// <param name="mode">Operation mode</param>
    /// <param name="asofDate">As-of date (YYYYMMDD)</param>
    /// <param name="errFile">Error log file path</param>
    /// <returns>Error structure</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT InitPayments(
        [MarshalAs(UnmanagedType.LPStr)] string dbPath,
        [MarshalAs(UnmanagedType.LPStr)] string type,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        int asofDate,
        [MarshalAs(UnmanagedType.LPStr)] string errFile);

    /// <summary>
    /// Performs amortization processing.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT Amortize(
        int valDate,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        [MarshalAs(UnmanagedType.LPStr)] string processFlag,
        int id,
        [MarshalAs(UnmanagedType.LPStr)] string secNo,
        [MarshalAs(UnmanagedType.LPStr)] string wi,
        [MarshalAs(UnmanagedType.LPStr)] string secXtend,
        [MarshalAs(UnmanagedType.LPStr)] string acctType,
        int transNo);

    /// <summary>
    /// Generates dividend and interest records.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT GenerateDivInt(
        int valDate,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        [MarshalAs(UnmanagedType.LPStr)] string type,
        int id,
        [MarshalAs(UnmanagedType.LPStr)] string secNo,
        [MarshalAs(UnmanagedType.LPStr)] string wi,
        [MarshalAs(UnmanagedType.LPStr)] string secXtend,
        [MarshalAs(UnmanagedType.LPStr)] string acctType,
        int transNo);

    /// <summary>
    /// Processes payment of dividends and interest.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT PayDivInt(
        int valDate,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        [MarshalAs(UnmanagedType.LPStr)] string processFlag,
        int id,
        [MarshalAs(UnmanagedType.LPStr)] string secNo,
        [MarshalAs(UnmanagedType.LPStr)] string wi,
        [MarshalAs(UnmanagedType.LPStr)] string secXtend,
        [MarshalAs(UnmanagedType.LPStr)] string acctType,
        int transNo);

    /// <summary>
    /// Generates forward maturity records.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT GenerateForwardMaturity(
        int valDate,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        int id,
        [MarshalAs(UnmanagedType.LPStr)] string secNo,
        [MarshalAs(UnmanagedType.LPStr)] string wi,
        [MarshalAs(UnmanagedType.LPStr)] string secXtend,
        [MarshalAs(UnmanagedType.LPStr)] string acctType);

    /// <summary>
    /// Generates maturity processing records.
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT GenerateMaturity(
        int valDate,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        int id,
        [MarshalAs(UnmanagedType.LPStr)] string secNo,
        [MarshalAs(UnmanagedType.LPStr)] string wi,
        [MarshalAs(UnmanagedType.LPStr)] string secXtend,
        [MarshalAs(UnmanagedType.LPStr)] string acctType);

    /// <summary>
    /// Generates phantom income records (e.g. for TIPS).
    /// </summary>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.StdCall)]
    public static extern NativeERRSTRUCT GeneratePhantomIncome(
        int valDate,
        [MarshalAs(UnmanagedType.LPStr)] string mode,
        int id,
        [MarshalAs(UnmanagedType.LPStr)] string secNo,
        [MarshalAs(UnmanagedType.LPStr)] string wi,
        [MarshalAs(UnmanagedType.LPStr)] string secXtend,
        [MarshalAs(UnmanagedType.LPStr)] string acctType,
        int transNo);
}
