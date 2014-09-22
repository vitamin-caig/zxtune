@echo off

:: copy win32-msvc2012 to mkspecs

SET VS_PATH=%PROGRAMFILES(x86)%\Microsoft Visual Studio 12.0
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\VC\vcvarsall.bat" x86

:Quit
SET INSTPATH=E:\Build\qt-4.8.5-windows-x86
mkdir %INSTPATH%

SET QMAKEPATH=%CD%

..\build.bat win32-msvc2012 -prefix %INSTPATH%
