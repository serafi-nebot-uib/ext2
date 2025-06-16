/**************************************************************************
* FILENAME: ficheros.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "ficheros.h"
#include "bloques.h"

/**
 * Escribir n bytes a los datos de un inodo.
 *
 * @param ninodo número de inodo al que escribir
 * @param buf_original buffer de datos origen; de dónde se van a volcar los datos
 * @param offset número de byte del inodo del cual empezar a escribir
 * @param nbytes número de bytes a escribir
 * @return número de bytes escritos, FALLO en caso de error
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    // TODO: as an improvement, do not wait at the very beginning and signal at the very end of the function.
    //       this can be optimized to a lower level of granularity
    mi_waitSem();
    // obtener inodo a partir del número de inodo y comprobar que tiene permisos de escritura
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    // comprobar que el inodo tenga permisos de escritura
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) {
        ERROR("no hay permisos de escritura");
        mi_signalSem();
        return FALLO;
    }

    unsigned char *const src = (unsigned char *) buf_original;  // puntero al buffer de origen
    unsigned char dst[BLOCKSIZE] = {}; // puntero al buffer de destino (se usa como buffer temporal para leer el bloque, modificar los datos y escribir)

    const unsigned int start = offset; // número de byte al que empezar a escribir
    const unsigned int end = offset + nbytes; // número de byte al que terminar de escribir
    const unsigned int bstart = start / BLOCKSIZE; // número de bloque lógico inicial
    const unsigned int bend = end / BLOCKSIZE; // número de bloque lógico final

    // iterar sobre el rango de bloques lógicos a escribir
    for (unsigned int i = bstart; i <= bend; i++) {
        // calcular el byte inicial y el tamaño a escribir dentro del bloque
        unsigned int boff = offset % BLOCKSIZE;

        unsigned int size = (i == bend ? (end % BLOCKSIZE) : BLOCKSIZE) - boff;
        if (size == 0) break;

        // obtener el numero de bloque fisico a partir del numero de bloque logico
        int bn = traducir_bloque_inodo(ninodo, i, 1);
        if (bn == FALLO) {
            mi_signalSem();
            return FALLO;
        }

        // leer el bloque físico, escribir cambios en el buffer temporal y escribir al bloque físico
        if (bread(bn, dst) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        memcpy(&dst[boff], &src[offset - start], size);
        offset += size;
        if (bwrite(bn, dst) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    }

    unsigned int size = offset - start; // número total de bytes escritos

    // actualizar tamEnBytesLog, si hemos ampliado el tamaño total del inodo, mtime y ctime
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    time_t t = time(NULL);
    inodo.mtime = t; // actualizar el mtime (modificación de datos)

    // si es necesario, incrementar el tamaño del fichero
    if (inodo.tamEnBytesLog < start + size) {
        inodo.tamEnBytesLog = start + size;
        inodo.ctime = t; // actualizar el ctime (modificación del inodo)
    }

    // escribir el inodo actualizado
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return size;
}

/**
 * Lee los nbytes de los datos de un inodo a partir de un offset dado
 *
 * @param ninodo número de inodo del que leer
 * @param buf_original buffer de datos destino; dónde se van a volcar los datos
 * @param offset número de byte del inodo del cual empezar a leer
 * @param nbytes número de bytes a escribir
 * @return número de bytes leídos, FALLO en caso de error
 */
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned char *dst = (unsigned char *)buf_original;
    unsigned char buff[BLOCKSIZE] = {}; // buffer de un bloque

    // TODO: as an improvement, do not wait at the very beginning and signal at the very end of the function.
    //       this can be optimized to a lower level of granularity
    mi_waitSem();
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == -1) {
        mi_signalSem();
        return FALLO;
    }
    // comprueba que el inodo tenga permisos de lectura
    if (!INODE_P(inodo.permisos, INODE_P_READ)) {
        ERROR("no hay permisos de lectura");
        mi_signalSem();
        return FALLO;
    }
    inodo.atime = time(NULL); // actualiza la fecha de último acceso al inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();

    // no podemos leer nada
    if (offset >= inodo.tamEnBytesLog) return 0;

    // pretende leer más allá de EOF, leemos sólo los bytes que podemos desde el offset hasta EOF
    if ((offset + nbytes) >= inodo.tamEnBytesLog) nbytes = inodo.tamEnBytesLog - offset;

    // bytes de inicio y final a leer
    const unsigned int start = offset, end = offset + nbytes;
    // inicio y final de bloques lógicos
    const unsigned int bstart = start / BLOCKSIZE, bend = end / BLOCKSIZE;
    // offset final del último bloque
    const unsigned int end_off = end % BLOCKSIZE;

    unsigned int idx = 0; // índice del próximo byte a leer
    // iterar sobre bloques lógicos del principio
    for (unsigned int i = bstart; i <= bend; i++) {
        unsigned int off = (offset + idx) % BLOCKSIZE; // offset del byte a leer dentro del bloque actual
        unsigned int size = (i == bend ? end_off : BLOCKSIZE) - off; // tamaño a leer del bloque actual
        if (size == 0) break; // ya no hay más datos a leer

        int bn = traducir_bloque_inodo(ninodo, i, 0); // numero de bloque lógico a leer (se utiliza sólo dentro del bucle)
        if (bn != FALLO) {
            if (bread(bn, buff) == FALLO) return FALLO; // leer el bloque actual entero y cargarlo a un buffer temporal
            memcpy(&dst[idx], &buff[off], size); // copiar el buffer temporal al buffer de datos de destino
        }

        idx += size; // incrementar el índice actual con el tamaño de bytes leídos
    }

    return idx;
}

/**
 * Obtener metainformación de un inodo.
 *
 * @param ninodo numero de inodo del cual leer la metainformación
 * @param p_stat estructura de datos a la cual volcar la metainformación
 * @return EXITO si no hay error, FALLO en caso contrario
 */
int mi_stat_f(unsigned int ninodo, stat_t *p_stat) {
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    memcpy(p_stat, &inodo, sizeof(*p_stat)); // copiar los datos del inodo al struct stat
    return EXITO;
}

/**
 * Modificar los permisos de un inodo.
 *
 * @param ninodo numero de inodo del cual modificar los permisos
 * @param permisos nuevos permisos
 * @return EXITO si no hay error, FALLO en caso contrario
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    mi_waitSem();

    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    inodo.permisos = permisos; // actualizar los permisos del inodo
    inodo.ctime = time(NULL); // actualizar el ctime (modificación del inodo)

    // escribimos el inodo modificado
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return EXITO;
}
