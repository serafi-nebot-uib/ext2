/**************************************************************************
* FILENAME: mi_rm.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> </ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta = argv[2];

    DEBUG(2, "nombre_dispositivo: %s", nombre_dispositivo);
    DEBUG(2, "ruta: %s", ruta);

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    int ret = mi_unlink(ruta);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: %s", nombre_dispositivo);
        return FALLO;
    }

    return ret;
}
