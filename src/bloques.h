/**************************************************************************
* FILENAME: bloques.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __BLOQUES_H__
#define __BLOQUES_H__

#include <errno.h>    //errno
#include <fcntl.h>    //O_WRONLY, O_CREAT, O_TRUNC
#include <stdio.h>    //printf(), fprintf(), stderr, stdout, stdin
#include <stdlib.h>   //exit(), EXIT_SUCCESS, EXIT_FAILURE, atoi()
#include <string.h>   // strerror()
#include <sys/stat.h> //S_IRUSR, S_IWUSR
#include <unistd.h>   // SEEK_SET, read(), write(), open(), close(), lseek()

#include "logging.h"

#define BLOCKSIZE 1024 // bytes

#define EXITO   0
#define FALLO   -1

int bmount(const char *camino);
int bumount();
int bwrite(unsigned int nbloque, const void *buf);
int bread(unsigned int nbloque, void *buf);

#endif
