/**************************************************************************
* FILENAME: ficheros_basico.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "ficheros_basico.h"

static unsigned char block_buff[BLOCKSIZE] = {};
static superbloque_t sb = {};
static inodo_t inodos[INODOS_IN_BLOCK] = {};

/**
 * Calcula el tamaño en bloques para el mapa de bits
 *
 * @param nbloques número total de bloques
 * @return número de bloques para el mapa de bits
 */
int tamMB(unsigned int nbloques) {
    int res = nbloques / 8;
    return (res / BLOCKSIZE) + ((res % BLOCKSIZE) != 0);
}

/**
 * Calcula el tamaño en bloques para el array de inodos
 *
 * @param ninodos número total de inodos del sistema de ficheros
 * @return número de de bloques para el array de inodos
 */
int tamAI(unsigned int ninodos) {
    int res = (ninodos * INODOSIZE);
    return (res / BLOCKSIZE) + ((res % BLOCKSIZE) != 0);
}

/**
 * Inicializa los datos del superbloque
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
    return bwrite(posSB, &sb); // Escribe el superbloque en la posición predeterminada del dispositivo virtual
}

/**
 * Inicializa el mapa de bits del sistema de ficheros
 * 
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
 */
int initMB() {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque almacenado en el dispositivo virtual

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

    int mb_bit_cnt = tamSB + tamMB(sb.totBloques) + tamAI(sb.totInodos); // Cantidad inicial de bits ocupados del Mapa de Bits
    int mb_byte_cnt = mb_bit_cnt / 8;                                    // Cantidad inicial de bytes ocupados del Mapa de Bits
    int mb_block_cnt = mb_byte_cnt / BLOCKSIZE;                          // Cantidad inicial de bloques ocupados del Mapa de Bits
    int mb_extra_byte_off = mb_byte_cnt % BLOCKSIZE;                     // Cantidad adicional de bytes ocupados
    int mb_extra_bit_cnt = mb_bit_cnt % 8;                               // Cantidad adicional de bits ocupados

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
    for (int i = mb_extra_byte_off+1; i < BLOCKSIZE; i++) block_buff[i] = 0; // Los bytes restantes del bloque se ponen a 0
    if (bwrite(sb.posPrimerBloqueMB + mb_block_cnt, block_buff) == FALLO) return FALLO;

    sb.cantBloquesLibres -= mb_bit_cnt; // mb_bit_cnt = cantidad de bloques que ocupan los metadatos
    if (bwrite(posSB, &sb) == FALLO) return FALLO; // Vuelve a escribir el superbloque actualizado en el dispositivo virtual

    return EXITO;
}

/**
 * Inicializa el array de inodos libres del sistema de ficheros
 * 
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
 */
int initAI() {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque almacenado en el dispositivo virtual

    inodo_t inodos[BLOCKSIZE / INODOSIZE];
    unsigned int inode_next = sb.posPrimerInodoLibre + 1; 
    for (int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) { // Itera sobre todos los bloques que contienen inodos
        if (bread(i, inodos) == FALLO)  return FALLO; // Lee un bloque y almacena los inodos que este contiene en un array
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l'; // Pone cada inodo del bloque actual como libre
            if (inode_next >= sb.totInodos) {
                inodos[j].punterosDirectos[0] = UINT_MAX; 
                break;
            }
            inodos[j].punterosDirectos[0] = inode_next++; // Al estar todos libres, cada inodo apunta al siguiente
        }
        if (bwrite(i, inodos) == FALLO) return FALLO; // Vuelve a escribir el bloque de inodos en el dispositivo virtual
    }

    return EXITO;
}

/**
 * Escribe el valor del parámetro bit al bit del mapa de bits correspondiente al número de bloque indicado por el parámetro nbloque.
 * Se escribe 0 si bit = 0, se escribe 1 si bit != 0.
 *
 * @param nbloque número de bloque que modificar en el mapa de bits
 * @param bit nuevo valor del bit a modificar
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque almacenado en el dispositivo virtual

    unsigned int pos_byte = nbloque / 8;                                  // Byte dentro del MB que contiene el bit asociado al nbloque
    unsigned int pos_bit = nbloque % 8;                                   // Bit del Byte anterior que contiene el bit asociado al nbloque
    unsigned int idx_byte = pos_byte % BLOCKSIZE; // numBloqueMB          // Núm. de bloque dentro del MB donde se encuentra el Byte anterior
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE; // numBloqueAbs, Núm. de bloque absoluto

    DEBUG("input params: %u [nbloque], %u [bit (bitValue)]", nbloque, bit);
    DEBUG("pos_byte: %d", pos_byte);
    DEBUG("pos_bit: %d", pos_bit);
    DEBUG("idx_byte: %d", idx_byte);
    DEBUG("idx_block: %d", idx_block);

    // Lee el bloque donde se encuentra el bit asociado al nbloque, usando la posición absoluta
    if (bread(idx_block, block_buff) == FALLO) return FALLO;

    unsigned char mask = 1 << (7 - pos_bit);
    DEBUG("MASK value: %2$s (0x%1$02x, %1$hhu)", mask, BIN_STR8(mask));
    DEBUG("prev value: %2$s (0x%1$02x, %1$hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));
    if (bit) block_buff[idx_byte] |= mask; // Si el valor pasado por parámetro es 1, hace una OR entre el byte del MB y la máscara
    else block_buff[idx_byte] &= ~mask;    // Si es 0, hace una AND entre el byte del MB y la máscara negada
    DEBUG("new  value: %2$s (0x%1$02x, %1$hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));

    if (bwrite(idx_block, block_buff) == FALLO) return FALLO; // Escribe el bloque del MB modificado en el dispositivo virtual

    return EXITO;
}

/**
 * Lee el valor del bit del mapa de bits correspondiente al número de bloque indicado por el parámetro nbloque.
 *
 * @param nbloque número de bloque del cual leer el bit
 * @return valor del bit correspondiente a nbloque, FALLO si ha habido un error
 */
int leer_bit(unsigned int nbloque) {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque del dispositivo virtual

    unsigned int pos_byte = nbloque / 8;                                  // Byte dentro del MB que contiene el bit asociado al nbloque
    unsigned int pos_bit = nbloque % 8;                                   // Bit del Byte anterior que contiene el bit asociado al nbloque
    unsigned int idx_byte = pos_byte % BLOCKSIZE;                         // Núm. de bloque dentro del MB donde se encuentra el Byte anterior
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE; // numBloqueAbs, Núm. de bloque absoluto

    DEBUG("pos_byte: %d", pos_byte);
    DEBUG("pos_bit: %d", pos_bit);
    DEBUG("idx_byte: %d", idx_byte);
    DEBUG("idx_block: %d", idx_block);

    // Lee el bloque que contiene el bit que nos interesa a partir de la posición absoluta calculada
    if (bread(idx_block, block_buff) == FALLO) return FALLO;
    unsigned char mask = 1 << (7 - pos_bit);
    DEBUG("value: %2$s (0x%1$02x, %1$hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));
    DEBUG("MASK:  %2$s (0x%1$02x, %1$hhu)", mask, BIN_STR8(mask));
    DEBUG("bit value = %d", ((block_buff[idx_byte] & mask)>>(7 - pos_bit)));
    return ((block_buff[idx_byte] & mask)>>(7 - pos_bit)); // Devuelve el valor del bit solicitado
}

/**
 * Reserva el primer bloque libre y lo resetea a 0.
 *
 * @return número de bloque reservado, FALLO en caso de error
 */
int reservar_bloque() {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque del dispositivo virtual
    if (sb.cantBloquesLibres == 0) return FALLO; // Comprueba que haya bloques libres para poder realizar la reserva

    unsigned char bufferAux[BLOCKSIZE] = {}; // Declaramos un buffer auxiliar,
    memset(bufferAux, 0xff, BLOCKSIZE);      // posteriormente lo inicializamos con todos sus bits a 1
    unsigned int block_cnt_mb = sb.posUltimoBloqueMB - sb.posPrimerBloqueMB; // Tamaño en bloques del Mapa de Bits, restringe el bucle for
    unsigned int nblock_mb = 0;
    DEBUG("sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG("sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG("block_cnt_mb: %u", block_cnt_mb);
    for (; nblock_mb < block_cnt_mb; nblock_mb++) {
        if (bread(sb.posPrimerBloqueMB + nblock_mb, block_buff) == FALLO) return FALLO; // Lee el bloque actual
        if (memcmp(block_buff, bufferAux, BLOCKSIZE)) break; // Se compara el bloque actual con el buffer auxiliar cuyo contenido son todo 1's,
    }                                                        // sale del bucle si se encuentra algún bit a 0 en el bloque actual

    DEBUG("nblock_mb: %u", nblock_mb);

    // Obtiene la posición del primer byte del bloque que tiene algún bit a 0
    unsigned int nbyte = 0;
    while (nbyte < BLOCKSIZE && block_buff[nbyte] == 0xff) nbyte++;
    DEBUG("nbyte: %u", nbyte);

    // Obtiene la posición del primer bit que está a 0 dentro del byte seleccionado anteriormente
    unsigned char nbit = 0;
    unsigned char val = block_buff[nbyte]; 
    // unsigned char val = 0xf0;
    DEBUG("[%1$d]val: %2$s (0x%3$02x, %3$hhu, initial byte value)", nbit,  BIN_STR8(val), val);

    while (val & 0x80) { // Comprueba el valor del MSB del Byte seleccionado
        val <<= 1; // Si el MSB no es 0, desplaza una posición hacia la izquierda
        nbit++;
        DEBUG("[%1$d]val: %2$s (0x%3$02x, %3$hhu)", nbit,  BIN_STR8(val), val);
    }
    DEBUG("[nbit]: %u  (núm. desplazamientos)", nbit);

    // Modifica la zona de metadatos para que el bloque quede reservado
    unsigned int nblock = (nblock_mb * BLOCKSIZE + nbyte) * 8 + nbit;
    DEBUG("nblock: %u", nblock);
    if (escribir_bit(nblock, 1) == FALLO) return FALLO; // Pone como ocupado el bit del MB asociado al bloque reservado
    sb.cantBloquesLibres--;
    DEBUG("sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    if (bwrite(posSB, &sb) == FALLO) return FALLO; // Escribe los cambios en el dispositivo virtual

    // Limpia la zona de datos correspondiente al bloque reservado
    memset(bufferAux, 0, BLOCKSIZE);
    if (bwrite(nblock, bufferAux) == FALLO) return FALLO;

    return nblock;
}

/**
 * Libera un bloque específico indicando que está libre en el MB.
 *
 * @param nbloque número de bloque a liberar
 * @return número de bloque liberado, FALLO en caso de error
 */
int liberar_bloque(unsigned int nbloque) {
    if (bread(posSB, &sb) == FALLO) return FALLO; 
    if (escribir_bit(nbloque, 0) == FALLO) return FALLO;  
    sb.cantBloquesLibres++;
    DEBUG("sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    if (bwrite(posSB, &sb) == FALLO) return FALLO;
    return nbloque;
}

/**
 * Escribe sobre un inodo específico del array de inodos.
 *
 * @param ninodo índice del inodo dentro del array de inodos
 * @param inodo puntero al inodo a escribir
 * @return EXITO si se ha escrito el inodo correctamente, FALLO en caso contrario
 */
int escribir_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // Número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // Índice del inodo dentro del bloque

    DEBUG("nblock: %u", nblock);
    DEBUG("inodo_idx: %u", inodo_idx);
    DEBUG("inodo block%u[%u]", nblock, inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO;  // Lee el bloque en donde se encuentra el array con el inodo a sobreescribir
    inodos[inodo_idx] = *inodo;                        // Sobreescribe el inodo del array con el inodo pasado por parámetro
    if (bwrite(nblock, inodos) == FALLO) return FALLO; // Vuelve a escribir el bloque en el dispositivo virtual

    return EXITO;
}

/**
 * Lee un inodo específico del array de inodos.
 *
 * @param ninodo índice del inodo dentro del array de inodos
 * @param inodo puntero al inodo donde se van a leer los datos
 * @return EXITO si se ha leído el inodo correctamente, FALLO en caso contrario
 */
int leer_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // Número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // Índice del inodo dentro del bloque

    DEBUG("nblock: %u", nblock);
    DEBUG("inodo_idx: %u", inodo_idx);
    DEBUG("inodo block%u[%u]", nblock, inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO; // Lee el bloque en donde se encuentra el inodo que queremos leer
    *inodo = inodos[inodo_idx]; // Pone en la dirección de memoria pasada por parámetro el struct del inodo leído

    return EXITO;
}

/**
 * Reserva un inodo que esté libre, si lo hay
 *
 * @param tipo tipo del inodo a reservar
 * @param permisos permisos del inodo a reservar
 * @return posición del inodo reservado en el array de inodos, FALLO si no hay inodos libres o si ha habido un error
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque del dispositivo virtual

    if (sb.cantInodosLibres == 0) { // Comprueba que haya inodos libres 
        ERROR("no hay inodos libres");
        return FALLO;
    }
    DEBUG("(PRE) sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG("(PRE) sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);

    inodo_t inodo = {};
    unsigned int inodo_pos = sb.posPrimerInodoLibre;
    if (leer_inodo(inodo_pos, &inodo) == FALLO) return FALLO; // Lee el inodo libre a partir de la posición indicada por el superbloque
    inodo.tipo = tipo;
    inodo.permisos = permisos;
    inodo.nlinks = 1;
    inodo.tamEnBytesLog = 0;
    time_t t = time(NULL);
    inodo.atime = t;
    inodo.mtime = t;
    inodo.ctime = t;
    inodo.btime = t;
    inodo.numBloquesOcupados = 0;

    // Actualiza el superbloque antes de resetear los punteros del inodo
    sb.posPrimerInodoLibre = inodo.punterosDirectos[0]; // Actualiza la lista enlazada de inodos libres
    sb.cantInodosLibres--;                              // y decrementa el contador de inodos libres
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    // Resetea los arrays de punteros del inodo reservado y los escribe en el dispositivo virtual
    memset(inodo.punterosDirectos, 0, 12*sizeof(unsigned int));
    memset(inodo.punterosIndirectos, 0, 3*sizeof(unsigned int));
    if (escribir_inodo(inodo_pos, &inodo) == FALLO) return FALLO; 

    DEBUG("(POST) sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG("(POST) sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);

    return inodo_pos;
}