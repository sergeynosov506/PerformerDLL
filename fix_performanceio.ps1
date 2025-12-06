# Final comprehensive fix for ALL remaining STDCALL issues
Write-Host "`n=== Final STDCALL Fix - PerformanceIO Files ===" -ForegroundColor Cyan

$files = @(
    "PerformanceIO.h",
    "PerformanceIO_Holdings.h",
    "PerformanceIO_Holdings.cpp",
    "PerformanceIO_Scripts.h",
    "PerformanceIO_Scripts.cpp",
    "PerformanceIO_SecTrans.h",
    "PerformanceIO_SecTrans.cpp",
    "PerformanceIO_Taxperf.h",
    "PerformanceIO_Taxperf.cpp"
)

$totalFixed = 0

foreach ($fileName in $files) {
    $filePath = "E:\projects\PerformerDLL\Source\$fileName"
    
    if (Test-Path $filePath) {
        $content = Get-Content $filePath -Raw
        $originalContent = $content
        
        # Replace patterns
        $content = $content -replace 'DLLAPI void ([A-Z])', 'DLLAPI void STDCALL $1'
        $content = $content -replace 'DLLAPI int ([A-Z])', 'DLLAPI int STDCALL $1'
        $content = $content -replace 'DLLAPI long ([A-Z])', 'DLLAPI long STDCALL $1'
        $content = $content -replace 'DLLAPI double ([A-Z])', 'DLLAPI double STDCALL $1'
        $content = $content -replace 'DLLAPI BOOL ([A-Z])', 'DLLAPI BOOL STDCALL $1'
        $content = $content -replace 'DLLAPI ERRSTRUCT  ([A-Z])', 'DLLAPI ERRSTRUCT STDCALL $1'
        $content = $content -replace 'DLLAPI ERRSTRUCT ([A-Z])', 'DLLAPI ERRSTRUCT STDCALL $1'
        # Fix int with tab
        $content = $content -replace 'DLLAPI int\t ([A-Z])', 'DLLAPI int STDCALL $1'
        
        if ($content -ne $originalContent) {
            Set-Content $filePath $content -NoNewline
            Write-Host "  ✅ Fixed $fileName" -ForegroundColor Green
            $totalFixed++
        }
    }
}

# Fix the special cases
$content = Get-Content "E:\projects\PerformerDLL\Source\TransImportIO.h" -Raw
$content = $content -replace 'DLLAPI void GrouppedHoldingsFor', 'DLLAPI void STDCALL GrouppedHoldingsFor'
Set-Content "E:\projects\PerformerDLL\Source\TransImportIO.h" $content -NoNewline

$content = Get-Content "E:\projects\PerformerDLL\Source\RollIO.h" -Raw
$content = $content -replace 'DLLAPI int  UnprepareRollQueries', 'DLLAPI int STDCALL UnprepareRollQueries'
Set-Content "E:\projects\PerformerDLL\Source\RollIO.h" $content -NoNewline

Write-Host "`n✅ Fixed $totalFixed PerformanceIO files + 2 special cases" -ForegroundColor Green
