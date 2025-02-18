/**************************************************************************
* FILENAME: bloques.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/
#include "bloques.h"

// Descriptor del fichero actual
static int fd = 0;

// Macro que formatea y simplifica la impresión de errno
#define ERR_PRINT(name) fprintf(stderr, NEGRITA RED "%s→" name "(): " RESET RED "%s\n" RESET,  __func__, strerror(errno))

// Modo de creación de ficheros: (-rw-rw-rw-)
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/**
 * Monta el dispositivo virtual abriendo/creando un fichero
 *
 * @param camino ruta al dispositivo virtual
 * @return descriptor del fichero creado, FALLO si hay error
 */
int bmount(const char *camino) {
    int ret = FALLO; // Contiene el valor de retorno
    // Se cambia la mascara de creación de ficheros a 000 para que se permita qualquier tipo de modo
    // esto es necesario ya que en algunos sistemas la máscara por defecto = 0022, 
    // lo que significa que si creamos un fichero en modo 0666 se va a crear en modo:
    //      0666 & ~0022 = 0b110110110 & ~0b000010010
    //                   = 0b110110110 &  0b111101101
    //                   = 0b110100100
    //                   = 0644
    mode_t mask = umask(000);
    // Abrir/crear el fichero con los permisos por defecto (FILE_MODE)
    if ((fd = open(camino, O_RDWR | O_CREAT, FILE_MODE)) < 0) ERR_PRINT("open");
    else ret = fd;
    umask(mask); // Restaurar la antigua mascara de creación
    return ret;
}

/**
 * Desmonta el dispositivo virtual cerrando el fichero
 *
 * @return 0 si se ha desmontado correctamente el dispositivo virtual, FALLO en caso contrario
 */
int bumount() {
    int ret = 0;
    if (close(fd) < 0) {
        ERR_PRINT("close");
        ret = FALLO;
    }
    return ret;
}

/**
 * Escribe los datos almacenados en buffer de datos buf al bloque número nbloque
 *
 * @param nbloque número de bloque al que escribir
 * @param buf puntero al buffer de datos a escribir (debe ser un buffer de BLOCKSIZE bytes)
 * @return número de bytes escritos, FALLO en caso de error
 */
int bwrite(unsigned int nbloque, const void *buf) {
    // Desplaza el cursor del archivo hasta el primer byte del bloque especificado (nbloque ∗ BLOCKSIZE)
    if (lseek(fd, nbloque*BLOCKSIZE, SEEK_SET) < 0) {
        ERR_PRINT("lseek");
        return FALLO;
    }
    size_t nbytes = write(fd, buf, BLOCKSIZE);
    if (nbytes < 0) {
        ERR_PRINT("write");
        return FALLO;
    }
    return nbytes;
}

/**
 * Lee los datos almacenados en el bloque número nbloque al buffer de datos buf
 *
 * @param nbloque número de bloque al que leer
 * @param buf puntero al buffer de datos a leer (debe ser un buffer de BLOCKSIZE bytes)
 * @return número de bytes leídos, FALLO en caso de error
 */
int bread(unsigned int nbloque, void *buf) {
    // Mueve el cursor del fichero al primer byte del bloque indicado (nbloque * BLOCKSIZE)
    if (lseek(fd, nbloque*BLOCKSIZE, SEEK_SET) < 0) {
        ERR_PRINT("lseek");
        return FALLO;
    }
    size_t nbytes = read(fd, buf, BLOCKSIZE);
    if (nbytes < 0) {
        ERR_PRINT("read");
        return FALLO;
    }
    return nbytes;
}
