@ECHO OFF

IF EXIST variables.bat CALL variables.bat

ECHO "1) windows x86"
ECHO "2) windows x86_64"
ECHO "3) mingw x86"
ECHO "4) mingw x86_64"
ECHO "5) empty"

CHOICE /C 12345

GOTO mode%ERRORLEVEL%

:mode0
:mode255
exit

:mode1
CALL make\platforms\setup_windows.bat
GOTO setps

:mode2
CALL make\platforms\setup_windows_x64.bat
GOTO setps

:mode3
CALL make\platforms\setup_mingw.bat
GOTO setps

:mode4
CALL make\platforms\setup_mingw_x64.bat
GOTO setps

:setps
PROMPT $L%platform% %arch%$G $P$G
:mode5
cmd
