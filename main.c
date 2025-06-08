#include "config.h"
#include "utils/utils.h"
#include "ui/ui.h"
#include "hardware/hardware.h"
#include "processes/processes.h"

SYSTEM_INFO sysInfo;
int numProcessors;
HWND hTab, hListView, hButtonEndTask, hHardwarePanel;
HFONT hFontTabs, hFontButton;
HBRUSH hbrBackground = NULL;
COLORREF clrBackground = RGB(255, 255, 255);

// Definições de largura e altura da janela principal
int gWindowWidth = 1280;
int gWindowHeight = 720;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        hbrBackground = CreateSolidBrush(clrBackground);

        AddTabs(hwnd, gWindowWidth, gWindowHeight);
        AddListView(hwnd, gWindowWidth, gWindowHeight);
        AddHardwarePanel(hwnd);
        AddFooter(hwnd, gWindowWidth, gWindowHeight);
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

    case WM_CTLCOLORSTATIC: {
        HWND hStatic = (HWND)lParam;
        HDC hdcStatic = (HDC)wParam;

        SetBkColor(hdcStatic, clrBackground);
        SetTextColor(hdcStatic, RGB(0, 0, 0)); 
           
        return (LRESULT)GetStockObject(NULL_BRUSH); 
    }

    case WM_ERASEBKGND: {
        if (hwnd == hTab) {
            return 0;
        }
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        FillRect((HDC)wParam, &rcClient, hbrBackground);
        return 1; 
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
        if (hbrBackground) {
            DeleteObject(hbrBackground);
        }
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
    /*RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    gWindowWidth = (int)(workArea.right * 0.90);
    gWindowHeight = (int)(workArea.bottom * 0.80);*/

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TaskManagerClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, "Task Manager", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        gWindowWidth, gWindowHeight, 
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, "Error creating window!", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    style &= ~WS_SIZEBOX;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    CenterWindowToScreen(hwnd, gWindowWidth, gWindowHeight);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    UpdateProcessList();

    EnableDebugPrivilege();
    InitializeSystemInfo();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}