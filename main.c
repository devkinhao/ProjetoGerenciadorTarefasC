#include "config.h"
#include "utils/utils.h"
#include "ui/ui.h"
#include "hardware/hardware.h"
#include "processes/processes.h"

// Variáveis globais
SYSTEM_INFO sysInfo;              // Armazena informações do sistema
int numProcessors;                // Número de núcleos de processamento
HWND hTab, hListView, hButtonEndTask, hHardwarePanel;  // Handles dos controles
HFONT hFontTabs, hFontButton;     // Handles das fontes
HBRUSH hbrBackground = NULL;      // Pincel para o fundo da janela
COLORREF clrBackground = RGB(255, 255, 255);  // Cor de fundo branca

// Dimensões padrão da janela
int gWindowWidth = 1280;
int gWindowHeight = 720;

// Procedimento da janela principal - trata todas as mensagens do Windows
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        // Inicialização da janela principal
        hbrBackground = CreateSolidBrush(clrBackground);  // Cria pincel branco para o fundo

        // Cria todos os controles da interface:
        AddTabs(hwnd, gWindowWidth, gWindowHeight);       // Adiciona as abas (Processos/Hardware)
        AddListView(hwnd, gWindowWidth, gWindowHeight);   // Cria a lista de processos
        AddHardwarePanel(hwnd);                           // Configura o painel de hardware
        AddFooter(hwnd, gWindowWidth, gWindowHeight);     // Adiciona rodapé com botões
        SetupTimer(hwnd);                                 // Configura timer para atualizações periódicas
        break;

    case WM_NOTIFY: {
        // Trata notificações enviadas pelos controles filhos
        LPNMHDR pnmh = (LPNMHDR)lParam;
        
        // Verifica se a notificação é da troca de aba
        if (pnmh->idFrom == ID_TAB_CONTROL && pnmh->code == TCN_SELCHANGE) {
            int selectedTab = TabCtrl_GetCurSel(hTab);
            OnTabSelectionChanged(hwnd, selectedTab);  // Atualiza a interface para a aba selecionada
        }
        // Verifica se a notificação é de mudança de seleção na lista de processos
        else if (pnmh->idFrom == ID_LIST_VIEW && pnmh->code == LVN_ITEMCHANGED) {
            UpdateEndTaskButtonState();  // Habilita/desabilita o botão "Encerrar tarefa"
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        // Personaliza a aparência dos controles estáticos (labels)
        HWND hStatic = (HWND)lParam;
        HDC hdcStatic = (HDC)wParam;

        // Configura cores de fundo e texto:
        SetBkColor(hdcStatic, clrBackground);  // Fundo branco
        SetTextColor(hdcStatic, RGB(0, 0, 0)); // Texto preto
           
        return (LRESULT)GetStockObject(NULL_BRUSH); // Retorna pincel transparente
    }

    case WM_ERASEBKGND: {
        // Trata o evento de pintura do fundo da janela
        if (hwnd == hTab) {
            return 0;  // Não pinta o fundo da aba (deixa o controle nativo fazer isso)
        }
        
        // Pinta o fundo da janela principal com a cor branca
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        FillRect((HDC)wParam, &rcClient, hbrBackground);
        return 1; // Indica que o fundo foi pintado
    }

    case WM_TIMER: {
        // Trata o evento de timer (atualizações periódicas)
        int selectedTab = TabCtrl_GetCurSel(hTab);
        
        // Atualiza a aba ativa:
        if (selectedTab == 0) {
            UpdateProcessList();    // Atualiza lista de processos
        } else if (selectedTab == 1) {
            UpdateHardwareInfo();   // Atualiza informações de hardware
        }
        break;
    }

    case WM_CONTEXTMENU:
        // Trata o clique com botão direito (menu de contexto)
        if ((HWND)wParam == hListView) {
            POINT pt;
            pt.x = LOWORD(lParam);  // Obtém coordenada X do clique
            pt.y = HIWORD(lParam);  // Obtém coordenada Y do clique
            ShowContextMenu(hListView, hwnd, pt);  // Exibe menu para o processo selecionado
        }
        break;

    case WM_COMMAND:
        // Trata comandos (cliques em botões, seleções de menu)
        if (LOWORD(wParam) == ID_END_TASK_BUTTON) {
            EndSelectedProcess(hListView, hwnd);  // Encerra o processo selecionado
        }
        break;

    case WM_DESTROY:
        // Limpeza antes de fechar a aplicação
        if (hbrBackground) {
            DeleteObject(hbrBackground);  // Libera o pincel de fundo
        }
        CleanupResources();               // Libera outros recursos (fontes, etc)
        PostQuitMessage(0);               // Encerra o loop de mensagens
        break;

    default:
        // Tratamento padrão para todas as outras mensagens não tratadas
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;  // Retorno padrão para mensagens processadas
}

// Habilita privilégios de depuração para acessar todos os processos
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

// Obtém informações básicas do sistema
void InitializeSystemInfo() {
    GetSystemInfo(&sysInfo);            // Preenche a estrutura sysInfo
    numProcessors = sysInfo.dwNumberOfProcessors;  // Armazena número de processadores
}

// Ponto de entrada do programa
int main() {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;          // Define o procedimento da janela
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TaskManagerClass";
    
    // Registra a classe da janela
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Error registering window class!", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Cria a janela principal
    HWND hwnd = CreateWindow(wc.lpszClassName, "Task Manager", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        gWindowWidth, gWindowHeight, 
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd) {
        MessageBox(NULL, "Error creating window!", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Remove botão de maximizar e redimensionamento
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    style &= ~WS_SIZEBOX;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Centraliza a janela na tela
    CenterWindowToScreen(hwnd, gWindowWidth, gWindowHeight);

    // Mostra e atualiza a janela
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    // Configurações iniciais
    EnableDebugPrivilege();       // Habilita privilégios para acessar todos os processos
    InitializeSystemInfo();       // Obtém informações do sistema
    
    // Atualiza a lista de processos pela primeira vez
    UpdateProcessList();

    // Loop principal de mensagens
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}