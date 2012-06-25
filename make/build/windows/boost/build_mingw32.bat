@echo off
set MINGW_ROOT_DIRECTORY=e:\build\MinGW
ECHO %PATH% | FIND "%MINGW_ROOT_DIRECTORY%" > NUL && GOTO noset
set PATH=%MINGW_ROOT_DIRECTORY%\bin;%PATH%
:noset
set INSTPATH=E:\Build\boost-1.49.0-mingw-x86
mkdir %INSTPATH%
..\build.bat %INSTPATH% gcc 32
