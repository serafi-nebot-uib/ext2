#include <stdio.h>
#include <stdint.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#include "core/directorios.h"

#define REGMAX 500000
#define NUMPROCESOS 2
#define NUMESCRITURAS 50

typedef struct { //sizeof(struct REGISTRO): 24 bytes
    time_t fecha; //Precisión segundos [opcionalmente microsegundos con struct timeval]
    pid_t pid; //PID del proceso que lo ha creado
    int nEscritura; //Entero con el nº de escritura, de 1 a 50 (orden por tiempo)
    int nRegistro; //Entero con el nº del registro dentro del fichero: [0..REGMAX-1] (orden por posición)
} registro_t;

static uint32_t acabados = 0;
static const char *dev_name = NULL;

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) acabados++;
}

void clean_exit(int code) {
    if (dev_name != NULL) {
        if (bumount() == FALLO) {
            ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
            exit(FALLO);
        }
    }
    exit(code);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        ERROR("sintaxis: %s <disco>", argv[0]);
    }

    dev_name = argv[1];

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char simdir[64];
    strftime(simdir, sizeof(simdir), "/simul_%Y%m%d%H%M%S/", t);

    int r = mi_creat(simdir, 06);
    if (r != 0) {
        mostrar_error_buscar_entrada(r);
        clean_exit(FALLO);
    }

    signal(SIGCHLD, reaper);

    for (size_t i = 0; i < NUMPROCESOS; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            char path[128];
            snprintf(path, sizeof(path), "%sproceso_PID%zu/", simdir, i);

            r = mi_creat(path, 06);
            if (r != 0) {
                ERROR("no se ha podido crear el directorio: %s", path);
                mostrar_error_buscar_entrada(r);
                clean_exit(FALLO);
            }

            strcat(path, "prueba.dat");

            r = mi_creat(path, 06);
            if (r != 0) {
                ERROR("no se ha podido crear el fichero: %s", path);
                mostrar_error_buscar_entrada(r);
                clean_exit(FALLO);
            }

            srand(time(NULL) + getpid());

            for (size_t j = 0; j < NUMESCRITURAS; j++) {
                registro_t registro;
                registro.fecha = time(NULL);
                registro.pid = getpid();
                registro.nEscritura = j;
                registro.nRegistro = rand() % REGMAX;
                if (mi_write(path, &registro, j * sizeof(registro), sizeof(registro)) == FALLO) {
                    ERROR("no se ha podido escribir el registro en el fichero: %s", path);
                    clean_exit(FALLO);
                }
                usleep(50000);
            }

            usleep(150000);
            clean_exit(EXITO);
        }
    }

    clean_exit(EXITO);
}
