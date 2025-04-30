#ifndef UI_H
#define UI_H

#include "..\config.h"

void AddTabs(HWND hwndParent);
void AddListView(HWND hwndParent);
void AddFooter(HWND hwndParent);
void SetupTimer(HWND hwnd);
void OnTabSelectionChanged(HWND hwndParent, int selectedTab);
void CleanupResources();
void ShowAffinityDialog(HWND hwndParent, HANDLE hProcess);
void ShowContextMenu(HWND hwndListView, HWND hwndParent, POINT pt);


#endif // UI_H
