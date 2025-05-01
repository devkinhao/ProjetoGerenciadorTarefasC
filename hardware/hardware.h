#ifndef HARDWARE_H
#define HARDWARE_H

#include "..\config.h"

// Declaracoes de funcoes para a aba Hardware
BOOL Is64BitWindows();
ULONGLONG FileTimeToULL(FILETIME ft);
HWND CreateLabel(HWND parent, const char* text, int x, int y, int width, int height);
HWND CreateGroupBox(HWND parent, const char* title, int x, int y, int width, int height);
double CalculateCpuUsage();
void UpdateCpuInfo();
void UpdateRamInfo();
void UpdateDiskInfo(HWND hDiskLabel);
void UpdateOSInfo(HWND hLabelOs) ;
void UpdateUptimeInfo(HWND hLabelUptime);
void UpdateGPUInfo(HWND hLabelGpu);
void UpdateBatteryInfo(HWND hLabelBattery);
void UpdateSystemInfo(HWND hLabelSystem);
void AddHardwarePanel(HWND hwndParent);
void UpdateHardwareInfo(void);

// Declaracao das variaveis dos labels
extern HWND hLabelCpu, hLabelRam, hLabelDisk, hLabelOs, hLabelUptime;

#endif // HARDWARE_H