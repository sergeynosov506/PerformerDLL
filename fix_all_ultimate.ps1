# ULTIMATE comprehensive fix for ALL remaining STDCALL issues
Write-Host "`n=== ULTIMATE STDCALL Fix - All Remaining Files ===" -ForegroundColor Cyan

# Get all .h and .cpp files
$allFiles = Get-ChildItem "E:\projects\PerformerDLL\Source" -Include *.h, *.cpp -Recurse | Where-Object { $_.Name -notlike "*.bak" }

$totalFixed = 0

foreach ($file in $allFiles) {
    $content = Get-Content $file.FullName -Raw
    $originalContent = $content
    
    # Replace all patterns - be comprehensive
    $content = $content -replace 'DLLAPI void ([A-Z])', 'DLLAPI void STDCALL $1'
    $content = $content -replace 'DLLAPI int ([A-Z])', 'DLLAPI int STDCALL $1'
    $content = $content -replace 'DLLAPI long ([A-Z])', 'DLLAPI long STDCALL $1'
    $content = $content -replace 'DLLAPI double ([A-Z])', 'DLLAPI double STDCALL $1'
    $content = $content -replace 'DLLAPI BOOL ([A-Z])', 'DLLAPI BOOL STDCALL $1'
    $content = $content -replace 'DLLAPI ERRSTRUCT  ([A-Z])', 'DLLAPI ERRSTRUCT STDCALL $1'
    $content = $content -replace 'DLLAPI ERRSTRUCT ([A-Z])', 'DLLAPI ERRSTRUCT STDCALL $1'
    # Fix int with tab or double space
    $content = $content -replace 'DLLAPI int\t ([A-Z])', 'DLLAPI int STDCALL $1'
    $content = $content -replace 'DLLAPI int  ([A-Z])', 'DLLAPI int STDCALL $1'
    
    if ($content -ne $originalContent) {
        Set-Content $file.FullName $content -NoNewline
        Write-Host "  ✅ $($file.Name)" -ForegroundColor Green
        $totalFixed++
    }
}

Write-Host "`n✅ Fixed $totalFixed files total" -ForegroundColor Green
Write-Host "`nRunning final verification..." -ForegroundColor Cyan
