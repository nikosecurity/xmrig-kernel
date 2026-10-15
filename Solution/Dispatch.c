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
	UNREFERENCED_PARAMETER(DeviceObject);

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
		//
		// It is wrapped in a try-catch to prevent any issues with older hardware (which does not support the target MSR).
		__try
		{
			*(unsigned long long*)SystemBuffer = __readmsr(ReadMsrData.Register);
			BytesReturned = sizeof(unsigned long long);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// If this instruction causes a fault, then it is absolutely because the MSR is unsupported on the CPU.
			// So, we bail out safely and we just set the return status to STATUS_NOT_SUPPORTED (0xC00000BB).
			Status = STATUS_NOT_SUPPORTED;
		}

		// BytesReturned is only set if the rdmsr instruction executes without any errors.
		// Because BytesReturned is initialized to 0 by default and is only set when data is returned, there are no concerns with leaking arbitrary kernel memory as it's simply not possible.
		// Also because of that, we do not need to zero out the system buffer nor the BytesReturned variable either. We're all good!

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
		//
		// It is wrapped in a try-catch to prevent any issues with older hardware (which does not support the target MSR).
		__try
		{
			__writemsr(WriteMsrData.Register, WriteMsrData.Value);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			// If this instruction causes a fault, then it is absolutely because the MSR is unsupported on the CPU.
			// So, we bail out safely and we just set the return status to STATUS_NOT_SUPPORTED (0xC00000BB).
			Status = STATUS_NOT_SUPPORTED;
		}

		// Break out of the switch and complete execution.
		break;
	}
	case IOCTL_SAVE_ALL_MSRS:
	{
		Status = (NTSTATUS)KeIpiGenericCall((PKIPI_BROADCAST_WORKER)XMRigSaveStates, 0);
		break;
	}
	case IOCTL_RESTORE_ALL_MSRS:
	{
		Status = (NTSTATUS)KeIpiGenericCall((PKIPI_BROADCAST_WORKER)XMRigRestoreStates, 0);
		break;
	}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesReturned;

	IoCompleteRequest(Irp, 0);
	return Status;
}