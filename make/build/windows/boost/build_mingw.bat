@echo off
copy /Y user-config.jam tools\build\v2\user-config.jam

set MINGW_ROOT_DIRECTORY=E:\Build\MinGW
ECHO %PATH% | FIND "%MINGW_ROOT_DIRECTORY%" > NUL && GOTO noset
set PATH=%MINGW_ROOT_DIRECTORY%\bin;%PATH%
:noset

set OPTIONS=-j%NUMBER_OF_PROCESSORS% --build-type=minimal --layout=tagged
set PARTS=--without-mpi --without-python --without-log --without-math
set BOOST_COMMON_DIR=E:\Build\boost-1.55.0

::mingw x86_64
set BOOST_DIR=%BOOST_COMMON_DIR%-mingw-x86_64
bjam --stagedir=%BOOST_DIR% %OPTIONS% %PARTS% address-model=64 toolset=gcc stage
::mingw x86
set BOOST_DIR=%BOOST_COMMON_DIR%-mingw-x86
bjam --stagedir=%BOOST_DIR% %OPTIONS% %PARTS% address-model=32 toolset=gcc stage
