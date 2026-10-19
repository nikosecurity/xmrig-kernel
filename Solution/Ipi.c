#include <ntifs.h>
#include <intrin.h>

#include "Defs.h"

//
// Note: I need to find a way to validate each individual MSR before accessing them.
// Currently, if this code were to execute on a CPU that does not support each MSR, the system will inevitably crash.
// As such, I will only implement these within the I/O dispatch routines for now as opposed to automatically saving and restoring upon DriverEntry and DriverUnload.
//
// The way to do this is via checking whether or not the functionality is supported on the CPU ahead of time prior to reading or writing to those MSRs.
// However, I honestly do not know how to confirm that (I don't use cpuid enough, but from what I've read it should be in there).
//
// As such, I will leave it as-is for the time being. Worst case scenario, it is a bug check on older, unsupported hardware, which under Microsoft's own guidelines does not constitute as a vulnerability in this case.
// (The reasoning for the above is that if an attacker already has local code execution, they can already crash the system via RtlRaiseHardError or shutdown /s /t 0 /f,
// this driver's functionality cannot be invoked remotely, and as such does not have security impact)
//
// I may come back to this one day in the future, however.
//

NTSTATUS XMRigSaveStates(void)
{
	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			g_MsrStates[KeGetCurrentProcessorIndex()]->Values[i] = __readmsr(g_MsrStates[KeGetCurrentProcessorIndex()]->Registers[i]);
		}
		break;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			g_MsrStates[KeGetCurrentProcessorIndex()]->Values[i] = __readmsr(g_MsrStates[KeGetCurrentProcessorIndex()]->Registers[i]);
		}
		break;
	}
	default:
	{
		return STATUS_UNKNOWN_REVISION;
	}
	}

	return STATUS_SUCCESS;
}

NTSTATUS XMRigRestoreStates(void)
{
	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			__writemsr(g_MsrStates[KeGetCurrentProcessorIndex()]->Registers[i], g_MsrStates[KeGetCurrentProcessorIndex()]->Values[i]);
		}
		break;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			__writemsr(g_MsrStates[KeGetCurrentProcessorIndex()]->Registers[i], g_MsrStates[KeGetCurrentProcessorIndex()]->Values[i]);
		}
		break;
	}
	default:
	{
		return STATUS_UNKNOWN_REVISION;
	}
	}

	return STATUS_SUCCESS;
}