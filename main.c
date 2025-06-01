#include "config.h"
#include "utils/utils.h"
#include "ui/ui.h"
#include "hardware/hardware.h"
#include "processes/processes.h"

SYSTEM_INFO sysInfo;
int numProcessors;
HWND hTab, hListView, hButtonEndTask, hHardwarePanel;
HFONT hFontTabs, hFontButton;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        AddTabs(hwnd);
        AddListView(hwnd);
        AddHardwarePanel(hwnd);
        AddFooter(hwnd);
        SetupTimer(hwnd);
        break;

    case WM_NOTIFY: {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        if (pnmh->idFrom == ID_TAB_CONTROL && pnmh->code == TCN_SELCHANGE) {
            int selectedTab = TabCtrl_GetCurSel(hTab);
            OnTabSelectionChanged(hwnd, selectedTab);
        }
        else if (pnmh->idFrom == ID_LIST_VIEW && pnmh->code == LVN_ITEMCHANGED) {
            UpdateEndTaskButtonState();
        }
        break;
    }

    case WM_TIMER: {
        int selectedTab = TabCtrl_GetCurSel(hTab);
        if (selectedTab == 0) {
            UpdateProcessList();
        } else if (selectedTab == 1) {
            UpdateHardwareInfo();
        }
        break;
    }

    case WM_CONTEXTMENU:
        if ((HWND)wParam == hListView) {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ShowContextMenu(hListView, hwnd, pt);
        }
        break;
    
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_END_TASK_BUTTON)
        {
            EndSelectedProcess(hListView, hwnd);
        }
        break;

    case WM_DESTROY:
        CleanupResources();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

void EnableDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        CloseHandle(hToken);
    }
}

void InitializeSystemInfo() {
    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;
}

int main() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TaskManagerClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, "Task Manager", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        WINDOW_WIDTH, WINDOW_HEIGHT, 
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
    {
        MessageBox(NULL, "Error creating window!", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Remover o botão de maximizar (WS_MAXIMIZEBOX) após a criação da janela
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;  // Remove o botão de maximizar
    style &= ~WS_SIZEBOX;      // Remove o redimensionamento
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Definir a posição e tamanho fixos da janela
    SetWindowPos(hwnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_NOZORDER | SWP_NOMOVE);

    // Ativa a permissão de depuração para acessar processos do SYSTEM
    EnableDebugPrivilege();

    InitializeSystemInfo();
    // Centraliza a janela
    CenterWindowToScreen(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    UpdateProcessList();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}