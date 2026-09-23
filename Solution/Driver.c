#include <ntddk.h>
#include <Wdmsec.h>
#include <intrin.h>

#include "Defs.h"

UNICODE_STRING g_DeviceName = { 0 };
UNICODE_STRING g_DosDeviceName = { 0 };

PDEVICE_OBJECT g_DeviceObject = 0;

CPU_VENDOR g_Vendor = 0;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	unsigned long long* MsrData = 0;
	unsigned long MsrDataSize = 0;

	NTSTATUS Status = STATUS_SUCCESS;

	RtlInitUnicodeString(&g_DeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&g_DosDeviceName, DOS_DEVICE_NAME);

	g_Vendor = XMRigGetVendor();
	switch (g_Vendor)
	{
	case CpuIntel:
	{
		MsrDataSize = (sizeof(g_Whitelist_Intel) / sizeof(ULONG)) * sizeof(unsigned long long);
		break;
	}
	case CpuAmd:
	{
		MsrDataSize = (sizeof(g_Whitelist_Amd) / sizeof(ULONG)) * sizeof(unsigned long long);
		break;
	}
	default:
	{
		return STATUS_UNKNOWN_REVISION;
	}
	}

	Status = IoCreateDeviceSecure(DriverObject, MsrDataSize, &g_DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, 0, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL, 0, &g_DeviceObject);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	MsrData = g_DeviceObject->DeviceExtension;
	if (!MsrData)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			MsrData[i] = __readmsr(g_Whitelist_Intel[i]);
		}

		break;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			MsrData[i] = __readmsr(g_Whitelist_Amd[i]);
		}
		break;
	}
	}

	Status = IoCreateSymbolicLink(&g_DosDeviceName, &g_DeviceName);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(g_DeviceObject);
		return Status;
	}

	ASSERT(DriverObject);
	DriverObject->MajorFunction[IRP_MJ_CREATE] = XMRigDispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = XMRigDispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = XMRigDispatchControl;
	DriverObject->DriverUnload = XMRigUnload;

	return Status;
}

NTSTATUS XMRigUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	unsigned long long* MsrData = 0;

	// We don't need to check the return values on any of these functions because this is the unload routine.
	// If the kernel fails to delete an object, then there's not really much else we can do about that.
	// Though, we should still return STATUS_SUCCESS. This way, the driver can still unload cleanly.

	IoDeleteSymbolicLink(&g_DosDeviceName);

	if (!g_DeviceObject)
	{
		return STATUS_SUCCESS;
	}

	MsrData = g_DeviceObject->DeviceExtension;
	if (!MsrData)
	{
		IoDeleteDevice(g_DeviceObject);
		return STATUS_SUCCESS;
	}

	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			__writemsr(g_Whitelist_Intel[i], MsrData[i]);
		}

		break;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			__writemsr(g_Whitelist_Amd[i], MsrData[i]);
		}

		break;
	}
	}

	IoDeleteDevice(g_DeviceObject);

	return STATUS_SUCCESS;
}