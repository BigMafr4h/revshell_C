# Trojan Backdoor revshell.c
Um reverse Shell escrito em C, confesso que demorei alguns dias para entender como funciona essas bibliotecas do windows e seus funcionamentos(principalmente Handles e Pipes).
Neste mesmo código iria também criar modos de abstração de trojan ou criação de outro bloco de código para persistencia para pastas como startup, criar uma tarefa automátizada com schtasks ou até construir direto em C windows(não sei fazer e ia demorar muito pra aprender e me estressar mais ainda com essas bibliotecas). Porém ia tornar esse código uma arma completa e feita sendo que o intuíto aqui é apenas didático.
!!!!!!!!!!!!

Significado:  Trojan Backdoor é um tipo de malware que se disfarça de software legítimo (como um Trojan(cavalo de tróia)) para enganar o usuário, mas, uma vez instalado, cria uma "porta dos fundos" (backdoor) no sistema, permitindo que hackers acessem, controlem e executem comandos remotamente no computador infectado, roubando dados ou instalando mais ameaças, sem o conhecimento do usuário

!!!!!!!!!!!!!!!!!!

Diferentemente de um SO como UNIX ou Posix, que são sistemas fáceis de criar payloads ou shells devido a facilidade de criar sockets, processos, threads etc..
No windows a coisa é mais embaixo, o erro comum para quem cria no Unix é que não se pode usar diretamente um SOCKET como HANDLE no Windows. São tipos diferentes e o CreateProcess não aceita sockets diretamente para redirecionamento. O Windows requer pipes nomeados ou anônimos para redirecionar I/O entre processos.

Compilação com MinGW64:
x86_64-w64-mingw32-gcc reverse_shell.c -o reverse_shell.exe -lws2_32 -Wl,-subsystem,windows
