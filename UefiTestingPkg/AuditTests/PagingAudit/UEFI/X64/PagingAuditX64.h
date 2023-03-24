#include <Register/Cpuid.h>
#include <Register/Amd/Cpuid.h>

#define X64_PAGE_TABLE_PRESENT         0x1ULL
#define X64_PAGE_TABLE_RW              0x2ULL
#define X64_PAGE_TABLE_NX              0x8000000000000000ULL
#define X64_PAGE_TABLE_PAGE_SIZE_FLAG  0x80ULL

#define X64_IS_READ_WRITE(page)  ((page & X64_PAGE_TABLE_RW) != 0)
#define X64_IS_EXECUTABLE(page)  ((page & X64_PAGE_TABLE_NX) == 0)
#define X64_IS_PRESENT(page)     ((page & X64_PAGE_TABLE_PRESENT) != 0)
#define X64_IS_LEAF(page)        ((page & X64_PAGE_TABLE_PAGE_SIZE_FLAG) != 0)

#define X64_PAGE_TABLE_ADDRESS_MASK  0x000FFFFFFFFFF000ULL
