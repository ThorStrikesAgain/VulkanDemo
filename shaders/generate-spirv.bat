@echo off
setlocal enabledelayedexpansion

set scriptDir=%~dp0
set spirvDir=%scriptDir%spirv

if not exist "%spirvDir%" (
    mkdir "%spirvDir%"
)

cd /D "%spirvDir%"
for %%f in (..\glsl\*) do (
    "%VULKAN_SDK%\Bin\glslangValidator.exe" -V -o %%~nxf.spv ..\glsl\%%~nxf
    if !errorlevel! NEQ 0 (
        exit /b
    )
)
