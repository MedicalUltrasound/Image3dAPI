@echo off
set NUSPEC_FILE=%1
set VERSION_NUMBER=%2
set NUGET_REPO=%3

:: Change NuGet packaging version and project URL
python.exe ..\PackagingGE\SetAutopkgVersion.py %NUSPEC_FILE% %VERSION_NUMBER% https://github.com/MedicalUltrasound/Image3dAPI/tree/%VERSION_NUMBER% 
IF %ERRORLEVEL% NEQ 0 exit /B 1

:: Package artifacts
nuget pack %NUSPEC_FILE%
IF %ERRORLEVEL% NEQ 0 exit /B 1

:: Publish artifact
if NOT DEFINED NUGET_REPO exit /B 0
:: Use timeout not dividable by 60 as work-around for https://nuget.codeplex.com/workitem/3917
NuGet.exe push *.nupkg -Source %NUGET_REPO% -Timeout 1201
IF %ERRORLEVEL% NEQ 0 exit /B 1
