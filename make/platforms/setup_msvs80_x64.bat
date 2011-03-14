@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\VC\vcvarsall.bat" x86_amd64
SET MSVS_VERSION=vc80

SET BUILD_ARCH=%MSVS_VERSION%_x86_64
SET self=%0%
call %self:setup_msvs80_x64=setup_build%
call %self:setup_msvs80_x64=setup_boost% 64
call %self:setup_msvs80_x64=setup_qt% 64
SET self=
:Quit
