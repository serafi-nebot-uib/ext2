/**************************************************************************
* FILENAME: mi_link.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "directorios.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> </ruta_fichero_original> </ruta_enlace>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta_original = argv[2];
    char *ruta_enlace = argv[3];

    DEBUG(1, "nombre_dispositivo: %s", nombre_dispositivo);
    DEBUG(1, "ruta_original: %s", ruta_original);
    DEBUG(1, "ruta_enlace: %s", ruta_enlace);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    int ret = mi_link(ruta_original, ruta_enlace);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: %s", nombre_dispositivo);
        return FALLO;
    }

    return ret;
}

