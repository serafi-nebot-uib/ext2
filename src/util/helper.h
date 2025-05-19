/**************************************************************************
* FILENAME: helper.h
* AUTHOR: Serafí Nebot, Ignasi Paredes, Jaume Galmés
**************************************************************************/

#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <time.h>

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void hexdump_raw(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start, const uint16_t col_cnt, const uint16_t col_sep, FILE *file);
void hexdump_col(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start, const uint16_t col_cnt, const uint16_t col_sep);
void hexdump(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start);
void timeval_fmt(const struct timeval *tv, char *buf, size_t buflen);

#endif
