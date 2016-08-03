@echo off
set PRIMARY_NUGET_REPO=%1
set SECONDARY_NUGET_REPO=%2

set PATH=%PATH%;C:\Python27
set PATH=%PATH%;"C:\Program Files\Git\bin"
set AUTOPKG_FILE=..\PackagingGE\Image3dAPI.autopkg

:: Dependencies:
:: * Python.exe in PATH
:: * Visual Studio command prompt (msbuild & C++ compiler in PATH)
:: * CoApp - tools for building C/C++ NuGet packages for Windows (http://coapp.org/)

pushd ..

echo Building projects:
msbuild /nologo /verbosity:minimal /target:Build /property:Configuration="Debug";Platform="x64" Image3dApi\Image3dAPI.vcxproj
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Build /property:Configuration="Debug";Platform="x64" DummyLoader\DummyLoader.vcxproj
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Build /property:Configuration="Release";Platform="x64" DummyLoader\DummyLoader.vcxproj
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Build /property:Configuration="Debug";Platform="Win32" DummyLoader\DummyLoader.vcxproj
IF %ERRORLEVEL% NEQ 0 exit /B 1
msbuild /nologo /verbosity:minimal /target:Build /property:Configuration="Release";Platform="Win32" DummyLoader\DummyLoader.vcxproj
IF %ERRORLEVEL% NEQ 0 exit /B 1

echo Determine previous tag:
git describe --abbrev=0 --tags > PREV_TAG.txt
set /p PREV_TAG=< PREV_TAG.txt

echo Creating new GIT tag:
python.exe PackagingGE\DetermineNextTag.py minor > NEW_TAG.txt
IF %ERRORLEVEL% NEQ 0 exit /B 1
set /p NEW_TAG=< NEW_TAG.txt
git tag %NEW_TAG%
if DEFINED PRIMARY_NUGET_REPO (
  git push origin %NEW_TAG%
)

echo Generating changelog (with tag decoration, graph and change stats):
git log %PREV_TAG%..%NEW_TAG% --decorate --graph --stat > changelog.txt

pushd Image3dApi
CALL ..\PackagingGE\PackagePublishNuget.bat %AUTOPKG_FILE% %NEW_TAG% %PRIMARY_NUGET_REPO%
IF %ERRORLEVEL% NEQ 0 exit /B 1
popd

pushd DummyLoader
CALL ..\PackagingGE\PackagePublishNuget.bat ..\PackagingGE\DummyLoader.autopkg %NEW_TAG% %SECONDARY_NUGET_REPO%
IF %ERRORLEVEL% NEQ 0 exit /B 1
popd

popd
echo [done]
