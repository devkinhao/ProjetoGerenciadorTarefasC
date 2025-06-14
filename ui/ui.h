#ifndef UI_H
#define UI_H

// Prevenção contra inclusão múltipla do header
#include "..\config.h"  // Inclui configurações globais e definições comuns

// Estrutura para passar parâmetros para o diálogo de afinidade
typedef struct {
    HANDLE hProcess;        // Handle do processo que terá a afinidade modificada
    char processName[MAX_PATH]; // Nome do processo (para exibição)
    DWORD pid;              // ID do processo
} AffinityDialogParams;

/* Funções de manipulação de diálogos */
void CenterDialogInParent(HWND hDlg, HWND hwndParent); // Centraliza um diálogo em relação à sua janela pai

/* Funções de construção da interface principal */
void AddTabs(HWND hwndParent, int width, int height); // Cria o controle de abas na janela principal
void AddListView(HWND hwndParent, int width, int height); // Cria a lista de processos com colunas para exibição
void AddFooter(HWND hwndParent, int width, int height); // Adiciona o rodapé com botões na janela principal

/* Funções de gerenciamento da aplicação */
void SetupTimer(HWND hwnd); // Configura o timer para atualizações periódicas
void OnTabSelectionChanged(HWND hwndParent, int selectedTab); // Manipula a troca entre as abas de processos e hardware
void CleanupResources(); // Libera recursos alocados (fontes, etc)
void UpdateEndTaskButtonState(); // Atualiza o estado do botão "Encerrar tarefa" conforme seleção
void UpdateOkButtonState(HWND hDlg, HWND hOk); // Atualiza o estado do botão OK no diálogo de afinidade

/* Funções relacionadas ao diálogo de afinidade de processador */
INT_PTR CALLBACK AffinityDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); // Procedimento do diálogo de afinidade
void ShowAffinityDialog(HWND hwndParent, HANDLE hProcess, const char* processName); // Exibe o diálogo de configuração de afinidade

/* Funções relacionadas ao diálogo de detalhes do processo */
INT_PTR CALLBACK ShowProcessDetailsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam); // Procedimento do diálogo de detalhes
void ShowProcessDetailsDialog(HWND hwndParent, HANDLE hProcess, const char* processName, DWORD pid); // Exibe o diálogo com informações detalhadas do processo

/* Funções de menu de contexto */
void ShowContextMenu(HWND hwndListView, HWND hwndParent, POINT pt); // Exibe o menu de contexto para um processo selecionado

#endif // UI_H