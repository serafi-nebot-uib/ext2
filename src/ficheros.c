#include "ficheros.h"

int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    return EXITO;
}

int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    return EXITO;
}

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    return EXITO;
}

int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    return EXITO;
}
