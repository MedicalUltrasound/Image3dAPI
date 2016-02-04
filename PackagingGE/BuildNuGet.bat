@echo off
set NUGET_REPO=%1

set PATH=%PATH%;C:\Python27
set PATH=%PATH%;"C:\Program Files\Git\bin"
set AUTOPKG_FILE=..\PackagingGE\Image3dAPI.autopkg

cd ..\Image3dAPI

echo Building project:
msbuild /nologo /verbosity:minimal /target:Build /property:Configuration="Debug";Platform="x64" Image3dAPI.vcxproj
IF %ERRORLEVEL% NEQ 0 exit /B 1

echo Determine previous tag:
git describe --abbrev=0 --tags > PREV_TAG.txt
set /p PREV_TAG=< PREV_TAG.txt

echo Creating new GIT tag:
python.exe ..\PackagingGE\DetermineNextTag.py minor > NEW_TAG.txt
IF %ERRORLEVEL% NEQ 0 exit /B 1
set /p NEW_TAG=< NEW_TAG.txt
git tag %NEW_TAG%
if DEFINED NUGET_REPO (
  git push origin %NEW_TAG%
)

echo Generating changelog (with tag decoration, graph and change stats):
git log %PREV_TAG%..%NEW_TAG% --decorate --graph --stat > changelog.txt

CALL ../PackagingGE/PackagePublishNuget.bat %AUTOPKG_FILE% %NEW_TAG% %NUGET_REPO%
IF %ERRORLEVEL% NEQ 0 exit /B 1

echo [done]
