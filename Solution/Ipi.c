#include <ntifs.h>
#include <intrin.h>

#include "Defs.h"

//
// Note: I need to find a way to validate each individual MSR before accessing them.
// Currently, if this code were to execute on a CPU that does not support each MSR, the system will inevitably crash.
// As such, I will only implement these within the I/O dispatch routines for now as opposed to automatically saving and restoring upon DriverEntry and DriverUnload.
//

NTSTATUS XMRigSaveStates(void)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PMSR_STATE CurrentState = g_MsrState;

	for (unsigned long i = 0; i < KeQueryActiveProcessorCount(0); i++)
	{
		if (!CurrentState)
		{
			__debugbreak();
		}

		if (CurrentState->ProcessorNumber == KeGetCurrentProcessorIndex())
		{
			break;
		}

		CurrentState = CurrentState->Flink;
	}

	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			CurrentState->Values[i] = __readmsr(CurrentState->Registers[i]);
		}
		break;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			CurrentState->Values[i] = __readmsr(CurrentState->Registers[i]);
		}
		break;
	}
	default:
	{
		return STATUS_UNKNOWN_REVISION;
	}
	}

	return Status;
}

NTSTATUS XMRigRestoreStates(void)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PMSR_STATE CurrentState = g_MsrState;

	for (unsigned long i = 0; i < KeQueryActiveProcessorCount(0); i++)
	{
		if (!CurrentState)
		{
			__debugbreak();
		}

		if (CurrentState->ProcessorNumber == KeGetCurrentProcessorIndex())
		{
			break;
		}

		CurrentState = CurrentState->Flink;
	}

	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			__writemsr(CurrentState->Registers[i], CurrentState->Values[i]);
		}
		break;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			__writemsr(CurrentState->Registers[i], CurrentState->Values[i]);
		}
		break;
	}
	default:
	{
		return STATUS_UNKNOWN_REVISION;
	}
	}

	return Status;
}