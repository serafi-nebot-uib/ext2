#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

// TODO: this is not ideal, but I want clangd lsp to shut up
#include "../src/logging.h"
#include "../src/bloques.h"
#include "../src/ficheros_basico.h"
#include "../src/ficheros.h"

#define DEFAULT_DEVICE_NAME "disco_test"
#define DEFAULT_BLOCK_CNT 100000

#define EXISTS(path) (access(path, F_OK) == 0)
#define DELETE_IF_EXISTS(path) {if (EXISTS(path)) assert(remove(DEFAULT_DEVICE_NAME) == 0);}

// TODO: file descriptors are not closed when assert fails?

static unsigned char buff_1[BLOCKSIZE] = {};
static unsigned char buff_2[BLOCKSIZE] = {};
static superbloque_t sb = {};
static inodo_t inodos[BLOCKSIZE / INODOSIZE] = {};

void test_mount_umount() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);
    assert(EXISTS(DEFAULT_DEVICE_NAME));
    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

void test_bread_bwrite() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    memset(buff_1, 0xff, BLOCKSIZE);
    assert(bwrite(0, buff_1) != FALLO);
    assert(bread(0, buff_2) != FALLO);
    assert(memcmp(buff_1, buff_2, BLOCKSIZE) == 0);

    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

void test_bloques_tam() {
    assert(tamMB(1000) == 1);
    assert(tamMB(100000) == 13);
    assert(tamMB(1000000) == 123);

    assert(tamAI(5000) == 625);
    assert(tamAI(25000) == 3125);
    assert(tamAI(125500) == 15688);
}

void test_struct_size() {
    assert(sizeof(superbloque_t) == BLOCKSIZE);
    assert(sizeof(inodo_t) == INODOSIZE);
}

static unsigned int test_initSB_bs[] = { 200000, 500000, 1000000 };
static superbloque_t test_initSB_sb[] = {
    [0] = {
        .posPrimerBloqueMB = 1,
        .posUltimoBloqueMB = 25,
        .posPrimerBloqueAI = 26,
        .posUltimoBloqueAI = 6275,
        .posPrimerBloqueDatos = 6276,
        .posUltimoBloqueDatos = 199999,
        .posInodoRaiz = 0,
        .posPrimerInodoLibre = 0,
        .cantBloquesLibres = 193724,
        .cantInodosLibres = 50000,
        .totBloques = 200000,
        .totInodos = 50000
    },
    [1] = {
        .posPrimerBloqueMB = 1,
        .posUltimoBloqueMB = 62,
        .posPrimerBloqueAI = 63,
        .posUltimoBloqueAI = 15687,
        .posPrimerBloqueDatos = 15688,
        .posUltimoBloqueDatos = 499999,
        .posInodoRaiz = 0,
        .posPrimerInodoLibre = 0,
        .cantBloquesLibres = 484312,
        .cantInodosLibres = 125000,
        .totBloques = 500000,
        .totInodos = 125000
    },
    [2] = {
        .posPrimerBloqueMB = 1,
        .posUltimoBloqueMB = 123,
        .posPrimerBloqueAI = 124,
        .posUltimoBloqueAI = 31373,
        .posPrimerBloqueDatos = 31374,
        .posUltimoBloqueDatos = 999999,
        .posInodoRaiz = 0,
        .posPrimerInodoLibre = 0,
        .cantBloquesLibres = 968626,
        .cantInodosLibres = 250000,
        .totBloques = 1000000,
        .totInodos = 250000
    }

};

void test_init_fs() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    for (unsigned int i = 0; i < sizeof(test_initSB_bs) / sizeof(*test_initSB_bs); i++) {
        assert(initSB(test_initSB_bs[i], test_initSB_bs[i] / 4) != FALLO);
        assert(initMB() != FALLO);
        assert(initAI() != FALLO);
        assert(bread(posSB, &sb) != FALLO);
        assert(memcmp(&sb, &test_initSB_sb[i], INODOSIZE) == 0);

        unsigned int ptr = 1;
        for (unsigned int j = sb.posPrimerBloqueAI; j <= sb.posUltimoBloqueAI; j++) {
            assert(bread(j, inodos) != FALLO);
            for (unsigned int k = 0; k < BLOCKSIZE / INODOSIZE; k++) {
                assert(inodos[k].punterosDirectos[0] == ptr++);
                if (ptr == test_initSB_sb[i].totInodos) ptr = ~0;
            }
        }
    }

    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

void test_leer_escribir_bit() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    memset(buff_1, 0x00, BLOCKSIZE);
    for (int i = 0; i < DEFAULT_BLOCK_CNT; i++) assert(bwrite(i, buff_1) != FALLO);
    assert(initSB(DEFAULT_BLOCK_CNT, DEFAULT_BLOCK_CNT / 4) != FALLO);

    unsigned int block = BLOCKSIZE / 3;
    assert(escribir_bit(block, 1) != FALLO);
    assert(leer_bit(block) == 1);
    assert(escribir_bit(block, 0) != FALLO);
    assert(leer_bit(block) == 0);

    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

// right now only checks if blocks can be reserved/freed and in the order it does so
void test_reservar_bloque() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    assert(initSB(DEFAULT_BLOCK_CNT, DEFAULT_BLOCK_CNT / 4) != FALLO);
    assert(initMB() != FALLO);
    assert(initAI() != FALLO);

    unsigned int blocks[5] = {};
    for (unsigned int i = 0; i < sizeof(blocks) / sizeof(*blocks); i++) {
        blocks[i] = reservar_bloque();
        assert(blocks[i] != FALLO);
        if (i > 0) assert(blocks[i-1]+1 == blocks[i]);
    }

    for (unsigned int i = 0; i < sizeof(blocks) / sizeof(*blocks); i++) {
        assert(leer_bit(blocks[i]) == 1);
        assert(liberar_bloque(blocks[i]) == blocks[i]);
        assert(leer_bit(blocks[i]) == 0);
    }

    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

void test_traducir_bloque_inodo() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    assert(initSB(DEFAULT_BLOCK_CNT, DEFAULT_BLOCK_CNT / 4) != FALLO);
    assert(initMB() != FALLO);
    assert(initAI() != FALLO);

    int ninode = reservar_inodo('d', 07);
    assert(ninode != FALLO);

    unsigned int tests[] = { 8, 204, 30004, 400004, 468750 };
    for (unsigned int i = 0; i < sizeof(tests) / sizeof(*tests); i++) {
        printf("%s: %u\n", __func__, tests[i]);
        assert(traducir_bloque_inodo(ninode, tests[i], 0) == FALLO);
        assert(traducir_bloque_inodo(ninode, tests[i], 1) != FALLO);
        assert(traducir_bloque_inodo(ninode, tests[i], 0) != FALLO);
    }

    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

void test_write_f() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    memset(buff_1, 0x00, BLOCKSIZE);
    for (int i = 0; i < DEFAULT_BLOCK_CNT; i++) assert(bwrite(i, buff_1) != FALLO);
    assert(initSB(DEFAULT_BLOCK_CNT, DEFAULT_BLOCK_CNT / 4) != FALLO);
    assert(initMB() != FALLO);
    assert(initAI() != FALLO);

    int ninode = reservar_inodo('d', 07);
    assert(ninode != FALLO);

    const unsigned int start = 9000;
    const unsigned int size = 3571;

    // const unsigned int start = 1024;
    // const unsigned int size = 128;

    unsigned char *buff = (unsigned char *) malloc(size);
    assert(buff != NULL);
    memset(buff, 0x41, size);

    assert(mi_write_f(ninode, buff, start, size) == size);

    free(buff);

    assert(bumount() != FALLO);
    // DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

// TODO: reservar_bloque test

int main(int argc, char **argv) {
    // test_mount_umount();
    // test_bread_bwrite();
    // test_struct_size();
    // test_bloques_tam();
    // test_init_fs();
    // test_leer_escribir_bit();
    // test_reservar_bloque();
    // test_traducir_bloque_inodo();
    test_write_f();
    return 0;
}
