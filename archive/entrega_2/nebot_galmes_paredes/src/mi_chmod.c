/**************************************************************************
* FILENAME: mi_chmod.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "sintaxis: %s <disco> <permisos> </ruta>\n" RESET, argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1];
    const unsigned char perm = atoi(argv[2]);
    const char *const path = argv[3];

    if (perm > 7) {
        ERROR("permisos %hhu incorrectos; debe ser un valor 0-7", perm);
        return FALLO;
    }

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    int ret = mi_chmod(path, perm);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return ret;
}
