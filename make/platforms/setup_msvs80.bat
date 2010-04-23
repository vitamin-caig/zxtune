@ECHO OFF

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

SET PATH=%VS_PATH%\VC\bin;%VS_PATH%\Common7\IDE;%PATH%
SET LIB=%VS_PATH%\VC\lib;%VS_PATH%\VC\PlatformSDK\Lib
SET INCLUDE=%VS_PATH%\VC\include;%VS_PATH%\VC\PlatformSDK\Include
SET MSVS_VERSION=vc80

SET self=%0%
call %self:setup_msvs80=setup_boost%
call %self:setup_msvs80=setup_qt%
SET self=
:Quit
