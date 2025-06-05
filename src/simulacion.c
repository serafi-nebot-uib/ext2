/**************************************************************************
* FILENAME: simulacion.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"
#include "simulacion.h"

#define SHOW_WRITES 0

static uint32_t acabados = 0;
// nombre del dispositivo, se utiliza como variable global para poder utilizarla en la función clean_exit
static const char *dev_name = NULL;
#if SHOW_WRITES > 0
static const char *colors[11] = { ORANGE, LBLUE, LGREEN, YELLOW, BLUE, MAGENTA, CYAN, RED, ROSE, GREEN, GRAY };
#endif

/**
 * Enterrador de subprocesos
 */
void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) acabados++;
}

/**
 * Dormir sin despertarse al recibir una señal
 * @param msec cantidad de milisegundos a dormir
 */
void sleep_uninterrupted(unsigned int msec) {
    struct timespec req = { .tv_sec = msec / 1000, .tv_nsec = (msec % 1000) * 1000000 }, rem = { 0 };
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) req = rem;
}

/**
 * Limpiar los recursos y parar la ejecución del programa.
 *
 * @param code código de error con el cual parar el programa
 */
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

    // montar el dispositivo para poder acceder a el
    // a partir de aqui si hay un error crítico se debe salir con clean_exit
    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    // obtenemos el timestamp actual para crear el directorio de simulación
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    // ruta del directorio de simulación
    char simdir[64];
    strftime(simdir, sizeof(simdir), "/simul_%Y%m%d%H%M%S/", t);

    // creamos el directorio de simulación
    int r = mi_creat(simdir, 6);
    if (r != 0) {
        mostrar_error_buscar_entrada(r);
        clean_exit(FALLO);
    }

    // asignamos el enterrador al a señal SIGCHLD
    signal(SIGCHLD, reaper);

    printf("*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n", NUMPROCESOS, NUMESCRITURAS);

    // iteramos sobre todos los procesos a crear
    for (size_t i = 0; i < NUMPROCESOS; i++) {
#if SHOW_WRITES > 0
        const char *const color = colors[i % (sizeof(colors) / sizeof(*colors))];
#endif
        // creamos el subproceso con fork
        pid_t pid = fork();

        if (pid == 0) { // si es el hijo
            char path[128]; // ruta al directorio del subproceso
            pid = getpid(); // obtenemos el pid del hijo para crear el directorio del subproceso
            snprintf(path, sizeof(path), "%sproceso_PID%u/", simdir, pid);

            // creamos el directorio del subproceso
            r = mi_creat(path, 06);
            if (r != 0) {
                ERROR("no se ha podido crear el directorio: %s", path);
                mostrar_error_buscar_entrada(r);
                clean_exit(FALLO);
            }

            // concatenamos el nombre de fichero de datos al final de la ruta de directorio del subproceso
            // path ahora apunta al fichero de datos del subproceso
            strcat(path, "prueba.dat");
            // creamos el fichero de datos del subproceso
            r = mi_creat(path, 6);
            if (r != 0) {
                ERROR("no se ha podido crear el fichero: %s", path);
                mostrar_error_buscar_entrada(r);
                clean_exit(FALLO);
            }

            // inicializamos la semilla del generador de numeros aleatorios
            srand(time(NULL) + getpid());

            // iteramos para todas las escrituras a realizar
            for (size_t j = 0; j < NUMESCRITURAS; j++) {
                registro_t registro;
                gettimeofday(&registro.fecha, NULL);
                registro.pid = pid;
                registro.nEscritura = j;
                registro.nRegistro = rand() % REGMAX;
#if SHOW_WRITES > 0
                printf("%sescritura %zu en %s\n" RESET, color, j, path);
#endif
                // escribimos el registro al final del fichero de datos del subproceso
                if (mi_write(path, &registro, registro.nRegistro * sizeof(registro), sizeof(registro)) == FALLO) {
                    ERROR("no se ha podido escribir el registro en el fichero: %s", path);
                    clean_exit(FALLO);
                }
                // esperamos 0.05 segundos entre escrituras
                // usleep(50000);
                sleep_uninterrupted(50);
            }

            printf("proceso %4zu completadas %4d escrituras en %s\n", i + 1, NUMESCRITURAS, path);

            clean_exit(EXITO);
        }
        // esperamos 0.15 segundo entre lanzamiento de procesos
        // usleep(150000);
        sleep_uninterrupted(150);
    }

    // esperamos a que terminen todos los procesos
    while (acabados < NUMPROCESOS) pause();

    clean_exit(EXITO);
    return EXITO;
}