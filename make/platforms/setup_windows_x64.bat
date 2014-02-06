@echo off

SET VS_PATH=%PROGRAMFILES(x86)%\Microsoft Visual Studio 12.0
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\VC\vcvarsall.bat" x86_amd64

:Quit
SET platform=windows
SET arch=x86_64
