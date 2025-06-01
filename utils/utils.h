#ifndef UTILS_H
#define UTILS_H

#include "..\config.h"

HFONT CreateFontForControl();
void CenterWindowRelativeToParent(HWND hwnd, int windowWidth, int windowHeight);
void CenterWindowToScreen(HWND hwnd, int windowWidth, int windowHeight);
void SafeSetWindowText(HWND hWnd, const char* newText);

#endif // UTILS_H
