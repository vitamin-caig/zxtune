SET TARGET=apps/bundle

ECHO Building
make package release=1 -C %TARGET% && GOTO Exit
:Error
ECHO Failed
SET
:Exit
pause
