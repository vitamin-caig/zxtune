@ECHO OFF

call make\platforms\setup_msvs80_x64.bat

call build.bat windows vc80_x86_64
