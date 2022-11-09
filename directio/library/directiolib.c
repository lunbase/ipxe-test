/*
 * Copyright (C) 2011 Jernej Simončič
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#define UNICODE
#define NOGDI
#include <windows.h>
#include <wchar.h>
#define BUILDING_DIRECTIOLIB
#include "directiolib.h"

typedef void (WINAPI *GETNATIVESYSTEMINFO)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *ISWOW64PROCESS)(HANDLE,PBOOL);

HINSTANCE hDirectIoLib; /* assigned hInstance of the DLL in DllMain */

/* GetModuleFileName can return \\?\ paths, so MAX_PATH might be too short */
#define DIRECTIO_MAX_PATH  32767

wchar_t DriverPath[DIRECTIO_MAX_PATH];
HANDLE dio_handle = INVALID_HANDLE_VALUE;
ULONG dio_arch = DIO_ARCH_NONE;

#define DIRECTIO_SERVICE L"DirectIO"
#define DIRECTIO_FILE    L"\\\\.\\DirectIO"
#define DIRECTIO_DRIVER  L"DirectIO.sys"

int dio_find_arch()
{
#ifdef _WIN64
	return DIO_ARCH_X64;
#else
	GETNATIVESYSTEMINFO fnGetNativeSystemInfo;
	ISWOW64PROCESS fnIsWow64Process;
	SYSTEM_INFO SysInfo;
	BOOL bIsWow64 = FALSE;

	int result = DIO_ARCH_NONE;

	fnIsWow64Process = (ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

	if (NULL == fnIsWow64Process) {
		/* function doesn't exist - we're on x86 */
		return DIO_ARCH_X86;
	}
	
	if (fnIsWow64Process(GetCurrentProcess(),&bIsWow64)) {
		if (!bIsWow64) {
			/* we're not on Wow64, so this is x86 */
			result = DIO_ARCH_X86;
		} else {
			/* WOW64 - check if this is AMD64 */
			fnGetNativeSystemInfo = 
				(GETNATIVESYSTEMINFO) GetProcAddress(
				GetModuleHandle(TEXT("kernel32")),"GetNativeSystemInfo");

			if (NULL != fnGetNativeSystemInfo) {
				fnGetNativeSystemInfo(&SysInfo);
				if (SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
					result = DIO_ARCH_X64;
				}
			}
		}
	}

	return result;
#endif
}

BOOL dio_prepare()
{
	HRSRC hResource;
	HGLOBAL hDriver;
	LPVOID lpDriver;
	DWORD dwSize;
	HANDLE hDriverFile;
	HANDLE hDriverMap;
	LPVOID lpMapView;

	if (!GetTempPath( DIRECTIO_MAX_PATH, DriverPath )) {
		return 0;
	}

	wcsncat(DriverPath, DIRECTIO_DRIVER,
		DIRECTIO_MAX_PATH-wcslen(DIRECTIO_DRIVER)-1);

	/* find the driver resource for the correct platform */
	dio_arch = dio_find_arch();
	switch (dio_arch) {
	case DIO_ARCH_X86:
		hResource = FindResource(hDirectIoLib, 
			MAKEINTRESOURCE(RC_DRV_X86), RT_RCDATA);
		break;

	case DIO_ARCH_X64:
		hResource = FindResource(hDirectIoLib,
			MAKEINTRESOURCE(RC_DRV_AMD64), RT_RCDATA);
		break;

	default:
		return 0;

	}
	if (NULL == hResource)
		return 0;

	/* load the resource */
	hDriver = LoadResource(hDirectIoLib, hResource);
	if (NULL == hDriver)
		return 0;

	/* get the pointer to the resource and it's size */
	lpDriver = LockResource(hDriver);
	if (NULL == lpDriver)
		return 0;
	dwSize = SizeofResource(hDirectIoLib, hResource);
	if (0 == dwSize)
		return 0;

	/* open the file the driver will be saved to and map it */
	hDriverFile = CreateFile( DriverPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (INVALID_HANDLE_VALUE == hDriverFile)
		return 0;
	hDriverMap = CreateFileMapping( hDriverFile, NULL, PAGE_READWRITE, 0, dwSize, NULL );
	if (NULL == hDriverMap)
		return 0;
	lpMapView = MapViewOfFile(hDriverMap, FILE_MAP_WRITE, 0, 0, 0);
	if (NULL == lpMapView)
		return 0;

	/* write the driver */
	CopyMemory(lpMapView, lpDriver, dwSize);

	UnmapViewOfFile(lpMapView);
	CloseHandle(hDriverMap);
	CloseHandle(hDriverFile);

	return 1;
}

void _stdcall dio_uninstall()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	SERVICE_STATUS stat;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
		return;

	hService = OpenService(hSCManager, DIRECTIO_SERVICE, SERVICE_ALL_ACCESS);
	if (hService == NULL) {
		CloseServiceHandle(hSCManager);
		return;
	}

	ControlService(hService, SERVICE_CONTROL_STOP, &stat);

	DeleteService(hService);

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	DeleteFile(DriverPath);

	return;
}

BOOL dio_install()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;	
	BOOL result;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
		return 0;

	hService = CreateService( hSCManager, DIRECTIO_SERVICE, L"DirectIO driver",
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START,
		SERVICE_ERROR_NORMAL, DriverPath, NULL, NULL, NULL, NULL, NULL );

	CloseServiceHandle( hSCManager );

	if (hService == NULL)
		return 0;

	if (!StartService(hService, 0, NULL)) {
		if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
			result = 1; /* service is running, so we're ok */
		} else {
			result = 0;
		}
	} else {
		result = 1;
	}
		
	CloseServiceHandle( hService );

	return result;
}

BOOL _stdcall dio_read_port(
	USHORT port,
	USHORT size,
	PULONG value)
{
	DIO_PORT_INFO dio_port;
	DWORD junk;

	if (dio_handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	
	dio_port.port = port;
	dio_port.size = size;

	return DeviceIoControl(dio_handle, IOCTL_DIO_READ, &dio_port,
		sizeof(dio_port), value, sizeof(value), &junk, NULL);

}

BOOL _stdcall dio_write_port(
	USHORT port,
	USHORT size,
	ULONG value)
{
	DIO_PORT_INFO dio_port;
	DWORD junk;

	if (dio_handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	
	dio_port.port = port;
	dio_port.size = size;
	dio_port.value = value;

	return DeviceIoControl(dio_handle, IOCTL_DIO_WRITE, &dio_port,
		sizeof(dio_port), NULL, 0, &junk, NULL);
}

PBYTE _stdcall dio_map_memory(
	u64 paddr,
	u64 size)
{
	DIO_MMAP_INFO mmap_info;
	DWORD dwBytesReturned;
	PBYTE vaddr;

	if (dio_handle == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	memset(&mmap_info, 0, sizeof(mmap_info));
	mmap_info.addr = paddr;
	mmap_info.size = size;

	if (!DeviceIoControl(dio_handle, IOCTL_DIO_MAPMEM, &mmap_info,
		sizeof(DIO_MMAP_INFO), &mmap_info, sizeof(mmap_info),
		&dwBytesReturned, NULL)) {
		return NULL;
	}

#ifdef _WIN64
	vaddr = (PBYTE)mmap_info.addr;
#else /*_WIN64*/
	vaddr = (PBYTE)(u32)mmap_info.addr;
#endif /*_WIN64*/

	return vaddr;
}


VOID _stdcall dio_unmap_memory(
	u64 paddr,
	u64 size)
{
	DIO_MMAP_INFO mmap_info;
	DWORD dwBytesReturned;

	if (dio_handle == INVALID_HANDLE_VALUE) {
		return;
	}
	
	mmap_info.addr = paddr;
	mmap_info.size = size;

	DeviceIoControl(dio_handle, IOCTL_DIO_UNMAPMEM, &mmap_info,
		sizeof(mmap_info), NULL, 0, &dwBytesReturned, NULL);

	return;
}

BOOL _stdcall dio_read_mem(u64 paddr, u16 size, u32 *value)
{
	PBYTE vaddr;

	if (dio_handle == INVALID_HANDLE_VALUE) {
		return false;
	}

	vaddr = dio_map_memory(paddr, 4);
	if (!vaddr)
		return false;

	*value = *((u32 *)vaddr);
	dio_unmap_memory(paddr, 4);

	return true;
}


BOOL _stdcall dio_write_mem(u64 paddr, u16 size, u32 value)
{
	PBYTE vaddr;

	if (dio_handle == INVALID_HANDLE_VALUE) {
		return false;
	}
	
	vaddr = dio_map_memory(paddr, 4);
	if (!vaddr)
		return false;

	*((u32 *)vaddr) = value;
	dio_unmap_memory(paddr, 4);

	return true;
}

void _stdcall dio_exit()
{
	DWORD dwBytesReturned;
	
	if (dio_handle != INVALID_HANDLE_VALUE) {
		DeviceIoControl(dio_handle, IOCTL_DIO_EXIT, NULL,
			0, NULL, 0, &dwBytesReturned, NULL);

		CloseHandle(dio_handle);
		dio_handle = INVALID_HANDLE_VALUE;
	}

	dio_uninstall();
}

BOOL _stdcall dio_init()
{
	BOOL okey;
	DWORD dwBytesReturned;
	
	/* try to open existing driver, if available */
	dio_handle = CreateFile(DIRECTIO_FILE, GENERIC_READ | GENERIC_WRITE, 
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dio_handle != INVALID_HANDLE_VALUE) {
		return true;
	}
	
	okey = dio_prepare();
	if (!okey) {
		goto bailout;
	}
	dio_uninstall();
	okey = dio_install();
	if (!okey) {
		goto bailout;
	}

	dio_handle = CreateFile(DIRECTIO_FILE, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dio_handle == INVALID_HANDLE_VALUE) {
		okey = false;
		goto bailout;
	}
	
	okey = DeviceIoControl(dio_handle, IOCTL_DIO_INIT, NULL,
		0, NULL, 0, &dwBytesReturned, NULL);

bailout:
	if (!okey) {
		dio_exit();
	}

	return true;

}


BOOL APIENTRY DllMain(
	HINSTANCE hInstance,
	DWORD dwReason,
	LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		hDirectIoLib = hInstance;
		break;
	}

	return 1;
}

#if !defined(_M_AMD64) && !defined(_M_IX86)
  #error "Only x86 and AMD64 are supported"
#endif

