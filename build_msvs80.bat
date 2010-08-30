@ECHO OFF

call make\platforms\setup_msvs80.bat

call build.bat "zxtune123 zxtune-qt" windows vc80_x86
