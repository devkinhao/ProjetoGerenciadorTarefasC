#include "hardware.h"
#include "..\utils\utils.h"
#include <winbase.h>
#include <ctype.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <powrprof.h>
#include <commctrl.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "comctl32.lib")

// Definições de layout
#define SECTION_SPACING 10
#define GROUPBOX_HEIGHT 30
#define ITEM_SPACING 25
#define LEFT_COLUMN_WIDTH 350
#define RIGHT_COLUMN_WIDTH (WINDOW_WIDTH - LEFT_COLUMN_WIDTH - 40)

// Estrutura para controles da CPU
typedef struct {
    HWND hGroup;
    HWND hName;
    HWND hCores;
    HWND hUsage;
    HWND hSpeed;
} CpuControls;

// Variáveis de controle
static CpuControls cpuControls;
HWND hHardwarePanel;
HWND hRamGroup, hDiskGroup, hOsGroup, hGpuGroup, hBatteryGroup, hSystemGroup;
HWND hLabelRam, hLabelDisk, hLabelOs, hLabelGpu, hLabelBattery, hLabelSystem, hLabelUptime;

static FILETIME prevIdleTime = {0}, prevKernelTime = {0}, prevUserTime = {0};

// Função para verificar se o Windows é 64 bits
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

// Converter FILETIME para ULONGLONG
ULONGLONG FileTimeToULL(FILETIME ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

// Calcular a utilização da CPU em %
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

void AddHardwarePanel(HWND hwndParent) {
    // Painel principal com margens ajustadas
    hHardwarePanel = CreateWindowEx(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 40, WINDOW_WIDTH - 20, WINDOW_HEIGHT - 50,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);

    ShowWindow(hHardwarePanel, SW_HIDE);

    // Configurações de estilo
    const char* groupBoxClass = "BUTTON";
    DWORD groupBoxStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
    DWORD valueStyle = WS_CHILD | WS_VISIBLE | SS_LEFT;

    // Constantes de layout ajustadas
    const int GROUPBOX_TOP_MARGIN = 25;  // Aumentado para melhor visualização
    const int UPTIME_TOP_MARGIN = 30;    // Margem acima do uptime
    const int UPTIME_HEIGHT = 25;        // Altura do campo uptime

    // Posicionamento inicial
    int yPos = 10;

    // --- COLUNA ESQUERDA ---
    
    // Seção CPU (altura ajustada)
    cpuControls.hGroup = CreateWindowEx(0, groupBoxClass, "CPU",
        groupBoxStyle,
        10, yPos, LEFT_COLUMN_WIDTH, 130,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    cpuControls.hName = CreateWindowEx(0, "STATIC", "Model: Loading...",
        valueStyle,
        20, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 20,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    cpuControls.hCores = CreateWindowEx(0, "STATIC", "Cores: Loading...",
        valueStyle,
        20, yPos + GROUPBOX_TOP_MARGIN + ITEM_SPACING, LEFT_COLUMN_WIDTH - 20, 20,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    cpuControls.hUsage = CreateWindowEx(0, "STATIC", "Usage: 0%",
        valueStyle,
        20, yPos + GROUPBOX_TOP_MARGIN + ITEM_SPACING*2, LEFT_COLUMN_WIDTH - 20, 20,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    cpuControls.hSpeed = CreateWindowEx(0, "STATIC", "Speed: 0.00 GHz",
        valueStyle,
        20, yPos + GROUPBOX_TOP_MARGIN + ITEM_SPACING*3, LEFT_COLUMN_WIDTH - 20, 20,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    yPos += 130 + SECTION_SPACING;

    // Seção RAM (altura ajustada)
    hRamGroup = CreateWindowEx(0, groupBoxClass, "Memory",
        groupBoxStyle,
        10, yPos, LEFT_COLUMN_WIDTH, 75,  // Aumentado para 75
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    hLabelRam = CreateWindowEx(0, "STATIC", "Total: Loading...\nFree: Loading...",
        valueStyle | SS_NOPREFIX,
        20, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45,  // Aumentado para 45
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    yPos += 75 + SECTION_SPACING;  // Ajustado para 75

    // Seção Disco (altura ajustada)
    hDiskGroup = CreateWindowEx(0, groupBoxClass, "Storage",
        groupBoxStyle,
        10, yPos, LEFT_COLUMN_WIDTH, 75,  // Aumentado para 75
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    hLabelDisk = CreateWindowEx(0, "STATIC", "Total: Loading...\nFree: Loading...",
        valueStyle | SS_NOPREFIX,
        20, yPos + GROUPBOX_TOP_MARGIN, LEFT_COLUMN_WIDTH - 20, 45,  // Aumentado para 45
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    // --- COLUNA DIREITA ---
    yPos = 10;

    // Seção Sistema Operacional (altura ajustada)
    hOsGroup = CreateWindowEx(0, groupBoxClass, "Operating System",
        groupBoxStyle,
        LEFT_COLUMN_WIDTH + 20, yPos, RIGHT_COLUMN_WIDTH, 75,  // Aumentado para 75
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    hLabelOs = CreateWindowEx(0, "STATIC", "OS: Loading...\nVersion: Loading...",
        valueStyle | SS_NOPREFIX,
        LEFT_COLUMN_WIDTH + 30, yPos + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45,  // Aumentado para 45
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    yPos += 75 + SECTION_SPACING;  // Ajustado para 75

    // Seção GPU (altura ajustada)
    hGpuGroup = CreateWindowEx(0, groupBoxClass, "Graphics",
        groupBoxStyle,
        LEFT_COLUMN_WIDTH + 20, yPos, RIGHT_COLUMN_WIDTH, 60,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    hLabelGpu = CreateWindowEx(0, "STATIC", "GPU: Loading...",
        valueStyle,
        LEFT_COLUMN_WIDTH + 30, yPos + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 20,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    yPos += 60 + SECTION_SPACING;

    // Seção Bateria (altura ajustada)
    hBatteryGroup = CreateWindowEx(0, groupBoxClass, "Power",
        groupBoxStyle,
        LEFT_COLUMN_WIDTH + 20, yPos, RIGHT_COLUMN_WIDTH, 60,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    hLabelBattery = CreateWindowEx(0, "STATIC", "Battery: Loading...",
        valueStyle,
        LEFT_COLUMN_WIDTH + 30, yPos + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 20,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    yPos += 60 + SECTION_SPACING;

    // Seção Sistema (altura ajustada)
    hSystemGroup = CreateWindowEx(0, groupBoxClass, "System",
        groupBoxStyle,
        LEFT_COLUMN_WIDTH + 20, yPos, RIGHT_COLUMN_WIDTH, 75,  // Aumentado para 75
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
    
    hLabelSystem = CreateWindowEx(0, "STATIC", "Computer: Loading...\nUser: Loading...",
        valueStyle | SS_NOPREFIX,
        LEFT_COLUMN_WIDTH + 30, yPos + GROUPBOX_TOP_MARGIN, RIGHT_COLUMN_WIDTH - 20, 45,  // Aumentado para 45
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    // Uptime reposicionado mais para cima
    hLabelUptime = CreateWindowEx(0, "STATIC", "Uptime: Loading...",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, WINDOW_HEIGHT - 100,  // Movido para cima (de -80 para -100)
        WINDOW_WIDTH - 40, UPTIME_HEIGHT,
        hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
}

void UpdateHardwareInfo() {
    char buffer[256];

    // Atualizar informações da CPU
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

    // Obter número de processadores lógicos (forma mais confiável)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD logicalProcessors = sysInfo.dwNumberOfProcessors;

    double cpuUsage = CalculateCpuUsage();
    double clockSpeed = (double)cpuMHz / 1000.0;

    SetWindowText(cpuControls.hName, cpuName);
    snprintf(buffer, sizeof(buffer), "Logical Processors: %d", logicalProcessors);
    SetWindowText(cpuControls.hCores, buffer);
    snprintf(buffer, sizeof(buffer), "Usage: %d%%", (int)cpuUsage);
    SetWindowText(cpuControls.hUsage, buffer);
    snprintf(buffer, sizeof(buffer), "Speed: %.2f GHz", clockSpeed);
    SetWindowText(cpuControls.hSpeed, buffer);

    // Atualizar informações da RAM
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    DWORDLONG availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    DWORD usagePercent = memInfo.dwMemoryLoad;

    snprintf(buffer, sizeof(buffer), "Total: %llu MB\nFree: %llu MB (%lu%%)", totalRamMB, availRamMB, usagePercent);
    SetWindowText(hLabelRam, buffer);

    // Atualizar informações do Disco
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceEx(NULL, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        DWORDLONG totalDiskGB = totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024);
        DWORDLONG freeDiskGB = totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024);
        DWORD usagePercentDisk = 100 - (DWORD)((freeDiskGB * 100) / totalDiskGB);

        snprintf(buffer, sizeof(buffer), "Total: %llu GB\nFree: %llu GB (%lu%%)", totalDiskGB, freeDiskGB, usagePercentDisk);
        SetWindowText(hLabelDisk, buffer);
    }

    // Atualizar informações do SO
    HKEY hKeyOS;
    char osName[256] = "Unknown OS";
    char osDisplayVersion[256] = "Unknown Version";
    char osBuild[256] = "Unknown Build";
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKeyOS) == ERROR_SUCCESS) {
        DWORD size = sizeof(osName);
        RegQueryValueEx(hKeyOS, "ProductName", NULL, NULL, (LPBYTE)osName, &size);

        size = sizeof(osDisplayVersion);
        RegQueryValueEx(hKeyOS, "DisplayVersion", NULL, NULL, (LPBYTE)osDisplayVersion, &size);

        size = sizeof(osBuild);
        RegQueryValueEx(hKeyOS, "CurrentBuild", NULL, NULL, (LPBYTE)osBuild, &size);

        RegCloseKey(hKeyOS);
    }

    snprintf(buffer, sizeof(buffer), "OS: %s\nVersion: %s (Build %s)", osName, osDisplayVersion, osBuild);
    SetWindowText(hLabelOs, buffer);

    // Atualizar Uptime
    ULONGLONG uptimeMillis = GetTickCount();
    ULONGLONG uptimeSecs = uptimeMillis / 1000;
    DWORD days = (DWORD)(uptimeSecs / 86400);
    DWORD hours = (DWORD)((uptimeSecs % 86400) / 3600);
    DWORD minutes = (DWORD)((uptimeSecs % 3600) / 60);
    DWORD seconds = (DWORD)(uptimeSecs % 60);

    snprintf(buffer, sizeof(buffer), "Uptime: %u:%02u:%02u:%02u", days, hours, minutes, seconds);
    SetWindowText(hLabelUptime, buffer);

    // Atualizar informações da GPU
    DISPLAY_DEVICE dd;
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(DISPLAY_DEVICE);
    
    if (EnumDisplayDevices(NULL, 0, &dd, 0)) {
        for (int i = strlen(dd.DeviceString) - 1; i >= 0 && isspace(dd.DeviceString[i]); i--) {
            dd.DeviceString[i] = '\0';
        }
        snprintf(buffer, sizeof(buffer), "GPU: %s", dd.DeviceString);
    } else {
        snprintf(buffer, sizeof(buffer), "GPU: Not detected");
    }
    SetWindowText(hLabelGpu, buffer);

    // Atualizar informações da Bateria
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
    SetWindowText(hLabelBattery, buffer);

    // Atualizar informações do Sistema
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD sizeName = sizeof(computerName);
    GetComputerNameA(computerName, &sizeName);

    char userName[256];
    DWORD sizeUser = sizeof(userName);
    GetUserNameA(userName, &sizeUser);

    HKEY hKey;
    char manufacturer[256] = "Unknown";
    char model[256] = "Unknown";
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD bufSize = sizeof(manufacturer);
        RegQueryValueEx(hKey, "SystemManufacturer", NULL, NULL, (LPBYTE)manufacturer, &bufSize);

        bufSize = sizeof(model);
        RegQueryValueEx(hKey, "SystemProductName", NULL, NULL, (LPBYTE)model, &bufSize);
        RegCloseKey(hKey);
    }

    snprintf(buffer, sizeof(buffer), "Computer: %s (%s %s)\nUser: %s", 
             computerName, manufacturer, model, userName);
    SetWindowText(hLabelSystem, buffer);
}