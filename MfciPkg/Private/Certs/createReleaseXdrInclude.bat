REM Create the PCD DSC include for the test certificate trust anchor
python ..\..\..\..\..\MU_BASECORE\BaseTools\Scripts\BinToPcd.py -i .\SharedMfciTrustAnchor.cer -o .\SharedMfciTrustAnchor.dsc.inc -p gMfciPkgTokenSpaceGuid.PcdMfciPkcs7CertBufferXdr -x -v
