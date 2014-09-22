SET TARGET=apps/bundle

IF EXIST variables.bat CALL variables.bat

ECHO Building
make package release=1 -C %TARGET% && GOTO Exit
:Error
ECHO Failed
SET
:Exit
pause
