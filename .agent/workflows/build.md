---
description: Build OLEDBIO.dll using msbuild
---

# Build OLEDBIO.dll

This workflow builds the OLEDBIO.dll library using MSBuild.

## Prerequisites

Visual Studio is installed at: `C:\Program Files\Microsoft Visual Studio\18\Professional`

## Build Commands

### Debug Build (Win32)
// turbo
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /p:Configuration=Debug /p:Platform=Win32 /t:Build"
```

### Release Build (Win32)
// turbo
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /p:Configuration=Release /p:Platform=Win32 /t:Build"
```

### Build Both Debug and Release
// turbo-all
1. Build Debug:
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /p:Configuration=Debug /p:Platform=Win32 /t:Build"
```

2. Build Release:
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /p:Configuration=Release /p:Platform=Win32 /t:Build"
```

### Clean Build
// turbo
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /t:Clean"
```

### Rebuild Debug
// turbo
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /p:Configuration=Debug /p:Platform=Win32 /t:Rebuild"
```

### Rebuild Release  
// turbo
```cmd
cmd /c "\"C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat\" && msbuild e:\projects\PerformerDLL\Source\OLEDBIO.vcxproj /p:Configuration=Release /p:Platform=Win32 /t:Rebuild"
```

## Output Locations

- **Debug**: `e:\projects\PerformerDLL\Source\Debug\OLEDBIO.dll`
- **Release**: `e:\projects\PerformerDLL\Source\Release\OLEDBIO.dll`
