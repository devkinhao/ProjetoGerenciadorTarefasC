#include "ui.h"
#include "../utils/utils.h"
#include "../hardware/hardware.h"
#pragma comment(lib, "version.lib")

static HWND* checkboxes = NULL;

void AddTabs(HWND hwndParent, int width, int height) {
    hTab = CreateWindowEx(0, WC_TABCONTROL, "",
        WS_CHILD | WS_VISIBLE,
        0, 0, width, height,
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

void AddListView(HWND hwndParent, int width, int height) {
    hListView = CreateWindowEx(0, WC_LISTVIEW, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 40, width - 25, height - 121,
        hwndParent, (HMENU)ID_LIST_VIEW, GetModuleHandle(NULL), NULL);

    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    // Agora temos 7 colunas
    char* columns[] = { "Process Name", "User", "PID", "Status", "CPU (%)", "Memory (MB)", "Disk (MB/s)", "Path" };
    int widths[] = { 200, 115, 60, 95, 65, 95, 90, 510 };

    for (int i = 0; i < 8; i++) {
        lvc.pszText = columns[i];
        lvc.cx = widths[i];
        ListView_InsertColumn(hListView, i, &lvc);
    }
}

void AddFooter(HWND hwndParent, int width, int height) {
    const int buttonWidth = 85;
    const int buttonHeight = 22;

    int x = width - buttonWidth - 15;  
    int y = height - buttonHeight - 40; 

    hButtonEndTask = CreateWindowEx(0, "BUTTON", "End Task",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, buttonWidth, buttonHeight,
        hwndParent, (HMENU)ID_END_TASK_BUTTON, GetModuleHandle(NULL), NULL);
    
    hFontButton = CreateFontForControl();
    SendMessage(hButtonEndTask, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    EnableWindow(hButtonEndTask, FALSE);
}

void SetupTimer(HWND hwnd) {
    SetTimer(hwnd, 1, 1000, NULL);
}

void OnTabSelectionChanged(HWND hwndParent, int selectedTab) {
    ShowWindow(hListView, selectedTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hHardwarePanel, selectedTab == 1 ? SW_SHOW : SW_HIDE);

    if (selectedTab == 0) {
        ShowWindow(hButtonEndTask, SW_SHOW);

        // Desabilita o botão ao voltar para a aba de processos
        EnableWindow(hButtonEndTask, FALSE);
    } else {
        ShowWindow(hButtonEndTask, SW_HIDE);
        UpdateHardwareInfo();
    }
}

void CleanupResources() {
    if (hFontTabs) DeleteObject(hFontTabs);
    if (hFontButton) DeleteObject(hFontButton);
}

void UpdateEndTaskButtonState() {
    int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    EnableWindow(hButtonEndTask, selectedIndex != -1);
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
    static HWND hLabelPriority, hLabelMemory, hLabelPrivateMemory, hLabelPageFaults, hLabelCpuTime, hLabelIO, hLabelThreads, hLabelHandles;

    void UpdateDynamicLabels() {
        char buf[128];
        PROCESS_MEMORY_COUNTERS_EX pmc;
        
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            snprintf(buf, sizeof(buf), "Page Faults: %lu", pmc.PageFaultCount);
            SafeSetWindowText(hLabelPageFaults, buf);

            DWORD prio = GetPriorityClass(hProcess);
            const char* prioStr;
            switch(prio) {
                case IDLE_PRIORITY_CLASS: prioStr = "Low"; break;
                case BELOW_NORMAL_PRIORITY_CLASS: prioStr = "Below Normal"; break;
                case NORMAL_PRIORITY_CLASS: prioStr = "Normal"; break;
                case ABOVE_NORMAL_PRIORITY_CLASS: prioStr = "Above Normal"; break;
                case HIGH_PRIORITY_CLASS: prioStr = "High"; break;
                case REALTIME_PRIORITY_CLASS: prioStr = "Realtime"; break;
                default: prioStr = "Unknown"; break;
            }
            snprintf(buf, sizeof(buf), "Priority: %s", prioStr);
            SafeSetWindowText(hLabelPriority, buf);

            double memKB = pmc.WorkingSetSize / 1024.0;
            snprintf(buf, sizeof(buf), "Memory (working set): %.0f K", memKB);
            SafeSetWindowText(hLabelMemory, buf);

            // Calcula Private Working Set real usando QueryWorkingSetEx
            SIZE_T privateWSBytes = 0;
            PSAPI_WORKING_SET_EX_INFORMATION* wsInfo = NULL;
            SIZE_T pageSize;
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            pageSize = si.dwPageSize;

            MEMORY_BASIC_INFORMATION mbi;
            LPVOID addr = 0;
            while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
                if ((mbi.State & MEM_COMMIT) && !(mbi.Type & MEM_IMAGE)) {
                    SIZE_T regionPages = mbi.RegionSize / pageSize;
                    wsInfo = (PSAPI_WORKING_SET_EX_INFORMATION*)malloc(sizeof(*wsInfo) * regionPages);
                    if (!wsInfo) break;

                    for (SIZE_T i = 0; i < regionPages; ++i) {
                        wsInfo[i].VirtualAddress = (PVOID)((uintptr_t)mbi.BaseAddress + i * pageSize);
                    }

                    if (QueryWorkingSetEx(hProcess, wsInfo, sizeof(*wsInfo) * regionPages)) {
                        for (SIZE_T i = 0; i < regionPages; ++i) {
                            if (wsInfo[i].VirtualAttributes.Valid &&
                                wsInfo[i].VirtualAttributes.ShareCount == 0) {
                                privateWSBytes += pageSize;
                            }
                        }
                    }

                    free(wsInfo);
                }

                addr = (LPBYTE)mbi.BaseAddress + mbi.RegionSize;
            }
            double privateWSKB = privateWSBytes / 1024.0;
            snprintf(buf, sizeof(buf), "Memory (private working set): %.0f K", privateWSKB);
            SafeSetWindowText(hLabelPrivateMemory, buf);
        }

        // CPU Time
        FILETIME ftCreate, ftExit, ftKernel, ftUser;
        if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
            ULONGLONG total = ((ULONGLONG)ftKernel.dwHighDateTime << 32 | ftKernel.dwLowDateTime) +
                              ((ULONGLONG)ftUser.dwHighDateTime << 32 | ftUser.dwLowDateTime);
            DWORD sec = (DWORD)(total / 10000000);
            DWORD min = sec / 60; sec %= 60;
            snprintf(buf, sizeof(buf), "CPU Time: %02lu:%02lu", min, sec);
            SafeSetWindowText(hLabelCpuTime, buf);
        }

        // I/O
        IO_COUNTERS io;
        if (GetProcessIoCounters(hProcess, &io)) {
            double r = io.ReadTransferCount / (1024.0 * 1024.0);
            double w = io.WriteTransferCount / (1024.0 * 1024.0);
            snprintf(buf, sizeof(buf), "Total I/O: %.1f MB read / %.1f MB written", r, w);
            SafeSetWindowText(hLabelIO, buf);
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
        SafeSetWindowText(hLabelThreads, buf);

        // Handles
        DWORD count;
        if (GetProcessHandleCount(hProcess, &count)) {
            snprintf(buf, sizeof(buf), "Handles: %lu", count);
            SafeSetWindowText(hLabelHandles, buf);
        }
    }

    switch (message) {
        case WM_INITDIALOG: {
        hProcess = ((AffinityDialogParams*)lParam)->hProcess;
        pid = ((AffinityDialogParams*)lParam)->pid;
        strncpy(processName, ((AffinityDialogParams*)lParam)->processName, MAX_PATH);

        char description[256] = "Unknown";
        char company[256] = "Unknown";
        char timeStr[64] = "Unavailable";
        char bitnessStr[16] = "Unknown";

        // Descrição e empresa
        DWORD verHandle = 0;
        char fullPathForVersionInfo[MAX_PATH] = {0}; // Temporary buffer for path if needed for version info
        HMODULE hMod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleFileNameEx(hProcess, hMod, fullPathForVersionInfo, sizeof(fullPathForVersionInfo));
        }

        DWORD verSize = GetFileVersionInfoSize(fullPathForVersionInfo, &verHandle);
        if (verSize > 0) {
            BYTE* verData = (BYTE*)malloc(verSize);
            if (GetFileVersionInfo(fullPathForVersionInfo, verHandle, verSize, verData)) {
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

        // Tempo de início do processo
        FILETIME ftCreate, ftExit, ftKernel, ftUser;
        SYSTEMTIME stUTC, stLocal;
        if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser)) {
            FileTimeToSystemTime(&ftCreate, &stUTC);
            SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
            snprintf(timeStr, sizeof(timeStr), "%02d/%02d/%04d %02d:%02d:%02d",
                stLocal.wDay, stLocal.wMonth, stLocal.wYear,
                stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
        }

        // Bitness (32/64-bit)
        BOOL isTargetWow64 = FALSE;
        IsWow64Process(hProcess, &isTargetWow64);
    #if defined(_WIN64)
        strcpy(bitnessStr, isTargetWow64 ? "32-bit" : "64-bit");
    #else
        BOOL isHostWow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &isHostWow64);
        strcpy(bitnessStr, (isHostWow64 && !isTargetWow64) ? "64-bit" : "32-bit");
    #endif

        // Layout da janela
        RECT rcClient;
        GetClientRect(hDlg, &rcClient);
        int clientWidth = rcClient.right - rcClient.left;
        int margin = 10;
        int groupPadding = 10;
        int y = margin;

        HFONT hFont = CreateFontForControl();
        char buffer[512];

        //
        // 🔷 Group Box 1 – Informações estáticas
        //
        int staticGroupHeight = 22 * 6 + 20; // 6 linhas + margem inferior
        HWND hGroupStatic = CreateWindow("BUTTON", "Static Information", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            margin, y, clientWidth - 2 * margin, staticGroupHeight, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hGroupStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
        y += 20; // espaço para título do group box

        int x = margin + groupPadding;
        int labelWidth = clientWidth - 2 * (margin + groupPadding);

        snprintf(buffer, sizeof(buffer), "Process: %s", processName);
        HWND hStaticName = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticName, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "PID: %lu", pid);
        HWND hStaticPID = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticPID, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "Description: %s", description);
        HWND hStaticDesc = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticDesc, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "Company: %s", company);
        HWND hStaticComp = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticComp, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "Start Time: %s", timeStr);
        HWND hStaticStart = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticStart, WM_SETFONT, (WPARAM)hFont, TRUE); y += 22;

        snprintf(buffer, sizeof(buffer), "Bitness: %s", bitnessStr);
        HWND hStaticBitness = CreateWindow("STATIC", buffer, WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStaticBitness, WM_SETFONT, (WPARAM)hFont, TRUE); y += 20;

        //
        // 🟩 Group Box 2 – Informações dinâmicas
        //
        int dynamicGroupTop = y + 30;
        int dynamicGroupHeight = 22 * 8 + 20;

        HWND hGroupDynamic = CreateWindow("BUTTON", "Live Metrics", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            margin, dynamicGroupTop - 20, clientWidth - 2 * margin, dynamicGroupHeight, hDlg, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hGroupDynamic, WM_SETFONT, (WPARAM)hFont, TRUE);

        x = margin + groupPadding;
        y = dynamicGroupTop;

        hLabelPriority = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22; 
        hLabelMemory = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelPrivateMemory = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelPageFaults = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22; 
        hLabelCpuTime = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelIO = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelThreads = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;
        hLabelHandles = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, labelWidth, 20, hDlg, NULL, GetModuleHandle(NULL), NULL); y += 22;

        SendMessage(hLabelPriority, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelMemory, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelPrivateMemory, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelPageFaults, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelCpuTime, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelIO, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelThreads, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hLabelHandles, WM_SETFONT, (WPARAM)hFont, TRUE);

        //
        // Botão "Close"
        //
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

    DestroyMenu(hPriorityMenu);
    DestroyMenu(hMenu);
    CloseHandle(hProcess);
}