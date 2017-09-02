@echo off

if not exist spirv (
    mkdir spirv
)

cd spirv
for %%f in (..\glsl\*) do (
    "%VULKAN_SDK%\Bin\glslangValidator.exe" -V -o %%~nxf.spv ..\glsl\%%~nxf
)
cd ..