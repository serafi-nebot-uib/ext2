#include "ficheros_basico.h"

static unsigned char block_buff[BLOCKSIZE] = {};
static superbloque_t sb = {};
static inodo_t inodos[INODOS_IN_BLOCK] = {};

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
    for (int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) {
        if (bread(i, inodos) == FALLO)  return FALLO;
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l';
            if (inode_next >= sb.totInodos) {
                inodos[j].punterosDirectos[0] = UINT_MAX;
                break;
            }
            inodos[j].punterosDirectos[0] = inode_next++;
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
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
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
 *
 * @return numero de bloque reservado, FALLO en caso de error
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
    DEBUG("sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    // limpiar el bloque reservado, en caso de que sea un bloque reutilizado
    memset(aux, 0, BLOCKSIZE);
    if (bwrite(nblock, aux) == FALLO) return FALLO;

    return nblock;
}

/**
 * Liberar bloque.
 *
 * @param nbloque numero de bloque a liberar
 * @return numero de bloque liberado, FALLO en caso de error
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
 * Escribir inodo en el array de inodos.
 *
 * @param ninodo índice del inodo dentro del array de inodos
 * @param inodo puntero al inodo a escribir
 * @return EXITO si se ha escrito el inodo correctamente, FALLO en caso contrario
 */
int escribir_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // numero de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // indice del inodo dentro del bloque

    DEBUG("nblock: %u", nblock);
    DEBUG("inodo_idx: %u", inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO;
    inodos[inodo_idx] = *inodo;
    if (bwrite(nblock, inodos) == FALLO) return FALLO;

    return EXITO;
}

/**
 * Leer inodo en el array de inodos.
 *
 * @param ninodo índice del inodo dentro del array de inodos
 * @param inodo puntero al inodo donde se van a leer los datos
 * @return EXITO si se ha leído el inodo correctamente, FALLO en caso contrario
 */
int leer_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // numero de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // indice del inodo dentro del bloque

    DEBUG("nblock: %u", nblock);
    DEBUG("inodo_idx: %u", inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO;
    *inodo = inodos[inodo_idx];

    return EXITO;
}

/**
 * Reservar inodo
 *
 * @param tipo tipo del inodo a reservar
 * @param permisos permisos del inodo a reservar
 * @return poición del inodo reservado en el array de inodos
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    if (sb.cantInodosLibres == 0) {
        ERROR("no hay inodos libres");
        return FALLO;
    }

    inodo_t inodo = {};
    unsigned int inodo_pos = sb.posPrimerInodoLibre;
    if (leer_inodo(inodo_pos, &inodo) == FALLO) return FALLO;
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

    // cambiar el superbloque antes de resetear los punteros del inodo
    sb.posPrimerInodoLibre = inodo.punterosDirectos[0];
    sb.cantInodosLibres--;
    if (bwrite(posSB, &sb) == FALLO) return FALLO;

    memset(inodo.punterosDirectos, 0, 12*sizeof(unsigned int));
    memset(inodo.punterosIndirectos, 0, 3*sizeof(unsigned int));
    if (escribir_inodo(inodo_pos, &inodo) == FALLO) return FALLO;

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
int obtener_nRangoBL(inodo_t *inodo, unsigned int nblogico, unsigned int *ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos[0];
        return 0;
    } else if (nblogico < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos[0];
        return 1;
    } else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];
        return 2;
    } else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];
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

    unsigned int ptr = 0, idx = 0;
    int lvl = obtener_nRangoBL(&inode, nblogico, &ptr);
    if (lvl < 0) return FALLO;

    while (lvl > 0) {
        if ((idx = obtener_indice(nblogico, lvl)) == FALLO) return FALLO;
    }

    return EXITO;
}
