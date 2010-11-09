@ECHO OFF

SET QT_VERSION=4.5.3
call make\platforms\setup_msvs80.bat

call build.bat "zxtune123 zxtune-qt" windows vc80_x86
