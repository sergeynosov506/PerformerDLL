using System;
using System.Runtime.InteropServices;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for YldDurtn.dll - Bond yield and duration calculations.
/// Contains functions for bond pricing, yield calculations, and risk metrics (duration, convexity).
/// </summary>
/// <remarks>
/// YldDurtn.dll handles:
/// - Bond yield calculations (price to yield, yield to price)
/// - Macaulay duration and modified duration
/// - Bond cash flow generation
/// - Accrued interest calculations
/// - Day count conventions
/// 
/// Note: Most functions require a blBond structure which contains bond characteristics
/// (coupon, maturity, settlement date, daycount conventions, etc.)
/// </remarks>
public static class YieldDuration
{
    private const string DLL_NAME = "YldDurtn.dll";

    /// <summary>
    /// Represents a bond structure used by the yield/duration functions.
    /// This is an opaque pointer - the actual structure is managed by the unmanaged DLL.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct BlBond
    {
        public IntPtr Pointer;
    }

    #region Yield Calculations

    /// <summary>
    /// Calculate yield given bond price.
    /// </summary>
    /// <param name="price">Bond price (as percentage of par, e.g., 100.0 for par)</param>
    /// <param name="bond">Pointer to bond structure</param>
    /// <returns>Yield as a decimal (e.g., 0.05 for 5%)</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blPriceToYield(
        double price,
        IntPtr bond);

    /// <summary>
    /// Calculate price given bond yield.
    /// </summary>
    /// <param name="yield">Yield as a decimal (e.g., 0.05 for 5%)</param>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="accruedInterest">Pointer to receive accrued interest</param>
    /// <returns>Bond price (as percentage of par)</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blYieldToPrice(
        double yield,
        IntPtr bond,
        out double accruedInterest);

    #endregion

    #region Duration Calculations

    /// <summary>
    /// Calculate Macaulay duration from bond price and yield.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="price">Bond price</param>
    /// <param name="yield">Bond yield</param>
    /// <returns>Macaulay duration in years</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blMacaulayDuration(
        IntPtr bond,
        double price,
        double yield);

    /// <summary>
    /// Calculate Macaulay duration from bond price (yield calculated internally).
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="price">Bond price</param>
    /// <returns>Macaulay duration in years</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blMacaulayDurationFromPrice(
        IntPtr bond,
        double price);

    /// <summary>
    /// Calculate Macaulay duration from bond yield (price calculated internally).
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="yield">Bond yield</param>
    /// <returns>Macaulay duration in years</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blMacaulayDurationFromYield(
        IntPtr bond,
        double yield);

    /// <summary>
    /// Calculate modified duration from bond price and yield.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="price">Bond price</param>
    /// <param name="yield">Bond yield</param>
    /// <returns>Modified duration</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blModifiedDuration(
        IntPtr bond,
        double price,
        double yield);

    /// <summary>
    /// Calculate modified duration from bond price.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="price">Bond price</param>
    /// <returns>Modified duration</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blModifiedDurationFromPrice(
        IntPtr bond,
        double price);

    /// <summary>
    /// Calculate modified duration from bond yield.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    /// <param name="yield">Bond yield</param>
    /// <returns>Modified duration</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern double blModifiedDurationFromYield(
        IntPtr bond,
        double yield);

    #endregion

    #region Bond Setup and Cash Flows

    /// <summary>
    /// Create bond cash flows for a municipal bond.
    /// </summary>
    /// <param name="issueDate">Issue date (YYYYMMDD format)</param>
    /// <param name="maturityDate">Maturity date (YYYYMMDD format)</param>
    /// <param name="settlementDate">Settlement date (YYYYMMDD format)</param>
    /// <param name="couponRate">Annual coupon rate (e.g., 0.05 for 5%)</param>
    /// <returns>Pointer to bond structure</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr blMuniBondCashFlows(
        int issueDate,
        int maturityDate,
        int settlementDate,
        double couponRate);

    /// <summary>
    /// Setup daycount conventions for a bond.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void blSetupDaycountConventions(IntPtr bond);

    /// <summary>
    /// Compute cash flows from bond characteristic dates.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void blComputeCashFlowsFromDates(IntPtr bond);

    /// <summary>
    /// Compute time to cash flows for duration calculations.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void blComputeTimeToCashFlows(IntPtr bond);

    /// <summary>
    /// Calculate accrued interest for a bond.
    /// </summary>
    /// <param name="bond">Pointer to bond structure</param>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void blCalcAI(IntPtr bond);

    #endregion

    #region Date Utilities

    /// <summary>
    /// Convert Julian date to long date format (YYYYMMDD).
    /// </summary>
    /// <param name="julianDate">Julian date</param>
    /// <param name="offset">Offset to apply</param>
    /// <returns>Date in YYYYMMDD format</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int dlJulianToLong(
        int julianDate,
        int offset);

    #endregion
}
