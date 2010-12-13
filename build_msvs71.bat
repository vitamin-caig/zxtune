@ECHO OFF

call make\platforms\setup_msvs71.bat

call build.bat windows vc71_x86
