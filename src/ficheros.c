#include "ficheros.h"
#include "bloques.h"
#include "ficheros_basico.h"

/**
 * Escribir n bytes a los datos de un inodo.
 *
 * @param ninodo numero de inodo al que escribir
 * @param buf_original buffer de datos origen; de dónde se van a volcar los datos
 * @param offset numero de byte del inodo del cual empezar a escribir
 * @param nbytes numero de bytes a escribir
 * @return numero de bytes escritos, FALLO en caso de error
 */
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

/** 
 *
 * @param ninodo
 * @param buf_original
 * @param offset
 * @param nbytes
 * @return bytesLeidos
*/
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned char * dst = (unsigned char *) buf_original;
    unsigned char buff[BLOCKSIZE] = {}; //Buffer de un bloque 
    int bytesLeidos = 0; // Número de bytes leídos realmente
    inodo_t inodo = {};
    
    if(leer_inodo(ninodo, &inodo) == -1) return FALLO;

    // Comprueba que el inodo tenga permisos de lectura
    if (!INODE_P(inodo.permisos, INODE_P_READ)) {
        fprintf(stderr, RED "No hay permisos de lectura\n" RESET); // POSAR DEBUG()
        return FALLO;
    }

    // No podemos leer nada
    if(offset >= inodo.tamEnBytesLog) return (bytesLeidos = 0);

    // pretende leer más allá de EOF, leemos sólo los bytes que podemos desde el offset hasta EOF
    if((offset + nbytes) >= inodo.tamEnBytesLog) nbytes = inodo.tamEnBytesLog - offset;

    // offset: posición inicial en bytes
    int ultimoByteLogico = offset + nbytes - 1;
    int primerBL = offset / BLOCKSIZE; // primer bloque lógico
    int ultimoBL = (offset + nbytes -1) / BLOCKSIZE; // último bloque lógico
    int desp1 = offset % BLOCKSIZE; // Bytes de offset, desplazamiento dentro del bloque INICIAL
    int desp2 = ultimoByteLogico % BLOCKSIZE; // Bytes de offset, desplazamiento dentro del ÚLTIMO bloque
    
    int nbfisico;
    unsigned int index = 0; //bytesCopiados, controla donde se escriben en el array buf_original los datos leídos en cada iteracion

    unsigned int nblogico = primerBL;
    for(nblogico = primerBL; nblogico <= ultimoBL; nblogico++){
        if((nbfisico = traducir_bloque_inodo(ninodo, nblogico, 0)) == -1){ //Consigue el bfisico asociado al blogico
            bytesLeidos += BLOCKSIZE; // Si no existe el bloque físico asociado al blogico, incrementa el contador
            continue;                 // y salta a la siguiente iteración
        }
        // ¿Se deja el espacio en buf_original de los bloques saltados haciendo una copia exacta del fichero?
        //  o se hace append de los bloques existentes?
        //  Si se debe dejar espacio entonces buf_original[bytesLeidos] a la hora de hace memcpy


        if((bytesLeidos = bread(nbfisico, buff)) == -1) return FALLO; // Lee el bloque físico
        bytesLeidos += BLOCKSIZE; // Incrementa el contador de bytes leídos sumándole un bloque

        if(nblogico == primerBL || primerBL == ultimoBL) { //Si es la 1era iteración o el numBytes a leer no ocupan más de un bloque
            memcpy(&dst[index], &buff[desp1], BLOCKSIZE - desp1); // tamaño a leer en el primer bloque BLOCKSIZE-numBytesIgnorados(desp1)
            index += BLOCKSIZE - desp1; 
        } else if(nblogico == ultimoBL) { 
            memcpy(&dst[index], &buff[0], desp2+1); //el+1 (?) pq?? tamaño a leer en el último bloque desde 0 hasta desp2
            index += desp2+1;
        } else { // Si es un bloque intermedio (no es ni el primer bloque ni el último, por tanto no hay offset)
            memcpy(&dst[index], &buff, BLOCKSIZE); // Copia el bloque entero leído en buf_original
            index += BLOCKSIZE;
        }
    }

    inodo.atime = time(NULL); // Actualiza la fecha de último acceso al inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;
    return bytesLeidos;
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
