#include "ficheros.h"
#define OFFSET_SIZE 5

int main(int argc, char **argv) {
    if (argc != 4 || (argv[3][0] != '0' && argv[3][0] != '1')) { //Aseguramos que hay un 0 o un 1
        fprintf(stderr, BOLD "Sintaxis: " RESET "%s <nombre_dispositivo><\"$(cat fichero)\"> <diferentes_inodos>\n", argv[0]);
        fprintf(stderr, BOLD "Offsets: " RESET "9000, 209000, 30725000, 409605000, 480000000\n");
        fprintf(stderr, "Si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n\n");
        return FALLO;
    }

    const char * nombre_dispositivo = argv[1];
    const char * buf_original = argv[2];  // texto
    unsigned int nbytes = strlen(buf_original); // longitud del texto
    
    // Si diferentes_inodos = 0 se reserva un solo inodo para todos los offsets. 
    // Si diferentes_inodos = 1 se reserva un inodo diferente para cada offset. 
    const int diferentes_inodos = atoi(argv[3]);
    
    // fprintf(stderr, GRAY "\n[leer.c]\n");
    // fprintf(stderr, "(argv[1]) nombre_dispositivo: %s\n", nombre_dispositivo);
    // fprintf(stderr, "(argv[2]) buf_original: %s\n", buf_original);
    // fprintf(stderr, "(argv[3]) diferentes_inodos = %d\n" RESET, diferentes_inodos);
    // fprintf(stderr, GRAY BOLD "buf_original (HEX, src): \n" RESET);
    // for (size_t i = 0; i < nbytes; i++) fprintf(stderr, "%02X ", buf_original[i]); 
    // fprintf(stderr, "\n\n");
    printf("longitud texto (nbytes): %u\n", nbytes); // this is not a debug print
    

    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    /* Ejemplos de offsets para utilizar los diferentes tipos de punteros: 
    9.000 (⊂BL 8), 209000 (⊂BL 204), 30725000 (⊂BL 30004),  409605000 (⊂BL 400004) y 480000000 (⊂BL 468750) */
    unsigned int offsets[OFFSET_SIZE] = {9000, 209000, 30725000, 409605000, 480000000};
    int ninodo;
    int bytesEscritos;

    //Reserva un inodo de tipo fichero con permisos de lectura y escritura
    if((ninodo = reservar_inodo ('f', 6)) == FALLO) return FALLO;

    for(unsigned int offset_idx = 0; offset_idx < OFFSET_SIZE; offset_idx++){
        // fprintf(stderr, GRAY "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━ INICIO ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" RESET);
        printf("\nNº inodo reservado: %d\n", ninodo);
        printf("offset: %u\n", offsets[offset_idx]);

        // fprintf(stderr, GRAY "[pre-write buffer]: " RESET); write(1, buf_original, nbytes); printf("\n");
        if((bytesEscritos = mi_write_f(ninodo, buf_original, offsets[offset_idx], nbytes)) == FALLO) return FALLO;

        //*** TEST ***
        char buf_original_read[nbytes]; //se vuelca en otro buffer ya que el original es CONST y no permite modificar
        memset(buf_original_read, 0, nbytes);

        // fprintf(stderr, GRAY "\n[buffer set to 0]\n" RESET);
        // fprintf(stderr, GRAY "Executing mi_read_f()\n" RESET);  
        int bytesLeidos;
        if((bytesLeidos = mi_read_f(ninodo, buf_original_read, offsets[offset_idx], nbytes)) == FALLO) return FALLO;
        // fprintf(stderr, GRAY "\nBytes LEIDOS: %d [returned value from mi_read_f(), should be greater than %d (nbytes)]\n" RESET, bytesLeidos, nbytes);
        // fprintf(stderr, GRAY "[post-read buffer]: " RESET); write(1, buf_original_read, nbytes); printf("\n");
        //*************

        // fprintf(stderr, GRAY "\nExecuting mi_stat_f()\n" RESET);  
        stat_t stat = {};
        if(mi_stat_f(ninodo, &stat) == FALLO) return FALLO;
        printf("Bytes escritos: %u\n", bytesEscritos);
        printf("stat.tamEnBytesLog=%u\n", stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%u\n", stat.numBloquesOcupados);

        if(diferentes_inodos && (offset_idx < OFFSET_SIZE-1)) {
            if((ninodo = reservar_inodo ('f', 6)) == FALLO) return FALLO;
        }
    }
    
    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }
    return 0;
}
