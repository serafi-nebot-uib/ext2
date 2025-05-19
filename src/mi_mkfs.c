/**************************************************************************
* FILENAME: mi_mkfs.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "sintaxis: %s <nombre_dispositivo> <nbloques>\n" RESET, argv[0]);
        return FALLO;
    }

    const char *const dev_name = argv[1];
    const int block_cnt = atoi(argv[2]);

    if (bmount(dev_name) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    for (int i = 0; i < block_cnt; i++) {
        if (bwrite(i, buffer) == FALLO) {
            fprintf(stderr, "error al escribir el bloque %d\n", i);
            bumount();
            return FALLO;
        }
    }

    int ret = EXITO;
    if (initSB(block_cnt, block_cnt / 4) == FALLO || initMB() == FALLO || initAI() == FALLO || reservar_inodo('d', 7) == FALLO) ret = FALLO;

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo virtual %s", dev_name);
        return FALLO;
    }

    return ret;
}
