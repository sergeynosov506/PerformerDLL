# Generate a clean DEF file from current DLL exports
# This creates a DEF file with clean names (no decoration)

$dllPath = "..\Debug\OLEDBIO.dll"
$outputFile = "OLEDBIO.def"

Write-Host "`n=== Generating Clean DEF File ===" -ForegroundColor Cyan

# Find dumpbin
$dumpbinPath = $null
$dumpbinCmd = Get-Command dumpbin -ErrorAction SilentlyContinue
if ($dumpbinCmd) {
    $dumpbinPath = $dumpbinCmd.Source
}

if (-not $dumpbinPath) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vsWhere) {
        $vsPath = & $vsWhere -latest -property installationPath
        if ($vsPath) {
            $searchPaths = @(
                "$vsPath\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
                "$vsPath\VC\Tools\MSVC\*\bin\Hostx86\x86\dumpbin.exe"
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
    exit 1
}

# Get exports
$dumpbinOutput = & $dumpbinPath /EXPORTS $dllPath 2>&1
$exports = @()
$inExportSection = $false

foreach ($line in $dumpbinOutput) {
    if ($line -match "ordinal\s+hint\s+RVA\s+name") {
        $inExportSection = $true
        continue
    }
    
    if ($inExportSection -and $line -match "^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(.+)$") {
        $fullName = $matches[1].Trim()
        
        # Remove ILT/forwarding info (e.g. " = @ILT+..." or " = other.dll.func")
        if ($fullName -match "^(.+?)\s+=") {
            $fullName = $matches[1]
        }
        
        # Extract clean name from __stdcall decoration: _FunctionName@ByteCount
        if ($fullName -match "^_(.+?)@\d+") {
            $cleanName = $matches[1]
            $exports += [PSCustomObject]@{
                CleanName     = $cleanName
                DecoratedName = $fullName
            }
        }
        # Handle functions with no parameters (no @ByteCount)
        elseif ($fullName -match "^_(.+)$") {
            $cleanName = $matches[1]
            $exports += [PSCustomObject]@{
                CleanName     = $cleanName
                DecoratedName = $fullName
            }
        }
        # Functions without underscore prefix (already clean or different convention)
        else {
            $exports += [PSCustomObject]@{
                CleanName     = $fullName
                DecoratedName = $fullName
            }
        }
    }
}

Write-Host "Found $($exports.Count) exported functions" -ForegroundColor Green

# Generate DEF file
$defContent = @"
LIBRARY "OLEDBIO"
DESCRIPTION "OLEDBIO DLL - Generated $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"

EXPORTS
    ; Clean function names mapped from __stdcall decorated names
    ; Total exports: $($exports.Count)
    
"@

# Sort by clean name
$sortedExports = $exports | Sort-Object CleanName

foreach ($export in $sortedExports) {
    if ($export.CleanName -ne $export.DecoratedName) {
        # Map clean name to decorated name
        $defContent += "    $($export.CleanName) = $($export.DecoratedName)`r`n"
    }
    else {
        # Already clean, just list it
        $defContent += "    $($export.CleanName)`r`n"
    }
}

# Save DEF file
$defContent | Out-File -FilePath $outputFile -Encoding ASCII

Write-Host "`n✅ DEF file generated: $outputFile" -ForegroundColor Green
Write-Host "`nPreview (first 20 exports):" -ForegroundColor Yellow
Write-Host ("=" * 80) -ForegroundColor Gray
Get-Content $outputFile | Select-Object -First 25
Write-Host "..." -ForegroundColor Gray
Write-Host ("=" * 80) -ForegroundColor Gray

Write-Host "`n✅ Next steps:" -ForegroundColor Cyan
Write-Host "1. The DEF file has been updated with clean name mappings" -ForegroundColor White
Write-Host "2. Rebuild your project (the DEF file is already configured)" -ForegroundColor White
Write-Host "3. Exports will now show clean names!" -ForegroundColor White
