@ECHO OFF

IF NOT "%VisualStudioVersion%" == "12.0" call "%PROGRAMFILES(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

SET platform=windows
SET arch=x86
