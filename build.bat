SET Binaries=%~1%
SET Platform=%2%
SET Mode=release
SET Arch=%3%

:: checking for textator
textator --version > NUL || GOTO Error
zip -v > NUL || GOTO Error

ECHO Updating
svn up > NUL || GOTO Error

ECHO Cleaning
rmdir /S /Q bin\%Platform%\%Mode% lib\%Platform%\%Mode% obj\%Platform%\%Mode%
FOR %%B IN (%Binaries%) DO (
ECHO Building %%B with mode=%Mode% platform=%Platform% arch=%Arch%
make -j package mode=%Mode% platform=%Platform% arch=%Arch% -C apps\%%B || GOTO Error
ECHO Done!
)
GOTO Exit
:Error
ECHO Failed
:Exit
pause