#ifndef PROCESSES_H
#define PROCESSES_H

#include "..\config.h"

typedef struct {
    DWORD pid;
    char name[260];
    char status[32];
    char cpu[16];
    char memory[32];
    FILETIME prevKernel;
    FILETIME prevUser;
    FILETIME prevSystemKernel;
    FILETIME prevSystemUser;
} ProcessInfo;

ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB);
void GetCpuUsage(DWORD pid, char *cpuBuffer, ProcessInfo *procInfo);
int FindProcessIndex(DWORD pid);
void UpdateProcessList();
void EndSelectedProcess(HWND hListView, HWND hwndParent);

#endif // PROCESSES_H
