@echo off
set INSTPATH=E:\Build\boost-1.49.0-vc71-x86
mkdir %INSTPATH%
..\build.bat %INSTPATH% msvc-7.1 32
