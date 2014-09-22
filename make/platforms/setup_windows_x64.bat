@echo off

IF NOT "%VisualStudioVersion%" == "12.0" call "%PROGRAMFILES(x86)%\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86_amd64

:: Hack against setting Platform=x64 variable required for msbuild
SET Platform=
SET platform=windows
SET arch=x86_64
