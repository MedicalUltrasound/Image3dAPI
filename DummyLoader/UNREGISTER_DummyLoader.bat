@echo off
echo NOTICE: Script MUST be run as Administrator.
:: Errors from "reg" tool are muted to avoid flooding the build log with errors from already deleted registry entries.

:: Fix issue with "Run as Administrator" current dir
setlocal enableextensions
cd /d "%~dp0"


:: Remove all traces of DummyLoader from registry
for %%R in (HKEY_LOCAL_MACHINE HKEY_CURRENT_USER) do (
  :: TypeLib
  reg delete "%%R\SOFTWARE\Classes\TypeLib\{ED4540BD-07B5-44B0-BCDE-3E2C1D99183B}" /f 2> NUL

  for %%P in (32 64) do (
    :: Image3dSource class
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dSource"                    /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dSource.1"                  /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\CLSID\{50BE330D-F729-4D8F-A1E4-C939E0598EDF}" /f /reg:%%P 2> NUL

    :: Image3dFileLoader class
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dFileLoader"                /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dFileLoader.1"              /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\CLSID\{1326A2C6-7753-4584-B866-CDF3C6E240F1}" /f /reg:%%P 2> NUL
  )
)

::pause
