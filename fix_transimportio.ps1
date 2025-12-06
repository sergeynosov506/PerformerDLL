# PowerShell script to add STDCALL to TransImportIO.cpp function implementations
$file = "E:\projects\PerformerDLL\Source\TransImportIO.cpp"
$content = Get-Content $file -Raw

# Define the replacements (line numbers from the search)
$replacements = @(
    @{Line = 106; Pattern = 'DLLAPI void SelectPortClos'; Replace = 'DLLAPI void STDCALL SelectPortClos' },
    @{Line = 378; Pattern = 'DLLAPI void InsertDTrans '; Replace = 'DLLAPI void STDCALL InsertDTrans ' },
    @{Line = 567; Pattern = 'DLLAPI void InsertDTrnDesc'; Replace = 'DLLAPI void STDCALL InsertDTrnDesc' },
    @{Line = 663; Pattern = 'DLLAPI void InsertDPayTran'; Replace = 'DLLAPI void STDCALL InsertDPayTran' },
    @{Line = 764; Pattern = 'DLLAPI void SelectTransNoByDtransNo '; Replace = 'DLLAPI void STDCALL SelectTransNoByDtransNo ' },
    @{Line = 884; Pattern = 'DLLAPI void SelectTransNoByUnitsAndDates '; Replace = 'DLLAPI void STDCALL SelectTransNoByUnitsAndDates ' },
    @{Line = 1004; Pattern = 'DLLAPI void SelectMapTransNoEx'; Replace = 'DLLAPI void STDCALL SelectMapTransNoEx' },
    @{Line = 1116; Pattern = 'DLLAPI void SelectMapTransNoExByTransNo'; Replace = 'DLLAPI void STDCALL SelectMapTransNoExByTransNo' },
    @{Line = 1238; Pattern = 'DLLAPI void InsertMapTransNoEx'; Replace = 'DLLAPI void STDCALL InsertMapTransNoEx' },
    @{Line = 1358; Pattern = 'DLLAPI void UpdateMapTransNoEx'; Replace = 'DLLAPI void STDCALL UpdateMapTransNoEx' },
    @{Line = 1461; Pattern = 'DLLAPI void DeleteMapTransNoEx'; Replace = 'DLLAPI void STDCALL DeleteMapTransNoEx' },
    @{Line = 1590; Pattern = 'DLLAPI void InsertTaxlotRecon'; Replace = 'DLLAPI void STDCALL InsertTaxlotRecon' },
    @{Line = 1722; Pattern = 'DLLAPI void InsertAllTaxlotRecon'; Replace = 'DLLAPI void STDCALL InsertAllTaxlotRecon' },
    @{Line = 1752; Pattern = 'DLLAPI void InsertOneTaxlotRecon'; Replace = 'DLLAPI void STDCALL InsertOneTaxlotRecon' },
    @{Line = 1991; Pattern = 'DLLAPI void InsertTradeExchange'; Replace = 'DLLAPI void STDCALL InsertTradeExchange' },
    @{Line = 2101; Pattern = 'DLLAPI void UpdateTradeExchangeByPK'; Replace = 'DLLAPI void STDCALL UpdateTradeExchangeByPK' },
    @{Line = 2290; Pattern = 'DLLAPI void InsertBnkSet'; Replace = 'DLLAPI void STDCALL InsertBnkSet' },
    @{Line = 2467; Pattern = 'DLLAPI void InsertBnkSetEx'; Replace = 'DLLAPI void STDCALL InsertBnkSetEx' },
    @{Line = 2588; Pattern = 'DLLAPI void UpdateBnkSetExRevNo'; Replace = 'DLLAPI void STDCALL UpdateBnkSetExRevNo' },
    @{Line = 2712; Pattern = 'DLLAPI void GrouppedHoldingsFor'; Replace = 'DLLAPI void STDCALL GrouppedHoldingsFor' },
    @{Line = 2872; Pattern = 'DLLAPI void SelectBnksetexNoByUnitsAndDates '; Replace = 'DLLAPI void STDCALL SelectBnksetexNoByUnitsAndDates ' }
)

# Apply replacements
foreach ($rep in $replacements) {
    $content = $content -replace [regex]::Escape($rep.Pattern), $rep.Replace
}

# Write back
Set-Content $file $content -NoNewline

Write-Host "✅ Fixed all TransImportIO.cpp functions" -ForegroundColor Green
