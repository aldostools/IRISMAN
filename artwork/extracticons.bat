@echo off
if not "%1"=="" goto extracticon

echo Extracting icons...
echo:
for %%i in (*.iso) do call extracticons.bat %%i
echo done.
pause>nul
goto end

:extracticon
if exist ICON0.PNG del ICON0.PNG
echo [*] %1
set pngname=%1
set pngname=%pngname:~0,-4%.png
if exist %pngname% goto end
"%ProgramFiles%\7-zip\7z.exe" e "%1" PS3_GAME\ICON0.PNG >nul
if exist ICON0.PNG goto renameicon
"%ProgramFiles%\7-zip\7z.exe" e "%1" PSP_GAME\ICON0.PNG >nul
:renameicon
ren ICON0.PNG "%pngname%"
:end
