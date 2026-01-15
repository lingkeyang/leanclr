
@echo off
setlocal enabledelayedexpansion

rem Directory of this script
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build"

rem Args: CONFIG (Debug/Release), ARCH (x64/x86)
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
set "ARCH=%~2"
if "%ARCH%"=="" set "ARCH=x64"

echo === Config: %CONFIG% ^| Arch: %ARCH% ===


echo build basic_test_runner
pushd basic_test_runner
call build.bat %CONFIG% %ARCH%
if errorlevel 1 (
    echo ERROR: basic_test_runner build failed.
    popd
    exit /b 1
)
popd

echo build managed tests
pushd managed
call dotnet build -c %CONFIG%
if errorlevel 1 (
    echo ERROR: managed tests build failed.
    popd
    exit /b 1
)
popd
echo All tests built successfully.
