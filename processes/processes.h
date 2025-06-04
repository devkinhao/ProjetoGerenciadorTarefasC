#ifndef PROCESSES_H
#define PROCESSES_H

#include "..\config.h"

// Definições de tamanho de buffer para garantir consistência no código
#define MAX_NAME_LENGTH 260
#define MAX_STATUS_LENGTH 32
#define MAX_CPU_LENGTH 16
#define MAX_MEMORY_LENGTH 32
#define MAX_DISK_LENGTH 16
#define UNKNOWN_TEXT "Unknown"
#define NA_TEXT "N/A"

typedef struct {
    DWORD pid;
    char name[MAX_NAME_LENGTH];
    char status[MAX_STATUS_LENGTH];
    char cpu[MAX_CPU_LENGTH];
    char memory[MAX_MEMORY_LENGTH];
    char disk[MAX_DISK_LENGTH];
    FILETIME prevKernel;
    FILETIME prevUser;
    FILETIME prevSystemKernel;
    FILETIME prevSystemUser;
    ULONGLONG prevReadBytes;
    ULONGLONG prevWriteBytes;
    ULONGLONG lastIoCheckTime;
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


