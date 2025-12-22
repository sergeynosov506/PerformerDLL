program TestOLEDBIO;

uses
  SysUtils, Windows;

const
  OLEDBIO_DLL = 'OLEDBIO.dll';

type
  // ERRSTRUCT matches C++ struct ERRSTRUCT
  ERRSTRUCT = packed record
    iID: Integer;
    lRecNo: Integer;
    sRecType: array[0..1] of Char; // 1 + NT (NT=1) = 2
    iBusinessError: Integer;
    iSqlError: Integer;
    iIsamCode: Integer;
  end;
  PERRSTRUCT = ^ERRSTRUCT;

  // SYSVALUES matches C++ struct SYSVALUES
  // sName[80+NT], sValue[100+NT]
  SYSVALUES = packed record
    sName: array[0..81] of Char;
    sValue: array[0..101] of Char;
  end;
  PSYSVALUES = ^SYSVALUES;

type
  TInitializeOLEDBIO = procedure(sAlias, sMode, sType: PChar; lAsofDate: Integer; iPrepareWhat: Integer; var pzErr: ERRSTRUCT); stdcall;
  TSelectSysValues = procedure(var pzSysvalues: SYSVALUES; var pzErr: ERRSTRUCT); stdcall;

var
  hDll: THandle;
  InitializeOLEDBIO: TInitializeOLEDBIO;
  SelectSysValues: TSelectSysValues;
  zErr: ERRSTRUCT;
  zSysVal: SYSVALUES;

begin
//  Writeln('Loading ' + OLEDBIO_DLL + '...');
  hDll := LoadLibrary(PChar(OLEDBIO_DLL));

  if hDll = 0 then
  begin
    Writeln('Error: Could not load DLL. Error code: ', GetLastError);
    Readln;
    Exit;
  end;

  @InitializeOLEDBIO := GetProcAddress(hDll, 'InitializeOLEDBIO');
  if @InitializeOLEDBIO = nil then
    Writeln('Error: InitializeOLEDBIO not found');
//  else
//    Writeln('InitializeOLEDBIO found.');

  // Try to find SelectSysValues (CamelCase as exported by alias)
  @SelectSysValues := GetProcAddress(hDll, 'SelectSysValues');
  if @SelectSysValues = nil then
  begin
    Writeln('SelectSysValues (CamelCase) not found, trying SelectSysvalues (lowercase)...');
    @SelectSysValues := GetProcAddress(hDll, 'SelectSysvalues');
  end;

//  if @SelectSysValues <> nil then
//    Writeln('SelectSysValues found.');
//  else
//    Writeln('Error: SelectSysValues not found.');

  if (@InitializeOLEDBIO <> nil) and (@SelectSysValues <> nil) then
  begin
    FillChar(zErr, SizeOf(zErr), 0);
    // InitializeOLEDBIO(Alias, Mode, Type, Date, PrepareWhat, Err)
    // Adjust parameters as needed for your environment parameters
//    Writeln('Calling InitializeOLEDBIO...');
    try
        // Use direct ODBC connection string (supported by updated TransIO_Login.cpp fallback)
        InitializeOLEDBIO('DemoSalesN', 'test', 'test', 0, 0, zErr);
//        Writeln('InitializeOLEDBIO called successfully.');
    except
        on E: Exception do Writeln('Exception during InitializeOLEDBIO: ', E.Message);
    end;

    FillChar(zSysVal, SizeOf(zSysVal), 0);
    StrPCopy(zSysVal.sName, 'SomeSettingName'); // Replace with actual setting name to query

//    Writeln('Calling SelectSysValues...');
    try
        SelectSysValues(zSysVal, zErr);
//        Writeln('SelectSysValues called. Result Value: ', zSysVal.sValue);
    except
        on E: Exception do Writeln('Exception during SelectSysValues: ', E.Message);
    end;
  end;

  FreeLibrary(hDll);
//  Writeln('Done. Press Enter to exit.');
  Readln;
end.
