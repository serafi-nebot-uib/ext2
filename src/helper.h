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

void hexdump_raw(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start, const uint16_t col_cnt, const uint16_t col_sep, FILE *file);
void hexdump_col(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start, const uint16_t col_cnt, const uint16_t col_sep);
void hexdump(const void * const buff, size_t offset, const size_t size, const uint32_t addr_start);

#endif
