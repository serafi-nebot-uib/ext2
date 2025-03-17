#include "ficheros.h"
#define TAM_BUFFER BLOCKSIZE * 2  // Cantidad de Bytes por transacción de lectura (BPT)

int main(int argc, char **argv) {
    if (argc < 2 || argv[2][0] == '-') { //Aseguramos que hay un número de inodo positivo
        fprintf(stderr, BOLD "Sintaxis: " RESET "%s <nombre_dispositivo> <ninodo>\n", argv[0]);
        return FALLO;
    }

    const char * nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);

    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "error al montar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }

    stat_t stat = {};
    if(mi_stat_f(ninodo, &stat) == FALLO) return FALLO;
    unsigned char buffer_texto[TAM_BUFFER] = {};
    memset(buffer_texto, 0, TAM_BUFFER);
    unsigned int offset = 0;
    int leidos, total_leidos = 0;
    
    if((leidos = mi_read_f(ninodo, buffer_texto, offset, TAM_BUFFER)) == FALLO){
        fprintf(stderr, GRAY "\ntotal_leidos %d\n" RESET, total_leidos);
        fprintf(stderr, GRAY "tamEnBytesLog %u\n" RESET, stat.tamEnBytesLog);    
        return FALLO;
    }
    total_leidos += leidos;

    while(leidos > 0){
        memset(buffer_texto, 0, TAM_BUFFER);
        offset += TAM_BUFFER;
        if((leidos = mi_read_f(ninodo, buffer_texto, offset, TAM_BUFFER)) == FALLO) return FALLO;
        write(1, buffer_texto, leidos);
        total_leidos += leidos;
    }
    

    // Imprime el total de los Bytes leídos y el tamaño del inodo en Bytes lógicos
    fprintf(stderr, GRAY "\ntotal_leidos %d\n" RESET, total_leidos);
    fprintf(stderr, GRAY "tamEnBytesLog %u\n" RESET, stat.tamEnBytesLog);

    if (bumount() == FALLO) {
        fprintf(stderr, "error al desmontar el dispositivo virtual %s\n", nombre_dispositivo);
        return FALLO;
    }
    return 0;
}