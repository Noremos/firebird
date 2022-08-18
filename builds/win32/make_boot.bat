::
:: This bat file doesn't use cd, all the paths are full paths.
:: with this convention this bat file is position independent
:: and it will be easier to move the place of somefiles.
::

@echo off
set ERRLEV=0

:CHECK_ENV
@call setenvvar.bat %*
@if errorlevel 1 (goto :END)


::===========
:MAIN
@echo.

@echo Cleaning output directory
@rmdir /S /Q "%FB_OUTPUT_DIR%" 2>nul
:: short delay to let OS complete actions by rmdir above
@timeout 1 >nul

@echo Creating directories
:: Create the directory hierarchy.
for %%v in ( alice auth burp dsql gpre isql jrd misc msgs examples yvalve utilities) do (
  @mkdir %FB_GEN_DIR%\%%v 2>nul
)

@mkdir %FB_GEN_DIR%\utilities\gstat 2>nul
@mkdir %FB_GEN_DIR%\auth\SecurityDatabase 2>nul
@mkdir %FB_GEN_DIR%\gpre\std 2>nul

@mkdir %FB_OUTPUT_DIR%\include\firebird\impl 2>nul
@mkdir %FB_OUTPUT_DIR%\tzdata 2>nul

call :interfaces
if "%ERRLEV%"=="1" goto :END

call :btyacc
if "%ERRLEV%"=="1" goto :END

call :LibTom
if "%ERRLEV%"=="1" goto :END

call :decNumber
if "%ERRLEV%"=="1" goto :END

if "%FB_TARGET_PLATFORM%"=="x64" call :ttmath
if "%ERRLEV%"=="1" goto :END

call :re2
if "%ERRLEV%"=="1" goto :END

call :zlib
if "%ERRLEV%"=="1" goto :END

@echo Generating DSQL parser...
@call parse.bat %*
if "%ERRLEV%"=="1" goto :END

::=======
call :gpre_boot
if "%ERRLEV%"=="1" goto :END

::=======
@echo Preprocessing the source files needed to build gpre and isql...
@call preprocess.bat %FB_CONFIG% BOOT

::=======
call :engine
if "%ERRLEV%"=="1" goto :END

call :gpre
if "%ERRLEV%"=="1" goto :END

call :isql
if "%ERRLEV%"=="1" goto :END

@copy %FB_ROOT_PATH%\builds\install\misc\firebird.conf %FB_BIN_DIR%\firebird.conf

:: Copy ICU and zlib to the output directory
@mkdir %FB_BIN_DIR%
@copy %FB_ROOT_PATH%\extern\icu\icudt???.dat %FB_BIN_DIR% >nul 2>&1
@copy %FB_ICU_SOURCE_BIN%\*.dll %FB_BIN_DIR% >nul 2>&1
@copy %FB_ROOT_PATH%\extern\icu\tzdata-extract\* %FB_OUTPUT_DIR%\tzdata >nul 2>&1
@copy %FB_ROOT_PATH%\extern\zlib\%FB_TARGET_PLATFORM%\*.dll %FB_BIN_DIR% >nul 2>&1

::=======
@call :databases

::=======
@echo Preprocessing the entire source tree...
@call preprocess.bat %FB_CONFIG%

::=======
@call :msgs
if "%ERRLEV%"=="1" goto :END

::=======
@call create_msgs.bat %FB_CONFIG%
::=======

@call :NEXT_STEP
@goto :END


::===================
:: BUILD btyacc
:btyacc
@echo.
@echo Building btyacc (%FB_OBJ_DIR%)...
@call compile.bat builds\win32\%VS_VER%\FirebirdBoot btyacc_%FB_TARGET_PLATFORM%.log btyacc
if errorlevel 1 call :boot2 btyacc
goto :EOF

::===================
:: BUILD LibTom
:LibTom
@echo.
@echo Building LibTomMath (%FB_OBJ_DIR%)...
@call compile.bat extern\libtommath\libtommath_MSVC%MSVC_VERSION% libtommath_%FB_CONFIG%_%FB_TARGET_PLATFORM%.log libtommath
if errorlevel 1 call :boot2 libtommath_%FB_OBJ_DIR%
@echo Building LibTomCrypt (%FB_OBJ_DIR%)...
@call compile.bat extern\libtomcrypt\libtomcrypt_MSVC%MSVC_VERSION% libtomcrypt_%FB_CONFIG%_%FB_TARGET_PLATFORM%.log libtomcrypt
if errorlevel 1 call :boot2 libtomcrypt_%FB_OBJ_DIR%
goto :EOF

::===================
:: BUILD decNumber
:decNumber
@echo.
@echo Building decNumber (%FB_OBJ_DIR%)...
@call compile.bat extern\decNumber\msvc\decNumber_MSVC%MSVC_VERSION% decNumber_%FB_CONFIG%_%FB_TARGET_PLATFORM%.log decNumber
if errorlevel 1 call :boot2 decNumber_%FB_OBJ_DIR%
goto :EOF

::===================
:: BUILD ttmath
:ttmath
@echo.
@echo Building ttmath (%FB_OBJ_DIR%)...
@mkdir %FB_ROOT_PATH%\extern\ttmath\%FB_CONFIG% 2>nul
if /I "%FB_CONFIG%"=="debug" (
  @ml64.exe /c /Zi /Fo %FB_ROOT_PATH%\extern\ttmath\%FB_CONFIG%\ttmathuint_x86_64_msvc.obj %FB_ROOT_PATH%\extern\ttmath\ttmathuint_x86_64_msvc.asm
) else (
  @ml64.exe /c /Fo %FB_ROOT_PATH%\extern\ttmath\%FB_CONFIG%\ttmathuint_x86_64_msvc.obj %FB_ROOT_PATH%\extern\ttmath\ttmathuint_x86_64_msvc.asm
)
if errorlevel 1 call :boot2 ttmath_%FB_OBJ_DIR%
goto :EOF

::===================
:: BUILD re2
:re2
@echo.
@echo Building re2...
@mkdir %FB_ROOT_PATH%\extern\re2\builds\%FB_TARGET_PLATFORM% 2>nul
@pushd %FB_ROOT_PATH%\extern\re2\builds\%FB_TARGET_PLATFORM%
@cmake -G "%MSVC_CMAKE_GENERATOR%" -A %FB_TARGET_PLATFORM% -S %FB_ROOT_PATH%\extern\re2
if errorlevel 1 call :boot2 re2
@cmake --build %FB_ROOT_PATH%\extern\re2\builds\%FB_TARGET_PLATFORM% --target ALL_BUILD --config %FB_CONFIG% > re2_%FB_CONFIG%_%FB_TARGET_PLATFORM%.log
@popd
goto :EOF

::===================
:: Build CLOOP and generate interface headers
:interfaces
@echo.
@echo Building CLOOP and generating interfaces...
@nmake /s /x interfaces_%FB_TARGET_PLATFORM%.log /f gen_helper.nmake updateCloopInterfaces
if errorlevel 1 call :boot2 interfaces
goto :EOF

::===================
:: Extract zlib
:zlib
@echo Extracting pre-built zlib
if exist %FB_ROOT_PATH%\extern\zlib\zlib.h (
  @echo %FB_ROOT_PATH%\extern\zlib\zlib.h already extracted
) else (
  %FB_ROOT_PATH%\extern\zlib\zlib.exe -y > zlib_%FB_TARGET_PLATFORM%.log
  if errorlevel 1 call :boot2 zlib
)
goto :EOF

::===================
:: BUILD gpre_boot
:gpre_boot
@echo.
@echo Building gpre_boot (%FB_OBJ_DIR%)...
@call compile.bat builds\win32\%VS_VER%\FirebirdBoot gpre_boot_%FB_TARGET_PLATFORM%.log gpre_boot
if errorlevel 1 call :boot2 gpre_boot
goto :EOF

::===================
:: BUILD engine
:engine
@echo.
@echo Building engine (%FB_OBJ_DIR%)...
@call compile.bat builds\win32\%VS_VER%\Firebird engine_%FB_TARGET_PLATFORM%.log DLLs\engine
@call compile.bat builds\win32\%VS_VER%\Firebird engine_%FB_TARGET_PLATFORM%.log DLLs\ib_util
if errorlevel 1 call :boot2 engine
@goto :EOF

::===================
:: BUILD gpre
:gpre
@echo.
@echo Building gpre (%FB_OBJ_DIR%)...
@call compile.bat builds\win32\%VS_VER%\Firebird gpre_%FB_TARGET_PLATFORM%.log EXEs\gpre
if errorlevel 1 call :boot2 gpre
@goto :EOF

::===================
:: BUILD isql
:isql
@echo.
@echo Building isql (%FB_OBJ_DIR%)...
@call compile.bat builds\win32\%VS_VER%\Firebird isql_%FB_TARGET_PLATFORM%.log EXEs\isql
if errorlevel 1 call :boot2 isql
@goto :EOF

::===================
:: ERROR boot
:boot2
echo.
echo Error building %1, see %1_%FB_TARGET_PLATFORM%.log
echo.
set ERRLEV=1
goto :EOF


::===================
:: BUILD messages
:msgs
@echo.
@echo Building build_msg (%FB_OBJ_DIR%)...
@call compile.bat builds\win32\%VS_VER%\FirebirdBoot build_msg_%FB_TARGET_PLATFORM%.log build_msg
if errorlevel 1 goto :msgs2
@goto :EOF
:msgs2
echo.
echo Error building build_msg, see build_msg_%FB_TARGET_PLATFORM%.log
echo.
set ERRLEV=1
goto :EOF

::==============
:databases
@rmdir /s /q %FB_GEN_DIR%\dbs 2>nul
@mkdir %FB_GEN_DIR%\dbs 2>nul

@echo Create security5.fdb...
@echo create database '%FB_GEN_DB_DIR%\dbs\security5.fdb'; | "%FB_BIN_DIR%\isql" -q
@echo Apply security.sql...
@"%FB_BIN_DIR%\isql" -q %FB_GEN_DB_DIR%/dbs/security5.fdb -i %FB_ROOT_PATH%\src\dbs\security.sql
@copy %FB_GEN_DIR%\dbs\security5.fdb %FB_GEN_DIR%\dbs\security.fdb > nul

@echo Creating metadata.fdb...
@echo create database '%FB_GEN_DB_DIR%/dbs/metadata.fdb'; | "%FB_BIN_DIR%\isql" -q -sqldialect 1
@copy %FB_GEN_DIR%\dbs\metadata.fdb %FB_GEN_DIR%\dbs\yachts.lnk > nul

@goto :EOF


::==============
:NEXT_STEP
@echo.
@echo    You may now run make_all.bat [DEBUG] [CLEAN]
@echo.
@goto :EOF

:END
