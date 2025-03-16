#include "processes.h"

static FILETIME prevProcessKernel = {0}, prevProcessUser = {0};
static FILETIME prevSystemKernel = {0}, prevSystemUser = {0};

ProcessInfo processes[1024];
int processCount = 0;

ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB)
{
    ULARGE_INTEGER a, b;
    a.LowPart = ftA.dwLowDateTime;
    a.HighPart = ftA.dwHighDateTime;
    b.LowPart = ftB.dwLowDateTime;
    b.HighPart = ftB.dwHighDateTime;
    return (b.QuadPart > a.QuadPart) ? (b.QuadPart - a.QuadPart) : 0;
}

void GetCpuUsage(DWORD pid, char *cpuBuffer, ProcessInfo *procInfo)
{
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    FILETIME sysIdle, sysKernel, sysUser;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL)
    {
        strcpy(cpuBuffer, "0.0");
        return;
    }

    // Obtém o tempo total do sistema (para cálculo da base)
    if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser))
    {
        CloseHandle(hProcess);
        strcpy(cpuBuffer, "0.0");
        return;
    }

    // Obtém os tempos do processo
    if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser))
    {
        CloseHandle(hProcess);
        strcpy(cpuBuffer, "0.0");
        return;
    }

    ULONGLONG sysKernelDiff = DiffFileTimes(procInfo->prevSystemKernel, sysKernel);
    ULONGLONG sysUserDiff = DiffFileTimes(procInfo->prevSystemUser, sysUser);
    ULONGLONG sysTotalDiff = sysKernelDiff + sysUserDiff;

    ULONGLONG procKernelDiff = DiffFileTimes(procInfo->prevKernel, ftKernel);
    ULONGLONG procUserDiff = DiffFileTimes(procInfo->prevUser, ftUser);
    ULONGLONG procTotalDiff = procKernelDiff + procUserDiff;

    double cpuPercent = 0.0;
    if (sysTotalDiff > 0)
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        int numProcessors = sysInfo.dwNumberOfProcessors;

        cpuPercent = (double)(procTotalDiff * 100.0) / (double)sysTotalDiff;
        // cpuPercent /= numProcessors; // Ajusta pelo número de CPUs
        if (cpuPercent < 0.0)
            cpuPercent = 0.0;
        if (cpuPercent > 100.0)
            cpuPercent = 100.0; // Limita a 100%
    }

    sprintf(cpuBuffer, "%.1f%%", cpuPercent);

    // Atualiza os valores anteriores para a próxima medição
    memcpy(&procInfo->prevKernel, &ftKernel, sizeof(FILETIME));
    memcpy(&procInfo->prevUser, &ftUser, sizeof(FILETIME));
    memcpy(&procInfo->prevSystemKernel, &sysKernel, sizeof(FILETIME));
    memcpy(&procInfo->prevSystemUser, &sysUser, sizeof(FILETIME));

    CloseHandle(hProcess);
}

int FindProcessIndex(DWORD pid) {
    for (int i = 0; i < processCount; i++) {
        if (processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

void UpdateProcessList() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    LVITEM lvi;

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
                // New process, add to ListView
                lvi.mask = LVIF_TEXT;
                lvi.iItem = index;
                lvi.iSubItem = 0;
                lvi.pszText = pe32.szExeFile;
                ListView_InsertItem(hListView, &lvi);

                processes[processCount].pid = pe32.th32ProcessID;
                strcpy(processes[processCount].name, pe32.szExeFile);
                strcpy(processes[processCount].status, "Running");
                strcpy(processes[processCount].cpu, "0.0");
                strcpy(processes[processCount].memory, "N/A");

                // Set columns (PID, STATUS, etc.)
                char pidText[16];
                _itoa(pe32.th32ProcessID, pidText, 10);
                ListView_SetItemText(hListView, index, 1, pidText);
                ListView_SetItemText(hListView, index, 2, processes[processCount].status);
                ListView_SetItemText(hListView, index, 3, processes[processCount].cpu);
                ListView_SetItemText(hListView, index, 4, processes[processCount].memory);
                ListView_SetItemText(hListView, index, 5, "N/A"); // Placeholder for GPU

                processCount++;
            } else {
                // Existing process, update information
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    // Update memory usage
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                        char memBuffer[32];
                        unsigned long memoryInKB = pmc.WorkingSetSize / 1024;
                        if (memoryInKB == 0) {
                            strcpy(memBuffer, "N/A");
                        } else {
                            sprintf(memBuffer, "%lu K", memoryInKB);
                        }
                        ListView_SetItemText(hListView, processIndex, 4, memBuffer);
                        strcpy(processes[processIndex].memory, memBuffer);
                    }

                    // Update CPU usage using historical FILETIME
                    GetCpuUsage(pe32.th32ProcessID, processes[processIndex].cpu, &processes[processIndex]);
                    ListView_SetItemText(hListView, processIndex, 3, processes[processIndex].cpu);

                    // Placeholder for GPU
                    ListView_SetItemText(hListView, processIndex, 5, "N/A");

                    CloseHandle(hProcess);
                }

                // Update PID and status columns
                char pidText[16];
                _itoa(pe32.th32ProcessID, pidText, 10);
                ListView_SetItemText(hListView, processIndex, 1, pidText);
                strcpy(processes[processIndex].status, "Running");
                ListView_SetItemText(hListView, processIndex, 2, processes[processIndex].status);
            }

            index++;
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
}

void EndSelectedProcess(HWND hListView, HWND hwndParent)
{
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1)
    {
        MessageBox(hwndParent, "Selecione um processo para finalizar.", "Aviso", MB_OK | MB_ICONWARNING);
        return;
    }

    char pidText[16];
    LVITEM item = {0};
    item.iSubItem = 1;
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
