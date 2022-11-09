

typedef UCHAR               u8;
typedef USHORT              u16;
typedef ULONG               u32;
typedef ULONGLONG           u64;
typedef CHAR                s8;
typedef SHORT               s16;
typedef LONG                s32;
typedef LONGLONG            s64;
typedef BOOLEAN             bool;
#define true  TRUE
#define false FALSE

#define DIO_DEVNAME   L"\\Device\\DirectIO"
#define DIO_LNKNAME   L"\\DosDevices\\DirectIO"

/* needed when compiling on mingw */
#ifndef CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif
#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED                 0
#endif
#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS                 0
#endif

#define DIO_DEVTYPE   0x8D10
#define DIO_FUNCTION  0xD10

#define IOCTL_DIO_INIT      CTL_CODE(DIO_DEVTYPE, DIO_FUNCTION + 0, \
							METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DIO_EXIT      CTL_CODE(DIO_DEVTYPE, DIO_FUNCTION + 1, \
							METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_DIO_READ      CTL_CODE(DIO_DEVTYPE, DIO_FUNCTION + 2, \
							METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_DIO_WRITE     CTL_CODE(DIO_DEVTYPE, DIO_FUNCTION + 3, \
							METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DIO_MAPMEM    CTL_CODE(DIO_DEVTYPE, DIO_FUNCTION + 4, \
							METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DIO_UNMAPMEM  CTL_CODE(DIO_DEVTYPE, DIO_FUNCTION + 5, \
							METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DIO_PORT_INFO {
	u16 port;
	u32 size;
	u32 value;
} DIO_PORT_INFO, *PDIO_PORT_INFO;

typedef struct _DIO_MMAP_INFO
{
	u64 addr; /*input:physical, output:virtual*/
	u64 size;
} DIO_MMAP_INFO, *PDIO_MMAP_INFO;
