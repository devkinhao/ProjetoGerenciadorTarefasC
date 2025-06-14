#include "hardware.h"
#include "../utils/utils.h"

// Definições de constantes para layout da interface
#define SECTION_SPACING 15          // Espaçamento entre seções
#define GROUPBOX_HEIGHT 30          // Altura padrão dos GroupBoxes
#define ITEM_SPACING 25             // Espaçamento entre itens
#define LEFT_COLUMN_WIDTH 350       // Largura da coluna esquerda
#define RIGHT_COLUMN_WIDTH 350      // Largura da coluna direita
#define GROUPBOX_TOP_MARGIN 25      // Margem superior dentro dos GroupBoxes

// Variáveis externas definidas em main.c
extern int gWindowWidth;            // Largura da janela principal
extern int gWindowHeight;           // Altura da janela principal
extern HBRUSH hbrBackground;        // Pincel para o fundo
extern COLORREF clrBackground;      // Cor de fundo

// Ponteiro para o procedimento original da janela do painel de hardware
static WNDPROC g_lpfnOriginalHardwarePanelProc;

// Estrutura para armazenar os controles da seção de CPU
typedef struct {
    HWND hGroup;      // Grupo da CPU
    HWND hName;       // Nome do processador
    HWND hCores;      // Número de núcleos
    HWND hUsage;      // Uso da CPU
    HWND hSpeed;      // Velocidade do processador
    HWND hProcesses;  // Número de processos
    HWND hThreads;    // Número de threads
    HWND hHandles;    // Número de handles
    HWND hCacheL1;    // Cache L1
    HWND hCacheL2;    // Cache L2
    HWND hCacheL3;    // Cache L3
} CpuControls;

// Variáveis estáticas para controles de hardware
static CpuControls cpuControls;
HWND hRamGroup, hDiskGroup, hOsGroup, hGpuGroup, hBatteryGroup, hSystemGroup, hNetworkGroup;
HWND hLabelRam, hLabelDisk, hLabelOs, hLabelGpu, hLabelBattery, hLabelSystem, hLabelUptime, hLabelNetwork;

// Variáveis para cálculo de uso da CPU
static FILETIME prevIdleTime = {0}, prevKernelTime = {0}, prevUserTime = {0};

// Variáveis para cálculo de uso de rede
static DWORD64 prevIn = 0, prevOut = 0;
static ULONGLONG prevTime = 0;

// Função para verificar se o sistema operacional é 64-bit
BOOL Is64BitWindows() {
#if defined(_WIN64)
    return TRUE;  // Compilado como 64-bit
#elif defined(_WIN32)
    BOOL bIsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &bIsWow64);  // Verifica se processo está rodando em modo WOW64
    return bIsWow64;
#else
    return FALSE;  // Não é Windows
#endif
}

// Converte FILETIME para ULONGLONG (unsigned long long)
ULONGLONG FileTimeToULL(FILETIME ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

// Cria um rótulo (label) na interface
HWND CreateLabel(HWND parent, const char* text, int x, int y, int width, int height, DWORD alignStyle) {
    // Cria a janela do rótulo
    HWND hLabel = CreateWindowEx(0, "STATIC", text,
        WS_CHILD | WS_VISIBLE | alignStyle,
        x, y, width, height, 
        parent, NULL, GetModuleHandle(NULL), NULL);

    // Cria e aplica a fonte personalizada
    HFONT hFont = CreateFontForControl();
    SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hLabel;
}

// Cria um grupo (GroupBox) na interface
HWND CreateGroupBox(HWND parent, const char* title, int x, int y, int width, int height) {
    // Cria a janela do GroupBox
    HWND hGroupBox = CreateWindowEx(0, "BUTTON", title,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 
        x, y, width, height, 
        parent, NULL, GetModuleHandle(NULL), NULL);

    // Cria e aplica a fonte personalizada
    HFONT hFont = CreateFontForControl();
    SendMessage(hGroupBox, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hGroupBox;
}

// Calcula o uso da CPU em porcentagem
double CalculateCpuUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0;  // Falha ao obter tempos do sistema
    }

    // Calcula diferenças desde a última medição
    ULONGLONG idleDiff = FileTimeToULL(idleTime) - FileTimeToULL(prevIdleTime);
    ULONGLONG kernelDiff = FileTimeToULL(kernelTime) - FileTimeToULL(prevKernelTime);
    ULONGLONG userDiff = FileTimeToULL(userTime) - FileTimeToULL(prevUserTime);

    ULONGLONG totalSystem = kernelDiff + userDiff;  // Tempo total do sistema

    // Atualiza valores anteriores
    prevIdleTime = idleTime;
    prevKernelTime = kernelTime;
    prevUserTime = userTime;

    if (totalSystem == 0) return 0.0;  // Evita divisão por zero

    // Retorna porcentagem de uso (tempo não ocioso / tempo total)
    return (double)(totalSystem - idleDiff) * 100.0 / totalSystem;
}

// Atualiza as informações da CPU na interface
void UpdateCpuInfo() {
    char buffer[256];
    HKEY hKeyProcessor;
    char cpuName[256] = "Unknown CPU";
    DWORD cpuMHz = 0;
    
    // Obtém informações do processador do registro do Windows
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKeyProcessor) == ERROR_SUCCESS) {
        DWORD size = sizeof(cpuName);
        RegQueryValueEx(hKeyProcessor, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &size);
        size = sizeof(cpuMHz);
        RegQueryValueEx(hKeyProcessor, "~MHz", NULL, NULL, (LPBYTE)&cpuMHz, &size);
        RegCloseKey(hKeyProcessor);
    }

    // Calcula uso e velocidade do processador
    double cpuUsage = CalculateCpuUsage();
    double clockSpeed = (double)cpuMHz / 1000.0;

    // Atualiza os controles da interface
    SafeSetWindowText(cpuControls.hName, cpuName);
    snprintf(buffer, sizeof(buffer), "Logical Processors: %d", sysInfo.dwNumberOfProcessors);
    SafeSetWindowText(cpuControls.hCores, buffer);
    snprintf(buffer, sizeof(buffer), "Usage: %d%%", (int)cpuUsage);
    SafeSetWindowText(cpuControls.hUsage, buffer);
    snprintf(buffer, sizeof(buffer), "Base speed: %.2f GHz", clockSpeed);
    SafeSetWindowText(cpuControls.hSpeed, buffer);

    // Obtém informações de processos, threads e handles
    DWORD processCount = 0;
    DWORD threadCount = 0;
    DWORD handleCount = 0;

    DWORD processes[1024], cbNeeded, cProcesses;
    if (EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        cProcesses = cbNeeded / sizeof(DWORD);
        processCount = cProcesses;

        // Conta handles abertos por cada processo
        for (DWORD i = 0; i < cProcesses; i++) {
            if (processes[i] != 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processes[i]);
                if (hProcess) {
                    DWORD hCount = 0;
                    if (GetProcessHandleCount(hProcess, &hCount)) {
                        handleCount += hCount;
                    }
                    CloseHandle(hProcess);
                }
            }
        }

        // Cria snapshot de threads para contagem
        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32);

            if (Thread32First(hThreadSnap, &te32)) {
                do {
                    threadCount++;
                } while (Thread32Next(hThreadSnap, &te32));
            }
            CloseHandle(hThreadSnap);
        }
    }

    // Atualiza contagens na interface
    snprintf(buffer, sizeof(buffer), "Processes: %lu", processCount);
    SafeSetWindowText(cpuControls.hProcesses, buffer);
    snprintf(buffer, sizeof(buffer), "Threads: %lu", threadCount);
    SafeSetWindowText(cpuControls.hThreads, buffer);
    snprintf(buffer, sizeof(buffer), "Handles: %lu", handleCount);
    SafeSetWindowText(cpuControls.hHandles, buffer);

    // Obtém informações de cache do processador
    DWORD len = 0;
    GetLogicalProcessorInformation(NULL, &len);
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(len);
    if (!info) return;

    if (!GetLogicalProcessorInformation(info, &len)) {
        free(info);
        return;
    }

    // Soma os tamanhos dos caches L1, L2 e L3
    DWORD count = len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    DWORD l1TotalKB = 0, l2TotalKB = 0, l3TotalKB = 0;

    for (DWORD i = 0; i < count; ++i) {
        if (info[i].Relationship == RelationCache) {
            CACHE_DESCRIPTOR cache = info[i].Cache;
            if (cache.Level == 1) l1TotalKB += cache.Size / 1024;
            else if (cache.Level == 2) l2TotalKB += cache.Size / 1024;
            else if (cache.Level == 3) l3TotalKB = cache.Size / 1024; // L3 geralmente é compartilhado
        }
    }

    free(info);

    // Exibe informações de cache formatadas
    if (l1TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L1 Cache: %.1f MB", l1TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L1 Cache: %u KB", l1TotalKB);
    SafeSetWindowText(cpuControls.hCacheL1, buffer);

    if (l2TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L2 Cache: %.1f MB", l2TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L2 Cache: %u KB", l2TotalKB);
    SafeSetWindowText(cpuControls.hCacheL2, buffer);

    if (l3TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L3 Cache: %.1f MB", l3TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L3 Cache: %u KB", l3TotalKB);
    SafeSetWindowText(cpuControls.hCacheL3, buffer);
}

// Atualiza informações de memória RAM
void UpdateRamInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    
    // Converte valores para MB e calcula porcentagem de uso
    DWORDLONG totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    DWORDLONG availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    DWORD usagePercent = memInfo.dwMemoryLoad;
    
    // Atualiza interface
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Total: %llu MB\nFree: %llu MB (used %lu%%)", totalRamMB, availRamMB, usagePercent);
    SafeSetWindowText(hLabelRam, buffer);
}

// Atualiza informações de disco
int UpdateDiskInfo(HWND hDiskLabel) {
    DWORD drives = GetLogicalDrives();
    char buffer[2048] = "";
    char temp[512];
    int partitionCount = 0;

    // Itera por todas as letras de drive possíveis (A-Z)
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (drives & (1 << (letter - 'A'))) {
            char rootPath[4] = { letter, ':', '\\', '\0' };
            char volumeName[MAX_PATH] = {0};
            char typeStr[16] = "Unknown";

            ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

            // Obtém informações do volume
            GetVolumeInformationA(rootPath, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);

            // Determina se o disco é SSD ou HDD
            char drivePath[7];
            snprintf(drivePath, sizeof(drivePath), "\\\\.\\%c:", letter);
            HANDLE hDevice = CreateFileA(drivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL, OPEN_EXISTING, 0, NULL);
            if (hDevice != INVALID_HANDLE_VALUE) {
                STORAGE_PROPERTY_QUERY query = { StorageDeviceSeekPenaltyProperty, PropertyStandardQuery };
                DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenalty = { 0 };
                DWORD bytesReturned;

                if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                                    &seekPenalty, sizeof(seekPenalty), &bytesReturned, NULL)) {
                    strcpy(typeStr, seekPenalty.IncursSeekPenalty ? "HDD" : "SSD");
                }

                CloseHandle(hDevice);
            }

            // Obtém espaço livre e total no disco
            if (GetDiskFreeSpaceEx(rootPath, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
                double totalGB = (double)totalBytes.QuadPart / (1024 * 1024 * 1024);
                double freeGB = (double)totalFreeBytes.QuadPart / (1024 * 1024 * 1024);

                // Formata a string de informação do disco
                if (volumeName[0]) {
                    snprintf(temp, sizeof(temp),
                        "Disco %c (%s): %.2f GB free / %.2f GB total (%s)\n",
                        letter, volumeName, freeGB, totalGB, typeStr);
                } else {
                    snprintf(temp, sizeof(temp),
                        "Disco %c: %.2f GB free / %.2f GB total (%s)\n",
                        letter, freeGB, totalGB, typeStr);
                }

                strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
                partitionCount++;
            }
        }
    }

    // Atualiza interface ou mostra mensagem se não houver discos
    SafeSetWindowText(hDiskLabel, strlen(buffer) ? buffer : "No drives detected.");
    return partitionCount;
}

// Atualiza informações do sistema operacional
void UpdateOSInfo(HWND hLabelOs) {
    HKEY hKeyOS;
    char osName[256] = "Unknown OS";
    char osDisplayVersion[256] = "Unknown Version";
    char osBuild[256] = "Unknown Build";
    char buffer[512];

    // Obtém informações do registro do Windows
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKeyOS) == ERROR_SUCCESS) {
        DWORD size;

        size = sizeof(osName);
        RegQueryValueExA(hKeyOS, "ProductName", NULL, NULL, (LPBYTE)osName, &size);

        size = sizeof(osDisplayVersion);
        RegQueryValueExA(hKeyOS, "DisplayVersion", NULL, NULL, (LPBYTE)osDisplayVersion, &size);

        size = sizeof(osBuild);
        RegQueryValueExA(hKeyOS, "CurrentBuild", NULL, NULL, (LPBYTE)osBuild, &size);

        RegCloseKey(hKeyOS);
    }

    // Formata e exibe as informações
    snprintf(buffer, sizeof(buffer), "OS: %s\nVersion: %s (Build %s)", osName, osDisplayVersion, osBuild);
    SafeSetWindowText(hLabelOs, buffer);
}

// Atualiza informações de tempo de atividade (uptime)
void UpdateUptimeInfo(HWND hLabelUptime) {
    char buffer[128];
    ULONGLONG uptimeMillis = GetTickCount64(); 
    ULONGLONG uptimeSecs = uptimeMillis / 1000;
    
    // Calcula dias, horas, minutos e segundos
    DWORD days = (DWORD)(uptimeSecs / 86400);
    DWORD hours = (DWORD)((uptimeSecs % 86400) / 3600);
    DWORD minutes = (DWORD)((uptimeSecs % 3600) / 60);
    DWORD seconds = (DWORD)(uptimeSecs % 60);

    // Formata e exibe o tempo de atividade
    snprintf(buffer, sizeof(buffer), "Uptime: %u:%02u:%02u:%02u", days, hours, minutes, seconds);
    SafeSetWindowText(hLabelUptime, buffer);
}

// Atualiza informações da GPU
int UpdateGPUInfo(HWND hLabelGpu) {
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA devInfoData;
    char buffer[2048] = "";
    char temp[512];
    int gpuCount = 0;

    // Obtém lista de dispositivos de vídeo
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        SafeSetWindowText(hLabelGpu, "GPU: Not detected");
        return 0;
    }

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Itera por todas as GPUs encontradas
    for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        char deviceName[256] = "Unknown";
        char driverKey[256] = "";
        char driverVersion[128] = "Unknown";
        char driverDate[128] = "Unknown";

        // Obtém nome da GPU
        SetupDiGetDeviceRegistryPropertyA(
            hDevInfo, &devInfoData, SPDRP_DEVICEDESC, NULL,
            (PBYTE)deviceName, sizeof(deviceName), NULL);

        // Obtém informações do driver
        if (SetupDiGetDeviceRegistryPropertyA(
                hDevInfo, &devInfoData, SPDRP_DRIVER, NULL,
                (PBYTE)driverKey, sizeof(driverKey), NULL)) {

            HKEY hKey;
            char regPath[256];
            snprintf(regPath, sizeof(regPath),
                     "SYSTEM\\CurrentControlSet\\Control\\Class\\%s", driverKey);

            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD size = sizeof(driverVersion);
                RegQueryValueExA(hKey, "DriverVersion", NULL, NULL, (LPBYTE)driverVersion, &size);

               size = sizeof(driverDate);
                if (RegQueryValueExA(hKey, "DriverDate", NULL, NULL, (LPBYTE)driverDate, &size) == ERROR_SUCCESS) {
                    // Converte formato da data de MM/DD/YYYY para DD/MM/YYYY
                    int mm, dd, yyyy;
                    if (sscanf(driverDate, "%d-%d-%d", &mm, &dd, &yyyy) == 3) {
                        snprintf(driverDate, sizeof(driverDate), "%02d/%02d/%04d", dd, mm, yyyy);
                    }
                }

                RegCloseKey(hKey);
            }
        }

        // Formata informações da GPU
        snprintf(temp, sizeof(temp),
                 "GPU %d: %s\n  Driver Version: %s\n  Driver Date: %s\n\n",
                 gpuCount + 1, deviceName, driverVersion, driverDate);

        strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
        gpuCount++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    // Se não encontrou GPUs, exibe mensagem
    if (gpuCount == 0) {
        snprintf(buffer, sizeof(buffer), "GPU: Not detected");
    }

    SafeSetWindowText(hLabelGpu, buffer);
    return gpuCount;
}

// Atualiza informações da bateria
void UpdateBatteryInfo(HWND hLabelBattery) {
    char buffer[256];
    SYSTEM_POWER_STATUS status;

    if (GetSystemPowerStatus(&status)) {
        if (status.BatteryFlag == 128) {
            snprintf(buffer, sizeof(buffer), "Battery: Not present\nStatus: N/A");
        } else {
            const char* statusStr = "Unknown";

            // Determina status da bateria
            if (status.ACLineStatus == 1) {
                statusStr = (status.BatteryLifePercent == 100) ? "Fully charged" : "Charging";
            } else if (status.ACLineStatus == 0) {
                statusStr = "Discharging";
            }

            // Calcula tempo restante de bateria se estiver descarregando
            char timeLeftStr[64] = "";
            if (status.ACLineStatus == 0) {
                if (status.BatteryLifeTime != (DWORD)-1 && status.BatteryLifeTime > 0) {
                    int totalMinutes = status.BatteryLifeTime / 60;
                    int hours = totalMinutes / 60;
                    int minutes = totalMinutes % 60;
                    snprintf(timeLeftStr, sizeof(timeLeftStr), "\nTime Left: %dhr %02dmin", hours, minutes);
                } else {
                    snprintf(timeLeftStr, sizeof(timeLeftStr), "\nTime Left: calculating...");
                }
            }

            // Formata e exibe informações da bateria
            snprintf(buffer, sizeof(buffer), 
                "Battery: %d%% (%s)%s", 
                status.BatteryLifePercent, statusStr, timeLeftStr);
        }
    } else {
        snprintf(buffer, sizeof(buffer), "Battery: Unknown");
    }

    SafeSetWindowText(hLabelBattery, buffer);
}

// Atualiza informações do sistema (nome do computador, usuário, fabricante, modelo)
void UpdateSystemInfo(HWND hLabelSystem) {
    char buffer[512];
    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = "Unknown";
    DWORD sizeName = sizeof(computerName);
    GetComputerNameA(computerName, &sizeName);

    char userName[256] = "Unknown";
    DWORD sizeUser = sizeof(userName);
    GetUserNameA(userName, &sizeUser);

    // Obtém informações do fabricante e modelo do sistema
    HKEY hKey;
    char manufacturer[256] = "Unknown";
    char model[256] = "Unknown";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD bufSize = sizeof(manufacturer);
        RegQueryValueExA(hKey, "SystemManufacturer", NULL, NULL, (LPBYTE)manufacturer, &bufSize);

        bufSize = sizeof(model);
        RegQueryValueExA(hKey, "SystemProductName", NULL, NULL, (LPBYTE)model, &bufSize);
        RegCloseKey(hKey);
    }

    // Formata e exibe as informações
    snprintf(buffer, sizeof(buffer), "Computer: %s (%s %s)\nUser: %s", 
             computerName, manufacturer, model, userName);
    SafeSetWindowText(hLabelSystem, buffer);
}

// Obtém o índice da interface de rede padrão
DWORD GetDefaultInterfaceIndex() {
    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    DWORD dwIndex = (DWORD)-1;

    // Obtém tabela de roteamento IP
    if (GetIpForwardTable(pIpForwardTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIpForwardTable = (PMIB_IPFORWARDTABLE)malloc(dwSize);
        if (pIpForwardTable) {
            if (GetIpForwardTable(pIpForwardTable, &dwSize, FALSE) == NO_ERROR) {
                // Procura pela rota padrão (0.0.0.0)
                for (DWORD i = 0; i < pIpForwardTable->dwNumEntries; i++) {
                    if (pIpForwardTable->table[i].dwForwardDest == 0) {
                        dwIndex = pIpForwardTable->table[i].dwForwardIfIndex;
                        break;
                    }
                }
            }
            free(pIpForwardTable);
        }
    }
    return dwIndex;
}

// Obtém e formata informações de uso da rede
void GetNetworkUsage(char* outBuffer, size_t bufSize) {
    PMIB_IFTABLE pIfTable = NULL;
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PMIB_IPFORWARDTABLE pIpForwardTable = NULL;
    DWORD dwSize = 0;
    char ipAddress[16] = "N/A";
    char adapterName[256] = "N/A";
    const int maxAdapterNameLength = 45; // Limite de caracteres para o nome do adaptador

    // Obtém informações dos adaptadores de rede
    dwSize = 0;
    if (GetAdaptersInfo(NULL, &dwSize) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (PIP_ADAPTER_INFO)malloc(dwSize);
        if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &dwSize) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                if (pAdapter->IpAddressList.IpAddress.String[0] != '0') {
                    // Trunca nome do adaptador se for muito longo
                    if (strlen(pAdapter->Description) > maxAdapterNameLength) {
                        _snprintf(adapterName, sizeof(adapterName), "%.*s...", 
                                 maxAdapterNameLength - 3, pAdapter->Description);
                    } else {
                        strncpy(adapterName, pAdapter->Description, sizeof(adapterName) - 1);
                    }
                    
                    strncpy(ipAddress, pAdapter->IpAddressList.IpAddress.String, sizeof(ipAddress) - 1);
                    break;
                }
                pAdapter = pAdapter->Next;
            }
        }
    }

    // Obtém interface de rede padrão
    DWORD dwDefaultIfIndex = (DWORD)-1;
    dwSize = 0;
    if (GetIpForwardTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIpForwardTable = (PMIB_IPFORWARDTABLE)malloc(dwSize);
        if (pIpForwardTable && GetIpForwardTable(pIpForwardTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIpForwardTable->dwNumEntries; i++) {
                if (pIpForwardTable->table[i].dwForwardDest == 0) {
                    dwDefaultIfIndex = pIpForwardTable->table[i].dwForwardIfIndex;
                    break;
                }
            }
        }
    }

    // Calcula estatísticas de rede (bytes recebidos/enviados)
    DWORD64 totalIn = 0, totalOut = 0;
    dwSize = 0;
    if (GetIfTable(NULL, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = (MIB_IFTABLE*)malloc(dwSize);
        if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
                MIB_IFROW* row = &pIfTable->table[i];
                if (dwDefaultIfIndex == (DWORD)-1 || row->dwIndex == dwDefaultIfIndex) {
                    totalIn += row->dwInOctets;
                    totalOut += row->dwOutOctets;
                    if (dwDefaultIfIndex != (DWORD)-1) break;
                }
            }
        }
    }

    // Calcula velocidade de rede (Kbps)
    ULONGLONG currentTime = GetTickCount64();
    double elapsedSec = (currentTime - prevTime) / 1000.0;
    double inKbps = 0.0, outKbps = 0.0;

    if (prevTime != 0 && elapsedSec > 0.0) {
        inKbps = (totalIn - prevIn) * 8.0 / 1000.0 / elapsedSec;
        outKbps = (totalOut - prevOut) * 8.0 / 1000.0 / elapsedSec;
    }

    // Atualiza valores para próxima chamada
    prevIn = totalIn;
    prevOut = totalOut;
    prevTime = currentTime;

    // Formata saída com informações de rede
    _snprintf(outBuffer, bufSize, 
             "Adapter: %s\n"
             "IPv4: %s\n"
             "Receive: %.1f Kbps\n"
             "Send: %.1f Kbps", 
             adapterName, ipAddress, inKbps, outKbps);

    // Libera memória alocada
    if (pIfTable) free(pIfTable);
    if (pAdapterInfo) free(pAdapterInfo);
    if (pIpForwardTable) free(pIpForwardTable);
}

// Procedimento de janela personalizado para o painel de hardware
LRESULT CALLBACK HardwarePanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND: {
            // Pinta o fundo do painel com a cor de fundo
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            FillRect((HDC)wParam, &rcClient, hbrBackground);
            return 1; // Indica que a mensagem foi tratada
        }
        case WM_CTLCOLORSTATIC: {
            // Configura cores para controles estáticos (labels)
            HDC hdcStatic = (HDC)wParam;
            SetBkColor(hdcStatic, clrBackground);  // Cor de fundo
            SetTextColor(hdcStatic, RGB(0, 0, 0)); // Cor do texto (preto)
            return (LRESULT)hbrBackground;         // Retorna pincel de fundo
        }
        case WM_NCDESTROY:
            // Restaura o procedimento original quando a janela é destruída
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_lpfnOriginalHardwarePanelProc);
            break;
    }
    // Passa mensagens não tratadas para o procedimento original
    return CallWindowProc(g_lpfnOriginalHardwarePanelProc, hwnd, msg, wParam, lParam);
}

// Cria e configura o painel de hardware
void AddHardwarePanel(HWND hwndParent) {
    // Cria o painel principal
    hHardwarePanel = CreateWindowEx(0, "STATIC", "", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 40, gWindowWidth - 20, gWindowHeight - 50,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);

    // Substitui o procedimento de janela pelo personalizado
    g_lpfnOriginalHardwarePanelProc = (WNDPROC)SetWindowLongPtr(hHardwarePanel, GWLP_WNDPROC, (LONG_PTR)HardwarePanelProc);
    
    ShowWindow(hHardwarePanel, SW_HIDE);

    int yPos = 10;

    // --- Seção CPU ---
    cpuControls.hGroup = CreateGroupBox(hHardwarePanel, "CPU", 22.5, yPos, LEFT_COLUMN_WIDTH, 240);
    cpuControls.hName = CreateLabel(hHardwarePanel, "Model: Loading...", 32.5, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCores = CreateLabel(hHardwarePanel, "Cores: Loading...", 32.5, yPos + 45, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hUsage = CreateLabel(hHardwarePanel, "Usage: 0%", 32.5, yPos + 65, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hSpeed = CreateLabel(hHardwarePanel, "Speed: 0.00 GHz", 32.5, yPos + 85, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hProcesses = CreateLabel(hHardwarePanel, "Processes: Loading...", 32.5, yPos + 105, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hThreads = CreateLabel(hHardwarePanel, "Threads: Loading...", 32.5, yPos + 125, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hHandles = CreateLabel(hHardwarePanel, "Handles: Loading...", 32.5, yPos + 145, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCacheL1 = CreateLabel(hHardwarePanel, "L1 Cache: Loading...", 32.5, yPos + 165, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCacheL2 = CreateLabel(hHardwarePanel, "L2 Cache: Loading...", 32.5, yPos + 185, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCacheL3 = CreateLabel(hHardwarePanel, "L3 Cache: Loading...", 32.5, yPos + 205, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    
    yPos += 240 + SECTION_SPACING;

    // --- Seção RAM ---
    hRamGroup = CreateGroupBox(hHardwarePanel, "Memory", 22.5, yPos, LEFT_COLUMN_WIDTH, 75);
    hLabelRam = CreateLabel(hHardwarePanel, "Total: Loading...\nFree: Loading...", 32.5, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPos += 75 + SECTION_SPACING;

    // --- Seção DISCO (ajustável dinamicamente) ---
    hDiskGroup = CreateGroupBox(hHardwarePanel, "Storage", 22.5, yPos, LEFT_COLUMN_WIDTH, 75);
    hLabelDisk = CreateLabel(hHardwarePanel, "", 32.5, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45, SS_LEFT);

    // Atualiza conteúdo e obtém quantidade de partições
    int diskCount = UpdateDiskInfo(hLabelDisk);

    // Ajusta altura com base no número de partições
    int diskLineHeight = 20;
    int diskGroupBoxHeight = diskCount * diskLineHeight + 20;
    if (diskGroupBoxHeight < 75) diskGroupBoxHeight = 75;

    // Redimensiona controles
    SetWindowPos(hDiskGroup, NULL, 0, 0, LEFT_COLUMN_WIDTH, diskGroupBoxHeight, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowPos(hLabelDisk, NULL, 0, 0, LEFT_COLUMN_WIDTH - 20, diskGroupBoxHeight - 30, SWP_NOMOVE | SWP_NOZORDER);

    yPos += diskGroupBoxHeight + SECTION_SPACING;

    // --- Coluna da direita ---
    int yPosRight = 10;
    hOsGroup = CreateGroupBox(hHardwarePanel, "Operating System", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelOs = CreateLabel(hHardwarePanel, "OS: Loading...\nVersion: Loading...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPosRight += 75 + SECTION_SPACING;

    // --- Seção GPU (ajustável dinamicamente) ---
    hGpuGroup = CreateGroupBox(hHardwarePanel, "Graphics", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelGpu = CreateLabel(hHardwarePanel, "", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);

    // Atualiza conteúdo e obtém quantidade de GPUs
    int gpuCount = UpdateGPUInfo(hLabelGpu);

    // Ajusta altura com base no número de GPUs
    int gpuLineHeight  = 20;
    int gpuBlockHeight = 4 * gpuLineHeight;
    int gpuGroupBoxHeight = gpuCount * gpuBlockHeight;

    // Redimensiona controles
    SetWindowPos(hGpuGroup, NULL, 0, 0, RIGHT_COLUMN_WIDTH, gpuGroupBoxHeight, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowPos(hLabelGpu, NULL, 0, 0, RIGHT_COLUMN_WIDTH - 20, gpuGroupBoxHeight - 30, SWP_NOMOVE | SWP_NOZORDER);

    yPosRight += gpuGroupBoxHeight  + SECTION_SPACING;

    // --- Seção Bateria ---
    hBatteryGroup = CreateGroupBox(hHardwarePanel, "Power", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelBattery = CreateLabel(hHardwarePanel, "Battery: Loading...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPosRight += 75 + SECTION_SPACING;

    // --- Seção Sistema ---
    hSystemGroup = CreateGroupBox(hHardwarePanel, "System", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelSystem = CreateLabel(hHardwarePanel, "Computer: Loading...\nUser: Loading...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPosRight += 75 + SECTION_SPACING;

    // --- Seção Rede ---
    hNetworkGroup = CreateGroupBox(hHardwarePanel, "Network", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 100);
    hLabelNetwork = CreateLabel(hHardwarePanel, "Receive: ...\nSend: ...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 70, SS_LEFT);

    // --- Uptime (no rodapé) ---
    hLabelUptime = CreateLabel(hHardwarePanel, "Uptime: Loading...", 0, gWindowHeight - 100, gWindowWidth, 20, SS_CENTER);
}

// Atualiza todas as informações de hardware
void UpdateHardwareInfo() {
    UpdateCpuInfo();          // Atualiza informações da CPU
    UpdateRamInfo();          // Atualiza informações de memória
    UpdateDiskInfo(hLabelDisk); // Atualiza informações de disco
    UpdateOSInfo(hLabelOs);    // Atualiza informações do sistema operacional
    UpdateUptimeInfo(hLabelUptime); // Atualiza tempo de atividade
    UpdateGPUInfo(hLabelGpu);  // Atualiza informações da GPU
    UpdateBatteryInfo(hLabelBattery); // Atualiza informações da bateria
    UpdateSystemInfo(hLabelSystem); // Atualiza informações do sistema
    
    // Atualiza informações de rede
    char netBuffer[128];
    GetNetworkUsage(netBuffer, sizeof(netBuffer));
    SafeSetWindowText(hLabelNetwork, netBuffer);
}