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
    bread(posSB, &sb);
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
    for (int i = 0; i < mb_block_cnt; i++) bwrite(sb.posPrimerBloqueMB + i, buff);

    buff[mb_extra_byte_off] = ~((1 << (8 - mb_extra_bit_cnt)) - 1);
    DEBUG("buff[%d]: %hhu", mb_extra_byte_off, buff[mb_extra_byte_off]);
    for (int i = mb_extra_byte_off+1; i < BLOCKSIZE; i++) buff[i] = 0;
    bwrite(sb.posPrimerBloqueMB + mb_block_cnt, buff);

    sb.cantBloquesLibres -= mb_bit_cnt; // mb_bit_cnt = cantidad de bloques que ocupan los metadatos
    bwrite(posSB, &sb);

    return 0;
}

// int initAI();
