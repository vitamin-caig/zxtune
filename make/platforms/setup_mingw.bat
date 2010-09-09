@ECHO OFF

SET self=%0%
call %self:setup_mingw=setup_build%

SET MINGW_DIR=%BUILD_TOOLS_DIR%\MinGW\bin

ECHO %PATH% | FIND "%MINGW_DIR%" > NUL && GOTO Quit

:: for simplification
SET MSVS_VERSION=mingw
SET PATH=%MINGW_DIR%;%PATH%
call %self:setup_mingw=setup_qt%
SET MSVS_VERSION=
SET CPATH=%INCLUDE%
SET LIBRARY_PATH=%LIB%
:Quit
SET self=
