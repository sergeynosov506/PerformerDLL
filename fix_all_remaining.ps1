# Comprehensive STDCALL fix script for all remaining files
Write-Host "`n=== Fixing All Remaining STDCALL Issues ===" -ForegroundColor Cyan

$files = @(
    "PerformanceIO_UnitValue.h",
    "PerformanceIO_UnitValue.cpp",
    "RollIO.h",
    "RollIO.cpp",
    "RollIO_Account.h",
    "RollIO_Account.cpp",
    "RollIO_Holdmap.h",
    "RollIO_Holdmap.cpp",
    "RollIO_Portmain.h",
    "RollIO_Portmain.cpp",
    "RollIO_Position.h",
    "RollIO_Position.cpp",
    "RollIO_Trans.h",
    "RollIO_Trans.cpp",
    "tadbrtns.cpp"
)

$totalFixed = 0

foreach ($fileName in $files) {
    $filePath = "E:\projects\PerformerDLL\Source\$fileName"
    
    if (Test-Path $filePath) {
        $content = Get-Content $filePath -Raw
        $originalContent = $content
        
        # Replace patterns: DLLAPI <type> FunctionName with DLLAPI <type> STDCALL FunctionName
        $content = $content -replace 'DLLAPI void ([A-Z])', 'DLLAPI void STDCALL $1'
        $content = $content -replace 'DLLAPI int ([A-Z])', 'DLLAPI int STDCALL $1'
        $content = $content -replace 'DLLAPI long ([A-Z])', 'DLLAPI long STDCALL $1'
        $content = $content -replace 'DLLAPI double ([A-Z])', 'DLLAPI double STDCALL $1'
        $content = $content -replace 'DLLAPI BOOL ([A-Z])', 'DLLAPI BOOL STDCALL $1'
        $content = $content -replace 'DLLAPI ERRSTRUCT  ([A-Z])', 'DLLAPI ERRSTRUCT STDCALL $1'
        $content = $content -replace 'DLLAPI ERRSTRUCT ([A-Z])', 'DLLAPI ERRSTRUCT STDCALL $1'
        
        if ($content -ne $originalContent) {
            Set-Content $filePath $content -NoNewline
            Write-Host "  ✅ Fixed $fileName" -ForegroundColor Green
            $totalFixed++
        }
        else {
            Write-Host "  ⏭️  Skipped $fileName (no changes needed)" -ForegroundColor Gray
        }
    }
    else {
        Write-Host "  ⚠️  File not found: $fileName" -ForegroundColor Yellow
    }
}

Write-Host "`n✅ Fixed $totalFixed files" -ForegroundColor Green
Write-Host "`nRunning verification..." -ForegroundColor Cyan
