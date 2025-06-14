#ifndef PROCESSES_H
#define PROCESSES_H

#include "..\config.h"

// Definições de tamanho para buffers de texto
#define MAX_NAME_LENGTH 260       // Tamanho máximo para nomes de processos
#define MAX_STATUS_LENGTH 32      // Tamanho máximo para status
#define MAX_CPU_LENGTH 16         // Tamanho máximo para string de CPU
#define MAX_MEMORY_LENGTH 32      // Tamanho máximo para string de memória
#define MAX_DISK_LENGTH 16        // Tamanho máximo para string de disco
#define MAX_PATH_LENGTH 260       // Tamanho máximo para caminhos
#define UNKNOWN_TEXT "Unknown"    // Texto padrão para informações desconhecidas
#define NA_TEXT "N/A"             // Texto padrão para informações não disponíveis

// Estrutura que armazena informações sobre um processo
typedef struct {
    DWORD pid;                    // ID do processo
    char name[MAX_NAME_LENGTH];   // Nome do executável
    char status[MAX_STATUS_LENGTH]; // Status do processo
    char cpu[MAX_CPU_LENGTH];     // Uso de CPU (em %)
    char memory[MAX_MEMORY_LENGTH]; // Uso de memória
    char disk[MAX_DISK_LENGTH];   // Atividade de disco
    char path[MAX_PATH_LENGTH];   // Caminho completo do executável
    
    // Campos para cálculo de métricas
    FILETIME prevKernel;          // Tempo de kernel na última medição
    FILETIME prevUser;            // Tempo de usuário na última medição
    FILETIME prevSystemKernel;    // Tempo de kernel do sistema na última medição
    FILETIME prevSystemUser;      // Tempo de usuário do sistema na última medição
    ULONGLONG prevReadBytes;      // Bytes lidos na última medição
    ULONGLONG prevWriteBytes;     // Bytes escritos na última medição
    ULONGLONG lastIoCheckTime;    // Último momento que a atividade de disco foi verificada
} ProcessInfo;

// Declarações de funções
ULONGLONG DiffFileTimes(FILETIME ftA, FILETIME ftB);  // Calcula diferença entre dois FILETIMEs
void GetCpuUsage(HANDLE hProcess, char *cpuBuffer, ProcessInfo *procInfo);  // Calcula uso de CPU
void GetMemoryUsage(HANDLE hProcess, char *buffer, size_t bufferSize);      // Obtém uso de memória
void GetDiskUsage(HANDLE hProcess, char *diskBuffer, ProcessInfo *procInfo); // Calcula atividade de disco
int FindProcessIndex(DWORD pid);  // Encontra índice de um processo na lista pelo PID
void GetProcessUser(HANDLE hProcess, char* userBuffer, DWORD bufferSize);   // Obtém usuário dono do processo
void UpdateProcessInfo(int processIndex, HANDLE hProcess);  // Atualiza informações estáticas do processo
void UpdateProcessMetrics(int processIndex, HANDLE hProcess); // Atualiza métricas de desempenho
void RemoveNonExistingProcesses(bool processExists[]);  // Remove processos que não existem mais
void UpdateProcessList();         // Atualiza a lista completa de processos
void EndSelectedProcess(HWND hListView, HWND hwndParent);  // Encerra um processo selecionado

#endif // PROCESSES_H