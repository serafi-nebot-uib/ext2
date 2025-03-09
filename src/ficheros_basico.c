/**************************************************************************
* FILENAME: ficheros_basico.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "ficheros_basico.h"
#include "bloques.h"

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
    return bwrite(posSB, &sb);
}

/**
 * Inicializa el mapa de bits del sistema de ficheros
 * 
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
 */
int initMB() {
    if (bread(posSB, &sb) == FALLO) return FALLO; // lee el superbloque almacenado en el dispositivo virtual

    DEBUG("sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG("sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG("sb.posPrimerBloqueAI: %u", sb.posPrimerBloqueAI);
    DEBUG("sb.posUltimoBloqueAI: %u", sb.posUltimoBloqueAI);
    DEBUG("sb.posPrimerBloqueDatos: %u", sb.posPrimerBloqueDatos);
    DEBUG("sb.posUltimoBloqueDatos: %u", sb.posUltimoBloqueDatos);
    DEBUG("sb.posInodoRaiz: %u", sb.posInodoRaiz);
    DEBUG("sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);
    DEBUG("sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    DEBUG("sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG("sb.totBloques: %u", sb.totBloques);
    DEBUG("sb.totInodos: %u", sb.totInodos);

    int mb_bit_cnt = tamSB + tamMB(sb.totBloques) + tamAI(sb.totInodos); // cantidad inicial de bits ocupados del Mapa de Bits
    int mb_byte_cnt = mb_bit_cnt / 8;                                    // cantidad inicial de bytes ocupados del Mapa de Bits
    int mb_block_cnt = mb_byte_cnt / BLOCKSIZE;                          // cantidad inicial de bloques ocupados del Mapa de Bits
    int mb_extra_byte_off = mb_byte_cnt % BLOCKSIZE;                     // cantidad adicional de bytes ocupados
    int mb_extra_bit_cnt = mb_bit_cnt % 8;                               // cantidad adicional de bits ocupados

    DEBUG("mb_bit_cnt: %d", mb_bit_cnt);
    DEBUG("mb_byte_cnt: %d", mb_byte_cnt);
    DEBUG("mb_block_cnt: %d", mb_block_cnt);
    DEBUG("mb_extra_byte_off: %d", mb_extra_byte_off);
    DEBUG("mb_extra_bit_cnt: %d", mb_extra_bit_cnt);

    memset(block_buff, 0xff, BLOCKSIZE);
    for (int i = 0; i < mb_block_cnt; i++)
        if (bwrite(sb.posPrimerBloqueMB + i, block_buff) == FALLO) return FALLO;

    block_buff[mb_extra_byte_off] = ~((1 << (8 - mb_extra_bit_cnt)) - 1); 
    DEBUG("block_buff[%d]: %hhu", mb_extra_byte_off, block_buff[mb_extra_byte_off]);
    for (int i = mb_extra_byte_off+1; i < BLOCKSIZE; i++) block_buff[i] = 0; // los bytes restantes del bloque se ponen a 0
    if (bwrite(sb.posPrimerBloqueMB + mb_block_cnt, block_buff) == FALLO) return FALLO;

    sb.cantBloquesLibres -= mb_bit_cnt; // mb_bit_cnt = cantidad de bloques que ocupan los metadatos
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    return EXITO;
}

/**
 * Inicializa el array de inodos libres del sistema de ficheros
 * 
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
 */
int initAI() {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    inodo_t inodos[BLOCKSIZE / INODOSIZE];
    unsigned int inode_next = sb.posPrimerInodoLibre + 1; 
    for (int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) { // itera sobre todos los bloques que contienen inodos
        if (bread(i, inodos) == FALLO)  return FALLO; // lee un bloque y almacena los inodos que este contiene en un array
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l'; // pone cada inodo del bloque actual como libre
            if (inode_next >= sb.totInodos) {
                inodos[j].punterosDirectos[0] = UINT_MAX; 
                break;
            }
            inodos[j].punterosDirectos[0] = inode_next++; // al estar todos libres, cada inodo apunta al siguiente
        }
        if (bwrite(i, inodos) == FALLO) return FALLO; // vuelve a escribir el bloque de inodos en el dispositivo virtual
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
    if (bread(posSB, &sb) == FALLO) return FALLO; // lee el superbloque almacenado en el dispositivo virtual

    unsigned int pos_byte = nbloque / 8;                                  // byte dentro del MB que contiene el bit asociado al nbloque
    unsigned int pos_bit = nbloque % 8;                                   // bit del byte anterior que contiene el bit asociado al nbloque
    unsigned int idx_byte = pos_byte % BLOCKSIZE; // numBloqueMB          // núm. de bloque dentro del MB donde se encuentra el Byte anterior
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE; // numbloqueabs, Núm. de bloque absoluto

    DEBUG("input params: %u [nbloque], %u [bit (bitValue)]", nbloque, bit);
    DEBUG("pos_byte: %d", pos_byte);
    DEBUG("pos_bit: %d", pos_bit);
    DEBUG("idx_byte: %d", idx_byte);
    DEBUG("idx_block: %d", idx_block);

    // lee el bloque donde se encuentra el bit asociado al nbloque, usando la posición absoluta
    if (bread(idx_block, block_buff) == FALLO) return FALLO;

    unsigned char mask = 1 << (7 - pos_bit);
    DEBUG("mask value: %2$s (0x%1$02x, %1$3hhu)", mask, BIN_STR8(mask));
    DEBUG("prev value: %2$s (0x%1$02x, %1$3hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));
    if (bit) block_buff[idx_byte] |= mask; // si el valor pasado por parámetro es 1, hace una OR entre el byte del MB y la máscara
    else block_buff[idx_byte] &= ~mask;    // si es 0, hace una AND entre el byte del MB y la máscara negada
    DEBUG(" new value: %2$s (0x%1$02x, %1$3hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));

    if (bwrite(idx_block, block_buff) == FALLO) return FALLO; // escribe el bloque del MB modificado en el dispositivo virtual

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

    unsigned int pos_byte = nbloque / 8;                                  // byte dentro del MB que contiene el bit asociado al nbloque
    unsigned int pos_bit = nbloque % 8;                                   // bit del Byte anterior que contiene el bit asociado al nbloque
    unsigned int idx_byte = pos_byte % BLOCKSIZE;                         // núm. de bloque dentro del MB donde se encuentra el Byte anterior
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE; // numBloqueAbs, Núm. de bloque absoluto

    DEBUG("pos_byte: %u", pos_byte);
    DEBUG("pos_bit: %u", pos_bit);
    DEBUG("idx_byte: %u", idx_byte);
    DEBUG("idx_block: %u", idx_block);

    // lee el bloque que contiene el bit que nos interesa a partir de la posición absoluta calculada
    if (bread(idx_block, block_buff) == FALLO) return FALLO;
    unsigned char mask = 1 << (7 - pos_bit);
    DEBUG(" pre value: %2$s (0x%1$02x, %1$3hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));
    DEBUG("      mask: %2$s (0x%1$02x, %1$3hhu)", mask, BIN_STR8(mask));
    int value = ((block_buff[idx_byte] & mask) >> (7 - pos_bit));
    DEBUG("post value: %2$s (0x%1$02x, %1$3hhu)", value, BIN_STR8(value));
    return value;
}

/**
 * Reserva el primer bloque libre y lo resetea a 0.
 *
 * @return número de bloque reservado, FALLO en caso de error
 */
int reservar_bloque() {
    if (bread(posSB, &sb) == FALLO) return FALLO; // lee el superbloque del dispositivo virtual
    if (sb.cantBloquesLibres == 0) return FALLO; // comprueba que haya bloques libres para poder realizar la reserva

    unsigned char bufferAux[BLOCKSIZE] = {}; // declaramos un buffer auxiliar,
    memset(bufferAux, 0xff, BLOCKSIZE);      // posteriormente lo inicializamos con todos sus bits a 1
    unsigned int block_cnt_mb = sb.posUltimoBloqueMB - sb.posPrimerBloqueMB; // Tamaño en bloques del Mapa de Bits, restringe el bucle for
    unsigned int nblock_mb = 0;
    DEBUG("sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG("sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG("block_cnt_mb: %u", block_cnt_mb);
    for (; nblock_mb < block_cnt_mb; nblock_mb++) {
        if (bread(sb.posPrimerBloqueMB + nblock_mb, block_buff) == FALLO) return FALLO; // lee el bloque actual
        if (memcmp(block_buff, bufferAux, BLOCKSIZE)) break; // se compara el bloque actual con el buffer auxiliar cuyo contenido son todo 1's,
    }                                                        // sale del bucle si se encuentra algún bit a 0 en el bloque actual

    DEBUG("nblock_mb: %u", nblock_mb);

    // obtiene la posición del primer byte del bloque que tiene algún bit a 0
    unsigned int nbyte = 0;
    while (nbyte < BLOCKSIZE && block_buff[nbyte] == 0xff) nbyte++;
    DEBUG("nbyte: %u", nbyte);

    // obtiene la posición del primer bit que está a 0 dentro del byte seleccionado anteriormente
    unsigned char nbit = 0;
    unsigned char val = block_buff[nbyte]; 
    DEBUG("[%1$d]val: %2$s (0x%3$02x, %3$3hhu, initial byte value)", nbit, BIN_STR8(val), val);

    while (val & 0x80) { // comprueba el valor del MSB del Byte seleccionado
        val <<= 1; // si el MSB no es 0, desplaza una posición hacia la izquierda
        nbit++;
        DEBUG("[%1$d]val: %2$s (0x%3$02x, %3$3hhu)", nbit,  BIN_STR8(val), val);
    }
    DEBUG("[nbit]: %u  (núm. desplazamientos)", nbit);

    // modifica la zona de metadatos para que el bloque quede reservado
    unsigned int nblock = (nblock_mb * BLOCKSIZE + nbyte) * 8 + nbit;
    DEBUG("nblock: %u", nblock);
    if (escribir_bit(nblock, 1) == FALLO) return FALLO; // pone como ocupado el bit del MB asociado al bloque reservado
    sb.cantBloquesLibres--;
    DEBUG("sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    if (bwrite(posSB, &sb) == FALLO) return FALLO; // escribe los cambios en el dispositivo virtual

    // limpia la zona de datos correspondiente al bloque reservado
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

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // índice del inodo dentro del bloque

    DEBUG("nblock: %u", nblock);
    DEBUG("inodo_idx: %u", inodo_idx);
    DEBUG("inodo block%u[%u]", nblock, inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO;  // lee el bloque en donde se encuentra el array con el inodo a sobreescribir
    inodos[inodo_idx] = *inodo;                        // sobreescribe el inodo del array con el inodo pasado por parámetro
    if (bwrite(nblock, inodos) == FALLO) return FALLO; // vuelve a escribir el bloque en el dispositivo virtual

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

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // índice del inodo dentro del bloque

    DEBUG("nblock: %u", nblock);
    DEBUG("inodo_idx: %u", inodo_idx);
    DEBUG("inodo block%u[%u]", nblock, inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO; // lee el bloque en donde se encuentra el inodo que queremos leer
    *inodo = inodos[inodo_idx]; // pone en la dirección de memoria pasada por parámetro el struct del inodo leído

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
    if (bread(posSB, &sb) == FALLO) return FALLO;

    if (sb.cantInodosLibres == 0) {
        ERROR("no hay inodos libres");
        return FALLO;
    }

    DEBUG("[pre] sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG("[pre] sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);

    inodo_t inodo = {};
    unsigned int inodo_pos = sb.posPrimerInodoLibre;
    if (leer_inodo(inodo_pos, &inodo) == FALLO) return FALLO; // lee el inodo libre a partir de la posición indicada por el superbloque
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

    // actualiza el superbloque antes de resetear los punteros del inodo
    sb.posPrimerInodoLibre = inodo.punterosDirectos[0]; // actualiza la lista enlazada de inodos libres
    sb.cantInodosLibres--;                              // y decrementa el contador de inodos libres
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    // resetea los arrays de punteros del inodo reservado y los escribe en el dispositivo virtual
    memset(inodo.punterosDirectos, 0, 12*sizeof(unsigned int));
    memset(inodo.punterosIndirectos, 0, 3*sizeof(unsigned int));
    if (escribir_inodo(inodo_pos, &inodo) == FALLO) return FALLO; 

    DEBUG("[post] sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG("[post] sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);

    return inodo_pos;
}

/**
 * Obtener el rango de punteros en el que se situa el bloque lógico nblogico
 *
 * @param inodo inodo en el que buscar los punteros
 * @param nblogico numero de bloque lógico del cual obtener el puntero
 * @param ptr puntero a la variable que se va a actualizar con el valor del puntero correspondiente
 * @return 0 si nblogico esta en los punteros directos, 1 si esta en punteros indirectos 0, 2 si esta en punteros indirectos 1, 3 si esta dentro de punteros indirectos 2 y -1 si esta fuera de rango
 */
int obtener_nRangoBL(inodo_t *inodo, unsigned int nblogico, unsigned int **ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos;
        return 0;
    } else if (nblogico < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos;
        return 1;
    } else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos;
        return 2;
    } else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos;
        return 3;
    }
    *ptr = 0;
    ERROR("bloque logico %d fuera de rango", nblogico);
    return -1;
}

/**
 * Obtener índice dentro del array punteros correspondiente al bloque lógico y nivel de puntero indicado
 *
 * @param nblogico número de bloque lógico
 * @param nuvel_punteros nivel del array de punteros
 * @return índice dentro del array de punteros correspondiente al bloque lógico y nivel de puntero indicado
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    if (nblogico < DIRECTOS) return nblogico;
    if (nblogico < INDIRECTOS0) return nblogico - DIRECTOS;
    if (nblogico < INDIRECTOS1) {
        if (nivel_punteros == 2) return (nblogico - INDIRECTOS0) / NPUNTEROS;
        if (nivel_punteros == 1) return (nblogico - INDIRECTOS0) % NPUNTEROS;
        ERROR("nivel puntero %u invalido para numero de bloque lógico %u", nivel_punteros, nblogico);
        return FALLO;
    }
    if (nblogico < INDIRECTOS2) {
        if (nivel_punteros == 3) return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        if (nivel_punteros == 2) return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        if (nivel_punteros == 1) return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        ERROR("nivel puntero %u invalido para numero de bloque lógico %u", nivel_punteros, nblogico);
        return FALLO;
    }
    ERROR("bloque logico %u fuera de rango", nblogico);
    return FALLO;
}

int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    inodo_t inode = {};
    if (leer_inodo(ninodo, &inode) == FALLO) return FALLO;

    unsigned int *ptr = 0;
    int depth = obtener_nRangoBL(&inode, nblogico, &ptr);
    if (depth < 0) return FALLO;
    int idx = depth == 0 ? obtener_indice(nblogico, depth) : depth-1;
    if (idx == FALLO) return FALLO;
    unsigned int nblock = ptr[idx];

    if (nblock == 0) {
        if (!reservar) return FALLO;
        ptr[idx] = reservar_bloque();
        nblock = ptr[idx];
        inode.numBloquesOcupados++;
        inode.ctime = time(NULL);
        printf("reserved block: %u\n", ptr[idx]);
        if (escribir_inodo(ninodo, &inode) == FALLO) return FALLO;
    }

    unsigned int buff[NPUNTEROS] = {};
    for (unsigned int lvl = depth; lvl > 0; lvl--) {
        if (bread(nblock, buff) == FALLO) return FALLO;
        idx = obtener_indice(nblogico, lvl);
        if (buff[idx] == 0) {
            if (!reservar) return FALLO;
            buff[idx] = reservar_bloque();
            printf("reserved block: %u\n", buff[idx]);
            if (bwrite(nblock, buff) == FALLO) return FALLO;
            inode.numBloquesOcupados++;
            inode.ctime = time(NULL);
            if (escribir_inodo(ninodo, &inode) == FALLO) return FALLO;
        }
        nblock = buff[idx];
    }

    return nblock;
}
