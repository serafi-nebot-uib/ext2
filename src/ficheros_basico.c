#include "ficheros_basico.h"
#include "bloques.h"

/**
 * Calcular el tamaño en bloques para el mapa de bits
 *
 * @param nbloques número total de bloques
 * @return número de bloques para el mapa de bits
 */
int tamMB(unsigned int nbloques) {
    int res = nbloques / 8;
    return (res / BLOCKSIZE) + ((res % BLOCKSIZE) != 0);
}

/**
 * Calcular el tamaño en bloques para el array de inodos
 *
 * @param ninodos número total de inodos del sistema de ficheros
 * @return número de de bloques para el array de inodos
 */
int tamAI(unsigned int ninodos) {
    int res = (ninodos * INODOSIZE);
    return (res / BLOCKSIZE) + ((res % BLOCKSIZE) != 0);
}

/**
 * Inicializar los datos del superbloque
 *
 * @param nbloques número de bloques del sistema de ficheros
 * @return número de bytes escritos, FALLO en caso de error
 */
int initSB(unsigned int nbloques, unsigned int ninodos) {
    superbloque_t sb = {};
    sb.posPrimerBloqueMB = posSB + tamSB;
    sb.posUltimoBloqueMB = sb.posPrimerBloqueMB + tamMB(nbloques) - 1;
    sb.posPrimerBloqueAI = sb.posUltimoBloqueMB + 1;
    sb.posUltimoBloqueAI = sb.posPrimerBloqueAI + tamAI(ninodos) - 1;
    sb.posPrimerBloqueDatos = sb.posUltimoBloqueAI + 1;
    sb.posUltimoBloqueDatos = nbloques - 1;
    sb.posInodoRaiz = 0;
    sb.posPrimerInodoLibre = 0;
    sb.cantBloquesLibres = nbloques;
    sb.cantInodosLibres = ninodos;
    sb.totBloques = nbloques;
    sb.totInodos = ninodos;
    return bwrite(posSB, &sb);
}

/**
 * Inicializar el mapa de bits del sistema de ficheros
 */
int initMB() {
    superbloque_t sb = {};
    if (bread(posSB, &sb) == FALLO) return FALLO;
    char buff[BLOCKSIZE] = {};

    DEBUG("sb.posPrimerBloqueMB: %d", sb.posPrimerBloqueMB);
    DEBUG("sb.posUltimoBloqueMB: %d", sb.posUltimoBloqueMB);
    DEBUG("sb.posPrimerBloqueAI: %d", sb.posPrimerBloqueAI);
    DEBUG("sb.posUltimoBloqueAI: %d", sb.posUltimoBloqueAI);
    DEBUG("sb.posPrimerBloqueDatos: %d", sb.posPrimerBloqueDatos);
    DEBUG("sb.posUltimoBloqueDatos: %d", sb.posUltimoBloqueDatos);
    DEBUG("sb.posInodoRaiz: %d", sb.posInodoRaiz);
    DEBUG("sb.posPrimerInodoLibre: %d", sb.posPrimerInodoLibre);
    DEBUG("sb.cantBloquesLibres: %d", sb.cantBloquesLibres);
    DEBUG("sb.cantInodosLibres: %d", sb.cantInodosLibres);
    DEBUG("sb.totBloques: %d", sb.totBloques);
    DEBUG("sb.totInodos: %d", sb.totInodos);


    int mb_bit_cnt = tamSB + tamMB(sb.totBloques) + tamAI(sb.totInodos);
    int mb_byte_cnt = mb_bit_cnt / 8;
    int mb_block_cnt = mb_byte_cnt / BLOCKSIZE;
    int mb_extra_byte_off = mb_byte_cnt % BLOCKSIZE;
    int mb_extra_bit_cnt = mb_bit_cnt % 8;

    DEBUG("mb_bit_cnt: %d", mb_bit_cnt);
    DEBUG("mb_byte_cnt : %d", mb_byte_cnt);
    DEBUG("mb_block_cnt: %d", mb_block_cnt);
    DEBUG("mb_extra_byte_off: %d", mb_extra_byte_off);
    DEBUG("mb_extra_bit_cnt: %d", mb_extra_bit_cnt);

    memset(buff, 0xff, BLOCKSIZE);
    for (int i = 0; i < mb_block_cnt; i++)
        if (bwrite(sb.posPrimerBloqueMB + i, buff) == FALLO) return FALLO;

    buff[mb_extra_byte_off] = ~((1 << (8 - mb_extra_bit_cnt)) - 1);
    DEBUG("buff[%d]: %hhu", mb_extra_byte_off, buff[mb_extra_byte_off]);
    for (int i = mb_extra_byte_off+1; i < BLOCKSIZE; i++) buff[i] = 0;
    if (bwrite(sb.posPrimerBloqueMB + mb_block_cnt, buff) == FALLO) return FALLO;

    sb.cantBloquesLibres -= mb_bit_cnt; // mb_bit_cnt = cantidad de bloques que ocupan los metadatos
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    return 0;
}

/**
 * Inicializar el array de inodos libres del sistema de ficheros
 */
int initAI() {
    inodo_t inodos[BLOCKSIZE / INODOSIZE];

    superbloque_t sb = {};
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int inode_next = sb.posPrimerInodoLibre + 1;
    for (int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI && inode_next < sb.totInodos; i++) {
        if (bread(i, inodos) == FALLO)  return FALLO;
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l';
            if (inode_next < sb.totInodos) inodos[j].punterosDirectos[0] = inode_next++;
            else inodos[j].punterosDirectos[0] = UINT_MAX;
        }
        if (bwrite(i, inodos) == FALLO) return FALLO;
    }

    return 0;
}

/**
 * Escribre el valor del parametro bit al bit del mapa de bits correspondiente al numero de bloque indicado por el parametro nbloque.
 * Se escribe 0 si bit = 0, se escribe 1 si bit != 0.
 *
 * @param nbloque numero de bloque que modificar en el mapa de bits
 * @param bit nuevo valor del bit a midificar
 * @return 0 si se escribe el valor correctamente, FALLO en caso contrario
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    superbloque_t sb = {};
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int pos_byte = nbloque / 8;
    unsigned int pos_bit = nbloque % 8;
    unsigned int idx_byte = pos_byte % BLOCKSIZE;
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE;

    DEBUG("pos_byte: %d", pos_byte);
    DEBUG("pos_bit: %d", pos_bit);
    DEBUG("idx_byte: %d", idx_byte);
    DEBUG("idx_block: %d", idx_block);

    unsigned char buff[BLOCKSIZE] = {};
    if (bread(idx_block, buff) == FALLO) return FALLO;

    char mask = 1 << (7 - pos_bit);
    DEBUG("mask: 0x%1$02x = %1$hhu", mask);
    DEBUG("previous value: 0x%1$02x = %1$hhu", buff[idx_byte]);
    if (bit) buff[idx_byte] |= mask;
    else buff[idx_byte] &= ~mask;
    DEBUG("new value: 0x%1$02x = %1$hhu", buff[idx_byte]);

    if (bwrite(idx_block, buff) == FALLO) return FALLO;

    return 0;
}
