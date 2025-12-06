# PowerShell script to verify DLL export signatures
# Checks for common issues with DLLAPI/STDCALL usage

Write-Host "`n=== DLL Export Signature Verification ===" -ForegroundColor Cyan
Write-Host "Checking for signature issues...`n" -ForegroundColor Green

$issues = @()
$warnings = @()

# Check 1: Double extern "C"
Write-Host "1. Checking for double 'extern `"C`"' declarations..." -ForegroundColor Yellow
$doubleExternC = Get-ChildItem -Path . -Include *.cpp, *.h -Recurse | 
Select-String -Pattern 'extern\s+"C"\s+DLLAPI' -CaseSensitive

if ($doubleExternC) {
    $issues += "Found double 'extern `"C`"' declarations:"
    foreach ($match in $doubleExternC) {
        $issues += "  - $($match.Filename):$($match.LineNumber): $($match.Line.Trim())"
    }
}
else {
    Write-Host "   ✅ No double 'extern `"C`"' found" -ForegroundColor Green
}

# Check 2: Mixed __stdcall
Write-Host "2. Checking for mixed '__stdcall' usage..." -ForegroundColor Yellow
$mixedStdcall = Get-ChildItem -Path . -Include *.cpp, *.h -Recurse | 
Select-String -Pattern 'DLLAPI.*__stdcall' -CaseSensitive

if ($mixedStdcall) {
    $warnings += "Found mixed '__stdcall' usage (should use STDCALL macro):"
    foreach ($match in $mixedStdcall) {
        $warnings += "  - $($match.Filename):$($match.LineNumber): $($match.Line.Trim())"
    }
}
else {
    Write-Host "   ✅ No mixed '__stdcall' found" -ForegroundColor Green
}

# Check 3: Direct __declspec usage
Write-Host "3. Checking for direct '__declspec(dllexport)' usage..." -ForegroundColor Yellow
$directDeclspec = Get-ChildItem -Path . -Include *.cpp, *.h -Recurse | 
Select-String -Pattern '__declspec\(dllexport\).*__stdcall' -CaseSensitive |
Where-Object { $_.Line -notmatch '#define\s+DLLAPI' }

if ($directDeclspec) {
    $warnings += "Found direct '__declspec(dllexport)' usage (should use DLLAPI macro):"
    foreach ($match in $directDeclspec) {
        $warnings += "  - $($match.Filename):$($match.LineNumber): $($match.Line.Trim())"
    }
}
else {
    Write-Host "   ✅ No direct '__declspec(dllexport)' found" -ForegroundColor Green
}

# Check 4: Missing STDCALL in DLLAPI functions
Write-Host "4. Checking for missing 'STDCALL' in DLLAPI functions..." -ForegroundColor Yellow
$missingStdcall = Get-ChildItem -Path . -Include *.cpp, *.h -Recurse | 
Select-String -Pattern 'DLLAPI\s+\w+\s+(?!STDCALL)\w+\s*\(' -CaseSensitive |
Where-Object { 
    $_.Line -notmatch '#define' -and 
    $_.Line -notmatch '//' -and 
    $_.Line -notmatch 'typedef' -and
    $_.Line -match 'DLLAPI'
}

if ($missingStdcall) {
    $issues += "`nFound DLLAPI functions missing 'STDCALL' macro:"
    foreach ($match in $missingStdcall) {
        $issues += "  - $($match.Filename):$($match.LineNumber): $($match.Line.Trim())"
    }
}
else {
    Write-Host "   ✅ All DLLAPI functions have STDCALL" -ForegroundColor Green
}

# Check 5: Count DLLAPI functions
Write-Host "`n5. Counting exported functions..." -ForegroundColor Yellow
$dllApiFunctions = Get-ChildItem -Path . -Include *.h -Recurse | 
Select-String -Pattern 'DLLAPI.*STDCALL' -CaseSensitive

$functionCount = ($dllApiFunctions | Measure-Object).Count
Write-Host "   Found $functionCount DLLAPI function declarations" -ForegroundColor Cyan

# Summary
Write-Host "`n=== Summary ===" -ForegroundColor Cyan
if ($issues.Count -eq 0 -and $warnings.Count -eq 0) {
    Write-Host "✅ All signatures are correct!" -ForegroundColor Green
    Write-Host "✅ No issues found" -ForegroundColor Green
    Write-Host "✅ $functionCount functions using correct DLLAPI/STDCALL pattern" -ForegroundColor Green
}
else {
    if ($issues.Count -gt 0) {
        Write-Host "`n❌ ISSUES FOUND ($($issues.Count)):" -ForegroundColor Red
        foreach ($issue in $issues) {
            Write-Host $issue -ForegroundColor Red
        }
    }
    
    if ($warnings.Count -gt 0) {
        Write-Host "`n⚠️  WARNINGS ($($warnings.Count)):" -ForegroundColor Yellow
        foreach ($warning in $warnings) {
            Write-Host $warning -ForegroundColor Yellow
        }
    }
}

Write-Host "`n=== Recommendations ===" -ForegroundColor Cyan
Write-Host "✅ Always use: DLLAPI void STDCALL FunctionName(params);" -ForegroundColor White
Write-Host "❌ Never use: extern `"C`" DLLAPI void STDCALL FunctionName(params);" -ForegroundColor White
Write-Host "❌ Never use: DLLAPI void __stdcall FunctionName(params);" -ForegroundColor White
Write-Host "❌ Never use: DLLAPI void FunctionName(params); // Missing STDCALL!" -ForegroundColor White

Write-Host "`nVerification complete!`n" -ForegroundColor Green
