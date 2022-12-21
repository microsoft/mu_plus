@echo off
echo Creating container dfci_server
if exist Src\ssl\NUL rmdir /s /q Src\ssl
mkdir Src\ssl
copy ..\Certs\DFCI_HTTPS.key Src\ssl
copy ..\Certs\DFCI_HTTPS.pem Src\ssl

docker build . -t dfci_server
