# PowerShell script to list DLL exports
# Usage: .\list_dll_exports.ps1 path\to\your.dll

param(
    [Parameter(Mandatory=$false)]
    [string]$DllPath = ".\Debug\OLEDBIO.dll"
)

if (-not (Test-Path $DllPath)) {
    Write-Host "Error: DLL not found at: $DllPath" -ForegroundColor Red
    Write-Host "Please provide a valid DLL path" -ForegroundColor Yellow
    exit 1
}

Write-Host "`n=== DLL Export Analysis ===" -ForegroundColor Cyan
Write-Host "DLL: $DllPath`n" -ForegroundColor Green

# Use dumpbin to get exports
$dumpbinOutput = & dumpbin /EXPORTS $DllPath 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: dumpbin failed. Make sure you're running from a Visual Studio Developer Command Prompt" -ForegroundColor Red
    exit 1
}

# Parse the output
$exports = @()
$inExportSection = $false

foreach ($line in $dumpbinOutput) {
    if ($line -match "ordinal\s+hint\s+RVA\s+name") {
        $inExportSection = $true
        continue
    }
    
    if ($inExportSection -and $line -match "^\s+(\d+)\s+([0-9A-F]+)\s+([0-9A-F]+)\s+(.+)$") {
        $exports += [PSCustomObject]@{
            Ordinal = $matches[1]
            Name = $matches[4].Trim()
        }
    }
}

# Display results
Write-Host "Total Exported Functions: $($exports.Count)" -ForegroundColor Yellow
Write-Host "`nExported Functions:" -ForegroundColor Cyan
Write-Host ("=" * 80) -ForegroundColor Gray

$exports | Sort-Object Name | Format-Table -AutoSize Ordinal, Name

# Save to file
$outputFile = "dll_exports_$(Get-Date -Format 'yyyyMMdd_HHmmss').txt"
$exports | Sort-Object Name | Out-File $outputFile
Write-Host "`nExports saved to: $outputFile" -ForegroundColor Green
