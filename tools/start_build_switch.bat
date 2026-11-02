@echo off
rem kameo-nx: tools\build_switch.bat with its output in a log and the exit line last, for builds started in the background.
rem Usage: tools\start_build_switch.bat <log name in out\> [extra build args]
set "LOGF=%~dp0..\out\%1"
call "%~dp0build_switch.bat" %2 %3 %4 > "%LOGF%" 2>&1
echo BUILD_EXIT %ERRORLEVEL% >> "%LOGF%"
