#include "ficheros.h"

int main(int argc, char **argv) {
    if (argc != 4 || (argv[3][0] != '0' && argv[3][0] != '1')) { // aseguramos que hay un 0 o un 1
        fprintf(stderr, BOLD "sintaxis: " RESET "%s <nombre_dispositivo> <texto> <diferentes_inodos>\n", argv[0]);
        fprintf(stderr, BOLD "offsets: " RESET "9000, 209000, 30725000, 409605000, 480000000\n");
        fprintf(stderr, "si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n\n");
        return FALLO;
    }

    const char * nombre_dispositivo = argv[1];
    const char * buf_original = argv[2];  // texto
    unsigned int nbytes = strlen(buf_original); // longitud del texto

    // si diferentes_inodos = 0 se reserva un solo inodo para todos los offsets.
    // si diferentes_inodos = 1 se reserva un inodo diferente para cada offset.
    const int diferentes_inodos = atoi(argv[3]);
    printf("longitud texto (nbytes): %u\n", nbytes); // this is not a debug print

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo virtual: %s", nombre_dispositivo);
        return FALLO;
    }

    // ejemplos de offsets para utilizar los diferentes tipos de punteros:
    // 9.000 (⊂BL 8), 209000 (⊂BL 204), 30725000 (⊂BL 30004),  409605000 (⊂BL 400004) y 480000000 (⊂BL 468750)
    unsigned int offsets[] = { 9000, 209000, 30725000, 409605000, 480000000 };
    int ninodo;
    int bytesEscritos;

    // reserva un inodo de tipo fichero con permisos de lectura y escritura
    if ((ninodo = reservar_inodo ('f', 6)) == FALLO) return FALLO;

    size_t offsets_size = sizeof(offsets) / sizeof(*offsets);
    for (unsigned int offset_idx = 0; offset_idx < offsets_size; offset_idx++){
        printf("\nNº inodo reservado: %d\n", ninodo);
        printf("offset: %u\n", offsets[offset_idx]);

        if ((bytesEscritos = mi_write_f(ninodo, buf_original, offsets[offset_idx], nbytes)) == FALLO) return FALLO;

        char buf_original_read[nbytes]; // se vuelca en otro buffer ya que el original es CONST y no permite modificar
        memset(buf_original_read, 0, nbytes);

        if (mi_read_f(ninodo, buf_original_read, offsets[offset_idx], nbytes) == FALLO) return FALLO;

        stat_t stat = {};
        if (mi_stat_f(ninodo, &stat) == FALLO) return FALLO;
        printf("bytes escritos: %u\n", bytesEscritos);
        printf("stat.tipo: %c\n", stat.tipo);
        printf("stat.permisos: %hhu\n", stat.permisos);
        printf("stat.atime: %lu\n", stat.atime);
        printf("stat.mtime: %lu\n", stat.mtime);
        printf("stat.ctime: %lu\n", stat.ctime);
        printf("stat.btime: %lu\n", stat.btime);
        printf("stat.nlinks: %u\n", stat.nlinks);
        printf("stat.tamEnBytesLog: %u\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados: %u\n", stat.numBloquesOcupados);

        if (diferentes_inodos && (offset_idx < offsets_size - 1)) {
            if ((ninodo = reservar_inodo('f', 6)) == FALLO) return FALLO;
        }
    }

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    return 0;
}
