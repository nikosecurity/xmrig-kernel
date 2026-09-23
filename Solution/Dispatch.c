#include <ntifs.h>
#include <intrin.h>

#include "Defs.h"

NTSTATUS XMRigDispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;

	IoCompleteRequest(Irp, 0);
	return Status;
}

NTSTATUS XMRigDispatchControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION IO = 0;

	PVOID SystemBuffer = 0;

	ULONG_PTR BytesReturned = 0;
	NTSTATUS Status = STATUS_INVALID_PARAMETER;

	// Get the stack location of the IRP for later usage.
	IO = IoGetCurrentIrpStackLocation(Irp);

	// Friendly reminder, this is buffered I/O! You can use the same buffer for both the input AND output!
	SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

	switch (IO->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_READ_MSR:
	{
		READ_MSR_DATA ReadMsrData = { 0 };

		// In addition to the other obvious checks, we check to ensure the output buffer length is 8.
		// Assuming that the MSR supplied is a permitted MSR, we want to read the data from it and return its value to user-mode.
		if (!SystemBuffer || IO->Parameters.DeviceIoControl.InputBufferLength != sizeof(ReadMsrData) || IO->Parameters.DeviceIoControl.OutputBufferLength != sizeof(unsigned long long))
		{
			// The user-mode client sent invalid data, so the driver should just bail out gracefully.
			break;
		}

		memcpy(&ReadMsrData, SystemBuffer, sizeof(ReadMsrData));

		Status = XMRigIsValidRegister(ReadMsrData.Register);
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// rdmsr takes in an argument from ecx (model-specific register) and returns a value via rax.
		*(unsigned long long*)SystemBuffer = __readmsr(ReadMsrData.Register);
		BytesReturned = sizeof(unsigned long long);

		// Break out of the switch and complete execution.
		break;
	}
	case IOCTL_WRITE_MSR:
	{
		WRITE_MSR_DATA WriteMsrData = { 0 };

		if (!SystemBuffer || IO->Parameters.DeviceIoControl.InputBufferLength != sizeof(WriteMsrData) || IO->Parameters.DeviceIoControl.OutputBufferLength)
		{
			// The user-mode client sent invalid data, so again, the driver should just bail out gracefully.
			break;
		}

		memcpy(&WriteMsrData, SystemBuffer, sizeof(WriteMsrData));

		Status = XMRigIsValidRegister(WriteMsrData.Register);
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// wrmsr takes in arguments from ecx (register) and rax (value).
		// There are no return values.
		__writemsr(WriteMsrData.Register, WriteMsrData.Value);

		// Break out of the switch and complete execution.
		break;
	}
	case IOCTL_RESET_MSR:
	{
		unsigned long long* MsrData = (unsigned long long*)DeviceObject->DeviceExtension;

		// No validation needs to be performed on the input buffer as nothing is read from or written to.

		switch (g_Vendor)
		{
		case CpuIntel:
		{
			for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
			{
				__writemsr(g_Whitelist_Intel[i], MsrData[i]);
			}

			Status = STATUS_SUCCESS;
			break;
		}
		case CpuAmd:
		{
			for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
			{
				__writemsr(g_Whitelist_Amd[i], MsrData[i]);
			}

			Status = STATUS_SUCCESS;
			break;
		}
		default:
		{
			Status = STATUS_UNKNOWN_REVISION;
			break;
		}
		}

		break;
	}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesReturned;

	IoCompleteRequest(Irp, 0);
	return Status;
}