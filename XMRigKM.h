#pragma once

#define DEVICE_NAME L"\\Device\\XMRigKM"
#define DOS_DEVICE_NAME L"\\DosDevices\\XMRigKM"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);