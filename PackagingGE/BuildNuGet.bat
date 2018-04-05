@echo off
set NUGET_REPO=%1

set PATH=%PATH%;C:\Python27
set PATH=%PATH%;"C:\Program Files\Git\bin"

:: Dependencies:
:: * Python.exe in PATH
:: * Visual Studio command prompt (msbuild & C++ compiler in PATH)
:: * NuGet.exe in PATH (https://www.nuget.org/downloads)

pushd ..

if DEFINED NUGET_REPO (
  python.exe PackagingGE\DetermineNextTag.py minor > NEW_TAG.txt
) else (
  :: Local nuget build. Not to be deployed.
  python.exe PackagingGE\DetermineNextTag.py patch > NEW_TAG.txt
)
IF %ERRORLEVEL% NEQ 0 exit /B 1
set /p VERSION=< NEW_TAG.txt


echo Building projects:
msbuild /nologo /verbosity:minimal /target:Image3dAPI;DummyLoader /property:Configuration="Debug";Platform="Win32" Image3dAPI.sln
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Image3dAPI;DummyLoader /property:Configuration="Release";Platform="Win32" Image3dAPI.sln
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Image3dAPI;DummyLoader /property:Configuration="Debug";Platform="x64" Image3dAPI.sln
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Image3dAPI;DummyLoader /property:Configuration="Release";Platform="x64" Image3dAPI.sln
IF %ERRORLEVEL% NEQ 0 exit /B 1

echo Determine previous tag:
git describe --abbrev=0 --tags > PREV_TAG.txt
set /p PREV_TAG=< PREV_TAG.txt


echo Creating new GIT tag:
git tag %VERSION%
if DEFINED NUGET_REPO (
  git push origin %VERSION%
)

echo Generating changelog (with tag decoration, graph and change stats):
git log %PREV_TAG%..%VERSION% --decorate --graph --stat > changelog.txt

pushd Image3dApi
CALL ..\PackagingGE\PackagePublishNuget.bat ..\PackagingGE\Image3dAPI.nuspec %VERSION% %NUGET_REPO%
IF %ERRORLEVEL% NEQ 0 exit /B 1
popd

pushd DummyLoader
CALL ..\PackagingGE\PackagePublishNuget.bat ..\PackagingGE\DummyLoader.redist.nuspec %VERSION% %NUGET_REPO%
IF %ERRORLEVEL% NEQ 0 exit /B 1
popd

popd
echo [done]
