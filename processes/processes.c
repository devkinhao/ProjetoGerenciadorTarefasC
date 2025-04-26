#include "processes.h"

static FILETIME prevProcessKernel = {0}, prevProcessUser = {0};
static FILETIME prevSystemKernel = {0}, prevSystemUser = {0};

#define MAX_PROCESSES 1024

ProcessInfo processes[MAX_PROCESSES];
int processCount = 0;

ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB) {
    ULARGE_INTEGER a, b;
    a.LowPart = ftA.dwLowDateTime;
    a.HighPart = ftA.dwHighDateTime;
    b.LowPart = ftB.dwLowDateTime;
    b.HighPart = ftB.dwHighDateTime;
    return (b.QuadPart > a.QuadPart) ? (b.QuadPart - a.QuadPart) : 0;
}

void GetCpuUsage(HANDLE hProcess, char *cpuBuffer, ProcessInfo *procInfo) {
    if (hProcess == NULL) {
        strcpy(cpuBuffer, "0.0%");
        return;
    }

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    FILETIME sysIdle, sysKernel, sysUser;

    if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser)) {
        strcpy(cpuBuffer, "0.0%");
        return;
    }

    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        strcpy(cpuBuffer, "0.0%");
        return;
    }

    ULONGLONG sysKernelDiff = DiffFileTimes(procInfo->prevSystemKernel, sysKernel);
    ULONGLONG sysUserDiff = DiffFileTimes(procInfo->prevSystemUser, sysUser);
    ULONGLONG sysTotalDiff = sysKernelDiff + sysUserDiff;

    ULONGLONG procKernelDiff = DiffFileTimes(procInfo->prevKernel, ftKernel);
    ULONGLONG procUserDiff = DiffFileTimes(procInfo->prevUser, ftUser);
    ULONGLONG procTotalDiff = procKernelDiff + procUserDiff;

    double cpuPercent = 0.0;
    if (sysTotalDiff > 0) {
        cpuPercent = ((double)procTotalDiff / sysTotalDiff) * 100.0;
    }

    if (cpuPercent < 0.0) cpuPercent = 0.0;
    if (cpuPercent > 100.0) cpuPercent = 100.0;

    sprintf(cpuBuffer, "%.1f%%", cpuPercent);

    memcpy(&procInfo->prevKernel, &ftKernel, sizeof(FILETIME));
    memcpy(&procInfo->prevUser, &ftUser, sizeof(FILETIME));
    memcpy(&procInfo->prevSystemKernel, &sysKernel, sizeof(FILETIME));
    memcpy(&procInfo->prevSystemUser, &sysUser, sizeof(FILETIME));
}

void GetMemoryUsage(HANDLE hProcess, char *buffer, size_t bufferSize) {
    if (hProcess != NULL) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            double workingSetInMB = (double)pmc.WorkingSetSize / (1024 * 1024);

            if (workingSetInMB < 0.1) {
                snprintf(buffer, bufferSize, "N/A");
            } else {
                snprintf(buffer, bufferSize, "%.1f MB", workingSetInMB);
            }
        } else {
            snprintf(buffer, bufferSize, "N/A");
        }
    } else {
        snprintf(buffer, bufferSize, "N/A");
    }
}

void GetDiskUsage(HANDLE hProcess, char *diskBuffer, ProcessInfo *procInfo) {
    if (hProcess == NULL) {
        strcpy(diskBuffer, "N/A");
        return;
    }

    IO_COUNTERS ioCounters;
    if (GetProcessIoCounters(hProcess, &ioCounters)) {
        ULONGLONG readDiff = ioCounters.ReadTransferCount - procInfo->prevReadBytes;
        ULONGLONG writeDiff = ioCounters.WriteTransferCount - procInfo->prevWriteBytes;

        procInfo->prevReadBytes = ioCounters.ReadTransferCount;
        procInfo->prevWriteBytes = ioCounters.WriteTransferCount;

        double readMBps = (double)readDiff / (1024.0 * 1024.0);
        double writeMBps = (double)writeDiff / (1024.0 * 1024.0);

        sprintf(diskBuffer, "%.1f MB/s", readMBps + writeMBps);
    } else {
        strcpy(diskBuffer, "N/A");
    }
}

int FindProcessIndex(DWORD pid) {
    for (int i = 0; i < processCount; i++) {
        if (processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

void GetProcessUser(HANDLE hProcess, char* userBuffer, DWORD bufferSize) {
    if (hProcess == NULL) {
        strcpy(userBuffer, "Unknown");
        return;
    }

    HANDLE hToken;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        DWORD dwSize;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
        PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
        if (pTokenUser != NULL) {
            if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
                SID_NAME_USE snu;
                char name[256];
                DWORD nameSize = 256;
                char domain[256];
                DWORD domainSize = 256;

                if (LookupAccountSid(NULL, pTokenUser->User.Sid, name, &nameSize, domain, &domainSize, &snu)) {
                    snprintf(userBuffer, bufferSize, "%s", name);
                } else {
                    strcpy(userBuffer, "Unknown");
                }
            }
            free(pTokenUser);
        }
        CloseHandle(hToken);
    } else {
        strcpy(userBuffer, "Unknown");
    }
}

void UpdateProcessInfo(int processIndex, HANDLE hProcess) {
    char user[64] = "Unknown";
    char pidText[16];

    GetProcessUser(hProcess, user, sizeof(user));
    _itoa(processes[processIndex].pid, pidText, 10);

    ListView_SetItemText(hListView, processIndex, 1, user);
    ListView_SetItemText(hListView, processIndex, 2, pidText);
    ListView_SetItemText(hListView, processIndex, 3, "Running");
}

void UpdateProcessMetrics(int processIndex, HANDLE hProcess) {
    char memBuffer[32], cpuBuffer[32], diskBuffer[32];

    GetMemoryUsage(hProcess, memBuffer, sizeof(memBuffer));
    GetCpuUsage(hProcess, cpuBuffer, &processes[processIndex]);
    GetDiskUsage(hProcess, diskBuffer, &processes[processIndex]);

    ListView_SetItemText(hListView, processIndex, 5, memBuffer);
    ListView_SetItemText(hListView, processIndex, 4, cpuBuffer);
    ListView_SetItemText(hListView, processIndex, 6, diskBuffer);

    strcpy(processes[processIndex].memory, memBuffer);
    strcpy(processes[processIndex].cpu, cpuBuffer);
    strcpy(processes[processIndex].disk, diskBuffer);
}

void RemoveNonExistingProcesses(bool processExists[]) {
    for (int i = 0; i < processCount; i++) {
        if (!processExists[i]) {
            ListView_DeleteItem(hListView, i);
            for (int j = i; j < processCount - 1; j++) {
                processes[j] = processes[j + 1];
            }
            processCount--;
            i--;
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

            if (processIndex == -1) {
                if (processCount >= MAX_PROCESSES) {
                    break;
                }

                lvi.mask = LVIF_TEXT;
                lvi.iItem = index;
                lvi.iSubItem = 0;
                lvi.pszText = pe32.szExeFile;
                ListView_InsertItem(hListView, &lvi);

                processes[processCount].pid = pe32.th32ProcessID;
                strcpy(processes[processCount].name, pe32.szExeFile);
                strcpy(processes[processCount].status, "Running");
                strcpy(processes[processCount].cpu, "N/A");
                strcpy(processes[processCount].memory, "N/A");

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    UpdateProcessInfo(processCount, hProcess);
                    UpdateProcessMetrics(processCount, hProcess);
                    CloseHandle(hProcess);
                } else {
                    char user[64] = "Unknown", pidText[16];
                    _itoa(pe32.th32ProcessID, pidText, 10);
                    ListView_SetItemText(hListView, processCount, 1, user);
                    ListView_SetItemText(hListView, processCount, 2, pidText);
                    ListView_SetItemText(hListView, processCount, 3, "Access Denied");
                }

                processExists[processCount] = true;
                processCount++;
            } else {
                processExists[processIndex] = true;

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    UpdateProcessInfo(processIndex, hProcess);
                    UpdateProcessMetrics(processIndex, hProcess);
                    CloseHandle(hProcess);
                }
            }

            index++;
        } while (Process32Next(hProcessSnap, &pe32));
    }

    RemoveNonExistingProcesses(processExists);

    CloseHandle(hProcessSnap);
}

void EndSelectedProcess(HWND hListView, HWND hwndParent) {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1)
    {
        MessageBox(hwndParent, "Selecione um processo para finalizar.", "Aviso", MB_OK | MB_ICONWARNING);
        return;
    }

    char pidText[16];
    LVITEM item = {0};
    item.iSubItem = 2;
    item.iItem = selectedIndex;
    item.mask = LVIF_TEXT;
    item.pszText = pidText;
    item.cchTextMax = sizeof(pidText);

    if (!ListView_GetItem(hListView, &item))
    {
        MessageBox(hwndParent, "Erro ao obter o PID.", "Erro", MB_OK | MB_ICONERROR);
        return;
    }

    DWORD pid = strtoul(pidText, NULL, 10);

    char confirmMessage[256];
    snprintf(confirmMessage, sizeof(confirmMessage), "Deseja realmente finalizar o processo com PID %u?", pid);
    int response = MessageBox(hwndParent, confirmMessage, "Confirmar", MB_YESNO | MB_ICONQUESTION);
    if (response != IDYES)
    {
        return;
    }

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (TerminateProcess(hProcess, 0))
    {
        // Remover o item da lista após finalizar o processo
        ListView_DeleteItem(hListView, selectedIndex);

        MessageBox(hwndParent, "Processo finalizado com sucesso.", "Sucesso", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        DWORD error = GetLastError();
        char errorMessage[256];
        snprintf(errorMessage, sizeof(errorMessage), "Erro ao finalizar o processo. Código de erro: %lu", error);
        MessageBox(hwndParent, errorMessage, "Erro", MB_OK | MB_ICONERROR);
    }
        CloseHandle(hProcess);  
}
