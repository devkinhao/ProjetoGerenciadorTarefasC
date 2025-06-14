#include "hardware.h"
#include "../utils/utils.h"


#define SECTION_SPACING 15
#define GROUPBOX_HEIGHT 30
#define ITEM_SPACING 25
#define LEFT_COLUMN_WIDTH 350
#define RIGHT_COLUMN_WIDTH 350

#define GROUPBOX_TOP_MARGIN 25

extern int gWindowWidth;
extern int gWindowHeight;

// Declare hbrBackground e clrBackground como externos (definidos em main.c)
extern HBRUSH hbrBackground;
extern COLORREF clrBackground;

// Variável estática para guardar o procedimento de janela original de hHardwarePanel
static WNDPROC g_lpfnOriginalHardwarePanelProc;


typedef struct {
    HWND hGroup;
    HWND hName;
    HWND hCores;
    HWND hUsage;
    HWND hSpeed;
    HWND hProcesses;
    HWND hThreads;
    HWND hHandles;
    HWND hCacheL1;
    HWND hCacheL2;
    HWND hCacheL3;
} CpuControls;

static CpuControls cpuControls;
HWND hRamGroup, hDiskGroup, hOsGroup, hGpuGroup, hBatteryGroup, hSystemGroup;
HWND hLabelRam, hLabelDisk, hLabelOs, hLabelGpu, hLabelBattery, hLabelSystem, hLabelUptime;

static FILETIME prevIdleTime = {0}, prevKernelTime = {0}, prevUserTime = {0};

BOOL Is64BitWindows() {
#if defined(_WIN64)
    return TRUE;
#elif defined(_WIN32)
    BOOL bIsWow64 = FALSE;
    IsWow64Process(GetCurrentProcess(), &bIsWow64);
    return bIsWow64;
#else
    return FALSE;
#endif
}

ULONGLONG FileTimeToULL(FILETIME ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

HWND CreateLabel(HWND parent, const char* text, int x, int y, int width, int height, DWORD alignStyle) {
    // Criar a janela
    HWND hLabel = CreateWindowEx(0, "STATIC", text,
        WS_CHILD | WS_VISIBLE | alignStyle,
        x, y, width, height, 
        parent, NULL, GetModuleHandle(NULL), NULL);

    // Criar a fonte personalizada
    HFONT hFont = CreateFontForControl();
    
    // Aplicar a fonte ao label
    SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hLabel;
}


HWND CreateGroupBox(HWND parent, const char* title, int x, int y, int width, int height) {
    // Criar a janela
    HWND hGroupBox = CreateWindowEx(0, "BUTTON", title,
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 
        x, y, width, height, 
        parent, NULL, GetModuleHandle(NULL), NULL);

    // Criar a fonte personalizada
    HFONT hFont = CreateFontForControl();
    
    // Aplicar a fonte ao group box
    SendMessage(hGroupBox, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hGroupBox;
}

double CalculateCpuUsage() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0;
    }

    ULONGLONG idleDiff = FileTimeToULL(idleTime) - FileTimeToULL(prevIdleTime);
    ULONGLONG kernelDiff = FileTimeToULL(kernelTime) - FileTimeToULL(prevKernelTime);
    ULONGLONG userDiff = FileTimeToULL(userTime) - FileTimeToULL(prevUserTime);

    ULONGLONG totalSystem = kernelDiff + userDiff;

    prevIdleTime = idleTime;
    prevKernelTime = kernelTime;
    prevUserTime = userTime;

    if (totalSystem == 0) return 0.0;

    return (double)(totalSystem - idleDiff) * 100.0 / totalSystem;
}

void UpdateCpuInfo() {
    char buffer[256];
    HKEY hKeyProcessor;
    char cpuName[256] = "Unknown CPU";
    DWORD cpuMHz = 0;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKeyProcessor) == ERROR_SUCCESS) {
        DWORD size = sizeof(cpuName);
        RegQueryValueEx(hKeyProcessor, "ProcessorNameString", NULL, NULL, (LPBYTE)cpuName, &size);
        size = sizeof(cpuMHz);
        RegQueryValueEx(hKeyProcessor, "~MHz", NULL, NULL, (LPBYTE)&cpuMHz, &size);
        RegCloseKey(hKeyProcessor);
    }

    double cpuUsage = CalculateCpuUsage();
    double clockSpeed = (double)cpuMHz / 1000.0;

    SafeSetWindowText(cpuControls.hName, cpuName);
    snprintf(buffer, sizeof(buffer), "Logical Processors: %d", sysInfo.dwNumberOfProcessors);
    SafeSetWindowText(cpuControls.hCores, buffer);
    snprintf(buffer, sizeof(buffer), "Usage: %d%%", (int)cpuUsage);
    SafeSetWindowText(cpuControls.hUsage, buffer);
    snprintf(buffer, sizeof(buffer), "Base speed: %.2f GHz", clockSpeed);
    SafeSetWindowText(cpuControls.hSpeed, buffer);

     // Obter informações de processos, threads e handles do sistema
    DWORD processCount = 0;
    DWORD threadCount = 0;
    DWORD handleCount = 0;

    DWORD processes[1024], cbNeeded, cProcesses;
    if (EnumProcesses(processes, sizeof(processes), &cbNeeded)) {
        cProcesses = cbNeeded / sizeof(DWORD);
        processCount = cProcesses;

        // Somar handles
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

        // Contar threads: snapshot único
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

    snprintf(buffer, sizeof(buffer), "Processes: %lu", processCount);
    SafeSetWindowText(cpuControls.hProcesses, buffer);

    snprintf(buffer, sizeof(buffer), "Threads: %lu", threadCount);
    SafeSetWindowText(cpuControls.hThreads, buffer);

    snprintf(buffer, sizeof(buffer), "Handles: %lu", handleCount);
    SafeSetWindowText(cpuControls.hHandles, buffer);

    // Cache info (L1, L2, L3)
    DWORD len = 0;
    GetLogicalProcessorInformation(NULL, &len);
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(len);
    if (!info) return;

    if (!GetLogicalProcessorInformation(info, &len)) {
        free(info);
        return;
    }

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

    // Exibir L1
    if (l1TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L1 Cache: %.1f MB", l1TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L1 Cache: %u KB", l1TotalKB);
    SafeSetWindowText(cpuControls.hCacheL1, buffer);

    // Exibir L2
    if (l2TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L2 Cache: %.1f MB", l2TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L2 Cache: %u KB", l2TotalKB);
    SafeSetWindowText(cpuControls.hCacheL2, buffer);

    // Exibir L3
    if (l3TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L3 Cache: %.1f MB", l3TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L3 Cache: %u KB", l3TotalKB);
    SafeSetWindowText(cpuControls.hCacheL3, buffer);
}

void UpdateRamInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    DWORDLONG availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    DWORD usagePercent = memInfo.dwMemoryLoad;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Total: %llu MB\nFree: %llu MB (used %lu%%)", totalRamMB, availRamMB, usagePercent);
    SafeSetWindowText(hLabelRam, buffer);
}

int UpdateDiskInfo(HWND hDiskLabel) {
    DWORD drives = GetLogicalDrives();
    char buffer[2048] = "";
    char temp[512];
    int partitionCount = 0;

    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (drives & (1 << (letter - 'A'))) {
            char rootPath[4] = { letter, ':', '\\', '\0' };
            char volumeName[MAX_PATH] = {0};
            char typeStr[16] = "Unknown";

            ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

            GetVolumeInformationA(rootPath, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);

            // Obter tipo de disco (SSD ou HDD)
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

            if (GetDiskFreeSpaceEx(rootPath, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
                double totalGB = (double)totalBytes.QuadPart / (1024 * 1024 * 1024);
                double freeGB = (double)totalFreeBytes.QuadPart / (1024 * 1024 * 1024);

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

    SafeSetWindowText(hDiskLabel, strlen(buffer) ? buffer : "No drives detected.");
    return partitionCount;
}

void UpdateOSInfo(HWND hLabelOs) {
    HKEY hKeyOS;
    char osName[256] = "Unknown OS";
    char osDisplayVersion[256] = "Unknown Version";
    char osBuild[256] = "Unknown Build";
    char buffer[512];

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

    snprintf(buffer, sizeof(buffer), "OS: %s\nVersion: %s (Build %s)", osName, osDisplayVersion, osBuild);
    SafeSetWindowText(hLabelOs, buffer);
}

void UpdateUptimeInfo(HWND hLabelUptime) {
    char buffer[128];
    ULONGLONG uptimeMillis = GetTickCount64(); 
    ULONGLONG uptimeSecs = uptimeMillis / 1000;
    DWORD days = (DWORD)(uptimeSecs / 86400);
    DWORD hours = (DWORD)((uptimeSecs % 86400) / 3600);
    DWORD minutes = (DWORD)((uptimeSecs % 3600) / 60);
    DWORD seconds = (DWORD)(uptimeSecs % 60);

    snprintf(buffer, sizeof(buffer), "Uptime: %u:%02u:%02u:%02u", days, hours, minutes, seconds);
    SafeSetWindowText(hLabelUptime, buffer);
}

int UpdateGPUInfo(HWND hLabelGpu) {
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA devInfoData;
    char buffer[2048] = "";
    char temp[512];
    int gpuCount = 0;

    hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        SafeSetWindowText(hLabelGpu, "GPU: Not detected");
        return 0;
    }

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
        char deviceName[256] = "Unknown";
        char driverKey[256] = "";
        char driverVersion[128] = "Unknown";
        char driverDate[128] = "Unknown";

        // Nome da GPU
        SetupDiGetDeviceRegistryPropertyA(
            hDevInfo, &devInfoData, SPDRP_DEVICEDESC, NULL,
            (PBYTE)deviceName, sizeof(deviceName), NULL);

        // Nome da chave do driver
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
                    // Converte de MM/DD/YYYY para DD/MM/YYYY
                    int mm, dd, yyyy;
                    if (sscanf(driverDate, "%d-%d-%d", &mm, &dd, &yyyy) == 3) {
                        snprintf(driverDate, sizeof(driverDate), "%02d/%02d/%04d", dd, mm, yyyy);
                    }
                }

                RegCloseKey(hKey);
            }
        }

        snprintf(temp, sizeof(temp),
                 "GPU %d: %s\n  Driver Version: %s\n  Driver Date: %s\n\n",
                 gpuCount + 1, deviceName, driverVersion, driverDate);

        strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
        gpuCount++;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    if (gpuCount == 0) {
        snprintf(buffer, sizeof(buffer), "GPU: Not detected");
    }

    SafeSetWindowText(hLabelGpu, buffer);
    return gpuCount;
}

void UpdateBatteryInfo(HWND hLabelBattery) {
    char buffer[256];
    SYSTEM_POWER_STATUS status;

    if (GetSystemPowerStatus(&status)) {
        if (status.BatteryFlag == 128) {
            snprintf(buffer, sizeof(buffer), "Battery: Not present\nStatus: N/A");
        } else {
            const char* statusStr = "Unknown";

            if (status.ACLineStatus == 1) {
                statusStr = (status.BatteryLifePercent == 100) ? "Fully charged" : "Charging";
            } else if (status.ACLineStatus == 0) {
                statusStr = "Discharging";
            }

            // 🟩 TIME LEFT
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

            snprintf(buffer, sizeof(buffer), 
                "Battery: %d%% (%s)%s", 
                status.BatteryLifePercent, statusStr, timeLeftStr);
        }
    } else {
        snprintf(buffer, sizeof(buffer), "Battery: Unknown");
    }

    SafeSetWindowText(hLabelBattery, buffer);
}

void UpdateSystemInfo(HWND hLabelSystem) {
    char buffer[512];
    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = "Unknown";
    DWORD sizeName = sizeof(computerName);
    GetComputerNameA(computerName, &sizeName);

    char userName[256] = "Unknown";
    DWORD sizeUser = sizeof(userName);
    GetUserNameA(userName, &sizeUser);

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

    snprintf(buffer, sizeof(buffer), "Computer: %s (%s %s)\nUser: %s", 
             computerName, manufacturer, model, userName);
    SafeSetWindowText(hLabelSystem, buffer);
}

// Novo procedimento de janela para hHardwarePanel
LRESULT CALLBACK HardwarePanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND: {
            // Pinta o fundo do próprio hHardwarePanel de branco
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            FillRect((HDC)wParam, &rcClient, hbrBackground);
            return 1; // Indicamos que tratamos o fundo
        }
        case WM_CTLCOLORSTATIC: {
            // Esta mensagem é enviada ao hHardwarePanel pelos seus filhos (labels e group boxes)
            HDC hdcStatic = (HDC)wParam;
            // HWND hStatic = (HWND)lParam; // Handle do controle estático filho

            // Define a cor de fundo do controle filho como branco
            SetBkColor(hdcStatic, clrBackground);
            // Define a cor do texto do controle filho como preto
            SetTextColor(hdcStatic, RGB(0, 0, 0));

            // Retorna o pincel branco para pintar o fundo do controle filho
            return (LRESULT)hbrBackground;
        }
        case WM_NCDESTROY:
            // Quando a janela está sendo destruída, restaura o procedimento original.
            // Isso é importante para evitar problemas se a memória da janela for reutilizada.
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)g_lpfnOriginalHardwarePanelProc);
            break;
    }
    // Chama o procedimento de janela original para o tratamento padrão de outras mensagens
    return CallWindowProc(g_lpfnOriginalHardwarePanelProc, hwnd, msg, wParam, lParam);
}

void AddHardwarePanel(HWND hwndParent) {
    hHardwarePanel = CreateWindowEx(0, "STATIC", "", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0, 40, gWindowWidth - 20, gWindowHeight - 50,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);


    g_lpfnOriginalHardwarePanelProc = (WNDPROC)SetWindowLongPtr(hHardwarePanel, GWLP_WNDPROC, (LONG_PTR)HardwarePanelProc);
    
    ShowWindow(hHardwarePanel, SW_HIDE);

    int yPos = 10;

    // --- CPU Section ---
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
    hDiskGroup = CreateGroupBox(hHardwarePanel, "Storage", 22.5, yPos, LEFT_COLUMN_WIDTH, 75);  // altura será ajustada depois
    hLabelDisk = CreateLabel(hHardwarePanel, "", 32.5, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45, SS_LEFT);

    // Atualiza conteúdo e obtém quantidade de partições
    int diskCount = UpdateDiskInfo(hLabelDisk);

    // Define altura com base no número de partições
    int diskLineHeight = 20;
    int diskGroupBoxHeight = diskCount * diskLineHeight + 20;
    if (diskGroupBoxHeight < 75) diskGroupBoxHeight = 75;

    // Ajusta tamanho do GroupBox e do Label
    SetWindowPos(hDiskGroup, NULL, 0, 0, LEFT_COLUMN_WIDTH, diskGroupBoxHeight, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowPos(hLabelDisk, NULL, 0, 0, LEFT_COLUMN_WIDTH - 20, diskGroupBoxHeight - 30, SWP_NOMOVE | SWP_NOZORDER);

    // Atualiza yPos
    yPos += diskGroupBoxHeight + SECTION_SPACING;

    // --- Coluna da direita ---
    int yPosRight = 10;  // Começar a coluna direita no mesmo nível da coluna esquerda
    hOsGroup = CreateGroupBox(hHardwarePanel, "Operating System", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelOs = CreateLabel(hHardwarePanel, "OS: Loading...\nVersion: Loading...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPosRight += 75 + SECTION_SPACING;

    // --- Seção GPU (ajustável dinamicamente) ---
    hGpuGroup = CreateGroupBox(hHardwarePanel, "Graphics", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelGpu = CreateLabel(hHardwarePanel, "", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);

    // Atualiza conteúdo e obtém quantidade de GPUs
    int gpuCount = UpdateGPUInfo(hLabelGpu);

    // Define altura com base no número de GPUs
    int gpuLineHeight  = 20;
    int gpuBlockHeight = 4 * gpuLineHeight;
    int gpuGroupBoxHeight = gpuCount * gpuBlockHeight;

    // Ajusta tamanho do GroupBox e do Label
    SetWindowPos(hGpuGroup, NULL, 0, 0, RIGHT_COLUMN_WIDTH, gpuGroupBoxHeight, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowPos(hLabelGpu, NULL, 0, 0, RIGHT_COLUMN_WIDTH - 20, gpuGroupBoxHeight - 30, SWP_NOMOVE | SWP_NOZORDER);

    // Atualiza yPosRight
    yPosRight += gpuGroupBoxHeight  + SECTION_SPACING;

    hBatteryGroup = CreateGroupBox(hHardwarePanel, "Power", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelBattery = CreateLabel(hHardwarePanel, "Battery: Loading...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPosRight += 75 + SECTION_SPACING;

    hSystemGroup = CreateGroupBox(hHardwarePanel, "System", LEFT_COLUMN_WIDTH + 20 + 12.5, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelSystem = CreateLabel(hHardwarePanel, "Computer: Loading...\nUser: Loading...", LEFT_COLUMN_WIDTH + 42.5, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    
    hLabelUptime = CreateLabel(hHardwarePanel, "Uptime: Loading...", 0, gWindowHeight - 100, gWindowWidth, 20, SS_CENTER);

}

void UpdateHardwareInfo() {
    UpdateCpuInfo();
    UpdateRamInfo();
    UpdateDiskInfo(hLabelDisk);
    UpdateOSInfo(hLabelOs);
    UpdateUptimeInfo(hLabelUptime);
    UpdateGPUInfo(hLabelGpu);
    UpdateBatteryInfo(hLabelBattery);
    UpdateSystemInfo(hLabelSystem);
}