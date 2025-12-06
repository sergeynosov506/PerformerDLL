unit OLEDBIOTest;
{
  Unit: OLEDBIOTest.pas
  Purpose: Test harness for OLEDBIO.DLL - SysSettings and HoldMap functions
  Author: Generated 2025-12-04
}

interface

uses
  Windows, SysUtils, Classes;

const
  OLEDBIO_DLL = 'OLEDBIO.DLL';
  NT = 1;

type
  // Error structure - must match C++ ERRSTRUCT
  TERRSTRUCT = packed record
    iSqlError: Integer;
    sNativeError: array[0..255] of AnsiChar;
    sSqlState: array[0..5] of AnsiChar;
    sErrorMsg: array[0..511] of AnsiChar;
    sFuncName: array[0..63] of AnsiChar;
  end;
  PERRSTRUCT = ^TERRSTRUCT;

  // SYSSETTING structure - must match C++ SYSSETTING
  TSYSSETTING = packed record
    sDateforaccrualflag: array[0..1+NT] of AnsiChar;
    lEarliestactiondate: Longint;
    iPaymentsStartDate: Integer;
    sSystemcurrency: array[0..4+NT] of AnsiChar;
    sWeightedStatisticsFlag: array[0..1+NT] of AnsiChar;
    sEquityRating: array[0..1+NT] of AnsiChar;
    sFixedRating: array[0..1+NT] of AnsiChar;
    sPerformanceType: array[0..1+NT] of AnsiChar;
    sYieldType: array[0..1+NT] of AnsiChar;
    iFlowWeightMethod: Integer;
  end;
  PSYSSETTING = ^TSYSSETTING;

  // Function pointer types
  TInitializeOLEDBIO = procedure(sAlias, sMode, sType: PAnsiChar;
    lAsofDate: Longint; iPrepareWhat: Integer; pzErr: PERRSTRUCT); stdcall;
  TFreeOLEDBIO = procedure; stdcall;
  TSelectSyssettings = procedure(pzSyssetng: PSYSSETTING; pzErr: PERRSTRUCT); stdcall;
  TSelectHoldmap = procedure(lAsofDate: Longint;
    sHoldingsName, sHoldcashName, sPortmainName, sPortbalName,
    sPayrecName, sHXrefName, sHoldtotName: PAnsiChar;
    pzErr: PERRSTRUCT); stdcall;
  TInitHoldmapCS = procedure(lAsofDate: Longint; pzErr: PERRSTRUCT); stdcall;

var
  // DLL Handle
  hDLL: THandle = 0;
  
  // Function pointers
  InitializeOLEDBIO: TInitializeOLEDBIO = nil;
  FreeOLEDBIO: TFreeOLEDBIO = nil;
  SelectSyssettings: TSelectSyssettings = nil;
  SelectHoldmap: TSelectHoldmap = nil;
  InitHoldmapCS: TInitHoldmapCS = nil;

// Public functions
function LoadOLEDBIO: Boolean;
procedure UnloadOLEDBIO;
function TestSysSettings(const sAlias: AnsiString; lAsofDate: Longint): Boolean;
function TestHoldMap(const sAlias: AnsiString; lAsofDate: Longint): Boolean;

implementation

function LoadOLEDBIO: Boolean;
begin
  Result := False;
  
  if hDLL <> 0 then
  begin
    Result := True;
    Exit;
  end;
  
  hDLL := LoadLibrary(OLEDBIO_DLL);
  if hDLL = 0 then
  begin
    WriteLn('ERROR: Failed to load ', OLEDBIO_DLL, ' - Error: ', GetLastError);
    Exit;
  end;
  
  // Get function addresses
  @InitializeOLEDBIO := GetProcAddress(hDLL, 'InitializeOLEDBIO');
  @FreeOLEDBIO := GetProcAddress(hDLL, 'FreeOLEDBIO');
  @SelectSyssettings := GetProcAddress(hDLL, 'SelectSyssettings');
  @SelectHoldmap := GetProcAddress(hDLL, 'SelectHoldmap');
  @InitHoldmapCS := GetProcAddress(hDLL, 'InitHoldmapCS');
  
  // Verify required functions loaded
  if not Assigned(InitializeOLEDBIO) then
    WriteLn('WARNING: InitializeOLEDBIO not found');
  if not Assigned(FreeOLEDBIO) then
    WriteLn('WARNING: FreeOLEDBIO not found');
  if not Assigned(SelectSyssettings) then
    WriteLn('WARNING: SelectSyssettings not found');
  if not Assigned(SelectHoldmap) then
    WriteLn('WARNING: SelectHoldmap not found');
  if not Assigned(InitHoldmapCS) then
    WriteLn('WARNING: InitHoldmapCS not found');
    
  Result := Assigned(InitializeOLEDBIO) and Assigned(FreeOLEDBIO);
  
  if Result then
    WriteLn('SUCCESS: OLEDBIO.DLL loaded successfully')
  else
    WriteLn('ERROR: Required functions not found in DLL');
end;

procedure UnloadOLEDBIO;
begin
  if hDLL <> 0 then
  begin
    // Clean up DLL resources
    if Assigned(FreeOLEDBIO) then
      FreeOLEDBIO;
      
    FreeLibrary(hDLL);
    hDLL := 0;
    
    // Clear function pointers
    InitializeOLEDBIO := nil;
    FreeOLEDBIO := nil;
    SelectSyssettings := nil;
    SelectHoldmap := nil;
    InitHoldmapCS := nil;
    
    WriteLn('DLL unloaded');
  end;
end;

procedure InitErrStruct(var zErr: TERRSTRUCT);
begin
  FillChar(zErr, SizeOf(zErr), 0);
end;

function CheckError(const zErr: TERRSTRUCT; const sFuncName: string): Boolean;
begin
  Result := (zErr.iSqlError = 0);
  if not Result then
  begin
    WriteLn('ERROR in ', sFuncName, ':');
    WriteLn('  SQL Error: ', zErr.iSqlError);
    WriteLn('  Native Error: ', zErr.sNativeError);
    WriteLn('  SQL State: ', zErr.sSqlState);
    WriteLn('  Message: ', zErr.sErrorMsg);
  end;
end;

function TestSysSettings(const sAlias: AnsiString; lAsofDate: Longint): Boolean;
var
  zErr: TERRSTRUCT;
  zSysSetting: TSYSSETTING;
begin
  Result := False;
  
  if not LoadOLEDBIO then
    Exit;
    
  WriteLn('');
  WriteLn('=== Testing SysSettings ===');
  WriteLn('Alias: ', sAlias);
  WriteLn('AsofDate: ', lAsofDate);
  
  // Initialize the DLL
  InitErrStruct(zErr);
  WriteLn('Initializing OLEDBIO...');
  InitializeOLEDBIO(PAnsiChar(sAlias), 'R', 'T', lAsofDate, 0, @zErr);
  
  if not CheckError(zErr, 'InitializeOLEDBIO') then
    Exit;
  WriteLn('Initialization successful');
    
  // Select SysSettings
  if Assigned(SelectSyssettings) then
  begin
    FillChar(zSysSetting, SizeOf(zSysSetting), 0);
    InitErrStruct(zErr);
    
    WriteLn('Calling SelectSyssettings...');
    SelectSyssettings(@zSysSetting, @zErr);
    
    if CheckError(zErr, 'SelectSyssettings') then
    begin
      WriteLn('SysSettings Results:');
      WriteLn('  DateForAccrualFlag: ', zSysSetting.sDateforaccrualflag);
      WriteLn('  EarliestActionDate: ', zSysSetting.lEarliestactiondate);
      WriteLn('  PaymentsStartDate: ', zSysSetting.iPaymentsStartDate);
      WriteLn('  SystemCurrency: ', zSysSetting.sSystemcurrency);
      WriteLn('  WeightedStatisticsFlag: ', zSysSetting.sWeightedStatisticsFlag);
      WriteLn('  EquityRating: ', zSysSetting.sEquityRating);
      WriteLn('  FixedRating: ', zSysSetting.sFixedRating);
      WriteLn('  PerformanceType: ', zSysSetting.sPerformanceType);
      WriteLn('  YieldType: ', zSysSetting.sYieldType);
      WriteLn('  FlowWeightMethod: ', zSysSetting.iFlowWeightMethod);
      Result := True;
    end;
  end
  else
    WriteLn('SelectSyssettings not available');
    
  WriteLn('=== SysSettings Test Complete ===');
end;

function TestHoldMap(const sAlias: AnsiString; lAsofDate: Longint): Boolean;
var
  zErr: TERRSTRUCT;
  sHoldingsName: array[0..39] of AnsiChar;
  sHoldcashName: array[0..39] of AnsiChar;
  sPortmainName: array[0..39] of AnsiChar;
  sPortbalName: array[0..39] of AnsiChar;
  sPayrecName: array[0..39] of AnsiChar;
  sHXrefName: array[0..39] of AnsiChar;
  sHoldtotName: array[0..39] of AnsiChar;
begin
  Result := False;
  
  if not LoadOLEDBIO then
    Exit;
    
  WriteLn('');
  WriteLn('=== Testing HoldMap ===');
  WriteLn('Alias: ', sAlias);
  WriteLn('AsofDate: ', lAsofDate);
  
  // Initialize the DLL if not already done
  InitErrStruct(zErr);
  WriteLn('Initializing OLEDBIO...');
  InitializeOLEDBIO(PAnsiChar(sAlias), 'R', 'T', lAsofDate, 0, @zErr);
  
  if not CheckError(zErr, 'InitializeOLEDBIO') then
    Exit;
  WriteLn('Initialization successful');
  
  // Select HoldMap
  if Assigned(SelectHoldmap) then
  begin
    FillChar(sHoldingsName, SizeOf(sHoldingsName), 0);
    FillChar(sHoldcashName, SizeOf(sHoldcashName), 0);
    FillChar(sPortmainName, SizeOf(sPortmainName), 0);
    FillChar(sPortbalName, SizeOf(sPortbalName), 0);
    FillChar(sPayrecName, SizeOf(sPayrecName), 0);
    FillChar(sHXrefName, SizeOf(sHXrefName), 0);
    FillChar(sHoldtotName, SizeOf(sHoldtotName), 0);
    InitErrStruct(zErr);
    
    WriteLn('Calling SelectHoldmap...');
    SelectHoldmap(lAsofDate,
      @sHoldingsName[0], @sHoldcashName[0], @sPortmainName[0],
      @sPortbalName[0], @sPayrecName[0], @sHXrefName[0], @sHoldtotName[0],
      @zErr);
    
    if CheckError(zErr, 'SelectHoldmap') then
    begin
      WriteLn('HoldMap Results:');
      WriteLn('  HoldingsName: ', sHoldingsName);
      WriteLn('  HoldcashName: ', sHoldcashName);
      WriteLn('  PortmainName: ', sPortmainName);
      WriteLn('  PortbalName: ', sPortbalName);
      WriteLn('  PayrecName: ', sPayrecName);
      WriteLn('  HXrefName: ', sHXrefName);
      WriteLn('  HoldtotName: ', sHoldtotName);
      Result := True;
    end;
  end
  else
    WriteLn('SelectHoldmap not available');
    
  WriteLn('=== HoldMap Test Complete ===');
end;

end.

//------------------------------------------------------------------------------
// Example Usage (in main program):
//------------------------------------------------------------------------------
{
program TestOLEDBIO;

{$APPTYPE CONSOLE}

uses
  SysUtils,
  OLEDBIOTest in 'OLEDBIOTest.pas';

var
  sAlias: AnsiString;
  lAsofDate: Longint;
begin
  try
    // Configure connection - adjust these values
    sAlias := 'YourDatabaseAlias';  // or connection string
    lAsofDate := 45292;  // Example: Excel date for 2023-12-31
    
    WriteLn('OLEDBIO DLL Test Harness');
    WriteLn('========================');
    
    // Test SysSettings
    if TestSysSettings(sAlias, lAsofDate) then
      WriteLn('SysSettings: PASSED')
    else
      WriteLn('SysSettings: FAILED');
      
    // Test HoldMap
    if TestHoldMap(sAlias, lAsofDate) then
      WriteLn('HoldMap: PASSED')
    else
      WriteLn('HoldMap: FAILED');
      
    // Cleanup
    UnloadOLEDBIO;
    
    WriteLn('');
    WriteLn('Tests complete. Press Enter to exit...');
    ReadLn;
  except
    on E: Exception do
      WriteLn('Exception: ', E.Message);
  end;
end.
}
