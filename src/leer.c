/**************************************************************************
* FILENAME: leer.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "directorios.h"
#include "helper.h"

#define TAM_BUFFER (BLOCKSIZE * 2)
// #define TAM_BUFFER 1500

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "sintaxis: %s <nombre_dispositivo> <ninodo>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    unsigned char buffer[TAM_BUFFER] = { 0 };
    int leidos;
    int total_leidos = 0;
    unsigned int offset = 0;

    // lectura secuencial del fichero (simula el comportamiento del comando "cat") se lee bloque a bloque hasta que mi_read_f() retorne 0 (EOF)
    while ((leidos = mi_read_f(ninodo, buffer, offset, TAM_BUFFER)) > 0) {
        // hexdump_col(buffer, 0, leidos, offset, 32, 8);
        if (write(fileno(stdout), buffer, leidos) < 0) {
            ERRSYS("write");
            break;
        }
        total_leidos += leidos;
        offset += leidos;
        memset(buffer, 0, TAM_BUFFER);
    }

    stat_t stat;
    if (mi_stat_f(ninodo, &stat) == FALLO) {
        ERROR("no se ha podido obtener la información del inodo");
    } else {
        fprintf(stderr, GRAY "\ntotal_leidos %d\n" RESET, total_leidos);
        fprintf(stderr, GRAY "tamEnBytesLog %u\n" RESET, stat.tamEnBytesLog);
    }

    if (bumount() == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    return 0;
}
