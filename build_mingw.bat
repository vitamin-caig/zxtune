@ECHO OFF

call make\platforms\setup_mingw.bat

call build.bat "zxtune123 zxtune-qt" mingw x86
