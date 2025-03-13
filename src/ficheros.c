#include "ficheros.h"
#include "ficheros_basico.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) {
        ERROR("inodo %d no tiene permisos de escritura", ninodo);
        return FALLO;
    }

    unsigned char buff[BLOCKSIZE] = {};
    const unsigned int block_first = offset / BLOCKSIZE;
    const unsigned int block_last = (offset + nbytes - 1) / BLOCKSIZE;
    for (unsigned int i = block_first; i <= block_last; i++) {
        int block = traducir_bloque_inodo(ninodo, i, 1);
        if (block == FALLO) return FALLO;
        if (bread(block, buff) == FALLO) return FALLO;
        unsigned int block_off = offset % BLOCKSIZE;
        unsigned int size = i == block_last ? 0 : BLOCKSIZE - block_off;
        memcpy(buff, &buf_original[offset], size);
        if (bwrite(block, buff) == FALLO) return FALLO;
    }

    // const unsigned int start = offset;
    // const unsigned int end = offset + nbytes - 1;
    // while (offset < end) {
    //     unsigned int block = offset / BLOCKSIZE;
    //     unsigned int idx = offset % BLOCKSIZE;
    // }

    return EXITO;
}

int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    return EXITO;
}

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    return EXITO;
}

int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    return EXITO;
}
