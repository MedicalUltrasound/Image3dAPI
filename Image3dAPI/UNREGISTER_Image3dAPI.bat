@echo off
echo NOTICE: Script MUST be run as Administrator.
:: Errors from "reg" tool are muted to avoid flooding the build log with errors from already deleted registry entries.

:: Fix issue with "Run as Administrator" current dir
setlocal enableextensions
cd /d "%~dp0"


:: Remove all traces of Image3dAPI interfaces from registry

:: IImage3dTypeLibraryGenerator.idl
reg delete "HKCR\TypeLib\{1979020F-3402-49AD-A17C-507A1BBE4D09}" /f 2> NUL

for %%P in (32 64) do (
  :: IImage3d.idl
  reg delete "HKCR\Interface\{881DC121-1C8B-44AE-99E2-AAE4AD6A50E0}" /f /reg:%%P 2> NUL
  reg delete "HKCR\Interface\{381BA014-DA39-48B2-B0E7-7454D439469A}" /f /reg:%%P 2> NUL
)

::pause
