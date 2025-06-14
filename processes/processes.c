#include "processes.h"

#define MAX_PROCESSES 1024  // Número máximo de processos a serem monitorados

// Variáveis estáticas para armazenar tempos anteriores de CPU (para cálculo de uso)
static FILETIME prevProcessKernel = {0}, prevProcessUser = {0};
static FILETIME prevSystemKernel = {0}, prevSystemUser = {0};

// Array para armazenar informações dos processos
ProcessInfo processes[MAX_PROCESSES];
int processCount = 0;  // Contador de processos ativos

// Calcula a diferença entre dois FILETIMEs (em 100-nanosegundos)
ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB) {
    ULARGE_INTEGER a = {.LowPart = ftA.dwLowDateTime, .HighPart = ftA.dwHighDateTime};
    ULARGE_INTEGER b = {.LowPart = ftB.dwLowDateTime, .HighPart = ftB.dwHighDateTime};
    return (b.QuadPart > a.QuadPart) ? (b.QuadPart - a.QuadPart) : 0;
}

// Calcula e retorna o uso de CPU de um processo como string formatada
void GetCpuUsage(HANDLE hProcess, char *cpuBuffer, ProcessInfo *procInfo) {
    if (!hProcess) {
        snprintf(cpuBuffer, MAX_CPU_LENGTH, "0.0%%");
        return;
    }

    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    FILETIME sysIdle, sysKernel, sysUser;

    // Obtém tempos do sistema e do processo
    if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser) ||
        !GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        snprintf(cpuBuffer, MAX_CPU_LENGTH, "0.0%%");
        return;
    }

    // Calcula diferenças desde a última medição
    ULONGLONG sysTotalDiff = DiffFileTimes(procInfo->prevSystemKernel, sysKernel) +
                             DiffFileTimes(procInfo->prevSystemUser, sysUser);

    ULONGLONG procTotalDiff = DiffFileTimes(procInfo->prevKernel, ftKernel) +
                              DiffFileTimes(procInfo->prevUser, ftUser);

    // Calcula porcentagem de uso da CPU
    double cpuPercent = (sysTotalDiff > 0) ? ((double)procTotalDiff / sysTotalDiff) * 100.0 : 0.0;
    cpuPercent = cpuPercent < 0.0 ? 0.0 : (cpuPercent > 100.0 ? 100.0 : cpuPercent);

    snprintf(cpuBuffer, MAX_CPU_LENGTH, "%.1f%%", cpuPercent);

    // Atualiza valores anteriores para próxima chamada
    procInfo->prevKernel = ftKernel;
    procInfo->prevUser = ftUser;
    procInfo->prevSystemKernel = sysKernel;
    procInfo->prevSystemUser = sysUser;
}

// Obtém e formata o uso de memória de um processo
void GetMemoryUsage(HANDLE hProcess, char *buffer, size_t bufferSize) {
    if (!hProcess) {
        snprintf(buffer, bufferSize, NA_TEXT);  // "N/A" se processo inválido
        return;
    }

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        // Converte WorkingSetSize para MB
        double workingSetInMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
        snprintf(buffer, bufferSize, (workingSetInMB < 0.1) ? NA_TEXT : "%.1f MB", workingSetInMB);
    } else {
        snprintf(buffer, bufferSize, NA_TEXT);
    }
}

// Calcula e retorna o uso de disco de um processo (MB/s)
void GetDiskUsage(HANDLE hProcess, char *diskBuffer, ProcessInfo *procInfo) {
    if (!hProcess) {
        snprintf(diskBuffer, MAX_DISK_LENGTH, NA_TEXT);
        return;
    }

    IO_COUNTERS ioCounters;
    if (GetProcessIoCounters(hProcess, &ioCounters)) {
        ULONGLONG currentTime = GetTickCount64(); // tempo atual em milissegundos
        ULONGLONG elapsedTime = currentTime - procInfo->lastIoCheckTime;

        // Calcula diferença de bytes lidos/escritos
        ULONGLONG readDiff = ioCounters.ReadTransferCount - procInfo->prevReadBytes;
        ULONGLONG writeDiff = ioCounters.WriteTransferCount - procInfo->prevWriteBytes;

        // Atualiza valores para próxima chamada
        procInfo->prevReadBytes = ioCounters.ReadTransferCount;
        procInfo->prevWriteBytes = ioCounters.WriteTransferCount;
        procInfo->lastIoCheckTime = currentTime;

        // Calcula MB/s
        double seconds = elapsedTime / 1000.0;
        double mbps = (seconds > 0.0) ? ((readDiff + writeDiff) / (1024.0 * 1024.0)) / seconds : 0.0;

        snprintf(diskBuffer, MAX_DISK_LENGTH, "%.1f MB/s", mbps);
    } else {
        snprintf(diskBuffer, MAX_DISK_LENGTH, NA_TEXT);
    }
}

// Encontra o índice de um processo no array pelo PID
int FindProcessIndex(DWORD pid) {
    for (int i = 0; i < processCount; ++i) {
        if (processes[i].pid == pid) return i;
    }
    return -1;  // Não encontrado
}

// Obtém o nome do usuário dono do processo
void GetProcessUser(HANDLE hProcess, char *userBuffer, DWORD bufferSize) {
    strncpy(userBuffer, UNKNOWN_TEXT, bufferSize);

    if (!hProcess) return;

    HANDLE hToken;
    if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        DWORD dwSize;
        // Obtém informações do token do usuário
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
        PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);

        if (pTokenUser && GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
            char name[256], domain[256];
            DWORD nameSize = sizeof(name), domainSize = sizeof(domain);
            SID_NAME_USE snu;

            // Converte SID para nome de usuário
            if (LookupAccountSid(NULL, pTokenUser->User.Sid, name, &nameSize, domain, &domainSize, &snu)) {
                strncpy(userBuffer, name, bufferSize);
            }
        }
        free(pTokenUser);
        CloseHandle(hToken);
    }
}

// Obtém o caminho completo do executável do processo
void GetProcessPath(HANDLE hProcess, char *pathBuffer, DWORD bufferSize) {
    if (!hProcess) {
        strncpy(pathBuffer, UNKNOWN_TEXT, bufferSize);
        return;
    }

    HMODULE hMod;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        GetModuleFileNameEx(hProcess, hMod, pathBuffer, bufferSize);
    } else {
        strncpy(pathBuffer, UNKNOWN_TEXT, bufferSize);
    }
}

// Atualiza informações estáticas do processo na lista
void UpdateProcessInfo(int processIndex, HANDLE hProcess) {
    if (!hProcess) return;

    char user[64] = UNKNOWN_TEXT;
    char pidText[16];
    char processPath[MAX_PATH_LENGTH] = UNKNOWN_TEXT;

    // Obtém informações do processo
    GetProcessUser(hProcess, user, sizeof(user));
    _itoa(processes[processIndex].pid, pidText, 10);
    GetProcessPath(hProcess, processPath, sizeof(processPath)); 

    char currentText[MAX_PATH_LENGTH]; 

    // Atualiza usuário se mudou
    ListView_GetItemText(hListView, processIndex, 1, currentText, sizeof(currentText));
    if (strcmp(currentText, user) != 0)
        ListView_SetItemText(hListView, processIndex, 1, user);

    // Atualiza PID se mudou (não deveria)
    ListView_GetItemText(hListView, processIndex, 2, currentText, sizeof(currentText));
    if (strcmp(currentText, pidText) != 0)
        ListView_SetItemText(hListView, processIndex, 2, pidText);

    // Atualiza status para "Running"
    ListView_GetItemText(hListView, processIndex, 3, currentText, sizeof(currentText));
    if (strcmp(currentText, "Running") != 0)
        ListView_SetItemText(hListView, processIndex, 3, "Running");

    // Atualiza caminho se mudou
    ListView_GetItemText(hListView, processIndex, 7, currentText, sizeof(currentText));
    if (strcmp(currentText, processPath) != 0) {
        ListView_SetItemText(hListView, processIndex, 7, processPath);
        strncpy(processes[processIndex].path, processPath, sizeof(processes[processIndex].path) - 1);
    }
}

// Atualiza métricas dinâmicas do processo (CPU, memória, disco)
void UpdateProcessMetrics(int processIndex, HANDLE hProcess) {
    if (!hProcess) return;

    char memBuffer[32], cpuBuffer[32], diskBuffer[32];

    // Obtém métricas atuais
    GetCpuUsage(hProcess, cpuBuffer, &processes[processIndex]);
    GetMemoryUsage(hProcess, memBuffer, sizeof(memBuffer));
    GetDiskUsage(hProcess, diskBuffer, &processes[processIndex]);

    // Atualiza CPU se mudou
    if (strcmp(processes[processIndex].cpu, cpuBuffer) != 0) {
        ListView_SetItemText(hListView, processIndex, 4, cpuBuffer);
        strncpy(processes[processIndex].cpu, cpuBuffer, sizeof(processes[processIndex].cpu));
    }

    // Atualiza memória se mudou
    if (strcmp(processes[processIndex].memory, memBuffer) != 0) {
        ListView_SetItemText(hListView, processIndex, 5, memBuffer);
        strncpy(processes[processIndex].memory, memBuffer, sizeof(processes[processIndex].memory));
    }

    // Atualiza disco se mudou
    if (strcmp(processes[processIndex].disk, diskBuffer) != 0) {
        ListView_SetItemText(hListView, processIndex, 6, diskBuffer);
        strncpy(processes[processIndex].disk, diskBuffer, sizeof(processes[processIndex].disk));
    }
}

// Remove processos que não existem mais
void RemoveNonExistingProcesses(bool processExists[]) {
    for (int i = 0; i < processCount; ++i) {
        if (!processExists[i]) {
            // Remove da ListView
            ListView_DeleteItem(hListView, i);
            
            // Remove do array de processos
            for (int j = i; j < processCount - 1; ++j) {
                processes[j] = processes[j + 1];
            }
            
            --processCount;
            --i;  // Reavalia a nova posição i
        }
    }
}

// Atualiza a lista de processos (principal função do módulo)
void UpdateProcessList() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    LVITEM lvi;
    bool processExists[MAX_PROCESSES] = {false};  // Marca processos existentes

    // Cria snapshot dos processos atuais
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    int index = 0;

    // Itera por todos os processos no snapshot
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            int processIndex = FindProcessIndex(pe32.th32ProcessID);

            // Se é um novo processo
            if (processIndex == -1 && processCount < MAX_PROCESSES) {
                // Adiciona novo item na ListView
                lvi.mask = LVIF_TEXT;
                lvi.iItem = index;
                lvi.iSubItem = 0;
                lvi.pszText = pe32.szExeFile;
                ListView_InsertItem(hListView, &lvi);

                // Inicializa estrutura do processo
                processes[processCount].pid = pe32.th32ProcessID;
                strncpy(processes[processCount].name, pe32.szExeFile, sizeof(processes[processCount].name) - 1);
                strncpy(processes[processCount].status, "Running", sizeof(processes[processCount].status) - 1);
                strncpy(processes[processCount].cpu, NA_TEXT, sizeof(processes[processCount].cpu) - 1);
                strncpy(processes[processCount].memory, NA_TEXT, sizeof(processes[processCount].memory) - 1);
                strncpy(processes[processCount].disk, NA_TEXT, sizeof(processes[processCount].disk) - 1);
                strncpy(processes[processCount].path, UNKNOWN_TEXT, sizeof(processes[processCount].path) - 1);

                // Tenta abrir o processo para obter mais informações
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    UpdateProcessInfo(processCount, hProcess);
                    UpdateProcessMetrics(processCount, hProcess);
                    CloseHandle(hProcess);
                } else {
                    // Se não tem permissão, preenche com valores padrão
                    char user[64] = UNKNOWN_TEXT, pidText[16], processPath[MAX_PATH_LENGTH] = UNKNOWN_TEXT;
                    _itoa(pe32.th32ProcessID, pidText, 10);
                    ListView_SetItemText(hListView, processCount, 1, user);
                    ListView_SetItemText(hListView, processCount, 2, pidText);
                    ListView_SetItemText(hListView, processCount, 3, "Access Denied");
                    ListView_SetItemText(hListView, processCount, 7, processPath);
                }

                processExists[processCount] = true;
                ++processCount;
            } else {
                // Processo já existente - atualiza informações
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

    // Remove processos que não existem mais
    RemoveNonExistingProcesses(processExists);

    CloseHandle(hProcessSnap);
}

// Finaliza um processo selecionado pelo usuário
void EndSelectedProcess(HWND hListView, HWND hwndParent) {
    // Obtém índice do item selecionado
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        MessageBox(hwndParent, "Select a process to end.", "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    // Obtém PID do processo selecionado
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

    // Confirmação do usuário
    char confirmMessage[256];
    snprintf(confirmMessage, sizeof(confirmMessage), "Do you really want to end the process with PID %u?", pid);
    int response = MessageBox(hwndParent, confirmMessage, "Confirm", MB_YESNO | MB_ICONQUESTION);
    if (response != IDYES) {
        return;
    }

    // Tenta finalizar o processo
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        if (TerminateProcess(hProcess, 0)) {
            // Remove da lista se bem-sucedido
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