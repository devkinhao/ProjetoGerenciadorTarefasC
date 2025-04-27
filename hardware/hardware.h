#ifndef HARDWARE_H
#define HARDWARE_H

#include "..\config.h"

// Declaracoes de funcoes para a aba Hardware
void GetDiskModel(char* diskModel, size_t size);
void AddHardwarePanel(HWND hwndParent);
void UpdateHardwareInfo(void);

// Declaracao das variaveis dos labels
extern HWND hLabelCpu, hLabelRam, hLabelDisk, hLabelOs, hLabelUptime;

#endif // HARDWARE_H