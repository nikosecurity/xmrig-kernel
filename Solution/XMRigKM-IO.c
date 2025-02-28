#include <ntifs.h>
#include <intrin.h>

#include "XMRigKM-IO.h"

// This is an array of permitted MSRs (to prevent syscall and sysenter overwrites, among other vectors).
unsigned long g_pAllowedMSRs[] =
{
	MSR_MEMORY_PREFETCH_INTEL
};

NTSTATUS XMRigDispatchCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	NTSTATUS Status = STATUS_SUCCESS;

	pIrp->IoStatus.Status = Status;

	IoCompleteRequest(pIrp, 0);
	return Status;
}

NTSTATUS XMRigDispatchClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	NTSTATUS Status = STATUS_SUCCESS;

	pIrp->IoStatus.Status = Status;

	IoCompleteRequest(pIrp, 0);
	return Status;
}

NTSTATUS XMRigDispatchControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	PIO_STACK_LOCATION pIO = 0;

	PVOID pSystemBuffer = 0;
	ULONG_PTR BytesReturned = 0;

	unsigned long Register = 0;

	NTSTATUS Status = STATUS_INVALID_PARAMETER;

	// Get the stack location of the IRP for later usage.
	pIO = IoGetCurrentIrpStackLocation(pIrp);

	// Friendly reminder, this is buffered I/O! You can use the same buffer for both the input AND output!
	pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;

	switch (pIO->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_READ_MSR:
	{
		READ_MSR_DATA ReadMsrData = { 0 };

		// In addition to the other obvious checks, we check to ensure the output buffer length is 8.
		// Assuming that the MSR supplied is a permitted MSR, we want to read the data from it and return its value to user-mode.
		if (!pSystemBuffer || pIO->Parameters.DeviceIoControl.InputBufferLength != sizeof(ReadMsrData) || pIO->Parameters.DeviceIoControl.OutputBufferLength != sizeof(unsigned long long))
		{
			// The user-mode client sent invalid data, so the driver should just bail out gracefully.
			break;
		}

		memcpy(&ReadMsrData, pSystemBuffer, sizeof(ReadMsrData));

		for (unsigned long i = 0; i < sizeof(g_pAllowedMSRs) / sizeof(unsigned long); i++)
		{
			Register = g_pAllowedMSRs[i];

			if (ReadMsrData.Register == Register)
			{
				Status = STATUS_SUCCESS;
				BytesReturned = sizeof(unsigned long long);

				// rdmsr takes in an argument from ecx (model-specific register) and returns a value via rax.
				*(unsigned long long*)pSystemBuffer = __readmsr(Register);

				// Break out of the for-loop and finish execution.
				break;
			}
		}

		break;
	}
	case IOCTL_WRITE_MSR:
	{
		WRITE_MSR_DATA WriteMsrData = { 0 };

		if (!pSystemBuffer || pIO->Parameters.DeviceIoControl.InputBufferLength != sizeof(WriteMsrData))
		{
			// The user-mode client sent invalid data, so again, the driver should just bail out gracefully.
			break;
		}

		memcpy(&WriteMsrData, pSystemBuffer, sizeof(WriteMsrData));

		for (unsigned long i = 0; i < sizeof(g_pAllowedMSRs) / sizeof(unsigned long); i++)
		{
			Register = g_pAllowedMSRs[i];

			if (WriteMsrData.Register == Register)
			{
				Status = STATUS_SUCCESS;

				// wrmsr takes in arguments from ecx (register) and rax (value).
				// There are no return values.
				__writemsr(Register, WriteMsrData.Value);

				// Break out of the for-loop and finish execution.
				break;
			}
		}

		break;
	}
	}

	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = BytesReturned;

	IoCompleteRequest(pIrp, 0);
	return Status;
}