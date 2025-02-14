#include "bloques.h"

#include <string.h>
#include <errno.h>


static int fd = 0;
extern int errno;

#define ERRNO(name) { fprintf(stderr, BOLD RED name ": " RESET RED "%s\n", strerror(errno)); }

int bmount(const char *camino) {
    if ((fd = open(camino, O_RDWR | O_CREAT, 0666)) < 0) {
        ERRNO("open");
        return FALLO;
    }
    return fd;
}

int bumount() {
    if (close(fd) < 0) {
        ERRNO("close");
        return FALLO;
    }
    return 0;
}

int bwrite(unsigned int nbloque, const void *buf) {
    if (lseek(fd, nbloque*BLOCKSIZE, SEEK_SET) < 0) {
        ERRNO("lseek");
        return FALLO;
    }
    size_t nbytes = write(fd, buf, BLOCKSIZE);
    if (nbytes < 0) {
        ERRNO("write");
        return FALLO;
    }
    return nbytes;
}

int bread(unsigned int nbloque, void *buf) {
    if (lseek(fd, nbloque*BLOCKSIZE, SEEK_SET) < 0) {
        ERRNO("lseek");
        return FALLO;
    }
    size_t nbytes = read(fd, buf, BLOCKSIZE);
    if (nbytes < 0) {
        ERRNO("read");
        return FALLO;
    }
    return nbytes;
}
