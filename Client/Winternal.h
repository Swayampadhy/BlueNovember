#pragma once

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemModuleInformation = 11,
} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_MODULE {
	PVOID   Unknown1;
	PVOID   Unknown2;
	PVOID   Base;
	ULONG   Size;
	ULONG   Flags;
	USHORT  Index;
	USHORT  NameLength;
	USHORT  LoadCount;
	USHORT  PathLength;
	CHAR    ImageName[256];
} SYSTEM_MODULE, * PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG                ModulesCount;
	SYSTEM_MODULE        Modules[0];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;

typedef
NTSTATUS(WINAPI* _NtQuerySystemInformation)(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength);