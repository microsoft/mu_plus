# DXE Is Capsule Supported Lib

This library performs a sanity check to determine if a capsule is supported by the UEFI firmware.
It *does not* perform any sort of signature or hash.
In the case of nested capsules, there is stubbed out functionality to check if the ESRT table contains an entry
related to the nested capsule.

## Why DXE_DRIVER Only

This is called from the DXE_RUNTIME_DRIVER indirectly via the `CapsuleServiceProtocol`.
Because this is a DXE protocol and the Capsule Runtime service doesn't call the protocol after ExitBootServices,
this is safe.
