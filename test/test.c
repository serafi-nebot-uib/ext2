#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

// TODO: this is not ideal, but I want clangd lsp to shut up
#include "../src/logging.h"
#include "../src/bloques.h"
#include "../src/ficheros_basico.h"

#define DEFAULT_DEVICE_NAME "disco_test"

#define EXISTS(path) (access(path, F_OK) == 0)
#define DELETE_IF_EXISTS(path) {if (EXISTS(path)) assert(remove(DEFAULT_DEVICE_NAME) == 0);}

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

int main(int argc, char **argv) {
    test_mount_umount();
    test_bread_bwrite();
    test_struct_size();
    test_bloques_tam();
    test_init_fs();
    return 0;
}
