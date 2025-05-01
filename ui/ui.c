#include "ui.h"
#include "../utils/utils.h"
#include "../hardware/hardware.h"

void AddTabs(HWND hwndParent) {
    hTab = CreateWindowEx(0, WC_TABCONTROL, "",
        WS_CHILD | WS_VISIBLE,
        0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        hwndParent, (HMENU)ID_TAB_CONTROL, GetModuleHandle(NULL), NULL);

    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = "Processes";
    TabCtrl_InsertItem(hTab, 0, &tie);
    tie.pszText = "Hardware";
    TabCtrl_InsertItem(hTab, 1, &tie);

    hFontTabs = CreateFontForControl();
    SendMessage(hTab, WM_SETFONT, (WPARAM)hFontTabs, TRUE);
}

void AddListView(HWND hwndParent) {
    hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 40, LISTVIEW_WIDTH, LISTVIEW_HEIGHT,
        hwndParent, (HMENU)ID_LIST_VIEW, GetModuleHandle(NULL), NULL);

    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    // Agora temos 7 colunas
    char* columns[] = { "Process Name", "User", "PID", "Status", "CPU (%)", "Memory (MB)", "Disk (MB/s)" };
    int widths[] = { 200, 99, 70, 100, 80, 100, 100 };

    for (int i = 0; i < 7; i++) {
        lvc.pszText = columns[i];
        lvc.cx = widths[i];
        ListView_InsertColumn(hListView, i, &lvc);
    }
}

void AddFooter(HWND hwndParent) {
    hButtonEndTask = CreateWindowEx(0, "BUTTON", "End Task",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        600, 575, 180, 30,
        hwndParent, (HMENU)ID_END_TASK_BUTTON, GetModuleHandle(NULL), NULL);

    hFontButton = CreateFontForControl();

    SendMessage(hButtonEndTask, WM_SETFONT, (WPARAM)hFontButton, TRUE);
}

void SetupTimer(HWND hwnd) {
    SetTimer(hwnd, 1, 1000, NULL);
}

void OnTabSelectionChanged(HWND hwndParent, int selectedTab) {
    ShowWindow(hListView, selectedTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hHardwarePanel, selectedTab == 1 ? SW_SHOW : SW_HIDE);

    if (selectedTab == 0) {
        ShowWindow(hButtonEndTask, SW_SHOW);
    } else {
        ShowWindow(hButtonEndTask, SW_HIDE);
        UpdateHardwareInfo(); // <- Atualiza info quando mudar para a aba 1 (Hardware)
    }
}

void CleanupResources() {
    if (hFontTabs) DeleteObject(hFontTabs);
    if (hFontButton) DeleteObject(hFontButton);
}

// Criação de tela modal para afinidade de CPU
void ShowAffinityDialog(HWND hwndParent, HANDLE hProcess) {
    DWORD_PTR processAffinity, systemAffinity;
    if (!GetProcessAffinityMask(hProcess, &processAffinity, &systemAffinity)) {
        MessageBox(hwndParent, "Error getting affinity", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    HWND hDlg = CreateWindowEx(WS_EX_DLGMODALFRAME, "STATIC", "Set Affinity",
        WS_POPUPWINDOW | WS_CAPTION | WS_SYSMENU,
        300, 200, 300, 100 + 30 * numProcessors,
        hwndParent, NULL, GetModuleHandle(NULL), NULL);

    HWND* checkboxes = malloc(sizeof(HWND) * numProcessors);
    for (int i = 0; i < numProcessors; i++) {
        char label[32];
        snprintf(label, sizeof(label), "CPU %d", i);
        checkboxes[i] = CreateWindow("BUTTON", label,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            20, 20 + i * 30, 100, 25,
            hDlg, (HMENU)(1000 + i), GetModuleHandle(NULL), NULL);

        if (processAffinity & ((DWORD_PTR)1 << i)) {
            SendMessage(checkboxes[i], BM_SETCHECK, BST_CHECKED, 0);
        }
    }

    HWND hOk = CreateWindow("BUTTON", "OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        140, 30 * numProcessors + 30, 60, 25,
        hDlg, (HMENU)9999, GetModuleHandle(NULL), NULL);

    HWND hCancel = CreateWindow("BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        210, 30 * numProcessors + 30, 70, 25,
        hDlg, (HMENU)9998, GetModuleHandle(NULL), NULL);

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg;
    BOOL running = TRUE;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_COMMAND) {
            int wmId = LOWORD(msg.wParam);
            if (wmId == 9999) { // OK
                DWORD_PTR newMask = 0;
                for (int i = 0; i < numProcessors; i++) {
                    if (SendMessage(checkboxes[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        newMask |= ((DWORD_PTR)1 << i);
                    }
                }
                if (newMask == 0) {
                    MessageBox(hwndParent, "Select at least one CPU", "Warning", MB_OK | MB_ICONWARNING);
                } else {
                    if (!SetProcessAffinityMask(hProcess, newMask)) {
                        MessageBox(hwndParent, "Error setting affinitty", "Error", MB_OK | MB_ICONERROR);
                    } else {
                        MessageBox(hwndParent, "Affinity set successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                        running = FALSE;
                    }
                }
            } else if (wmId == 9998) { // Cancelar
                running = FALSE;
            }
        } else if (msg.message == WM_CLOSE) {
            running = FALSE;
        }
    }

    for (int i = 0; i < numProcessors; i++) {
        DestroyWindow(checkboxes[i]);
    }
    free(checkboxes);
    DestroyWindow(hDlg);
}

void ShowContextMenu(HWND hwndListView, HWND hwndParent, POINT pt) {
    int selectedIndex = ListView_GetNextItem(hwndListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1) return;

    char pidText[16];
    LVITEM item = {0};
    item.iSubItem = 2;
    item.iItem = selectedIndex;
    item.mask = LVIF_TEXT;
    item.pszText = pidText;
    item.cchTextMax = sizeof(pidText);
    if (!ListView_GetItem(hwndListView, &item)) return;

    DWORD pid = strtoul(pidText, NULL, 10);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        MessageBox(hwndParent, "Unable to open process", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    DWORD currentPriority = GetPriorityClass(hProcess);

    HMENU hMenu = CreatePopupMenu();
    HMENU hPriorityMenu = CreatePopupMenu();

    AppendMenu(hPriorityMenu, MF_STRING | (currentPriority == REALTIME_PRIORITY_CLASS ? MF_CHECKED : 0), 101, "Realtime");
    AppendMenu(hPriorityMenu, MF_STRING | (currentPriority == HIGH_PRIORITY_CLASS ? MF_CHECKED : 0), 102, "High");
    AppendMenu(hPriorityMenu, MF_STRING | (currentPriority == ABOVE_NORMAL_PRIORITY_CLASS ? MF_CHECKED : 0), 103, "Above Normal");
    AppendMenu(hPriorityMenu, MF_STRING | (currentPriority == NORMAL_PRIORITY_CLASS ? MF_CHECKED : 0), 104, "Normal");
    AppendMenu(hPriorityMenu, MF_STRING | (currentPriority == BELOW_NORMAL_PRIORITY_CLASS ? MF_CHECKED : 0), 105, "Below Normal");
    AppendMenu(hPriorityMenu, MF_STRING | (currentPriority == IDLE_PRIORITY_CLASS ? MF_CHECKED : 0), 106, "Low");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPriorityMenu, "Set priority");
    AppendMenu(hMenu, MF_STRING, 2, "Set affinity");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwndParent, NULL);

    if (cmd >= 101 && cmd <= 106) {
        DWORD priority;
        switch (cmd) {
            case 101: priority = REALTIME_PRIORITY_CLASS; break;
            case 102: priority = HIGH_PRIORITY_CLASS; break;
            case 103: priority = ABOVE_NORMAL_PRIORITY_CLASS; break;
            case 104: priority = NORMAL_PRIORITY_CLASS; break;
            case 105: priority = BELOW_NORMAL_PRIORITY_CLASS; break;
            case 106: priority = IDLE_PRIORITY_CLASS; break;
        }

        if (SetPriorityClass(hProcess, priority)) {
            MessageBox(hwndParent, "Priority set successfully!", "Success", MB_OK);
        } else {
            MessageBox(hwndParent, "Failed to set priority", "Error", MB_OK | MB_ICONERROR);
        }
    } else if (cmd == 2) {
        ShowAffinityDialog(hwndParent, hProcess);
    }

    CloseHandle(hProcess);
    DestroyMenu(hMenu);
}