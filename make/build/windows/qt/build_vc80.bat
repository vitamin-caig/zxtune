@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit
SET PATH=c:\windows;c:\windows\system32;%VS_PATH%\vc\bin;%VS_PATH%\common7\ide;%VS_PATH%\common7\tools\bin
SET INCLUDE=%VS_PATH%\vc\include;%VS_PATH%\vc\platformSDK\include
SET LIB=%VS_PATH%\vc\lib;%VS_PATH%\vc\platformsdk\lib

:Quit
..\build.bat win32-msvc2005
