#ifndef PROCESSES_H
#define PROCESSES_H

#include "..\config.h"
#include <stdbool.h>

typedef struct {
    DWORD pid;
    char name[260];
    char status[32];
    char cpu[16];
    char memory[32];
    char disk[16];
    FILETIME prevKernel;
    FILETIME prevUser;
    FILETIME prevSystemKernel;
    FILETIME prevSystemUser;
    ULONGLONG prevReadBytes;
    ULONGLONG prevWriteBytes;
} ProcessInfo;

ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB);
void GetCpuUsage(HANDLE hProcess, char *cpuBuffer, ProcessInfo *procInfo);
void GetMemoryUsage(HANDLE hProcess, char *buffer, size_t bufferSize);
void GetDiskUsage(HANDLE hProcess, char *diskBuffer, ProcessInfo *procInfo);
int FindProcessIndex(DWORD pid);
void GetProcessUser(HANDLE hProcess, char* userBuffer, DWORD bufferSize);
void UpdateProcessInfo(int processIndex, HANDLE hProcess);
void UpdateProcessMetrics(int processIndex, HANDLE hProcess);
void RemoveNonExistingProcesses(bool processExists[]);
void UpdateProcessList();
void EndSelectedProcess(HWND hListView, HWND hwndParent);

#endif // PROCESSES_H
