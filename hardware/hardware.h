#ifndef HARDWARE_H
#define HARDWARE_H

#include "..\config.h"

// Declarações de funções para a aba Hardware
BOOL Is64BitWindows();                      // Verifica se o Windows é 64-bit
ULONGLONG FileTimeToULL(FILETIME ft);       // Converte FILETIME para ULONGLONG
HWND CreateLabel(HWND parent, const char* text, int x, int y, int width, int height, DWORD alignStyle);  // Cria um label
HWND CreateGroupBox(HWND parent, const char* title, int x, int y, int width, int height);  // Cria um group box
double CalculateCpuUsage();                 // Calcula uso total da CPU
void UpdateCpuInfo();                       // Atualiza informações da CPU
void UpdateRamInfo();                       // Atualiza informações de memória
int UpdateDiskInfo(HWND hDiskLabel);        // Atualiza informações de disco
void UpdateOSInfo(HWND hLabelOs);           // Atualiza informações do SO
void UpdateUptimeInfo(HWND hLabelUptime);   // Atualiza tempo de atividade
int UpdateGPUInfo(HWND hLabelGpu);          // Atualiza informações da GPU
void UpdateBatteryInfo(HWND hLabelBattery); // Atualiza informações da bateria
void UpdateSystemInfo(HWND hLabelSystem);   // Atualiza informações do sistema
DWORD GetDefaultInterfaceIndex();           // Obtém índice da interface de rede padrão
void GetNetworkUsage(char* outBuffer, size_t bufSize);  // Obtém uso da rede
void AddHardwarePanel(HWND hwndParent);     // Adiciona o painel de hardware
void UpdateHardwareInfo(void);              // Atualiza todas as informações de hardware

// Variáveis globais dos labels (definidas em hardware.c)
extern HWND hLabelCpu, hLabelRam, hLabelDisk, hLabelOs, hLabelUptime;

#endif // HARDWARE_H