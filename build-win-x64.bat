@echo off
setlocal

where cmake >nul 2>nul || (echo ERROR: cmake not found in PATH & exit /b 1)

if "%QT_BASE%"=="" (
  for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Qt\*" 2^>nul') do (
    if not defined QT_BASE if exist "C:\Qt\%%D\mingw_64\lib\cmake\Qt6\Qt6Config.cmake" set "QT_BASE=C:\Qt\%%D\mingw_64"
  )
)
if "%QT_BASE%"=="" (
  echo ERROR: Qt not found. Set QT_BASE=C:\Qt\^<version^>\mingw_64
  exit /b 1
)

set "QT_CMAKE=%QT_BASE%\lib\cmake"
if "%MINGW_BIN%"=="" set "MINGW_BIN=C:\Qt\Tools\mingw1310_64\bin"
if "%NINJA_EXE%"=="" set "NINJA_EXE=C:\Qt\Tools\Ninja\ninja.exe"
set "WINDEPLOYQT=%QT_BASE%\bin\windeployqt.exe"

if not exist "%MINGW_BIN%\g++.exe" ( echo ERROR: MinGW not found at %MINGW_BIN% & exit /b 1 )
if not exist "%NINJA_EXE%"        ( echo ERROR: ninja.exe not found at %NINJA_EXE% & exit /b 1 )
if not exist "%WINDEPLOYQT%"      ( echo ERROR: windeployqt.exe not found at %WINDEPLOYQT% & exit /b 1 )

set "PATH=%MINGW_BIN%;%PATH%"

set "BUILD_DIR=release\PP18_VideoTools_win-x64"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -S . -B "%BUILD_DIR%" -G "Ninja" ^
  -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" ^
  -DCMAKE_PREFIX_PATH="%QT_CMAKE%" ^
  -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 ( echo ERROR: CMake configure failed & exit /b 1 )

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 ( echo ERROR: Build failed & exit /b 1 )

set "EXE=%BUILD_DIR%\PP18_VideoTools.exe"
if not exist "%EXE%" ( echo ERROR: %EXE% not found after build. & exit /b 1 )

"%WINDEPLOYQT%" --release --no-translations "%EXE%"

echo.
echo [OK] Build + deploy finished. See %BUILD_DIR%.
endlocal
pause
