#include <Library/ArmLib.h>
#include <Chipset/AArch64Mmu.h>

#define AARCH64_IS_TABLE(page, level)  ((level == 3) ? FALSE : (((page) & TT_TYPE_MASK) == TT_TYPE_TABLE_ENTRY))
#define AARCH64_IS_BLOCK(page, level)  ((level == 3) ? (((page) & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY_LEVEL3) : ((page & TT_TYPE_MASK) == TT_TYPE_BLOCK_ENTRY))
#define AARCH64_ROOT_TABLE_LEN(T0SZ)   (TT_ENTRY_COUNT >> ((T0SZ) - 16) % 9)
#define AARCH64_IS_VALID  0x1
#define AARCH64_IS_READ_WRITE(page)  (((page & TT_AP_RW_RW) != 0) || ((page & TT_AP_MASK) == 0))
#define AARCH64_IS_EXECUTABLE(page)  ((page & TT_UXN_MASK) == 0 || (page & TT_PXN_MASK) == 0)
#define AARCH64_IS_ACCESSIBLE(page)  ((page & TT_AF) != 0)

#define AARCH64_ADDRESS_MASK  0x000FFFFFFFFFF000ULL
