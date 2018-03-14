@echo off 
cls
cd %~dp0

REM set /p seconds="time in seconds: "
REM set /p cubeDim="Cube Dimension (N x N x N): "
REM set /p threadCount="Draw thread count: "

set seconds=%1
set cubeDim=%2
set threadCount=%3
set output=%4
set confNum=%5
set initial=%6
set testNum=%7

set "exename=Vulkan"
set "drawArg=-cubeDim %cubeDim% -cubePad 1 -threadCount %threadCount%"

REM Pipeloins statistikler
%exename%.exe -csv -sec 3 -pipelineStatistics %drawArg%

REM OpenHardwareMonitor
start C:\PROGRA~2\OpenHardwareMonitor\OpenHardwareMonitor.exe 
timeout 10
%exename%.exe -csv -sec %seconds% -OHM -pi 1000 %drawArg%
taskkill /IM OpenHardwareMonitor.exe

REM Perfmonitor
logman delete %exename%PerfData
logman create counter %exename%PerfData -cf CollectWinPerfmonDataCfg.cfg -f csv -o "%cd%\perfdata" -si 1
logman start %exename%PerfData -as
%exename%.exe -sec %seconds% -frameTime %drawArg%
logman stop %exename%PerfData 

REM Move files to data directory
FOR /F "delims=_. tokens=2" %%i IN ('dir /B data_*.csv') DO call :mover %%i %output% %confNum% %initial% %testNum%
EXIT

:mover
 set dir=data\%2\%1
 set cnum=%3
 set init=%4
 set tnum=%5
 mkdir %dir%
 move perfdata*.csv %dir%\perfmon_%1.csv 
 REM move data_*.csv %dir%\ohmon_%1.csv
 move data_*.csv %dir%\vk%init%-%tnum%-%cnum%.csv
 move stat_*.csv %dir%\pipeline_%1.csv
 move conf_*.csv %dir%\
 move frameTime_*.csv %dir%\
 GOTO :EOF
