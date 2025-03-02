#include "ficheros_basico.h"
#include "bloques.h"

static unsigned char block_buff[BLOCKSIZE] = {};
static superbloque_t sb = {};

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
    if (bread(posSB, &sb) == FALLO) return FALLO;

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

    memset(block_buff, 0xff, BLOCKSIZE);
    for (int i = 0; i < mb_block_cnt; i++)
        if (bwrite(sb.posPrimerBloqueMB + i, block_buff) == FALLO) return FALLO;

    block_buff[mb_extra_byte_off] = ~((1 << (8 - mb_extra_bit_cnt)) - 1);
    DEBUG("block_buff[%d]: %hhu", mb_extra_byte_off, block_buff[mb_extra_byte_off]);
    for (int i = mb_extra_byte_off+1; i < BLOCKSIZE; i++) block_buff[i] = 0;
    if (bwrite(sb.posPrimerBloqueMB + mb_block_cnt, block_buff) == FALLO) return FALLO;

    sb.cantBloquesLibres -= mb_bit_cnt; // mb_bit_cnt = cantidad de bloques que ocupan los metadatos
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    return EXITO;
}

/**
 * Inicializar el array de inodos libres del sistema de ficheros
 */
int initAI() {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    inodo_t inodos[BLOCKSIZE / INODOSIZE];
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

    return EXITO;
}

/**
 * Escribir el valor del parametro bit al bit del mapa de bits correspondiente al numero de bloque indicado por el parametro nbloque.
 * Se escribe 0 si bit = 0, se escribe 1 si bit != 0.
 *
 * @param nbloque numero de bloque que modificar en el mapa de bits
 * @param bit nuevo valor del bit a midificar
 * @return 0 si se escribe el valor correctamente, FALLO en caso contrario
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int pos_byte = nbloque / 8;
    unsigned int pos_bit = nbloque % 8;
    unsigned int idx_byte = pos_byte % BLOCKSIZE;
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE;

    DEBUG("pos_byte: %d", pos_byte);
    DEBUG("pos_bit: %d", pos_bit);
    DEBUG("idx_byte: %d", idx_byte);
    DEBUG("idx_block: %d", idx_block);

    if (bread(idx_block, block_buff) == FALLO) return FALLO;

    char mask = 1 << (7 - pos_bit);
    DEBUG("mask: 0x%1$02x = %1$hhu", mask);
    DEBUG("previous value: 0x%1$02x = %1$hhu", block_buff[idx_byte]);
    if (bit) block_buff[idx_byte] |= mask;
    else block_buff[idx_byte] &= ~mask;
    DEBUG("new value: 0x%1$02x = %1$hhu", block_buff[idx_byte]);

    if (bwrite(idx_block, block_buff) == FALLO) return FALLO;

    return EXITO;
}

/**
 * Leer el valor del bit del mapa de bits correspondiente al numero de bloque indicado por el parametro nbloque.
 *
 * @param nbloque numero de bloque del cual leer el bit
 * @return valor del bit correspondiente a nbloque
 */
int leer_bit(unsigned int nbloque) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int pos_byte = nbloque / 8;
    unsigned int pos_bit = nbloque % 8;
    unsigned int idx_byte = pos_byte % BLOCKSIZE;
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE;

    DEBUG("pos_byte: %d", pos_byte);
    DEBUG("pos_bit: %d", pos_bit);
    DEBUG("idx_byte: %d", idx_byte);
    DEBUG("idx_block: %d", idx_block);

    if (bread(idx_block, block_buff) == FALLO) return FALLO;
    char mask = 1 << (7 - pos_bit);
    DEBUG("value: 0x%1$02x = %1$hhu", block_buff[idx_byte]);
    DEBUG("mask: 0x%1$02x = %1$hhu", mask);
    return (block_buff[idx_byte] & mask) != 0;
}

/**
 * Reservar el primer bloque libre y lo resetea a 0.
 */
int reservar_bloque() {
    if (bread(posSB, &sb) == FALLO) return FALLO;
    if (sb.cantBloquesLibres == 0) return FALLO;

    // iterar todos los bloques del mapa de bits para obtener el primer bloque que contiene algun byte que tiene algun bit a 0
    // utilizamos un buffer auxiliar para poder hacer la comparación con memcmp, que puede ser mas eficiente que iterar todo el bloque manualmente
    unsigned char aux[BLOCKSIZE] = {};
    memset(aux, 0xff, BLOCKSIZE);
    unsigned int block_cnt_mb = sb.posUltimoBloqueMB - sb.posPrimerBloqueMB;
    unsigned int nblock_mb = 0;
    DEBUG("sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG("sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG("block_cnt_mb: %u", block_cnt_mb);
    for (; nblock_mb < block_cnt_mb; nblock_mb++) {
        if (bread(sb.posPrimerBloqueMB + nblock_mb, block_buff) == FALLO) return FALLO;
        if (memcmp(block_buff, aux, BLOCKSIZE)) break;
    }
    DEBUG("nblock_mb: %u", nblock_mb);

    // obtener la posición del primer byte del bloque que tiene algun bit a 0
    unsigned int nbyte = 0;
    while (nbyte < BLOCKSIZE && block_buff[nbyte] == 0xff) nbyte++;
    DEBUG("nbyte: %u", nbyte);

    // obtener la posición del primer bit que esta a 0
    unsigned char nbit = 0;
    unsigned char val = block_buff[nbyte];
    // unsigned char val = 0xf0;
    while (val & 0x80) {
        DEBUG("val: 0x%1$02x = %1$u", val);
        val <<= 1;
        nbit++;
    }
    DEBUG("nbit: %u", nbit);

    // modificar la zona de metadatos para que el bloque quede reservado
    unsigned int nblock = (nblock_mb * BLOCKSIZE + nbyte) * 8 + nbit; 
    DEBUG("nblock: %u", nblock);
    if (escribir_bit(nblock, 1) == FALLO) return FALLO;
    sb.cantBloquesLibres--;
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    // limpiar el bloque reservado, en caso de que sea un bloque reutilizado
    memset(aux, 0, BLOCKSIZE);
    if (bwrite(nblock, aux) == FALLO) return FALLO;

    return nblock;
}
