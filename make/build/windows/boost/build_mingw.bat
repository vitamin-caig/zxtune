@echo off

set MINGW_ROOT_DIRECTORY=E:\Build\mingw64-8.1.0-posix-sjlj
ECHO %PATH% | FIND "%MINGW_ROOT_DIRECTORY%" > NUL && GOTO noset
set PATH=%MINGW_ROOT_DIRECTORY%\bin;%PATH%
:noset

REM bootstrap.bat
set BOOST_COMMON_DIR=e:\Build\boost-1.75.0

b2    toolset=gcc link=static threading=multi target-os=windows variant=release --layout=system ^
      address-model=64 cxxflags=-mno-ms-bitfields cxxflags=-mmmx cxxflags=-msse cxxflags=-msse2 cxxflags=-ffunction-sections cxxflags=-fdata-sections cxxflags=-std=c++2a ^
      --with-filesystem --with-locale --with-program_options --with-system -j%NUMBER_OF_PROCESSORS% ^
      --includedir=%BOOST_COMMON_DIR%/include --libdir=%BOOST_COMMON_DIR%-mingw-x86_64/lib install
