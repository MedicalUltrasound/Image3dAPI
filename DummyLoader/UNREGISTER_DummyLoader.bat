@echo off
echo NOTICE: Script MUST be run as Administrator.
:: Errors from "reg" tool are muted to avoid flooding the build log with errors from already deleted registry entries.

:: Fix issue with "Run as Administrator" current dir
setlocal enableextensions
cd /d "%~dp0"


:: Remove all traces of DummyLoader from registry
for %%R in (HKEY_LOCAL_MACHINE HKEY_CURRENT_USER) do (
  :: TypeLib
  reg delete "%%R\SOFTWARE\Classes\TypeLib\{67E59584-3F6A-4852-8051-103A4583CA5E}" /f 2> NUL

  for %%P in (32 64) do (
    :: Image3dStream class
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dStream"                    /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dStream.1"                  /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\CLSID\{78317A0E-56BF-4735-AB5B-FE0751219FE8}" /f /reg:%%P 2> NUL

    :: Image3dSource class
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dSource"                    /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dSource.1"                  /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\CLSID\{6FA82ED5-6332-4344-8417-DEA55E72098C}" /f /reg:%%P 2> NUL

    :: Image3dFileLoader class
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dFileLoader"                /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\DummyLoader.Image3dFileLoader.1"              /f 2> NUL
    reg delete "%%R\SOFTWARE\Classes\CLSID\{8E754A72-0067-462B-9267-E84AF84828F1}" /f /reg:%%P 2> NUL
  )
)

::pause
