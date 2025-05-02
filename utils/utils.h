#ifndef UTILS_H
#define UTILS_H

#include "..\config.h"

HFONT CreateFontForControl();
void CenterWindowRelativeToParent(HWND hwnd, int windowWidth, int windowHeight);
void CenterWindowToScreen(HWND hwnd, int windowWidth, int windowHeight);

#endif // UTILS_H
