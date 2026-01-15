@echo off
setlocal enabledelayedexpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

set RUNNER=%~dp0\basic_test_runner\build\bin\%CONFIG%\test.exe

if not exist "%RUNNER%" (
    echo ERROR: Test runner not found at "%RUNNER%". Please run `build_all.bat` to build the tests first.
    endlocal & exit /b 1
)

%RUNNER%

if errorlevel 1 (
    echo Some tests failed.
    endlocal & exit /b %ERRORLEVEL%
) else (
    echo All tests passed.
)