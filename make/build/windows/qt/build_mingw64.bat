@echo off

:: for mingw4.8.1 apply patch from https://bugreports.qt-project.org/secure/attachment/36215/mingw-w64-i686-x86_64-intrinsics.patch
:: copy win32-g++-64 to mkspecs

SET MINGW_PATH=%CD:~0,2%\Build\MinGW\bin
ECHO %PATH% | FIND "%MINGW_PATH%" > NUL && GOTO Quit

SET LIB=
SET INCLUDE=
SET PATH=c:\windows;c:\windows\system32;%MINGW_PATH%

:Quit
SET INSTPATH=%CD:~0,2%\Build\qt-4.8.5-mingw-x86_64
mkdir %INSTPATH%

SET QMAKEPATH=%CD%

..\build.bat win32-g++-64 -prefix %INSTPATH%
