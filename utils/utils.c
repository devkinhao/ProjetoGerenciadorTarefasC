#include "utils.h"

// Cria uma fonte padrão para os controles da interface
HFONT CreateFontForControl() {
    return CreateFont(FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
}

// Centraliza uma janela na tela
void CenterWindowToScreen(HWND hwnd, int windowWidth, int windowHeight) {
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);  // Obtém área útil da tela

    int screenWidth = rect.right - rect.left;
    int screenHeight = rect.bottom - rect.top;

    // Calcula posição centralizada
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    // Aplica a posição (mantém o tamanho atual)
    SetWindowPos(hwnd, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
}

// Atualiza o texto de um controle apenas se for diferente do atual
void SafeSetWindowText(HWND hWnd, const char* newText) {
    char currentText[512];
    GetWindowText(hWnd, currentText, sizeof(currentText));
    
    // Só atualiza se o texto for diferente
    if (strcmp(currentText, newText) != 0) {
        SetWindowText(hWnd, newText);
    }
}