@echo off
setlocal enabledelayedexpansion

REM Try to find vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not exist "%VSWHERE%" (
    echo vswhere.exe not found. Cannot locate MSBuild.
    exit /b 1
)

REM Find MSBuild
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
  set "MSBUILD_PATH=%%i"
)

if "%MSBUILD_PATH%"=="" (
  echo MSBuild not found via vswhere.
  exit /b 1
)

echo Found MSBuild at: "%MSBUILD_PATH%"

REM Build OLEDBIO
"%MSBUILD_PATH%" "e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj" /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /v:minimal > build_output.txt 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED. Output:
    type build_output.txt
    exit /b 1
) else (
    echo Build SUCCESS.
    REM Check if file exists
    if exist "E:\projects\PerformerDLL\OLEDBIO.dll" (
        echo OLEDBIO.dll found at "E:\projects\PerformerDLL\OLEDBIO.dll"
    ) else (
        echo OLEDBIO.dll NOT found at expected path!
        dir "E:\projects\PerformerDLL\"
    )
)
