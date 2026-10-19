@echo off
cls

:: ── Output directories ────────────────────────────────────────────────────────
:: Visual Studio (MSBuild) outputs
set VS_DEBUG=..\Out\x64\Debug
set VS_RELEASE=..\Out\x64\Release

:: CMake outputs (override with: Deploy.bat <build_dir>)
if "%~1"=="" (
    set CMAKE_BUILD=..\build
) else (
    set CMAKE_BUILD=%~1
)
set CMAKE_DEBUG=%CMAKE_BUILD%\Debug
set CMAKE_RELEASE=%CMAKE_BUILD%\Release

:: ── GLEW runtime DLL ──────────────────────────────────────────────────────────
set GLEW_DLL=..\AppGPUEcoSim\ThirdParty\GLEW\bin\glew32.dll

for %%D in ("%VS_DEBUG%" "%VS_RELEASE%" "%CMAKE_DEBUG%" "%CMAKE_RELEASE%") do (
    if exist "%%~D\" XCOPY /Y "%GLEW_DLL%" "%%~D\"
)

:: ── Qt6Sql runtime DLL + SQLite driver plugin ──────────────────────────────────
:: windeployqt does not always pick up Qt6Sql.dll / the sqldrivers plugin,
:: so copy them explicitly (matches the QSQLITE driver used by callback-display-export.cpp).
for %%D in ("%VS_RELEASE%" "%CMAKE_RELEASE%") do (
    if exist "%%~D\" (
        XCOPY /Y "%QTDIR64%\bin\Qt6Sql.dll" "%%~D\"
        if not exist "%%~D\sqldrivers\" mkdir "%%~D\sqldrivers"
        XCOPY /Y "%QTDIR64%\plugins\sqldrivers\qsqlite.dll" "%%~D\sqldrivers\"
    )
)
for %%D in ("%VS_DEBUG%" "%CMAKE_DEBUG%") do (
    if exist "%%~D\" (
        XCOPY /Y "%QTDIR64%\bin\Qt6Sqld.dll" "%%~D\"
        if not exist "%%~D\sqldrivers\" mkdir "%%~D\sqldrivers"
        XCOPY /Y "%QTDIR64%\plugins\sqldrivers\qsqlited.dll" "%%~D\sqldrivers\"
    )
)

:: ── Qt deployment ─────────────────────────────────────────────────────────────
set WDQ=%QTDIR64%\bin\windeployqt.exe

if exist "%VS_RELEASE%\AppGPUEcoSim.exe"    "%WDQ%" --release "%VS_RELEASE%\AppGPUEcoSim.exe"
if exist "%VS_DEBUG%\AppGPUEcoSim.exe"      "%WDQ%"           "%VS_DEBUG%\AppGPUEcoSim.exe"

if exist "%CMAKE_RELEASE%\EcoSim.exe" "%WDQ%" --release "%CMAKE_RELEASE%\EcoSim.exe"
if exist "%CMAKE_DEBUG%\EcoSim.exe"   "%WDQ%"           "%CMAKE_DEBUG%\EcoSim.exe"
