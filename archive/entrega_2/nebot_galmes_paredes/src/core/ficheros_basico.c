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

    DEBUG(3, "sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG(3, "sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG(3, "sb.posPrimerBloqueAI: %u", sb.posPrimerBloqueAI);
    DEBUG(3, "sb.posUltimoBloqueAI: %u", sb.posUltimoBloqueAI);
    DEBUG(3, "sb.posPrimerBloqueDatos: %u", sb.posPrimerBloqueDatos);
    DEBUG(3, "sb.posUltimoBloqueDatos: %u", sb.posUltimoBloqueDatos);
    DEBUG(3, "sb.posInodoRaiz: %u", sb.posInodoRaiz);
    DEBUG(3, "sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);
    DEBUG(3, "sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    DEBUG(3, "sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG(3, "sb.totBloques: %u", sb.totBloques);
    DEBUG(3, "sb.totInodos: %u", sb.totInodos);

    int mb_bit_cnt = tamSB + tamMB(sb.totBloques) + tamAI(sb.totInodos); // cantidad inicial de bits ocupados del Mapa de Bits
    int mb_byte_cnt = mb_bit_cnt / 8;                                    // cantidad inicial de bytes ocupados del Mapa de Bits
    int mb_block_cnt = mb_byte_cnt / BLOCKSIZE;                          // cantidad inicial de bloques ocupados del Mapa de Bits
    int mb_extra_byte_off = mb_byte_cnt % BLOCKSIZE;                     // cantidad adicional de bytes ocupados
    int mb_extra_bit_cnt = mb_bit_cnt % 8;                               // cantidad adicional de bits ocupados

    DEBUG(3, "mb_bit_cnt: %d", mb_bit_cnt);
    DEBUG(3, "mb_byte_cnt: %d", mb_byte_cnt);
    DEBUG(3, "mb_block_cnt: %d", mb_block_cnt);
    DEBUG(3, "mb_extra_byte_off: %d", mb_extra_byte_off);
    DEBUG(3, "mb_extra_bit_cnt: %d", mb_extra_bit_cnt);

    memset(block_buff, 0xff, BLOCKSIZE);
    for (int i = 0; i < mb_block_cnt; i++)
        if (bwrite(sb.posPrimerBloqueMB + i, block_buff) == FALLO) return FALLO;

    block_buff[mb_extra_byte_off] = ~((1 << (8 - mb_extra_bit_cnt)) - 1);
    DEBUG(3, "block_buff[%d]: %hhu", mb_extra_byte_off, block_buff[mb_extra_byte_off]);
    for (int i = mb_extra_byte_off + 1; i < BLOCKSIZE; i++) block_buff[i] = 0; // los bytes restantes del bloque se ponen a 0
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
        if (bread(i, inodos) == FALLO)
            return FALLO; // lee un bloque y almacena los inodos que este contiene en
                          // un array
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
 * Escribe el valor del parámetro bit al bit del mapa de bits correspondiente al
 * número de bloque indicado por el parámetro nbloque. Se escribe 0 si bit = 0,
 * se escribe 1 si bit != 0.
 *
 * @param nbloque número de bloque que modificar en el mapa de bits
 * @param bit nuevo valor del bit a modificar
 * @return EXITO si se escribe el valor correctamente, FALLO en caso contrario
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    if (bread(posSB, &sb) == FALLO) return FALLO; // lee el superbloque almacenado en el dispositivo virtual

    unsigned int pos_byte = nbloque / 8;                                  // byte dentro del MB que contiene el bit asociado al nbloque
    unsigned int pos_bit = nbloque % 8;                                   // bit del byte anterior que contiene el bit asociado al nbloque
    unsigned int idx_byte = pos_byte % BLOCKSIZE;                         // numBloqueMB          // núm. de bloque dentro del
                                                                          // MB donde se encuentra el Byte anterior
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE; // numbloqueabs, Núm. de bloque absoluto

    DEBUG(3, "input params: %u [nbloque], %u [bit (bitValue)]", nbloque, bit);
    DEBUG(3, "pos_byte: %d", pos_byte);
    DEBUG(3, "pos_bit: %d", pos_bit);
    DEBUG(3, "idx_byte: %d", idx_byte);
    DEBUG(3, "idx_block: %d", idx_block);

    // lee el bloque donde se encuentra el bit asociado al nbloque, usando la
    // posición absoluta
    if (bread(idx_block, block_buff) == FALLO) return FALLO;

    unsigned char mask = 1 << (7 - pos_bit);
    DEBUG(3, "mask value: %2$s (0x%1$02x, %1$3hhu)", mask, BIN_STR8(mask));
    DEBUG(3, "prev value: %2$s (0x%1$02x, %1$3hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));
    if (bit) block_buff[idx_byte] |= mask; // si el valor pasado por parámetro es 1, hace una OR entre el byte del MB y la máscara
    else block_buff[idx_byte] &= ~mask;    // si es 0, hace una AND entre el byte del MB y la máscara negada
    DEBUG(3, " new value: %2$s (0x%1$02x, %1$3hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));

    if (bwrite(idx_block, block_buff) == FALLO) return FALLO; // escribe el bloque del MB modificado en el dispositivo virtual

    return EXITO;
}

/**
 * Lee el valor del bit del mapa de bits correspondiente al número de bloque
 * indicado por el parámetro nbloque.
 *
 * @param nbloque número de bloque del cual leer el bit
 * @return valor del bit correspondiente a nbloque, FALLO si ha habido un error
 */
int leer_bit(unsigned int nbloque) {
    if (bread(posSB, &sb) == FALLO) return FALLO; // Lee el superbloque del dispositivo virtual

    unsigned int pos_byte = nbloque / 8;                                  // byte dentro del MB que contiene el bit asociado al nbloque
    unsigned int pos_bit = nbloque % 8;                                   // bit del Byte anterior que contiene el bit asociado al nbloque
    unsigned int idx_byte = pos_byte % BLOCKSIZE;                         // núm. de bloque dentro del MB donde se encuentra
                                                                          // el Byte anterior
    unsigned int idx_block = sb.posPrimerBloqueMB + pos_byte / BLOCKSIZE; // numBloqueAbs, Núm. de bloque absoluto

    DEBUG(3, "pos_byte: %u", pos_byte);
    DEBUG(3, "pos_bit: %u", pos_bit);
    DEBUG(3, "idx_byte: %u", idx_byte);
    DEBUG(3, "idx_block: %u", idx_block);

    // lee el bloque que contiene el bit que nos interesa a partir de la posición
    // absoluta calculada
    if (bread(idx_block, block_buff) == FALLO) return FALLO;
    unsigned char mask = 1 << (7 - pos_bit);
    DEBUG(3, " pre value: %2$s (0x%1$02x, %1$3hhu)", block_buff[idx_byte], BIN_STR8(block_buff[idx_byte]));
    DEBUG(3, "      mask: %2$s (0x%1$02x, %1$3hhu)", mask, BIN_STR8(mask));
    int value = ((block_buff[idx_byte] & mask) >> (7 - pos_bit));
    DEBUG(3, "post value: %2$s (0x%1$02x, %1$3hhu)", value, BIN_STR8(value));
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
    memset(bufferAux, 0xff, BLOCKSIZE);                                                       // posteriormente lo inicializamos con todos sus bits a 1
    unsigned int block_cnt_mb = sb.posUltimoBloqueMB - sb.posPrimerBloqueMB; // Tamaño en bloques del Mapa de Bits, restringe el bucle for
    unsigned int nblock_mb = 0;
    DEBUG(3, "sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG(3, "sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG(3, "block_cnt_mb: %u", block_cnt_mb);
    for (; nblock_mb < block_cnt_mb; nblock_mb++) {
        if (bread(sb.posPrimerBloqueMB + nblock_mb, block_buff) == FALLO) return FALLO; // lee el bloque actual
        if (memcmp(block_buff, bufferAux, BLOCKSIZE)) break; // se compara el bloque actual con el buffer auxiliar cuyo contenido son todo 1's,
    } // sale del bucle si se encuentra algún bit a 0 en el bloque actual

    DEBUG(3, "nblock_mb: %u", nblock_mb);

    // obtiene la posición del primer byte del bloque que tiene algún bit a 0
    unsigned int nbyte = 0;
    while (nbyte < BLOCKSIZE && block_buff[nbyte] == 0xff) nbyte++;
    DEBUG(3, "nbyte: %u", nbyte);

    // obtiene la posición del primer bit que está a 0 dentro del byte
    // seleccionado anteriormente
    unsigned char nbit = 0;
    unsigned char val = block_buff[nbyte];
    DEBUG(3, "[%1$d]val: %2$s (0x%3$02x, %3$3hhu, initial byte value)", nbit, BIN_STR8(val), val);

    while (val & 0x80) { // comprueba el valor del MSB del Byte seleccionado
        val <<= 1;       // si el MSB no es 0, desplaza una posición hacia la izquierda
        nbit++;
        DEBUG(3, "[%1$d]val: %2$s (0x%3$02x, %3$3hhu)", nbit, BIN_STR8(val), val);
    }
    DEBUG(3, "[nbit]: %u  (núm. desplazamientos)", nbit);

    // modifica la zona de metadatos para que el bloque quede reservado
    unsigned int nblock = (nblock_mb * BLOCKSIZE + nbyte) * 8 + nbit;
    DEBUG(3, "nblock: %u", nblock);
    if (escribir_bit(nblock, 1) == FALLO) return FALLO; // pone como ocupado el bit del MB asociado al bloque reservado
    sb.cantBloquesLibres--;
    DEBUG(3, "sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
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
    DEBUG(3, "sb.cantBloquesLibres: %u", sb.cantBloquesLibres);
    if (bwrite(posSB, &sb) == FALLO) return FALLO;
    return nbloque;
}

/**
 * Escribe sobre un inodo específico del array de inodos.
 *
 * @param ninodo índice del inodo dentro del array de inodos
 * @param inodo puntero al inodo a escribir
 * @return EXITO si se ha escrito el inodo correctamente, FALLO en caso
 * contrario
 */
int escribir_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK;                     // índice del inodo dentro del bloque

    DEBUG(3, "nblock: %u", nblock);
    DEBUG(3, "inodo_idx: %u", inodo_idx);
    DEBUG(3, "inodo block%u[%u]", nblock, inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO;                                  // lee el bloque en donde se encuentra el array con el inodo a sobreescribir
    inodos[inodo_idx] = *inodo;                        // sobreescribe el inodo del array con el inodo pasado por parámetro
    if (bwrite(nblock, inodos) == FALLO) return FALLO; // vuelve a escribir el bloque en el dispositivo virtual

    return EXITO;
}

/**
 * Lee un inodo específico del array de inodos.
 *
 * @param ninodo índice del inodo dentro del array de inodos
 * @param inodo puntero al inodo donde se van a escribir los datos
 * @return EXITO si se ha leído el inodo correctamente, FALLO en caso contrario
 */
int leer_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK;                     // índice del inodo dentro del bloque

    DEBUG(3, "nblock: %u", nblock);
    DEBUG(3, "inodo_idx: %u", inodo_idx);
    DEBUG(3, "inodo block%u[%u]", nblock, inodo_idx);

    if (bread(nblock, inodos) == FALLO) return FALLO;           // lee el bloque en donde se encuentra el inodo que queremos leer
    *inodo = inodos[inodo_idx]; // pone en la dirección de memoria pasada por parámetro el struct del inodo leído

    return EXITO;
}

/**
 * Reserva un inodo que esté libre, si lo hay
 *
 * @param tipo tipo del inodo a reservar
 * @param permisos permisos del inodo a reservar
 * @return posición del inodo reservado en el array de inodos, FALLO si no hay
 * inodos libres o si ha habido un error
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    if (sb.cantInodosLibres == 0) {
        ERROR("no hay inodos libres");
        return FALLO;
    }

    DEBUG(3, "[pre] sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG(3, "[pre] sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);

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

    // resetea los arrays de punteros del inodo reservado y los escribe en el
    // dispositivo virtual
    memset(inodo.punterosDirectos, 0, 12 * sizeof(unsigned int));
    memset(inodo.punterosIndirectos, 0, 3 * sizeof(unsigned int));
    if (escribir_inodo(inodo_pos, &inodo) == FALLO) return FALLO;

    DEBUG(3, "[post] sb.cantInodosLibres: %u", sb.cantInodosLibres);
    DEBUG(3, "[post] sb.posPrimerInodoLibre: %u", sb.posPrimerInodoLibre);

    return inodo_pos;
}

/**
 * Obtener el rango de punteros en el que se situa el bloque lógico nblogico
 *
 * @param inodo inodo en el que buscar los punteros
 * @param nblogico numero de bloque lógico del cual obtener el puntero
 * @param ptr puntero a la variable que se va a actualizar con el valor del
 * puntero correspondiente
 * @return 0 si nblogico esta en los punteros directos, 1 si esta en punteros
 * indirectos 0, 2 si esta en punteros indirectos 1, 3 si esta dentro de
 * punteros indirectos 2 y -1 si esta fuera de rango
 */
int obtener_nRangoBL(inodo_t *inodo, unsigned int nblogico, unsigned int *ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos[nblogico];
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
 * Obtener índice dentro del array punteros correspondiente al bloque lógico y
 * nivel de puntero indicado
 *
 * @param nblogico número de bloque lógico
 * @param nuvel_punteros nivel del array de punteros
 * @return índice dentro del array de punteros correspondiente al bloque lógico
 * y nivel de puntero indicado
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

/**
 *  Función que retorna la dirección física de un bloque dentro del disco a
 * partir de un número de bloque lógico y un número de inodo del array de
 * inodos, La función realiza la búsqueda del bloque lógico dentro del sistema
 * de arrays anidados y dispone de dos modos controlados por el valor reservar,
 *  pasado por parámetro, estos funcionan de la siguiente manera:
 *  Si reservar == 0, la función busca el bloque lógico en el array, si no lo
 * encuentra devuelve -1 Si reservar == 1, la función busca el bloque lógico en
 * el array, si no lo encuentra, realiza una reserva de un bloque en el disco
 *
 * @param ninodo número de inodo en el array de inodos
 * @param nblogico número de bloque lógico sobre el que queremos conocer su
 * dirección física dentro del disco (núm. bloque físico)
 * @param reservar flag que permite seleccionar el modo de operación
 * @return puntero a la dirección del bloque físico buscado si correcto, FALLO
 * (-1) si ha habido un error o no se ha encontrado el bloque (cuando reservar =
 * 0)
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    unsigned int ptr = 0, ptr_ant = 0, salvar_inodo = 0;
    int indice = 0;
    unsigned int buffer[NPUNTEROS];
    inodo_t inodo = {};

    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO; // Lee el en el array de inodos, el inodo cuyo número se ha pasado por parámetro (ninodo)

    // Devuelve el rango en donde se encuentra el num de bloque lógico indicado
    // (nblogico), en caso de ser directo el puntero apunta a la posición nblogico
    int nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);

    // nRangoBL=0 para bloques lógicos [0 , 11],                    Array de
    // punteros Directos nRangoBL=1 para bloques lógicos [12 , 267], indirectos[0]
    // nRangoBL=2 para bloques lógicos [268 , 65.803],              indirectos[1]
    // nRangoBL=3 para bloques lógicos [65.804 , 16.843.019],       indirectos[2]

    // el nivel_punteros +alto es el que cuelga directamente del inodo
    int nivel_punteros = nRangoBL;
    if (nivel_punteros < 0) return FALLO;

    while (nivel_punteros > 0) { // iterar para cada nivel de punteros indirectos // Si los
                                 // punteros son indirectos (0 = directos, 1-2-3 = indirectos)
        // indirectos[] es un array de 3 elementos [INDIRECTOS0, INDIRECTOS1,
        // INDIRECTOS2], cada uno es la dirección a un bloque de punteros (luego a
        // su vez, algunos de esos bloques apuntan a más bloques de punteros,
        // haciendo recursividad)

        if (ptr == 0) { // no cuelgan bloques de punteros, es decir
                        // - el puntero de indirectos[0,1 o 2] no está declarado, no apunta
                        // a ningún sitio
                        // - el array del nivel en el que esté, no apunta a ningún sitio,
                        //   es decir no permite seguir la ruta teórica que permitiría
                        //   llegar al bloque buscado

            // Si no se pretende reservar ningún bloque, finaliza la ejecución ya que no hay nada que devolver
            if (reservar == 0) return -1;

            // reserva el bloque de punteros, la función devuelve el puntero con el
            // que posteriormente se hará la inicialización de alguno de los 3
            // elementos del array de indirectos[0,1 o 2] que no haya sido declarado
            // previamente
            if ((ptr = reservar_bloque()) == FALLO) return FALLO;

            // Se actualiza el num de bloques ocupados por el inodo en el campo de
            // datos del disco y se pone la fecha actual como fecha de última
            // modificación del inodo
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL); // time_t t = time(NULL);
            salvar_inodo = 1;         // Es un flag para marcar que se han realizado cambios en el inodo
                                      // y que estos deben sobreescribirse en el disco

            if (nivel_punteros == nRangoBL) { // el bloque cuelga directamente del inodo, es decir,
                                              // estamos en la primera iteración
                // pone el bloque de punteros reservado en el array de indirectos
                // P.EJ.: según obtener_rango -> indirectos[0] tiene rango 1,
                // por tanto para escribir el ptr en indirectos[0] se debe poner
                // indirectos[nRangoBL-1]
                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
                DEBUG(2, "inodo.punterosIndirectos[%1$d] = %2$u (reservado BF %2$u para punteros_nivel%3$d)", nRangoBL - 1, ptr, nivel_punteros);
            } else { // el bloque cuelga de otro bloque de punteros
                buffer[indice] = ptr;
                if (bwrite(ptr_ant, buffer) == FALLO)
                    return FALLO; // salvamos en el dispositivo el buffer de punteros
                                  // modificado, es decir el array anterior, en el que se
                                  // ha añadido una dirección nueva al reservar un bloque
                DEBUG(2, "punteros_nivel%1$d [%2$d] = %3$u (reservado BF %3$u para punteros_nivel%4$d)", nivel_punteros + 1, indice, ptr, nivel_punteros);
            }
            memset(buffer, 0, BLOCKSIZE); // ponemos a 0 todos los punteros del buffer
        } else {
            if (bread(ptr, buffer) == FALLO) return FALLO; // leemos del dispositivo el bloque de punteros ya existente
        }

        // P.ej.: en Indirectos2, que hay 3 niveles de recursividad, la función
        // obtener_indice funciona de la siguiente manera: Si nivel_punteros == 3
        // obtener_indice devuelve indice pertinente (para ir hacia el nbloque
        // buscado) del array de punteros que cuelga del inodo (Nivel3) si
        // nivel_punteros == 2 obtener_indice devuelve el indice del array que
        // cuelga del anterior array si nivel_punteros == 1 obtener_indice devuelve
        // el indice del array donde finalmente se encuentran los punteros a los
        // datos buscados

        if ((indice = obtener_indice(nblogico, nivel_punteros)) == FALLO)
            return FALLO;     // Devuelve el índice dentro del array donde se encuentra el
                              // bloque buscado
        ptr_ant = ptr;        // Guarda la dirección del array donde se ha reservado el bloque en
                              // una de sus entradas, para despues actualizarlo en el disco
        ptr = buffer[indice]; // actualiza el puntero, copia la dirección que
                              // almacena el array de punteros actual en el indice
                              // concreto es decir se prepara para la siguiente
                              // iteración saber en que dirección leer, habiendo
                              // profundizado una capa más
        nivel_punteros--;     // Decrementa el nivel, es decir va más adentro,
                              // profundiza en la recursividad
    }

    if (ptr == 0) { // no existe bloque de datos
        if (reservar == 0) return -1;
        if ((ptr = reservar_bloque()) == FALLO) return FALLO; // de datos
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;

        if (nRangoBL == 0) {                        // si era un puntero Directo
            inodo.punterosDirectos[nblogico] = ptr; // asignamos la direción del bl. de datos en el inodo
            DEBUG(2, "inodo.punterosDirectos[%1$u] = %2$u (reservado BF %2$u para BL %1$u)]", nblogico, ptr);
        } else {
            buffer[indice] = ptr; // asignamos la dirección del bloque de datos en el buffer
            if (bwrite(ptr_ant, buffer) == FALLO) return FALLO; // salvamos en el dispositivo el buffer de punteros modificado
            DEBUG(2, "punteros_nivel%1$d [%2$d] = %3$u (reservado BF %3$u para BL %4$u)", nivel_punteros + 1, indice, ptr, nblogico);
        }
    }

    if (salvar_inodo && (escribir_inodo(ninodo, &inodo)) == FALLO) return FALLO;
    return ptr; // Devuelve la dirección del bloque de datos buscado
}

/**
 * Liberar bloques de datos de un inodo.
 *
 * @param primerBL primer bloque lógico a partir del cual liberar los bloques
 * @param inodo inodo del cual liberar los bloques
 * @return número de bloques liberados o FALLO en caso de error
 */
int liberar_bloques_inodo(unsigned int primerBL, inodo_t *inodo) {
    // si el inodo no tiene datos no hay nada que liberar
    if (inodo->tamEnBytesLog == 0) return 0;

    // calcular el último bloque lógico
    unsigned int ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) ultimoBL -= 1;
    DEBUG(2, "primer BL: %u, último BL: %u", primerBL, ultimoBL);

#if DEBUG_LVL > 0
    unsigned int read_cnt = 0, write_cnt = 0;
#endif
    unsigned int ptr = 0;                            // puntero actual
    unsigned int freed = 0;                          // cantidad de bloques lógicos liberados
    unsigned int ptrs[INODE_PTR_LVL_MAX][NPUNTEROS]; // array de bloques de punteros
    // array de punteros utilizado para comprobar si el bloque de punteros actual
    // está completamente vacío
    unsigned int ptrs_cmp[NPUNTEROS] = {0};

    for (unsigned int bl = primerBL; bl <= ultimoBL; bl++) {
        // obtener el rango para el bloque lógico actual
        int range = obtener_nRangoBL(inodo, bl, &ptr);
        if (range < 0) return FALLO;

        // obviar bloque si no está reservado
        if (ptr == 0) continue;

        if (range == 0) {                                                           // si el puntero al bloque se encuentra en el array de
                                                                                    // punteros directos
            if (liberar_bloque(inodo->punterosDirectos[bl]) == FALLO) return FALLO; // libera el bloque
            DEBUG(2, "liberado BF %u de datos para BL %u", inodo->punterosDirectos[bl], bl);
            inodo->punterosDirectos[bl] = 0;
            freed++;
        } else {
            int lvl = range;
            // punteros e índices para el nivel actual
            unsigned int lvl_ptrs[INODE_PTR_LVL_MAX], lvl_idxs[INODE_PTR_LVL_MAX];

            // recorre los bloques de punteros anidados hasta llegar al bloque de
            // datos
            while (ptr > 0 && lvl > 0) {
                // obtiene el índice del puntero al bloque lógico actual dentro del
                // array de punteros correspondiente
                int indice = obtener_indice(bl, lvl);

                // lee el bloque de punteros si es el primer bloque o la primera vez que
                // visitamos este bloque
                if (bl == primerBL || indice == 0) {
                    if (bread(ptr, ptrs[lvl - 1]) == FALLO) return FALLO;
#if DEBUG_LVL > 0
                    read_cnt++;
#endif
                }

                // a medida que se va bajando de nivel el puntero y su índice dentro del
                // bloque de punteros se guardan
                lvl_ptrs[lvl - 1] = ptr;
                lvl_idxs[lvl - 1] = indice;

                // toma el nuevo valor del puntero, ubicado dentro del array leído, en
                // el índice obtenido
                ptr = ptrs[lvl - 1][indice];
                lvl--; // Baja de nivel
            }

            // obviar bloque si no está reservado
            if (ptr == 0) continue;
            if (liberar_bloque(ptr) == FALLO) return FALLO;
            if (lvl > 0) DEBUG(2, "liberado BF %u de punteros nivel %d para BL %u", ptr, lvl, bl);
            else DEBUG(2, "liberado BF %u de datos para BL %u", ptr, bl);
            freed++; // incrementa el contador de bloques liberados

            // actualizar el puntero padre del bloque liberado
            for (lvl = 1; lvl <= range; lvl++) {
                int idx = lvl_idxs[lvl - 1];
                ptrs[lvl - 1][idx] = 0;

                // si el bloque de punteros no está todo a 0's aún quedan punteros sin
                // liberar y no hay que actualizar el puntero padre
                if (memcmp(ptrs[lvl - 1], ptrs_cmp, BLOCKSIZE) != 0) {
                    if (bwrite(lvl_ptrs[lvl - 1], ptrs[lvl - 1]) == FALLO) return FALLO;
#if DEBUG_LVL > 0
                    write_cnt++;
#endif
                    break;
                }

                if (liberar_bloque(lvl_ptrs[lvl - 1]) == FALLO) return FALLO; // liberar puntero padre
                if (lvl > 0) DEBUG(2, "liberado BF %u de punteros nivel %d para BL %u", lvl_ptrs[lvl - 1], lvl, bl);
                else DEBUG(2, "liberado BF %u de datos para BL %u", ptr, bl);
                freed++;

                if (lvl == range) {
                    // estamos en el nivel más alto, actualizamos el array de punteros
                    // indirectos del inodo directamente
                    inodo->punterosIndirectos[range - 1] = 0;
                } else {
                    // actualizar el puntero padre
                    ptrs[lvl][lvl_idxs[lvl]] = 0;
                    if (bwrite(lvl_ptrs[lvl], ptrs[lvl]) == FALLO) return FALLO;
#if DEBUG_LVL > 0
                    write_cnt++;
#endif
                }
            }
        }
    }

    DEBUG(2, "total bloques liberados: %u, total_breads: %u, total_bwrites: %u", freed, read_cnt, write_cnt);
    return freed;
}

/**
 * Liberar inodo
 *
 * @param ninodo número de inodo a liberar
 * @return número de inodo liberado o FALLO en caso de error
 */
int liberar_inodo(unsigned int ninodo) {
    inodo_t inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    int freed = liberar_bloques_inodo(0, &inodo); // libera el inodo empezando desde el primer bloque
    if (freed < 0) return FALLO;

    superbloque_t sb;
    if (bread(posSB, &sb) == FALLO) return FALLO; // lee el superbloque

    // actualizamos los campos del inodo liberado
    inodo.numBloquesOcupados -= freed;
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;
    inodo.punterosDirectos[0] = sb.posPrimerInodoLibre; // enlazamos el inodo actual con el que estaba al
                                                        // principio de la lista
    time(&inodo.ctime);

    // establecemos al inodo como el primer inodo libre en la lista e
    // incrementamos el contador de inodos libres
    sb.posPrimerInodoLibre = ninodo;
    sb.cantInodosLibres++;

    if (bwrite(posSB, &sb) == FALLO) return FALLO;
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO; // escribe el inodo liberado en el sb

    return ninodo;
}

/**
 * Truncar inodo a partir de un número de bytes.
 *
 * @param ninodo número de inodo que truncar
 * @param nbytes número de bytes que deben quedar en el inodo
 * @return número de bloques liberados o FALLO en caso de error
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    inodo_t inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    if (!INODE_P(inodo.permisos, INODE_P_WRITE)) return FALLO; // comprobar que el inodo tiene permisos de escritura
    if (nbytes > inodo.tamEnBytesLog)
        return 0; // si nbytes es mayor al número de bytes en el inodo ya podemos
                  // considerar el inodo como truncado

    unsigned int primerBL = nbytes / BLOCKSIZE;
    if (nbytes % BLOCKSIZE != 0) primerBL++;

    int freed = liberar_bloques_inodo(primerBL, &inodo); // liberamos desde primerBL hasta el final
    if (freed == FALLO) return FALLO;

    // actualizamos los datos del inodo y posteriormente los escribimos en el
    // array de inodos del sb
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= freed;
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return freed;
}
