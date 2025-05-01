#include "utils.h"

HFONT CreateFontForControl() {
    return CreateFont(FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
}

void CenterWindow(HWND hwnd, int windowWidth, int windowHeight) {
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    int screenWidth  = rect.right  - rect.left;
    int screenHeight = rect.bottom - rect.top;

    int posX = (screenWidth  - windowWidth)  / 2;
    int posY = (screenHeight - windowHeight) / 2;

    SetWindowPos(hwnd, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
}
