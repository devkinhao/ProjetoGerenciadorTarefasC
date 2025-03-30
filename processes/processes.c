#include "processes.h"

static FILETIME prevProcessKernel = {0}, prevProcessUser = {0};
static FILETIME prevSystemKernel = {0}, prevSystemUser = {0};

#define MAX_PROCESSES 1024

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

void GetCpuUsage(DWORD pid, char *cpuBuffer, ProcessInfo *procInfo) {
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

void GetMemoryUsage(DWORD pid, char *buffer, size_t bufferSize) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            // Memória física usada (Working Set Size)
            double workingSetInMB = (double)pmc.WorkingSetSize / (1024 * 1024);

            // Exibe a memória física em MB
            if (workingSetInMB < 0.1) {
                snprintf(buffer, bufferSize, "N/A");
            } else {
                snprintf(buffer, bufferSize, "%.1f MB", workingSetInMB);
            }
        } else {
            snprintf(buffer, bufferSize, "N/A");
        }
        CloseHandle(hProcess);
    } else {
        snprintf(buffer, bufferSize, "N/A");
    }
}

void GetDiskUsage(DWORD pid, char *diskBuffer, ProcessInfo *procInfo) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        strcpy(diskBuffer, "N/A");
        return;
    }

    IO_COUNTERS ioCounters;
    if (GetProcessIoCounters(hProcess, &ioCounters)) {
        // Calcula a diferença desde a última coleta
        ULONGLONG readDiff = ioCounters.ReadTransferCount - procInfo->prevReadBytes;
        ULONGLONG writeDiff = ioCounters.WriteTransferCount - procInfo->prevWriteBytes;

        // Atualiza os valores anteriores
        procInfo->prevReadBytes = ioCounters.ReadTransferCount;
        procInfo->prevWriteBytes = ioCounters.WriteTransferCount;

        // Converte para MB/s
        double readMBps = (double)readDiff / (1024.0 * 1024.0);
        double writeMBps = (double)writeDiff / (1024.0 * 1024.0);

        // Formata a string final
        sprintf(diskBuffer, "%.1f MB/s", readMBps + writeMBps);
    } else {
        strcpy(diskBuffer, "N/A");
    }

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

void GetProcessUser(DWORD processID, char* userBuffer, DWORD bufferSize) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (hProcess == NULL) {
        strcpy(userBuffer, "Unknown");
        return;
    }

    HANDLE hToken;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        DWORD dwSize;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
        PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
        if (GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
            SID_NAME_USE snu;
            char name[256];
            DWORD nameSize = 256;
            char domain[256];
            DWORD domainSize = 256;

            // Tenta obter o nome de usuário e o nome do domínio
            if (LookupAccountSid(NULL, pTokenUser->User.Sid, name, &nameSize, domain, &domainSize, &snu)) {
                // Apenas exibe o nome do usuário, sem o domínio
                snprintf(userBuffer, bufferSize, "%s", name);
            } else {
                strcpy(userBuffer, "Unknown");
            }
        }
        free(pTokenUser);
        CloseHandle(hToken);
    }

    CloseHandle(hProcess);
}

void RemoveNonExistingProcesses(bool processExists[]) {
    for (int i = 0; i < processCount; i++) {
        if (!processExists[i]) {
            ListView_DeleteItem(hListView, i);
            for (int j = i; j < processCount - 1; j++) {
                processes[j] = processes[j + 1];
            }
            processCount--;
            i--; // Ajustar índice após remoção
        }
    }
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
    bool processExists[MAX_PROCESSES] = {false}; // Array para rastrear processos ativos

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            int processIndex = FindProcessIndex(pe32.th32ProcessID);

            if (processIndex == -1) {
                // Novo processo, adiciona ao ListView
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

                // Adiciona User
                char user[64];
                GetProcessUser(pe32.th32ProcessID, user, sizeof(user)); // Implemente a função GetProcessUser
                ListView_SetItemText(hListView, index, 1, user);

                // Adiciona PID, STATUS, CPU, MEMÓRIA e DISCO
                char pidText[16];
                _itoa(pe32.th32ProcessID, pidText, 10);
                ListView_SetItemText(hListView, index, 2, pidText);
                ListView_SetItemText(hListView, index, 3, processes[processCount].status);
                ListView_SetItemText(hListView, index, 4, processes[processCount].cpu);

                // Obtém o uso de memória
                char memBuffer[32];
                GetMemoryUsage(pe32.th32ProcessID, memBuffer, sizeof(memBuffer));
                ListView_SetItemText(hListView, index, 5, memBuffer);
                strcpy(processes[processCount].memory, memBuffer);

                // Obtém o uso de disco
                char diskBuffer[32];
                GetDiskUsage(pe32.th32ProcessID, diskBuffer, &processes[processCount]);
                ListView_SetItemText(hListView, index, 6, diskBuffer);

                processExists[processCount] = true;
                processCount++;
            } else {
                // Processo existente, atualizar informações
                processExists[processIndex] = true;
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    // Atualizar uso de memória
                    char memBuffer[32];
                    GetMemoryUsage(pe32.th32ProcessID, memBuffer, sizeof(memBuffer));
                    ListView_SetItemText(hListView, processIndex, 5, memBuffer);
                    strcpy(processes[processIndex].memory, memBuffer);

                    // Atualizar uso de CPU
                    GetCpuUsage(pe32.th32ProcessID, processes[processIndex].cpu, &processes[processIndex]);
                    ListView_SetItemText(hListView, processIndex, 4, processes[processIndex].cpu);

                    // Atualizar uso de Disco
                    char diskBuffer[16];
                    GetDiskUsage(pe32.th32ProcessID, diskBuffer, &processes[processIndex]);
                    strcpy(processes[processIndex].disk, diskBuffer);
                    ListView_SetItemText(hListView, processIndex, 6, diskBuffer);

                    CloseHandle(hProcess);
                }

                // Atualizar PID e status
                char pidText[16];
                _itoa(pe32.th32ProcessID, pidText, 10);
                ListView_SetItemText(hListView, processIndex, 2, pidText);
                strcpy(processes[processIndex].status, "Running");
                ListView_SetItemText(hListView, processIndex, 3, processes[processIndex].status);
            }

            index++;
        } while (Process32Next(hProcessSnap, &pe32));
    }

    // Remover processos que não existem mais
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
