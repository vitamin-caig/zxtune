@ECHO OFF

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio .NET 2003
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\Common7\Tools\vsvars32.bat"

SET platform=windows
SET arch=x86

:Quit
