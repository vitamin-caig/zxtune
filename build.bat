:: checking for textator
textator --version > NUL || GOTO Error
zip -v > NUL || GOTO Error

ECHO Updating
svn up > NUL || GOTO Error

ECHO Cleaning
make clean_package release=1 -C apps > NUL || GOTO Error
make clean release=1 -C apps > NUL || GOTO Error

make -j package release=1 -C apps && GOTO Exit
:Error
ECHO Failed
SET
:Exit
pause