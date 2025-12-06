# PowerShell script to generate DEF file from existing DLL
# Uses dumpbin to extract exports and create a DEF file

param(
    [Parameter(Mandatory = $false)]
    [string]$DllPath = "..\Debug\OLEDBIO.dll",
    
    [Parameter(Mandatory = $false)]
    [string]$OutputFile = "OLEDBIO_from_dll.def",
    
    [Parameter(Mandatory = $false)]
    [switch]$IncludeOrdinals = $false
)

if (-not (Test-Path $DllPath)) {
    Write-Host "Error: DLL not found at: $DllPath" -ForegroundColor Red
    Write-Host "Please provide a valid DLL path" -ForegroundColor Yellow
    exit 1
}

Write-Host "`n=== DEF File Generator from DLL ===" -ForegroundColor Cyan
Write-Host "DLL: $DllPath" -ForegroundColor Green
Write-Host "Output: $OutputFile`n" -ForegroundColor Green

# Get DLL name
$dllName = [System.IO.Path]::GetFileNameWithoutExtension($DllPath)

# Find dumpbin.exe
Write-Host "Locating dumpbin.exe..." -ForegroundColor Yellow

$dumpbinPath = $null

# Try to find dumpbin in PATH first
$dumpbinCmd = Get-Command dumpbin -ErrorAction SilentlyContinue
if ($dumpbinCmd) {
    $dumpbinPath = $dumpbinCmd.Source
}

# If not in PATH, search common Visual Studio locations
if (-not $dumpbinPath) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath
        if ($vsPath) {
            # Search for dumpbin in VS installation
            $searchPaths = @(
                "$vsPath\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
                "$vsPath\VC\Tools\MSVC\*\bin\Hostx86\x86\dumpbin.exe",
                "$vsPath\VC\bin\dumpbin.exe"
            )
            
            foreach ($pattern in $searchPaths) {
                $found = Get-ChildItem $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
                if ($found) {
                    $dumpbinPath = $found.FullName
                    break
                }
            }
        }
    }
}

if (-not $dumpbinPath) {
    Write-Host "Error: dumpbin.exe not found" -ForegroundColor Red
    Write-Host "Please run this script from a Visual Studio Developer Command Prompt" -ForegroundColor Yellow
    Write-Host "Or ensure Visual Studio is installed with C++ tools" -ForegroundColor Yellow
    exit 1
}

Write-Host "Using dumpbin: $dumpbinPath" -ForegroundColor Green
Write-Host "Running dumpbin..." -ForegroundColor Yellow

$dumpbinOutput = & $dumpbinPath /EXPORTS $DllPath 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: dumpbin failed" -ForegroundColor Red
    Write-Host "Output: $dumpbinOutput" -ForegroundColor Yellow
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
            Ordinal = [int]$matches[1]
            Name    = $matches[4].Trim()
        }
    }
}

if ($exports.Count -eq 0) {
    Write-Host "Warning: No exports found in DLL" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found $($exports.Count) exported functions" -ForegroundColor Green

# Generate DEF file
$defContent = @"
LIBRARY "$dllName"
DESCRIPTION "Auto-generated from $dllName.dll on $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"

EXPORTS
    ; Total exports: $($exports.Count)
    
"@

# Sort exports by name
$sortedExports = $exports | Sort-Object Name

foreach ($export in $sortedExports) {
    if ($IncludeOrdinals) {
        $defContent += "    $($export.Name) @$($export.Ordinal)`r`n"
    }
    else {
        $defContent += "    $($export.Name)`r`n"
    }
}

# Save DEF file
$defContent | Out-File -FilePath $OutputFile -Encoding ASCII

Write-Host "`nDEF file generated successfully!" -ForegroundColor Green
Write-Host "Output file: $OutputFile" -ForegroundColor Cyan

# Show preview
Write-Host "`nPreview (first 30 lines):" -ForegroundColor Yellow
Write-Host ("=" * 80) -ForegroundColor Gray
Get-Content $OutputFile | Select-Object -First 30
if ($exports.Count -gt 25) {
    Write-Host "..." -ForegroundColor Gray
    Write-Host "($($exports.Count - 25) more exports...)" -ForegroundColor Gray
}
Write-Host ("=" * 80) -ForegroundColor Gray

# Create comparison file with ordinals
if (-not $IncludeOrdinals) {
    $ordinalFile = [System.IO.Path]::GetFileNameWithoutExtension($OutputFile) + "_with_ordinals.def"
    $ordinalContent = $defContent -replace "EXPORTS\r\n    ; Total exports:", "EXPORTS`r`n    ; With ordinals for reference`r`n    ; Total exports:"
    $ordinalContent = "LIBRARY `"$dllName`"`r`nDESCRIPTION `"Auto-generated with ordinals`"`r`n`r`nEXPORTS`r`n"
    foreach ($export in $sortedExports) {
        $ordinalContent += "    $($export.Name) @$($export.Ordinal)`r`n"
    }
    $ordinalContent | Out-File -FilePath $ordinalFile -Encoding ASCII
    Write-Host "`nVersion with ordinals saved to: $ordinalFile" -ForegroundColor Cyan
}

Write-Host "`nUsage Instructions:" -ForegroundColor Yellow
Write-Host "1. Review the generated DEF file" -ForegroundColor White
Write-Host "2. Add it to your Visual Studio project" -ForegroundColor White
Write-Host "3. Configure: Project Properties → Linker → Input → Module Definition File" -ForegroundColor White
Write-Host "4. Rebuild your project" -ForegroundColor White

Write-Host "`nNote: If using DEF file, you can remove __declspec(dllexport) from source code" -ForegroundColor Cyan
