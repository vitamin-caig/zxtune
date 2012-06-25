@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

SET PATH=c:\windows;c:\windows\system32;%VS_PATH%\vc\bin\amd64;%VS_PATH%\vc\bin;%VS_PATH%\Common7\Tools\Bin;C:\Perl\bin
SET INCLUDE=%VS_PATH%\vc\include;%VS_PATH%\vc\platformSDK\include
SET LIB=%VS_PATH%\vc\lib\amd64;%VS_PATH%\vc\platformsdk\lib\amd64

:Quit
SET INSTPATH=E:\Build\qt-4.8.1-vc80-x86_64
mkdir %INSTPATH%

SET QMAKEPATH=%CD%
..\build.bat win32-msvc2005 -prefix %INSTPATH%

