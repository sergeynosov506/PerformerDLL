# Fix duplicate STDCALL issue
Write-Host "`n=== Fixing Duplicate STDCALL ===" -ForegroundColor Cyan

$allFiles = Get-ChildItem "E:\projects\PerformerDLL\Source" -Include *.h, *.cpp -Recurse | Where-Object { $_.Name -notlike "*.bak" }

$totalFixed = 0

foreach ($file in $allFiles) {
    $content = Get-Content $file.FullName -Raw
    $originalContent = $content
    
    # Fix duplicate STDCALL
    $content = $content -replace 'STDCALL\s+STDCALL', 'STDCALL'
    
    if ($content -ne $originalContent) {
        Set-Content $file.FullName $content -NoNewline
        Write-Host "  ✅ $($file.Name)" -ForegroundColor Green
        $totalFixed++
    }
}

Write-Host "`n✅ Fixed $totalFixed files with duplicate STDCALL" -ForegroundColor Green
Write-Host "`nNow rebuild your DLL and the exports should be clean!" -ForegroundColor Cyan
