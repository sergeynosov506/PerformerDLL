# Missing STDCALL Fix Report

## Summary

The verification script found **78 functions** missing the `STDCALL` macro across multiple files.

## Impact

Functions without `STDCALL` will:
- ❌ Use wrong calling convention (`__cdecl` instead of `__stdcall`)
- ❌ Export with mangled/decorated names
- ❌ Cause stack corruption (caller/callee disagree on cleanup)
- ❌ Be incompatible with Delphi code

## Files Requiring Fixes

### 1. TransIO_Currency (FIXED ✅)
- **File:** `TransIO_Currency.h` and `TransIO_Currency.cpp`
- **Function:** `SelectCurrencySecno`
- **Status:** FIXED

### 2. TransIO_Income (3 functions)
- **File:** `TransIO_Income.h` and `TransIO_Income.cpp`
- **Functions:**
  - `GetLastIncomeDate`
  - `GetLastIncomeDateMIPS`
  - `GetIncomeForThePeriod`

### 3. TransImportIO (~30 functions)
- **File:** `TransImportIO.h` and `TransImportIO.cpp`
- **Functions:** (partial list)
  - `SelectPortClos`
  - `InsertDTrans`
  - `InsertDTrnDesc`
  - `InsertDPayTran`
  - `SelectTransNoByDtransNo`
  - `SelectTransNoByUnitsAndDates`
  - `SelectMapTransNoEx`
  - `InsertMapTransNoEx`
  - `UpdateMapTransNoEx`
  - `DeleteMapTransNoEx`
  - `SelectMapTransNoExByTransNo`
  - `InsertTaxlotRecon`
  - `InsertOneTaxlotRecon`
  - `InsertAllTaxlotRecon`
  - `InsertTradeExchange`
  - `UpdateTradeExchangeByPK`
  - `InsertBnkSet`
  - `InsertBnkSetEx`
  - `UpdateBnkSetExRevNo`
  - `SelectBnksetexNoByUnitsAndDates`
  - `GrouppedHoldingsFor`

### 4. ValuationIO (~44 functions)
- **File:** `ValuationIO.cpp` (legacy file, may need refactoring)
- **Functions:** (partial list)
  - `SelectAllAccdivForAnAccount`
  - `SelectPendingAccdivTransForAnAccount`
  - `SelectUnitsForASoldSecurity`
  - `SelectAllDivhistForAnAccount`
  - `SelectNextCallDatePrice`
  - `SelectPartEquities`
  - `SelectAllHedgxrefForAnAccount`
  - `SelectAllHoldcashForAnAccount`
  - `SelectAllHoldingsForAnAccount`
  - `SelectUnitsHeldForASecurity`
  - `SelectOneHistfinc`
  - `SelectOneHisteqty`
  - `SelectOneHistpric`
  - `SelectAllPayrecForAnAccount`
  - `UpdatePayrec`
  - `UpdatePortmainValDate`
  - `SelectAllForHoldTot`
  - `SelectAllRatings`
  - `IsAssetInternational`
  - `GetInterSegID`
  - `SelectSegmentIdFromSegMap`
  - `SelectSegmentLevelId`
  - `SelectUnsupervised`
  - `SelectUnsupervisedSegmentId`
  - `SelectPledgedSegment`
  - `SelectSegment`
  - `SelectDivint`
  - `SelectAllSegments`
  - `SelectAllSegmap`
  - `SelectAdhocPortmain`
  - `SelectSecurityRate`
  - `IsManualAccrInt`
  - `SelectCustomPricesForAnAccount`
  - `InitializeValuationIO`
  - `FreeValuationIO`

## Fix Pattern

For each function, add `STDCALL` between return type and function name:

### Before (Wrong):
```cpp
// Header:
DLLAPI void FunctionName(params);

// Implementation:
DLLAPI void FunctionName(params) { ... }
```

### After (Correct):
```cpp
// Header:
DLLAPI void STDCALL FunctionName(params);

// Implementation:
DLLAPI void STDCALL FunctionName(params) { ... }
```

## Automated Fix Strategy

### Option 1: Manual Fix (Recommended for Critical Files)
Fix `TransIO_Income` and `TransImportIO` manually to ensure correctness.

### Option 2: Search and Replace
Use careful find/replace in each file:

**Find:** `DLLAPI (void|int|long|double|BOOL|ERRSTRUCT) (\w+)\(`  
**Replace:** `DLLAPI $1 STDCALL $2(`

⚠️ **Warning:** Review each change carefully!

### Option 3: Script-Based Fix
Create a PowerShell script to add STDCALL automatically (risky, needs review).

## Priority

### High Priority (Fix Immediately):
1. ✅ `TransIO_Currency` - FIXED
2. `TransIO_Income` - 3 functions
3. `TransImportIO` - ~30 functions

### Medium Priority:
4. `ValuationIO` - ~44 functions (legacy file, may be replaced)

## Verification

After fixing, run:
```powershell
.\verify_dll_signatures.ps1
```

Should show:
```
✅ All DLLAPI functions have STDCALL
✅ All signatures are correct!
```

## Next Steps

1. Fix `TransIO_Income.h` and `TransIO_Income.cpp`
2. Fix `TransImportIO.h` and `TransImportIO.cpp`
3. Decide on `ValuationIO.cpp` (fix or replace with refactored modules)
4. Re-run verification script
5. Rebuild and test DLL exports

## Notes

- All fixes must be applied to BOTH header (.h) and implementation (.cpp) files
- Verify exports with `dumpbin /EXPORTS OLEDBIO.dll` after building
- Test with Delphi code to ensure compatibility
