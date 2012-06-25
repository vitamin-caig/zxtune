@echo off

SET MINGW_PATH=%CD:~0,2%\Build\MinGW\bin
ECHO %PATH% | FIND "%MINGW_PATH%" > NUL && GOTO Quit

SET LIB=
SET INCLUDE=
SET PATH=c:\windows;c:\windows\system32;%MINGW_PATH%;C:\Perl\bin

:Quit
SET INSTPATH=E:\Build\qt-4.8.1-mingw-x86
mkdir %INSTPATH%

SET QMAKEPATH=%CD%

..\build.bat win32-g++-32 -prefix %INSTPATH%
