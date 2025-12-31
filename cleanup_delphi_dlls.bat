@echo off
REM Cleanup script to remove legacy Delphi DLLs that have been replaced with C++ implementations
REM Created: 2025-12-29
REM Purpose: Remove StarsUtils.dll and DelphiCInterface.dll from deployment folders

echo ========================================
echo Cleaning up legacy Delphi DLLs
echo ========================================
echo.

REM Clean from Roll.Tests bin folders
echo Cleaning Roll.Tests bin folders...
if exist "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\StarsUtils.dll" (
    del /F /Q "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\StarsUtils.dll"
    echo   - Deleted: Roll.Tests\bin\Debug\net8.0\StarsUtils.dll
)

if exist "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\x64\StarsUtils.dll" (
    del /F /Q "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\x64\StarsUtils.dll"
    echo   - Deleted: Roll.Tests\bin\Debug\net8.0\x64\StarsUtils.dll
)

if exist "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\x64\Debug\net8.0\StarsUtils.dll" (
    del /F /Q "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\x64\Debug\net8.0\StarsUtils.dll"
    echo   - Deleted: Roll.Tests\bin\x64\Debug\net8.0\StarsUtils.dll
)

if exist "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\DelphiCInterface.dll" (
    del /F /Q "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\DelphiCInterface.dll"
    echo   - Deleted: Roll.Tests\bin\Debug\net8.0\DelphiCInterface.dll
)

if exist "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\x64\DelphiCInterface.dll" (
    del /F /Q "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\Debug\net8.0\x64\DelphiCInterface.dll"
    echo   - Deleted: Roll.Tests\bin\Debug\net8.0\x64\DelphiCInterface.dll
)

if exist "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\x64\Debug\net8.0\DelphiCInterface.dll" (
    del /F /Q "e:\projects\PerformerDLL\Roll\Roll.Tests\bin\x64\Debug\net8.0\DelphiCInterface.dll"
    echo   - Deleted: Roll.Tests\bin\x64\Debug\net8.0\DelphiCInterface.dll
)

echo.
echo ========================================
echo Cleanup complete!
echo ========================================
echo.
echo Note: These DLLs are no longer needed because:
echo   - StarsUtils.dll functions now in OLEDBIO.dll (C++)
echo   - DelphiCInterface.dll functions now in OLEDBIO.dll (C++)
echo.
echo The following DLLs still reference StarsUtils.dll and need updating:
echo   - Performance.dll (perfdll.cpp)
echo   - Digenerate.dll (digenerate2.cpp)
echo   - CompositeCreate.dll (CompositeCreate.cpp)
echo   - Valuation.dll (value2.cpp)
echo.

pause
