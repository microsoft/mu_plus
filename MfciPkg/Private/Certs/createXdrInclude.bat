REM Create the PCD DSC include for the test certificate trust anchor
python ..\..\..\..\..\MU_BASECORE\BaseTools\Scripts\BinToPcd.py -i ..\..\UnitTests\Data\Certs\CA.cer -o .\CA-test.dsc.inc -p gMfciPkgTokenSpaceGuid.PcdMfciPkcs7CertBufferXdr -x -v
