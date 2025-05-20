#include "ui.h"
#include "../utils/utils.h"
#include "../hardware/hardware.h"
#pragma comment(lib, "version.lib")

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
    static AffinityDialogParams* params;
    static HWND hAllProcessors;
    static HWND hOk;

    switch (message) {
        case WM_INITDIALOG: {
            params = (AffinityDialogParams*)lParam;
            
            DWORD_PTR processAffinity, systemAffinity;
            if (!GetProcessAffinityMask(params->hProcess, &processAffinity, &systemAffinity)) {
                MessageBox(hDlg, "Error getting affinity", "Error", MB_OK | MB_ICONERROR);
                EndDialog(hDlg, 0);
                return FALSE;
            }

            HFONT hFont = CreateFontForControl();

            // Obter tamanho da área cliente
            RECT rcClient;
            GetClientRect(hDlg, &rcClient);
            int clientWidth = rcClient.right - rcClient.left;
            int clientHeight = rcClient.bottom - rcClient.top;

            // Label com nome do processo
            char labelText[256];
            snprintf(labelText, sizeof(labelText), "Which processors are allowed to run \"%s\"?", params->processName);
            HWND hLabel = CreateWindow("STATIC", labelText, WS_CHILD | WS_VISIBLE | SS_LEFT, 
                                      10, 10, clientWidth - 20, 30, hDlg, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

            // "<All Processors>" checkbox
            hAllProcessors = CreateWindow("BUTTON", "<All Processors>", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 
                                         20, 50, 150, 25, hDlg, (HMENU)1000, GetModuleHandle(NULL), NULL);
            SendMessage(hAllProcessors, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Calcular layout das checkboxes
            int maxHeight = 200;  // Altura máxima desejada para as checkboxes
            int checkboxHeight = 20;
            int checkboxWidth = 100;
            int startY = 80;
            int startX = 20;
            int marginRight = 20;
            
            // Calcular quantas linhas cabem na altura máxima
            int rowsPerColumn = (maxHeight - (startY - 50)) / checkboxHeight;
            if (rowsPerColumn < 1) rowsPerColumn = 1;
            
            // Calcular quantas colunas serão necessárias
            int numColumns = (numProcessors + rowsPerColumn - 1) / rowsPerColumn;
            if (numColumns < 1) numColumns = 1;
            
            // Ajustar largura da checkbox se necessário para caber na janela
            int availableWidth = clientWidth - startX - marginRight;
            if ((numColumns * checkboxWidth) > availableWidth) {
                checkboxWidth = availableWidth / numColumns;
                if (checkboxWidth < 60) checkboxWidth = 60; // Largura mínima
            }

            // Checkboxes por CPU
            checkboxes = malloc(sizeof(HWND) * numProcessors);
            BOOL allChecked = TRUE;
            
            int currentColumn = 0;
            int currentRow = 0;
            
            for (int i = 0; i < numProcessors; i++) {
                char label[32];
                snprintf(label, sizeof(label), "CPU %d", i);
                
                int x = startX + (currentColumn * checkboxWidth);
                int y = startY + (currentRow * checkboxHeight);
                
                checkboxes[i] = CreateWindow("BUTTON", label, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    x, y, checkboxWidth - 10, checkboxHeight, hDlg, (HMENU)(UINT_PTR)(1001 + i), GetModuleHandle(NULL), NULL);
                
                SendMessage(checkboxes[i], WM_SETFONT, (WPARAM)hFont, TRUE);

                if (processAffinity & ((DWORD_PTR)1 << i)) {
                    SendMessage(checkboxes[i], BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    allChecked = FALSE;
                }
                
                // Avançar para a próxima posição
                currentRow++;
                if (currentRow >= rowsPerColumn) {
                    currentRow = 0;
                    currentColumn++;
                }
            }

            // Marcar <All Processors> se todas estão marcadas
            SendMessage(hAllProcessors, BM_SETCHECK, allChecked ? BST_CHECKED : BST_UNCHECKED, 0);

            // Posicionar botões OK/Cancel
            int buttonWidth = 60;
            int buttonHeight = 25;
            int buttonSpacing = 10;
            int buttonsY = clientHeight - buttonHeight - 10;
            int totalButtonsWidth = buttonWidth * 2 + buttonSpacing;
            int buttonsX = (clientWidth - totalButtonsWidth) / 2;

            hOk = CreateWindow("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_DISABLED,
                buttonsX, buttonsY, buttonWidth, buttonHeight, hDlg, (HMENU)9999, GetModuleHandle(NULL), NULL);
            HWND hCancel = CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                buttonsX + buttonWidth + buttonSpacing, buttonsY, buttonWidth, buttonHeight, hDlg, (HMENU)9998, GetModuleHandle(NULL), NULL);
            
            SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

            UpdateOkButtonState(hDlg, hOk);

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
                    if (!SetProcessAffinityMask(params->hProcess, newMask)) {
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
    AffinityDialogParams params;
    params.hProcess = hProcess;
    strncpy(params.processName, processName, MAX_PATH - 1);
    params.processName[MAX_PATH - 1] = '\0';
    
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(101), hwndParent, AffinityDialogProc, (LPARAM)&params);
}

INT_PTR CALLBACK ShowProcessDetailsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static HANDLE hProcess;
    static char processName[MAX_PATH];
    static DWORD pid;
    static HWND hLabelMemory, hLabelCpuTime, hLabelIO, hLabelThreads, hLabelHandles;

    void UpdateDynamicLabels() {
        char buf[128];

        // Memória
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            double memMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
            snprintf(buf, sizeof(buf), "Memory: %.1f MB", memMB);
            SetWindowText(hLabelMemory, buf);
        }

        // CPU Time
        FILETIME ftCreate, ftExit, ftKernel, ftUser;
        if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
            ULONGLONG total = ((ULONGLONG)ftKernel.dwHighDateTime << 32 | ftKernel.dwLowDateTime) +
                              ((ULONGLONG)ftUser.dwHighDateTime << 32 | ftUser.dwLowDateTime);
            DWORD sec = (DWORD)(total / 10000000);
            DWORD min = sec / 60; sec %= 60;
            snprintf(buf, sizeof(buf), "CPU Time: %02lu:%02lu", min, sec);
            SetWindowText(hLabelCpuTime, buf);
        }

        // I/O
        IO_COUNTERS io;
        if (GetProcessIoCounters(hProcess, &io)) {
            double r = io.ReadTransferCount / (1024.0 * 1024.0);
            double w = io.WriteTransferCount / (1024.0 * 1024.0);
            snprintf(buf, sizeof(buf), "Total I/O: %.1f MB read / %.1f MB written", r, w);
            SetWindowText(hLabelIO, buf);
        }

        // Threads
        DWORD threads = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te = { .dwSize = sizeof(te) };
            if (Thread32First(snap, &te)) {
                do {
                    if (te.th32OwnerProcessID == pid) threads++;
                } while (Thread32Next(snap, &te));
            }
            CloseHandle(snap);
        }
        snprintf(buf, sizeof(buf), "Threads: %lu", threads);
        SetWindowText(hLabelThreads, buf);

        // Handles
        DWORD count;
        if (GetProcessHandleCount(hProcess, &count)) {
            snprintf(buf, sizeof(buf), "Handles: %lu", count);
            SetWindowText(hLabelHandles, buf);
        }
    }

    switch (message) {
    case WM_INITDIALOG: {
        hProcess = ((AffinityDialogParams*)lParam)->hProcess;
        pid = ((AffinityDialogParams*)lParam)->pid;
        strncpy(processName, ((AffinityDialogParams*)lParam)->processName, MAX_PATH);

        char fullPath[MAX_PATH] = "Unknown";
        char description[256] = "Unknown";
        char company[256] = "Unknown";

        // Caminho completo do executável
        HMODULE hMod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleFileNameEx(hProcess, hMod, fullPath, sizeof(fullPath));
        }

        // Descrição e empresa
        DWORD verHandle = 0;
        DWORD verSize = GetFileVersionInfoSize(fullPath, &verHandle);
        if (verSize > 0) {
            BYTE* verData = (BYTE*)malloc(verSize);
            if (GetFileVersionInfo(fullPath, verHandle, verSize, verData)) {
                struct LANGANDCODEPAGE {
                    WORD wLanguage;
                    WORD wCodePage;
                } *lpTranslate;
                UINT cbTranslate;
                if (VerQueryValue(verData, "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &cbTranslate)) {
                    char subBlock[128];
                    UINT size;
                    char* value;

                    snprintf(subBlock, sizeof(subBlock),
                        "\\StringFileInfo\\%04x%04x\\FileDescription",
                        lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
                    if (VerQueryValue(verData, subBlock, (LPVOID*)&value, &size) && size > 0) {
                        strncpy(description, value, sizeof(description) - 1);
                    }

                    snprintf(subBlock, sizeof(subBlock),
                        "\\StringFileInfo\\%04x%04x\\CompanyName",
                        lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);
                    if (VerQueryValue(verData, subBlock, (LPVOID*)&value, &size) && size > 0) {
                        strncpy(company, value, sizeof(company) - 1);
                    }
                }
            }
            free(verData);
        }

        // Bitness
        BOOL isWow64 = FALSE;
        IsWow64Process(hProcess, &isWow64);
        BOOL is64BitProcess = FALSE;
    #if defined(_WIN64)
        is64BitProcess = !isWow64;
    #else
        BOOL isWow64OS = FALSE;
        IsWow64Process(GetCurrentProcess(), &isWow64OS);
        is64BitProcess = isWow64OS && !isWow64;
    #endif
        const char* bitness = is64BitProcess ? "64-bit" : "32-bit";

        // Tamanho da área cliente
        RECT rcClient;
        GetClientRect(hDlg, &rcClient);
        int clientWidth = rcClient.right - rcClient.left;
        int y = 10;

        HFONT hFont = CreateFontForControl();

        // Campos estáticos
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "Process: %s", processName);
        HWND hStaticName = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticName, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "Description: %s", description);
        HWND hStaticDesc = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticDesc, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "Company: %s", company);
        HWND hStaticComp = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticComp, WM_SETFONT, (WPARAM)hFont, TRUE); y += 30;

        // Campos dinâmicos
        hLabelMemory = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelCpuTime = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelIO = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelThreads = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelHandles = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, y, clientWidth - 20, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 30;

        SendMessage(hLabelMemory, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelCpuTime, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelIO, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelThreads, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelHandles, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Botão "Close" centralizado
        int buttonWidth = 80, buttonHeight = 25;
        int buttonX = (clientWidth - buttonWidth) / 2;
        int buttonY = rcClient.bottom - buttonHeight - 10;

        HWND hClose = CreateWindow("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            buttonX, buttonY, buttonWidth, buttonHeight, hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
        SendMessage(hClose, WM_SETFONT, (WPARAM)hFont, TRUE);
        UpdateDynamicLabels();
        SetTimer(hDlg, 1, 1000, NULL);
        return TRUE;
    }

    case WM_TIMER: {
        UpdateDynamicLabels();
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            KillTimer(hDlg, 1);
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        KillTimer(hDlg, 1);
        EndDialog(hDlg, 0);
        break;
    }

    return FALSE;
}

void ShowProcessDetailsDialog(HWND hwndParent, HANDLE hProcess, const char* processName, DWORD pid) {
    AffinityDialogParams params;
    params.hProcess = hProcess;
    strncpy(params.processName, processName, MAX_PATH);
    params.pid = pid;
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(102), hwndParent, ShowProcessDetailsDialogProc, (LPARAM)&params);
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
    AppendMenu(hMenu, MF_STRING, 3, "More details");

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
    else if (cmd == 3) {
    ShowProcessDetailsDialog(hwndParent, hProcess, processName, pid);
}

    CloseHandle(hProcess);
    DestroyMenu(hMenu);
}