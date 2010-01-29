@ECHO OFF

SET MINGW_ROOT=C:\MinGW

::set variables if required
ECHO %PATH% | FIND "%MINGW_ROOT%" > NUL
IF ERRORLEVEL 1 SET PATH=%MINGW_ROOT%\bin;%PATH%

call build.bat zxtune123 mingw x86
