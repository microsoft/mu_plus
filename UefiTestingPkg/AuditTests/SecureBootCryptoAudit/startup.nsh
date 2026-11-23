@echo -off
for %b run (0 9)
  if exist FS%b:\SecureBootCryptoAuditApp.efi then
    FS%b:
    cd \
    echo Starting the Secure Boot ECIT audit...
    SecureBootCryptoAuditApp.efi
    echo Secure Boot ECIT audit returned.
    goto Done
  endif
endfor

echo ERROR: SecureBootCryptoAuditApp.efi was not found on FS0 through FS9.

:Done
