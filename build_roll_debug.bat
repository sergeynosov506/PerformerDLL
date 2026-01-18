@echo off
setlocal enabledelayedexpansion

REM MSBuild path found from previous step
set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"

echo Building Roll with MSBuild at: "%MSBUILD_PATH%"

REM Build Roll
"%MSBUILD_PATH%" "e:\projects\PerformerDLL\Roll\Roll.vcxproj" /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /v:minimal > build_roll_output.txt 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED. Output:
    type build_roll_output.txt
    exit /b 1
) else (
    echo Build SUCCESS.
    if exist "E:\projects\PerformerDLL\Roll.dll" (
        echo Roll.dll found at "E:\projects\PerformerDLL\Roll.dll"
    ) else (
        echo Roll.dll NOT found at expected path!
        dir "E:\projects\PerformerDLL\"
    )
)
