#ifndef CONFIG_H
#define CONFIG_H

#define _WIN32_IE 0x0400
#define _WIN32_WINNT 0x0501
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 650
#define FONT_SIZE 16
#define LISTVIEW_WIDTH 770
#define LISTVIEW_HEIGHT 520
#define CPU_USAGE_BUFFER_SIZE 16  // Define a buffer size for saf

#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <psapi.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Psapi.lib")

#define ID_TAB_CONTROL 1001
#define ID_LIST_VIEW 1002
#define ID_END_TASK_BUTTON 1003

SYSTEM_INFO sysInfo;
int numProcessors;

HWND hTab, hListView, hButtonEndTask, hHardwarePanel;
HFONT hFontTabs, hFontButton;

//static BOOL firstTime = TRUE;

#endif  // CONFIG_H
