/*
 *	The PCI Library -- Access to i386 I/O ports on Windows
 *
 *	Copyright (c) 2004 Alexander Stock <stock.alexander@gmx.de>
 *	Copyright (c) 2006 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <io.h>
#include <windows.h>

typedef BOOL (WINAPI *SETDLLDIRECTORY)(LPCSTR);

typedef BOOL (WINAPI *DIO_INIT)(void);
typedef VOID (WINAPI *DIO_EXIT)(void);
typedef BOOL (WINAPI *DIO_WRITE_PORT)(u16,u16,u32);
typedef BOOL (WINAPI *DIO_READ_PORT)(u16,u16,u32*);
typedef BOOL (WINAPI *DIO_WRITE_MEM)(u64,u16,u32);
typedef BOOL (WINAPI *DIO_READ_MEM)(u64,u16,u32*);
typedef PBYTE (WINAPI *DIO_MAP_MEMORY)(u64,u64);
typedef VOID (WINAPI *DIO_UNMAP_MEMORY)(u64,u64);

#ifdef WIN64
#define DIO_LIB_NAME "directio64.dll"
#else
#define DIO_LIB_NAME "directio32.dll"
#endif


DIO_INIT dio_init;
DIO_EXIT dio_exit;
DIO_READ_PORT dio_read_port;
DIO_WRITE_PORT dio_write_port;
DIO_WRITE_MEM dio_read_mem;
DIO_READ_MEM dio_write_mem;
DIO_MAP_MEMORY dio_map_memory;
DIO_UNMAP_MEMORY dio_unmap_memory;

#define GET_DIO_PROC(n,d)   \
    n = (typeof(n))GetProcAddress(lib, #n); \
    if (!n) { \
        a->warning("i386-io-windows: Couldn't find " #n " function.\n"); \
        return 0; \
    }

static int
intel_setup_io(struct pci_access *a)
{
  HMODULE lib;

  SETDLLDIRECTORY fnSetDllDirectory;
  
  /* remove current directory from DLL search path */
  fnSetDllDirectory = GetProcAddress(GetModuleHandle("kernel32"), "SetDllDirectoryA");
  if (fnSetDllDirectory)
    fnSetDllDirectory("");

  lib = LoadLibrary(DIO_LIB_NAME);
  if (!lib)
    {
      a->warning("i386-io-windows: Couldn't load %s.", DIO_LIB_NAME);
      return 0;
    }
  
  GET_DIO_PROC(dio_init, DIO_INIT);
  GET_DIO_PROC(dio_exit, DIO_EXIT);
  GET_DIO_PROC(dio_read_port, DIO_READ_PORT);
  GET_DIO_PROC(dio_write_port, DIO_WRITE_PORT);
  GET_DIO_PROC(dio_read_mem, DIO_READ_MEM);
  GET_DIO_PROC(dio_write_mem, DIO_WRITE_MEM);
  GET_DIO_PROC(dio_map_memory, DIO_MAP_MEMORY);
  GET_DIO_PROC(dio_unmap_memory, DIO_UNMAP_MEMORY);


  if (!dio_init())
    {
      a->warning("i386-io-windows: DirectIO_Init() failed.");
      return 0;
    }

  return 1;
}

static inline int
intel_cleanup_io(struct pci_access *a UNUSED)
{
  dio_exit();
  return 1;
}

static inline u8
inb(u16 port)
{
  DWORD pv;

  if (dio_read_port(port, 1, &pv))
    return (u8)pv;
  return 0;
}

static inline u16
inw(u16 port)
{
  DWORD pv;

  if (dio_read_port(port, 2, &pv))
    return (u16)pv;
  return 0;
}

static inline u32
inl(u16 port)
{
  DWORD pv;

  if (dio_read_port(port, 4, &pv))
    return (u32)pv;
  return 0;
}

static inline void
outb(u8 value, u16 port)
{
  dio_write_port(port, 1, value);
}

static inline void
outw(u16 value, u16 port)
{
  dio_write_port(port, 2, value);
}

static inline void
outl(u32 value, u16 port)
{
  dio_write_port(port, 4, value);
}

static inline u8*
map_mem(u64 paddr, u64 size)
{
  return dio_map_memory(paddr, size);
}

static inline void
unmap_mem(u64 paddr, u64 size)
{
  dio_unmap_memory(paddr, size);
}


