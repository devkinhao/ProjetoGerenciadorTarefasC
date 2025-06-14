#ifndef CONFIG_H
#define CONFIG_H

// Definições de compatibilidade com Windows
#define _WIN32_IE 0x0700          // Versão mínima do Internet Explorer para compatibilidade
#define _WIN32_WINNT 0x0A00       // Versão mínima do Windows (Windows 10)

// Configurações de interface
#define FONT_SIZE 15              // Tamanho padrão da fonte
extern int gWindowWidth;          // Largura da janela principal (definida em main.c)
extern int gWindowHeight;         // Altura da janela principal (definida em main.c)

// IDs dos controles da interface
#define ID_TAB_CONTROL     1001   // ID da aba de navegação
#define ID_LIST_VIEW       1002   // ID da lista de processos
#define ID_END_TASK_BUTTON 1003   // ID do botão "Encerrar tarefa"

// Tamanho do buffer para exibição de uso de CPU
#define CPU_USAGE_BUFFER_SIZE 16

// Inclusão de headers necessários
#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winbase.h>
#include <ctype.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <powrprof.h>
#include <dxgi.h>
#include <setupapi.h>
#include <devguid.h>     // Para GUIDs de dispositivos
#include <regstr.h>   
#include <UxTheme.h>

// Links para as bibliotecas necessárias
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "version.lib")

// Variáveis globais compartilhadas
extern SYSTEM_INFO sysInfo;       // Informações do sistema
extern int numProcessors;         // Número de processadores
extern HWND hTab, hListView, hButtonEndTask, hHardwarePanel;  // Handles dos controles
extern HFONT hFontTabs, hFontButton;  // Handles das fontes
extern HBRUSH hbrBackground;      // Pincel para fundo
extern COLORREF clrBackground;    // Cor de fundo

#endif // CONFIG_H