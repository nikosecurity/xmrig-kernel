#include <ntddk.h>

#include "XMRigKM.h"
#include "XMRigKM-IO.h"

UNICODE_STRING g_DeviceName = { 0 };
UNICODE_STRING g_DosDeviceName = { 0 };

PDEVICE_OBJECT g_pDeviceObject = 0;

// TODO: Create a DeviceExtension that contains the loaded state of the driver.
// If it is being unloaded, set it equal to zero (to prevent any potential calls to the I/O routines).
NTSTATUS XMRigUnload(PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);

	// We don't need to check the return value because this is the unload routine; it is the fault of the kernel for failing to free a memory allocation.
	// Nothing more we can do.
	IoDeleteSymbolicLink(&g_DosDeviceName);

	if (g_pDeviceObject)
	{
		IoDeleteDevice(g_pDeviceObject);
	}

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNREFERENCED_PARAMETER(pRegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;

	RtlInitUnicodeString(&g_DeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&g_DosDeviceName, DOS_DEVICE_NAME);

	Status = IoCreateDevice(pDriverObject, 0, &g_DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, 0, &g_pDeviceObject);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	Status = IoCreateSymbolicLink(&g_DosDeviceName, &g_DeviceName);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(g_pDeviceObject);
		return Status;
	}

	ASSERT(pDriverObject);
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = XMRigDispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = XMRigDispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = XMRigDispatchControl;
	pDriverObject->DriverUnload = XMRigUnload;

	return Status;
}