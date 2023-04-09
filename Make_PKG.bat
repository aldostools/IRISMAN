@echo off

set PS3SDK=C:/PSDK3v2
set PSL1GHT=%PS3SDK%/psl1ght
set PS3DEV=%PS3SDK%/ps3dev
set WIN_PS3SDK=C:/PSDK3v2

set PATH="%WIN_PS3SDK%/mingw/msys/1.0/bin;%WIN_PS3SDK%/mingw/bin;%WIN_PS3SDK%/ps3dev/bin;%WIN_PS3SDK%/ps3dev/ppu/bin;%WIN_PS3SDK%/ps3dev/spu/bin;%WIN_PS3SDK%/mingw/Python27;%PATH%"

cd %PS3SDK%
make pkg

if %errorlevel% neq 0 (
    echo "Error: make pkg failed with exit code %errorlevel%. Exiting."
    exit /b %errorlevel%
)

pause
