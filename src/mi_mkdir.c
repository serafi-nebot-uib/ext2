/**************************************************************************
* filename: mi_mkdir.c
* author: serafí nebot, ignasi paredes, jaume galmés
**************************************************************************/

#include "core/directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> <permisos> </ruta>\n", argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1];
    const unsigned char perm = atoi(argv[2]);
    const char *const path = argv[3];

    if (perm > 7) {
        ERROR("permisos %hhu incorrectos; debe ser un valor 0-7", perm);
        return FALLO;
    }

    if (path[strlen(path) - 1] != '/') {
        ERROR("la ruta debe terminar en '/'");
        return FALLO;
    }

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    int ret = mi_creat(path, perm);
    if (ret != 0) mostrar_error_buscar_entrada(ret);

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return ret;
}
