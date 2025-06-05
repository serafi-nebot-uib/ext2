/**************************************************************************
* FILENAME: escribir.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "core/directorios.h"

int main(int argc, char *argv[]) {
    if (argc != 4 || (argv[3][0] != '0' && argv[3][0] != '1')) {
        // aseguramos que hay un 0 o un 1
        fprintf(stderr, RED "sintaxis: %s <nombre_dispositivo> <texto> <diferentes_inodos>\n" RESET, argv[0]);
        fprintf(stderr, RED "offsets: 9000, 209000, 30725000, 409605000, 480000000\n" RESET);
        fprintf(stderr, "si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];

    // si diferentes_inodos = 0 se reserva un solo inodo para todos los offsets.
    // si diferentes_inodos = 1 se reserva un inodo diferente para cada offset.
    int diferentes_inodos = atoi(argv[3]);

    unsigned int nbytes = strlen(texto);
    printf("longitud texto (nbytes): %u\n", nbytes); // this is not a debug print

    if (bmount(nombre_dispositivo) == FALLO) {
        ERROR("no se ha podido montar el dispositivo: \"%s\"", nombre_dispositivo);
        return FALLO;
    }

    unsigned int offsets[] = { 9000, 209000, 30725000, 409605000, 480000000 };
    int ninodo, bytes_escritos;
    stat_t stat;

    if (diferentes_inodos == 0) {
        // se reserva un único inodo para todas las escrituras
        ninodo = reservar_inodo('f', 6);
        if (ninodo < 0) {
            ERROR("no se ha podido reservar el inodo");
            bumount();
            return FALLO;
        }

        // escribimos el mismo texto en distintos offsets usando el mismo inodo
        for (int i = 0; i < sizeof(offsets) / sizeof(*offsets); i++) {
            printf("\nNº inodo reservado: %d\n", ninodo);
            printf("offset: %u\n", offsets[i]);
            bytes_escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);
            if (bytes_escritos < 0) ERROR("no se ha podido escribir al offset: %u", offsets[i]);
            else printf("Bytes escritos: %d\n", bytes_escritos);

            // actualizamos y mostramos la información del inodo tras la escritura
            if (mi_stat_f(ninodo, &stat) == FALLO) {
                ERROR("no se ha podido obtener la información del inodo: %u", ninodo);
            } else {
                printf("stat.tamEnBytesLog = %u\n", stat.tamEnBytesLog);
                printf("stat.numBloquesOcupados = %d\n", stat.numBloquesOcupados);
            }
        }
    } else {
        // se reserva un inodo distinto para cada offset
        for (int i = 0; i < sizeof(offsets) / sizeof(*offsets); i++) {
            ninodo = reservar_inodo('f', 6);
            if (ninodo < 0) {
                ERROR("no se ha podido reservar el inodo para el offset: %u", offsets[i]);
                continue;
            }
            printf("\nNº inodo reservado: %d\n", ninodo);
            printf("offset: %u\n", offsets[i]);
            bytes_escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);
            if (bytes_escritos < 0) ERROR("no se ha podido escribir al offset: %u", offsets[i]);
            else printf("Bytes escritos: %d\n", bytes_escritos);
            // obtenemos la información del inodo
            if (mi_stat_f(ninodo, &stat) == FALLO) {
                ERROR("no se ha podido obtener la información del inodo: %u", ninodo);
            } else {
                printf("stat.tamEnBytesLog = %u\n", stat.tamEnBytesLog);
                printf("stat.numBloquesOcupados = %d\n", stat.numBloquesOcupados);
            }
        }
    }

    if (bumount() == FALLO) {
        ERROR("no se ha podido desmontar el dispositivo: %s", nombre_dispositivo);
        return FALLO;
    }

    return 0;
}
