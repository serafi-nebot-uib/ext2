// TODO: set include path in makefile
//
#include <stdio.h>

#include "ficheros_basico.h"

#define DEVICE_NAME "disco"

int main(int argc, char **argv) {
    // printf("tamMB(1000): %d\n", tamMB(1000));
    // printf("tamMB(100000): %d\n", tamMB(100000));
    // printf("tamMB(1000000): %d\n", tamMB(1000000));
    // printf("\n");
    //
    // printf("tamAI(5000): %d\n", tamAI(5000));
    // printf("tamAI(25000): %d\n", tamAI(25000));
    // printf("tamAI(125500): %d\n", tamAI(125500));
    // printf("\n");

    if (bmount(DEVICE_NAME) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", DEVICE_NAME);
        return FALLO;
    }

    int block_cnt = 1000000;
    int inode_cnt = block_cnt / 4;
    initSB(block_cnt, inode_cnt);
    initMB();

    escribir_bit(40003, 0);

    bumount();

    return 0;
}
