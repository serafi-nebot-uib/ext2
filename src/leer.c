#include “ficheros.h”
#define OFFSET_SIZE 5

int main(int argc, char **argv) {
    if (argc != 4 || (argv[3][0] != '0' & argv[3][0] != '1')) {
        fprintf(stderr, BOLD "Sintaxis: " RESET "%s <nombre_dispositivo><\"$(cat fichero)\"> <diferentes_inodos>\n", argv[0]);
        fprintf(stderr, BOLD "Offsets: " RESET "9000, 209000, 30725000, 409605000, 480000000\n");
        fprintf(stderr, "Si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n");
        return FALLO;
    }

    const char * nombre_dispositivo = argv[1];
    const char * texto = argv[2];
    size_t longitud = strlen(texto); //nbytes
    
    // /* Si diferentes_inodos = 0 se reserva un solo inodo para todos los offsets. 
    //    Si diferentes_inodos = 1, se reserva un inodo diferente para cada offset. 

    // (!) No se ha realizado un control de errores para la entrada de argv[3] (diferentes_inodos)
    //     Hemos supuesto que cualquier valor != 0 se interpretará como un 1
    // */
    const int diferentes_inodos = atoi(argv[3]);
    
    fprintf(stderr, GRAY "\n[leer.c]\n");
    fprintf(stderr, "(argv[1]) nombre_dispositivo: %s\n", nombre_dispositivo);
    fprintf(stderr, "(argv[2]) texto: %s\n", texto);
    fprintf(stderr, "(argv[3]) diferentes_inodos = %d\n" RESET, diferentes_inodos);
    printf("longitud texto: %d\n", longitud); // this is not a debug print
    
    /* Ejemplos de offsets para utilizar los diferentes tipos de punteros: 
    9.000 (⊂BL 8), 209.000 (⊂BL 204), 30.725.000 (⊂BL 30.004),  409.605.000 (⊂BL 400.004) y 480.000.000 (⊂BL 468.750) */

    unsigned int offsets[OFFSET_SIZE] = {9000, 209000, 30725000, 409605000, 480000000};
    unsigned int offset_idx = 0;
    int ninodo;
    int bytesEscritos;
    
    if(!diferentes_inodos){ // Si es 0 ->  se reserva un solo inodo para todos los offsets
        //Reserva un inodo de tipo fichero con permisos de lectura y escritura
        if((ninodo = reservar_inodo ('f', 6)) == -1) return FALLO;
        printf("Nº inodo reservado: %d\n", ninodo);

        for(offset_idx = 0; offset_idx < OFFSET_SIZE; offset_idx++){
            if(mi_write_f(ninodo, /*(const void *)*/&texto, offsets[offset_idx], longitud) {
            printf("offset: %u\n", offsets[offset_idx]);
        }

    } else if (diferentes_inodos) // Si es 1 -> se reserva un inodo diferente para cada offset.

    
    

    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    
    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }
    return 0;
}
