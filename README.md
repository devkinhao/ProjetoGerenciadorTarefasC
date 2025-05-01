# TaskManagerC
 Task Manager feito em C - Sistemas Operacionais

 Para compilar os arquivos e gerar o executável após mudanças no código, seguir os passos abaixo (FOI TROCADO PARA UM COMPILADOR MAIS ATUALIZADO):

 1°: Para fazer o download do compilador MINGW-w64 (mais atualizado), é necessário baixar o MSYS2. Basta entrar no link https://www.msys2.org/, descer um pouco até o tópico 'Installation', e clicar no passo 1: 'Download the installer: msys2-x86_64-versão.exe';
 
 2°: Abrir o terminal do MINGW-w64, que estará dentro do path: "C:\msys64\mingw64.exe" (após a instalação do passo 1 ter sido concluída);
 
 3°: Executar o comando 'pacman -Syu' para baixar os pacotes mais atualizados;
 
 4°: Depois disso, vai pedir pra fechar o terminal. Daí é só abrir de novo e executar o mesmo comando 'pacman -Syu';
 
 5°: Feito isso, basta executar o comando 'pacman -S mingw-w64-x86_64-gcc' pra baixar os pacotes e arquivos do compilador gcc;

 6°: Basta entrar na pasta do projeto utilizando o comando CD: 'cd C:/Users/erick/Desktop/ProjetoGerenciadorTarefasC' (no meu caso estava neste local);
 
 7°: Executar o comando gcc main.c ui/ui.c processes/processes.c hardware/hardware.c utils/utils.c -o TaskManager.exe -mwindows -lcomctl32 -lpsapi -lkernel32
