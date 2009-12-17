@echo off

set BOOST_VERSION=1_40
set BOOST_DIR=c:\Boost_%BOOST_VERSION%
set VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio .NET 2003
set PATH=%VS_PATH%\Vc7\bin;%VS_PATH%\Common7\IDE;%PATH%
set LIB=%BOOST_DIR%\lib;%VS_PATH%\Vc7\lib;%VS_PATH%\Vc7\PlatformSDK\Lib
set INCLUDE=%BOOST_DIR%;%VS_PATH%\Vc7\include;%VS_PATH%\Vc7\PlatformSDK\Include
set MSVS_VERSION=vc71
