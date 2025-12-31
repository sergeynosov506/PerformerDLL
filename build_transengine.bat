@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat"
msbuild "e:\projects\PerformerDLL\TransEngine\TransEngine.vcxproj" /p:Configuration=Debug /p:Platform=x64 /t:Build
