#include "directio.h"
#include "directiorc.h"

#if defined(BUILDING_DIRECTIOLIB)
#define DIOAPI  __declspec(dllexport)
#else
#define DIOAPI  __declspec(dllimport)
#endif

#define DIO_ARCH_NONE        0
#define DIO_ARCH_X86         1
#define DIO_ARCH_X64         2

DIOAPI BOOL WINAPI dio_read_port(u16 port, u16 size, u32 *value);
DIOAPI BOOL WINAPI dio_write_port(u16 port, u16 size, u32 value);
DIOAPI BOOL WINAPI dio_read_mem(u64 phy_addr, u16 size, u32 *value);
DIOAPI BOOL WINAPI dio_write_mem(u64 phy_addr, u16 size, u32 value);
DIOAPI PBYTE WINAPI dio_map_memory(u64 paddr, u64 size);
DIOAPI VOID WINAPI dio_unmap_memory(u64 paddr, u64 size);
DIOAPI BOOL WINAPI dio_init();
DIOAPI void WINAPI dio_exit();


