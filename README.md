# TaskManagerC
 Task Manager feito em C - Sistemas Operacionais

 Para compilar os arquivos e gerar o executável após mudanças no código, seguir os passos abaixo:

 1°: Fazer o download do compilador MinGW, através do link:
 https://sourceforge.net/projects/mingw/files/latest/download
 
 2°: Compilar o projeto modificado e gerar um novo executável (pelo do terminal da IDE (recomendo VSCODE)), através do comando abaixo:
 
 gcc main.c ui/ui.c processes/processes.c hardware/hardware.c utils/utils.c -o GerenciadorTarefas.exe -mwindows -lcomctl32 -lpsapi
