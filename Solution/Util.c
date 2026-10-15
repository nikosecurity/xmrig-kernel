#include <ntifs.h>
#include <intrin.h>

#include "Defs.h"

// These are arrays of permitted MSRs (to prevent syscall and sysenter overwrites, among other vectors).
// While this could be dynamically allocated, there's no real reason for me to over-engineer this driver more than I already have. If it gets the job done, it gets the job done!

ULONG g_Whitelist_Intel[1] =
{
	MSR_MEMORY_PREFETCH_INTEL_1
};
ULONG g_Whitelist_Amd[4] =
{
	MSR_MEMORY_PREFETCH_AMD_1, MSR_MEMORY_PREFETCH_AMD_2, MSR_MEMORY_PREFETCH_AMD_3, MSR_MEMORY_PREFETCH_AMD_4
};

CPU_VENDOR g_Vendor = 0;
PMSR_STATE g_MsrState = 0;

CPU_VENDOR XMRigGetVendor(void)
{
	int cpuInfo[4] = { 0 };

	// Call CPUID with Function ID 0
	__cpuid(cpuInfo, 0);

	// The vendor string is split across EBX, EDX, and ECX
	// Intel: "GenuineIntel" -> EBX: 0x756E6547, EDX: 0x49656E69, ECX: 0x6C65746E
	// AMD:   "AuthenticAMD" -> EBX: 0x68747541, EDX: 0x69746E65, ECX: 0x444D4163

	if (cpuInfo[1] == 0x756E6547 && cpuInfo[3] == 0x49656E69 && cpuInfo[2] == 0x6C65746E)
	{
		// Intel CPU
		return CpuIntel;
	}
	else if (cpuInfo[1] == 0x68747541 && cpuInfo[3] == 0x69746E65 && cpuInfo[2] == 0x444D4163)
	{
		// AMD CPU
		return CpuAmd;
	}

	return CpuUnknown;
}

NTSTATUS XMRigIsValidRegister(ULONG Register)
{
	switch (g_Vendor)
	{
	case CpuIntel:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Intel) / sizeof(ULONG); i++)
		{
			if (Register == g_Whitelist_Intel[i])
			{
				// The MSR is whitelisted, so we allow it.
				return STATUS_SUCCESS;
			}
		}

		// The MSR is not present within the whitelist array, so we deny access to it.
		return STATUS_ACCESS_DENIED;
	}
	case CpuAmd:
	{
		for (ULONG i = 0; i < sizeof(g_Whitelist_Amd) / sizeof(ULONG); i++)
		{
			if (Register == g_Whitelist_Amd[i])
			{
				// The MSR is whitelisted, so we allow it.
				return STATUS_SUCCESS;
			}
		}

		// The MSR is not present within the whitelist array, so we deny access to it.
		return STATUS_ACCESS_DENIED;
	}
	}

	// If the vendor is unknown, then we simply return an unknown revision.
	return STATUS_UNKNOWN_REVISION;
}

NTSTATUS XMRigInitializeStates(void)
{
	PMSR_STATE PreviousState = 0;
	PMSR_STATE CurrentState = 0;

	unsigned long RegisterCount = 0;

	g_Vendor = XMRigGetVendor();
	switch (g_Vendor)
	{
	case CpuIntel:
	{
		RegisterCount = sizeof(g_Whitelist_Intel) / sizeof(ULONG);
		break;
	}
	case CpuAmd:
	{
		RegisterCount = sizeof(g_Whitelist_Amd) / sizeof(ULONG);
		break;
	}
	default:
	{
		return STATUS_UNKNOWN_REVISION;
	}
	}

	g_MsrState = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(MSR_STATE), POOL_TAG);
	if (!g_MsrState)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlSecureZeroMemory(g_MsrState, sizeof(MSR_STATE));

	g_MsrState->ProcessorNumber = 0;
	g_MsrState->Values = ExAllocatePool2(POOL_FLAG_NON_PAGED, RegisterCount * sizeof(unsigned long long), POOL_TAG);
	if (!g_MsrState->Values)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlSecureZeroMemory(g_MsrState->Values, RegisterCount * sizeof(unsigned long long));

	switch (g_Vendor)
	{
	case CpuIntel:
	{
		g_MsrState->Registers = g_Whitelist_Intel;
		break;
	}
	case CpuAmd:
	{
		g_MsrState->Registers = g_Whitelist_Amd;
		break;
	}
	}

	PreviousState = g_MsrState;

	for (unsigned long i = 1; i < KeQueryActiveProcessorCount(0); i++)
	{
		CurrentState = ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(MSR_STATE), POOL_TAG);
		if (!CurrentState)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		RtlSecureZeroMemory(CurrentState, sizeof(MSR_STATE));

		CurrentState->ProcessorNumber = i;
		CurrentState->Registers = g_MsrState->Registers;
		CurrentState->Values = ExAllocatePool2(POOL_FLAG_NON_PAGED, RegisterCount * sizeof(unsigned long long), POOL_TAG);
		if (!CurrentState->Values)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		RtlSecureZeroMemory(CurrentState->Values, RegisterCount * sizeof(unsigned long long));

		PreviousState->Flink = CurrentState;
		PreviousState = CurrentState;
	}

	return STATUS_SUCCESS;
}

void XMRigDestroyStates(void)
{
	PMSR_STATE CurrentState = 0;
	PMSR_STATE PreviousState = 0;

	char IsDone = 0;

	while (!IsDone)
	{
		CurrentState = g_MsrState;

		while (CurrentState->Flink)
		{
			PreviousState = CurrentState;
			CurrentState = CurrentState->Flink;
		}

		if (CurrentState != g_MsrState)
		{
			PreviousState->Flink = 0;
		}
		else
		{
			IsDone = 1;
		}

		ExFreePoolWithTag(CurrentState->Values, POOL_TAG);
		ExFreePoolWithTag(CurrentState, POOL_TAG);
	}

	g_MsrState = 0;
}