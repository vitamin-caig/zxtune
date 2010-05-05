SET Binaries=%~1%
SET Platform=%2%
SET Arch=%3%
SET Languages=en
SET Formats=txt

:: checking for textator
textator --version > NUL || GOTO Error
zip -v > NUL || GOTO Error

ECHO Updating
:: svn up > NUL || GOTO Error

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

ECHO Building %Binaries% for platform %Platform%_%Arch%
FOR %%B IN (%Binaries%) DO (
ECHO Building %%B
make -j %NUMBER_OF_PROCESSORS% mode=release platform=%Platform% defines=ZXTUNE_VERSION=rev%Revision% -C apps\%%B > %TargetDir%\build_%%B.log 2>&1 || GOTO Error

ECHO Compressing %TargetDir%\%%B_r%Suffix%.zip
zip -9Dj %TargetDir%\%%B_r%Suffix%.zip bin\%Platform%\release\%%B.exe apps\zxtune.conf || GOTO Error
IF EXIST apps\%%B\dist\%%B.txt (
ECHO Generating manuals
FOR /D %%F IN (%Formats%) DO (
FOR /D %%L IN (%Languages%) DO (
textator --process --keys %%L,%%F --asm --output bin\%%B_%%L.%%F apps\%%B\dist\%%B.txt || GOTO Error
zip -9Dj %TargetDir%\%%B_r%Suffix%.zip bin\%%B_%%L.%%F || GOTO Error
)
)
)
ECHO Copy additional files
copy bin\%Platform%\release\%%B.exe.pdb %TargetDir%
)
ECHO Done!
GOTO Exit
:Error
ECHO Failed
:Exit
