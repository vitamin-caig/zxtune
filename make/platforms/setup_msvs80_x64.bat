@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\VC\vcvarsall.bat" x64
SET TOOLSET=vc80

SET BUILD_ARCH=x86_64
SET self=%0%
call %self:setup_msvs80_x64=setup_build%
call %self:setup_msvs80_x64=setup_boost% 64
call %self:setup_msvs80_x64=setup_qt% 64
call %self:setup_msvs80_x64=setup_directx% 64
SET self=
:Quit
