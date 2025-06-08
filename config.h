#ifndef CONFIG_H
#define CONFIG_H

// Compatibilidade com Windows
#define _WIN32_IE 0x0700 // Mínimo para IE7, geralmente bom para controles modernos
#define _WIN32_WINNT 0x0A00 // Windows 10 para os recursos mais recentes

// Interface gráfica
#define FONT_SIZE 15
extern int gWindowWidth;
extern int gWindowHeight;

// Identificadores de controles
#define ID_TAB_CONTROL     1001
#define ID_LIST_VIEW       1002
#define ID_END_TASK_BUTTON 1003

// Tamanho do buffer de CPU
#define CPU_USAGE_BUFFER_SIZE 16

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
#include <devguid.h>     // GUID_DEVCLASS_DISPLAY
#include <regstr.h>   
#include <UxTheme.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Psapi.lib")

// Variáveis globais compartilhadas
extern SYSTEM_INFO sysInfo;
extern int numProcessors;
extern HWND hTab, hListView, hButtonEndTask, hHardwarePanel;
extern HFONT hFontTabs, hFontButton;

#endif // CONFIG_H
