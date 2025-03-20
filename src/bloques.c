/**************************************************************************
* FILENAME: bloques.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "bloques.h"

// descriptor del fichero actual
static int fd = 0;

// modo de creación de ficheros: (-rw-rw-rw-)
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/**
 * Montar el dispositivo virtual abriendo/creando un fichero
 *
 * @param camino ruta al dispositivo virtual
 * @return descriptor del fichero creado, FALLO si hay error
 */
int bmount(const char *camino) {
    int ret = FALLO; // contiene el valor de retorno
    // se cambia la mascara de creación de ficheros a 000 para que se permita qualquier tipo de modo
    // esto es necesario ya que en algunos sistemas la máscara por defecto = 0022, 
    // lo que significa que si creamos un fichero en modo 0666 se va a crear en modo:
    //      0666 & ~0022 = 0b110110110 & ~0b000010010
    //                   = 0b110110110 &  0b111101101
    //                   = 0b110100100
    //                   = 0644
    mode_t mask = umask(000);
    // abrir/crear el fichero con los permisos por defecto (FILE_MODE)
    if ((fd = open(camino, O_RDWR | O_CREAT, FILE_MODE)) < 0) {
        ERRSYS("open");
    } else {
        ret = fd;
    }
    umask(mask); // restaurar la antigua mascara de creación
    return ret;
}

/**
 * Desmontar el dispositivo virtual cerrando el fichero
 *
 * @return EXITO si se ha desmontado correctamente el dispositivo virtual, FALLO en caso contrario
 */
int bumount() {
    if (close(fd) >= 0) return EXITO;
    ERRSYS("close");
    return FALLO;
}

/**
 * Escribir los datos almacenados en buffer de datos buf al bloque número nbloque
 *
 * @param nbloque número de bloque al que escribir
 * @param buf puntero al buffer de datos a escribir (debe ser un buffer de BLOCKSIZE bytes)
 * @return número de bytes escritos, FALLO en caso de error
 */
int bwrite(unsigned int nbloque, const void *buf) {
    // desplaza el cursor del archivo hasta el primer byte del bloque especificado (nbloque ∗ BLOCKSIZE)
    if (lseek(fd, nbloque*BLOCKSIZE, SEEK_SET) < 0) {
        ERRSYS("lseek");
        return FALLO;
    }
    size_t nbytes = write(fd, buf, BLOCKSIZE);
    if (nbytes < 0) {
        ERRSYS("write");
        return FALLO;
    }
    return nbytes;
}

/**
 * Leer los datos almacenados en el bloque número nbloque al buffer de datos buf
 *
 * @param nbloque número de bloque al que leer
 * @param buf dirección donde se volcarán los datos leídos (debe ser un buffer de BLOCKSIZE bytes)
 * @return número de bytes leídos, FALLO en caso de error
 */
int bread(unsigned int nbloque, void *buf) {
    // mueve el cursor del fichero al primer byte del bloque indicado (nbloque * BLOCKSIZE)
    if (lseek(fd, nbloque * BLOCKSIZE, SEEK_SET) < 0) {
        ERRSYS("lseek");
        return FALLO;
    }
    size_t nbytes = read(fd, buf, BLOCKSIZE);
    if (nbytes < 0) {
        ERRSYS("read");
        return FALLO;
    }
    return nbytes;
}
