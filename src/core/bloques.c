/**************************************************************************
* FILENAME: bloques.c
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#include "bloques.h"
#include <semaphore.h>
#include <sys/mman.h>

// descriptor del fichero actual
static sem_t *mutex;
static int fd;
static volatile size_t size = 0;
static void *volatile addr = NULL;

// modo de creación de ficheros: (-rw-rw-rw-)
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

static unsigned int inside_sc = 0;

void mi_waitSem() {
    if (!inside_sc) waitSem(mutex);
    inside_sc++;
}

void mi_signalSem() {
    inside_sc--;
    if (!inside_sc) signalSem(mutex);
}

/**
 * Montar el dispositivo virtual abriendo/creando un fichero
 *
 * @param camino ruta al dispositivo virtual
 * @return descriptor del fichero creado, FALLO si hay error
 */
int bmount(const char *camino) {
    if (!mutex) {  // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
        mutex = initSem();
        if (mutex == NULL) return FALLO;
    }

    // TODO: is there a better way to write this without using goto statements?

    // se cambia la máscara de creación de ficheros a 000 para que se permita qualquier tipo de modo
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
        goto fail;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        ERRSYS("fstat");
        goto fail;
    }

    size = st.st_size;
    if (size < BLOCKSIZE) {
        size = BLOCKSIZE;
        if (ftruncate(fd, size) < 0) {
            ERRSYS("ftruncate");
            if (close(fd) < 0) ERRSYS("close");
            goto fail;
        }
    }

    if ((addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0)) == MAP_FAILED) {
        ERRSYS("mmap");
        goto fail;
    }

    umask(mask); // restaurar la antigua mascara de creación
    return EXITO;

fail:
    umask(mask); // restaurar la antigua mascara de creación
    size = 0;
    addr = NULL;
    return FALLO;
}

/**
 * Desmontar el dispositivo virtual cerrando el fichero
 *
 * @return EXITO si se ha desmontado correctamente el dispositivo virtual, FALLO en caso contrario
 */
int bumount() {
    deleteSem();
    int ret = EXITO;

    if (munmap(addr, size) < 0) {
        ERRSYS("munmap");
        ret = FALLO;
    }

    if (close(fd)) {
        ERRSYS("close");
        ret = FALLO;
    }

    size = 0;
    addr = NULL;

    return ret;
}

/**
 * Redimensionar el dispositivo virtual para evitar múltiples redimensionamientos durante escrituras.
 * Recomendable llamar a esta función justo después de bmount cuando se crea un nuevo
 * sistema de ficheros y se sabe el numero de bloques total (sz = num_bloques_total * BLOCKSIZE)
 *
 * @param sz nueva medida del fichero en bytes
 * @return EXITO si se ha re-escalado 
 */
int resize(size_t sz) {
    if (munmap(addr, size) < 0) {
        ERRSYS("munmap");
        return FALLO;
    }

    size = sz;
    if (ftruncate(fd, size) < 0) {
        ERRSYS("ftruncate");
        return FALLO;
    }

    if ((addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0)) == MAP_FAILED) {
        ERRSYS("mmap");
        return FALLO;
    }

    return EXITO;
}

/**
 * Escribir los datos almacenados en buffer de datos buf al bloque número nbloque
 *
 * @param nbloque número de bloque al que escribir
 * @param buf puntero al buffer de datos a escribir (debe ser un buffer de BLOCKSIZE bytes)
 * @return número de bytes escritos, FALLO en caso de error
 */
int bwrite(unsigned int nbloque, const void *buf) {
    size_t off = nbloque * BLOCKSIZE;
    if (off + BLOCKSIZE > size) resize(size + BLOCKSIZE);
    memcpy(addr + off, buf, BLOCKSIZE);
    return BLOCKSIZE;
}

/**
 * Leer los datos almacenados en el bloque número nbloque al buffer de datos buf
 *
 * @param nbloque número de bloque al que leer
 * @param buf dirección donde se volcarán los datos leídos (debe ser un buffer de BLOCKSIZE bytes)
 * @return número de bytes leídos, FALLO en caso de error
 */
int bread(unsigned int nbloque, void *buf) {
    size_t off = nbloque * BLOCKSIZE;
    if (off + BLOCKSIZE > size) resize(size + BLOCKSIZE);
    memcpy(buf, addr + off, BLOCKSIZE);
    return BLOCKSIZE;
}
