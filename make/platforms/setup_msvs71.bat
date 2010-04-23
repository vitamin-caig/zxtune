@ECHO OFF

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio .NET 2003
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

SET PATH=%VS_PATH%\Vc7\bin;%VS_PATH%\Common7\IDE;%PATH%
SET LIB=%VS_PATH%\Vc7\lib;%VS_PATH%\Vc7\PlatformSDK\Lib
SET INCLUDE=%VS_PATH%\Vc7\include;%VS_PATH%\Vc7\PlatformSDK\Include
SET MSVS_VERSION=vc71

SET self=%0%
call %self:setup_msvs71=setup_boost%
call %self:setup_msvs71=setup_qt%
SET self=
:Quit
