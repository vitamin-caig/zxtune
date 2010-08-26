@ECHO OFF

SET self=%0%
call %self:setup_mingw=setup_build%
SET self=

SET MINGW_DIR=%BUILD_TOOLS_DIR%\MinGW\bin

ECHO %PATH% | FIND "%MINGW_DIR%" > NUL && GOTO Quit

SET PATH=%MINGW_DIR%;%PATH%

:Quit
