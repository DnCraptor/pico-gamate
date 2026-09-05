@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem %~dp0 has a trailing backslash. Keep ROOT for file paths, but use SRC
rem without the trailing backslash when passing it as a quoted CMake argument.
set "ROOT=%~dp0"
set "SRC=%~dp0"
if "%SRC:~-1%"=="\" set "SRC=%SRC:~0,-1%"
cd /d "%ROOT%"

rem Build every supported RP2350 / Cortex-M33 pico-gamate configuration.
rem PICO_PLATFORM is intentionally NOT passed here: CMakeLists.txt fixes it to rp2350.
rem Excluded: RP2040, RISC-V, m1p2launcher, TFT/ILI9341, legacy TV.
rem Included video: VGA, HDMI, SOFTTV.
rem Included audio: PWM, I2S, I2S-CS4334, HWAY/AY-3-8910.

set "BUILD_TYPE=Release"
set "BUILD_ROOT=%ROOT%build\all"

rem Locate CMake.
set "CMAKE="
for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE set "CMAKE=%%I"
if not defined CMAKE if exist "%USERPROFILE%\.pico-sdk" (
    for /f "delims=" %%I in ('where /r "%USERPROFILE%\.pico-sdk" cmake.exe 2^>nul') do if not defined CMAKE set "CMAKE=%%I"
)
if not defined CMAKE (
    echo ERROR: cmake.exe not found. 1>&2
    exit /b 1
)

rem Locate Ninja.
set "NINJA="
for /f "delims=" %%I in ('where ninja.exe 2^>nul') do if not defined NINJA set "NINJA=%%I"
if not defined NINJA if exist "%USERPROFILE%\.pico-sdk" (
    for /f "delims=" %%I in ('where /r "%USERPROFILE%\.pico-sdk" ninja.exe 2^>nul') do if not defined NINJA set "NINJA=%%I"
)
if not defined NINJA (
    echo ERROR: ninja.exe not found in PATH or "%USERPROFILE%\.pico-sdk". 1>&2
    exit /b 1
)

echo CMake: %CMAKE%
echo Ninja: %NINJA%
echo Source: %SRC%
echo Platform: rp2350 ^(fixed by CMakeLists.txt^)

set /a COUNT=0
set "TOTAL=48"

for %%B in (murmulator murmulator2 olimex-pico-pc waveshare_rp2350_pizero) do (
    for %%V in (VGA HDMI SOFTTV) do (
        for %%A in (PWM I2S I2S-CS4334 HWAY) do (
            call :build_one %%B %%V %%A
            if errorlevel 1 exit /b !errorlevel!
        )
    )
)

echo.
echo All %TOTAL% supported RP2350/Cortex-M33 variants built.
echo UF2 files are under bin\%BUILD_TYPE%\.
exit /b 0

:build_one
set /a COUNT+=1
set "B=%~1"
set "V=%~2"
set "A=%~3"
set "TAG=!B!-!V!-!A!"
set "BDIR=%BUILD_ROOT%\!TAG!"

set "VGA=OFF"
set "HDMI=OFF"
set "SOFTTV=OFF"
if /I "!V!"=="VGA" set "VGA=ON"
if /I "!V!"=="HDMI" set "HDMI=ON"
if /I "!V!"=="SOFTTV" set "SOFTTV=ON"

set "I2S=OFF"
set "I2S_CS4334=OFF"
set "HWAY=OFF"
if /I "!A!"=="I2S" set "I2S=ON"
if /I "!A!"=="I2S-CS4334" (
    set "I2S=ON"
    set "I2S_CS4334=ON"
)
if /I "!A!"=="HWAY" set "HWAY=ON"

echo.
echo [!COUNT!/%TOTAL%] !TAG!

"%CMAKE%" -S "%SRC%" -B "!BDIR!" -G Ninja ^
  "-DCMAKE_MAKE_PROGRAM:FILEPATH=%NINJA%" ^
  "-DCMAKE_BUILD_TYPE=%BUILD_TYPE%" ^
  "-DPICO_BOARD=!B!" ^
  "-Dm1p2launcher=OFF" ^
  "-DTFT=OFF" ^
  "-DILI9341=OFF" ^
  "-DTV=OFF" ^
  "-DVGA=!VGA!" ^
  "-DHDMI=!HDMI!" ^
  "-DSOFTTV=!SOFTTV!" ^
  "-DI2S=!I2S!" ^
  "-DI2S_CS4334=!I2S_CS4334!" ^
  "-DHWAY=!HWAY!"
if errorlevel 1 (
    echo ERROR: configure failed for !TAG!. 1>&2
    exit /b 1
)

"%CMAKE%" --build "!BDIR!" --config %BUILD_TYPE% --target all
if errorlevel 1 (
    echo ERROR: build failed for !TAG!. 1>&2
    exit /b 1
)

exit /b 0
