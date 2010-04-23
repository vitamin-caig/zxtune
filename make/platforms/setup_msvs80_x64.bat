@echo off

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

SET PATH=%VS_PATH%\VC\bin\x86_amd64;%VS_PATH%\VC\bin;%VS_PATH%\Common7\IDE;%PATH%
SET LIB=%VS_PATH%\VC\lib\amd64;%VS_PATH%\VC\PlatformSDK\Lib\amd64
SET INCLUDE=%VS_PATH%\VC\include;%VS_PATH%\VC\PlatformSDK\Include
SET MSVS_VERSION=vc80

SET self=%0%
call %self:setup_msvs80_x64=setup_boost% 64
SET self=
:Quit
