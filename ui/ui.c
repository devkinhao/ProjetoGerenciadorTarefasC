#include "ui.h"
#include "../utils/utils.h"
#include "../hardware/hardware.h"

static HWND* checkboxes = NULL;

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

// Função para atualizar o estado do botão OK
void UpdateOkButtonState(HWND hDlg, HWND hOk) {
    BOOL enableOk = FALSE;
    for (int i = 0; i < numProcessors; i++) {
        if (SendMessage(checkboxes[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
            enableOk = TRUE;
            break;
        }
    }

    // Habilitar/Desabilitar o botão OK
    EnableWindow(hOk, enableOk);
}

// Função de Callback para a janela de diálogo (modal)
INT_PTR CALLBACK AffinityDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static HANDLE hProcess;
    static HWND hAllProcessors;
    static HWND hOk;
    static char processName[MAX_PATH];

    switch (message) {
        case WM_INITDIALOG: {
            // Se lParam é um struct, adapte aqui.
            hProcess = (HANDLE)lParam;
            GetWindowTextA(hDlg, processName, sizeof(processName)); // OU receba corretamente por lParam

            DWORD_PTR processAffinity, systemAffinity;
            if (!GetProcessAffinityMask(hProcess, &processAffinity, &systemAffinity)) {
                MessageBox(hDlg, "Error getting affinity", "Error", MB_OK | MB_ICONERROR);
                EndDialog(hDlg, 0);
                return FALSE;
            }

            HFONT hFont = CreateFontForControl();

            // Label com nome do processo
            char labelText[256];
            snprintf(labelText, sizeof(labelText), "Which processors are allowed to run \"%s\"?", processName);
            HWND hLabel = CreateWindow("STATIC", labelText, WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 10, 260, 30, hDlg, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

            // "<All Processors>" checkbox
            hAllProcessors = CreateWindow("BUTTON", "<All Processors>", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 20, 50, 150, 25, hDlg, (HMENU)1000, GetModuleHandle(NULL), NULL);
            SendMessage(hAllProcessors, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Checkboxes por CPU
            checkboxes = malloc(sizeof(HWND) * numProcessors);
            BOOL allChecked = TRUE;
            for (int i = 0; i < numProcessors; i++) {
                char label[32];
                snprintf(label, sizeof(label), "CPU %d", i);
                checkboxes[i] = CreateWindow("BUTTON", label, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    20, 80 + i * 30, 100, 25, hDlg, (HMENU)(UINT_PTR)(1001 + i), GetModuleHandle(NULL), NULL);       
                SendMessage(checkboxes[i], WM_SETFONT, (WPARAM)hFont, TRUE);

                if (processAffinity & ((DWORD_PTR)1 << i)) {
                    SendMessage(checkboxes[i], BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    allChecked = FALSE;
                }
            }
            // Marcar <All Processors> se todas estão marcadas
            SendMessage(hAllProcessors, BM_SETCHECK, allChecked ? BST_CHECKED : BST_UNCHECKED, 0);

            // Tamanho dos botões
            int buttonWidth = 60;
            int buttonHeight = 25;
            int buttonSpacing = 10;

            // Posição Y: 10 pixels acima do fundo
            RECT clientRect;
            GetClientRect(hDlg, &clientRect);
            int y = clientRect.bottom - buttonHeight - 10;

            // Posição X centralizada
            int totalWidth = buttonWidth * 2 + buttonSpacing;
            int xOk = (clientRect.right - totalWidth) / 2;
            int xCancel = xOk + buttonWidth + buttonSpacing;

            hOk = CreateWindow("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_DISABLED,
                xOk, y, buttonWidth, buttonHeight, hDlg, (HMENU)9999, GetModuleHandle(NULL), NULL);
            HWND hCancel = CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                xCancel, y, buttonWidth, buttonHeight, hDlg, (HMENU)9998, GetModuleHandle(NULL), NULL);
            
            SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

            UpdateOkButtonState(hDlg, hOk);

            // Centraliza a janela
            RECT rc;
            GetWindowRect(hDlg, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;
            CenterWindowRelativeToParent(hDlg, width, height);

            return TRUE;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);

            if (wmId == 1000) { // "<All Processors>"
                BOOL check = (SendMessage(hAllProcessors, BM_GETCHECK, 0, 0) == BST_CHECKED);
                for (int i = 0; i < numProcessors; i++) {
                    SendMessage(checkboxes[i], BM_SETCHECK, check ? BST_CHECKED : BST_UNCHECKED, 0);
                }
                UpdateOkButtonState(hDlg, hOk);

            } else if (wmId >= 1001 && wmId < 1001 + numProcessors) { // CPUs individuais
                // Atualiza <All Processors> com base nos checkboxes
                BOOL allCheckedNow = TRUE;
                for (int i = 0; i < numProcessors; i++) {
                    if (SendMessage(checkboxes[i], BM_GETCHECK, 0, 0) != BST_CHECKED) {
                        allCheckedNow = FALSE;
                        break;
                    }
                }
                SendMessage(hAllProcessors, BM_SETCHECK, allCheckedNow ? BST_CHECKED : BST_UNCHECKED, 0);
                UpdateOkButtonState(hDlg, hOk);

            } else if (wmId == 9999) { // OK
                DWORD_PTR newMask = 0;
                for (int i = 0; i < numProcessors; i++) {
                    if (SendMessage(checkboxes[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        newMask |= ((DWORD_PTR)1 << i);
                    }
                }

                if (newMask == 0) {
                    MessageBox(hDlg, "Select at least one CPU", "Warning", MB_OK | MB_ICONWARNING);
                } else {
                    if (!SetProcessAffinityMask(hProcess, newMask)) {
                        MessageBox(hDlg, "Error setting affinity", "Error", MB_OK | MB_ICONERROR);
                    } else {
                        MessageBox(hDlg, "Affinity set successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                        EndDialog(hDlg, 1);
                    }
                }

            } else if (wmId == 9998) { // Cancel
                EndDialog(hDlg, 0);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;
    }

    return FALSE;
}

// Função principal para mostrar a janela de afinidade
void ShowAffinityDialog(HWND hwndParent, HANDLE hProcess, const char* processName) {
    // Janela de diálogo modal
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(101), hwndParent, AffinityDialogProc, (LPARAM)hProcess);
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

    // Obter o nome do processo (subitem 0, por exemplo)
    char processName[MAX_PATH] = {0};
    LVITEM nameItem = {0};
    nameItem.iSubItem = 0;
    nameItem.iItem = selectedIndex;
    nameItem.mask = LVIF_TEXT;
    nameItem.pszText = processName;
    nameItem.cchTextMax = sizeof(processName);
    ListView_GetItem(hwndListView, &nameItem);

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
        ShowAffinityDialog(hwndParent, hProcess, processName);
    }

    CloseHandle(hProcess);
    DestroyMenu(hMenu);
}