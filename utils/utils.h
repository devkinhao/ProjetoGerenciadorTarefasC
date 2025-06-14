#ifndef UTILS_H
#define UTILS_H

#include "..\config.h"

HFONT CreateFontForControl(); // Cria uma fonte padrão para os controles
void CenterWindowToScreen(HWND hwnd, int windowWidth, int windowHeight); // Centraliza uma janela na tela
void SafeSetWindowText(HWND hWnd, const char* newText); // Atualiza o texto de um controle apenas se for diferente do atual

#endif // UTILS_H