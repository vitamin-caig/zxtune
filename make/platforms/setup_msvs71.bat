@ECHO OFF

SET VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio .NET 2003
ECHO %PATH% | FIND "%VS_PATH%" > NUL && GOTO Quit

call "%VS_PATH%\Common7\Tools\vsvars32.bat"
SET MSVS_VERSION=vc71

SET BOOST_VERSION=1_47
SET QT_VERSION=4.7.4
SET BUILD_ARCH=x86
SET self=%0%
call %self:setup_msvs71=setup_build%
call %self:setup_msvs71=setup_boost%
call %self:setup_msvs71=setup_qt%
SET self=
:Quit
