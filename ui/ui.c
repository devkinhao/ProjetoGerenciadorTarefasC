#include "ui.h"
#include "../utils\utils.h"

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
    char* columns[] = { "Process Name", "User", "PID", "Status", "CPU (%)", "Memory (MB)", "Disk (%)" };
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
    SetTimer(hwnd, 1, 500, NULL);
}

void OnTabSelectionChanged(HWND hwndParent, int selectedTab) {
    ShowWindow(hListView, selectedTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(hHardwarePanel, selectedTab == 1 ? SW_SHOW : SW_HIDE);
}

void CleanupResources() {
    if (hFontTabs) DeleteObject(hFontTabs);
    if (hFontButton) DeleteObject(hFontButton);
}
