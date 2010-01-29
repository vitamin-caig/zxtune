SET Binary=%1%
SET Platform=%2%
SET Arch=%3%

ECHO Building %Binary% for platform %Platform%_%Arch%

:: checking for textator
textator --version > NUL
IF %ERRORLEVEL% NEQ 0 GOTO Error
zip -v > NUL
IF %ERRORLEVEL% NEQ 0 GOTO Error

:: determine current build version
SET Revision=
FOR /F "tokens=2" %%R IN ('svn info ^| FIND "Revision"') DO SET Revision=000%%R
SET Revision=%Revision:~-4%
FOR /F %%M IN ('svn st ^| FIND "M  "') DO SET Revision=%Revision%-devel
echo Revision %Revision%

SET Suffix=%Revision%_%Platform%_%Arch%

:: calculate target dir
SET TargetDir=Builds\Revision%Suffix%
echo Target dir %TargetDir%

ECHO Clearing
IF EXIST %TargetDir% RMDIR /S /Q %TargetDir%
IF %ERRORLEVEL% NEQ 0 GOTO Error
RMDIR /S /Q bin\%Platform%\release lib\%Platform%\release obj\%Platform%\release

ECHO Creating build dir
mkdir %TargetDir%
IF %ERRORLEVEL% NEQ 0 GOTO Error

ECHO Building
make mode=release platform=%Platform% defines=ZXTUNE_VERSION=rev%Revision% -C apps\zxtune123 > %TargetDir%\build.log
::IF %ERRORLEVEL% NEQ 0 GOTO Error

SET ZipFile=%TargetDir%\%Binary%_r%Suffix%.zip
ECHO Compressing %ZipFile%
zip -9Dj %ZipFile% bin\%Platform%\release\%Binary%.exe
IF %ERRORLEVEL% NEQ 0 GOTO Error

ECHO Copy additional files
copy bin\%Platform%\release\%Binary%.exe.pdb %TargetDir%

ECHO Done!
GOTO Exit

:Error
ECHO Failed
:Exit
