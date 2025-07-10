@echo off
REM install.bat - Installs hexdump.exe from the latest GitHub release

REM 1. Set variables
setlocal
set REPO=OWNER/REPO_NAME
set EXE=hexdump.exe
set INSTALLDIR=%ProgramFiles%\hexdump
set TMPDIR=%TEMP%\hexdump_download

REM 2. Create temp and install directories
if not exist "%TMPDIR%" mkdir "%TMPDIR%"
if not exist "%INSTALLDIR%" mkdir "%INSTALLDIR%"

REM 3. Download latest release info (requires PowerShell 5+)
powershell -Command "\
  $repo='%REPO%'; \
  $api='https://api.github.com/repos/'+$repo+'/releases/latest'; \
  $json=Invoke-RestMethod -Uri $api; \
  $asset=$json.assets | Where-Object { $_.name -eq '%EXE%' }; \
  if ($asset -eq $null) { Write-Error 'hexdump.exe not found in latest release.'; exit 1 }; \
  $url=$asset.browser_download_url; \
  Invoke-WebRequest -Uri $url -OutFile '%TMPDIR%\\%EXE%';"
if not exist "%TMPDIR%\%EXE%" (
    echo Download failed.
    exit /b 1
)

REM 4. Move exe to install dir
move /Y "%TMPDIR%\%EXE%" "%INSTALLDIR%\%EXE%"

REM 5. Add to system PATH (requires admin)
setx /M PATH "%PATH%;%INSTALLDIR%"

REM 6. Cleanup
del /Q "%TMPDIR%\%EXE%"
rmdir "%TMPDIR%"

REM 7. Done
echo Installed hexdump.exe to %INSTALLDIR% and added to PATH.
endlocal
pause
