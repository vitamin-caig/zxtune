@echo off
set INSTPATH=E:\Build\boost-1.49.0-vc80-x86_64
mkdir %INSTPATH%
..\build.bat %INSTPATH% msvc 64
