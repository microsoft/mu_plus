# Memory Performance Profiler

This memory profiler is intended to provide metrics for a platform integrator.

## Under Test

```c
typedef
EFI_STATUS
(EFIAPI  *EFI_ALLOCATE_POOL) (
   IN EFI_MEMORY_TYPE            PoolType,
   IN UINTN                      Size,
   OUT VOID                      **Buffer
   );
```
[UEFI Specification Definition](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-allocatepool)

```c
typedef
EFI_STATUS
(EFIAPI *EFI_FREE_POOL) (
   IN VOID           *Buffer
   );
```
[UEFI Specification Definition](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-freepool)

```c
typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES) (
   IN EFI_ALLOCATE_TYPE                   Type,
   IN EFI_MEMORY_TYPE                     MemoryType,
   IN UINTN                               Pages,
   IN OUT EFI_PHYSICAL_ADDRESS            *Memory
   );

```
[UEFI Specification Definition](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-allocatepages)

```c
typedef
EFI_STATUS
(EFIAPI *EFI_FREE_PAGES) (
IN EFI_PHYSICAL_ADDRESS    Memory,
IN UINTN                   Pages
);
```
[UEFI Specification Definition](https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-freepages)