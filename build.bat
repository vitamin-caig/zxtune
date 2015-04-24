SET TARGET=apps/bundle

IF EXIST variables.bat CALL variables.bat

ECHO Building
make package -C %TARGET% && GOTO Exit
:Error
ECHO Failed
SET
:Exit
pause
