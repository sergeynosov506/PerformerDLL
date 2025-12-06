# PowerShell script to generate DEF file from source code
# Scans header files for DLLAPI declarations

param(
    [Parameter(Mandatory=$false)]
    [string]$SourcePath = ".",
    
    [Parameter(Mandatory=$false)]
    [string]$OutputFile = "OLEDBIO.def",
    
    [Parameter(Mandatory=$false)]
    [string]$LibraryName = "OLEDBIO"
)

Write-Host "`n=== DEF File Generator ===" -ForegroundColor Cyan
Write-Host "Scanning source path: $SourcePath" -ForegroundColor Green
Write-Host "Output file: $OutputFile`n" -ForegroundColor Green

# Find all header files
$headerFiles = Get-ChildItem -Path $SourcePath -Filter "*.h" -Recurse

$exports = @()
$functionPattern = 'DLLAPI\s+(?:[\w\*]+\s+)+(?:STDCALL\s+)?(\w+)\s*\('

foreach ($file in $headerFiles) {
    $content = Get-Content $file.FullName -Raw
    
    # Find all DLLAPI function declarations
    $matches = [regex]::Matches($content, $functionPattern)
    
    foreach ($match in $matches) {
        $functionName = $match.Groups[1].Value
        if ($functionName -and $functionName -ne "STDCALL") {
            $exports += [PSCustomObject]@{
                Name = $functionName
                File = $file.Name
            }
        }
    }
}

# Remove duplicates and sort
$uniqueExports = $exports | Sort-Object Name -Unique

Write-Host "Found $($uniqueExports.Count) exported functions" -ForegroundColor Yellow

# Generate DEF file content
$defContent = @"
LIBRARY "$LibraryName"
DESCRIPTION "OLEDB Input/Output Library - Auto-generated DEF file"

EXPORTS
    ; Auto-generated from source code on $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
    ; Total exports: $($uniqueExports.Count)
    
"@

# Group by prefix for better organization
$groups = $uniqueExports | Group-Object { 
    if ($_.Name -match '^(Initialize|Free|Close|Open|Connect)') { return "Initialization" }
    elseif ($_.Name -match '^(Select|Get|Read)') { return "Query" }
    elseif ($_.Name -match '^(Insert|Update|Delete|Write)') { return "Modification" }
    elseif ($_.Name -match '^(Calculate|Compute|Process)') { return "Processing" }
    else { return "Other" }
}

$ordinal = 1
foreach ($group in ($groups | Sort-Object Name)) {
    $defContent += "    ; $($group.Name) Functions`r`n"
    
    foreach ($export in ($group.Group | Sort-Object Name)) {
        $defContent += "    $($export.Name)`r`n"
        # Uncomment next line to add ordinals:
        # $defContent += "    $($export.Name) @$ordinal`r`n"
        $ordinal++
    }
    
    $defContent += "`r`n"
}

# Save DEF file
$defContent | Out-File -FilePath $OutputFile -Encoding ASCII

Write-Host "`nDEF file generated: $OutputFile" -ForegroundColor Green
Write-Host "`nPreview:" -ForegroundColor Cyan
Write-Host ("=" * 80) -ForegroundColor Gray
Get-Content $OutputFile | Select-Object -First 30
Write-Host "..." -ForegroundColor Gray
Write-Host ("=" * 80) -ForegroundColor Gray

# Also create a summary report
$reportFile = "export_summary.txt"
$reportContent = @"
Export Summary Report
Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

Total Exported Functions: $($uniqueExports.Count)

Functions by Category:
"@

foreach ($group in ($groups | Sort-Object Name)) {
    $reportContent += "`n`n$($group.Name) ($($group.Count)):`n"
    $reportContent += "-" * 40 + "`n"
    foreach ($export in ($group.Group | Sort-Object Name)) {
        $reportContent += "  $($export.Name) (from $($export.File))`n"
    }
}

$reportContent | Out-File -FilePath $reportFile
Write-Host "`nDetailed report saved to: $reportFile" -ForegroundColor Green

Write-Host "`nTo use this DEF file:" -ForegroundColor Yellow
Write-Host "1. Add $OutputFile to your Visual Studio project" -ForegroundColor White
Write-Host "2. Project Properties → Linker → Input → Module Definition File" -ForegroundColor White
Write-Host "3. Enter: $OutputFile" -ForegroundColor White
Write-Host "4. Remove __declspec(dllexport) from code (keep extern `"C`")" -ForegroundColor White
