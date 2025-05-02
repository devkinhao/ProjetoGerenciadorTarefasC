#include "hardware.h"
#include "../utils/utils.h"


#define SECTION_SPACING 10
#define GROUPBOX_HEIGHT 30
#define ITEM_SPACING 25
#define LEFT_COLUMN_WIDTH 350
#define RIGHT_COLUMN_WIDTH (WINDOW_WIDTH - LEFT_COLUMN_WIDTH - 40)

#define GROUPBOX_TOP_MARGIN 25
#define UPTIME_HEIGHT 20

typedef struct {
    HWND hGroup;
    HWND hName;
    HWND hCores;
    HWND hUsage;
    HWND hSpeed;
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

    SetWindowText(cpuControls.hName, cpuName);
    snprintf(buffer, sizeof(buffer), "Logical Processors: %d", sysInfo.dwNumberOfProcessors);
    SetWindowText(cpuControls.hCores, buffer);
    snprintf(buffer, sizeof(buffer), "Usage: %d%%", (int)cpuUsage);
    SetWindowText(cpuControls.hUsage, buffer);
    snprintf(buffer, sizeof(buffer), "Speed: %.2f GHz", clockSpeed);
    SetWindowText(cpuControls.hSpeed, buffer);

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
    SetWindowText(cpuControls.hCacheL1, buffer);

    // Exibir L2
    if (l2TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L2 Cache: %.1f MB", l2TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L2 Cache: %u KB", l2TotalKB);
    SetWindowText(cpuControls.hCacheL2, buffer);

    // Exibir L3
    if (l3TotalKB >= 1024)
        snprintf(buffer, sizeof(buffer), "L3 Cache: %.1f MB", l3TotalKB / 1024.0f);
    else
        snprintf(buffer, sizeof(buffer), "L3 Cache: %u KB", l3TotalKB);
    SetWindowText(cpuControls.hCacheL3, buffer);
}

void UpdateRamInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    DWORDLONG availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    DWORD usagePercent = memInfo.dwMemoryLoad;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Total: %llu MB\nFree: %llu MB (%lu%%)", totalRamMB, availRamMB, usagePercent);
    SetWindowText(hLabelRam, buffer);
}

void UpdateDiskInfo(HWND hDiskLabel) {
    DWORD drives = GetLogicalDrives();
    char buffer[2048] = "";
    char temp[512];

    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (drives & (1 << (letter - 'A'))) {
            char rootPath[4] = { letter, ':', '\\', '\0' };

            ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
            char volumeName[MAX_PATH] = {0};

            GetVolumeInformationA(rootPath, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);

            if (GetDiskFreeSpaceEx(rootPath, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
                double totalGB = (double)totalBytes.QuadPart / (1024 * 1024 * 1024);
                double freeGB = (double)totalFreeBytes.QuadPart / (1024 * 1024 * 1024);

                if (volumeName[0]) {
                    snprintf(temp, sizeof(temp),
                        "Disco %c (%s): %.2f GB livre / %.2f GB total\n",
                        letter, volumeName, freeGB, totalGB);
                } else {
                    snprintf(temp, sizeof(temp),
                        "Disco %c: %.2f GB livre / %.2f GB total\n",
                        letter, freeGB, totalGB);
                }

                strncat(buffer, temp, sizeof(buffer) - strlen(buffer) - 1);
            }
        }
    }

    SetWindowText(hDiskLabel, strlen(buffer) ? buffer : "Nenhuma unidade detectada.");
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
    SetWindowTextA(hLabelOs, buffer);
}

void UpdateUptimeInfo(HWND hLabelUptime) {
    char buffer[128];
    ULONGLONG uptimeMillis = GetTickCount(); 
    ULONGLONG uptimeSecs = uptimeMillis / 1000;
    DWORD days = (DWORD)(uptimeSecs / 86400);
    DWORD hours = (DWORD)((uptimeSecs % 86400) / 3600);
    DWORD minutes = (DWORD)((uptimeSecs % 3600) / 60);
    DWORD seconds = (DWORD)(uptimeSecs % 60);

    snprintf(buffer, sizeof(buffer), "Uptime: %u:%02u:%02u:%02u", days, hours, minutes, seconds);
    SetWindowTextA(hLabelUptime, buffer);
}

void UpdateGPUInfo(HWND hLabelGpu) {
    char buffer[256];
    DISPLAY_DEVICEA dd;
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    if (EnumDisplayDevicesA(NULL, 0, &dd, 0)) {
        for (int i = strlen(dd.DeviceString) - 1; i >= 0 && isspace(dd.DeviceString[i]); i--) {
            dd.DeviceString[i] = '\0';
        }
        snprintf(buffer, sizeof(buffer), "GPU: %s", dd.DeviceString);
    } else {
        snprintf(buffer, sizeof(buffer), "GPU: Not detected");
    }

    SetWindowTextA(hLabelGpu, buffer);
}

void UpdateBatteryInfo(HWND hLabelBattery) {
    char buffer[128];
    SYSTEM_POWER_STATUS status;

    if (GetSystemPowerStatus(&status)) {
        if (status.BatteryFlag == 128) {
            snprintf(buffer, sizeof(buffer), "Battery: Not present");
        } else {
            const char* statusStr = "Unknown";
            if (status.ACLineStatus == 1) statusStr = "Charging";
            else if (status.BatteryLifeTime != (DWORD)-1) statusStr = "Discharging";

            snprintf(buffer, sizeof(buffer), "Battery: %d%% (%s)", status.BatteryLifePercent, statusStr);
        }
    } else {
        snprintf(buffer, sizeof(buffer), "Battery: Unknown");
    }

    SetWindowTextA(hLabelBattery, buffer);
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
    SetWindowTextA(hLabelSystem, buffer);
}

void AddHardwarePanel(HWND hwndParent) {
    hHardwarePanel = CreateWindowEx(0, "STATIC", "", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 40, WINDOW_WIDTH - 20, WINDOW_HEIGHT - 50,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(hHardwarePanel, SW_HIDE);

    int yPos = 10;

    // --- CPU Section ---
    cpuControls.hGroup = CreateGroupBox(hHardwarePanel, "CPU", 10, yPos, LEFT_COLUMN_WIDTH, 210);
    cpuControls.hName = CreateLabel(hHardwarePanel, "Model: Loading...", 20, yPos + 25, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCores = CreateLabel(hHardwarePanel, "Cores: Loading...", 20, yPos + 50, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hUsage = CreateLabel(hHardwarePanel, "Usage: 0%", 20, yPos + 75, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hSpeed = CreateLabel(hHardwarePanel, "Speed: 0.00 GHz", 20, yPos + 100, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCacheL1 = CreateLabel(hHardwarePanel, "L1 Cache: Loading...", 20, yPos + 125, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCacheL2 = CreateLabel(hHardwarePanel, "L2 Cache: Loading...", 20, yPos + 150, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    cpuControls.hCacheL3 = CreateLabel(hHardwarePanel, "L3 Cache: Loading...", 20, yPos + 175, LEFT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    
    yPos += 210 + SECTION_SPACING;

    // --- Seções RAM, Disco, SO, GPU, Bateria, Sistema ---
    hRamGroup = CreateGroupBox(hHardwarePanel, "Memory", 10, yPos, LEFT_COLUMN_WIDTH, 75);
    hLabelRam = CreateLabel(hHardwarePanel, "Total: Loading...\nFree: Loading...", 20, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPos += 75 + SECTION_SPACING;

    hDiskGroup = CreateGroupBox(hHardwarePanel, "Storage", 10, yPos, LEFT_COLUMN_WIDTH, 75);
    hLabelDisk = CreateLabel(hHardwarePanel, "Total: Loading...\nFree: Loading...", 20, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPos += 75 + SECTION_SPACING;

    // --- Coluna da direita ---
    int yPosRight = 10;  // Começar a coluna direita no mesmo nível da coluna esquerda
    hOsGroup = CreateGroupBox(hHardwarePanel, "Operating System", LEFT_COLUMN_WIDTH + 20, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelOs = CreateLabel(hHardwarePanel, "OS: Loading...\nVersion: Loading...", LEFT_COLUMN_WIDTH + 30, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    yPosRight += 75 + SECTION_SPACING;

    hGpuGroup = CreateGroupBox(hHardwarePanel, "Graphics", LEFT_COLUMN_WIDTH + 20, yPosRight, RIGHT_COLUMN_WIDTH, 60);
    hLabelGpu = CreateLabel(hHardwarePanel, "GPU: Loading...", LEFT_COLUMN_WIDTH + 30, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    yPosRight += 60 + SECTION_SPACING;

    hBatteryGroup = CreateGroupBox(hHardwarePanel, "Power", LEFT_COLUMN_WIDTH + 20, yPosRight, RIGHT_COLUMN_WIDTH, 60);
    hLabelBattery = CreateLabel(hHardwarePanel, "Battery: Loading...", LEFT_COLUMN_WIDTH + 30, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 20, SS_LEFT);
    yPosRight += 60 + SECTION_SPACING;

    hSystemGroup = CreateGroupBox(hHardwarePanel, "System", LEFT_COLUMN_WIDTH + 20, yPosRight, RIGHT_COLUMN_WIDTH, 75);
    hLabelSystem = CreateLabel(hHardwarePanel, "Computer: Loading...\nUser: Loading...", LEFT_COLUMN_WIDTH + 30, yPosRight + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45, SS_LEFT);
    
    hLabelUptime = CreateLabel(hHardwarePanel, "Uptime: Loading...", (WINDOW_WIDTH - (WINDOW_WIDTH - 40)) / 2, WINDOW_HEIGHT - 100, WINDOW_WIDTH - 40, UPTIME_HEIGHT, SS_CENTER);

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