@echo off
REM Micrn Editor Installation Script for Windows
REM This script automates the build and installation process for Windows
REM Requires MinGW/MSYS2 or Cygwin

setlocal EnableDelayedExpansion

echo ==================================
echo   Micrn Editor Installation Script
echo ==================================
echo.

REM Check for MinGW/MSYS2/Cygwin environment
set "COMPILER_FOUND=0"
set "ENVIRONMENT="

REM Check for gcc
gcc --version >nul 2>&1
if !errorlevel! equ 0 (
    set "COMPILER_FOUND=1"
    echo [INFO] GCC compiler found.
) else (
    echo [ERROR] GCC compiler not found.
    goto :error_no_compiler
)

REM Check for make
make --version >nul 2>&1
if !errorlevel! equ 0 (
    echo [INFO] Make utility found.
) else (
    echo [ERROR] Make utility not found.
    goto :error_no_make
)

REM Detect environment
echo %MSYSTEM% | findstr /i "mingw\|msys" >nul 2>&1
if !errorlevel! equ 0 (
    set "ENVIRONMENT=MSYS2"
    echo [INFO] Detected MSYS2/MinGW environment.
    goto :check_ncurses
)

uname -s 2>nul | findstr /i "cygwin" >nul 2>&1
if !errorlevel! equ 0 (
    set "ENVIRONMENT=CYGWIN"
    echo [INFO] Detected Cygwin environment.
    goto :check_ncurses
)

echo [WARNING] Unable to detect MinGW/MSYS2/Cygwin environment.
echo [INFO] Assuming standard Windows with MinGW in PATH.
set "ENVIRONMENT=MINGW"

:check_ncurses
echo [INFO] Checking for ncurses library...

REM Try to compile a simple ncurses test
echo #include ^<ncurses.h^> > test_ncurses.c
echo int main(){ initscr(); endwin(); return 0; } >> test_ncurses.c

gcc -o test_ncurses.exe test_ncurses.c -lncurses >nul 2>&1
if !errorlevel! equ 0 (
    echo [SUCCESS] ncurses library found.
    del test_ncurses.c test_ncurses.exe >nul 2>&1
) else (
    echo [ERROR] ncurses library not found.
    del test_ncurses.c >nul 2>&1
    goto :error_no_ncurses
)

REM Parse command line arguments
set "INSTALL_TYPE=system"

:parse_args
if "%~1"=="" goto :build
if "%~1"=="--user" (
    set "INSTALL_TYPE=user"
    shift
    goto :parse_args
)
if "%~1"=="--help" goto :show_help
if "%~1"=="-h" goto :show_help

echo [ERROR] Unknown option: %~1
goto :show_help

:build
echo [INFO] Building Micrn Editor...

REM Clean previous build
make clean >nul 2>&1

REM Build the program
make
if !errorlevel! neq 0 (
    echo [ERROR] Build failed.
    goto :error_build
)

echo [SUCCESS] Build completed successfully.

:install
if "%INSTALL_TYPE%"=="user" (
    echo [INFO] Installing to user directory...
    goto :install_user
) else (
    echo [INFO] Installing system-wide...
    goto :install_system
)

:install_system
make install
if !errorlevel! neq 0 (
    echo [ERROR] System installation failed.
    echo [INFO] You may need administrator privileges.
    echo [INFO] Try running this script as administrator or use --user option.
    goto :error_install
)
goto :test_install

:install_user
make install-user
if !errorlevel! neq 0 (
    echo [ERROR] User installation failed.
    goto :error_install
)

REM Check if user's local bin is in PATH
echo %PATH% | findstr /i "%USERPROFILE%\.local\bin" >nul
if !errorlevel! neq 0 (
    echo [WARNING] %USERPROFILE%\.local\bin is not in your PATH.
    echo [INFO] You may need to add it manually or restart your terminal.
)

:test_install
echo [INFO] Testing installation...

where micrn >nul 2>&1
if !errorlevel! equ 0 (
    echo [SUCCESS] Installation successful! You can now run 'micrn' from anywhere.
    echo [INFO] Try: micrn --help
) else (
    echo [WARNING] Command 'micrn' not found in PATH.
    echo [INFO] You may need to restart your terminal or update your PATH.
)

echo.
echo [SUCCESS] Installation complete!
echo.
echo [INFO] You can now use the Micrn Editor by typing 'micrn [filename]'

if "%INSTALL_TYPE%"=="system" (
    echo [INFO] To uninstall: make uninstall
) else (
    echo [INFO] To uninstall: make uninstall-user
)

goto :end

:show_help
echo Usage: install.bat [OPTIONS]
echo.
echo Options:
echo   --user       Install to user's local directory (no admin required)
echo   --help, -h   Show this help message
echo.
echo Examples:
echo   install.bat          # System installation (requires admin)
echo   install.bat --user   # User installation
echo.
goto :end

:error_no_compiler
echo.
echo [ERROR] GCC compiler not found.
echo [INFO] Please install one of the following:
echo   - MSYS2: https://www.msys2.org/
echo   - MinGW-w64: https://www.mingw-w64.org/
echo   - Cygwin: https://www.cygwin.com/
echo.
echo After installation, make sure gcc is in your PATH.
goto :end

:error_no_make
echo.
echo [ERROR] Make utility not found.
echo [INFO] Make should be included with MinGW/MSYS2/Cygwin.
echo [INFO] Please check your installation.
goto :end

:error_no_ncurses
echo.
echo [ERROR] ncurses library not found.
echo.
if "%ENVIRONMENT%"=="MSYS2" (
    echo [INFO] To install ncurses in MSYS2:
    echo   pacman -S ncurses-devel
) else if "%ENVIRONMENT%"=="CYGWIN" (
    echo [INFO] To install ncurses in Cygwin:
    echo   Run Cygwin setup and install libncurses-devel
) else (
    echo [INFO] Please install ncurses development library for your environment.
)
goto :end

:error_build
echo.
echo [ERROR] Build failed. Please check the error messages above.
goto :end

:error_install
echo.
echo [ERROR] Installation failed. Please check the error messages above.
goto :end

:end
pause
exit /b %errorlevel%
