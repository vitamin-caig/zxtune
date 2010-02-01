@ECHO OFF

call make\platforms\setup_msvs8_x64.bat

call build.bat zxtune123 windows x86_64
