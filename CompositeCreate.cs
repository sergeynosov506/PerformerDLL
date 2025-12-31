using System;
using System.Runtime.InteropServices;
using PerformerDLL.Interop.Common;

namespace PerformerDLL.Source
{
    public static class CompositeCreate
    {
        private const string DllName = "CompositeCreate.dll";

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void InitializeDllRoutines(
            int lAsofDate, 
            string sDBAlias, 
            string sMode, 
            string sType, 
            bool bPrepareQueries, 
            string sErrFile, 
            out NativeERRSTRUCT zErr);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern void CreateComposite(
            int iOwnerID, 
            int lDate, 
            int iPortfolioType, 
            int lHoldingsFlag, 
            out NativeERRSTRUCT zErr);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        public static extern NativeERRSTRUCT MergeCompositePortfolio(
            int iID, 
            int lFromDate, 
            int lToDate, 
            bool bDaily, 
            int lEarliestDate, 
            string sInSessionID);
    }
}
