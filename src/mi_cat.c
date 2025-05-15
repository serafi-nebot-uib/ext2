/**************************************************************************
* FILENAME: mi_cat.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

#define TAM_BUFFER (BLOCKSIZE * 2)

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> </ruta_fichero>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta = argv[2];

    DEBUG(1, "nombre_dispositivo: %s", nombre_dispositivo);
    DEBUG(1, "ruta: %s", ruta);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    int ret = EXITO;
    stat_t stat;
    if (mi_stat(ruta, &stat) == FALLO) {
        ERROR("no se ha podido leer \"%s\"", ruta);
        ret = FALLO;
    } else {
        if (stat.tipo != 'f') {
            ERROR("\"%s\" (%c) no es un fichero", ruta, stat.tipo);
            ret = FALLO;
        } else {
            unsigned char buffer[TAM_BUFFER] = { 0 };
            int leidos;
            int total_leidos = 0;
            unsigned int offset = 0;

            // lectura secuencial del fichero (simula el comportamiento del comando "cat") se lee bloque a bloque hasta que mi_read_f() retorne 0 (EOF)
            while ((leidos = mi_read(ruta, buffer, offset, TAM_BUFFER)) > 0) {
                // hexdump_col(buffer, 0, leidos, offset, 32, 8);
                if (write(fileno(stdout), buffer, leidos) < 0) {
                    ERRSYS("write");
                    ret = FALLO;
                    break;
                }
                total_leidos += leidos;
                offset += leidos;
                memset(buffer, 0, TAM_BUFFER);
            }
            printf("\n\nTotal_leidos %d", total_leidos);
        }
    }

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: %s", nombre_dispositivo);
        return FALLO;
    }

    return ret;
}

