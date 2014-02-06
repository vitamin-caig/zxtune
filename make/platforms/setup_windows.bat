@ECHO OFF

SET VS_PATH=%PROGRAMFILES(x86)%\Microsoft Visual Studio 12.0
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\VC\vcvarsall.bat" x86

SET platform=windows
SET arch=x86

:Quit
