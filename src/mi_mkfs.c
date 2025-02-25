/**************************************************************************
* FILENAME: mi_mkfs.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "bloques.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, BOLD "uso: " RESET "%s <nombre_dispositivo> <nbloques>\n", argv[0]);
        return FALLO;
    }

    const char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]);

    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
            fprintf(stderr, "error al escribir el bloque %d\n", i);
            bumount();
            return FALLO;
        }
    }

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    return EXITO;
}
