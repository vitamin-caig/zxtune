@echo off
copy /Y user-config.jam tools\build\v2\user-config.jam

set OPTIONS=-j%NUMBER_OF_PROCESSORS% --build-type=minimal --layout=tagged
set PARTS=--without-mpi --without-python --without-log
set BOOST_COMMON_DIR=E:\Build\boost-1.55.0

::windows x86_64
set BOOST_DIR=%BOOST_COMMON_DIR%-windows-x86_64
bjam --stagedir=%BOOST_DIR% %OPTIONS% %PARTS% address-model=64 toolset=msvc-12.0 stage
::windows x86
set BOOST_DIR=%BOOST_COMMON_DIR%-windows-x86
::fix for msvc2013
copy /Y has_member_function_callable_with.hpp %BOOST_COMMON_DIR%\include\boost\intrusive\detail\has_member_function_callable_with.hpp
::mkdir %BOOST_DIR%
bjam --prefix=%BOOST_DIR% --includedir=%BOOST_COMMON_DIR%/include %OPTIONS% %PARTS% address-model=32 toolset=msvc-12.0 install
