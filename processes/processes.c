#include "processes.h"

#define MAX_PROCESSES 1024

static FILETIME prevProcessKernel = {0}, prevProcessUser = {0};
static FILETIME prevSystemKernel = {0}, prevSystemUser = {0};

ProcessInfo processes[MAX_PROCESSES];
int processCount = 0;

ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB) {
    ULARGE_INTEGER a = {.LowPart = ftA.dwLowDateTime, .HighPart = ftA.dwHighDateTime};
    ULARGE_INTEGER b = {.LowPart = ftB.dwLowDateTime, .HighPart = ftB.dwHighDateTime};
    return (b.QuadPart > a.QuadPart) ? (b.QuadPart - a.QuadPart) : 0;
}

void GetCpuUsage(HANDLE hProcess, char *cpuBuffer, ProcessInfo *procInfo) {
    if (!hProcess) {
        snprintf(cpuBuffer, MAX_CPU_LENGTH, "0.0%%");
        return;
    }

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    FILETIME sysIdle, sysKernel, sysUser;

    if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser) ||
        !GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        snprintf(cpuBuffer, MAX_CPU_LENGTH, "0.0%%");
        return;
    }

    ULONGLONG sysTotalDiff = DiffFileTimes(procInfo->prevSystemKernel, sysKernel) +
                             DiffFileTimes(procInfo->prevSystemUser, sysUser);

    ULONGLONG procTotalDiff = DiffFileTimes(procInfo->prevKernel, ftKernel) +
                              DiffFileTimes(procInfo->prevUser, ftUser);

    double cpuPercent = (sysTotalDiff > 0) ? ((double)procTotalDiff / sysTotalDiff) * 100.0 : 0.0;
    cpuPercent = cpuPercent < 0.0 ? 0.0 : (cpuPercent > 100.0 ? 100.0 : cpuPercent);

    snprintf(cpuBuffer, MAX_CPU_LENGTH, "%.1f%%", cpuPercent);

    procInfo->prevKernel = ftKernel;
    procInfo->prevUser = ftUser;
    procInfo->prevSystemKernel = sysKernel;
    procInfo->prevSystemUser = sysUser;
}

void GetMemoryUsage(HANDLE hProcess, char *buffer, size_t bufferSize) {
    if (!hProcess) {
        snprintf(buffer, bufferSize, NA_TEXT);
        return;
    }

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        double workingSetInMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
        snprintf(buffer, bufferSize, (workingSetInMB < 0.1) ? NA_TEXT : "%.1f MB", workingSetInMB);
    } else {
        snprintf(buffer, bufferSize, NA_TEXT);
    }
}

void GetDiskUsage(HANDLE hProcess, char *diskBuffer, ProcessInfo *procInfo) {
    if (!hProcess) {
        snprintf(diskBuffer, MAX_DISK_LENGTH, NA_TEXT);
        return;
    }

    IO_COUNTERS ioCounters;
    if (GetProcessIoCounters(hProcess, &ioCounters)) {
        ULONGLONG readDiff = ioCounters.ReadTransferCount - procInfo->prevReadBytes;
        ULONGLONG writeDiff = ioCounters.WriteTransferCount - procInfo->prevWriteBytes;

        procInfo->prevReadBytes = ioCounters.ReadTransferCount;
        procInfo->prevWriteBytes = ioCounters.WriteTransferCount;

        double mbps = (readDiff + writeDiff) / (1024.0 * 1024.0);
        snprintf(diskBuffer, MAX_DISK_LENGTH, "%.1f MB/s", mbps);
    } else {
        snprintf(diskBuffer, MAX_DISK_LENGTH, NA_TEXT);
    }
}

int FindProcessIndex(DWORD pid) {
    for (int i = 0; i < processCount; ++i) {
        if (processes[i].pid == pid) return i;
    }
    return -1;
}

void GetProcessUser(HANDLE hProcess, char *userBuffer, DWORD bufferSize) {
    strncpy(userBuffer, UNKNOWN_TEXT, bufferSize);

    if (!hProcess) return;

    HANDLE hToken;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        DWORD dwSize;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
        PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);

        if (pTokenUser && GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
            char name[256], domain[256];
            DWORD nameSize = sizeof(name), domainSize = sizeof(domain);
            SID_NAME_USE snu;

            if (LookupAccountSid(NULL, pTokenUser->User.Sid, name, &nameSize, domain, &domainSize, &snu)) {
                strncpy(userBuffer, name, bufferSize);
            }
        }
        free(pTokenUser);
        CloseHandle(hToken);
    }
}

void UpdateProcessInfo(int processIndex, HANDLE hProcess) {
    if (!hProcess) return;

    char user[64] = UNKNOWN_TEXT;
    char pidText[16];

    GetProcessUser(hProcess, user, sizeof(user));
    _itoa(processes[processIndex].pid, pidText, 10);

    // Atualizar somente se diferente
    char currentText[64];

    ListView_GetItemText(hListView, processIndex, 1, currentText, sizeof(currentText));
    if (strcmp(currentText, user) != 0)
        ListView_SetItemText(hListView, processIndex, 1, user);

    ListView_GetItemText(hListView, processIndex, 2, currentText, sizeof(currentText));
    if (strcmp(currentText, pidText) != 0)
        ListView_SetItemText(hListView, processIndex, 2, pidText);

    ListView_GetItemText(hListView, processIndex, 3, currentText, sizeof(currentText));
    if (strcmp(currentText, "Running") != 0)
        ListView_SetItemText(hListView, processIndex, 3, "Running");
}

void UpdateProcessMetrics(int processIndex, HANDLE hProcess) {
    if (!hProcess) return;

    char memBuffer[32], cpuBuffer[32], diskBuffer[32];

    GetCpuUsage(hProcess, cpuBuffer, &processes[processIndex]);
    GetMemoryUsage(hProcess, memBuffer, sizeof(memBuffer));
    GetDiskUsage(hProcess, diskBuffer, &processes[processIndex]);

    // CPU
    if (strcmp(processes[processIndex].cpu, cpuBuffer) != 0) {
        ListView_SetItemText(hListView, processIndex, 4, cpuBuffer);
        strncpy(processes[processIndex].cpu, cpuBuffer, sizeof(processes[processIndex].cpu));
    }

    // Memória
    if (strcmp(processes[processIndex].memory, memBuffer) != 0) {
        ListView_SetItemText(hListView, processIndex, 5, memBuffer);
        strncpy(processes[processIndex].memory, memBuffer, sizeof(processes[processIndex].memory));
    }

    // Disco
    if (strcmp(processes[processIndex].disk, diskBuffer) != 0) {
        ListView_SetItemText(hListView, processIndex, 6, diskBuffer);
        strncpy(processes[processIndex].disk, diskBuffer, sizeof(processes[processIndex].disk));
    }
}

void RemoveNonExistingProcesses(bool processExists[]) {
    for (int i = 0; i < processCount; ++i) {
        if (!processExists[i]) {
            ListView_DeleteItem(hListView, i);
            for (int j = i; j < processCount - 1; ++j) {
                processes[j] = processes[j + 1];
            }
            --processCount;
            --i;
        }
    }
}

void UpdateProcessList() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    LVITEM lvi;
    bool processExists[MAX_PROCESSES] = {false};

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    int index = 0;

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            int processIndex = FindProcessIndex(pe32.th32ProcessID);

            if (processIndex == -1 && processCount < MAX_PROCESSES) {
                lvi.mask = LVIF_TEXT;
                lvi.iItem = index;
                lvi.iSubItem = 0;
                lvi.pszText = pe32.szExeFile;
                ListView_InsertItem(hListView, &lvi);

                processes[processCount].pid = pe32.th32ProcessID;
                strncpy(processes[processCount].name, pe32.szExeFile, sizeof(processes[processCount].name) - 1);
                strncpy(processes[processCount].status, "Running", sizeof(processes[processCount].status) - 1);
                strncpy(processes[processCount].cpu, NA_TEXT, sizeof(processes[processCount].cpu) - 1);
                strncpy(processes[processCount].memory, NA_TEXT, sizeof(processes[processCount].memory) - 1);

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    UpdateProcessInfo(processCount, hProcess);
                    UpdateProcessMetrics(processCount, hProcess);
                    CloseHandle(hProcess);
                } else {
                    char user[64] = UNKNOWN_TEXT, pidText[16];
                    _itoa(pe32.th32ProcessID, pidText, 10);
                    ListView_SetItemText(hListView, processCount, 1, user);
                    ListView_SetItemText(hListView, processCount, 2, pidText);
                    ListView_SetItemText(hListView, processCount, 3, "Access Denied");
                }

                processExists[processCount] = true;
                ++processCount;
            } else {
                processExists[processIndex] = true;

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    UpdateProcessInfo(processIndex, hProcess);
                    UpdateProcessMetrics(processIndex, hProcess);
                    CloseHandle(hProcess);
                }
            }

            ++index;
        } while (Process32Next(hProcessSnap, &pe32));
    }

    RemoveNonExistingProcesses(processExists);

    CloseHandle(hProcessSnap);
}

void EndSelectedProcess(HWND hListView, HWND hwndParent) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        MessageBox(hwndParent, "Select a process to end.", "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    char pidText[16];
    LVITEM item = {0};
    item.iSubItem = 2;
    item.iItem = selectedIndex;
    item.mask = LVIF_TEXT;
    item.pszText = pidText;
    item.cchTextMax = sizeof(pidText);

    if (!ListView_GetItem(hListView, &item)) {
        MessageBox(hwndParent, "Error getting PID.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    DWORD pid = strtoul(pidText, NULL, 10);

    char confirmMessage[256];
    snprintf(confirmMessage, sizeof(confirmMessage), "Do you really want to end the process with PID %u?", pid);
    int response = MessageBox(hwndParent, confirmMessage, "Confirm", MB_YESNO | MB_ICONQUESTION);
    if (response != IDYES) {
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        if (TerminateProcess(hProcess, 0)) {
            ListView_DeleteItem(hListView, selectedIndex);
            MessageBox(hwndParent, "Process completed successfully.", "Success", MB_OK | MB_ICONINFORMATION);
        } else {
            DWORD error = GetLastError();
            char errorMessage[256];
            snprintf(errorMessage, sizeof(errorMessage), "Error completing process. Error code: %lu", error);
            MessageBox(hwndParent, errorMessage, "Error", MB_OK | MB_ICONERROR);
        }
        CloseHandle(hProcess);
    } else {
        DWORD error = GetLastError();
        char errorMessage[256];
        snprintf(errorMessage, sizeof(errorMessage), "Unable to open process. Error code: %lu", error);
        MessageBox(hwndParent, errorMessage, "Error", MB_OK | MB_ICONERROR);
    }
}
