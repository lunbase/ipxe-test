/*
 * DirectIO
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

#include <ntddk.h>
#include "directio.h"

#pragma warning(disable:4305)

#ifndef AMD64
void Ke386SetIoAccessMap(int, PVOID);
void Ke386QueryIoAccessMap(int, PVOID);
void Ke386IoSetAccessProcess(PEPROCESS, int);
#endif /*!AMD64*/

typedef struct _DIO_SECTION {
	HANDLE handle;
	PVOID object;
	PVOID vaddr;
	PVOID paddr;
	SIZE_T size;
} DIO_SECTION, *PDIO_SECTION;

typedef struct _DIO_CONTEXT {
	ULONG flags;
	PVOID cache;
	DIO_SECTION section;
} DIO_CONTEXT, *PDIO_CONTEXT;

NTSTATUS dio_map_memory(PDIO_SECTION section)
{
	UNICODE_STRING sect_name;
	OBJECT_ATTRIBUTES sect_attr;
	HANDLE sect_handle;
	PVOID sect_object = NULL;
	PVOID sect_vaddr = NULL;
	SIZE_T sect_size;
	PHYSICAL_ADDRESS paddr_base, paddr_begin, paddr_end;
	ULONG is_mmio;
	BOOLEAN Result1, Result2;
	NTSTATUS status;

	RtlInitUnicodeString(&sect_name, L"\\Device\\PhysicalMemory");

	InitializeObjectAttributes(&sect_attr, &sect_name,
		OBJ_CASE_INSENSITIVE, (HANDLE)NULL, (PSECURITY_DESCRIPTOR)NULL);

	status = ZwOpenSection(&sect_handle, SECTION_ALL_ACCESS, &sect_attr);
	if (!NT_SUCCESS(status)) {
		KdPrint(("ERROR: ZwOpenSection failed"));
		goto bailout;
	}

	status = ObReferenceObjectByHandle(sect_handle, SECTION_ALL_ACCESS, (POBJECT_TYPE)NULL,
		KernelMode, &sect_object, (POBJECT_HANDLE_INFORMATION)NULL);
	if (!NT_SUCCESS(status)) {
		KdPrint(("ERROR: ObReferenceObjectByHandle failed"));
		goto bailout;
	}

	paddr_begin.QuadPart = (ULONGLONG)(ULONG_PTR)section->paddr;
	paddr_end.QuadPart = paddr_begin.QuadPart + section->size;

	is_mmio = 0;
	Result1 = HalTranslateBusAddress(1, 0, paddr_begin, &is_mmio, &paddr_begin);
	is_mmio = 0;
	Result2 = HalTranslateBusAddress(1, 0, paddr_end, &is_mmio, &paddr_end);
	if (!Result1 || !Result2) {
		KdPrint(("ERROR: HalTranslateBusAddress failed"));
	}

	sect_size = (SIZE_T)(paddr_end.QuadPart - paddr_begin.QuadPart);
	paddr_base = paddr_begin;
	status = ZwMapViewOfSection(sect_handle, ZwCurrentProcess(), &sect_vaddr, 0L, sect_size,
		&paddr_base, &sect_size, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);
	if (status == STATUS_CONFLICTING_ADDRESSES) {
		// already mapped with a different caching attribute, try again
		status = ZwMapViewOfSection(sect_handle, ZwCurrentProcess(), &sect_vaddr, 0L, sect_size,
			&paddr_base, &sect_size, ViewShare, 0, PAGE_READWRITE);
	}
	if (!NT_SUCCESS(status)) {
		KdPrint(("ERROR: ZwMapViewOfSection failed"));
		goto bailout;
	}
	sect_vaddr = (u8*)sect_vaddr + paddr_begin.QuadPart - paddr_base.QuadPart;

	section->handle = sect_handle;
	section->object = sect_object;
	section->vaddr = sect_vaddr;
	section->size = sect_size;

bailout:
	if (!NT_SUCCESS(status))
		ZwClose(sect_handle);

	return status;
}


NTSTATUS dio_unmap_memory(PDIO_SECTION section)
{
	NTSTATUS status;

	status = ZwUnmapViewOfSection(ZwCurrentProcess(), section->vaddr);
	if (!NT_SUCCESS(status))
		KdPrint(("ERROR: UnmapViewOfSection failed"));
	ObDereferenceObject(section->object);
	ZwClose(section->handle);
	RtlZeroMemory(section, sizeof(*section));

	return status;
}

NTSTATUS dio_ctrl_default(
	IN PDEVICE_OBJECT dev_obj,
	IN PIRP irp)
{
	PIO_STACK_LOCATION irp_stack;
	PDIO_CONTEXT dio_ctxt;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(dev_obj);

	/* defaults */
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	irp_stack = IoGetCurrentIrpStackLocation(irp);
	if (!irp_stack)
		goto bailout;
	dio_ctxt = irp_stack->FileObject->FsContext2;

	switch (irp_stack->MajorFunction) {
	case IRP_MJ_CREATE:
		if (dio_ctxt)
			break;
		dio_ctxt = MmAllocateNonCachedMemory(sizeof(DIO_CONTEXT));
		if (!dio_ctxt) {
			irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlZeroMemory(dio_ctxt, sizeof(DIO_CONTEXT));
		irp_stack->FileObject->FsContext2 = dio_ctxt;
		break;

	case IRP_MJ_CLOSE:
		if (!dio_ctxt)
			break;
		MmFreeNonCachedMemory(dio_ctxt, sizeof(DIO_CONTEXT));
		irp_stack->FileObject->FsContext2 = NULL;
		break;

	default:
		irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

bailout:
	status = irp->IoStatus.Status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS dio_ctrl_device(
	IN PDEVICE_OBJECT dev_obj,
	IN PIRP irp)
{
	PIO_STACK_LOCATION irp_stack;
	PDIO_CONTEXT dio_ctxt;
	DIO_PORT_INFO dio_port;
	DIO_MMAP_INFO dio_mmap;
	void *sys_buff;
	u32 buff_olen;
	u32 buff_ilen;
	u32 ctrl_code;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(dev_obj);

	/* defaults */
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	irp_stack = IoGetCurrentIrpStackLocation(irp);
	if (!irp_stack)
		goto bailout;
	if (irp_stack->MajorFunction != IRP_MJ_DEVICE_CONTROL) {
		irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
		goto bailout;
	}

	sys_buff = irp->AssociatedIrp.SystemBuffer;
	ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;
	buff_ilen = irp_stack->Parameters.DeviceIoControl.InputBufferLength;
	buff_olen = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;
	dio_ctxt = irp_stack->FileObject->FsContext2;

	switch (ctrl_code) {
	case IOCTL_DIO_INIT:
#ifndef AMD64
#define IOPM_SIZE 0x2000
		if (dio_ctxt->cache)
			break;
		dio_ctxt->cache = MmAllocateNonCachedMemory(IOPM_SIZE);
		if (!dio_ctxt->cache) {
			irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlZeroMemory(dio_ctxt->cache, IOPM_SIZE);
		Ke386IoSetAccessProcess(PsGetCurrentProcess(), 1);
		Ke386SetIoAccessMap(1, dio_ctxt->cache);
#endif /*!AMD64*/
		break;

	case IOCTL_DIO_EXIT:
#ifndef AMD64
		if (!dio_ctxt->cache)
			break;
		Ke386IoSetAccessProcess(PsGetCurrentProcess(), 0);
		Ke386SetIoAccessMap(1, dio_ctxt->cache);
		MmFreeNonCachedMemory(dio_ctxt->cache, IOPM_SIZE);
		dio_ctxt->cache = NULL;
#endif /*!AMD64*/
		break;

	case IOCTL_DIO_READ:
		if (buff_ilen < sizeof(dio_port)) {
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
		if (buff_olen < sizeof(dio_port.value)) {
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		RtlCopyMemory(&dio_port, sys_buff, sizeof(dio_port));

		if (dio_port.size == 1)
			dio_port.value = (u32)READ_PORT_UCHAR((u8 *)dio_port.port);
		else if (dio_port.size == 2)
			dio_port.value = (u32)READ_PORT_USHORT((u16 *)dio_port.port);
		else if (dio_port.size == 4)
			dio_port.value = (u32)READ_PORT_ULONG((u32 *)dio_port.port);
		else {
			dio_port.value = (u32)0xFFFFFFFF;
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		}

		irp->IoStatus.Information = sizeof(dio_port.value);
		RtlCopyBytes(sys_buff, &dio_port.value, sizeof(dio_port.value));
		break;

	case IOCTL_DIO_WRITE:
		if (buff_ilen < sizeof(dio_port)) {
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlCopyMemory(&dio_port, sys_buff, sizeof(dio_port));

		if (dio_port.size == 1)
			WRITE_PORT_UCHAR((PUCHAR)dio_port.port, (UCHAR)dio_port.value);
		else if (dio_port.size == 2)
			WRITE_PORT_USHORT((PUSHORT)dio_port.port, (USHORT)dio_port.value);
		else if (dio_port.size == 4)
			WRITE_PORT_ULONG((PULONG)dio_port.port, (ULONG)dio_port.value);
		else {
			dio_port.value = (u32)0xFFFFFFFF;
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		}
		break;

	case IOCTL_DIO_MAPMEM:
		if (buff_ilen < sizeof(dio_mmap)) {
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlCopyMemory(&dio_mmap, sys_buff, sizeof(dio_mmap));

		if (dio_ctxt->flags) {
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}

		dio_ctxt->section.paddr = (PVOID)dio_mmap.addr;
		dio_ctxt->section.size = (SIZE_T)dio_mmap.size;
		status = dio_map_memory(&dio_ctxt->section);
		irp->IoStatus.Status = status;
		if (!NT_SUCCESS(status)) {
			break;
		}

		dio_ctxt->flags = 1;
		dio_mmap.addr = (u64)dio_ctxt->section.vaddr;
		RtlCopyMemory(sys_buff, &dio_mmap, sizeof(dio_mmap));
		irp->IoStatus.Information = sizeof(dio_mmap);
		break;

	case IOCTL_DIO_UNMAPMEM:
		if (buff_ilen < sizeof(dio_mmap)) {
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
		RtlCopyMemory(&dio_mmap, sys_buff, sizeof(dio_mmap));

		if (!dio_ctxt->flags) {
			irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
			break;
		}
		status = dio_unmap_memory(&dio_ctxt->section);
		irp->IoStatus.Status = status;
		if (!NT_SUCCESS(status)) {
			break;
		}
		dio_ctxt->flags = 0;
		break;

	default:
		irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

bailout:
	status = irp->IoStatus.Status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

void DriverUnload(
	IN PDRIVER_OBJECT drv_obj)
{
	UNICODE_STRING lnk_name;	
	NTSTATUS status;

	RtlInitUnicodeString(&lnk_name, DIO_LNKNAME);

	status = IoDeleteSymbolicLink(&lnk_name);
	if (!NT_SUCCESS(status)) {
		KdPrint(( "DIO: error on IoDeleteSymbolicLink" ));
	}

	IoDeleteDevice(drv_obj->DeviceObject);
}

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT drv_obj,
	IN PUNICODE_STRING reg_path)
{
	PDEVICE_OBJECT dev_obj;
	UNICODE_STRING dev_name;
	UNICODE_STRING lnk_name;
	NTSTATUS status;

	UNREFERENCED_PARAMETER(reg_path);

	drv_obj->DriverUnload = DriverUnload;
	drv_obj->MajorFunction[IRP_MJ_CREATE] = dio_ctrl_default;
	drv_obj->MajorFunction[IRP_MJ_CLOSE] = dio_ctrl_default;
	drv_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = dio_ctrl_device;

	RtlInitUnicodeString(&dev_name, DIO_DEVNAME);
	RtlInitUnicodeString(&lnk_name, DIO_LNKNAME);

	status = IoCreateDevice(drv_obj, 0, &dev_name, DIO_DEVTYPE, 0, false, &dev_obj);
	if (!NT_SUCCESS(status))
		return status;

	status = IoCreateSymbolicLink(&lnk_name, &dev_name);
	if (!NT_SUCCESS(status))
		return status;

	return STATUS_SUCCESS;
}
