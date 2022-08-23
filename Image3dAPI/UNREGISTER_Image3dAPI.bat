@echo off
echo NOTICE: Script MUST be run as Administrator.
:: Errors from "reg" tool are muted to avoid flooding the build log with errors from already deleted registry entries.

:: Fix issue with "Run as Administrator" current dir
setlocal enableextensions
cd /d "%~dp0"


:: Remove all traces of Image3dAPI interfaces from registry

:: IImage3dTypeLibraryGenerator.idl
reg delete "HKCR\TypeLib\{3ff1aab8-f3d8-33d4-825d-00104b3646c0}" /f 2> NUL

for %%P in (32 64) do (
  :: IImage3d.idl
  reg delete "HKCR\Interface\{D7D2A41A-F738-41EB-8133-1B06DDA1FF40}" /f /reg:%%P 2> NUL
  reg delete "HKCR\Interface\{D483D815-52DD-4750-8CA2-5C6C489588B6}" /f /reg:%%P 2> NUL
  reg delete "HKCR\Interface\{CD30759B-EB38-4469-9CA5-4DF75737A31B}" /f /reg:%%P 2> NUL
)

::pause
