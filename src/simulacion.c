#include "core/directorios.h"
#include "simulacion.h"

static uint32_t acabados = 0;
static const char *dev_name = NULL;
// static const char *colors[11] = { ORANGE, LBLUE, LGREEN, YELLOW, BLUE, MAGENTA, CYAN, RED, ROSE, GREEN, GRAY };

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) acabados++;
}

void clean_exit(int code) {
    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        exit(FALLO);
    }
    exit(code);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        ERROR("sintaxis: %s <disco>", argv[0]);
        return FALLO;
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

    printf("*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n", NUMPROCESOS, NUMESCRITURAS);

    for (size_t i = 0; i < NUMPROCESOS; i++) {
        // const char *const color = colors[i % (sizeof(colors) / sizeof(*colors))];
        pid_t pid = fork();

        if (pid == 0) {
            char path[128];
            pid = getpid();
            snprintf(path, sizeof(path), "%sproceso_PID%u/", simdir, pid);

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
                registro.pid = pid;
                registro.nEscritura = j;
                registro.nRegistro = rand() % REGMAX;
                // printf("%sescritura %zu en %s\n" RESET, color, j, path);
                if (mi_write(path, &registro, j * sizeof(registro), sizeof(registro)) == FALLO) {
                    ERROR("no se ha podido escribir el registro en el fichero: %s", path);
                    clean_exit(FALLO);
                }
                usleep(50000);
            }

            printf("proceso %zu completadas %d escrituras en %s\n", i, NUMESCRITURAS, path);

            usleep(150000);
            clean_exit(EXITO);
        }
    }

    while (acabados < NUMPROCESOS) pause();

    clean_exit(EXITO);
    return EXITO;
}
