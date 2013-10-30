:: checking for textator

ECHO Updating
svn up > NUL || GOTO Error

SET TARGET=apps/bundle

ECHO Cleaning
make clean release=1 -C %TARGET% > NUL || GOTO Error

ECHO Building
make package release=1 -C %TARGET% && GOTO Exit
:Error
ECHO Failed
SET
:Exit
pause
