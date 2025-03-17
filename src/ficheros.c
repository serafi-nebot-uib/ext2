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

        // fprintf(stderr, GRAY "mi_write_f() -> boff = %u\n" RESET, boff);
        // fprintf(stderr, GRAY BOLD "mi_write_f() -> buf_original (src): \n:" RESET);
        // for (size_t i = 0; i < nbytes; i++) fprintf(stderr, GRAY "%02X " RESET, src[i]); 
        // fprintf(stderr, "\n\n");
        // fprintf(stderr, GRAY BOLD "mi_write_f() -> buff escrito (bloque completo): \n" RESET);
        // for (size_t i = 0; i < BLOCKSIZE; i++){
        //     if(i >= boff && i < boff+size) fprintf(stderr, GRAY "%02X " RESET, dst[i]);
        //     else fprintf(stderr, GREEN "%02X " RESET, dst[i]); 
        // }
    }

    // fprintf(stderr, "\n");

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
 * @param ninodo
 * @param buf_original
 * @param offset
 * @param nbytes
 * @return bytesLeidos
*/
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    unsigned char * dst = (unsigned char *) buf_original;
    unsigned char buff[BLOCKSIZE] = {}; // Buffer de un bloque 
    memset(buff, 0, BLOCKSIZE);

    int bytesLeidos = 0; // Número de bytes leídos realmente
    inodo_t inodo = {};
    
    if(leer_inodo(ninodo, &inodo) == -1) return FALLO;

    // Comprueba que el inodo tenga permisos de lectura
    if (!INODE_P(inodo.permisos, INODE_P_READ)) {
        ERROR("inodo %d no tiene permisos de lectura", ninodo);
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

    // fprintf(stderr, GRAY "ultimoByteLogico: %d\n" RESET, ultimoByteLogico);
    // fprintf(stderr, GRAY "primerBL: %d\n" RESET, primerBL);
    // fprintf(stderr, GRAY "ultimoBL: %d\n" RESET, ultimoBL);
    // fprintf(stderr, GRAY "desp1: %d\n" RESET, desp1);
    // fprintf(stderr, GRAY "desp2: %d\n\n" RESET, desp2);

    int nbfisico;
    unsigned int index = 0; //bytesCopiados, controla donde se escriben en el array buf_original los datos leídos en cada iteracion

    unsigned int nblogico;
    for(nblogico = primerBL; nblogico <= ultimoBL; nblogico++){
        //fprintf(stderr, GRAY "for(nblogico...) iteración %d (nblogico = %d)\n" RESET, primerBL-nblogico, nblogico);

        if((nbfisico = traducir_bloque_inodo(ninodo, nblogico, 0)) == -1){ //Consigue el bfisico asociado al blogico
            // fprintf(stderr, GRAY "traducir_bloque_inodo() no ha conseguido el bfisico asociado al blogico [nbfisico = %d]\n" RESET, nbfisico);
            bytesLeidos += BLOCKSIZE; // Si no existe el bloque físico asociado al blogico, incrementa el contador
            continue;                 // y salta a la siguiente iteración
        }

        // Lee el bloque físico e incrementa el contador de bytes leídos
        int tmp;
        if((tmp = bread(nbfisico, buff)) == FALLO) return FALLO;
        bytesLeidos += tmp;
        //bytesLeidos += BLOCKSIZE;

        // fprintf(stderr, GRAY "mi_read_f() -> buff leido (bloque completo): \n" RESET);
        // for (size_t i = 0; i < BLOCKSIZE; i++){
        //     if(i >= desp1 && i < desp1+nbytes) fprintf(stderr, GRAY "%02X " RESET, buff[i]); 
        //     else fprintf(stderr, CYAN "%02X " RESET, buff[i]); 
        // }
        // fprintf(stderr, "\n\n:");
        // for (size_t i = desp1; i < desp1+nbytes; i++) fprintf(stderr, "%02X ", buff[i]); 
        // fprintf(stderr, "\n");
        
        if(nblogico == primerBL) { //Si es la 1era iteración
            // fprintf(stderr, GRAY "if(nblogico == primerBL): iteración %d (nblogico = %d)\n" RESET, primerBL-nblogico, nblogico);
            int size = 0;
            if(primerBL == ultimoBL) size = nbytes; // si el numBytes a leer no ocupan más de un bloque
            else size = BLOCKSIZE - desp1; 

            memcpy(&dst[0], &buff[desp1], size); // tamaño a leer en el primer bloque BLOCKSIZE-numBytesIgnorados(desp1)
            index += size; 
        } else if(nblogico == ultimoBL) { 
            // fprintf(stderr, GRAY "if(nblogico == ultimoBL): iteración %d (nblogico = %d)\n" RESET, primerBL-nblogico, nblogico);
            memcpy(&dst[index], &buff[0], desp2 + 1); //el+1 (?) pq?? tamaño a leer en el último bloque desde 0 hasta desp2
            index += desp2;
        } else { // Si es un bloque intermedio (no es ni el primer bloque ni el último, por tanto no hay offset)
            // fprintf(stderr, GRAY "if(nblogico == bloque intermedio): iteración %d (nblogico = %d)\n" RESET, primerBL-nblogico, nblogico);
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
