#include "ficheros.h"
#include "bloques.h"
#include "ficheros_basico.h"

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    // obtener inodo a partir del numero de inodo y comprobar que tiene permisos de escritura
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) {
        ERROR("inodo %d no tiene permisos de escritura", ninodo);
        return FALLO;
    }

    unsigned char * const src = (unsigned char *) buf_original; // puntero al buffer de origen
    unsigned char dst[BLOCKSIZE] = {}; // puntero al buffer de destino (se usa como buffer temporal para leer el bloque, modificar los datos y escribir)

    const unsigned int start = offset; // numero de byte al que empezar a escribir
    const unsigned int end = offset + nbytes; // numbero de byte al que terminar de escribir
    const unsigned int bstart = start / BLOCKSIZE; // numero de bloque lógico inicial
    const unsigned int bend = end / BLOCKSIZE; // numero de bloque lógico final

    // iterar sobre el rango de bloques lógicos a escribir
    for (unsigned int i = bstart; i <= bend; i++) {
        // calcular el byte inicial y el tamaño a escribir dentro del bloque
        unsigned int boff = offset % BLOCKSIZE;
        unsigned int size = (i == bend ? (end % BLOCKSIZE) : BLOCKSIZE) - boff;
        if (size == 0) break;

        // obtener el numero de bloque fisico a partir del numero de bloque logico
        int bn = traducir_bloque_inodo(ninodo, i, 1);
        if (bn == FALLO) return FALLO;

        // leer el bloque físico, escribir cambios en el buffer temporal y escribir al bloque físico
        if (bread(bn, dst) == FALLO) return FALLO;
        memcpy(&dst[boff], &src[offset - start], size);
        offset += size;
        if (bwrite(bn, dst) == FALLO) return FALLO;
    }

    unsigned int size = offset - start; // número total de bytes escritos

    // actualizar tamEnBytesLog, si hemos ampliado el tamaño total del inodo, mtime y ctime
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
