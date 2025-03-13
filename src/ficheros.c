#include "ficheros.h"
#include "bloques.h"
#include "ficheros_basico.h"

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) {
        ERROR("inodo %d no tiene permisos de escritura", ninodo);
        return FALLO;
    }

    unsigned char * const src = (unsigned char *) buf_original;
    unsigned char dst[BLOCKSIZE] = {};

    const unsigned int start = offset;
    const unsigned int end = offset + nbytes;
    const unsigned int block_start = start / BLOCKSIZE;
    const unsigned int block_end = end / BLOCKSIZE;

    DEBUG("start: %u", start);
    DEBUG("end: %u", end);
    DEBUG("block_start: %u", block_start);
    DEBUG("block_end: %u", block_end);

    for (unsigned int i = block_start; i <= block_end; i++) {
        int block = traducir_bloque_inodo(ninodo, i, 1);
        if (block == FALLO) return FALLO;
        if (bread(block, dst) == FALLO) return FALLO;

        unsigned int block_off = offset % BLOCKSIZE;
        unsigned int size = (i == block_end ? (end % BLOCKSIZE) : BLOCKSIZE) - block_off;
        if (size == 0) break;

        DEBUG("[%u] offset: %d", i, offset);
        DEBUG("[%u] block: %d", i, block);
        DEBUG("[%u] block_off: %d", i, block_off);
        DEBUG("[%u] size: %d", i, size);

        memcpy(&dst[block_off], &src[offset - start], size);
        offset += size;

        // for (unsigned int i = 0; i < BLOCKSIZE; i++) {
        //     if (i % 16 == 0) printf("\n");
        //     printf("%02x ", dst[i]);
        // }
        // printf("\n");

        if (bwrite(block, dst) == FALLO) return FALLO;
    }

    unsigned int size = offset - start;

    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    if (inodo.tamEnBytesLog < start + size) inodo.tamEnBytesLog = start + size;

    time_t t = time(NULL);
    inodo.mtime = t;
    inodo.ctime = t;

    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return size;
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
