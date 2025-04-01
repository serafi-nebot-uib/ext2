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

    DEBUG(3, "input params: %u [nbloque], %u [bit (bitValue)]", nbloque, bit);
    DEBUG(3, "pos_byte: %d", pos_byte);
    DEBUG(3, "pos_bit: %d", pos_bit);
    DEBUG(3, "idx_byte: %d", idx_byte);
    DEBUG(3, "idx_block: %d", idx_block);

    // lee el bloque donde se encuentra el bit asociado al nbloque, usando la posición absoluta
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

    DEBUG(3, "pos_byte: %u", pos_byte);
    DEBUG(3, "pos_bit: %u", pos_bit);
    DEBUG(3, "idx_byte: %u", idx_byte);
    DEBUG(3, "idx_block: %u", idx_block);

    // lee el bloque que contiene el bit que nos interesa a partir de la posición absoluta calculada
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
    memset(bufferAux, 0xff, BLOCKSIZE);      // posteriormente lo inicializamos con todos sus bits a 1
    unsigned int block_cnt_mb = sb.posUltimoBloqueMB - sb.posPrimerBloqueMB; // Tamaño en bloques del Mapa de Bits, restringe el bucle for
    unsigned int nblock_mb = 0;
    DEBUG(3, "sb.posPrimerBloqueMB: %u", sb.posPrimerBloqueMB);
    DEBUG(3, "sb.posUltimoBloqueMB: %u", sb.posUltimoBloqueMB);
    DEBUG(3, "block_cnt_mb: %u", block_cnt_mb);
    for (; nblock_mb < block_cnt_mb; nblock_mb++) {
        if (bread(sb.posPrimerBloqueMB + nblock_mb, block_buff) == FALLO) return FALLO; // lee el bloque actual
        if (memcmp(block_buff, bufferAux, BLOCKSIZE)) break; // se compara el bloque actual con el buffer auxiliar cuyo contenido son todo 1's,
    }                                                        // sale del bucle si se encuentra algún bit a 0 en el bloque actual

    DEBUG(3, "nblock_mb: %u", nblock_mb);

    // obtiene la posición del primer byte del bloque que tiene algún bit a 0
    unsigned int nbyte = 0;
    while (nbyte < BLOCKSIZE && block_buff[nbyte] == 0xff) nbyte++;
    DEBUG(3, "nbyte: %u", nbyte);

    // obtiene la posición del primer bit que está a 0 dentro del byte seleccionado anteriormente
    unsigned char nbit = 0;
    unsigned char val = block_buff[nbyte]; 
    DEBUG(3, "[%1$d]val: %2$s (0x%3$02x, %3$3hhu, initial byte value)", nbit, BIN_STR8(val), val);

    while (val & 0x80) { // comprueba el valor del MSB del Byte seleccionado
        val <<= 1; // si el MSB no es 0, desplaza una posición hacia la izquierda
        nbit++;
        DEBUG(3, "[%1$d]val: %2$s (0x%3$02x, %3$3hhu)", nbit,  BIN_STR8(val), val);
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
 * @return EXITO si se ha escrito el inodo correctamente, FALLO en caso contrario
 */
int escribir_inodo(unsigned int ninodo, inodo_t *inodo) {
    if (bread(posSB, &sb) == FALLO) return FALLO;

    unsigned int nblock = sb.posPrimerBloqueAI + ninodo / INODOS_IN_BLOCK; // número de bloque en el que se encuentra el inodo
    unsigned int inodo_idx = ninodo % INODOS_IN_BLOCK; // índice del inodo dentro del bloque

    DEBUG(3, "nblock: %u", nblock);
    DEBUG(3, "inodo_idx: %u", inodo_idx);
    DEBUG(3, "inodo block%u[%u]", nblock, inodo_idx);

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

    DEBUG(3, "nblock: %u", nblock);
    DEBUG(3, "inodo_idx: %u", inodo_idx);
    DEBUG(3, "inodo block%u[%u]", nblock, inodo_idx);

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

    // resetea los arrays de punteros del inodo reservado y los escribe en el dispositivo virtual
    memset(inodo.punterosDirectos, 0, 12*sizeof(unsigned int));
    memset(inodo.punterosIndirectos, 0, 3*sizeof(unsigned int));
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
 * @param ptr puntero a la variable que se va a actualizar con el valor del puntero correspondiente
 * @return 0 si nblogico esta en los punteros directos, 1 si esta en punteros indirectos 0, 2 si esta en punteros indirectos 1, 3 si esta dentro de punteros indirectos 2 y -1 si esta fuera de rango
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

/**
 *  Función que retorna la dirección física de un bloque dentro del disco a partir de un número de bloque lógico y un número de inodo del array de inodos,
 *  La función realiza la búsqueda del bloque lógico dentro del sistema de arrays anidados y dispone de dos modos controlados por el valor reservar, 
 *  pasado por parámetro, estos funcionan de la siguiente manera:
 *  Si reservar == 0, la función busca el bloque lógico en el array, si no lo encuentra devuelve -1
 *  Si reservar == 1, la función busca el bloque lógico en el array, si no lo encuentra, realiza una reserva de un bloque en el disco
 * 
 * @param ninodo número de inodo en el array de inodos
 * @param nblogico número de bloque lógico sobre el que queremos conocer su dirección física dentro del disco (núm. bloque físico)
 * @param reservar flag que permite seleccionar el modo de operación
 * @return puntero a la dirección del bloque físico buscado si correcto, FALLO (-1) si ha habido un error o no se ha encontrado el bloque (cuando reservar = 0)
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    unsigned int ptr = 0, ptr_ant = 0, salvar_inodo = 0;
    int indice = 0;
    unsigned int buffer[NPUNTEROS];
    inodo_t inodo = {};

    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO; // Lee el en el array de inodos, el inodo cuyo número se ha pasado por parámetro (ninodo)

    // Devuelve el rango en donde se encuentra el num de bloque lógico indicado (nblogico), en caso de ser directo el puntero apunta a la posición nblogico
    int nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr); 

    // nRangoBL=0 para bloques lógicos [0 , 11],                    Array de punteros Directos
    // nRangoBL=1 para bloques lógicos [12 , 267],                  indirectos[0]
    // nRangoBL=2 para bloques lógicos [268 , 65.803],              indirectos[1]
    // nRangoBL=3 para bloques lógicos [65.804 , 16.843.019],       indirectos[2]

    //el nivel_punteros +alto es el que cuelga directamente del inodo
    int nivel_punteros = nRangoBL;
    if (nivel_punteros < 0) return FALLO;

    while (nivel_punteros > 0) { // iterar para cada nivel de punteros indirectos // Si los punteros son indirectos (0 = directos, 1-2-3 = indirectos)
        // indirectos[] es un array de 3 elementos [INDIRECTOS0, INDIRECTOS1, INDIRECTOS2], 
        // cada uno es la dirección a un bloque de punteros 
        // (luego a su vez, algunos de esos bloques apuntan a más bloques de punteros, haciendo recursividad)

        if (ptr == 0) { // no cuelgan bloques de punteros, es decir 
                       // - el puntero de indirectos[0,1 o 2] no está declarado, no apunta a ningún sitio
                       // - el array del nivel en el que esté, no apunta a ningún sitio, 
                       //   es decir no permite seguir la ruta teórica que permitiría llegar al bloque buscado

            if (reservar == 0) return -1; // Si no se pretende reservar ningún bloque, finaliza la ejecución ya que no hay nada que devolver

            // reserva el bloque de punteros, la función devuelve el puntero con el que posteriormente 
            // se hará la inicialización de alguno de los 3 elementos del array de indirectos[0,1 o 2] 
            // que no haya sido declarado previamente
            if ((ptr = reservar_bloque()) == FALLO) return FALLO;

            // Se actualiza el num de bloques ocupados por el inodo en el campo de datos del disco
            // y se pone la fecha actual como fecha de última modificación del inodo
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL); //time_t t = time(NULL);
            salvar_inodo = 1; // Es un flag para marcar que se han realizado cambios en el inodo y que estos deben sobreescribirse en el disco

            if (nivel_punteros == nRangoBL) { // el bloque cuelga directamente del inodo, es decir, estamos en la primera iteración
                // pone el bloque de punteros reservado en el array de indirectos
                // P.EJ.: según obtener_rango -> indirectos[0] tiene rango 1,
                // por tanto para escribir el ptr en indirectos[0] se debe poner indirectos[nRangoBL-1]
                inodo.punterosIndirectos[nRangoBL-1] = ptr; 
                DEBUG(3, "inodo.punterosIndirectos[%1$d] = %2$u (reservado BF %2$u para punteros_nivel%3$d)", nRangoBL-1, ptr, nivel_punteros);
            } else { //el bloque cuelga de otro bloque de punteros
                buffer[indice] = ptr;
                if (bwrite(ptr_ant, buffer) == FALLO) return FALLO; // salvamos en el dispositivo el buffer de punteros modificado, 
                                                                    // es decir el array anterior, en el que se ha añadido
                                                                    // una dirección nueva al reservar un bloque  
                DEBUG(3, "punteros_nivel%1$d [%2$d] = %3$u (reservado BF %3$u para punteros_nivel%4$d)", nivel_punteros+1, indice, ptr, nivel_punteros);
            }
            memset(buffer, 0, BLOCKSIZE); // ponemos a 0 todos los punteros del buffer
        } else {
            if (bread(ptr, buffer) == FALLO) return FALLO; // leemos del dispositivo el bloque de punteros ya existente
        }

        // P.ej.: en Indirectos2, que hay 3 niveles de recursividad, la función obtener_indice funciona de la siguiente manera:
        // Si nivel_punteros == 3 obtener_indice devuelve indice pertinente (para ir hacia el nbloque buscado) del array de punteros que cuelga del inodo (Nivel3)
        // si nivel_punteros == 2 obtener_indice devuelve el indice del array que cuelga del anterior array
        // si nivel_punteros == 1 obtener_indice devuelve el indice del array donde finalmente se encuentran los punteros a los datos buscados

        if ((indice = obtener_indice(nblogico, nivel_punteros)) == FALLO) return FALLO; // Devuelve el índice dentro del array donde se encuentra el bloque buscado
        ptr_ant = ptr; // Guarda la dirección del array donde se ha reservado el bloque en una de sus entradas, para despues actualizarlo en el disco
        ptr = buffer[indice]; // actualiza el puntero, copia la dirección que almacena el array de punteros actual en el indice concreto
                              // es decir se prepara para la siguiente iteración saber en que dirección leer, habiendo profundizado una capa más
        nivel_punteros--; // Decrementa el nivel, es decir va más adentro, profundiza en la recursividad
    }

    if (ptr == 0) { //no existe bloque de datos
        if (reservar == 0) return -1;
        if ((ptr = reservar_bloque()) == FALLO) return FALLO; // de datos
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;

        if (nRangoBL == 0) { // si era un puntero Directo
            inodo.punterosDirectos[nblogico] = ptr; // asignamos la direción del bl. de datos en el inodo
            DEBUG(3, "inodo.punterosDirectos[%1$u] = %2$u (reservado BF %2$u para BL %1$u)]", nblogico, ptr);
        } else {
            buffer[indice] = ptr; // asignamos la dirección del bloque de datos en el buffer
            if (bwrite(ptr_ant, buffer) == FALLO) return FALLO; // salvamos en el dispositivo el buffer de punteros modificado 
            DEBUG(3, "punteros_nivel%1$d [%2$d] = %3$u (reservado BF %3$u para BL %4$u)", nivel_punteros+1, indice, ptr, nblogico);
        }
    }

    if (salvar_inodo && (escribir_inodo(ninodo, &inodo)) == FALLO) return FALLO;
    return ptr; // Devuelve la dirección del bloque de datos buscado
}

// int liberar_bloques_inodo(unsigned int primerBL, inodo_t *inodo) {
//     if (inodo == NULL) return FALLO;
//     if (inodo->tamEnBytesLog == 0) return 0;  // fichero vacío
//
//     unsigned int ultimoBL = 0;
//     unsigned int nBL = 0;
//     unsigned int ptr = 0;
//     int nRangoBL = 0;
//     unsigned int liberados = 0;
//
//     /* calcular el último bloque lógico ocupado */
//     if (inodo->tamEnBytesLog % BLOCKSIZE == 0) ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE - 1;
//     else ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
//
//     /* Para el manejo de bloques indirectos usaremos buffers auxiliares */
//     unsigned int bloques_punteros[3][NPUNTEROS];
//     unsigned int bufAux_punteros[NPUNTEROS];
//     unsigned int ptr_nivel[3];  // punteros a bloques de punteros de cada nivel
//     unsigned int indices[3];    // índices correspondientes de cada nivel
//     memset(bufAux_punteros, 0, BLOCKSIZE);
//
//     /* Recorrer los bloques lógicos desde primerBL hasta último */
//     for (nBL = primerBL; nBL <= ultimoBL; nBL++){
//         /* obtener el rango (0: directo, 1: primer indirecto, etc.) y
//            el puntero inicial correspondiente */
//         if ((nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr)) < 0) return FALLO;
//
//         int nivel_punteros = nRangoBL;
//         /* Si es bloque directo, el puntero está en inodo->punterosDirectos[] */
//         if (nRangoBL == 0) ptr = inodo->punterosDirectos[nBL];
//         else
//             /* Para bloques indirectos, se obtiene el primer puntero desde el array
//                de punterosIndirectos (recordando que el primer nivel indirecto se encuentra
//                en punterosIndirectos[0], etc.) */
//             ptr = inodo->punterosIndirectos[nRangoBL - 1];
//
//         /* Descender por la cadena de bloques de punteros hasta llegar al bloque de datos */
//         while (ptr > 0 && nivel_punteros > 0) {
//             int indice = obtener_indice(nBL, nivel_punteros);
//             /* Solo se hace bread si es la primera vez que se accede a este nivel */
//             if (indice == 0 || nBL == primerBL)
//                 if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO)
//                     return FALLO;
//             ptr_nivel[nivel_punteros - 1] = ptr;
//             indices[nivel_punteros - 1] = indice;
//             ptr = bloques_punteros[nivel_punteros - 1][indice];
//             nivel_punteros--;
//         }
//
//         /* Si se encontró un bloque de datos (ptr > 0) se procede a liberarlo */
//         if (ptr > 0) {
//             if (liberar_bloque(ptr) == FALLO) return FALLO;
//             liberados++;
//
//             /* Actualizar el puntero que apuntaba al bloque liberado */
//             if (nRangoBL == 0) {
//                 inodo->punterosDirectos[nBL] = 0;
//             } else {
//                 /* Para bloques indirectos, se retrocede por la cadena de punteros.
//                    Se pone a cero el puntero que referenciaba al bloque liberado.
//                    Luego se comprueba (mediante memcmp) si en ese bloque de punteros quedan
//                    otros punteros activos. Si no es así, se libera también ese bloque. */
//                 nivel_punteros = 1;
//                 while (nivel_punteros <= nRangoBL) {
//                     int indice = indices[nivel_punteros - 1];
//                     bloques_punteros[nivel_punteros - 1][indice] = 0;
//                     unsigned int ptr_actual = ptr_nivel[nivel_punteros - 1];
//                     if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros, BLOCKSIZE) == 0) {
//                         if (liberar_bloque(ptr_actual) == FALLO) return FALLO;
//                         liberados++;
//                         /* Si se libera el bloque de punteros a nivel máximo, actualizar el inodo */
//                         if (nivel_punteros == nRangoBL) inodo->punterosIndirectos[nRangoBL - 1] = 0;
//                         nivel_punteros++;
//                     } else {
//                         /* Si quedan punteros activos, se actualiza el bloque de punteros en disco */
//                         if (bwrite(ptr_actual, bloques_punteros[nivel_punteros - 1]) == FALLO) return FALLO;
//                         break;
//                     }
//                 }
//             }
//         }
//     }
//
//     return liberados;
// }




/***********************************************/
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

/**
 * Helper function to recursively free blocks within a specified range of logical blocks.
 * 
 * @param primerBL First logical block to free
 * @param ultimoBL Last logical block to free
 * @param ptr Pointer to the physical block number of the current index block
 * @param nivel Level of indirection (1 for single, 2 for double, 3 for triple)
 * @param bl_inicial First logical block covered by this index block
 * @param bl_final Last logical block covered by this index block
 * @return Number of blocks freed, or -1 on error
 */
int liberar_bloques_en_rango(unsigned int primerBL, unsigned int ultimoBL, unsigned int *ptr, int nivel, unsigned int bl_inicial, unsigned int bl_final) {
    // if the pointer is zero, no blocks to free
    if (*ptr == 0) return 0;

    // read the block of pointers
    unsigned int bloque_punteros[NPUNTEROS];
    if (bread(*ptr, bloque_punteros) == -1) {
        ERROR("fallo al leer el bloque: %u", *ptr);
        return FALLO;
    }

    // keep a copy to detect modifications
    unsigned int bloque_punteros_original[NPUNTEROS];
    memcpy(bloque_punteros_original, bloque_punteros, sizeof(bloque_punteros) / sizeof(*bloque_punteros));

    int liberados = 0;
    // base case: single indirect level (points to data blocks)
    if (nivel == 1) {
        for (int i = 0; i < NPUNTEROS; i++) {
            unsigned int bl = bl_inicial + i;
            if (bl >= primerBL && bl <= ultimoBL && bloque_punteros[i] != 0) {
                liberar_bloque(bloque_punteros[i]);  // Free the data block
                bloque_punteros[i] = 0;              // Clear the pointer
                liberados++;
            }
        }
    } else {  // higher levels of indirection (double or triple)
        unsigned int stride = 1; // number of logical blocks covered by each pointer
        for (int i = 1; i < nivel; i++) stride *= NPUNTEROS;
        for (int i = 0; i < NPUNTEROS; i++) {
            unsigned int sub_bl_inicial = bl_inicial + i * stride;
            unsigned int sub_bl_final = sub_bl_inicial + stride - 1;
            // Check if this subrange overlaps with the range to free
            if (sub_bl_final >= primerBL && sub_bl_inicial <= ultimoBL && bloque_punteros[i] != 0) {
                int liberados_rec = liberar_bloques_en_rango(
                    max(primerBL, sub_bl_inicial),
                    min(ultimoBL, sub_bl_final),
                    &bloque_punteros[i],
                    nivel - 1,
                    sub_bl_inicial,
                    sub_bl_final
                );
                if (liberados_rec < 0) {
                    return -1;
                }
                liberados += liberados_rec;
            }
        }
    }

    // Check if the block was modified
    if (memcmp(bloque_punteros, bloque_punteros_original, BLOCKSIZE) != 0) {
        unsigned int bufAux_punteros[NPUNTEROS];
        memset(bufAux_punteros, 0, BLOCKSIZE);  // Buffer of zeros for comparison
        // If all pointers are now zero, free the index block
        if (memcmp(bloque_punteros, bufAux_punteros, BLOCKSIZE) == 0) {
            liberar_bloque(*ptr);
            *ptr = 0;  // Update the parent pointer
            liberados++;  // Count the freed index block
        } else {
            // Write back the modified block
            if (bwrite(*ptr, bloque_punteros) == -1) {
                fprintf(stderr, "Error writing block %u\n", *ptr);
                return -1;
            }
        }
    }

    return liberados;
}

/**
 * Frees all occupied physical blocks associated with an inode starting from a given logical block.
 * 
 * @param primerBL First logical block to start freeing from
 * @param inodo Pointer to the inode structure
 * @return Number of blocks freed, or -1 on error
 */
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
    // If the file is empty, no blocks to free
    if (inodo->tamEnBytesLog == 0) {
        return 0;
    }

    // Calculate the last logical block with content
    unsigned int ultimoBL;
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
        ultimoBL = (inodo->tamEnBytesLog / BLOCKSIZE) - 1;
    } else {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }

    // If the starting block is beyond the last block, nothing to free
    if (primerBL > ultimoBL) {
        return 0;
    }

    int liberados = 0;

    // Define logical block ranges for each level
    unsigned int bl_starts[4] = {
        0,                           // Direct: BL 0
        DIRECTOS,                    // Indirect[0]: BL 12
        DIRECTOS + NPUNTEROS,        // Indirect[1]: BL 268
        DIRECTOS + NPUNTEROS + NPUNTEROS * NPUNTEROS  // Indirect[2]: BL 65804
    };
    unsigned int bl_ends[4] = {
        DIRECTOS - 1,                                   // BL 11
        DIRECTOS + NPUNTEROS - 1,                       // BL 267
        DIRECTOS + NPUNTEROS + NPUNTEROS * NPUNTEROS - 1,  // BL 65803
        DIRECTOS + NPUNTEROS + NPUNTEROS * NPUNTEROS + NPUNTEROS * NPUNTEROS * NPUNTEROS - 1  // BL 16843019
    };

    // Process each level: direct, single indirect, double indirect, triple indirect
    for (int level = 0; level < 4; level++) {
        if (primerBL <= bl_ends[level] && ultimoBL >= bl_starts[level]) {
            if (level == 0) {
                // Handle direct blocks
                unsigned int start_bl = max(primerBL, bl_starts[0]);
                unsigned int end_bl = min(ultimoBL, bl_ends[0]);
                for (unsigned int bl = start_bl; bl <= end_bl; bl++) {
                    if (inodo->punterosDirectos[bl] != 0) {
                        liberar_bloque(inodo->punterosDirectos[bl]);
                        inodo->punterosDirectos[bl] = 0;
                        liberados++;
                    }
                }
            } else {
                // Handle indirect blocks
                unsigned int *ptr = &inodo->punterosIndirectos[level - 1];
                if (*ptr != 0) {
                    int liberados_rec = liberar_bloques_en_rango(
                        max(primerBL, bl_starts[level]),
                        min(ultimoBL, bl_ends[level]),
                        ptr,
                        level,
                        bl_starts[level],
                        bl_ends[level]
                    );
                    if (liberados_rec < 0) {
                        return -1;
                    }
                    liberados += liberados_rec;
                }
            }
        }
    }

    return liberados;
}

/**
 * Frees an inode and all its associated blocks, adding it to the list of free inodes.
 * 
 * @param ninodo Inode number to free
 * @return The inode number freed, or -1 on error
 */
int liberar_inodo(unsigned int ninodo) {
    // Read the inode
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error reading inode %u\n", ninodo);
        return -1;
    }

    // Free all blocks starting from logical block 0
    int liberados = liberar_bloques_inodo(0, &inodo);
    if (liberados < 0) {
        return -1;
    }

    // Update the number of occupied blocks
    inodo.numBloquesOcupados -= liberados;

    // Mark the inode as free and reset its logical size
    inodo.tipo = 'l';  // 'l' indicates a free inode
    inodo.tamEnBytesLog = 0;

    // Update the list of free inodes
    struct superbloque SB;
    if (bread(0, &SB) == -1) {
        fprintf(stderr, "Error reading superblock\n");
        return -1;
    }
    unsigned int posPrimerInodoLibre = SB.posPrimerInodoLibre;
    inodo.punterosDirectos[0] = posPrimerInodoLibre;  // Link to the previous first free inode
    SB.posPrimerInodoLibre = ninodo;                  // Set this inode as the new first free inode
    SB.cantInodosLibres++;                            // Increment the count of free inodes

    // Write back the superblock
    if (bwrite(0, &SB) == -1) {
        fprintf(stderr, "Error writing superblock\n");
        return -1;
    }

    // Update the inode's creation time
    time(&inodo.ctime);

    // Write back the updated inode
    if (escribir_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, "Error writing inode %u\n", ninodo);
        return -1;
    }

    return ninodo;  // Return the freed inode number
}
