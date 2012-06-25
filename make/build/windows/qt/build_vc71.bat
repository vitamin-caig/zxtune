@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio .NET 2003
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

SET PATH=c:\windows;c:\windows\system32;%VS_PATH%\vc7\bin;%VS_PATH%\common7\ide;%VS_PATH%\common7\tools\bin;C:\Perl\bin
SET INCLUDE=%VS_PATH%\vc7\include;%VS_PATH%\vc7\platformSDK\include
SET LIB=%VS_PATH%\vc7\lib;%VS_PATH%\vc7\platformsdk\lib

:Quit
SET INSTPATH=E:\Build\qt-4.8.1-vc71-x86
mkdir %INSTPATH%

SET QMAKEPATH=%CD%
..\build.bat win32-msvc2003 -prefix %INSTPATH%
