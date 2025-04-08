#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

// TODO: this is not ideal, but I want clangd lsp to shut up
#include "../src/bloques.h"
#include "../src/directorios.h"
#include "../src/ficheros.h"
#include "../src/ficheros_basico.h"
#include "../src/helper.h"
#include "../src/logging.h"

#define DEFAULT_DEVICE_NAME "disco_test"
#define DEFAULT_BLOCK_CNT 100000

#define EXISTS(path) (access(path, F_OK) == 0)
#define DELETE_IF_EXISTS(path) { if (EXISTS(path)) assert(remove(DEFAULT_DEVICE_NAME) == 0); }

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
    [0] = {.posPrimerBloqueMB = 1,
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
           .totInodos = 50000},
    [1] = {.posPrimerBloqueMB = 1,
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
           .totInodos = 125000},
    [2] = {.posPrimerBloqueMB = 1,
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
           .totInodos = 250000}
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
        if (i > 0) assert(blocks[i - 1] + 1 == blocks[i]);
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

void test_read_write() {
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

    // const unsigned int start = 9000 + 9000 % 16;
    // const unsigned int size = 3571;
    // const unsigned int size = 3571*2;
    // const unsigned int size = 1600000;
    // const unsigned int start = 9000 + 9000 % 16;
    // const unsigned int size = 3571;

    unsigned char *buff = (unsigned char *)malloc(size);
    assert(buff != NULL);
    memset(buff, 0x41, size);

    assert(mi_write_f(ninode, buff, start, size) == size);
    memset(buff, 0, size);
    int s = mi_read_f(ninode, buff, start, size);
    hexdump_col(buff, 0, s, start, 32, 8);
    // TODO: probar casos especiales para mi_read_f (e.g. tamaño a leer > tamaño bytes lógico)

    free(buff);

    assert(bumount() != FALLO);
    // DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
}

void test_inode_block_free() {
    // test_read_write(false, true);

    // assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);
    assert(bmount("disco") != FALLO);

    unsigned char buff[BLOCKSIZE] = {};
    memset(buff, 0xff, BLOCKSIZE);

    unsigned int ninode = 0;
    inodo_t inode = {};
    assert(leer_inodo(ninode, &inode) == EXITO);

    for (unsigned int i = 0; i < 10000; i++) assert(mi_write_f(ninode, buff, i * BLOCKSIZE, BLOCKSIZE) == BLOCKSIZE);
    assert(leer_inodo(ninode, &inode) == EXITO);
    printf("inodo.numBloquesOcupados: %u\n", inode.numBloquesOcupados);

    // int freed = liberar_bloques_inodo(0, &inode);
    // printf("inode: %d; freed: %d\n", ninode, freed);
    // assert(escribir_inodo(ninode, &inode) == EXITO);

    // unsigned int ptr = 0;
    // int rango = obtener_nRangoBL(&inode, 9999, &ptr);
    // printf("rango: %d\n", rango);

    // inodo_t inode = {};
    // for (unsigned int ninode = 1; ninode < 2; ninode++) {
    //     assert(leer_inodo(ninode, &inode) == EXITO);
    //     int freed = liberar_bloques_inodo(0, &inode);
    //     printf("inode: %d; freed: %d\n", ninode, freed);
    //     assert(escribir_inodo(ninode, &inode) == EXITO);
    // }

    assert(bumount() != FALLO);
}

static char *test_extraer_camino_camino[] = { "/dir1/dir2/fichero", "/dir/", "/fichero" };
static char *test_extraer_camino_inicial[] = { "dir1", "dir", "fichero" };
static char *test_extraer_camino_final[] = { "/dir2/fichero", "/", "" };
static char test_extraer_camino_tipo[] = { 'd', 'd', 'f' };

void test_extraer_camino() {
    char *camino = NULL, inicial[TAMNOMBRE], final[TAMNOMBRE], tipo = 0;
    size_t n = sizeof(test_extraer_camino_tipo) / sizeof(*test_extraer_camino_tipo);

    for (size_t i = 0; i < n; i++) {
        camino = test_extraer_camino_camino[i];
        memset(inicial, 0, TAMNOMBRE);
        memset(final, 0, TAMNOMBRE);
        tipo = 0;

        assert(extraer_camino(camino, inicial, final, &tipo) == EXITO);
        assert(strcmp(test_extraer_camino_inicial[i], inicial) == 0);
        assert(strcmp(test_extraer_camino_final[i], final) == 0);
        assert(test_extraer_camino_tipo[i] == tipo);
    }
}

static char *test_buscar_entrada_entrada[] = { "pruebas/", "/pruebas/", "/pruebas/docs/", "/pruebas/", "/pruebas/docs/", "/pruebas/docs/doc1", "/pruebas/docs/doc1/doc11", "/pruebas/", "/pruebas/docs/doc1", "/pruebas/docs/doc1", "/pruebas/casos/", "/pruebas/docs/doc2" };
static int test_buscar_entrada_reservar[] = { 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1 };
static int test_buscar_entrada_ret[] = { ERROR_CAMINO_INCORRECTO, ERROR_NO_EXISTE_ENTRADA_CONSULTA, ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO, EXITO, EXITO, EXITO, ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO, ERROR_ENTRADA_YA_EXISTENTE, EXITO, ERROR_ENTRADA_YA_EXISTENTE, EXITO, EXITO };

void test_buscar_entrada() {
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
    assert(bmount(DEFAULT_DEVICE_NAME) != FALLO);

    memset(buff_1, 0x00, BLOCKSIZE);
    for (int i = 0; i < DEFAULT_BLOCK_CNT; i++) assert(bwrite(i, buff_1) != FALLO);
    assert(initSB(DEFAULT_BLOCK_CNT, DEFAULT_BLOCK_CNT / 4) != FALLO);
    assert(initMB() != FALLO);
    assert(initAI() != FALLO);

    int ninode = reservar_inodo('d', 07);
    assert(ninode != FALLO);

    size_t n = sizeof(test_buscar_entrada_entrada) / sizeof(*test_buscar_entrada_entrada);

    for (size_t i = 0; i < n; i++) {
        unsigned int p_inodo_dir = 0;
        unsigned int p_inodo = 0;
        unsigned int p_entrada = 0;
        int ret = buscar_entrada(test_buscar_entrada_entrada[i], &p_inodo_dir, &p_inodo, &p_entrada, test_buscar_entrada_reservar[i], 6);
        assert(ret == test_buscar_entrada_ret[i]);
    }

    assert(bumount() != FALLO);
    DELETE_IF_EXISTS(DEFAULT_DEVICE_NAME);
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
    // test_read_write();

    // assert(bmount("disco") != FALLO);
    // unsigned int blocks[] = { 3139, 3140, 3141, 3142, 3143, 3144, 3145, 3146, 3147, 3148, 3149, 3150, 3151, 3152, 3153, 3154, 3155, 3156, 3157, 3158, 3159, 3160, 3161, 3162, 3163, 3164, 3165, 3166,
    // 3167, 3168, 3169, 3170, 3171 };

    // test_inode_block_free();

    // printf("checking %lu blocks\n", sizeof(blocks) / sizeof(*blocks));
    // for (size_t i = 0; i < sizeof(blocks) / sizeof(*blocks); i++) {
    //     if (leer_bit(blocks[i]) == 1) printf("%u\n", blocks[i]);
    // }

    // test_extraer_camino();
    test_buscar_entrada();

    return 0;
}
