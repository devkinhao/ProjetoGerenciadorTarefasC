#include "utils.h"

HFONT CreateFontForControl() {
    return CreateFont(FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
}

void CenterWindowRelativeToParent(HWND hwnd, int windowWidth, int windowHeight) {
    HWND hwndParent = GetParent(hwnd);
    RECT parentRect;

    if (hwndParent && GetWindowRect(hwndParent, &parentRect)) {
        int parentWidth  = parentRect.right  - parentRect.left;
        int parentHeight = parentRect.bottom - parentRect.top;

        int posX = parentRect.left + (parentWidth  - windowWidth)  / 2;
        int posY = parentRect.top  + (parentHeight - windowHeight) / 2;

        SetWindowPos(hwnd, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
    }
}


void CenterWindowToScreen(HWND hwnd, int windowWidth, int windowHeight) {
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    int screenWidth  = rect.right  - rect.left;
    int screenHeight = rect.bottom - rect.top;

    int posX = (screenWidth  - windowWidth)  / 2;
    int posY = (screenHeight - windowHeight) / 2;

    SetWindowPos(hwnd, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
}

void SafeSetWindowText(HWND hWnd, const char* newText) {
    char currentText[512];
    GetWindowText(hWnd, currentText, sizeof(currentText));
    if (strcmp(currentText, newText) != 0) {
        SetWindowText(hWnd, newText);
    }
}