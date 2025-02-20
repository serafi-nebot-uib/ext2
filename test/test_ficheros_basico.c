// TODO: set include path in makefile
//
#include <stdio.h>

#include "ficheros_basico.h"

int main(int argc, char **argv) {
    printf("tamMB(1000): %d\n", tamMB(1000));
    printf("tamMB(100000): %d\n", tamMB(100000));
    printf("tamMB(1000000): %d\n", tamMB(1000000));
    printf("\n");

    printf("tamAI(5000): %d\n", tamAI(5000));
    printf("tamAI(25000): %d\n", tamAI(25000));
    printf("tamAI(125500): %d\n", tamAI(125500));

    return 0;
}
