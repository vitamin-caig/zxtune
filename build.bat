SET Platform=%1%
SET Arch=%2%
SET Mode=release

:: checking for textator
textator --version > NUL || GOTO Error
zip -v > NUL || GOTO Error

ECHO Updating
svn up > NUL || GOTO Error

ECHO Cleaning
make clean_package platform=%Platform% arch=%Arch% release=1 -C apps > NUL || GOTO Error
make clean platform=%Platform% arch=%Arch% release=1 -C apps > NUL || GOTO Error

make -j package platform=%Platform% arch=%Arch% release=1 -C apps && GOTO Exit
:Error
ECHO Failed
SET
:Exit
pause