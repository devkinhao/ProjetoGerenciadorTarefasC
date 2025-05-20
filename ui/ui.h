#ifndef UI_H
#define UI_H

#include "..\config.h"

typedef struct {
    HANDLE hProcess;
    char processName[MAX_PATH];
    DWORD pid;
} AffinityDialogParams;

void AddTabs(HWND hwndParent);
void AddListView(HWND hwndParent);
void AddFooter(HWND hwndParent);
void SetupTimer(HWND hwnd);
void OnTabSelectionChanged(HWND hwndParent, int selectedTab);
void CleanupResources();
void UpdateOkButtonState(HWND hDlg, HWND hOk);
INT_PTR CALLBACK AffinityDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowAffinityDialog(HWND hwndParent, HANDLE hProcess, const char* processName);
INT_PTR CALLBACK ShowProcessDetailsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ShowProcessDetailsDialog(HWND hwndParent, HANDLE hProcess, const char* processName, DWORD pid);
void ShowContextMenu(HWND hwndListView, HWND hwndParent, POINT pt);


#endif // UI_H
