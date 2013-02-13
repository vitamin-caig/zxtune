@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO SetDx
call "%VS_PATH%\VC\vcvarsall.bat" x64

:SetDx

SET | FIND "DXSDK_DIR" > NUL || GOTO SetMode
ECHO %INCLUDE% | FIND "%DXSDK_DIR%" > NUL && GOTO SetMode
SET INCLUDE=%INCLUDE%;%DXSDK_DIR%\Include

:SetMode
SET platform=windows
SET arch=x86_64
