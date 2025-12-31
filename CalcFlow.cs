using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Interop.Wrappers;

/// <summary>
/// P/Invoke wrapper for CalcFlow.dll - Cash flow, yield, and amortization calculations.
/// Contains 3 exported functions for fixed income analytics.
/// </summary>
public static class CalcFlow
{
    private const string DLL_NAME = "CalcFlow.dll";

    #region Structures

    /// <summary>
    /// Cash flow calculation results structure.
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct FLOWCALCSTRUCT
    {
        public double NetFlow;
        public double CashFlow;
        public double AccruedInterest;
        public double Principal;
        public double InterestIncome;
        public double Fees;
        public double Commissions;
        
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string CalculationDetails;
    }

    #endregion

    #region Initialization

    /// <summary>
    /// Initializes the CalcFlow engine with database connection.
    /// </summary>
    /// <param name="connectionString">ODBC connection string</param>
    /// <returns>0 if successful, error code otherwise</returns>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern int InitCalcFlow(
        [MarshalAs(UnmanagedType.LPStr)] string connectionString);

    #endregion

    #region Cash Flow Calculations

    /// <summary>
    /// Calculates net flow for a transaction.
    /// </summary>
    /// <param name="trans">Transaction to calculate</param>
    /// <param name="securityData">Security characteristics</param>
    /// <param name="flowResults">Output: calculated flow components</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// Calculates:
    /// - Net cash flow (principal + interest + fees)
    /// - Accrued interest
    /// - Principal amount
    /// - Fee allocations
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern NativeERRSTRUCT CalculateNetFlow(
        TRANS trans,
        [MarshalAs(UnmanagedType.LPStr)] string securityData,
        ref FLOWCALCSTRUCT flowResults);

    /// <summary>
    /// Calculates net flow for a transaction with performance attribution.
    /// </summary>
    /// <param name="trans">Transaction to calculate</param>
    /// <param name="securityData">Security characteristics</param>
    /// <param name="flowResults">Output: calculated flow components</param>
    /// <param name="includePerformance">Include performance calculations</param>
    /// <returns>Error structure</returns>
    /// <remarks>
    /// Extended version that includes performance attribution calculations.
    /// Used for performance reporting and analytics.
    /// </remarks>
    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl, EntryPoint = "CalculateNetFlowPerf")]
    public static extern NativeERRSTRUCT CalculateNetFlowWithPerformance(
        TRANS trans,
        [MarshalAs(UnmanagedType.LPStr)] string securityData,
        ref FLOWCALCSTRUCT flowResults,
        [MarshalAs(UnmanagedType.Bool)] bool includePerformance);

    #endregion

    #region Helper Methods

    /// <summary>
    /// Safely calculates cash flow with error handling.
    /// </summary>
    public static FLOWCALCSTRUCT SafeCalculateFlow(
        TRANS trans,
        string securityData,
        bool includePerformance = false)
    {
        var flowResults = new FLOWCALCSTRUCT();

        try
        {
            NativeERRSTRUCT result;
            
            if (includePerformance)
            {
                result = CalculateNetFlowWithPerformance(
                    trans,
                    securityData,
                    ref flowResults,
                    true);
            }
            else
            {
                result = CalculateNetFlow(
                    trans,
                    securityData,
                    ref flowResults);
            }

            if (!result.IsSuccess)
            {
                throw new NativeInteropException(
                    $"Cash flow calculation failed: {result.FormatError()}",
                    result);
            }

            return flowResults;
        }
        catch (DllNotFoundException)
        {
            throw new NativeInteropException(
                "CalcFlow.dll not found. Ensure it's in the application directory.",
                new NativeERRSTRUCT());
        }
    }

    /// <summary>
    /// Calculates total net flow for multiple transactions.
    /// </summary>
    public static double CalculateTotalNetFlow(
        IEnumerable<TRANS> transactions,
        string securityData)
    {
        double totalFlow = 0;

        foreach (var trans in transactions)
        {
            var flowResults = SafeCalculateFlow(trans, securityData);
            totalFlow += flowResults.NetFlow;
        }

        return totalFlow;
    }

    #endregion
}
