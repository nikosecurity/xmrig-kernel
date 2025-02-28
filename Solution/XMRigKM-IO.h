#pragma once

// Magic XMRig MSR (Model-specific register).
// This is used to disable memory prefetching, as the algorithm used by Monero is RandomX.
// This algorithm randomly accesses memory (hence the name), so prefetching memory addresses actually slows it down.
//
// Seems like setting this MSR to zero enables it, and setting it to 15 (or some other value) disables it to a seemingly controlled degree?
// Not sure.
#define MSR_MEMORY_PREFETCH_INTEL 0x1A4

// TODO: Add Ryzen prefetching MSRs to support AMD.

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