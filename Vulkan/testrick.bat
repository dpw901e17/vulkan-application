@echo off
cd %~dp0
:intro
CLS
DEL /Q *.csv

ECHO Welcome to the testrick of the Vulkan Application:

SET /p "cubeStartDim=How many cubes to start with (NxNxN)? "
SET /p "cubeInc=How many cubes shall each test increment with? "
SET /p "seconds=How many seconds shall the program run for? "
SET /p "threadCount=How many threads will be used? "
SET /p "outputFolder=What folder shall the collected data be put in? "
SET /p "initial=What is your initial [b/c/m]? "
SET /p "testNum=What number of test run is this (start at 1)? "
SET /p "testCount=How many times are we running this test? "

ECHO Now, next time you press ENTER, the tests will begin.
PAUSE

CALL :repeat %testCount% for_body
ECHO Test complete!
PAUSE

GOTO :EOF
:for_body
REM %1 = index
REM %2 = cubeStartDim
REM %3 = cubeInc
REM %4 = seconds
REM %5 = threadCount
REM %6 = outputFolder

SET /A cubeDim=%1*%3+%2%

START "Test #%1" /WAIT AutoRecord.bat %4 %cubeDim% %5 %6 %1 %7 %8
echo test #%1 done
REM subroutine for old-fashioned for-loop

GOTO :EOF
:repeat
SET count=%1
SET func=%2
SET i=0

:for_loop
IF %i% LSS %count% (
	SET /A i=%i%+1
	CALL :%func% %i% %cubeStartDim% %cubeInc% %seconds% %threadCount% %outputFolder% %initial% %testNum%
	GOTO :for_loop
) ELSE (
	GOTO :EOF
)