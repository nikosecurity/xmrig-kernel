#pragma once

#define DEVICE_NAME L"\\Device\\XMRigKM"
#define DOS_DEVICE_NAME L"\\DosDevices\\XMRigKM"

// Magic XMRig MSRs (Model-specific register).
// These are used to enable or disable memory prefetching, as the algorithm used by Monero is RandomX.
// This algorithm randomly accesses memory (hence the name), so prefetching memory addresses seems to slow it down.
//
// Seems like setting this MSR to zero enables it, and setting it to 15 (or some other value) disables it to a seemingly controlled degree?
// Not sure.
#define MSR_MEMORY_PREFETCH_INTEL_1 0x1A4

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
	unsigned long Padding;			// MSVC really wanted to add padding bytes in this structure definition as it was not aligned to an 8-byte boundary, so I added them myself here.
	unsigned long long Value;
} WRITE_MSR_DATA, * PWRITE_MSR_DATA;

typedef enum _CPU_VENDOR {
	CpuUnknown,
	CpuIntel,
	CpuAmd
} CPU_VENDOR;

NTSTATUS XMRigUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS XMRigDispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS XMRigDispatchControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

CPU_VENDOR XMRigGetVendor(void);
NTSTATUS XMRigIsValidRegister(ULONG Register);

extern CPU_VENDOR g_Vendor;
extern ULONG g_Whitelist_Intel[1];
extern ULONG g_Whitelist_Amd[4];