#include "ficheros.h"
#include "ficheros_basico.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    // inodo_t inodo = {};
    // if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    // if (!INODE_P(inodo.permisos, INODE_P_WRITE)) {
    //     ERROR("inodo %d no tiene permisos de escritura", ninodo);
    //     return FALLO;
    // }

    // unsigned char buff[BLOCKSIZE] = {};
    // const unsigned int block_first = offset / BLOCKSIZE;
    // const unsigned int block_last = (offset + nbytes - 1) / BLOCKSIZE;
    // for (unsigned int i = block_first; i <= block_last; i++) {
    //     int block = traducir_bloque_inodo(ninodo, i, 1);
    //     if (block == FALLO) return FALLO;
    //     if (bread(block, buff) == FALLO) return FALLO;
    //     unsigned int block_off = offset % BLOCKSIZE;
    //     unsigned int size = i == block_last ? 0 : BLOCKSIZE - block_off;
    //     memcpy(buff, &buf_original[offset], size);
    //     if (bwrite(block, buff) == FALLO) return FALLO;
    // }

    // // const unsigned int start = offset;
    // // const unsigned int end = offset + nbytes - 1;
    // // while (offset < end) {
    // //     unsigned int block = offset / BLOCKSIZE;
    // //     unsigned int idx = offset % BLOCKSIZE;
    // // }

    return EXITO;
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

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    return EXITO;
}

int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    return EXITO;
}
