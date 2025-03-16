#include "config.h"
#include "utils/utils.h"
#include "ui/ui.h"
#include "hardware/hardware.h"
#include "processes/processes.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        AddTabs(hwnd);
        AddListView(hwnd);
        AddFooter(hwnd);
        SetupTimer(hwnd);
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == ID_TAB_CONTROL && ((LPNMHDR)lParam)->code == TCN_SELCHANGE)
        {
            int selectedTab = TabCtrl_GetCurSel(hTab);
            OnTabSelectionChanged(hwnd, selectedTab);
        }
        break;

    case WM_TIMER:
        UpdateProcessList();
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

void InitializeSystemInfo() {
    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;
}

int main()
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TaskManagerClass";
    RegisterClass(&wc);

    // Criando a janela
    HWND hwnd = CreateWindow(wc.lpszClassName, "Task Manager", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
    {
        MessageBox(NULL, "Erro ao criar a janela!", "Erro", MB_ICONERROR | MB_OK);
        return 1;
    }

    InitializeSystemInfo();
    // Centraliza a janela
    CenterWindow(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
