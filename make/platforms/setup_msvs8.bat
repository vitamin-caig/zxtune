@echo off

set BOOST_VERSION=1_40
set BOOST_DIR=c:\Boost_%BOOST_VERSION%
set VS_PATH=%PROGRAMFILES%\Microsoft Visual Studio 8
set PATH=%VS_PATH%\VC\bin;%VS_PATH%\Common7\IDE;%PATH%
set LIB=%BOOST_DIR%\lib;%VS_PATH%\VC\lib;%VS_PATH%\VC\PlatformSDK\Lib
set INCLUDE=%BOOST_DIR%;%VS_PATH%\VC\include;%VS_PATH%\VC\PlatformSDK\Include
set MSVS_VERSION=vc80
