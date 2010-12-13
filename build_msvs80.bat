@ECHO OFF

SET QT_VERSION=4.5.3
call make\platforms\setup_msvs80.bat

call build.bat windows vc80_x86
