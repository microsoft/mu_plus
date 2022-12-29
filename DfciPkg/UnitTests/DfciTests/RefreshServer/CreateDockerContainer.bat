@echo off
setlocal
if not exist ..\DfciTests.ini goto error2

docker ps -a -f name=dfci_server | findstr dfci_server
if %ERRORLEVEL% NEQ 0 goto :no_container

echo The Dfci_Server container exists.

:Prompt
set /P AreYouSure=Are you sure you want to recreate the dfci_server container (Y/[N])?
if /I "%AreYouSure%" EQU "N" goto Done
if /I "%AreYouSure%" EQU "Y" goto erase_server
echo Invalid response %AreYouSure%
goto Prompt
:erase_server
docker rm -f dfci_server
if %ERRORLEVEL% NEQ 0 goto :error

docker rmi -f dfci_server
if %ERRORLEVEL% NEQ 0 goto :error

:no_container
echo Creating web server data
if exist Src\ssl\NUL rmdir /s /q Src\ssl
if exist Src\Responses rmdir /s /q Src\Responses
if exist Src\Requests rmdir /s /q Src\Requests

mkdir Src\ssl

mkdir Src\Requests
if %ERRORLEVEL% NEQ 0 goto :error

mkdir Src\Responses
if %ERRORLEVEL% NEQ 0 goto :error

cd BuildResponses
if %ERRORLEVEL% NEQ 0 goto :error

call GenResponses.bat
if %ERRORLEVEL% NEQ 0 goto :error

cd ..
if %ERRORLEVEL% NEQ 0 goto :error

copy ..\Certs\DFCI_HTTPS.key Src\ssl
if %ERRORLEVEL% NEQ 0 goto :error

copy ..\Certs\DFCI_HTTPS.pem Src\ssl
if %ERRORLEVEL% NEQ 0 goto :error

copy ..\DfciTests.ini Src
if %ERRORLEVEL% NEQ 0 goto :error

echo Creating dfci_server image.
docker build . -t dfci_server
if %ERRORLEVEL% NEQ 0 goto :error

echo Creating dfci_server container.
docker create -p 80:80 -p 443:443 --name dfci_server -it dfci_server
if %ERRORLEVEL% NEQ 0 goto :error

echo Container dfci_server created.
echo .
echo To start this container in the background, issue:
echo     docker start dfci_server
echo .
echo or, to start this container to see error messages:
echo     docker start -i dfci_server
echo .
echo .
echo Choose an option to start the dfci_server:
echo 1. Start in background
echo 2. Start in command window
echo 3. Do not start dfci_server
:Prompt
set /P ChooseStartOption=Select 1, 2, or 3
if /I "%AreYouSure%" EQU "1" goto start_background
if /I "%AreYouSure%" EQU "2" goto start_foreground
if /I "%AreYouSure%" EQU "3" goto Done
echo Invalid response %AreYouSure%
goto Prompt

:start_background
docker start dfci_server
goto Done

:start_foreground
docker start -i dfci_server
goto Done

:error
echo Error occured building docker container
goto Done

:error2
echo You must create DfciTests.ini before creating your container
:Done
endlocal
