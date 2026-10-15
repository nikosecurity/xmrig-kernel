#include <ntddk.h>
#include <Wdmsec.h>
#include <intrin.h>

#include "Defs.h"

UNICODE_STRING g_DeviceName = { 0 };
UNICODE_STRING g_DosDeviceName = { 0 };

PDEVICE_OBJECT g_DeviceObject = 0;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;

	RtlInitUnicodeString(&g_DeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&g_DosDeviceName, DOS_DEVICE_NAME);

	Status = XMRigInitializeStates();
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	Status = IoCreateDeviceSecure(DriverObject, 0, &g_DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, 0, &SDDL_DEVOBJ_SYS_ALL_ADM_ALL, 0, &g_DeviceObject);
	if (!NT_SUCCESS(Status))
	{
		return Status;
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

	// We don't need to check the return values on any of these functions because this is the unload routine.
	// If the kernel fails to delete an object, then there's not really much else we can do about that.
	// Though, we should still return STATUS_SUCCESS. This way, the driver can still unload cleanly.

	XMRigDestroyStates();

	IoDeleteSymbolicLink(&g_DosDeviceName);
	IoDeleteDevice(g_DeviceObject);
	return STATUS_SUCCESS;
}