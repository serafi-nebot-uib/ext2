/**************************************************************************
* FILENAME: permitir.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4 || argv[2][0] == '-') { //Aseguramos que es un ninodo positivo
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <nombre_dispositivo> <ninodo> <permisos>\n", argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1];
    const unsigned int ninodo = atoi(argv[2]);
    const unsigned char perm = atoi(argv[3]);

    if (perm > 7) {
        ERROR("permisos %hhu incorrectos; debe ser un valor 0-7", perm);
        return FALLO;
    }

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    if (mi_chmod_f(ninodo, perm) == FALLO) return FALLO;

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return 0;
}
