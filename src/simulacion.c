#include <stdio.h>
#include <stdint.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

struct REGISTRO { //sizeof(struct REGISTRO): 24 bytes
    time_t fecha; //Precisión segundos [opcionalmente microsegundos con struct timeval]
    pid_t pid; //PID del proceso que lo ha creado
    int nEscritura; //Entero con el nº de escritura, de 1 a 50 (orden por tiempo)
    int nRegistro; //Entero con el nº del registro dentro del fichero: [0..REGMAX-1] (orden por posición)
};

uint32_t acabados = 0;

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) acabados++;
}


int main(int argc, char **argv) {
    signal(SIGCHLD, reaper);
    return 0;
}
