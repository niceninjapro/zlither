@echo off
setlocal enabledelayedexpansion

echo ============================================
echo    ZLITHER STANDALONE BUILD
echo ============================================
echo.

REM Ask for debug or release
echo Choose build type:
echo   1. Debug (with console)
echo   2. Release (no console)
echo.
set /p BUILD_TYPE="Enter choice (1 or 2): "

if "%BUILD_TYPE%"=="1" (
    set BUILD_MODE=debug_windows
    set BUILD_NAME=Debug
) else if "%BUILD_TYPE%"=="2" (
    set BUILD_MODE=release_windows
    set BUILD_NAME=Release
) else (
    echo Invalid choice
    exit /b 1
)

echo Building %BUILD_NAME% configuration...
echo.

set PREMAKE_PATH=C:\premake5\premake5.exe
set OUTPUT_DIR=output

echo [1/6] Validating...
if not exist "%PREMAKE_PATH%" (
    echo ERROR: premake5 not found at %PREMAKE_PATH%
    exit /b 1
)
set PATH=C:\mingw64\bin;%PATH%
if not defined VULKAN_SDK set "VULKAN_SDK=C:\VulkanSDK\1.4.341.1"
if not exist "project.lua" (
    echo ERROR: project.lua not found
    exit /b 1
)
echo   OK
echo.

echo [2/6] Cleaning...
if exist "build" rmdir /s /q build >nul 2>&1
if exist "%OUTPUT_DIR%" rmdir /s /q %OUTPUT_DIR% >nul 2>&1
echo   OK
echo.

echo [3/6] Compiling shaders...
python3 build.py 2 >nul 2>&1
echo   OK
echo.

echo [4/6] Creating resources...
if exist "zlither-256px.ico" (
    REM Create RC file
    (
        echo 1 ICON "zlither-256px.ico"
    ) > zlither.rc
    
    REM Compile RC file to object file that gmake will link
    echo Compiling icon resource...
    windres -i zlither.rc -o zlither_res.o
    if exist zlither_res.o (
        echo   Icon resource compiled
    ) else (
        echo   WARNING: Icon compilation failed
    )
)
echo.

echo [5/6] Building...
echo Generating with premake5...
"%PREMAKE_PATH%" --file=project.lua gmake
if errorlevel 1 (
    echo ERROR: premake5 failed
    exit /b 1
)

if not exist "build\makefiles\Makefile" (
    echo ERROR: Makefile not generated
    exit /b 1
)

echo Making %BUILD_NAME% build...
cd build\makefiles
mingw32-make config=%BUILD_MODE%
if errorlevel 1 (
    echo ERROR: Build failed
    cd ..\..
    exit /b 1
)
cd ..\..
echo   OK
echo.

echo [6/6] Packaging...
if not exist "%OUTPUT_DIR%" mkdir %OUTPUT_DIR%

REM Find and copy executable
set EXE_FOUND=0
if exist "build\bin\windows_x86_64_%BUILD_TYPE%\app.exe" (
    copy /Y "build\bin\windows_x86_64_%BUILD_TYPE%\app.exe" "%OUTPUT_DIR%\Zlither.exe"
    echo   Copied: %OUTPUT_DIR%\Zlither.exe
    set EXE_FOUND=1
)

if !EXE_FOUND! equ 0 (
    if exist "build\bin\windows_x86_64_release\app.exe" (
        copy /Y "build\bin\windows_x86_64_release\app.exe" "%OUTPUT_DIR%\Zlither.exe"
        echo   Copied: %OUTPUT_DIR%\Zlither.exe
        set EXE_FOUND=1
    )
)

if !EXE_FOUND! equ 0 (
    if exist "build\bin\windows_x86_64_debug\app.exe" (
        copy /Y "build\bin\windows_x86_64_debug\app.exe" "%OUTPUT_DIR%\Zlither.exe"
        echo   Copied: %OUTPUT_DIR%\Zlither.exe
        set EXE_FOUND=1
    )
)

if !EXE_FOUND! equ 0 (
    echo ERROR: app.exe not found
    exit /b 1
)

REM Embed resources into exe
echo Embedding resources...
python3 pak_packer.py "%OUTPUT_DIR%\Zlither.exe"
if errorlevel 1 (
    echo ERROR: Failed to embed resources
    exit /b 1
)
if exist "%OUTPUT_DIR%\Zlither_embedded.exe" (
    cd "%OUTPUT_DIR%"
    del Zlither.exe
    ren Zlither_embedded.exe Zlither.exe
    cd ..
    echo   Resources embedded successfully
) else (
    echo ERROR: Embedded exe not created
    exit /b 1
)
)

REM Clean up RC file but keep compiled resource object for linking
if exist "zlither.rc" del zlither.rc >nul 2>&1
if exist "zlither.ico" del zlither.ico >nul 2>&1

echo ============================================
echo     BUILD COMPLETE
echo ============================================
echo.
echo Configuration: %BUILD_NAME%
echo Output: %OUTPUT_DIR%\Zlither.exe
echo Data: C:\Users\%USERNAME%\AppData\Local\Zlither
echo.
pause
