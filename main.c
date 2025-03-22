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

int main() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TaskManagerClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(wc.lpszClassName, "Task Manager", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, // Remover WS_MAXIMIZEBOX aqui
        CW_USEDEFAULT, CW_USEDEFAULT, 
        WINDOW_WIDTH, WINDOW_HEIGHT, 
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
    {
        MessageBox(NULL, "Erro ao criar a janela!", "Erro", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Remover o botão de maximizar (WS_MAXIMIZEBOX) após a criação da janela
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;  // Remove o botão de maximizar
    style &= ~WS_SIZEBOX;      // Remove o redimensionamento
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Definir a posição e tamanho fixos da janela
    SetWindowPos(hwnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_NOZORDER | SWP_NOMOVE);

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