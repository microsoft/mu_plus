# CapsuleRuntimeDxe

Supports the UpdateCapsule and QueryCapsuleCapabilities runtime services.
This driver does not apply capsules.

This driver uses a DXE protocol (gCapsuleServiceProtocolGuid) to provide these services.
Once ExitBootServices has been signaled, the driver will always return EFI_UNSUPPORTED for both calls.

## Why A Protocol

This is done for binary size reduction and library simplification.

Because this isn't a "true" runtime driver (it effectively becomes a NOP at runtime), libraries that would
be leveraged by this runtime service would need to support runtime.
This choice poses a challenge because this driver isn't a "true" runtime driver so many of the libraries
need to support runtime dxe in name only. This nuance can cause problems if an attempt is made to
use these libraries in a runtime driver.

## Protocol Producer

Currently CapsuleServiceProtocolDxe is the primary producer of the gCapsuleServiceProtocolGuid
protocol - there is additional documentation there, including a flow chart of the capsule update process.
