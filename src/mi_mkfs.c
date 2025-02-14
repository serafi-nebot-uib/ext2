#include "bloques.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, BOLD "usage: " RESET "%s <device_name> <block_count>\n", argv[0]);
        return FALLO;
    }

    const char *name = argv[1];
    int block_cnt = atoi(argv[2]);

    if (bmount(name) == FALLO) {
        fprintf(stderr, "error mounting device %s\n", name);
        return FALLO;
    }

    unsigned char buf[BLOCKSIZE];
    memset(buf, 0, BLOCKSIZE);

    for (int i = 0; i < block_cnt; i++) {
        if (bwrite(i, buf) == FALLO) {
            fprintf(stderr, "error writing block %d\n", i);
            bumount();
            return FALLO;
        }
    }

    if (bumount() == FALLO) {
        fprintf(stderr, "error unmounting device %s\n", name);
        return FALLO;
    }

    return EXITO;
}
