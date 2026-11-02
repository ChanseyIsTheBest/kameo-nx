@echo off
rem kameo-nx: tools\build_pc.bat with its output in a log and the exit line last, for builds started in the background.
rem Usage: tools\start_build_pc.bat <log name in out\> [RENUT ON/OFF] [extra build args]
set "RENUT=%2"
set "LOGF=%~dp0..\out\%1"
call "%~dp0build_pc.bat" %3 %4 %5 > "%LOGF%" 2>&1
echo BUILD_EXIT %ERRORLEVEL% >> "%LOGF%"
