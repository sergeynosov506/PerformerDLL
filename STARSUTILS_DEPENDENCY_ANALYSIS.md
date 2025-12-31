# StarsUtils.dll Dependency Analysis

## Status: ✅ Successfully Removed from TransEngine and CalcGainLoss

### What Was Fixed:
1. **TransEngine.dll** - Now loads date functions from OLEDBIO.dll instead of StarsUtils.dll
2. **CalcGainLoss.dll** - Now loads date functions from OLEDBIO.dll instead of StarsUtils.dll
3. **Roll.dll** - Already using OLEDBIO.dll for date functions

### Verification:
- ✅ Test passed **without** StarsUtils.dll in bin folder (12ms)
- ✅ All date functions (`rmdyjul`, `rjulmdy`, `NewDateFromCurrent`, `rstrdate`) work correctly from OLEDBIO.dll

---

## DLLs Still Referencing StarsUtils.dll

The following DLLs **still load StarsUtils.dll** and need to be updated:

### 1. **Performance.dll** (`perfdll.cpp`)
**Location:** Line 108-111, 788-834  
**Functions Used:**
- `rmdyjul`, `rjulmdy`
- `IsItAMarketHoliday`
- `CurrentMonthEnd`, `LastMonthEnd`, `IsItAMonthEnd`
- `CurrentDateAndTime`

**Action Needed:** Redirect these function loads to OLEDBIO.dll

---

### 2. **Digenerate.dll** (`digenerate2.cpp`)
**Location:** Lines 80-83, 456-486  
**Functions Used:**
- `rmdyjul`, `rjulmdy`, `rstrdate`
- `NextBusinessDay`
- `LastMonthEnd`

**Action Needed:** Redirect these function loads to OLEDBIO.dll

---

### 3. **CompositeCreate.dll** (`CompositeCreate.cpp`)
**Location:** Lines 542-543, 1047-1055  
**Functions Used:**
- `CurrentDateAndTime`
- `LastMonthEnd`

**Action Needed:** Redirect these function loads to OLEDBIO.dll

---

### 4. **Valuation.dll** (`value2.cpp`)
**Location:** Lines 44-45, 403-419  
**Functions Used:**
- `LastBusinessDay`
- `NextBusinessDay`
- `IsItAMarketHoliday`

**Action Needed:** Redirect these function loads to OLEDBIO.dll

---

## Functions That Need to be in OLEDBIO.dll

The following functions are currently in StarsUtils.dll but NOT yet in OLEDBIO.dll:

❌ **Missing from OLEDBIO:**
- `IsItAMarketHoliday`
- `CurrentMonthEnd`
- `LastMonthEnd`
- `IsItAMonthEnd`
- `CurrentDateAndTime`
- `LastBusinessDay`
- `NextBusinessDay`

✅ **Already in OLEDBIO:**
- `rmdyjul`
- `rjulmdy`
- `NewDateFromCurrent`
- `rstrdate`

---

## Next Steps

To completely remove StarsUtils.dll dependency from the entire project:

1. **Implement missing date/calendar functions in OLEDBIO.dll:**
   - Business day functions (Last/Next)
   - Month end functions
   - Market holiday checks
   - Current date/time

2. **Update each DLL to use OLEDBIO.dll instead of StarsUtils.dll:**
   - Performance.dll
   - Digenerate.dll
   - CompositeCreate.dll
   - Valuation.dll

3. **Run cleanup script** to remove StarsUtils.dll files from deployment folders

---

## Files Found

StarsUtils.dll was found in the following locations:
- `e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\StarsUtils.dll` ❌ Deleted
- `e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\x64\StarsUtils.dll`
- `e:\projects\PerformerDLL\Roll\Roll.Tests\bin\x64\Debug\net8.0\StarsUtils.dll`
