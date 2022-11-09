/*
 *	The PCI Library -- Access to i386 I/O ports on Linux
 *
 *	Copyright (c) 1997--2006 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <sys/io.h>

static int
intel_setup_io(struct pci_access *a UNUSED)
{
  return (iopl(3) < 0) ? 0 : 1;
}

static inline int
intel_cleanup_io(struct pci_access *a UNUSED)
{
  iopl(3);
  return -1;
}


#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static inline u8*
map_mem(u64 paddr, u64 size)
{
  int mem;
  void *addr;

  mem = open("/dev/mem", O_RDWR | O_SYNC);
  if (mem < 0) {
    return NULL;
  }

  addr = mmap((void*)0, size, PROT_READ | PROT_WRITE, MAP_SHARED, 
                   mem, paddr);
  if(addr == (void *)-1) {
    addr = NULL;
  }
  
  close(mem);
  
  return addr;
}

static inline void
unmap_mem(u64 vaddr, u64 size)
{
  munmap((void*)vaddr, size);
}