#include "ficheros.h"
#include "bloques.h"
#include "ficheros_basico.h"

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
    // obtener inodo a partir del número de inodo y comprobar que tiene permisos de escritura
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) {
        ERROR("inodo %d no tiene permisos de escritura", ninodo);
        return FALLO;
    }

    unsigned char * const src = (unsigned char *) buf_original; // puntero al buffer de origen
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

/** 
 * Lee los nbytes de los datos de un inodo a partir de un offset dado
 * 
 * @param ninodo número de inodo del que leer
 * @param buf_original buffer de datos destino; dónde se van a volcar los datos
 * @param offset número de byte del inodo del cual empezar a leer
 * @param nbytes número de bytes a escribir
 * @return número de bytes escritos, FALLO en caso de error
*/
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned char * dst = (unsigned char *) buf_original;
    unsigned char buff[BLOCKSIZE] = {}; // buffer de un bloque

    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == -1) return FALLO;
    // comprueba que el inodo tenga permisos de lectura
    if (!INODE_P(inodo.permisos, INODE_P_READ)) {
        ERROR("inodo %d no tiene permisos de lectura", ninodo);
        return FALLO;
    }

    // no podemos leer nada
    if (offset >= inodo.tamEnBytesLog) return 0;

    // pretende leer más allá de EOF, leemos sólo los bytes que podemos desde el offset hasta EOF
    if ((offset + nbytes) >= inodo.tamEnBytesLog) nbytes = inodo.tamEnBytesLog - offset;

    // offset: posición inicial en bytes
    int ultimoByteLogico = offset + nbytes - 1;
    int primerBL = offset / BLOCKSIZE; // primer bloque lógico
    int ultimoBL = ultimoByteLogico / BLOCKSIZE; // último bloque lógico
    int desp1 = offset % BLOCKSIZE; // bytes de offset, desplazamiento dentro del bloque INICIAL
    int desp2 = ultimoByteLogico % BLOCKSIZE; // bytes de offset, desplazamiento dentro del ÚLTIMO bloque

    DEBUG(3, "ultimoByteLogico: %d", ultimoByteLogico);
    DEBUG(3, "primerBL: %d", primerBL);
    DEBUG(3, "ultimoBL: %d", ultimoBL);
    DEBUG(3, "desp1: %d", desp1);
    DEBUG(3, "desp2: %d", desp2);

    int nbfisico;
    unsigned int index = 0; // bytes copiados, controla donde se escriben en el array buf_original los datos leídos en cada iteracion

    for (unsigned int nblogico = primerBL; nblogico <= ultimoBL; nblogico++) {
        if ((nbfisico = traducir_bloque_inodo(ninodo, nblogico, 0)) == FALLO) { // consigue el bfisico asociado al blogico
            DEBUG(1, "no se ha podido obtener el bloque físico asociado al bloque lógico %u", nblogico);
            index += BLOCKSIZE; // si no existe el bloque físico asociado al blogico, incrementa el contador
            continue;           // y salta a la siguiente iteración
        }

        // lee el bloque físico
        if (bread(nbfisico, buff) == FALLO) return FALLO;

        if (nblogico == primerBL) { // si es la 1era iteración
            // el tamaño a copiar depende del último byte a leer (el último bloque és el único caso especial)
            int size = primerBL == ultimoBL ? nbytes : BLOCKSIZE - desp1;
            memcpy(dst, &buff[desp1], size); // tamaño a leer en el primer bloque BLOCKSIZE-numBytesIgnorados(desp1)
            index += size; 
        } else if (nblogico == ultimoBL) {
            memcpy(&dst[index], &buff[0], desp2 + 1);
            index += desp2 + 1;
        } else { // si es un bloque intermedio (no es ni el primer bloque ni el último, por tanto no hay offset)
            memcpy(&dst[index], &buff, BLOCKSIZE); // copia el bloque entero leído en buf_original
            index += BLOCKSIZE;
        }
    }

    inodo.atime = time(NULL); // actualiza la fecha de último acceso al inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return index;
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
    memcpy(p_stat, &inodo, sizeof(*p_stat));
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
    inodo_t inodo = {};
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    inodo.permisos = permisos;
    inodo.ctime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;
    return EXITO;
}
