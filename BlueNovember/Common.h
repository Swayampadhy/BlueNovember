#pragma once

struct TargetProcess
{
    int ProcessId;
};

struct TargetCallback
{
    int Index;
};

struct NewCallback
{
    int Index;
    ULONG64 Pointer;
};

struct DSE
{
    ULONG64 Address;
};

typedef struct _CALLBACK_INFORMATION
{
    CHAR   ModuleName[256];
    ULONG64 Pointer;
} CALLBACK_INFORMATION, * PCALLBACK_INFORMATION;