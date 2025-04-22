/**************************************************************************
* FILENAME: mi_mkfs.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <disco> <permisos> </ruta>\n", argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1];
    const char perm = argv[2][0];
    const char *const path = argv[3];

    if (bmount(dev_name) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", dev_name);
        return FALLO;
    } else DEBUG(7, "test");

    int ret = mi_creat(path, perm);
    if (ret != 0) mostrar_error_buscar_entrada(ret);

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", dev_name);
        return FALLO;
    }

    return 0;
}
