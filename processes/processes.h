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
    char disk[16]; // Adicionado para armazenar a última medição de disco
    FILETIME prevKernel;
    FILETIME prevUser;
    FILETIME prevSystemKernel;
    FILETIME prevSystemUser;
    ULONGLONG prevReadBytes;  // 🆕 Bytes lidos na última medição
    ULONGLONG prevWriteBytes; // 🆕 Bytes escritos na última medição
} ProcessInfo;


ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB);
void GetCpuUsage(DWORD pid, char *cpuBuffer, ProcessInfo *procInfo);
void GetMemoryUsage(DWORD pid, char *buffer, size_t bufferSize);
void GetDiskUsage(DWORD pid, char *diskBuffer, ProcessInfo *procInfo);
int FindProcessIndex(DWORD pid);
void GetProcessUser(DWORD processID, char* userBuffer, DWORD bufferSize);
void UpdateProcessInfo(int processIndex, PROCESSENTRY32 pe32);
void UpdateProcessMetrics(int processIndex, DWORD processID);
void RemoveNonExistingProcesses(bool processExists[]);
void UpdateProcessList();
void EndSelectedProcess(HWND hListView, HWND hwndParent);

#endif // PROCESSES_H
