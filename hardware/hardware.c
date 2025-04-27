#include "hardware.h"
#include "..\utils\utils.h"
#include <winbase.h>
#pragma comment(lib, "Shlwapi.lib")

HWND hLabelCpu, hLabelRam, hLabelDisk, hLabelOs, hLabelUptime;

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

    double cpuUsage = (double)(totalSystem - idleDiff) * 100.0 / totalSystem;
    return cpuUsage;
}

// Função para obter o clock da CPU em tempo real (em GHz)
double GetCpuClockSpeed() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD clockSpeed = sysInfo.dwProcessorType; // Padrão, obtém tipo do processador
    return (double)clockSpeed / 1000.0;  // Converte para GHz
}

void AddHardwarePanel(HWND hwndParent) {
    hHardwarePanel = CreateWindowEx(0, "STATIC", "",
        WS_CHILD | WS_VISIBLE,
        10, 40, LISTVIEW_WIDTH, LISTVIEW_HEIGHT,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);

    ShowWindow(hHardwarePanel, SW_HIDE);

    hLabelCpu = CreateWindowEx(0, "STATIC", "CPU: ", WS_CHILD | WS_VISIBLE,
        20, 20, 500, 30, hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    hLabelRam = CreateWindowEx(0, "STATIC", "RAM: ", WS_CHILD | WS_VISIBLE,
        20, 60, 500, 30, hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    hLabelDisk = CreateWindowEx(0, "STATIC", "DISK: ", WS_CHILD | WS_VISIBLE,
        20, 100, 500, 30, hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    hLabelOs = CreateWindowEx(0, "STATIC", "OS: ", WS_CHILD | WS_VISIBLE,
        20, 140, 500, 30, hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);

    hLabelUptime = CreateWindowEx(0, "STATIC", "Uptime: ", WS_CHILD | WS_VISIBLE,
        20, 180, 500, 30, hHardwarePanel, NULL, GetModuleHandle(NULL), NULL);
}

void UpdateHardwareInfo() {
    // CPU Info
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

    const char* arch = Is64BitWindows() ? "x64" : "x86";

    // CPU Usage
    double cpuUsage = CalculateCpuUsage();

    // Arredondar o valor para um inteiro (sem casas decimais)
    int cpuUsageInt = (int)cpuUsage;

    // Clock Speed
    double clockSpeed = GetCpuClockSpeed();  // Obtém o clock em tempo real

    char cpuText[512];
    snprintf(cpuText, sizeof(cpuText), "CPU: %s (%s) - Usage: %d%% - Speed: %.2f GHz", cpuName, arch, cpuUsageInt, clockSpeed);
    SetWindowText(hLabelCpu, cpuText);

    // RAM Info
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);

    DWORDLONG totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
    DWORDLONG availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    DWORD usagePercent = memInfo.dwMemoryLoad;

    char ramText[512];
    snprintf(ramText, sizeof(ramText), "RAM: %llu MB total / %llu MB free (%lu%% used)", totalRamMB, availRamMB, usagePercent);
    SetWindowText(hLabelRam, ramText);

    // Disk Info
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceEx(NULL, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        DWORDLONG totalDiskGB = totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024);
        DWORDLONG freeDiskGB = totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024);
        DWORD usagePercentDisk = 100 - (DWORD)((freeDiskGB * 100) / totalDiskGB);

        char diskText[512];
        snprintf(diskText, sizeof(diskText), "DISK: %llu GB total / %llu GB free (%lu%% used)", totalDiskGB, freeDiskGB, usagePercentDisk);
        SetWindowText(hLabelDisk, diskText);
    }

    // OS Info (versão detalhada e build)
    HKEY hKeyOS;
    char osName[256] = "Unknown OS";
    char osDisplayVersion[256] = "Unknown Display Version";  // Alterado para DisplayVersion
    char osBuild[256] = "Unknown Build";
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKeyOS) == ERROR_SUCCESS) {
        DWORD size = sizeof(osName);
        RegQueryValueEx(hKeyOS, "ProductName", NULL, NULL, (LPBYTE)osName, &size);

        size = sizeof(osDisplayVersion);
        RegQueryValueEx(hKeyOS, "DisplayVersion", NULL, NULL, (LPBYTE)osDisplayVersion, &size);  // Usando DisplayVersion

        size = sizeof(osBuild);
        RegQueryValueEx(hKeyOS, "CurrentBuild", NULL, NULL, (LPBYTE)osBuild, &size);  // 19045.5737

        RegCloseKey(hKeyOS);
    }

    char osText[512];
    snprintf(osText, sizeof(osText), "OS: %s %s (OS build %s)", osName, osDisplayVersion, osBuild);  // Atualizado para usar DisplayVersion
    SetWindowText(hLabelOs, osText);

    // Uptime
    // Uptime
    ULONGLONG uptimeMillis = GetTickCount();  // Usando GetTickCount64 para maior precisão
    ULONGLONG uptimeSecs = uptimeMillis / 1000;
    DWORD days = (DWORD)(uptimeSecs / 86400);  // Número de dias (1 dia = 86400 segundos)
    DWORD hours = (DWORD)((uptimeSecs % 86400) / 3600);  // Horas restantes
    DWORD minutes = (DWORD)((uptimeSecs % 3600) / 60);  // Minutos restantes
    DWORD seconds = (DWORD)(uptimeSecs % 60);  // Segundos restantes

    char uptimeText[512];
    snprintf(uptimeText, sizeof(uptimeText), "Uptime: %02u:%02u:%02u:%02u", days, hours, minutes, seconds);
    SetWindowText(hLabelUptime, uptimeText);

}