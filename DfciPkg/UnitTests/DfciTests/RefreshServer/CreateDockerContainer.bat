@echo off

if not exist ..\DfciTests.ini goto error2

echo Creating container dfci_server
if exist Src\ssl\NUL rmdir /s /q Src\ssl
if exist Src\Responses rmdir /s /q Src\Responses
if exist Src\Requests rmdir /s /q Src\Requests

mkdir Src\ssl
if %ERRORLEVEL% NEQ 0 goto :error

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

docker build . -t dfci_server
if %ERRORLEVEL% NEQ 0 goto :error

echo Container dfci_server created.
echo .
echo To stop the currently running container, press Ctrl-C in this window, or issue:
echo docker stop dfci_server
echo from another command prompt window
echo .
echo To start this container again in the future, issue:
echo docker start -i dfci_server

docker run -p 80:80 -p 443:443 --name dfci_server -it dfci_server
if %ERRORLEVEL% NEQ 0 goto :error

goto Done
:error
echo Error occured building docker container
goto Done

:error2
echo You must create DfciTests.ini before creating your container
:Done
