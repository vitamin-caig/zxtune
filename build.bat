SET Binary=%1%
SET Platform=%2%
SET Arch=%3%
SET Languages=en
SET Formats=txt

ECHO Building %Binary% for platform %Platform%_%Arch%

:: checking for textator
textator --version > NUL || GOTO Error
zip -v > NUL || GOTO Error

ECHO Updating
svn up > NUL || GOTO Error

:: determine current build version
SET Revision=00000
FOR /F "tokens=2" %%R IN ('svn info ^| FIND "Revision"') DO SET Revision=000%%R
SET Revision=%Revision:~-4%
FOR /F %%M IN ('svn st ^| FIND "M  "') DO SET Revision=%Revision%-devel
ECHO Revision %Revision%

SET Suffix=%Revision%_%Platform%_%Arch%

:: calculate target dir
SET TargetDir=Builds\Revision%Suffix%
ECHO Target dir %TargetDir%

ECHO Clearing
IF EXIST %TargetDir% RMDIR /S /Q %TargetDir% || GOTO Error
RMDIR /S /Q bin\%Platform%\release lib\%Platform%\release obj\%Platform%\release

ECHO Creating build dir
mkdir %TargetDir% || GOTO Error

ECHO Building
make -j %NUMBER_OF_PROCESSORS% mode=release platform=%Platform% defines=ZXTUNE_VERSION=rev%Revision% -C apps\zxtune123 > %TargetDir%\build.log || GOTO Error

SET ZipFile=%TargetDir%\%Binary%_r%Suffix%.zip
ECHO Compressing %ZipFile%
zip -9Dj %ZipFile% bin\%Platform%\release\%Binary%.exe apps\zxtune.conf || GOTO Error
ECHO Generating manuals
FOR /D %%F IN (%Formats%) DO (
FOR /D %%L IN (%Languages%) DO (
textator --process --keys %%L,%%F --asm --output bin\%Binary%_%%L.%%F apps\%Binary%\dist\%Binary%.txt || GOTO Error
zip -9Dj %ZipFile% bin\%Binary%_%%L.%%F || GOTO Error
)
)

ECHO Copy additional files
copy bin\%Platform%\release\%Binary%.exe.pdb %TargetDir%

ECHO Done!
GOTO Exit

:Error
ECHO Failed
:Exit
