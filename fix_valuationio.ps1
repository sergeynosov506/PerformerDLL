# PowerShell script to add STDCALL to ValuationIO.cpp function implementations
$file = "E:\projects\PerformerDLL\Source\ValuationIO.cpp"
$content = Get-Content $file -Raw

# Define the replacements based on the line numbers
$patterns = @(
    'DLLAPI void SelectAllAccdivForAnAccount',
    'DLLAPI void SelectPendingAccdivTransForAnAccount',
    'DLLAPI void SelectUnitsForASoldSecurity',
    'DLLAPI void SelectAllDivhistForAnAccount',
    'DLLAPI void SelectNextCallDatePrice',
    'DLLAPI void SelectPartEquities',
    'DLLAPI void SelectAllHedgxrefForAnAccount',
    'DLLAPI void SelectAllHoldcashForAnAccount',
    'DLLAPI void SelectAllHoldingsForAnAccount',
    'DLLAPI void SelectUnitsHeldForASecurity',
    'DLLAPI void SelectOneHistfinc',
    'DLLAPI void SelectOneHisteqty',
    'DLLAPI void SelectOneHistpric',
    'DLLAPI void SelectAllPayrecForAnAccount',
    'DLLAPI void UpdatePayrec',
    'DLLAPI void UpdatePortmainValDate',
    'DLLAPI void SelectAllForHoldTot',
    'DLLAPI void SelectAllRatings',
    'DLLAPI BOOL IsAssetInternational',
    'DLLAPI int GetInterSegID',
    'DLLAPI int SelectSegmentIdFromSegMap',
    'DLLAPI int SelectSegmentLevelId',
    'DLLAPI void SelectUnsupervised',
    'DLLAPI int SelectUnsupervisedSegmentId',
    'DLLAPI int SelectPledgedSegment',
    'DLLAPI void SelectSegment',
    'DLLAPI double SelectDivint',
    'DLLAPI void SelectAllSegments',
    'DLLAPI void SelectAllSegmap',
    'DLLAPI void SelectAdhocPortmain',
    'DLLAPI double SelectSecurityRate',
    'DLLAPI BOOL IsManualAccrInt',
    'DLLAPI void SelectCustomPricesForAnAccount',
    'DLLAPI ERRSTRUCT  InitializeValuationIO',
    'DLLAPI void FreeValuationIO'
)

# Apply replacements
foreach ($pattern in $patterns) {
    # Split pattern to insert STDCALL
    if ($pattern -match 'DLLAPI (void|int|long|double|BOOL|ERRSTRUCT)\s+(.+)') {
        $type = $matches[1]
        $funcName = $matches[2]
        $newPattern = "DLLAPI $type STDCALL $funcName"
        $content = $content -replace [regex]::Escape($pattern), $newPattern
    }
}

# Write back
Set-Content $file $content -NoNewline

Write-Host "✅ Fixed all ValuationIO.cpp functions" -ForegroundColor Green
