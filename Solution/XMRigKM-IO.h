#pragma once

// Magic XMRig MSRs (Model-specific register).
// These are used to enable or disable memory prefetching, as the algorithm used by Monero is RandomX.
// This algorithm randomly accesses memory (hence the name), so prefetching memory addresses seems to slow it down.
//
// Seems like setting this MSR to zero enables it, and setting it to 15 (or some other value) disables it to a seemingly controlled degree?
// Not sure.
#define MSR_MEMORY_PREFETCH_INTEL 0x1A4

// As for these MSRs, these are for AMD.
// Not sure why there's four of them (and what's the difference between them), and testing them will be painful as my main system does not have an AMD CPU.
// But, these *do* exist in xmrig, and someone has to add them...
#define MSR_MEMORY_PREFETCH_AMD_1 0xC0011020
#define MSR_MEMORY_PREFETCH_AMD_2 0xC0011021
#define MSR_MEMORY_PREFETCH_AMD_3 0xC0011022
#define MSR_MEMORY_PREFETCH_AMD_4 0xC001102B

#define IOCTL_READ_MSR  CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _READ_MSR_DATA
{
	unsigned long Register;
} READ_MSR_DATA, * PREAD_MSR_DATA;

typedef struct _WRITE_MSR_DATA
{
	unsigned long Register;
	// The compiler wants to shove this up its ass soooooo badly, so I'll do that FOR it.
	unsigned long Padding;
	unsigned long long Value;
} WRITE_MSR_DATA, * PWRITE_MSR_DATA;

typedef struct _ALLOCATE_GB_PAGE
{
	SIZE_T Size;
} ALLOCATE_GB_PAGE, * PALLOCATE_GB_PAGE;

NTSTATUS XMRigDispatchCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS XMRigDispatchClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS XMRigDispatchControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);