@echo off
:: Build script for Windows (Qt6 + MinGW or MSVC)
:: Requires: Qt6 in PATH, cmake in PATH
::
:: Usage:
::   build.bat           — Debug build
::   build.bat Release   — Release build

setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

set BUILD_DIR=build\%CONFIG%

cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%CONFIG%
if errorlevel 1 goto :err

cmake --build %BUILD_DIR% --config %CONFIG% -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 goto :err

echo.
echo Build OK: %BUILD_DIR%\DailyHexPuzzle.exe
goto :eof

:err
echo Build FAILED.
exit /b 1
